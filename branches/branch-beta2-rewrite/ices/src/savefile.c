/* savefile.c
 * - Save a stream to a file, for archival purposes
 *
 * $Id: savefile.c,v 1.3.2.2 2002/02/09 05:07:01 msmith Exp $
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

#include "process.h"

#define MODULE "savefile/"
#include "logging.h"

/* FIXME: Extend savefile to be able to create a new output file either 
 * whenever a new stream is reached, or when a certain filesize is reached.
 */


typedef struct {
    FILE *file;
    char *filename;
} savefile_state;

static int event_handler(process_chain_element *mod, event_type ev, 
        void *param)
{
	switch(ev)
	{
		case EVENT_SHUTDOWN:
			if(mod)
			{
				if(mod->priv_data) {
                    if(((savefile_state *)mod->priv_data)->filename)
                        free(((savefile_state *)mod->priv_data)->filename);
					free(mod->priv_data);
                }
			}
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
static int savefile_read(instance_t *instance, void *self, 
        ref_buffer *in, ref_buffer **out)
{
	savefile_state *s = self;
    int ret;

    /* If this isn't set up right, don't cause errors... */
    if(!s->file) {
        *out = in;
        return in->len;
    }

    ret = fwrite(in->buf, 1, in->len, s->file);

    if(ret < in->len) {
        if(ret >= 0)
            LOG_WARN2("Could only write %d of %d bytes to savefile", 
                ret, in->len);
        else {
            LOG_ERROR2("Error writing to savefile (%s): %s", s->filename, 
                    strerror(errno));
            return -1;
        }
    }

    *out = in;

	return in->len;
}

int savefile_open_module(process_chain_element *mod, module_param_t *params)
{
	savefile_state *s;
	module_param_t *current;

    mod->name = "process-savefile";

    /* We actually don't care what input and output are, so we use MEDIA_DATA */
    mod->input_type = MEDIA_DATA;
	mod->output_type = MEDIA_DATA;

	mod->process = savefile_read;
	mod->event_handler = event_handler;

	mod->priv_data = calloc(1, sizeof(savefile_state));
	s = mod->priv_data;

	current = params;

	while(current)
	{
		if(!strcmp(current->name, "filename"))
			s->filename = strdup(current->value);
		else
			LOG_WARN1("Unknown parameter %s for savefile module",current->name);

		current = current->next;
	}
    config_free_params(params);

    s->file = fopen(s->filename, "wb");
    if(!s->file)
        LOG_ERROR2("Error opening file for savefile %s: %s", s->filename, 
                strerror(errno));
    else
        LOG_INFO1("Opened file %s for savestream", s->filename);

	return 0;
}


