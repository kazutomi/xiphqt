/* Common things between reader and writer threads */

#ifndef __BUFFER_H
#define __BUFFER_H

#include "ogg123.h"
#include <sys/types.h>

/* 4096 is the chunk size we request from libvorbis. */
#define BUFFER_CHUNK_SIZE 4096

typedef struct chunk_s
{
  long len; /* Length of the chunk (for if we only got partial data) */
  unsigned char data[BUFFER_CHUNK_SIZE]; 
} chunk_t;

typedef struct buf_s
{
  char status;       /* Status. See STAT_* below. */
  int fds[2];        /* Pipe file descriptors. */
  long size;         /* buffer size, for reference */
  long curfill;      /* how much the buffer is currently filled */
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



