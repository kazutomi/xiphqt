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

 last mod: $Id: buffer.c,v 1.7.2.10 2001/08/11 16:04:21 kcarnold Exp $

 ********************************************************************/

/* buffer.c
 *  buffering code for ogg123. This is Unix-specific. Other OSes anyone?
 *
 * Thanks to Lee McLouchlin's buffer(1) for inspiration; no code from
 * that program is used in this buffer.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h> /* for fork and pipe*/
#include <fcntl.h>
#include <signal.h>

#include <assert.h> /* for debug purposes */

#include "ogg123.h"
#include "buffer.h"

#define DEBUG_BUFFER

#ifdef DEBUG_BUFFER
FILE *debugfile;
#define DEBUG0(x) do { if (pthread_self() == buf->BufferThread) fprintf (debugfile, "R: " x "\n" ); else fprintf (debugfile, "W: " x "\n" ); } while (0)
#define DEBUG1(x, y) do { if (pthread_self() == buf->BufferThread) fprintf (debugfile, "R-: " x "\n", y ); else fprintf (debugfile, "W-: " x "\n", y ); } while (0)
#define DUMP_BUFFER_INFO(buf) do { fprintf (debugfile, "Buffer info: -reader=%p  -writer=%p  -buf=%p  -end=%p -curfill=%d\n", buf->reader, buf->writer, buf->buffer, buf->end, buf->curfill); } while (0)
#else
#define DEBUG0(x)
#define DEBUG1(x, y)
#define DUMP_BUFFER_INFO(buf)
#endif

#define LOCK_MUTEX(mutex) do { DEBUG1("Locking mutex %s.", #mutex); pthread_mutex_lock (&(mutex)); } while (0)
#define UNLOCK_MUTEX(mutex) do { DEBUG1("Unlocking mutex %s", #mutex); pthread_mutex_unlock(&(mutex)); } while (0) 

void Prebuffer (buf_t * buf)
{
  if (buf->prebuffer > 0)
    {
      LOCK_MUTEX (buf->StatMutex);
      buf->StatMask |= STAT_PREBUFFERING;
      UNLOCK_MUTEX (buf->StatMutex);
    }
}

void PthreadCleanup (void *arg)
{
  buf_t *buf = (buf_t*) arg;
  
  DEBUG0("PthreadCleanup");
  UNLOCK_MUTEX (buf->SizeMutex);
  UNLOCK_MUTEX (buf->StatMutex);

  /* kludge to get around pthreads vs. signal handling */
  pthread_cond_broadcast (&buf->DataReadyCondition);
  pthread_cond_broadcast (&buf->UnderflowCondition);
  pthread_cond_broadcast (&buf->OverflowCondition);
  UNLOCK_MUTEX (buf->SizeMutex);
  UNLOCK_MUTEX (buf->StatMutex);
}

void* BufferFunc (void *arg)
{
  sigset_t set;
  buf_t *buf = (buf_t*) arg;
  volatile buf_t *vbuf = (volatile buf_t*) buf; /* optimizers... grr */
  size_t WriteThisTime = 0;
  chunk *NewWriterPtr;
  char iseos = 0;

  DEBUG0("BufferFunc");
  sigfillset (&set);
  pthread_sigmask (SIG_SETMASK, &set, NULL);

  /* Run the initialization function, if there is one */
  if (buf->init_func)
    {
      int ret = buf->init_func (buf->initData);
      if (!ret)
	pthread_exit ((void*)ret);
    }

  pthread_cleanup_push (PthreadCleanup, buf);

  while (1)
    {
      /* don't touch the size unless we ask you to. */
      LOCK_MUTEX (buf->SizeMutex);

      /* paused? Remember to signal DataReady when unpaused. */
    checkPlaying:
      while (!(buf->StatMask & STAT_PLAYING) ||
	     (buf->StatMask & STAT_PREBUFFERING)) {
	DEBUG1 ("waiting on !playing || prebuffering (stat=%d)", buf->StatMask);
	pthread_cond_wait (&buf->DataReadyCondition, &buf->SizeMutex);
      }

      DUMP_BUFFER_INFO(buf);
      assert (buf->curfill >= 0);
      assert (buf->writer >= buf->buffer);
      assert (buf->writer <= buf->end);
      assert (buf->reader >= buf->buffer);
      assert (buf->reader <= buf->end);

      if (buf->curfill == 0) {
	UNLOCK_MUTEX (buf->SizeMutex);
	DEBUG0 ("signalling buffer underflow");
	pthread_cond_signal (&buf->UnderflowCondition);
	LOCK_MUTEX (buf->SizeMutex);
	Prebuffer (buf);
	DEBUG0 ("waiting on data ready");
	pthread_cond_wait (&buf->DataReadyCondition, &buf->SizeMutex);
	goto checkPlaying;
      }

      if (buf->reader < buf->writer)
	{
	  /* we have all the way to buf->end: 
	   * |-------------------------------|
	   * |-^       ^---------------------|
	   *  reader   writer, our range
	   * EOS applicable only if reader is at beginning of buffer
	   */
	  DEBUG1("up to buf->end, buf->end - buf->writer + 1 = %d", buf->end - buf->writer + 1);
	  if (buf->end - buf->writer + 1 > buf->OptimalWriteSize) {
	    WriteThisTime = buf->OptimalWriteSize;
	    NewWriterPtr = buf->writer + WriteThisTime;
	  } else {
	    NewWriterPtr = buf->buffer;
	    WriteThisTime = buf->end - buf->writer + 1;
	    if (buf->reader == buf->buffer)
	      iseos = buf->eos;
	  }
	}
      else
	{
	  /* we have up to buf->reader:
	   * |-------------------------------|
	   *    ^--------------^
	   *   writer         reader
	   * but we can't use buf->reader itself, becuase that's not in the data.
	   * EOS applicable if we're reading right up to reader.
	   */
	  DEBUG1("up to buf->reader, buf->reader - buf->writer = %d", buf->reader - buf->writer);
	  if (buf->reader - buf->writer > buf->OptimalWriteSize)
	    WriteThisTime = buf->OptimalWriteSize;
	  else {
	    WriteThisTime = buf->reader - buf->writer;
	    iseos = buf->eos;
	  }
	  NewWriterPtr = buf->writer + WriteThisTime;
	}
      
      DEBUG0("writing chunk to output");
      /* unlock while playing sample */
      UNLOCK_MUTEX (buf->SizeMutex);
      DEBUG1("WriteThisTime=%d", WriteThisTime);
      buf->write_func (buf->writer, WriteThisTime, 1, buf->data, iseos);

      DEBUG0("incrementing pointer");
      LOCK_MUTEX (buf->SizeMutex);
      buf->writer = NewWriterPtr;
      vbuf->curfill -= WriteThisTime;
      UNLOCK_MUTEX (buf->SizeMutex);

      /* slight abuse of the DataReady condition, but makes sense. */
      DEBUG0 ("signalling buffer no longer full");
      if (buf->curfill + WriteThisTime + buf->OptimalWriteSize >= buf->size)
	pthread_cond_signal (&buf->DataReadyCondition);
   }
  /* should never get here */
  pthread_cleanup_pop(1);
  DEBUG0("exiting");
}

buf_t *StartBuffer (long size, long prebuffer, void *data, 
		    size_t (*write_func) (void *, size_t, size_t, void *, char),
		    void *initData, int (*init_func) (void*), int OptimalWriteSize)
{
  buf_t *buf = malloc (sizeof(buf_t) + sizeof (chunk) * (size - 1));

  if (buf == NULL)
    {
      perror ("malloc");
      exit (1);
    }

  /* we no longer need those hacked-up shared memory things! yippee! */

#ifdef DEBUG_BUFFER
  debugfile = fopen ("/tmp/bufferdebug", "w");
  setvbuf (debugfile, NULL, _IONBF, 0);
#endif

  /* Initialize the buffer structure. */
  DEBUG0("buffer init");
  memset (buf, 0, sizeof(*buf));

  buf->data = data;
  buf->write_func = write_func;

  buf->initData = initData;
  buf->init_func = init_func;

  buf->reader = buf->writer = buf->buffer;
  buf->end = buf->buffer + (size - 1);
  buf->OptimalWriteSize = OptimalWriteSize;
  buf->size = size;
  buf->prebuffer = prebuffer;
  Prebuffer (buf);
  buf->StatMask |= STAT_PLAYING;

  /* pthreads initialization */
  pthread_mutex_init (&buf->SizeMutex, NULL);
  pthread_cond_init (&buf->UnderflowCondition, NULL);
  pthread_cond_init (&buf->OverflowCondition, NULL);
  
  pthread_create(&buf->BufferThread, NULL, BufferFunc, buf);

  return buf;
}

void _SubmitDataChunk (buf_t *buf, chunk *data, size_t size)
{
  char PrevSize;
  DEBUG1("submit_chunk, size %d", size);
  LOCK_MUTEX (buf->SizeMutex);

  PrevSize = buf->curfill;
  DUMP_BUFFER_INFO(buf);

  /* wait on buffer overflow */
  while (buf->curfill + size > buf->size)
    pthread_cond_wait (&buf->DataReadyCondition, &buf->SizeMutex);

  DEBUG0("writing chunk into buffer");
  buf->curfill += size;
  /* we're guaranteed to have enough space in the buffer by now */
  if (buf->reader < buf->writer) {
    DEBUG0("writer before end");
    /* don't worry about falling off end */
    memmove (buf->reader, data, size);
    buf->reader += size;
  } else {
    size_t avail = buf->end - buf->reader + 1;
    DEBUG0("don't run over the end!");
    if (avail >= size)
      memmove (buf->reader, data, size);
    else {
      memmove (buf->reader, data, avail);
      size -= avail;
      data += avail;
      buf->reader = buf->buffer;
      memmove (buf->reader, data, size);
    }
    buf->reader += size;
  }
    
  UNLOCK_MUTEX (buf->SizeMutex);

  if ((buf->StatMask & STAT_PREBUFFERING)
      && buf->curfill >= buf->prebuffer) {
    DEBUG0("prebuffering done, starting writer");
    LOCK_MUTEX (buf->StatMutex);
    buf->StatMask &= ~STAT_PREBUFFERING;
    UNLOCK_MUTEX (buf->StatMutex);
    pthread_cond_signal (&buf->DataReadyCondition);
  }
  else if (PrevSize == 0)
    pthread_cond_signal (&buf->DataReadyCondition);

  DEBUG0("submit_chunk exit");
}

void SubmitData (buf_t *buf, chunk *data, size_t size, size_t nmemb)
{
  int i, s;
  size *= nmemb;
  for (i = 0; i < size; i += buf->OptimalWriteSize) {
    s = i + buf->OptimalWriteSize <= size ? buf->OptimalWriteSize : size - i;
    _SubmitDataChunk (buf, data, s);
    data += s;
  }
}

void buffer_flush (buf_t *buf)
{
  DEBUG0("flush buffer");
  LOCK_MUTEX (buf->SizeMutex);
  buf->curfill = 0;
  buf->reader = buf->writer;
  UNLOCK_MUTEX (buf->SizeMutex);
  Prebuffer (buf);
}

void buffer_WaitForEmpty (buf_t *buf)
{
  DEBUG0("waiting for empty");
  LOCK_MUTEX (buf->SizeMutex);
  while (buf->curfill > 0)
    pthread_cond_wait (&buf->UnderflowCondition, &buf->SizeMutex);
  UNLOCK_MUTEX (buf->SizeMutex);
  Prebuffer (buf);
}

void buffer_shutdown (buf_t *buf)
{
  DEBUG0("shutdown buffer");
  if (buf && buf->BufferThread) {
    buffer_WaitForEmpty (buf);
    pthread_cancel (buf->BufferThread);
    pthread_join (buf->BufferThread, NULL);
    buf->BufferThread = 0;
  }
  DEBUG0("buffer done.");
}

long buffer_full (buf_t* buf) {
  return buf->curfill;
}

void buffer_Pause (buf_t *buf)
{
  LOCK_MUTEX (buf->StatMutex);
  buf->StatMask &= ~STAT_PLAYING;
  UNLOCK_MUTEX (buf->StatMutex);
}

void buffer_Unpause (buf_t *buf)
{
  LOCK_MUTEX (buf->StatMutex);
  buf->StatMask |= STAT_PLAYING;
  UNLOCK_MUTEX (buf->StatMutex);
  pthread_cond_signal (&buf->DataReadyCondition);
}

char buffer_Paused (buf_t *buf)
{
  return (char) !(buf->StatMask & STAT_PLAYING);
}

void buffer_MarkEOS (buf_t *buf)
{
  buf->eos = 1;
}


void buffer_cleanup (buf_t *buf) {
  if (buf) {
    buffer_shutdown (buf);
    PthreadCleanup (buf);
    memset (buf, 0, sizeof(buf));
    free (buf);
  }
}
