/* stream.h
 * - Core streaming functions/main loop.
 *
 * $Id: stream.h,v 1.2 2001/09/25 12:04:22 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */


#ifndef __RUNNER_H
#define __RUNNER_H

#include "thread/thread.h"
#include "inputmodule.h"
#include "cfgparse.h"
#include "encode.h"

#define MAX_QUEUE_NODE_BUFFER 16*1024
typedef struct _runner_tag runner;
typedef struct _instance_tag instance_t;

struct instance
{
    input_type type;
    int id;

    struct encoder_settings encode_settings;

    struct codec_ops *ops;

    int downmix;
    int passthru;

    struct output_state output;

    float quality;
    int channels;

    int resampleoutrate;

    struct instance *next;
};


struct runner
{
#ifdef USE_PIPES
    int fd [2];
#else
    input_buffer *pending, **pending_tail;
    cond_t   data_available;
#endif
    int id;
    struct instance *instances;
    thread_type *thread;
    int not_running;

    struct runner *next;
};


void *ices_runner (void *arg);
int  send_to_runner (struct runner *run, input_buffer *buffer);
void runner_close (struct runner *run);
void stream_cleanup(struct instance *stream);
int create_runner_thread (struct runner *run);
int parse_runner (xmlNodePtr node, void *arg);
struct runner *config_free_runner(struct runner *run);
void start_runners();

#endif

