/* mp3.c
 * - Functions for mp3 in ices
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

#define MPG_MD_MONO 3

typedef struct mp3_headerSt
{
	int lay;
	int version;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int stereo;
} mp3_header_t;

typedef struct {
  unsigned char* buf;
  size_t len;
  int pos;
} ices_mp3_in_t;

static unsigned int bitrates[3][3][15] =
{
	{
		{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
		{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
	},
	{
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	},
	{
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	}
};

static unsigned int s_freq[3][4] =
{
	{44100, 48000, 32000, 0},
	{22050, 24000, 16000, 0},
	{11025, 8000, 8000, 0}
};

static char *mode_names[5] = {"stereo", "j-stereo", "dual-ch", "single-ch", "multi-ch"};
static char *layer_names[3] = {"I", "II", "III"};
static char *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};

static int ices_mp3_bitrate = -1;
static int ices_mp3_sample_rate = -1;
static int ices_mp3_mode = -1;
static int ices_mp3_channels = -1;

/* -- static prototypes -- */
static int ices_mp3_parse (input_stream_t* source);
static ssize_t ices_mp3_read (input_stream_t* self, void* buf, size_t len);
#ifdef HAVE_LIBLAME
static ssize_t ices_mp3_readpcm (input_stream_t* self, size_t len,
				 int16_t* left, int16_t* right);
#endif
static int ices_mp3_close (input_stream_t* self);

/* Return the current song's bitrate */
int
ices_mp3_get_bitrate (void)
{
	return ices_mp3_bitrate;
}

/* Return the current song's sample rate */
int
ices_mp3_get_sample_rate (void)
{
	return ices_mp3_sample_rate;
}

/* Return the current song's mode */
int
ices_mp3_get_mode (void)
{
	return ices_mp3_mode;
}

/* return the number of channels in current song */
int
ices_mp3_get_channels (void)
{
	return ices_mp3_channels;
}

/* Global function definitions */

/* Parse mp3 file for header information; bitrate, channels, mode, sample_rate.
 */
int
ices_mp3_parse (input_stream_t* source)
{
  ices_mp3_in_t* mp3_data = (ices_mp3_in_t*) source->data;
  unsigned char *buffer;
  unsigned short temp;
  mp3_header_t mh;
  size_t len;
  int rc = 1;

  if (! mp3_data->len)
    return 1;

  /* Ogg/Vorbis often contains bits that almost look like MP3 headers */
  if (! strncmp ("OggS", mp3_data->buf, 4))
    return 1;

  /* first check for ID3v2 */
  if (! strncmp ("ID3", mp3_data->buf, 3)) {
    ices_id3v2_parse (source);
    /* We're committed now. If we don't find an MP3 header the file is
     * garbage */
    rc = -1;
  }

  len = mp3_data->len;
  buffer = mp3_data->buf + mp3_data->pos;

  /* ID3v2 may have consumed the buffer */
  if (! len) {
    buffer = (unsigned char*) malloc (1024);
    len = source->read (source, buffer, 1024);
    mp3_data->buf = buffer;
    mp3_data->len = len;
    mp3_data->pos = 0;
  }

  do {
    temp = ((buffer[0] << 4) & 0xFF0) | ((buffer[1] >> 4) & 0xE);
    buffer++;
    len--;
  } while ((temp != 0xFFE) && (len-4 > 0));

  if (temp != 0xFFE) {
    ices_log_error ("Couldn't find synch");
    return rc;
  }

  buffer--;
  switch ((buffer[1] >> 3 & 0x3)) {
    case 3:
      mh.version = 0;
      break;
    case 2:
      mh.version = 1;
      break;
    case 0:
      mh.version = 2;
      break;
    default:
      return rc;
      break;
  }

  mh.lay = 4 - ((buffer[1] >> 1) & 0x3);
  mh.error_protection = !(buffer[1] & 0x1);
  mh.bitrate_index = (buffer[2] >> 4) & 0x0F;
  mh.sampling_frequency = (buffer[2] >> 2) & 0x3;
  mh.padding = (buffer[2] >> 1) & 0x01;
  mh.extension = buffer[2] & 0x01;
  mh.mode = (buffer[3] >> 6) & 0x3;
  mh.mode_ext = (buffer[3] >> 4) & 0x03;
  mh.copyright = (buffer[3] >> 3) & 0x01;
  mh.original = (buffer[3] >> 2) & 0x1;
  mh.emphasis = (buffer[3]) & 0x3;
  mh.stereo = (mh.mode == MPG_MD_MONO) ? 1 : 2;

  /* sanity check */
  ices_mp3_bitrate = bitrates[mh.version][mh.lay -1][mh.bitrate_index];
  if (! ices_mp3_bitrate) {
    ices_log_error ("Bitrate is 0");
    return rc;
  }

  ices_log_debug ("Layer: %s\t\tVersion: %s\tFrequency: %d", layer_names[mh.lay - 1], version_names[mh.version], s_freq[mh.version][mh.sampling_frequency]);
  ices_log_debug ("Bitrate: %d kbit/s\tPadding: %d\tMode: %s", ices_mp3_bitrate, mh.padding, mode_names[mh.mode]);
  ices_log_debug ("Ext: %d\tMode_Ext: %d\tCopyright: %d\tOriginal: %d", mh.extension, mh.mode_ext, mh.copyright, mh.original);
  ices_log_debug ("Error Protection: %d\tEmphasis: %d\tStereo: %d", mh.error_protection, mh.emphasis, mh.stereo);

  ices_mp3_mode = mh.mode;
  ices_mp3_sample_rate = s_freq[mh.version][mh.sampling_frequency];
  if (mh.mode == 3)
    ices_mp3_channels = 1;
  else
    ices_mp3_channels = 2;

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

  self->bitrate = ices_mp3_get_bitrate ();

  return 0;
}

/* input_stream_t wrapper for fread */
static ssize_t
ices_mp3_read (input_stream_t* self, void* buf, size_t len)
{
  ices_mp3_in_t* mp3_data = self->data;
  int rlen = 0;

  if (mp3_data->len) {
    if (mp3_data->len > len) {
      rlen = len;
      memcpy (buf, mp3_data->buf + mp3_data->pos, len);
      mp3_data->len -= len;
      mp3_data->pos += len;
    } else {
      rlen = mp3_data->len;
      memcpy (buf, mp3_data->buf, mp3_data->len);
      mp3_data->len = 0;
      mp3_data->pos = 0;
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
  unsigned char buf[4096];
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
