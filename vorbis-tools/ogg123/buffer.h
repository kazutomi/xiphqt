/* Common things between reader and writer threads */

#ifndef __BUFFER_H
#define __BUFFER_H

#include "ogg123.h"

#include <sys/types.h>

/* We use chunks to make things easier. Quite possibly this is wasting
   at least two integers for every 4096 bytes, but it beats the
   horrificly involved calculations involved with interleaved channels
   and instantaneous bitrate information in a circular buffer. */
typedef struct chunk_s
{
  ogg_int64_t sample; /* sample number of starting sample */
  double bitrate;     /* instantaneous bitrate at sample */
  long len; /* Length of the chunk (for if we only got partial data) */
  char data[4096]; /* Data. 4096 is the chunk size we request from libvorbis. */
} chunk_t;

typedef struct buf_s
{
  nonbuf_shared_t nonbuf_shared; /* other shared data */
  char status;       /* Status. See STAT_* below. */
  int fds[2];        /* Pipe file descriptors. */
  long size;         /* buffer size, for reference */
  long prebuffer;    /* number of chunks to prebuffer */
  pid_t readerpid;   /* PID of reader process */
  pid_t writerpid;   /* PID of writer process */
  chunk_t *reader;   /* Chunk the reader is busy with */
  chunk_t *writer;   /* Chunk the writer is busy with */
  chunk_t *end;      /* Last chunk in the buffer (for convenience) */
  chunk_t buffer[1]; /* The buffer itself. It's more than one chunk. */
} buf_t;

buf_t *fork_writer (long size, devices_t *d, long prebuffer);
void submit_chunk (buf_t *buf, chunk_t chunk);
void buffer_shutdown (buf_t *buf);
void buffer_cleanup (buf_t *buf);
void buffer_flush (buf_t *buf);
long buffer_full (buf_t *buf);

#define STAT_FLUSH 1
#define STAT_SHUTDOWN 2
#define STAT_PREBUFFER 4
#define STAT_UNDERFLOW 8

#endif /* !defined (__BUFFER_H) */



