/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Kenneth C. Arnold <ogg@arnoldnet.net> AND OTHER CONTRIBUTORS  *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************
 
 last mod: $Id: buffer.h,v 1.2.2.9 2001/08/11 02:55:37 kcarnold Exp $
 
********************************************************************/

/* A (relatively) generic circular buffer interface */

#ifndef __BUFFER_H
#define __BUFFER_H

#include <pthread.h>

typedef unsigned char chunk; /* sizeof (chunk) must be 1; if you need otherwise it's not hard to fix */

typedef struct buf_s
{
  /* generic buffer interface */
  void * data;
  size_t (*write_func) (chunk *ptr, size_t size, size_t nmemb, void * d, char iseos);

  void * initData;
  int (*init_func) (void *);
  
  /* pthreads variables */
  pthread_t BufferThread;
  pthread_mutex_t SizeMutex;
  pthread_mutex_t StatMutex;
  pthread_cond_t UnderflowCondition; /* signalled on buffer underflow */
  pthread_cond_t OverflowCondition;  /* signalled on buffer overflow */
  pthread_cond_t DataReadyCondition; /* signalled when data is ready and it wasn't before */
  
  /* the buffer itself */
  char StatMask;
  long size;         /* buffer size, for reference */
  long curfill;      /* how much the buffer is currently filled */
  long prebuffer;    /* number of chunks to prebuffer */
  char eos;        /* set if reader is at end of stream */
  chunk *reader;   /* Chunk the reader is busy with */
  chunk *writer;   /* Chunk the writer is busy with */
  chunk *end;      /* Last chunk in the buffer (for convenience) */
  chunk buffer[1]; /* The buffer itself. It's more than one chunk. */
} buf_t;

#define STAT_PREBUFFERING 1
#define STAT_PLAYING 2
#define STAT_EMPTYING 4

#define TARGET_WRITE_SIZE 4096 /* to agree with other mechanisms used in ogg123 */

buf_t *StartBuffer (long size, long prebuffer, void *data, 
		    size_t (*write_func) (void *, size_t, size_t, void *, char),
		    void *initData, int (*init_func) (void*));
void SubmitData (buf_t *buf, chunk *data, size_t size, size_t nmemb);
void buffer_MarkEOS (buf_t *buf);
void buffer_shutdown (buf_t *buf);
void buffer_cleanup (buf_t *buf);
void buffer_flush (buf_t *buf);
void buffer_WaitForEmpty (buf_t *buf);
long buffer_full (buf_t *buf);

void buffer_Pause (buf_t *buf);
void buffer_Unpause (buf_t *buf);
char buffer_Paused (buf_t *buf);

#endif /* !defined (__BUFFER_H) */
