/* im_sun.c
 * - Raw PCM input from Solaris audio devices
 *
 * $Id: im_sun.c,v 1.7 2002/08/03 15:05:39 msmith Exp $
 *
 * by Ciaran Anscomb <ciarana@rd.bbc.co.uk>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@xiph.org>
 * Copyright (c) 2003 karl Heyes <karl@xiph.org>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#define SAMPLES    8192


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ogg/ogg.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#include <fcntl.h>


#include "thread/thread.h"
#include "stream.h"
#include "signals.h"
#include "inputmodule.h"
#include "metadata.h"

#include "im_sun.h"

#define MODULE "input-sun/"
#include "logging.h"

#define BUFSIZE 8192

#define MAX_DEAD_AUDIO_BYTES 48000*2*2


static void sun_return_buffer (input_module_t *mod __attribute__((unused)), input_buffer *ib)
{
    ib->critical = 0;
    ib->eos = 0;
    return;
}
                                                                                                                                 
                                                                                                                                 
static void sun_free_buffer (input_module_t *mod, input_buffer *ib)
{
    im_sun_state *s = mod->internal;
    float **ptr;
    int i;
                                                                                                                                 
    ptr = ib->buf;
    for (i=s->device_info.record.channels; i; i--)
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
                                                                                                                                 
static int sun_initialise_buffer (input_module_t *mod, input_buffer *ib)
{
    im_sun_state *s = mod->internal;
    float **ptr;
    int i;
                                                                                                                                 
    if ((ib->buf = calloc (1, s->default_len)) == NULL)
        return -1;
                                                                                                                                 
    if ((ib->buf = calloc (s->device_info.record.channels, sizeof (float*))) == NULL)
        return -1;
    ptr = ib->buf;
    for (i=s->device_info.record.channels ; i; i--)
    {
        if ((*ptr = calloc (sizeof(float), s->samples)) == NULL)
            return -1;
        ptr++;
    }
                                                                                                                                 
    ib->type = ICES_INPUT_PCM;
#ifdef WORDS_BIGENDIAN
    ib->subtype = INPUT_PCM_BE_16;
#else
    ib->subtype = INPUT_PCM_LE_16;
#endif
    ib->critical = 0;
    ib->channels = s->device_info.record.channels;
    ib->samplerate = s->device_info.record.sample_rate;
    ib->mod = mod;
                                                                                                                                 
    return 0;
}



void sun_close_module (input_module_t *mod)
{
    im_sun_state *s = mod->internal;
                                                                                                                                 
    LOG_INFO0("Closing Sun audio module");
    if (s->fd > -1)
    {
       close (s->fd);
       s->fd = -1;
    }
}

void sun_shutdown_module (input_module_t *mod)
{
    im_sun_state *s = mod->internal;
                                                                                                                                 
    LOG_INFO0 ("Shutdown Sun audio module");
    free (s);
    mod->internal = NULL;
}
                                                                                                                                 


/* Core streaming function for this module
 * This is what actually produces the data which gets streamed.
 *
 */
static int sun_read(input_module_t *mod)
{
    im_sun_state *s = mod->internal;
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
            read (s->fd, dead_audio, MAX_DEAD_AUDIO_BYTES);
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

        ib->samples = len/(ib->channels*2);

#ifdef __sparc
        uninterleave_pcm_be ((signed char*)s->read_buffer, ib->channels, ib->samples, ib->buf);
#else
        uninterleave_pcm_le ((signed char*)s->read_buffer, ib->channels, ib->samples, ib->buf);
#endif

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
        input_adv_sleep ((uint64_t)ib->samples * 1000000 / s->device_info.record.sample_rate);
        send_for_processing (mod, ib);
    }

    return 0;
}



int sun_init_module(input_module_t *mod)
{
    im_sun_state *s;
    module_param_t *current;
    char *device = "/dev/audio"; /* default device */
    int sample_rate = 44100;
    int channels = 2;
    int samples = SAMPLES;

    mod->name = "Sun Audio";

    mod->type = ICES_INPUT_PCM;
    mod->subtype = INPUT_PCM_LE_16;
    mod->getdata = sun_read;
    mod->initialise_buffer = sun_initialise_buffer;
    mod->free_input_buffer = sun_free_buffer;
    mod->buffer_count = ices_config->runner_count*10 + 5;
    mod->prealloc_count = ices_config->runner_count * 4;

    mod->internal = calloc(1, sizeof(im_sun_state));
    do
    {
        if (mod->internal == NULL)
            break;
        s = mod->internal;

        s->fd = -1; /* Set it to something invalid, for now */

        current = mod->module_params;

        while (current) {
            if (!strcmp(current->name, "rate"))
                sample_rate = atoi(current->value);
            else if (!strcmp(current->name, "channels"))
                channels = atoi(current->value);
            else if (!strcmp(current->name, "device"))
                device = current->value;
            else if (!strcmp(current->name, "samples"))
                samples = atoi (current->value);
            else if(!strcmp(current->name, "metadatafilename"))
                mod->metadata_filename = current->value;
            else
                LOG_WARN1("ignored parameter %s for sun module", current->name);
            current = current->next;
        }

        /* Try and set up what we want */
        AUDIO_INITINFO(&s->device_info);
        s->device_info.record.sample_rate = sample_rate;
        s->device_info.record.channels = channels; 
        s->device_info.record.precision = 16;
        s->device_info.record.encoding = AUDIO_ENCODING_LINEAR;
        s->device_info.record.port = AUDIO_LINE_IN;
        s->device_info.record.pause = 0;
        s->device = device;
        s->samples = samples;
        s->default_len = samples * 2 * channels;
        s->read_buffer_len = s->samples*2*channels;
        s->read_buffer = malloc (s->read_buffer_len);

        return 0;
    }
    while (0);
    /* error, need to cleanup */
    if (mod->internal)
    {
        free (mod->internal);
        mod->internal = NULL;
    }
    return -1;
}

int sun_open_module (input_module_t *mod)
{
    im_sun_state *s = mod->internal;
	int sample_rate = s->device_info.record.sample_rate;
	int channels = s->device_info.record.channels;

    /* First up, lets open the audio device */
    do
    {
        if ((s->fd = open(s->device, O_RDONLY, 0)) < 0)
        {
            LOG_ERROR2("Failed to open audio device %s: %s", s->device, strerror(errno));
            break;
        }

        if (ioctl (s->fd, AUDIO_SETINFO, &s->device_info) < 0)
        {
            LOG_ERROR2("Failed to configure audio device %s: %s",
                    s->device, strerror(errno));
            break;
        }
#ifdef __sun
        ioctl(s->fd, I_FLUSH, FLUSHR);
#endif
#ifdef __OpenBSD__
        ioctl(s->fd, AUDIO_FLUSH, NULL);
#endif

        /* Check all went according to plan */
        if (s->device_info.record.sample_rate != sample_rate)
        {
            LOG_ERROR0("Couldn't set sampling rate");
            break;
        }
        if (s->device_info.record.channels != channels) {
            LOG_ERROR0("Couldn't set number of channels");
            break;
        }
        if (s->device_info.record.precision != 16)
        {
            LOG_ERROR0("Couldn't set 16 bit precision");
            break;
        }
        if (s->device_info.record.encoding != AUDIO_ENCODING_LINEAR)
        {
            LOG_ERROR0("Couldn't set linear encoding");
            break;
        }

        /* We're done, and we didn't fail! */
        LOG_INFO3("Opened audio device %s at %d channel(s), %d Hz", 
                s->device, channels, sample_rate);

        s->newtrack = 1;
        return 0;
    }
    while (0);

    sun_close_module(mod); /* safe, this checks for valid contents */
    return NULL;
}


