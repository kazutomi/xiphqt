/* im_stdinpcm.h
 * - stdin reading
 *
 * $Id: im_stdinpcm.h,v 1.2.2.1 2002/02/07 09:11:11 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_STDINPCM_H__
#define __IM_STDINPCM_H__

#include "config.h"

typedef struct
{
	int rate;
	int channels;
	int newtrack;
} stdinpcm_state; 

int stdin_open_module(process_chain_element *mod, module_param_t *params);

#endif  /* __IM_STDINPCM_H__ */
