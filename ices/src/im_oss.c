/* im_oss.c
 * - Raw PCM input from OSS devices
 *
 * $Id: im_oss.c,v 1.5.2.1 2002/02/07 09:11:11 msmith Exp $
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
#include <unistd.h>
#include <ogg/ogg.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>


#include "thread.h"
#include "config.h"
#include "stream.h"
#include "metadata.h"
#include "process.h"

#include "im_oss.h"

#define MODULE "input-oss/"
#include "logging.h"

#define BUFSIZE 8192

static void close_module(process_chain_element *mod)
{
	if(mod)
	{
		if(mod->priv_data)
		{
			im_oss_state *s = mod->priv_data;
			if(s->fd >= 0)
				close(s->fd);
			thread_mutex_destroy(&s->metadatalock);
			free(s);
		}
	}
}
static int event_handler(process_chain_element *mod, event_type ev, void *param)
{
	im_oss_state *s = mod->priv_data;

	switch(ev)
	{
		case EVENT_SHUTDOWN:
			close_module(mod);
			break;
		case EVENT_NEXTTRACK:
			s->newtrack = 1;
			break;
		case EVENT_METADATAUPDATE:
			thread_mutex_lock(&s->metadatalock);
			if(s->metadata)
			{
				char **md = s->metadata;
				while(*md)
					free(*md++);
				free(s->metadata);
			}
			s->metadata = (char **)param;
			s->newtrack = 1;
			thread_mutex_unlock(&s->metadatalock);
			break;
		default:
			return -1;
	}

	return 0;
}

/*
static void metadata_update(void *self, vorbis_comment *vc)
{
	im_oss_state *s = self;
	char **md;

	thread_mutex_lock(&s->metadatalock);

	md = s->metadata;

	if(md)
	{
		while(*md)
			vorbis_comment_add(vc, *md++);
	}

	thread_mutex_unlock(&s->metadatalock);
}
*/

/* Core streaming function for this module
 * This is what actually produces the data which gets streamed.
 *
 * returns:  >0  Number of bytes read
 *            0  Non-fatal error.
 *           <0  Fatal error.
 */
static int oss_read(instance_t *instance, void *self, 
        ref_buffer *in, ref_buffer **out) 
{
	int result;
	im_oss_state *s = self;
    ref_buffer *rb;
    
    rb = new_ref_buffer(MEDIA_PCM, NULL, 0);
    *out = rb;

	rb->buf = malloc(BUFSIZE*2*s->channels);
	result = read(s->fd, rb->buf, BUFSIZE*2*s->channels);

	rb->len = result;
    rb->subtype = SUBTYPE_PCM_LE_16;
    rb->channels = s->channels;
    rb->rate = s->rate;

	if(s->newtrack)
	{
		rb->flags |= FLAG_CRITICAL | FLAG_BOS;
		s->newtrack = 0;
	}

	if(result == -1 && errno == EINTR)
	{
		return 0; /* Non-fatal error */
	}
	else if(result <= 0)
	{
		if(result == 0)
			LOG_INFO0("Reached EOF, no more data available");
		else
			LOG_ERROR1("Error reading from audio device: %s", strerror(errno));
        release_buffer(rb);
		return -1;
	}

	return rb->len;
}

int oss_open_module(process_chain_element *mod, module_param_t *params)
{
	im_oss_state *s;
	module_param_t *current;
	char *device = strdup("/dev/dsp"); /* default device */
	int format = AFMT_S16_LE;
	int channels, rate;
	int use_metadata = 1; /* Default to on */

    mod->name = "input-oss";
	mod->input_type = MEDIA_NONE;
    mod->output_type = MEDIA_PCM;
    
	mod->process = oss_read;
	mod->event_handler = event_handler;

	mod->priv_data = calloc(1, sizeof(im_oss_state));
	s = mod->priv_data;

	s->fd = -1; /* Set it to something invalid, for now */
	s->rate = 44100; /* Defaults */
	s->channels = 2; 
    s->newtrack = 1;

	thread_mutex_create(&s->metadatalock);

	current = params;

	while(current)
	{
		if(!strcmp(current->name, "rate"))
			s->rate = atoi(current->value);
		else if(!strcmp(current->name, "channels"))
			s->channels = atoi(current->value);
		else if(!strcmp(current->name, "device"))
			device = strdup(current->value);
		else if(!strcmp(current->name, "metadata"))
			use_metadata = atoi(current->value);
		/*else if(!strcmp(current->name, "metadatafilename"))
			ices_config->metadata_filename = strdup(current->value);*/
		else
			LOG_WARN1("Unknown parameter %s for im_oss module", current->name);

		current = current->next;
	}

    config_free_params(params);

	/* First up, lets open the audio device */
	if((s->fd = open(device, O_RDONLY, 0)) == -1)
	{
		/* Open failed */
		LOG_ERROR2("Failed to open audio device %s: %s", 
				device, strerror(errno));
		goto fail;
	}

	/* Now, set the required parameters on that device */

	if(ioctl(s->fd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		LOG_ERROR2("Failed to set sample format on audio device %s: %s", 
				device, strerror(errno));
		goto fail;
	}
	if(format != AFMT_S16_LE)
	{
		LOG_ERROR0("Couldn't set sample format to AFMT_S16_LE");
		goto fail;
	}

	channels = s->channels;
	if(ioctl(s->fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		LOG_ERROR2("Failed to set number of channels on audio device %s: %s", 
				device, strerror(errno));
		goto fail;
	}
	if(channels != s->channels)
	{
		LOG_ERROR0("Couldn't set number of channels");
		goto fail;
	}

	rate = s->rate;
	if(ioctl(s->fd, SNDCTL_DSP_SPEED, &rate) == -1)
	{
		LOG_ERROR2("Failed to set sampling rate on audio device %s: %s", 
				device, strerror(errno));
		goto fail;
	}
	if(rate != s->rate)
	{
		LOG_ERROR0("Couldn't set sampling rate");
		goto fail;
	}

	/* We're done, and we didn't fail! */
	LOG_INFO3("Opened audio device %s at %d channel(s), %d Hz", 
			device, channels, rate);

    /* FIXME: Re-add this stuff.
	if(use_metadata)
	{
        if(ices_config->metadata_filename)
            thread_create("im_oss-metadata", metadata_thread_signal, mod, 1);
        else
		    thread_create("im_oss-metadata", metadata_thread_stdin, mod, 1);
		LOG_INFO0("Started metadata update thread");
	}
    */

    free(device);
	return 0;

fail:
    free(device);
	close_module(mod); /* safe, this checks for valid contents */
	return -1;
}


