/* playlist.c
 * - Basic playlist functionality
 *
 * $Id: im_playlist.c,v 1.3.2.2 2002/02/09 03:55:36 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ogg/ogg.h>

#include "thread.h"

#include "config.h"
#include "stream.h"
#include "event.h"

#include "process.h"
#include "im_playlist.h"

#include "playlist_basic.h"

#define MODULE "playlist-builtin/"
#include "logging.h"

#define BUFSIZE 4096

typedef struct _module 
{
	char *name;
	int (*init)(module_param_t *, playlist_state_t *);
} module;

static module modules[] = {
	{ "basic", playlist_basic_initialise},
	{NULL,NULL}
};

static void close_module(process_chain_element *mod)
{
	if (mod == NULL) {
        LOG_ERROR0("close_module called without module available");
        return;
    }

	if (mod->priv_data) 
	{
		playlist_state_t *pl = (playlist_state_t *)mod->priv_data;
		pl->clear(pl->data);
		ogg_sync_clear(&pl->oy);
		free(pl);
	}
}

static int event_handler(process_chain_element *self, event_type ev, 
        void *param)
{
	switch(ev)
	{
		case EVENT_SHUTDOWN:
			close_module(self);
			break;
		case EVENT_NEXTTRACK:
			LOG_INFO0("Moving to next file in playlist.");
			((playlist_state_t *)self->priv_data)->nexttrack = 1;
			break;
		default:
			return -1;
	}

	return 0;
}

/* Core streaming function for this module
 * This is what actually produces the data which gets streamed.
 *
 * returns:  >0  Number of bytes read
 *            0  Non-fatal error.
 *           <0  Fatal error.
 */
static int playlist_read(instance_t *instance, void *self, 
        ref_buffer *in, ref_buffer **out)
{
	playlist_state_t *pl = (playlist_state_t *)self;
	int bytes;
	unsigned char *buf;
	char *newfn;
	int result;
	ogg_page og;

    if(in) {
        LOG_ERROR0("Non-null input buffer for playlist module, not allowed");
        return -1;
    }

	if (pl->errors > 5) 
	{
		LOG_WARN0("Too many consecutive errors - exiting");
		return -1;
	}

	if (!pl->current_file || pl->nexttrack) 
	{
		pl->nexttrack = 0;

		if(pl->current_file) 
        {
			fclose(pl->current_file);
            pl->current_file = NULL;
        }

		newfn = pl->get_filename(pl->data);

		if(pl->filename && !strcmp(pl->filename, newfn))
		{
			LOG_ERROR0("Cannot play same file twice in a row, skipping");
			pl->errors++;
			return 0;
		}

		pl->filename = newfn;

		if(!pl->filename)
		{
			LOG_INFO0("No more filenames available, end of playlist");
			return -1;
		}
		
		pl->current_file = fopen(pl->filename, "rb");

		LOG_INFO1("Currently playing %s", pl->filename);

		if (!pl->current_file) 
		{
			LOG_WARN2("Error opening file %s: %s",pl->filename, strerror(errno));
			pl->errors++;
			return 0;
		}

		/* Reinit sync, so that dead data from previous file is discarded */
		ogg_sync_clear(&pl->oy);
		ogg_sync_init(&pl->oy);
	}

	while(1)
	{
		result = ogg_sync_pageout(&pl->oy, &og);
		if(result < 0)
			LOG_WARN1("Corrupt or missing data in file (%s)", pl->filename);
		else if(result > 0)
		{
            void *buf = malloc(og.header_len + og.body_len);
            *out = new_ref_buffer(MEDIA_VORBIS, buf, 
                    og.header_len + og.body_len, 2);

            (*out)->channels = -1; /* We don't know yet, and it's unimportant */
            (*out)->rate = -1;
            (*out)->aux_data[0] = og.header_len;
            (*out)->aux_data[1] = og.body_len;
            (*out)->aux_data_len = 2;

			memcpy((*out)->buf, og.header, og.header_len);
			memcpy((*out)->buf+og.header_len, og.body, og.body_len);
			if(ogg_page_granulepos(&og)==0) {
				(*out)->flags = FLAG_CRITICAL;
                if(!pl->prev_was_header_page) {
                    (*out)->flags |= FLAG_BOS;
                }
                pl->prev_was_header_page = 1;
            }
            else
                pl->prev_was_header_page = 0;
			break;
		}

		/* If we got to here, we didn't have enough data. */
		buf = ogg_sync_buffer(&pl->oy, BUFSIZE);
		bytes = fread(buf,1, BUFSIZE, pl->current_file);
		if (bytes <= 0) 
		{
			if (feof(pl->current_file)) 
			{
				pl->nexttrack = 1;
				return playlist_read(instance, self, in, out);
			} 
			else 
			{
				LOG_ERROR2("Read error from %s: %s", 
						pl->filename, strerror(errno));
				fclose(pl->current_file);
				pl->current_file=NULL;
				pl->errors++;
				return 0; 
			}
		}
		else
			ogg_sync_wrote(&pl->oy, bytes);
	}

	pl->errors=0;

	return (*out)->len;
}

int playlist_open_module(process_chain_element *mod,
        module_param_t *params)
{
	playlist_state_t *pl;
	module_param_t *current;
	int (*init)(module_param_t *, playlist_state_t *)=NULL;

    mod->name = "input-playlist";
	mod->input_type = MEDIA_NONE;
    mod->output_type = MEDIA_VORBIS;

	mod->process = playlist_read;
	mod->event_handler = event_handler;

	mod->priv_data = calloc(1, sizeof(playlist_state_t));
	pl = (playlist_state_t *)mod->priv_data;

	current = params;
	while(current)
	{
		if (!strcmp(current->name, "type"))
		{
			int current_module = 0;

			while(modules[current_module].init)
			{
				if(!strcmp(current->value, modules[current_module].name))
				{
					init = modules[current_module].init; 
					break;
				}
				current_module++;
			}
			
			if(!init)
			{
				LOG_ERROR1("Unknown playlist type \"%s\"", current->value);
				goto fail;
			}
		}
		current = current->next;
	}

	if(init)
	{
		if(init(params, pl))
		{
			LOG_ERROR0("Playlist initialisation failed");
			goto fail;
		}
		else 
		{
			ogg_sync_init(&pl->oy);
            config_free_params(params);
			return 0; /* Success. Finished initialising */
		}
	}
	else
		LOG_ERROR0("No playlist type given, cannot initialise playlist module");

fail:
    config_free_params(params);

	if (mod) 
	{
		if (mod->priv_data)
			free(mod->priv_data);
	}

	return -1;
}


