/* id3.c
 * - Functions for id3 tags in ices
 * Copyright (c) 2000 Alexander Haväng
 * Copyright (c) 2001-3 Brendan Cully <brendan@icecast.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "definitions.h"
#include "metadata.h"

/* Local definitions */
#define ID3V2_FLAG_UNSYNC (1<<7)
#define ID3V2_FLAG_EXTHDR (1<<6)
#define ID3V2_FLAG_EXPHDR (1<<5)
#define ID3V2_FLAG_FOOTER (1<<4)

typedef struct {
  unsigned char major_version;
  unsigned char minor_version;
  unsigned char flags;
  size_t len;

  char* artist;
  char* title;

  unsigned int pos;
} id3v2_tag;

/* Private function declarations */
static int id3v2_read_exthdr (input_stream_t* source, id3v2_tag* tag);
ssize_t id3v2_read_frame (input_stream_t* source, id3v2_tag* tag);
static int id3v2_skip_data (input_stream_t* source, id3v2_tag* tag, size_t len);
static int id3v2_decode_synchsafe (unsigned char* synchsafe);

/* Global function definitions */

void
ices_id3v1_parse (input_stream_t* source)
{
  off_t pos;
  char tag[3];
  char song_name[31];
  char artist[31];
  char namespace[1024];

  if (! source->canseek)
    return;

  if ((pos = lseek (source->fd, 0, SEEK_CUR)) == -1) {
    ices_log ("Error seeking for ID3v1: %s",
	      ices_util_strerror (errno, namespace, 1024));
    return;
  }
  
  if (lseek (source->fd, -128, SEEK_END) == -1) {
    ices_log ("Error seeking for ID3v1: %s",
	      ices_util_strerror (errno, namespace, 1024));
    return;
  }

  memset (song_name, 0, 31);
  memset (artist, 0, 31);

  if ((read (source->fd, tag, 3) == 3) && !strncmp (tag, "TAG", 3)) {
    /* Don't stream the tag */
    source->filesize -= 128;

    if (read (source->fd, song_name, 30) != 30) {
      ices_log ("Error reading ID3v1 song title");
      goto out;
    }

    while (song_name[strlen (song_name) - 1] == ' ')
      song_name[strlen (song_name) - 1] = '\0';
    ices_log_debug ("ID3v1 song: %s", song_name);

    if (read (source->fd, artist, 30) != 30) {
      ices_log ("Error reading ID3v1 artist");
      goto out;
    }

    while (artist[strlen (artist) - 1] == '\040')
      artist[strlen (artist) - 1] = '\0';
    ices_log_debug ("ID3v1 artist: %s", artist);

    ices_metadata_set (artist, song_name);
  }
  
out:
  lseek (source->fd, pos, SEEK_SET);
}

void
ices_id3v2_parse (input_stream_t* source)
{
  unsigned char buf[1024];
  id3v2_tag tag;
  size_t remaining;
  ssize_t rv;

  if (source->read (source, buf, 10) != 10) {
    ices_log ("Error reading ID3v2");

    return;
  }

  tag.artist = tag.title = NULL;
  tag.pos = 0;
  tag.major_version = *(buf + 3);
  tag.minor_version = *(buf + 4);
  tag.flags = *(buf + 5);
  tag.len = id3v2_decode_synchsafe (buf + 6);
  ices_log_debug ("ID3v2: version %d.%d. Tag size is %d bytes.",
                  tag.major_version, tag.minor_version, tag.len);

  if ((tag.flags & ID3V2_FLAG_EXTHDR) && id3v2_read_exthdr (source, &tag) < 0) {
    ices_log ("Error reading ID3v2 extended header");

    return;
  }

  remaining = tag.len - tag.pos;
  if (tag.flags & ID3V2_FLAG_FOOTER)
    remaining -= 10;

  while (remaining > 10 && (tag.artist == NULL || tag.title == NULL)) {
    if ((rv = id3v2_read_frame (source, &tag)) < 0) {
      ices_log ("Error reading ID3v2 frames");

      return;
    }
    /* found padding */
    if (rv == 0)
      break;

    remaining -= rv;
  }

  ices_metadata_set (tag.artist, tag.title);

  remaining = tag.len - tag.pos;
  if (remaining) {
    ices_log_debug ("Skipping %d bytes to end of tag", remaining);

    id3v2_skip_data (source, &tag, remaining);
  }
}

static int
id3v2_read_exthdr (input_stream_t* source, id3v2_tag* tag)
{
  char hdr[6];
  size_t len;

  if (source->read (source, hdr, 6) != 6) {
    ices_log ("Error reading ID3v2 extended header");

    return -1;
  }
  tag->pos += 6;

  len = id3v2_decode_synchsafe (hdr);
  ices_log_debug ("ID3v2: %d byte extended header found, skipping.", len);

  if (len > 6)
    return id3v2_skip_data (source, tag, len - 6);
  else
    return 0;
}

ssize_t
id3v2_read_frame (input_stream_t* source, id3v2_tag* tag)
{
  char hdr[10];
  size_t len;
  char* buf;

  if (source->read (source, hdr, 10) != 10) {
    ices_log ("Error reading ID3v2 frame");

    return -1;
  }
  tag->pos += 10;

  if (hdr[0] == '\0')
    return 0;

  len = id3v2_decode_synchsafe (hdr + 4);
  hdr[4] = '\0';

  ices_log_debug("ID3v2: Frame type [%s] found, %d bytes", hdr, len);
  if (!strcmp (hdr, "TIT2") || !strcmp (hdr, "TPE1")) {
    if (! (buf = malloc(len))) {
      ices_log ("Error allocating memory while reading ID3v2 frame");
      
      return -1;
    }
    if (source->read (source, buf, len) < 0) {
      ices_log ("Error reading ID3v2 frame data");
      
      return -1;
    }
    tag->pos += len;

    /* skip encoding */
    if (!strcmp (hdr, "TIT2")) {
      ices_log_debug ("ID3v2: Title found: %s", buf + 1);
      tag->title = buf + 1;
    } else {
      ices_log_debug ("ID3v2: Artist found: %s", buf + 1);
      tag->artist = buf + 1;
    }
  } else if (id3v2_skip_data (source, tag, len))
    return -1;

  return len + 10;
}

static int
id3v2_skip_data (input_stream_t* source, id3v2_tag* tag, size_t len)
{
  char* buf;
  ssize_t rlen;

  if (! (buf = malloc(len))) {
    ices_log ("Error allocating memory while skipping ID3v2 data");
    
    return -1;
  }

  while (len) {
    if ((rlen = source->read (source, buf, len)) < 0) {
      ices_log ("Error skipping in ID3v2 tag.");
      free (buf);

      return -1;
    }
    tag->pos += rlen;
    len -= rlen;
  }

  free (buf);

  return 0;
}

static int
id3v2_decode_synchsafe (unsigned char* synchsafe)
{
  int res;

  res = synchsafe[3];
  res |= synchsafe[2] << 7;
  res |= synchsafe[1] << 14;
  res |= synchsafe[0] << 21;

  return res;
}
