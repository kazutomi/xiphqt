/* input.c
 *  - Main producer control loop. Fetches data from input modules, and controls
 *    submission of these to the runner threads. Timing control happens here.
 *    originally based on the work by Michael Smith
 * 
 * Copyright (c) 2001-4 Karl Heyes <karl@xiph.org>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "runner.h"
#include "thread/thread.h"
#include "timing/timing.h"
#include "cfgparse.h"
#include "metadata.h"
#include "inputmodule.h"
#include "im_playlist.h"
#include "im_pcm.h"
#include "signals.h"

#ifdef HAVE_OSS_AUDIO
#include "im_oss.h"
#endif

#ifdef HAVE_ALSA_AUDIO
#include "im_alsa.h"
#endif

#ifdef HAVE_JACK
#include "im_jack.h"
#endif

#ifdef HAVE_SUN_AUDIO
#include "im_sun.h"
#endif

#ifdef _WIN32
typedef __int64 int64_t
typedef unsigned __int64 uint64_t
#endif

#define MODULE "input/"
#include "logging.h"

#define MAX_BUFFER_FAILURES 15

char dead_audio [DEAD_AIR_BYTES];

module_t modules[] = {
	{ "playlist",   playlist_init_module,   playlist_open_module,  playlist_close_module,  playlist_shutdown_module },
    { "pcm",        pcm_initialise_module,  pcm_open_module,       pcm_close_module,       pcm_shutdown_module },
#ifdef HAVE_ALSA_AUDIO
	{ "alsa",       alsa_init_module,       alsa_open_module,      alsa_close_module,      alsa_shutdown_module },
#endif
#ifdef HAVE_JACK
	{ "jack",       jack_init_module,       jack_open_module,      jack_close_module,      jack_shutdown_module },
#endif
#ifdef HAVE_OSS_AUDIO
	{ "oss",        oss_init_module,        oss_open_module,       oss_close_module,       oss_shutdown_module },
#endif
#ifdef HAVE_SUN_AUDIO
	{ "sun",        sun_init_module,        sun_open_module, sun_close_module, sun_shutdown_module },
#endif
	{NULL,NULL,NULL,NULL,NULL}
};


static timing_control control;

void input_sleep (void)
{
    signed long sleep;

    sleep = control.senttime - ((timing_get_time() - control.starttime) *1000);

    if (sleep > 1000000)
    {
        LOG_WARN1 ("Sleeping for over 1 second (%ld), resetting timer", sleep);
        control.starttime = timing_get_time();
        control.senttime = 0;
        sleep = 0;
    }
#if 0
    printf ("sentime is %lld\n", control.senttime/1000);
    printf ("Sleeping for %ld\n", sleep);
#endif
    /* only sleep if the difference is above 5ms, might as well use the timeslice */
    if (sleep > 5000)
    {
        /* printf ("Sleeping for %ld\n", sleep);
        LOG_DEBUG1("sleep is %ld\n", sleep); */
        thread_sleep((unsigned long)sleep);
    }
}

void input_adv_sleep (unsigned long adv)
{
    control.senttime += adv; /* in uS */
    /* printf ("Adding %lu\n", adv); */
}


void uninterleave_pcm_le (signed char  *src, unsigned channels, unsigned samples, float **dest)
{
    unsigned int i,j;
    float *dst;
    signed char *from = src;
    for (j=0 ; j<channels ; j++)
    {
        dst = dest[j];
        from = src+(j<<1);
        for(i=samples; i ; i--)
        {
            *dst = (((*(from+1)) << 8) | (0x00ff&(int)(*from))) / 32768.f;
            dst++;
            from += channels*2;
        }
    }
}



void uninterleave_pcm_be (signed char  *src, unsigned channels, unsigned samples, float **dest)
{
    unsigned int i,j;
    float *dst;
    signed char *from = src;
    for (j=0 ; j<channels ; j++)
    {
        dst = dest[j];
        from = src+(j<<1);
        for(i=samples; i ; i--)
        {
            *dst = (((*from) << 8) | (0x00ff&(int)(*(from+1)))) / 32768.f;
            dst++;
            from += channels*2;
        }
    }
}

static int initialise_input_modules (void)
{
    input_module_t *mod;

    mod = ices_config->inputs;
    while (mod)
    {
        int i;
        input_buffer *ib;

        if (mod->initialise_module (mod) < 0)
            return -1;

        mod->free_list_tail = &mod->free_list;

        /* set the global minimum if need be */
        if (mod->buffer_count < 2)
            mod->buffer_count = 2;

        if (mod->prealloc_count < 2)
            mod->prealloc_count = 2;
        if (mod->prealloc_count > mod->buffer_count)
            mod->prealloc_count = mod->buffer_count;

        /* create the pre-allocated buffers */
        for (i=mod->prealloc_count; i ; i--)
        {
            ib = calloc (1, sizeof (input_buffer));
            if (ib == NULL)
                return -1;
            if (mod->initialise_buffer && mod->initialise_buffer (mod, ib) < 0)
            {
                free (ib);
                return -1;
            }

            *mod->free_list_tail = ib;
            mod->free_list_tail = &ib->next;
        }
        LOG_DEBUG4 ("Module %d (%s) has pre-allocated %d buffers out of %d",
                mod->id, mod->name, mod->prealloc_count, mod->buffer_count);

        mod = mod->next;
    }

    return 0;
}


void input_free_buffer(input_buffer *ib)
{
    input_module_t *mod;
    if (ib == NULL)
        return;

    mod = ib->mod;

    /* LOG_DEBUG1 ("releasing %llu", ib->serial); */
    if (ib->serial != mod->expected)
    {
        LOG_DEBUG2("expected %lld, saw %lld", mod->expected, ib->serial);
        mod->expected = ib->serial + 1;
    }
    else
        mod->expected++;
    metadata_free (ib->metadata);
    ib->metadata = NULL;
    if (mod->release_input_buffer)
        mod->release_input_buffer (mod, ib);
    ib->next = NULL;
    ib->critical = 0;
    ib->eos = 0;
    mod->released_serial++;
#if 0
    /* every so often, actually free a buffer, so that memory usage is reduced  */
    if (mod->allotted_serial - mod->released_serial > mod->prealloc_count)
    /* if ((ib->serial & (uint64_t)255) == 255) */
    {
        if (mod->free_input_buffer)
            mod->free_input_buffer (mod, ib);
        free (ib);
        /* LOG_DEBUG1 ("removed buffer, length now %d", mod->allotted_serial - mod->released_serial); */
    }
    else
#endif
    {
        *mod->free_list_tail = ib;
        mod->free_list_tail = &ib->next;
    }

    return;
}

input_buffer *input_alloc_buffer (input_module_t *mod)
{
    input_buffer *ib;

    do
    {
        ib = mod->free_list;
        if (ib == NULL || ib->next == NULL)
        {
            unsigned in_use = mod->allotted_serial - mod->released_serial;
            if (in_use > mod->buffer_count)
                return NULL;
            ib = calloc (1, sizeof (input_buffer));
            if (ib == NULL)
                return NULL;
            if (mod->initialise_buffer && mod->initialise_buffer (mod, ib) < 0)
            {
                free (ib);
                return NULL;
            }
            mod->buffer_alloc++;
            mod->delay_buffer_check = 1000;
            /* LOG_DEBUG1 ("added buffer, length now %d", mod->allotted_serial - mod->released_serial); */
        }
        else
        {
            mod->free_list = ib->next;
            if (mod->delay_buffer_check == 0 && mod->buffer_alloc > mod->prealloc_count)
            {
                /* LOG_DEBUG0("reducing free list"); */
                if (mod->free_input_buffer)
                    mod->free_input_buffer (mod, ib);
                mod->buffer_alloc--;
                ib = NULL;
                mod->delay_buffer_check = 500;
                continue;
            }
            if (mod->delay_buffer_check)
                mod->delay_buffer_check--;
        }
    }
    while (ib == NULL);

    ib->next = NULL;
    ib->serial = mod->allotted_serial++;
    /* LOG_DEBUG1 ("issue buffer id %llu", ib->serial); */

    return ib;
}



void process_input(input_module_t *mod)
{
	if (!mod)
	{
		LOG_ERROR0("NULL input module");
		return;
	}

    move_to_next_input = 0;
    control.samples = control.oldsamples = 0;

    while (mod->getdata (mod))
        ;
}

static input_module_t *open_next_input_module (input_module_t *mod)
{
    input_module_t *next_mod = mod;

    if (ices_config->shutdown || mod == NULL)
        return NULL;
    do
    {
        LOG_DEBUG1 ("checking module %d", next_mod->id);
        if (next_mod->failures < 10)
        {
            if (next_mod->open_module)
            {
                time_t start = time (NULL);

                if (next_mod->start+2 > start)
                {
                    LOG_WARN0("restarted input within 2 seconds, it probably failed");
                    next_mod->failures++;
                }
                if (next_mod->open_module (next_mod) == 0)
                {
                    next_mod->start = start;
                    next_mod->failures = 0;
                    ices_config->next_track = 0;
                    return next_mod;
                }
            }
            else
                next_mod->failures++;
        }
        else
            LOG_WARN2 ("Too many failures on input module %d (%s)", next_mod->id, next_mod->name);

        next_mod = next_mod->next;

    } while (next_mod != mod && next_mod);

    return NULL;
}


static void free_modules()
{
    input_module_t *mod = ices_config->inputs;

    LOG_DEBUG0 ("freeing up module storage");
    while (mod)
    {
        int i = 0;

        LOG_DEBUG1 ("freeing up module %s", mod->name);
        while (mod->free_list)
        {
            input_buffer *ib = mod->free_list;
            mod->free_list = ib->next;
            if (mod->free_input_buffer)
                mod->free_input_buffer (mod, ib);
            free (ib);
            i++;
        }
        LOG_DEBUG1 (" %d buffers freed", i);
        mod->free_list = NULL;
        mod->free_list_tail = NULL;
        mod = mod->next;
    }
}

void send_for_processing (input_module_t *mod, input_buffer *ib)
{
#if 0
    if (ib->critical) printf ("BOS seen\n");
    if (ib->eos) printf ("EOS seen\n");
#endif
    send_to_runner (ices_config->runners, ib);
}


void *input_loop(void *arg)
{
    input_module_t *mod;
    struct runner *r = ices_config->runners;

    start_runners();
    thread_sleep (300000);

    if (initialise_input_modules () < 0)
    {
        printf ("Unable to initialise input modules\n");
        return NULL;
    }

    /* start the clock */
    control.starttime = timing_get_time();

    mod = open_next_input_module (ices_config->inputs);

    while (ices_config->shutdown == 0)
    {
        if (mod == NULL)
            break;

        process_input (mod);

        if (mod->close_module)
        {
            LOG_INFO0("Closing input module");
            mod->close_module (mod);
        }

        mod = mod->next;
        if (mod == NULL)
        {
            if (ices_config->input_once_thru)
            {
               ices_config->shutdown = 1;
               break;
            }
            else
                mod = ices_config->inputs;
        }

        mod = open_next_input_module (mod);
    }

	LOG_DEBUG0("All input stopped, shutting down.");

    runner_close (r);
    thread_sleep (200000);
    free_modules();

    return NULL;
}

