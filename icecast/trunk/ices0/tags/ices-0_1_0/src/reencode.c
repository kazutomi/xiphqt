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

#ifdef HAVE_LAME_LAME_H
# include <lame/lame.h>
#else
# include <lame.h>
#endif

extern ices_config_t ices_config;

static int reencode_lame_allocated = 0;
static lame_global_flags* ices_reencode_flags;
static short int right[4096 * 30]; /* Probably overkill like hell, can someone calculate a better value please? */
static short int left[4096 * 30];

/* -- local prototypes -- */
static void reencode_lame_init (void);

/* Global function definitions */

/* Initialize the reencoding engine in ices, initialize
 * the liblame structures and be happy */
void
ices_reencode_initialize (void)
{
  if (!ices_config.reencode) 
    return;

#ifdef HAVE_LAME_NOGAP
  reencode_lame_init ();
#endif

  ices_log_debug ("Using LAME version %s\n", get_lame_version ());
}

/* For each song, reset the liblame engine, otherwize it craps out if
 * the bitrate or sample rate changes */
void
ices_reencode_reset (void) 
{
#ifndef HAVE_LAME_NOGAP  
  reencode_lame_init ();
#endif

  if (lame_decode_init () == -1) {
    ices_log ("Error: initialization of liblame's decoder failed!");
    ices_setup_shutdown ();
  }
}

/* If initialized, shutdown the reencoding engine */
void
ices_reencode_shutdown (void)
{
  if (reencode_lame_allocated)
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
  int ret;

#ifdef HAVE_LAME_NOGAP
  ret = lame_encode_flush_nogap (ices_reencode_flags, (char *)outbuf, maxlen);
#else
  ret = lame_encode_flush (ices_reencode_flags, (char*) outbuf, maxlen);
  lame_close (ices_reencode_flags);
  reencode_lame_allocated = 0;
#endif

  return ret;
}

/* Resets the lame engine. Depending on which version of LAME we have, we must
 * do this either only at startup or between each song */
static void
reencode_lame_init ()
{
  if (! (ices_reencode_flags = lame_init ())) {
    ices_log ("Error initialising LAME.");
    ices_setup_shutdown ();
  }

  /* not all of these functions were implemented by LAME 3.88 */
#ifdef HAVE_LAME_NOGAP
  lame_set_brate (ices_reencode_flags, ices_config.bitrate);
  if (ices_config.out_numchannels != -1)
    lame_set_num_channels (ices_reencode_flags, ices_config.out_numchannels);
  if (ices_config.out_samplerate != -1)
    lame_set_out_samplerate (ices_reencode_flags, ices_config.out_samplerate);
#else
  ices_reencode_flags->brate = ices_config.bitrate;
  if (ices_config.out_numchannels != -1)
    ices_reencode_flags->num_channels = ices_config.out_numchannels;
  if (ices_config.out_samplerate != -1)
    ices_reencode_flags->out_samplerate = ices_config.out_samplerate;
#endif

  /* lame_init_params isn't more specific about the problem */
  if (lame_init_params (ices_reencode_flags) < 0) {
    ices_log ("Error setting LAME parameters. Check bitrate, channels, and "
	      "sample rate.");
  }

  reencode_lame_allocated = 1;
}
