/* im_oss.h
 * - read pcm data from oss devices
 *
 * $Id: im_oss.h,v 1.2 2001/09/25 12:04:21 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_OSS_H__
#define __IM_OSS_H__

#include "inputmodule.h"
#include <ogg/ogg.h>

typedef struct
{
	int channels;
	int samplerate;
	int aux_data;
    int samples;
    int default_len;
    int user_terminated;

	int fd;
    char *devicename;
	int newtrack;
    void *read_buffer;
    unsigned read_buffer_len;
} im_oss_state; 

int oss_open_module(input_module_t *);
void oss_close_module (input_module_t *);

int oss_init_module(input_module_t *mod);
void oss_shutdown_module(input_module_t *mod);


#endif  /* __IM_OSS_H__ */
