/* im_playlist.h
 * - Basic playlist functionality
 *
 * $Id: im_playlist.h,v 1.3 2002/07/07 11:07:55 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_PLAYLIST_H__
#define __IM_PLAYLIST_H__

#include "inputmodule.h"
#include <ogg/ogg.h>

struct playlist_state
{
	FILE *current_file;
	char *filename; /* Currently streaming file */
	int errors; /* Consecutive errors */
	int next_track;
    int terminate;
    int sleep;
    int os_init;
    int more_headers;
    int prev_window;
    ogg_int64_t granulepos;
	ogg_sync_state oy;
	ogg_stream_state os;
    vorbis_info vi;
    vorbis_comment vc;
    ogg_packet *prev_packet;
    ogg_packet *prev_op;

	char *(*get_filename)(void *data); /* returns the next desired filename */
    void (*free_filename)(void *data, char *fn); /* Called when im_playlist is
                                                    done with this filename */
	void (*clear) (void *data); /* module clears self here */
    int (*write_func) (input_module_t *mod, void *buffer, unsigned len);
    void *(*alloc_buffer) (input_module_t *mod, unsigned len);
    void (*flush_func) (input_module_t *mod);

	void *data; /* Internal data for this particular playlist module */
};

int  playlist_open_module (input_module_t *);
void playlist_close_module (input_module_t *mod);
int  playlist_init_module (input_module_t *);
void playlist_shutdown_module (input_module_t *mod);


#endif  /* __IM_PLAYLIST_H__ */
