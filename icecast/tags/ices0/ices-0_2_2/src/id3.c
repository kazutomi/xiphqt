/* id3.c
 * - Functions for id3 tags in ices
 * Copyright (c) 2000 Alexander Haväng
 * Copyright (c) 2001 Brendan Cully
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

#include <thread.h>

extern ices_config_t ices_config;

/* Private function declarations */
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
  int taglen;
  ssize_t rlen;

  if (source->read (source, buf, 10) != 10)
  {
    ices_log ("Error reading ID3v1");
    return;
  }

  taglen = id3v2_decode_synchsafe (buf + 6);
  ices_log_debug ("ID3v2 tag size is %d bytes", taglen);

  ices_log_debug ("Skipping to MP3 data");
  while (taglen > 0) {
    rlen = taglen > sizeof (buf) ? sizeof (buf) : taglen;
    if ((rlen = source->read (source, buf, rlen)) < 0) {
      ices_log ("Error reading ID3v2 tag");
      return;
    }
    taglen -= rlen;
  }
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
