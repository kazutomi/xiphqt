/* im_stdinpcm.c
 * - Raw PCM input from stdin
 *
 * $Id: im_stdinpcm.c,v 1.2.2.1 2002/02/07 09:11:11 msmith Exp $
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

#include "im_stdinpcm.h"

#define MODULE "input-stdinpcm/"
#include "logging.h"

#define BUFSIZE 32768

static int event_handler(process_chain_element *mod, event_type ev, 
        void *param)
{
	switch(ev)
	{
		case EVENT_SHUTDOWN:
			if(mod)
			{
				if(mod->priv_data)
					free(mod->priv_data);
			}
			break;
		case EVENT_NEXTTRACK:
			((stdinpcm_state *)mod->priv_data)->newtrack = 1;
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
static int stdin_read(instance_t *instance, void *self, 
        ref_buffer *in, ref_buffer **out)
{
	int result;
	stdinpcm_state *s = self;
    ref_buffer *rb;

    rb = new_ref_buffer(MEDIA_PCM, NULL, 0);

	rb->buf = malloc(BUFSIZE);
	result = fread(rb->buf, 1,BUFSIZE, stdin);

	rb->len = result;
    rb->rate = s->rate;
    rb->channels = s->channels;
    rb->subtype = SUBTYPE_PCM_LE_16;

	if(s->newtrack)
	{
		rb->flags |= FLAG_CRITICAL | FLAG_BOS;
		s->newtrack = 0;
	}

	if(rb->len <= 0)
	{
		LOG_INFO0("Reached EOF, no more data available\n");
		release_buffer(rb);
		return -1;
	}

    *out = rb;

	return rb->len;
}

int stdin_open_module(process_chain_element *mod, module_param_t *params)
{
	stdinpcm_state *s;
	module_param_t *current;

    mod->name = "input-stdinpcm";
    mod->input_type = MEDIA_NONE;
	mod->output_type = MEDIA_PCM;

	mod->process = stdin_read;
	mod->event_handler = event_handler;

	mod->priv_data = malloc(sizeof(stdinpcm_state));
	s = mod->priv_data;

	s->rate = 44100; /* Defaults */
	s->channels = 2;
    s->newtrack = 1;

	current = params;

	while(current)
	{
		if(!strcmp(current->name, "rate"))
			s->rate = atoi(current->value);
		else if(!strcmp(current->name, "channels"))
			s->channels = atoi(current->value);
		else
			LOG_WARN1("Unknown parameter %s for stdinpcm module", current->name);

		current = current->next;
	}
    config_free_params(params);

	return 0;
}


