/* reencode.c
 * - Functions for reencoding in ices
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

#ifdef HAVE_LIBLAME

#ifdef HAVE_LAME_LAME_H
# include <lame/lame.h>
#else
# include <lame.h>
#endif

extern ices_config_t ices_config;

static int ices_reencode_initialized = 0;
static lame_global_flags* ices_reencode_flags;
static short int right[4096 * 30]; /* Probably overkill like hell, can someone calculate a better value please? */
static short int left[4096 * 30];


/* Global function definitions */

/* Initialize the reencoding engine in ices, initialize
 * the liblame structures and be happy */
void
ices_reencode_initialize (void)
{
	if (!ices_config.reencode) 
		return;

	if (! (ices_reencode_flags = lame_init ())) {
		ices_log ("Error: initialization of liblame failed!");
		ices_setup_shutdown ();
	}

	lame_set_brate (ices_reencode_flags, ices_config.bitrate);
	if (ices_config.out_numchannels != -1)
		lame_set_num_channels (ices_reencode_flags, ices_config.out_numchannels);
	if (ices_config.out_samplerate != -1)
		lame_set_out_samplerate (ices_reencode_flags, ices_config.out_samplerate);

	if (lame_init_params (ices_reencode_flags) == -1) {
		ices_log ("Error: lame_init_params() failed!");
	}

	ices_log_debug ("Initializing lame version %s\n", get_lame_version ());
	ices_reencode_initialized = 1;
}

/* Tell the reencoder what number of channels the current song has */
void
ices_reencode_set_channels (int channels)
{
	ices_reencode_flags->num_channels = channels;
}

/* Tell the reencoder what sample rate the current song is */
void
ices_reencode_set_sample_rate (int samplerate)
{
	ices_reencode_flags->in_samplerate = samplerate;
}

/* What mode is the current song in? */
void
ices_reencode_set_mode (int mode)
{
	ices_reencode_flags->mode = mode;
}

/* For each song, reset the liblame engine, otherwize it craps out if
 * the bitrate or sample rate changes */
void
ices_reencode_reset (void) 
{
	if (lame_decode_init () == -1) {
		ices_log ("Error: initialization of liblame's decoder failed!");
		ices_setup_shutdown ();
	}
}

/* If initialized, shutdown the reencoding engine */
void
ices_reencode_shutdown (void)
{
  if (ices_reencode_initialized)
    lame_close (ices_reencode_flags);
}

/* reencode buff, of len buflen, put max outlen reencoded bytes in outbuf */
int
ices_reencode_reencode_chunk (unsigned char *buff, int buflen, unsigned char *outbuf, int outlen)
{
	int decode_ret = lame_decode ((char *)buff, buflen, left, right);

	if (decode_ret > 0)
		return lame_encode_buffer (ices_reencode_flags, left, right, decode_ret, (char *)outbuf, outlen);
	return decode_ret;
}

/* At EOF of each file, flush the liblame buffers and get some extra candy */
int
ices_reencode_flush (unsigned char *outbuf, int maxlen)
{
	int ret = lame_encode_flush_nogap (ices_reencode_flags, (char *)outbuf, maxlen);
	return ret;
}
#endif
