/* im_sun.h
 * - read pcm data from sun devices
 *
 * $Id: im_sun.h,v 1.2 2001/09/25 12:04:21 msmith Exp $
 *
 * by Ciaran Anscomb <ciarana@rd.bbc.co.uk>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_SUN_H__
#define __IM_SUN_H__

#include <sys/audioio.h>
#include "inputmodule.h"
#include <ogg/ogg.h>

typedef struct
{
	audio_info_t device_info;
	int fd;
	int fdctl;
	char **metadata;
	int newtrack;
	int user_terminated;
	int samples;
	int default_len;
	int read_buffer_len;
	void *read_buffer;
	const char *device;
} im_sun_state; 

int sun_open_module (input_module_t *mod);
void sun_close_module (input_module_t *mod);
void sun_shutdown_module (input_module_t *mod);
int  sun_init_module (input_module_t *mod);


#endif  /* __IM_SUN_H__ */

