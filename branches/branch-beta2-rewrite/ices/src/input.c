/* input.c
 *  - Main producer control loop. Fetches data from input modules, and controls
 *    submission of these to the instance threads. Timing control happens here.
 *
 * $Id: input.c,v 1.12.2.4 2002/02/09 03:55:36 msmith Exp $
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
#include <sys/types.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <string.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#include "thread/thread.h"
#include "config.h"
#include "stream.h"
#include "timing.h"
#include "input.h"
#include "process.h"

#define MODULE "input/"
#include "logging.h"

#define MAX_BUFFER_FAILURES 15

#ifdef _WIN32
typedef __int64 int64_t
typedef unsigned __int64 uint64_t
#endif

typedef struct _timing_control_tag 
{
	uint64_t starttime;
	uint64_t senttime;
	int samples;
	int oldsamples;
	int samplerate;
	long serialno;
} timing_control;

/* This is identical to shout_sync(), really. */
static void _sleep(timing_control *control)
{
	uint64_t sleep;

	/* no need to sleep if we haven't sent data */
	if (control->senttime == 0) return;

	sleep = ((double)control->senttime / 1000) - 
		(timing_get_time() - control->starttime);

	if(sleep > 0) timing_sleep(sleep);
}

static int _calculate_pcm_sleep(ref_buffer *buf, timing_control *control)
{
	if(control->starttime == 0)
		control->starttime = timing_get_time();

	control->senttime += ((double)buf->len * 1000000.)/
        ((double)buf->aux_data[0]);

    return 0;
}

static int _calculate_ogg_sleep_page(timing_control *control, ogg_page *og)
{
	/* Largely copied from shout_send(), without the sending happening.*/
	ogg_stream_state os;
	ogg_packet op;
	vorbis_info vi;
	vorbis_comment vc;

	if(control->starttime == 0)
		control->starttime = timing_get_time();

	if(control->serialno != ogg_page_serialno(og)) {
        LOG_DEBUG1("New ogg stream, serial %d", ogg_page_serialno(og));
		control->serialno = ogg_page_serialno(og);

		control->oldsamples = 0;

		ogg_stream_init(&os, control->serialno);
		vorbis_info_init(&vi);
		vorbis_comment_init(&vc);

        if(ogg_stream_pagein(&os, og) < 0) {
            LOG_ERROR0("Error submitting page to libogg");
            goto fail;
        }
        if(ogg_stream_packetout(&os, &op) < 0) {
            LOG_ERROR0("Error retrieving packet from libogg");
            goto fail;
        }


		if(vorbis_synthesis_headerin(&vi, &vc, &op) < 0) 
        {
            LOG_ERROR0("Timing control: can't determine sample rate for input, "
                       "not vorbis.");
            control->samplerate = 0;
            goto fail;
        }
        else {
		    control->samplerate = vi.rate;
        }


		vorbis_comment_clear(&vc);
		vorbis_info_clear(&vi);
		ogg_stream_clear(&os);
	}

	control->samples = ogg_page_granulepos(og) - control->oldsamples;
	control->oldsamples = ogg_page_granulepos(og);

    if(control->samplerate) 
	    control->senttime += ((double)control->samples * 1000000 / 
		    	(double)control->samplerate);

    return 0;

fail:
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    ogg_stream_clear(&os);
    return -1;
}

static int _calculate_ogg_sleep(ref_buffer *buf, timing_control *control)
{
	ogg_page og;
    int ret,i;

    for(i=0; i < buf->aux_data_len; i += 2) {
	    og.header_len = buf->aux_data[i];
    	og.body_len = buf->aux_data[i+1];
	    og.header = buf->buf;
    	og.body = buf->buf + og.header_len;

        if((ret = _calculate_ogg_sleep_page(control, &og)) < 0)
            return ret;
    }
    return 0;
}

void input_flush_queue(buffer_queue *queue, int keep_critical)
{
	queue_item *item, *next, *prev=NULL;

	LOG_DEBUG0("Input queue flush requested");

    thread_mutex_lock(&ices_config->flush_lock);

	thread_mutex_lock(&queue->lock);
	if(!queue->head)
	{
		thread_mutex_unlock(&queue->lock);
        thread_mutex_unlock(&ices_config->flush_lock);
		return;
	}

	item = queue->head;
	while(item)
	{
		next = item->next;

		if(!(keep_critical && (item->buf->flags & FLAG_CRITICAL)))
		{
			thread_mutex_lock(&ices_config->refcount_lock);
			item->buf->count--;
			if(!item->buf->count)
			{
				free(item->buf->buf);
				free(item->buf);
			}
			thread_mutex_unlock(&ices_config->refcount_lock);

			if(prev)
				prev->next = next;
			else
				queue->head = next;

			free(item);
			item = next;

			queue->length--;
		}
		else
		{
			prev = item;
			item = next;
            LOG_DEBUG0("Keeping critical buffer on flush");
		}
	}

	/* Now, fix up the tail pointer */
	queue->tail = NULL;
	item = queue->head;

	while(item)
	{
		queue->tail = item;
		item = item->next;
	}

	thread_mutex_unlock(&queue->lock);
    thread_mutex_unlock(&ices_config->flush_lock);
}

void input_loop(void)
{
	timing_control *control = calloc(1, sizeof(timing_control));
	instance_t *instance, *prev, *next;
	queue_item *queued;
    process_chain_element *chain;
	int shutdown = 0;
    int valid_stream = 1;

    control->serialno = -1; /* FIXME: Ideally, we should use a flag here - this
                               is otherwise a valid serial number */

	thread_cond_create(&ices_config->queue_cond);
	thread_cond_create(&ices_config->event_pending_cond);
	thread_mutex_create(&ices_config->refcount_lock);
	thread_mutex_create(&ices_config->flush_lock);


	/* ok, basic config stuff done. Now, we want to start all our listening
	 * threads, and initialise the processing chains
	 */

    chain = ices_config->input_chain;
    while(chain) {
        chain->open(chain, chain->params);
        chain = chain->next;
    }

	instance = ices_config->instances;

	while(instance) 
	{
        chain = instance->output_chain;
        while(chain) {
            chain->open(chain, chain->params);
            chain = chain->next;
        }
		thread_create("stream", ices_instance_output, instance, 1);
		instance = instance->next;
	}

	/* now we go into the main loop
	 * We shut down the main thread ONLY once all the instances
	 * have killed themselves.
	 */
	while(!shutdown) 
	{
        ref_buffer *outchunk;
		buffer_queue *current;
		int ret;

		instance = ices_config->instances;
		prev = NULL;

		while(instance)
		{
			/* if an instance has died, get rid of it
			** this should be replaced with something that tries to 
			** restart the instance, probably.
			*/
			if (instance->died) 
			{
				LOG_DEBUG0("An instance died, removing it");
				next = instance->next;

				if (prev)
				{
					prev->next = next;
					prev = instance;
				}
				else
					ices_config->instances = next;

				/* Just in case, flush any existing buffers*/
				input_flush_queue(instance->queue, 0);

                create_event(instance->output_chain, EVENT_SHUTDOWN, NULL, 1);

				config_free_instance(instance);

				instance = next;
				continue;
			}

			prev = instance;
			instance = instance->next;
		}

		instance = ices_config->instances;

		if(!instance)
		{
			shutdown = 1;
			continue;
		}

		if(ices_config->shutdown) /* We've been signalled to shut down, but */
		{						  /* the instances haven't done so yet... */
			timing_sleep(250); /* sleep for quarter of a second */
			continue;
		}

        /* Process the input chain -
         * This includes fetching data from some sort of input module, and
         * possibly some processing on this data.
         */
        ret = process_chain(NULL, ices_config->input_chain, NULL, &outchunk);

        if(ret > 0 && outchunk->count != 1) {
            LOG_ERROR0("Output chunk has wrong refcount!!");
            exit(1);
        }

		/* input module signalled non-fatal error. Skip this chunk */
		if(ret==0)
		{
			if(outchunk)
                release_buffer(outchunk);
			continue;
		}

		/* Input module signalled fatal error, shut down - nothing we can do
		 * from here */
		if(ret < 0)
		{
			ices_config->shutdown = shutdown = 1;
			thread_cond_broadcast(&ices_config->queue_cond);
			continue;
		}

        if(outchunk->flags & FLAG_CRITICAL)
            valid_stream = 1;

		/* figure out how much time the data represents */
		switch(outchunk->type)
		{
			case MEDIA_VORBIS:
				ret = _calculate_ogg_sleep(outchunk, control);
				break;
			case MEDIA_PCM:
				ret = _calculate_pcm_sleep(outchunk, control);
				break;
            default:
                LOG_ERROR0("Cannot handle datatype in main input loop");
		}

        if(ret < 0)
            valid_stream = 0;

        if(valid_stream) 
        {
    		while(instance)
	    	{
		    	if(instance->skip || 
                        (instance->wait_for_critical && 
                        !(outchunk->flags & FLAG_CRITICAL)) ||
                        (instance->queue->length >= instance->max_queue_length
                        && !(outchunk->flags & FLAG_CRITICAL)))
	    		{
		    		instance = instance->next;
			    	continue;
    			}
    
	    		queued = malloc(sizeof(queue_item));
    
	    		queued->buf = outchunk;
    			current = instance->queue;
    
                acquire_buffer(outchunk);
    
	    		thread_mutex_lock(&current->lock);
    
	    		if(current->head == NULL)
		    	{
			    	current->head = current->tail = queued;
				    current->head->next = current->tail->next = NULL;
	    		}
    			else
			    {
		    		current->tail->next = queued;
				    queued->next = NULL;
    				current->tail = queued;
	    		}

		    	current->length++;
			    thread_mutex_unlock(&current->lock);

    			instance = instance->next;
	    	}
        }

        /* We create it with a refcount of 1, so release it now */
        release_buffer(outchunk);

        if(valid_stream) {
    		/* wake up the instances */
	    	thread_cond_broadcast(&ices_config->queue_cond);

		    _sleep(control);
        }
	}

	LOG_DEBUG0("All instances removed, shutting down control thread.");

	thread_cond_destroy(&ices_config->queue_cond);
	thread_cond_destroy(&ices_config->event_pending_cond);
	thread_mutex_destroy(&ices_config->flush_lock);
	thread_mutex_destroy(&ices_config->refcount_lock);

	free(control);

    create_event(ices_config->input_chain, EVENT_SHUTDOWN, NULL, 1);

	return;
}


