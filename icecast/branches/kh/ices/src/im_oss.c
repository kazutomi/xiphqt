/* im_oss.c
 * - Raw PCM input from OSS devices
 *
 * $Id: im_oss.c,v 1.7 2002/08/09 13:52:56 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.


 * Modified to work with non-blocking ices.

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include "thread/thread.h"
#include "cfgparse.h"
#include "stream.h"
#include "metadata.h"
#include "inputmodule.h"

#include "im_oss.h"
#include "signals.h"

#define MODULE "input-oss/"
#include "logging.h"

#define SAMPLES    8192

static void oss_return_buffer (input_module_t *mod __attribute__((unused)), input_buffer *ib)
{
    ib->critical = 0;
    ib->eos = 0;
    return;
}


static void oss_free_buffer (input_module_t *mod, input_buffer *ib)
{
    im_oss_state *s = mod->internal;
    float **ptr;
    int i;

    ptr = ib->buf;
    for (i=s->channels; i; i--)
    {
        if (ptr)
        {
            free (*ptr);
            *ptr = NULL;
        }
        ptr++;
    }
    free (ib->buf);
    ib->buf = NULL;
}


static int oss_initialise_buffer (input_module_t *mod, input_buffer *ib)
{
    im_oss_state *s = mod->internal;
    float **ptr;
    int i;

    if ((ib->buf = calloc (s->channels, sizeof (float*))) == NULL)
        return -1;
    ptr = ib->buf;
    for (i=s->channels ; i; i--)
    {
        if ((*ptr = calloc (sizeof(float), s->samples)) == NULL)
            return -1;
        ptr++;
    }

    ib->type = ICES_INPUT_PCM;
    ib->subtype = INPUT_PCM_LE_16;
    ib->critical = 0;
    ib->channels = s->channels;
    ib->samplerate = s->samplerate;
    ib->mod = mod;

    return 0;
}


/* Core streaming function for this module
 * This is what actually produces the data which gets streamed.
 *
 */
static int oss_read(input_module_t *mod)
{
	im_oss_state *s = mod->internal;
    input_buffer *ib;
    int len;
    int dead_air;
    
    while (1)
    {
        if (s->user_terminated)
        {
            s->user_terminated = 0;
            return 0;
        }

        dead_air = 100;
        while ((ib = input_alloc_buffer (mod)) == NULL)
        {
            if (dead_air == 100)
                LOG_WARN0 ("will skip input for a short time");
            read (s->fd, dead_audio, DEAD_AIR_BYTES);
            if (--dead_air == 0)
            {
                mod->failures++;
                return 0;
            }
        }

        len = read (s->fd, s->read_buffer, s->read_buffer_len);

        if (len == -1)
        {
            LOG_ERROR1("Error reading from audio device: %s", strerror(errno));
            input_free_buffer (ib);
            return 0;
        }
        if ((unsigned)len < s->read_buffer_len)
            LOG_DEBUG1 ("Read return short length %d", len);

        ib->samples = len/(ib->channels*2);
        uninterleave_pcm_le ((signed char*)s->read_buffer, ib->channels, ib->samples, ib->buf);

        if (s->newtrack)
        {
            LOG_DEBUG0 ("metadata updates flagged");
            metadata_thread_signal (mod, ib);
            ib->critical = 1;
            s->newtrack = 0;
        }

        if (metadata_update_signalled || ices_config->shutdown || move_to_next_input)
        {
            LOG_DEBUG0("marking buffer eos");
            s->newtrack = 1;
            ib->eos = 1;
            if (move_to_next_input || ices_config->shutdown)
                s->user_terminated = 1;
        }
        input_adv_sleep ((uint64_t)ib->samples * 1000000 / s->samplerate);
        send_for_processing (mod, ib);
    }
            
    return 0;
}



void oss_close_module (input_module_t *mod)
{
	im_oss_state *s = mod->internal;

    LOG_INFO0("Closing OSS module");
    if (s->fd > -1)
    {
       close (s->fd);
       s->fd = -1;
    }
}

void oss_shutdown_module (input_module_t *mod)
{
	im_oss_state *s = mod->internal;

    LOG_INFO0 ("Shutdown OSS module");
    free (s->read_buffer);
    free (s);
    mod->internal = NULL;
}



int oss_init_module(input_module_t *mod)
{
    im_oss_state *s;
    module_param_t *current;

    mod->name = "OSS";
    mod->type = ICES_INPUT_PCM;
    mod->getdata = oss_read;
    mod->release_input_buffer = oss_return_buffer;
    mod->initialise_buffer = oss_initialise_buffer;
    mod->free_input_buffer = oss_free_buffer;
    mod->buffer_count = ices_config->runner_count*5 + 25;
    mod->prealloc_count = 2 + ices_config->runner_count*2;

    mod->internal = calloc(1, sizeof(im_oss_state));
    if (mod->internal == NULL)
        return -1;

	s = mod->internal;

	s->fd = -1; /* Set it to something invalid, for now */
	s->samplerate = 44100; /* Defaults */
	s->channels = 2; 

	current = mod->module_params;

	while(current)
	{
		if(!strcmp(current->name, "rate"))
			s->samplerate = atoi(current->value);
		else if(!strcmp(current->name, "channels"))
			s->channels = atoi(current->value);
		else if(!strcmp(current->name, "device"))
			s->devicename = current->value;
		else if(!strcmp(current->name, "samples"))
			s->samples = atoi(current->value);
		else if(!strcmp(current->name, "metadatafilename"))
			mod->metadata_filename = current->value;
		else if (!strcmp(current->name, "comment"))
                ;
        else
			LOG_WARN1("Unknown parameter %s for oss module", current->name);

		current = current->next;
	}
    if (s->samples == 0)
        s->samples = s->samplerate/5;
    
    s->aux_data = s->samplerate * s->channels * 2;
    s->default_len = s->samples * s->channels * 2;
    s->read_buffer_len = s->samples*2*s->channels;
    s->read_buffer = malloc (s->read_buffer_len);

    LOG_INFO0 ("Module OSS initialised");
    return 0;
}


int oss_open_module(input_module_t *mod)
{
    im_oss_state *s = mod->internal;
	int format = AFMT_S16_LE;
	int channels, rate;
    int flags;

	/* First up, lets open the audio device */
    channels = s->channels;
    rate = s->samplerate;

    if (ices_config->shutdown)
        return -1;

	if((s->fd = open(s->devicename, O_RDONLY, 0)) == -1)
	{
		LOG_ERROR2("Failed to open audio device %s: %s", s->devicename, strerror(errno));
		goto fail;
	}
    flags = fcntl (s->fd, F_GETFL);
    if (flags == -1)
    {
        LOG_ERROR0 ("Cannot get file state");
        goto fail;
    }
    flags &= ~O_NONBLOCK;
    if (fcntl(s->fd, F_SETFL, flags) == -1)
    {
        LOG_ERROR0 ("Cannot set dsp file state");
        goto fail;
    }
	/* Now, set the required parameters on that device */

	if(ioctl(s->fd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		LOG_ERROR2("Failed to set sample format on audio device %s: %s", 
				s->devicename, strerror(errno));
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
				s->devicename, strerror(errno));
		goto fail;
	}
	if(channels != s->channels)
	{
		LOG_ERROR0("Couldn't set number of channels");
		goto fail;
	}

	rate = s->samplerate;
	if(ioctl(s->fd, SNDCTL_DSP_SPEED, &rate) == -1)
	{
		LOG_ERROR2("Failed to set sampling rate on audio device %s: %s", 
				s->devicename, strerror(errno));
		goto fail;
	}
	if(rate != s->samplerate)
	{
        LOG_WARN2("Could not set sampling rate to %d, using %d instead", s->samplerate, rate);
        s->samplerate = rate;
	}

	/* We're done, and we didn't fail! */
	LOG_INFO3("Opened audio device %s at %d channel(s), %d Hz", 
			s->devicename, channels, rate);

    s->newtrack = 1;
	return 0;
fail:
	oss_close_module(mod); /* safe, this checks for valid contents */
	return -1;
}




