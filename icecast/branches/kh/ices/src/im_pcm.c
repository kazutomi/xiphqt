/* im_pcm.c
 * - Raw PCM input from a pipe, based on the stdinpcm module.
 *
 * Copyright (c) 2002 Karl Heyes <karl@xiph.org>
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
#include <string.h>
#include <errno.h>
#include <ogg/ogg.h>
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "thread/thread.h"

#include "cfgparse.h"
#include "stream.h"
#include "signals.h"

#include "inputmodule.h"


#include "metadata.h"
#include "im_pcm.h"
#include "timing/timing.h"

#define MODULE "input-pcm/"
#include "logging.h"

#define SAMPLES 4096 

/* #define MAX_DEAD_AUDIO_BYTES 48000*2*2
static char dead_audio[MAX_DEAD_AUDIO_BYTES]; */
static uint64_t skip_bytes;

static int run_script (char *script, im_pcm_state *s)
{
    int fd [2];

    if (pipe (fd) < 0)
    {
        LOG_ERROR0 ("Pipe creation error");
        return -1;
    }
    switch (s->pid = fork ())
    {
    case -1:
        LOG_ERROR2 ("Unable to fork %s (%s)", script, strerror (errno));
        return -1;
    case 0:  /* child */
        LOG_DEBUG1 ("Starting command %s", script);
        close (1);
        dup2 (fd[1], 1);
        close (fd[0]);
        close (fd[1]);
        close (0);     /* We want to detach the keyboard from the script */
        /* execl (script, script, NULL); */
        execve (s->args[0], s->args, NULL);
        LOG_ERROR2 ("Unable to run command %s (%s)", script, strerror (errno));
        close (1);
        _exit (-1);
    default: /* parent */
        s->fd = fd[0];
        close (fd[1]);
        break;
    }
    return 0;
}

static void kill_script (im_pcm_state *s)
{
    LOG_DEBUG1 ("Sending SIGTERM to pid %d", s->pid);
    kill (s->pid, SIGTERM);
    close (s->fd);
    waitpid (s->pid, NULL, 0);
    s->fd = -1;
}


static int wait_for_pcm (im_pcm_state *s)
{
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO (&fds);
    FD_SET (s->fd, &fds);
    tv.tv_sec = s->timeout;
    tv.tv_usec = 0;
    ret = select (s->fd+1, &fds, NULL, NULL, &tv);
    if (ret == 0)
    {
        LOG_WARN0 ("Timeout reading from input");
        s->no_stream = 1;
        return -1;
    }
    if (ret == -1)
    {
        LOG_ERROR1("select failed error %s", strerror (errno));
        s->no_stream = 1;
        return -1;
    }
    /* LOG_DEBUG1 ("data availble %d", sleep_to); */
    return 0;
}


static int pcm_read(input_module_t *mod)
{
	im_pcm_state *s = mod->internal;
    input_buffer *ib = NULL;
    unsigned char *buf;
    unsigned remaining, len;
    int dead_air = 100;

    while (1)
    {
        if (s->terminate)
        {
            s->terminate = 0;
            return 0;
        }
        if (s->error)
        {
            LOG_INFO0("Error reading from stream, switching to next input");
            mod->failures++;
            return 0;
        }
        if (s->no_stream)
        {
            LOG_INFO0("End of input has been reached, switching to next input");
            return 0;
        }
        input_sleep();

        while ((ib = input_alloc_buffer (mod)) == NULL)
        {
            if (dead_air == 100)
                LOG_WARN0 ("will skip input for a short time");
            read (s->fd, dead_audio, DEAD_AIR_BYTES);
            if (--dead_air == 0)
                return 0;
        }

        buf = s->read_buffer;
        remaining = s->read_buffer_len;
        len = 0;
        if(s->newtrack)
        {
            LOG_DEBUG0 ("metadata updates flagged");
            metadata_thread_signal (mod, ib);
            ib->critical = 1;
            s->newtrack = 0;
        }
        if (metadata_update_signalled || ices_config->shutdown || move_to_next_input)
        {
            LOG_DEBUG0("signal or shutdown received");
            s->newtrack = 1;
            ib->eos = 1;
            if (ices_config->shutdown || move_to_next_input)
                s->terminate = 1;
        }

        while (remaining)
        {
            int sent;

            sent = read (s->fd, buf, remaining); /* non blocking */
            if (sent < 0)
            {
                if (errno == EAGAIN && wait_for_pcm (s) == 0)
                    continue;
                s->error = 1;
                break;
            }
            if (sent == 0)
            {
                s->no_stream = 1;
                break;
            }
            remaining -= sent;
            buf += sent;
        }
        len = s->read_buffer_len - remaining;
        if (remaining)
        {
            LOG_INFO0("No more data available");
            input_free_buffer (ib);
            break;
        }

        ib->samples = len/(ib->channels*2);

        uninterleave_pcm_le ((signed char*)s->read_buffer, ib->channels, ib->samples, ib->buf);

        input_adv_sleep ((uint64_t)ib->samples * 1000000 / ib->samplerate);
        send_for_processing (mod, ib);
    }

    return 0;
}


void pcm_close_module(input_module_t *mod)
{
    LOG_INFO0 ("Closing PCM input module");
    if(mod && mod->internal)
    {
        im_pcm_state *s = mod->internal;
        if (s->command && s->fd != -1)
        {
            kill_script (s);
        } 
    }
}


void pcm_shutdown_module(input_module_t *mod)
{
    LOG_INFO0 ("shutdown PCM input module");
    if (mod && mod->internal)
    {
        input_buffer *ib;

        while (1)
        {
            ib = mod->free_list;
            if (ib == NULL)
                break;
            mod->free_list = ib->next;
            free (ib->buf);
            free (ib);
        }
        free (mod->internal);
        mod->internal = NULL;
    }
}


static int pcm_initialise_buffer (input_module_t *mod, input_buffer *ib)
{
    im_pcm_state *s = mod->internal;
    float **ptr;
    int i;

    /* allocate memory for vorbis data */
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


int pcm_initialise_module(input_module_t *mod)
{
    im_pcm_state *s;
    module_param_t *current;

    LOG_INFO0 ("Initialising PCM input module");

    mod->name = "pcm";
    mod->type = ICES_INPUT_PCM;
    mod->getdata = pcm_read;
    mod->initialise_buffer = pcm_initialise_buffer;
    mod->buffer_count = ices_config->runner_count*15 + 5;
    mod->prealloc_count = ices_config->runner_count * 4;

    mod->internal = calloc(1, sizeof(im_pcm_state));
    if (mod->internal == NULL)
        return -1;

    s = mod->internal;

    /* Defaults */
    s->channels = 2; 
    s->samplerate = 44100;
    s->samples = SAMPLES;
    s->timeout = 5;
    s->arg_count = 1;

    current = mod->module_params;

    while(current)
    {
        if(!strcmp(current->name, "rate"))
            s->samplerate = atoi(current->value);
        else if(!strcmp(current->name, "channels"))
            s->channels = atoi(current->value);
        else if(!strcmp(current->name, "metadatafilename"))
            mod->metadata_filename = current->value;
        else if (!strcmp(current->name, "source") || !strcmp(current->name, "command"))
        {
            s->command = current->value;
            s->args[0] = s->command;
        }
        else if (!strcmp(current->name, "arg"))
            if (s->arg_count < MAX_ARGS)
                s->args[s->arg_count++] = current->value;
            else
            {
                LOG_WARN0("too many args, disabling module");
                return -1;
            }
        else if (!strcmp(current->name, "comment"))
            s->command = current->value;
        else if (!strcmp(current->name, "timeout"))
            s->timeout = atoi (current->value);
        else if (!strcmp(current->name, "samples"))
            s->samples = atoi (current->value);
        /* else if (!strcmp(current->name, "lossy"))
            s->sleep = _sleep_add_silence; */
        else if (!strcmp(current->name, "skip"))
            s->skip = atoi (current->value);
        else
            LOG_WARN1("Unknown parameter %s for pcm module", current->name);

        current = current->next;
    }
    s->args[s->arg_count] = NULL;
    s->aux_data = s->samplerate * s->channels * 2;
    s->default_duration = (s->default_len*1000)/s->aux_data;
    s->read_buffer_len = s->samples*2*s->channels;
    s->read_buffer = malloc (s->read_buffer_len);
    LOG_DEBUG1 ("Module PCM using buffers of %d samples", s->samples);

    return 0;
}



int pcm_open_module(input_module_t *mod)
{
    im_pcm_state *s = mod->internal;
    const char *label = "standard input";

    s->fd = 0;
    if (s->command)
    {
        if (run_script (s->command, s) < 0)
        {
            s->fd = -1;
            LOG_ERROR1 ("Failed to start script %s", s->command);
            return -1;
        }
        label = s->command;
    }
    s->newtrack = 1;
    s->silence_start = 0;
    s->error = 0;
    s->no_stream = 0;
    fcntl (s->fd, F_SETFL, O_NONBLOCK);
    skip_bytes = s->skip;
    LOG_INFO0("Retrieving PCM to skip");
    while (skip_bytes)
    {
        unsigned len;
        int ret; 

        if (wait_for_pcm (s) < 0)
        {
            s->terminate = 1;
            break;
        }
        if (skip_bytes > DEAD_AIR_BYTES)
            len = DEAD_AIR_BYTES;
        else
            len = skip_bytes;
        ret = read (s->fd, dead_audio, len);
        if (ret ==0)
            break;
        if (ret > 0)
            skip_bytes -= ret;
        else
        {
            s->terminate = 1;
            break;
        }
    }
    LOG_INFO1("Retrieving PCM from %s", label);

    return 0;
}


