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
 
 last mod: $Id: curl_interface.c,v 1.1.2.6 2001/08/31 18:01:12 kcarnold Exp $
 
********************************************************************/

#include "curl_interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* for memmove */
#include <signal.h>		/* for SIGINT */

#undef DEBUG_CURLINTERFACE

#ifdef DEBUG_CURLINTERFACE
#define debug(x, y...) do { fprintf (stderr, x , ## y); } while (0)
#else
#define debug(x, y...) do { } while (0)
#endif

/* we only need one function from ogg123 itself. */
extern void Ogg123UpdateStats(void);
/* and one flag. */
extern char exit_requested;

size_t
CurlWriteFunction (void *ptr, size_t size, size_t nmemb, void *arg)
{
  buf_t *buf = arg;
  debug ("CurlWriteFunction, submitting %d bytes.\n", size * nmemb);
  if (exit_requested)
    exit(0);
  SubmitData (buf, ptr, size, nmemb);
  Ogg123UpdateStats();
  return size * nmemb;
}

size_t
BufferWriteChunk (void *voidptr, size_t size, void *arg, char iseos)
{
  StreamInputBufferData_t *data = arg;
  unsigned char *ptr = voidptr;

  debug ("buffer writing chunk of %d, %d bytes to go\n", size,
	 data->BytesRequested);

  pthread_mutex_lock (&data->ReadDataMutex);
  while (data->BytesRequested == 0 && !data->ShuttingDown)
    pthread_cond_wait (&data->ReadRequestedCondition, &data->ReadDataMutex);

  data->EOS = iseos;
  if (iseos)
    debug ("End of stream.\n");

  if (size <= data->BytesRequested)
    {
      debug ("simply moving %d bytes in.\n", size);
      memmove (data->CurWritePtr, ptr, size);
      data->CurWritePtr += size;
      data->BytesRequested -= size;
      pthread_mutex_unlock (&data->ReadDataMutex);
      if (data->BytesRequested == 0 || iseos)
	pthread_cond_signal (&data->ReadDoneCondition);
    }
  else
    {
      /* There will be some excess data here. Write it, then block on needing more data. */
      debug ("writing %d bytes, ", data->BytesRequested);
      memmove (data->CurWritePtr, ptr, data->BytesRequested);
      data->CurWritePtr += data->BytesRequested;
      size -= data->BytesRequested;
      ptr += data->BytesRequested;
      data->BytesRequested = 0;
      debug ("saving %d bytes of excess data\n", size);
      memmove (data->ExcessData, ptr, size);
      data->ExcessDataSize = size;

      debug ("signalling successful read\n");
      pthread_mutex_unlock (&data->ReadDataMutex);
      pthread_cond_signal (&data->ReadDoneCondition);
    }

  debug ("buffer write done.\n");
  return size;
}

size_t
BufferWriteFunction (void *ptr, size_t size, size_t nmemb, void *arg,
		     char iseos)
{
  size_t written = 0;
  while (nmemb > 0)
    {
      if (nmemb == 1)
	written += BufferWriteChunk (ptr, size, arg, iseos);
      else
	written += BufferWriteChunk (ptr, size, arg, 0);
      nmemb--;
    }
  return written;
}

int
CurlProgressFunction (void *arg, size_t dltotal, size_t dlnow, size_t ultotal,
		      size_t ulnow)
{
  debug ("curlprogressfunction\n");
  return 0;
}

void
CurlSetopts (CURL * handle, buf_t * buf, InputOpts_t inputOpts)
{
  curl_easy_setopt (handle, CURLOPT_FILE, buf);
  curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION, CurlWriteFunction);
  curl_easy_setopt (handle, CURLOPT_URL, inputOpts.URL);
  if (inputOpts.ProxyPort)
    curl_easy_setopt (handle, CURLOPT_PROXYPORT, inputOpts.ProxyPort);
  if (inputOpts.ProxyHost)
    curl_easy_setopt (handle, CURLOPT_PROXY, inputOpts.ProxyHost);
  if (inputOpts.ProxyTunnel)
    curl_easy_setopt (handle, CURLOPT_HTTPPROXYTUNNEL, inputOpts.ProxyTunnel);
  curl_easy_setopt (handle, CURLOPT_NOPROGRESS, 1);
  if (inputOpts.Netrc)
    curl_easy_setopt (handle, CURLOPT_NETRC, inputOpts.Netrc);
  if (inputOpts.FollowLocation)
    curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION,
		      inputOpts.FollowLocation);
  if (inputOpts.Referer)
    curl_easy_setopt (handle, CURLOPT_REFERER, inputOpts.Referer);
  if (inputOpts.UserAgent)
    curl_easy_setopt (handle, CURLOPT_USERAGENT, inputOpts.UserAgent);
  if (inputOpts.Cookie)
    curl_easy_setopt (handle, CURLOPT_COOKIE, inputOpts.Cookie);
  if (inputOpts.CookieFile)
    curl_easy_setopt (handle, CURLOPT_COOKIEFILE, inputOpts.CookieFile);
  curl_easy_setopt (handle, CURLOPT_PROGRESSFUNCTION, CurlProgressFunction);
  curl_easy_setopt (handle, CURLOPT_PROGRESSDATA, buf);
}

void *
CurlGo (void *arg)
{
  buf_t *buf = arg;
  StreamInputBufferData_t *data = buf->data;
  CURLcode ret;
  debug ("CurlGo\n");
  ret = curl_easy_perform ((CURL *) data->CurlHandle);
  debug ("curl done.\n");
  buffer_MarkEOS (buf);
  buffer_ReaderQuit (buf);
  curl_easy_cleanup (data->CurlHandle);
  data->CurlHandle = 0;
  return (void *) ret;
}

buf_t *
InitStream (InputOpts_t inputOpts)
{
  StreamInputBufferData_t *data = calloc (1, sizeof (StreamInputBufferData_t));
  buf_t *buf;

  debug ("InitStream\n");
  debug (" Allocated data\n");

  if (!data)
    {
      perror ("malloc");
      exit (1);
    }

  debug (" init pthreads\n");
  pthread_mutex_init (&data->ReadDataMutex, NULL);
  pthread_cond_init (&data->ReadRequestedCondition, NULL);
  pthread_cond_init (&data->ReadDoneCondition, NULL);

  debug (" curl init\n");
  data->CurlHandle = curl_easy_init ();
  if (!data->CurlHandle)
    {
      perror ("curl_easy_init");
      exit (1);
    }

  debug (" start buffer\n");
  buf =
    StartBuffer (inputOpts.BufferSize, inputOpts.Prebuffer, data,
		 BufferWriteFunction, NULL, NULL, VORBIS_CHUNKIN_SIZE);

  if (!buf)
    {
      perror ("StartBuffer");
      exit (1);
    }

  debug (" set curl opts\n");
  CurlSetopts (data->CurlHandle, buf, inputOpts);

  debug (" init saving stream\n");
  if (inputOpts.SaveStream)
    data->SavedStream = fopen (inputOpts.SaveStream, "wb");

  pthread_create (&data->CurlThread, NULL, CurlGo, buf);

  debug ("returning.\n");
  return buf;
}

size_t
_StreamBufferRead (void *voidptr, size_t size, size_t nmemb, void *arg)
{
  StreamInputBufferData_t *data = arg;
  unsigned char *ptr = voidptr;
  size_t ret;

  ret = size *= nmemb;		/* makes things simpler and run smoother */

  debug ("StreamBufferRead %d bytes\n", ret);

  pthread_mutex_lock (&data->ReadDataMutex);

  if (size <= data->ExcessDataSize)
    {
      debug ("reading out of excess data\n");
      memmove (ptr, data->ExcessData, size);
      data->ExcessDataSize -= size;
      if (size < data->ExcessDataSize)
	memmove (data->ExcessData, data->ExcessData + size,
		 data->ExcessDataSize);
      pthread_mutex_unlock (&data->ReadDataMutex);
    }
  else
    {
      debug ("Using %d bytes of excess data.\n", data->ExcessDataSize);
      memmove (ptr, data->ExcessData, data->ExcessDataSize);
      ptr += data->ExcessDataSize;
      size -= data->ExcessDataSize;
      if (data->EOS)
	{
	  ret = data->ExcessDataSize;
	  debug ("Data marked EOS, so returning just excess data\n");
	  pthread_mutex_unlock (&data->ReadDataMutex);
	  data->ExcessDataSize = 0;
	  return ret;
	}
      else
	data->ExcessDataSize = 0;

      data->BytesRequested = size;
      data->WriteTarget = data->CurWritePtr = ptr;

      pthread_mutex_unlock (&data->ReadDataMutex);
      pthread_cond_signal (&data->ReadRequestedCondition);
      pthread_mutex_lock (&data->ReadDataMutex);

      while (data->BytesRequested > 0 && !data->EOS)
	{
	  debug ("Waiting for %d bytes of data to be read, eos=%d.\n",
		 data->BytesRequested, data->EOS);
	  pthread_cond_wait (&data->ReadDoneCondition, &data->ReadDataMutex);
	}
      if (data->EOS)
	ret -= data->BytesRequested;
      pthread_mutex_unlock (&data->ReadDataMutex);
    }
  debug ("buffer read done.\n");
  return ret;
}

size_t StreamBufferRead (void *ptr, size_t size, size_t nmemb, void *arg)
{
  StreamInputBufferData_t *data = arg;
  size_t ret = _StreamBufferRead (ptr, size, nmemb, arg);
  if (data->SavedStream)
    fwrite (ptr, ret, 1, data->SavedStream);
  return ret;
}

/* These are no-ops for now. */
int
StreamBufferSeek (void *arg, ogg_int64_t offset, int whence)
{
  debug ("StreamBufferSeek\n");
  return -1;
}

int
StreamBufferClose (void *arg)
{
  StreamInputBufferData_t *data = arg;

  debug ("StreamBufferClose\n");
  if (data)
    {
      pthread_kill (data->CurlThread, SIGTERM);
      pthread_join (data->CurlThread, NULL);
      data->ShuttingDown = 1;
      data->EOS = 1;
      data->BytesRequested = 0;
      pthread_cond_signal (&data->ReadRequestedCondition);
      memset (data, 0, sizeof(data));
      free (data);
    }
  return 0;
}

long
StreamBufferTell (void *arg)
{
  return 0;
}

void StreamInputCleanup (buf_t *buf)
{ 
  StreamBufferClose (buf->data);
  buf->data = 0;
  buffer_flush (buf);
  buffer_cleanup (buf);
}
