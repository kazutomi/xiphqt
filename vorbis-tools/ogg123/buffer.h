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
 
 last mod: $Id: buffer.h,v 1.2.2.16.2.1 2001/10/14 05:42:51 volsung Exp $
 
********************************************************************/

/* A (relatively) generic circular buffer interface */

#ifndef __BUFFER_H
#define __BUFFER_H

#include <pthread.h>

typedef unsigned char chunk; /* sizeof (chunk) must be 1; if you need otherwise it's not hard to fix */
typedef size_t (*pWriteFunc) (void *, size_t, size_t, void *, char);
typedef int (*pInitFunc) (void *);

typedef struct buf_s
{
  /* generic buffer interface */
  void * data;
  pWriteFunc write_func;

  void * initData;
  pInitFunc init_func;
  
  /* pthread variables */
  pthread_t thread;

  pthread_mutex_t mutex;
  
  pthread_cond_t playback_cond; /* signalled when playback can continue */
  pthread_cond_t write_cond;    /* signalled when more data can be written 
				   to the buffer */
  
  /* buffer info (constant) */
  int  audio_chunk_size;  /* write data to audio device in this chunk size, 
			     if possible */
  long prebuffer_size;    /* number of bytes to prebuffer */
  long size;              /* buffer size, for reference */

  /* ----- Everything after this point is protected by mutex ----- */

  /* buffering state variables */
  int prebuffering;
  int paused;
  int eos;

  /* buffer data */
  long curfill;     /* how much the buffer is currently filled */
  long start;       /* offset in buffer of start of available data */
  chunk buffer[1];   /* The buffer itself. It's more than one chunk. */
} buf_t;

/* --- Buffer allocation --- */

buf_t *buffer_create (long size, long prebuffer, void *data, 
		      pWriteFunc write_func, void *initData, 
		      pInitFunc init_func, int audio_chunk_size);
void buffer_destroy (buf_t *buf);

/* --- Buffer thread control --- */
int  buffer_thread_start   (buf_t *buf);
void buffer_thread_pause   (buf_t *buf);
void buffer_thread_unpause (buf_t *buf);
void buffer_thread_kill    (buf_t *buf);

/* --- Data buffering functions --- */
void buffer_submit_data (buf_t *buf, chunk *data, size_t size, size_t nmemb);
void buffer_mark_eos (buf_t *buf);

/* --- Buffer status functions --- */
void buffer_wait_for_empty (buf_t *buf);
long buffer_full (buf_t *buf);

#endif /* !defined (__BUFFER_H) */
