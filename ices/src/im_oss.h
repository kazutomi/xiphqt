/* im_oss.h
 * - read pcm data from oss devices
 *
 * $Id: im_oss.h,v 1.2.2.1 2002/02/07 09:11:11 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_OSS_H__
#define __IM_OSS_H__

#include "config.h"
#include "thread.h"
#include <ogg/ogg.h>

typedef struct
{
	int rate;
	int channels;

	int fd;
	char **metadata;
	int newtrack;
	mutex_t metadatalock;
} im_oss_state; 

int oss_open_module(process_chain_element *mod, module_param_t *params);

#endif  /* __IM_OSS_H__ */
