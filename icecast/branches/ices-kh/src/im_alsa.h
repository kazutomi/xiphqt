/* im_alsa.h
 * - read pcm data from oss devices
 *
 * $Id: im_alsa.h,v 1.1 2002/12/29 10:28:30 msmith Exp $
 *
 * by Jason Chu  <jchu@uvic.ca>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_ALSA_H__
#define __IM_ALSA_H__

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <ogg/ogg.h>
#include <alsa/asoundlib.h>
#include "inputmodule.h"

typedef struct
{
	int rate;
	int channels;
    unsigned buffer_time;
    unsigned periods;

	snd_pcm_t *fd;
    const char *device;
	int newtrack;
    int user_terminated;
    int samples;
    void *read_buffer;
    unsigned read_buffer_len;
} im_alsa_state; 


int  alsa_init_module(input_module_t *mod);
int  alsa_open_module (input_module_t *mod);
void alsa_shutdown_module (input_module_t *mod);
void alsa_close_module (input_module_t *mod);


#endif  /* __IM_ALSA_H__ */
