/* im_playlist.h
 * - Basic playlist functionality
 *
 * $Id: im_playlist.h,v 1.2.2.1 2002/02/07 09:11:11 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_PLAYLIST_H__
#define __IM_PLAYLIST_H__

#include "process.h"
#include <ogg/ogg.h>

typedef struct _playlist_state_tag
{
	FILE *current_file;
	char *filename; /* Currently streaming file */
	int errors; /* Consecutive errors */
	int nexttrack;
    int prev_was_header_page;
	ogg_sync_state oy;
	
	char *(*get_filename)(void *data); /* returns the next desired filename */
	void (*clear)(void *data); /* module clears self here */

	void *data; /* Internal data for this particular playlist module */

} playlist_state_t;

int playlist_open_module(process_chain_element *self, module_param_t *params);

#endif  /* __IM_PLAYLIST_H__ */
