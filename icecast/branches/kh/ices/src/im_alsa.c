/* im_alsa.c
 * - Raw PCM input from ALSA devices
 *
 * $Id: im_alsa.c,v 1.1 2002/12/29 10:28:30 msmith Exp $
 *
 * by Jason Chu <jchu@uvic.ca>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
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
#include <signals.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "cfgparse.h"
#include "metadata.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#include "im_alsa.h"

#define MODULE "input-alsa/"
#include "logging.h"

#define SAMPLES 8192


static int alsa_initialise_buffer (input_module_t *mod, input_buffer *ib)
{
	im_alsa_state *s = mod->internal;
    int i;
    float **ptr;

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
    ib->samplerate = s->rate;
    ib->mod = mod;
    return 0;
}


void alsa_close_module(input_module_t *mod)
{
	if (mod)
	{
		if (mod->internal)
		{
			im_alsa_state *s = mod->internal;
			if (s->fd != NULL)
				snd_pcm_close(s->fd);
			free(s);
		}
	}
}


void alsa_shutdown_module (input_module_t *mod)
{
    im_alsa_state *s = mod->internal;

    LOG_INFO0 ("Shutdown ALSA module");
    free (s);
    mod->internal = NULL;
}


/* Core streaming function for this module
 * This is what actually produces the data which gets streamed.
 *
 */
static int alsa_read(input_module_t *mod) 
{
	int result;
	im_alsa_state *s = mod->internal;
    int dead_air;
    input_buffer *ib;

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
            result = snd_pcm_readi(s->fd, dead_audio, DEAD_AIR_BYTES/(2*s->channels));
            if (--dead_air == 0)
            {
                mod->failures++;
                return 0;
            }
        }

        result = snd_pcm_readi (s->fd, s->read_buffer, s->samples);

        /* check what this condition means */
        if (result == -EPIPE)
        {
            snd_pcm_prepare(s->fd);
            input_free_buffer (ib);
            return 0;
        }
        if (result == -EBADFD)
        {
            LOG_ERROR0("Bad descriptor passed to snd_pcm_readi");
            input_free_buffer (ib);
            return 0;
        }
        if (s->newtrack)
        {
            LOG_DEBUG0("setting buffer critical");
            metadata_thread_signal (mod, ib);
            ib->critical = 1;
            s->newtrack = 0;
        }
        if (metadata_update_signalled || ices_config->shutdown || move_to_next_input)
        {
            LOG_DEBUG0("Sending EOS");
            s->newtrack = 1;
            ib->eos = 1;
            if (move_to_next_input || ices_config->shutdown)
                s->user_terminated = 1;
        }

        ib->samples = result;
        uninterleave_pcm_le ((signed char*)s->read_buffer, ib->channels, ib->samples, ib->buf);

        input_adv_sleep ((uint64_t)ib->samples * 1000000 / s->rate);
        send_for_processing (mod, ib);
        input_sleep ();
    }

	return 0;
}

int alsa_init_module(input_module_t *mod)
{
	im_alsa_state *s;
	module_param_t *current;
	char *device = "plughw:0,0"; /* default device */
	int channels, rate;
    unsigned int samples;

    mod->name = "ALSA";
	mod->type = ICES_INPUT_PCM;
	mod->subtype = INPUT_PCM_LE_16;
	mod->getdata = alsa_read;
    /* mod->release_input_buffer = alsa_return_buffer; */
    mod->initialise_buffer = alsa_initialise_buffer;
    mod->buffer_count = ices_config->runner_count*10 + 5;
    mod->prealloc_count = ices_config->runner_count * 4;

	mod->internal = calloc(1, sizeof(im_alsa_state));
    if (mod->internal == NULL)
        return -1;
	s = mod->internal;

	s->fd = NULL; /* Set it to something invalid, for now */
	rate = 44100; /* Defaults */
	channels = 2; 
    samples = SAMPLES;
    s->periods = 2;
    s->buffer_time = 500000;

	current = mod->module_params;

	while(current)
	{
		if(!strcmp(current->name, "rate"))
			rate = atoi(current->value);
		else if(!strcmp(current->name, "channels"))
			channels = atoi(current->value);
		else if(!strcmp(current->name, "device"))
			device = current->value;
		else if(!strcmp(current->name, "samples"))
			samples = atoi(current->value);
		else if(!strcmp(current->name, "periods"))
			s->periods = atoi(current->value);
		else if(!strcmp(current->name, "buffer-time"))
			s->buffer_time = atoi(current->value);
		else if(!strcmp(current->name, "metadatafilename"))
			mod->metadata_filename = current->value;
		else
			LOG_WARN1("Unknown parameter %s for alsa module", current->name);

		current = current->next;
	}
    s->rate = rate;
    s->channels = channels;
    s->samples = samples;
    s->device = device;

    s->read_buffer_len = s->samples*2*s->channels;
    s->read_buffer = malloc (s->read_buffer_len);

    LOG_INFO0("ALSA driver initialised");

    return 0;
}

int alsa_open_module(input_module_t *mod)
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	im_alsa_state *s = mod->internal;
    int err;
    unsigned int exact_rate;
    int dir = 1;

	snd_pcm_hw_params_alloca(&hwparams);

	if ((err = snd_pcm_open(&s->fd, s->device, stream, 0)) < 0)
	{
		LOG_ERROR2("Failed to open audio device %s: %s", s->device, snd_strerror(err));
		goto fail;
	}

	if ((err = snd_pcm_hw_params_any(s->fd, hwparams)) < 0)
	{
		LOG_ERROR1("Failed to initialize hwparams: %s", snd_strerror(err));
		goto fail;
	}
	if ((err = snd_pcm_hw_params_set_access(s->fd, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		LOG_ERROR1("Error setting access: %s", snd_strerror(err));
		goto fail;
	}
	if ((err = snd_pcm_hw_params_set_format(s->fd, hwparams, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		LOG_ERROR1("Couldn't set sample format to SND_PCM_FORMAT_S16_LE: %s", snd_strerror(err));
		goto fail;
	}
	if ((err = snd_pcm_hw_params_set_channels(s->fd, hwparams, s->channels)) < 0)
	{
		LOG_ERROR1("Error setting channels: %s", snd_strerror(err));
		goto fail;
	}

    exact_rate = s->rate;
    err = snd_pcm_hw_params_set_rate_near(s->fd, hwparams, &exact_rate, 0);
    if (err < 0)
    {
        LOG_ERROR2("Could not set sample rate to %d: %s", exact_rate, snd_strerror(err));
        goto fail;
    }
    if (exact_rate != s->rate)
    {
        LOG_WARN2("samplerate %d Hz not supported by your hardware try using "
                "%d instead", s->rate, exact_rate);
        goto fail;
    }

    if ((err = snd_pcm_hw_params_set_buffer_time_near(s->fd, hwparams, &s->buffer_time, &dir)) < 0)
    {
        LOG_ERROR2("Error setting buffer time %u: %s", s->buffer_time, snd_strerror(err));
        goto fail;
    }
    if ((err = snd_pcm_hw_params_set_periods(s->fd, hwparams, s->periods, 0)) < 0)
    {
        LOG_ERROR2("Error setting %u periods: %s", s->periods, snd_strerror(err));
        goto fail;
    }

	if ((err = snd_pcm_hw_params(s->fd, hwparams)) < 0)
	{
		LOG_ERROR1("Error setting HW params: %s", snd_strerror(err));
		goto fail;
	}
    s->newtrack = 1;

	/* We're done, and we didn't fail! */
	LOG_INFO1("Opened audio device %s", s->device);
    LOG_INFO4("with %d channel(s), %d Hz, buffer %u ms (%u periods)",
			s->channels, s->rate, s->buffer_time/1000, s->periods);

	return 0;

fail:
	alsa_shutdown_module(mod); /* safe, this checks for valid contents */
	return -1;
}


