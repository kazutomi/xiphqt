/* mp3.c
 * - Functions for mp3 in ices
 * Copyright (c) 2000 Alexander Haväng
 * Copyright (c) 2001-3 Brendan Cully
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
 * $Id: mp3.c,v 1.25 2003/03/15 20:58:54 brendan Exp $
 */

#include "definitions.h"

/* reference: http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm */

#define MPG_MD_MONO 3

#define MP3_BUFFER_SIZE 4096

typedef struct {
  int version;
  int layer;
  int error_protection;
  int bitrate;
  int samplerate;
  int padding;
  int extension;
  int mode;
  int mode_ext;
  int copyright;
  int original;
  int emphasis;
  int channels;
} mp3_header_t;

typedef struct {
  unsigned char* buf;
  size_t len;
  int pos;
} ices_mp3_in_t;

static unsigned int bitrates[2][3][15] =
{
  /* MPEG-1 */
  {
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
  },
  /* MPEG-2 LSF, MPEG-2.5 */
  {
    {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
  },
};

static unsigned int s_freq[3][4] =
{
  {44100, 48000, 32000, 0},
  {22050, 24000, 16000, 0},
  {11025, 8000, 8000, 0}
};

static char *mode_names[5] = {"stereo", "j-stereo", "dual-ch", "mono", "multi-ch"};
static char *layer_names[3] = {"I", "II", "III"};
static char *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};

/* -- static prototypes -- */
static int ices_mp3_parse (input_stream_t* source);
static ssize_t ices_mp3_read (input_stream_t* self, void* buf, size_t len);
#ifdef HAVE_LIBLAME
static ssize_t ices_mp3_readpcm (input_stream_t* self, size_t len,
				 int16_t* left, int16_t* right);
#endif
static int ices_mp3_close (input_stream_t* self);
static int mp3_parse_frame(const unsigned char* buf, mp3_header_t* header);
static int mp3_check_vbr(input_stream_t* source, mp3_header_t* header);
static size_t mp3_frame_length(mp3_header_t* header);

/* Global function definitions */

/* Parse mp3 file for header information; bitrate, channels, mode, sample_rate.
 */
static int ices_mp3_parse (input_stream_t* source)
{
  ices_mp3_in_t* mp3_data = (ices_mp3_in_t*) source->data;
  unsigned char *buffer;
  mp3_header_t mh;
  size_t len;
  int rc = 0;
  int off = 0;

  if (mp3_data->len < 4)
    return 1;

  /* Ogg/Vorbis often contains bits that almost look like MP3 headers */
  if (! strncmp ("OggS", mp3_data->buf, 4))
    return 1;

  /* first check for ID3v2 */
  if (! strncmp ("ID3", mp3_data->buf, 3))
    ices_id3v2_parse (source);

  /* refill buffer if ID3v2 parsing consumed it */
  if (!mp3_data->buf) {
    buffer = (unsigned char*) malloc (MP3_BUFFER_SIZE);
    len = source->read (source, buffer, MP3_BUFFER_SIZE);
    mp3_data->buf = buffer;
    mp3_data->len = len;
    mp3_data->pos = 0;
  }

  if (mp3_data->len < 4) {
    ices_log_error("Short file");
    return  -1;
  }

  /* seek past garbage if necessary */
  buffer = mp3_data->buf;
  do {
    len = mp3_data->len - mp3_data->pos;

    /* copy remaining bytes to front, refill buffer without malloc/free */
    if (len < 4) {
      memcpy (buffer, buffer + mp3_data->pos, len);
      /* make read fetch from source instead of buffer */
      mp3_data->buf = NULL;
      len += source->read(source, buffer + len, mp3_data->len - len);
      mp3_data->buf = buffer;
      mp3_data->pos = 0;
      if (len < 4)
        break;
    }

    /* we must be able to read at least 4 bytes of header */
    while (mp3_data->len - mp3_data->pos >= 4
        && !(rc = mp3_parse_frame(buffer + mp3_data->pos, &mh))) {
      mp3_data->pos++;
      off++;
    }
  } while (!rc);

  if (!rc) {
    ices_log_error ("Couldn't find synch");
    return -1;
  }

  if (off)
    ices_log_debug("Skipped %d bytes of garbage before MP3", off);

  source->samplerate = mh.samplerate;
  source->bitrate = mh.bitrate;

  /* bitrate of zero ensures that lazy reencoding won't be lazy */
  if (mp3_check_vbr(source, &mh) > 0)
    source->bitrate = 0;

  if (source->bitrate)
    ices_log_debug ("%s layer %s, %d kbps, %d Hz, %s", version_names[mh.version],
                    layer_names[mh.layer - 1], mh.bitrate, mh.samplerate, mode_names[mh.mode]);
  else
    ices_log_debug ("%s layer %s, VBR, %d Hz, %s", version_names[mh.version],
                    layer_names[mh.layer - 1], mh.samplerate, mode_names[mh.mode]);
  ices_log_debug ("Ext: %d\tMode_Ext: %d\tCopyright: %d\tOriginal: %d", mh.extension,
                  mh.mode_ext, mh.copyright, mh.original);
  ices_log_debug ("Error Protection: %d\tEmphasis: %d\tPadding: %d", mh.error_protection,
                  mh.emphasis, mh.padding);

  return 0;
}

int
ices_mp3_open (input_stream_t* self, const char* buf, size_t len)
{
  ices_mp3_in_t* mp3_data;
  int rc;

  if (!len)
    return -1;

  if (! ((mp3_data = (ices_mp3_in_t*) malloc (sizeof (ices_mp3_in_t))) &&
	 (mp3_data->buf = (unsigned char*) malloc (len))))
  {
    ices_log_error ("Malloc failed in ices_mp3_open");
    return -1;
  }

  memcpy (mp3_data->buf, buf, len);
  mp3_data->len = len;
  mp3_data->pos = 0;

  self->type = ICES_INPUT_MP3;
  self->data = mp3_data;

  self->read = ices_mp3_read;
#ifdef HAVE_LIBLAME
  self->readpcm = ices_mp3_readpcm;
#else
  self->readpcm = NULL;
#endif
  self->close = ices_mp3_close;

  ices_id3v1_parse (self);

  if ((rc = ices_mp3_parse (self))) {
    free (mp3_data->buf);
    free (mp3_data);
    return rc;
  }

  return 0;
}

/* input_stream_t wrapper for fread */
static ssize_t
ices_mp3_read (input_stream_t* self, void* buf, size_t len)
{
  ices_mp3_in_t* mp3_data = self->data;
  int remaining;
  int rlen = 0;

  if (mp3_data->buf) {
    remaining = mp3_data->len - mp3_data->pos;
    if (remaining > len) {
      rlen = len;
      memcpy (buf, mp3_data->buf + mp3_data->pos, len);
      mp3_data->pos += len;
    } else {
      rlen = remaining;
      memcpy (buf, mp3_data->buf + mp3_data->pos, remaining);
      free (mp3_data->buf);
      mp3_data->buf = NULL;
    }
  } else {
    /* we don't just use EOF because we'd like to avoid the ID3 tag */
    if (self->canseek && self->filesize - self->bytes_read < len)
      len = self->filesize - self->bytes_read;
    if (len)
      rlen = read (self->fd, buf, len);
  }

  self->bytes_read += rlen;

  return rlen;
}

#ifdef HAVE_LIBLAME
static ssize_t
ices_mp3_readpcm (input_stream_t* self, size_t len, int16_t* left,
		  int16_t* right)
{
  unsigned char buf[MP3_BUFFER_SIZE];
  ssize_t rlen;
  int nsamples = 0;

  while (! nsamples) {
    if ((rlen = self->read (self, buf, sizeof (buf))) <= 0)
      return rlen;

    nsamples = ices_reencode_decode (buf, rlen, len, left, right);
  }

  return nsamples;
}
#endif

static int
ices_mp3_close (input_stream_t* self)
{
  ices_mp3_in_t* mp3_data = (ices_mp3_in_t*) self->data;

  ices_util_free (mp3_data->buf);
  free (self->data);

  return close (self->fd);
}

static int mp3_parse_frame(const unsigned char* buf, mp3_header_t* header) {
  int bitrate_idx, samplerate_idx;

  if (((buf[0] << 4) | ((buf[1] >> 4) & 0xE)) != 0xFFE)
    return 0;

  switch ((buf[1] >> 3 & 0x3)) {
    case 3:
      header->version = 0;
      break;
    case 2:
      header->version = 1;
      break;
    case 0:
      header->version = 2;
      break;
    default:
      return 0;
  }

  bitrate_idx = (buf[2] >> 4) & 0xF;
  samplerate_idx = (buf[2] >> 2) & 0x3;
  header->mode = (buf[3] >> 6) & 0x3;
  header->layer = 4 - ((buf[1] >> 1) & 0x3);
  header->emphasis = (buf[3]) & 0x3;

  if ((bitrate_idx == 0xF) || (samplerate_idx == 0x3)
      || (header->layer == 4) || (header->emphasis == 2))
    return 0;

  header->error_protection = !(buf[1] & 0x1);
  if (header->version == 0)
    header->bitrate = bitrates[0][header->layer-1][bitrate_idx];
  else
    header->bitrate = bitrates[1][header->layer-1][bitrate_idx];
  header->samplerate = s_freq[header->version][samplerate_idx];
  header->padding = (buf[2] >> 1) & 0x01;
  header->extension = buf[2] & 0x01;
  header->mode_ext = (buf[3] >> 4) & 0x03;
  header->copyright = (buf[3] >> 3) & 0x01;
  header->original = (buf[3] >> 2) & 0x1;
  header->channels = (header->mode == MPG_MD_MONO) ? 1 : 2;

  return 1;
}

/* TODO: check without seeking */
static int mp3_check_vbr(input_stream_t* source, mp3_header_t* header) {
  ices_mp3_in_t* mp3_data = (ices_mp3_in_t*) source->data;
  char buf[4];
  mp3_header_t next_header;
  ssize_t framelen;
  off_t cur;
  int offset;

  if (!source->canseek)
    return -1;

  cur = lseek(source->fd, 0, SEEK_CUR);
  /* check for VBR tag */
  /* Tag offset varies (but FhG VBRI is always MPEG1 Layer III 160 kbps stereo) */
  if (header->version == 0) {
    if (header->channels == 1)
      offset = 21;
    else
      offset = 36;
  } else {
    if (header->channels == 1)
      offset = 13;
    else
      offset = 21;
  }

  offset -= mp3_data->len - mp3_data->pos;
  lseek(source->fd, offset, SEEK_CUR);
  if (read(source->fd, buf, 4) == 4) {
    if (!strncmp("VBRI", buf, 4) || !strncmp("Xing", buf, 4)) {
      ices_log_debug("VBR tag found");
      lseek(source->fd, cur, SEEK_SET);
      return 1;
    }
  } else {
    ices_log_debug("Error trying to read VBR tag");
    lseek(source->fd, cur, SEEK_SET);
    return -1;
  }

  /* otherwise check next frame if possible */
  offset = mp3_data->len - mp3_data->pos;
  lseek(source->fd, cur, SEEK_SET);
  if ((framelen = mp3_frame_length(header))) {
    ices_log_debug("Frame length expected: %d bytes", framelen);
    lseek(source->fd, framelen - offset, SEEK_CUR);
    if (read(source->fd, buf, 4) == 4) {
      if (!mp3_parse_frame(buf, &next_header))
        ices_log_debug("Couldn't find second frame header");
      else {
        ices_log_debug("Second frame: %s layer %s, %d kbps, %d Hz", 
                       version_names[next_header.version],
                       layer_names[next_header.layer - 1], next_header.bitrate,
	               next_header.samplerate);
        /* if bit rates don't match assume VBR */
        if (header->bitrate != next_header.bitrate)
	  return 1;
      }
    } else
      ices_log_debug("Error reading next frame");
    lseek(source->fd, cur, SEEK_SET);
  }
  
  return 0;
}

/* Calculate the expected length of the next frame, or return 0 if we don't know how */
static size_t mp3_frame_length(mp3_header_t* header) {
  if (!header->bitrate)
    return 0;

  if (header->layer == 1) {
    return (12000 * header->bitrate / header->samplerate + header->padding) * 4;
  } else {
    return 144000 * header->bitrate / header->samplerate + header->padding;
  }

  return 0;
}
