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

 last mod: $Id: buffer.c,v 1.7.2.6 2001/08/10 16:33:40 kcarnold Exp $

 ********************************************************************/

/* buffer.c
 *  buffering code for ogg123. This is Unix-specific. Other OSes anyone?
 *
 * Thanks to Lee McLouchlin's buffer(1) for inspiration; no code from
 * that program is used in this buffer.
 */

#include <sys/types.h>
#if HAVE_SMMAP
#include <sys/mman.h>
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h> /* for fork and pipe*/
#include <fcntl.h>
#include <signal.h>

#include "ogg123.h"
#include "buffer.h"

#undef DEBUG_BUFFER

#ifdef DEBUG_BUFFER
FILE *debugfile;
#define DEBUG0(x) do { fprintf (debugfile, x "\n" ); } while (0)
#define DEBUG1(x, y) do { fprintf (debugfile, x "\n" , y); } while (0)
#else
#define DEBUG0(x)
#define DEBUG1(x, y)
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

  DEBUG0("r: BufferFunc");
  sigfillset (&set);
  pthread_sigmask (SIG_SETMASK, &set, NULL);

  pthread_cleanup_push (PthreadCleanup, buf);
  while (1)
    {
      /* don't touch the size unless we ask you to. */
      LOCK_MUTEX (buf->SizeMutex);

      /* paused? Remember to signal DataReady when unpaused. */
    checkPlaying:
      while (!(buf->StatMask & STAT_PLAYING) ||
	     (buf->StatMask & STAT_PREBUFFERING)) {
	DEBUG1 ("r: waiting on !playing || prebuffering (stat=%d)", buf->StatMask);
	pthread_cond_wait (&buf->DataReadyCondition, &buf->SizeMutex);
      }

      if (buf->curfill == 0) {
	UNLOCK_MUTEX (buf->SizeMutex);
	DEBUG0 ("r: signalling buffer underflow");
	pthread_cond_signal (&buf->UnderflowCondition);
	LOCK_MUTEX (buf->SizeMutex);
	Prebuffer (buf);
	DEBUG0 ("r: waiting on data ready");
	pthread_cond_wait (&buf->DataReadyCondition, &buf->SizeMutex);
	goto checkPlaying;
      }

      /* unlock while playing sample */
      UNLOCK_MUTEX (buf->SizeMutex);

      DEBUG0("writing chunk");
      buf->write_func (buf->writer->data, buf->writer->len, 1, buf->data);

      DEBUG0("incrementing pointer");
      LOCK_MUTEX (buf->SizeMutex);
      if (vbuf->writer == vbuf->end)
	vbuf->writer = vbuf->buffer;
      else
	vbuf->writer++;
      vbuf->curfill--;
      UNLOCK_MUTEX (buf->SizeMutex);

      /* slight abuse of the DataReady condition, but makes sense. */
      DEBUG0 ("r: signalling buffer no longer full");
      if (buf->curfill + 1 == buf->size)
	pthread_cond_signal (&buf->DataReadyCondition);
   }
  /* should never get here */
  pthread_cleanup_pop(1);
  DEBUG0("r: exiting");
}

buf_t *StartBuffer (long size, long prebuffer, void *data, 
		    size_t (*write_func) (void *, size_t, size_t, void *))
{
  buf_t *buf = malloc (sizeof(buf_t) + sizeof (chunk_t) * (size - 1));

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

  buf->reader = buf->writer = buf->buffer;
  buf->end = buf->buffer + (size - 1);
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

void submit_chunk (buf_t *buf, chunk_t chunk)
{
  DEBUG0("submit_chunk");
  LOCK_MUTEX (buf->SizeMutex);
  /* wait on buffer overflow */
  while (buf->curfill == buf->size && buf->StatMask & STAT_PLAYING)
    pthread_cond_wait (&buf->DataReadyCondition, &buf->SizeMutex);

  DEBUG0("writing chunk");
  *(buf->reader) = chunk;
  if (buf->reader == buf->end)
    buf->reader = buf->buffer;
  else
    buf->reader++;
  buf->curfill++;

  UNLOCK_MUTEX (buf->SizeMutex);

  if ((buf->StatMask & STAT_PREBUFFERING)
      && buffer_full(buf) >= buf->prebuffer) {
    DEBUG0("prebuffering done, starting writer");
    LOCK_MUTEX (buf->StatMutex);
    buf->StatMask &= ~STAT_PREBUFFERING;
    UNLOCK_MUTEX (buf->StatMutex);
    pthread_cond_signal (&buf->DataReadyCondition);
  }
  else if (buf->curfill == 1)
    pthread_cond_signal (&buf->DataReadyCondition);

  DEBUG0("submit_chunk exit");
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

void buffer_cleanup (buf_t *buf) {
  if (buf) {
    buffer_shutdown (buf);
    PthreadCleanup (buf);
    free (buf);
  }
}
