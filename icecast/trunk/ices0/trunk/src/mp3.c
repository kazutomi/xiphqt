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
  unsigned char buf[4096];
  ssize_t len;
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
static int ices_mp3_parse_file (const char* buf, size_t len);
static ssize_t ices_mp3_read (input_stream_t* self, void* buf, size_t len);
static ssize_t ices_mp3_readpcm (input_stream_t* self, size_t len,
				 int16_t* left, int16_t* right);
static int ices_mp3_close (input_stream_t* self);
static int ices_mp3_get_metadata (input_stream_t* self, char* buf, size_t len);

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
ices_mp3_parse_file (const char *buf, size_t len)
{
  const unsigned char *buffer;
  unsigned short temp;
  mp3_header_t mh;
	
  len -= 4;
  buffer = buf;

  /* Ogg/Vorbis often contains bits that almost look like MP3 headers */
  if (! strncmp ("OggS", buf, 4))
    return -1;

  do {
    temp = ((buffer[0] << 4) & 0xFF0) | ((buffer[1] >> 4) & 0xE);
    buffer++;
    len--;
  } while ((temp != 0xFFE) && (len > 0));

  if (temp != 0xFFE)
    return -1;

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
      return -1;
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
  if (! ices_mp3_bitrate)
    return -1;

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

  if (!len || ices_mp3_parse_file (buf, len) < 0)
    return 1;

  self->bitrate = ices_mp3_get_bitrate ();

  if (! (mp3_data = (ices_mp3_in_t*) malloc (sizeof (ices_mp3_in_t)))) {
    ices_log_error ("Malloc failed in ices_mp3_open");
    return -1;
  }

  memcpy (mp3_data->buf, buf, len);
  mp3_data->len = len;

  self->type = ICES_INPUT_MP3;
  self->data = mp3_data;

  self->read = ices_mp3_read;
#ifdef HAVE_LIBLAME
  self->readpcm = ices_mp3_readpcm;
#endif
  self->get_metadata = ices_mp3_get_metadata;
  self->close = ices_mp3_close;

  return 0;
}

/* input_stream_t wrapper for fread */
static ssize_t
ices_mp3_read (input_stream_t* self, void* buf, size_t len)
{
  ices_mp3_in_t* mp3_data = self->data;
  int rlen;

  if (mp3_data->len) {
    if (mp3_data->len > len) {
      rlen = len;
      memcpy (buf, mp3_data->buf, len);
      mp3_data->len -= len;
    } else {
      rlen = mp3_data->len;
      memcpy (buf, mp3_data->buf, mp3_data->len);
      mp3_data->len = 0;
    }

    return rlen;
  }

  return read (self->fd, buf, len);
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
  free (self->data);

  return close (self->fd);
}

static int
ices_mp3_get_metadata (input_stream_t* self, char* buf, size_t len)
{
  char artist[1024];
  char title[1024];

  if (! self->canseek)
    return -1;

  ices_id3_parse_file (self->path, 0);
  ices_id3_get_artist (artist, sizeof (artist));
  ices_id3_get_title (title, sizeof (title));

  if (! *title)
    return -1;

  if (*artist)
    snprintf (buf, len, "%s - %s", artist, title);
  else
    snprintf (buf, len, "%s", artist);

  return 0;
}
