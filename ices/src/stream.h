/* stream.h
 * - Core streaming functions/main loop.
 *
 * $Id: stream.h,v 1.2.2.1 2002/02/07 09:11:12 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */


#ifndef __STREAM_H
#define __STREAM_H

#include <shout/shout.h>

#include "thread/thread.h"
#include "process.h"
#include "config.h"

typedef struct _queue_item {
	ref_buffer *buf;
	struct _queue_item *next;
} queue_item;

typedef struct buffer_queue {
	queue_item *head, *tail;
	int length;
	mutex_t lock;
} buffer_queue;

typedef struct
{
	char *hostname;
    char *connip;
	int port;
	char *password;
	char *mount;
	int reconnect_delay;
	int reconnect_attempts;
	int max_queue_length;

    char *stream_name;
    char *stream_genre;
    char *stream_description;

    shout_t *shout;
    int errors;

    int wait_for_bos;

} stream_state;

int stream_open_module(process_chain_element *mod, module_param_t *params);

#endif

