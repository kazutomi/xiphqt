/*
    Copyright (C) 2004 Kor Nielsen
    Pulse driver code copyright (C) 2006 Lennart Poettering
    Unholy union copyright (C) 2007 Monty and Red Hat, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <signal.h>
#include <fusd.h>

#include <pulse/pulseaudio.h>

// Define the DEBUG_LEVEL values used for printfs
#define DEBUG_LEVEL_CRITICAL 0
#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_NOTICE 3
#define DEBUG_LEVEL_INFO 4
#define DEBUG_LEVEL_DEBUG1 5
#define DEBUG_LEVEL_DEBUG2 6
#define DEBUG_LEVEL_DEBUG3 7
#define DEBUG_LEVEL_DEBUG4 8
#define DEBUG_LEVEL_NORMAL DEBUG_LEVEL_DEBUG1

// Define the returnvalues mentionned above in help message:
#define ERR_SUCCESS 0
#define ERR_BADARG 1
#define ERR_JACKD_FAIL 2
#define ERR_FUSD_FAIL 3
#define ERR_DETACH_FAIL 4
#define ERR_JACKD_EXIT 5

// Define the maximum value for device number
#define MAX_DEVICE_NUMBER 99

typedef enum {
    FD_INFO_MIXER,
    FD_INFO_STREAM,
} fd_info_type_t;

typedef struct fd_info fd_info;

struct fd_info {
  pthread_mutex_t mutex;
  int ref;
  int unusable;
  
  fd_info_type_t type;
  
  pa_sample_spec sample_spec;
  size_t fragment_size;
  unsigned n_fragments;
  
  pa_threaded_mainloop *mainloop;
  pa_context *context;
  pa_stream *play_stream;
  pa_stream *rec_stream;
  
  size_t read_offset;
  size_t read_size;
  size_t read_rem;
  char *read_buffer;
  struct fusd_file_info* read_file;

  struct fusd_file_info* poll_file;

  int ioctl_request;
  void *ioctl_argp;

  size_t write_size;
  size_t write_rem;
  const char *write_buffer;
  struct fusd_file_info* write_file;

  int operation_success;
  
  pa_cvolume sink_volume, source_volume;
  uint32_t sink_index, source_index;
  int volume_modify_count;
  
};

#define CONTEXT_CHECK_DEAD_GOTO(i, label) do { \
if (!(i)->context || pa_context_get_state((i)->context) != PA_CONTEXT_READY) { \
    debug(DEBUG_LEVEL_NORMAL, __FILE__": Not connected: %s", (i)->context ? pa_strerror(pa_context_errno((i)->context)) : "NULL"); \
    goto label; \
} \
} while(0);

#define PLAYBACK_STREAM_CHECK_DEAD_GOTO(i, label) do { \
if (!(i)->context || pa_context_get_state((i)->context) != PA_CONTEXT_READY || \
    !(i)->play_stream || pa_stream_get_state((i)->play_stream) != PA_STREAM_READY) { \
    debug(DEBUG_LEVEL_NORMAL, __FILE__": Not connected: %s", (i)->context ? pa_strerror(pa_context_errno((i)->context)) : "NULL"); \
    goto label; \
} \
} while(0);

#define RECORD_STREAM_CHECK_DEAD_GOTO(i, label) do { \
if (!(i)->context || pa_context_get_state((i)->context) != PA_CONTEXT_READY || \
    !(i)->rec_stream || pa_stream_get_state((i)->rec_stream) != PA_STREAM_READY) { \
    debug(DEBUG_LEVEL_NORMAL, __FILE__": Not connected: %s", (i)->context ? pa_strerror(pa_context_errno((i)->context)) : "NULL"); \
    goto label; \
} \
} while(0);

extern struct fusd_file_operations sndstat_file_ops;
extern struct fusd_file_operations dsp_file_ops;
extern struct fusd_file_operations mixer_file_ops;

extern fd_info* fd_info_new(fd_info_type_t type, int *_errno);
extern fd_info *fd_info_ref(fd_info *i);
extern void fd_info_unref(fd_info *i);
extern void reset_params(fd_info *i);

extern void debug(const int level, const char *format, ...);

