/* im_stdinpcm.h
 * - stdin reading
 *
 * $Id: im_stdinpcm.h,v 1.2 2001/09/25 12:04:21 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_PCM_H__
#define __IM_PCM_H__

#include "inputmodule.h"
#include <ogg/ogg.h>


#define MAX_ARGS 12

typedef struct _im_pcm_state im_pcm_state; 

struct _im_pcm_state
{
    int (*sleep) (im_pcm_state *s, void  *buf, unsigned remaining, timing_control *control);
	int samplerate;
	int channels;
	int newtrack;
    int timeout;
    char *command;
    FILE *file;
    pid_t pid;
    int fd;
    int max_buffers;
    unsigned samples;
    unsigned default_len;
    float default_duration;
    size_t aux_data;
    time_t silence_start;
    int error;
    int terminate;
    int no_stream;
    uint64_t skip;
    int arg_count;
    char *args[MAX_ARGS+1];   /* allow for NULL */
    void *read_buffer;
    unsigned read_buffer_len;
}; 

int pcm_initialise_module(input_module_t *);
int pcm_open_module(input_module_t *);
void pcm_close_module(input_module_t *mod);
void pcm_shutdown_module(input_module_t *mod);


#endif  /* __IM_PCM_H__ */
