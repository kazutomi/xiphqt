/* stream.c
 * - Core streaming functions/main loop.
 *
 * $Id: stream.c,v 1.11.2.1 2002/02/07 09:11:12 msmith Exp $
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
#include <pthread.h>
#include <unistd.h>

#include <shout/shout.h>

#include "config.h"
#include "input.h"
#include "net/resolver.h"
#include "signals.h"
#include "thread/thread.h"
#include "stream.h"
#include "event.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define MODULE "stream/"
#include "logging.h"

#define MAX_ERRORS 10

static void _set_stream_defaults(stream_state *stream)
{
    stream->hostname = strdup("localhost");
    stream->port = 8000;
    stream->password = strdup("hackme");
    stream->mount = strdup("/stream.ogg");
    stream->reconnect_delay = 2;
    stream->reconnect_attempts = 10;
    stream->stream_name = strdup("unnamed ices stream");
    stream->stream_genre = strdup("genre not set");
    stream->stream_description = strdup("no description supplied");
}


static int stream_chunk(instance_t *instance, void *self, ref_buffer *buffer,
        ref_buffer **out)
{
    stream_state *stream = self;
    int ret;

	if(stream->errors > MAX_ERRORS)
	{
		LOG_WARN0("Too many errors, shutting down");
        release_buffer(buffer);
		return -2;
	}

	/* buffer being NULL means that either a fatal error occured,
	 * or we've been told to shut down
	 */
	if(!buffer) {
        LOG_ERROR0("Null buffer: this should never happen.");
    	return -2;
    }

    if(stream->wait_for_bos)
    {
        if(buffer->flags & FLAG_BOS) {
            LOG_INFO0("Trying restart on new substream");
            stream->wait_for_bos = 0;
        }
        else {
            release_buffer(buffer);
            return -1;
        }
    }

    ret = shout_send_raw(stream->shout, buffer->buf, buffer->len);

    if(ret < buffer->len) /* < 0 for error, or short write for other errors */
    {
		LOG_ERROR1("Send error: %s", shout_get_error(stream->shout));
		{
			int i=0;

			/* While we're trying to reconnect, don't receive data
			 * to this instance, or we'll overflow once reconnect
			 * succeeds
			 */
			instance->skip = 1;

			/* Also, flush the current queue */
			input_flush_queue(instance->queue, 1);
			
			while((i < stream->reconnect_attempts ||
					stream->reconnect_attempts==-1) && !ices_config->shutdown)
			{
				i++;
				LOG_WARN0("Trying reconnect after server socket error");
				shout_close(stream->shout);
				if(shout_open(stream->shout) == SHOUTERR_SUCCESS)
				{
					LOG_INFO3("Connected to server: %s:%d%s", 
                            shout_get_host(stream->shout), 
                            shout_get_port(stream->shout), 
                            shout_get_mount(stream->shout));

                    /* After reconnect, we MUST start a new logical stream.
                     * This instructs the input chain to do so.
                     */
                    create_event(ices_config->input_chain, EVENT_NEXTTRACK, 
                            NULL, 0);
					break;
				}
				else
				{
					LOG_ERROR3("Failed to reconnect to %s:%d (%s)",
						shout_get_host(stream->shout), 
                        shout_get_port(stream->shout), 
                        shout_get_error(stream->shout));
					if(i==stream->reconnect_attempts)
					{
						LOG_ERROR0("Reconnect failed too many times, "
								  "giving up.");
                        /* We want to die now */
						stream->errors = MAX_ERRORS+1; 
					}
					else { /* Don't try again too soon */
                        LOG_DEBUG1("Sleeping for %d seconds before retrying",
                                stream->reconnect_delay);
						sleep(stream->reconnect_delay); 
                    }
				}
			}
			instance->skip = 0;
		}
		stream->errors++;
	}

	release_buffer(buffer);

    return 0;
}
	
static void close_module(process_chain_element *mod) 
{
    stream_state *stream = mod->priv_data;

	shout_close(stream->shout);
   	shout_free(stream->shout);

    if(stream->hostname)
        free(stream->hostname);
    if(stream->password)
        free(stream->password);
    if(stream->mount)
        free(stream->mount);
    if(stream->connip)
        free(stream->connip);

    free(stream);
}

static int event_handler(process_chain_element *self, event_type ev, 
        void *param)
{
    switch(ev)
    {
        case EVENT_SHUTDOWN:
            close_module(self);
            break;
        default:
            return -1;
    }

    return 0;
}

int stream_open_module(process_chain_element *mod, module_param_t *params)
{
    stream_state *stream = calloc(1, sizeof(stream_state));
    module_param_t *paramstart = params;

    _set_stream_defaults(stream);

    mod->name = "output-stream";
    mod->input_type = MEDIA_VORBIS;
    mod->output_type = MEDIA_NONE;

    mod->process = stream_chunk;
    mod->event_handler = event_handler;

    mod->priv_data = stream;

    while(params) {
        if(!strcmp(params->name, "hostname"))
            stream->hostname = strdup(params->value);
        else if(!strcmp(params->name, "port"))
            stream->port = atoi(params->value);
        else if(!strcmp(params->name, "password"))
            stream->password = strdup(params->value);
        else if(!strcmp(params->name, "mount"))
            stream->mount = strdup(params->value);
        else if(!strcmp(params->name, "reconnectdelay"))
            stream->reconnect_delay = atoi(params->value);
        else if(!strcmp(params->name, "reconnectattempts"))
            stream->reconnect_attempts = atoi(params->value);
        else if(!strcmp(params->name, "stream-name"))
            stream->stream_name = strdup(params->value);
        else if(!strcmp(params->name, "stream-genre"))
            stream->stream_genre = strdup(params->value);
        else if(!strcmp(params->name, "stream-description"))
            stream->stream_description = strdup(params->value);
        else
            LOG_ERROR1("Unrecognised parameter \"%s\"", params->name);
            
        params = params->next;
    } 

    config_free_params(paramstart);

	stream->shout = shout_new();

	/* we only support the ice protocol and vorbis streams currently */
	shout_set_format(stream->shout, SHOUT_FORMAT_VORBIS);
	shout_set_protocol(stream->shout, SHOUT_PROTOCOL_ICE);


	stream->connip = malloc(16);
	if(!resolver_getip(stream->hostname, stream->connip, 16))
	{
		LOG_ERROR1("Could not resolve hostname \"%s\"", stream->hostname);
		return -1;
	}

	if (shout_set_host(stream->shout, stream->connip)) {
		LOG_ERROR1("libshout error: %s\n", shout_get_error(stream->shout));
		return -1;
	}

	shout_set_port(stream->shout, stream->port);
	if (shout_set_password(stream->shout, stream->password)) {
		LOG_ERROR1("libshout error: %s\n", shout_get_error(stream->shout));
		return -1;
	}
	if (shout_set_mount(stream->shout, stream->mount)) {
		LOG_ERROR1("libshout error: %s\n", shout_get_error(stream->shout));
		return -1;
	}

	/* set the metadata for the stream*/
	if (stream->stream_name)
		if (shout_set_name(stream->shout, stream->stream_name)) {
			LOG_ERROR1("libshout error: %s\n", shout_get_error(stream->shout));
			return -1;
		}
	if (stream->stream_genre)
		if (shout_set_genre(stream->shout, stream->stream_genre)) {
			LOG_ERROR1("libshout error: %s\n", shout_get_error(stream->shout));
			return -1;
		}
	if (stream->stream_description)
		if (shout_set_description(stream->shout, stream->stream_description)) {
			LOG_ERROR1("libshout error: %s\n", shout_get_error(stream->shout));
			return -1;
		}

	if(shout_open(stream->shout) == SHOUTERR_SUCCESS)
	{
		LOG_INFO3("Connected to server: %s:%d%s", 
				shout_get_host(stream->shout), shout_get_port(stream->shout), 
                shout_get_mount(stream->shout));
    }
	else
	{
		LOG_ERROR3("Failed initial connect to %s:%d (%s)", 
				shout_get_host(stream->shout),shout_get_port(stream->shout), 
                shout_get_error(stream->shout));
	}

    return 0;
}

