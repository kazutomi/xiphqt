/* runner.c
 * runner processing. The process involved in by each runner
 *
 * Copyright (c) 2002-3 Karl Heyes <karl@xiph.org>
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
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <shout/shout.h>

#include "cfgparse.h"
#include "runner.h"
#include "stream.h"
#include "net/resolver.h"
#include "signals.h"
#include "thread/thread.h"
#include "reencode.h"
#include "encode.h"
#include "audio.h"
#include "inputmodule.h"

#include <om_file.h>
#include <om_shout.h>

#define DEFAULT_STREAM_NAME "unnamed ices stream"
#define DEFAULT_STREAM_GENRE "ices unset"
#define DEFAULT_STREAM_DESCRIPTION "no description set"
#define DEFAULT_QUALITY 3
#define DEFAULT_DOWNMIX 0
#define DEFAULT_RESAMPLE 0


#define MODULE "stream/"
#include "logging.h"

#define MAX_ERRORS 10

/*
 * Close channel to next runner 
 */

void runner_close (struct runner *run)
{
    if (run)
    {
        LOG_DEBUG1 ("Runner thread %d shutting down", run->id);
        close (run->fd[1]);
        run->fd[1] = -1;
        thread_join (run->thread);
    }
}

/*
 * send the input buffer to the next runner or if none 
 * free the buffer.
 *
 * The queue does not do locking, the assumption is that
 * there is always one on the queue, so the tail will never
 * be referring to the head.
 *
 */

int send_to_runner (struct runner *run, input_buffer *buffer)
{
    if (run) 
    {
        buffer->next = NULL;

#ifdef USE_PIPES
        if (write (run->fd[1], &buffer, sizeof (buffer)) < (ssize_t)sizeof (buffer))
        {
            LOG_WARN0 ("unable to write to runner");
            return -1;
        }
#else
        *run->pending_tail = buffer;
        run->pending_tail = &buffer->next;
        thread_cond_signal (&run->data_available);
#endif

        return 0;
    } 
    input_free_buffer (buffer);
    return 0;
}


/* retreive input buffer from runner queue. Companion
 * to previous function to remove from the queue, again
 * make sure there is at least one on the queue.
 */

input_buffer *runner_wait_for_data(struct runner *run)
{
    input_buffer *ib;

#ifdef USE_PIPES
    if (read (run->fd[0], &ib, sizeof (ib)) < (ssize_t)sizeof (ib))
        return NULL;
#else
    while (run->pending == NULL || (run->pending && run->pending->next == NULL))
    {
        thread_cond_wait (&run->data_available);
        if (ices_config->shutdown)
            return NULL;
    }

    ib = run->pending;
    run->pending = ib->next;
    ib->next = NULL;
#endif

    return ib;
}


void stream_cleanup (struct instance *stream)
{

    if (stream->ops && stream->ops->flush_data)
    {
        LOG_DEBUG1 ("Cleanup of stream %d required", stream->id);
        stream->ops->flush_data (stream);
    }
    output_clear (&stream->output);
}



/* process a block of data. This may be skipped, or may even kick off
 * a new connection.
 * 
 */
static void add_to_stream (struct instance *stream, input_buffer *ib)
{
    if (ib->critical)
        process_critical (stream, ib);

    /* LOG_DEBUG1 ("ops is %p", stream->ops); */
    if (stream->ops && stream->ops->process_buffer(stream, ib) < 0)
        stream_cleanup (stream);

    if (ib->type == ICES_INPUT_NONE)
        return;
    /* the normal end of stream flush */
    if (ib->eos && stream->ops)
    {
        if (stream->ops->flush_data)
        {
            LOG_DEBUG1("stream flushed due to EOS [%d]", stream->id);
            stream->ops->flush_data (stream);
        }
        /* EOS seen and handled so disable further processing until
         * another start of stream is sent.  */
        stream->ops = NULL;
    }

    return;
}

static struct instance *_allocate_instance (void)
{
    struct instance *instance = (struct instance *)calloc(1, sizeof(struct instance));
    static int id = 1;

    if (instance == NULL)
        return NULL;

    instance->resampleoutrate = DEFAULT_RESAMPLE;
    instance->passthru = 0;

    instance->encode_settings.quality = DEFAULT_QUALITY;
    instance->downmix = DEFAULT_DOWNMIX;

    instance->id = id++;
    instance->next = NULL;

    return instance;
}



int parse_instance (xmlNodePtr node, void *arg)
{
    struct runner *run = arg;
    struct instance *instance = _allocate_instance();

    while (instance)
    {
        struct cfg_tag  instance_tags[] =
        {
            { "name",           get_xml_string,     &instance->output.name },
            { "genre",          get_xml_string,     &instance->output.genre },
            { "description",    get_xml_string,     &instance->output.description },
            { "url",            get_xml_string,     &instance->output.url },
            { "downmix",        get_xml_bool,       &instance->downmix },
            { "passthru",       get_xml_bool,       &instance->passthru},
            { "passthrough",    get_xml_bool,       &instance->passthru},
            { "resample",       parse_resample,     &instance->resampleoutrate },
            { "encode",         parse_encode,       &instance->encode_settings },
            { "savestream",     parse_savefile,     &instance->output },
            { "savefile",       parse_savefile,     &instance->output },
            { "shout",          parse_shout,        &instance->output },
            { NULL, NULL, NULL }
        };

        /* config should be derived from runner */
        xmlMemoryDump();
        if (ices_config->stream_name)
            instance->output.name = xmlStrdup (ices_config->stream_name);
        if (ices_config->stream_genre)
            instance->output.genre = xmlStrdup (ices_config->stream_genre);
        if (ices_config->stream_description)
            instance->output.description = xmlStrdup (ices_config->stream_description);
        if (ices_config->stream_url)
            instance->output.url = xmlStrdup (ices_config->stream_url);

        if (parse_xml_tags ("instance", node->xmlChildrenNode, instance_tags))
            break;

        if (run->instances == NULL)
            run->instances = instance;
        else
        {
            struct instance *i = run->instances;
            while (i->next != NULL) i = i->next;
            i->next = instance;

        }
        return 0;
    }
    free (instance);

    return -1;
}



void *ices_runner (void *arg)
{
    struct runner *run = arg;
    struct instance *current;

#ifdef HAVE_SCHED_GET_PRIORITY_MAX
    int policy;
    struct sched_param param;

    pthread_getschedparam (pthread_self(), &policy, &param);
    param . sched_priority = sched_get_priority_min (SCHED_OTHER);

    if (pthread_setschedparam (pthread_self(), SCHED_OTHER, &param))
    {
        LOG_ERROR1 ("failed to set priority: %s", strerror (errno));
    }
    else
        LOG_INFO0 ("set priority on runner");
#endif
    LOG_INFO1 ("Runner %d ready", run->id);

    while (1)
    {
        input_buffer *buffer;
        
        buffer = runner_wait_for_data (run);

        if (buffer == NULL)
            break;

        current = run->instances;

        while (current != NULL)
        {
            add_to_stream (current, buffer);
            current = current->next;
        }

        send_to_runner (run->next, buffer);
    }

    runner_close (run->next);
    LOG_DEBUG1 ("Runner thread %d cleaning up streams", run->id);
    current = run->instances;
    while (current)
    {
        struct instance *next;
        next = current->next;
        stream_cleanup (current);
        current = next;
    }
    close (run->fd[0]);
    run->fd[0] = -1;
    run->not_running = 1;
    LOG_DEBUG1 ("Runner thread %d finshed", run->id);
    
    return NULL;
}

struct instance *instance_free (struct instance *instance)
{
    struct instance *next = NULL;
    if (instance)
    {
        next = instance->next;
        /* reencode_free (instance->reenc); */
        free (instance);
    }
    return next;
}


struct runner *config_free_runner(struct runner *run)
{
    struct runner *next = run->next;
    struct instance *instance = NULL;

    while ((instance = run->instances))
    {
        run->instances = instance->next;
        instance_free (instance);
    }

    free (run);

    return next;
}


struct runner *allocate_runner()
{
    static int runner_id = 1;
    struct runner *run = calloc (1,sizeof (struct runner));

    if (run == NULL)
        return NULL;

    run->not_running = 1;

#ifdef USE_PIPES
    pipe (run->fd);
#else
    thread_cond_create (&run->data_available);
    run->pending_tail = &run->pending;
#endif
    run->id = runner_id++;

    return run;
}


int create_runner_thread (struct runner *run)
{
    if (run == NULL)
        return -1;

    run->not_running = 0;

    run->thread = thread_create("runner", ices_runner, run, 0);
    if (run->thread == NULL)
    {
        run->not_running = 1;
        return -1;
    }
    return 0;
}


void start_runners()
{
    struct runner **runnerptr = &ices_config->runners;
    struct runner *run;

    while (*runnerptr != NULL)
    {
        run = *runnerptr;

        if (run->not_running && !ices_config->shutdown)
        {
            LOG_DEBUG0("starting runner");
            create_runner_thread (run);
        }

        runnerptr = &run->next;
    }
}



int parse_runner (xmlNodePtr node, void *arg)
{
    config_t *config = arg;
    struct runner *run = allocate_runner ();

    while (run)
    {
        struct cfg_tag runner_tag[] =
        {
            { "instance", parse_instance, run },
            { NULL, NULL, NULL }
        };
        if (parse_xml_tags ("runner", node->xmlChildrenNode, runner_tag))
            break;

        if (config->runners == NULL)
            config->runners = run;
        else
        {
            struct runner *i = config->runners;
            while (i->next != NULL) i = i->next;
            i->next = run;
        }
        config->runner_count++;
        return 0;
    }
    free (run);  /* separate function */
    return -1;
}

