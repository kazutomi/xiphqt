/* im_jack.h
 * - input from JACK applications
 *
 * $Id: im_jack.h,v 0.5 2004/01/22 22:53:30 j Exp $
 *
 *
 * (c) 2004 jan gerber <j@thing.net>,
 * based on im_alsa.c which is...
 * by Jason Chu  <jchu@uvic.ca>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_JACK_H__
#define __IM_JACK_H__

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
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include "inputmodule.h"

typedef struct
{
	int rate;
	int channels;
	int samples;

	int newtrack;
    int user_terminated;
    unsigned sleep;

	int jack_shutdown;
    const char *clientname;
	jack_client_t *client;
	jack_port_t **jack_ports;
    jack_ringbuffer_t **rb;
	volatile int can_process;

} im_jack_state; 

int  jack_init_module(input_module_t *mod);
int  jack_open_module (input_module_t *mod);
void jack_shutdown_module (input_module_t *mod);
void jack_close_module (input_module_t *mod);

#endif  /* __IM_JACK_H__ */
