/* reencode.c
 * - Functions for reencoding in ices
 * Copyright (c) 2000 Alexander Haväng
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
#include <lame.h>

extern ices_config_t ices_config;

static int ices_reencode_initialized = 0;
static lame_global_flags ices_reencode_flags;
static short int right[4096 * 30]; /* Probably overkill like hell, can someone calculate a better value please? */
static short int left[4096 * 30];


/* Global function definitions */
void
ices_reencode_initialize ()
{
	char version[21];

	if (!ices_config.reencode) 
		return;

	if (lame_init (&ices_reencode_flags) == -1) {
		ices_log ("Error: initialization of liblame failed!");
		ices_setup_shutdown ();
	}

	if (lame_decode_init () == -1) {
		ices_log ("Error: initialization of liblame's decoder failed!");
		ices_setup_shutdown ();
	}

	ices_reencode_flags.brate = ices_config.bitrate;

	if (lame_init_params (&ices_reencode_flags) == -1) {
		ices_log ("Error: Second stage init of lame failed!");
		ices_setup_shutdown ();
	}

	lame_version (&ices_reencode_flags, version);

	ices_log_debug ("Initializing lame version %s\n", version);
	ices_reencode_initialized = 0;
}

void
ices_reencode_set_channels (int channels)
{
	ices_reencode_flags.num_channels = channels;
}

void
ices_reencode_set_sample_rate (int samplerate)
{
	ices_reencode_flags.in_samplerate = samplerate;
}

void
ices_reencode_set_mode (int mode)
{
	ices_reencode_flags.mode = mode;
}

void
ices_reencode_reset () 
{
	if (lame_init (&ices_reencode_flags) == -1) {
		ices_log ("Error: initialization of liblame failed!");
		ices_setup_shutdown ();
	}

	if (lame_decode_init () == -1) {
		ices_log ("Error: initialization of liblame's decoder failed!");
		ices_setup_shutdown ();
	}

	ices_reencode_flags.brate = ices_config.bitrate;

	if (lame_init_params (&ices_reencode_flags) == -1) {
		ices_log ("Error: lame_init_params() failed!");
	}
}

void
ices_reencode_shutdown ()
{
	if (!ices_config.reencode || ices_reencode_initialized == 0)
		return;
}

int
ices_reencode_reencode_chunk (unsigned char *buff, int buflen, unsigned char *outbuf, int outlen)
{
	int decode_ret = lame_decode ((char *)buff, buflen, left, right);
	
	if (decode_ret > 0)
		return lame_encode_buffer (&ices_reencode_flags, left, right, decode_ret, (char *)outbuf, outlen);
	return decode_ret;
}

int
ices_reencode_flush (unsigned char *outbuf, int maxlen)
{
	return lame_encode_finish (&ices_reencode_flags, (char *)outbuf, maxlen);
}
#endif
