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
 
 last mod: $Id: curl_interface.c,v 1.1.2.1 2001/08/11 02:10:09 kcarnold Exp $
 
********************************************************************/

#include "curl_interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h> /* for SIGTERM */

#define debug(x, y...) do { fprintf (stderr, x , ## y); } while (0)

size_t CurlWriteFunction (void *ptr, size_t size, size_t nmemb, void *arg)
{
  buf_t *buf = arg;
  debug("CurlWriteFunction, submitting %d bytes.\n", size*nmemb);
  SubmitData (buf, ptr, size, nmemb);
  return size*nmemb;
}

size_t BufferWriteChunk (void *ptr, size_t size, void *arg)
{
  StreamInputBufferData_t *data = arg;

  debug("buffer writing chunk of %d, %d bytes to go\n", size, data->BytesRequested);

  pthread_mutex_lock (&data->ReadDataMutex);
  while (data->BytesRequested == 0)
    pthread_cond_wait (&data->ReadRequestedCondition, &data->ReadDataMutex);

  if (size <= data->BytesRequested)
    {
      fprintf (stderr, "simply moving %d bytes in.\n", size);
      memmove (data->CurWritePtr, ptr, size);
      data->CurWritePtr += size;
      data->BytesRequested -= size;
      pthread_mutex_unlock (&data->ReadDataMutex);
      if (data->BytesRequested == 0)
	pthread_cond_signal (&data->ReadDoneCondition);
    }
  else
    {
      /* There will be some excess data here. Write it, then block on needing more data. */
      fprintf (stderr, "writing %d bytes, ", data->BytesRequested);
      memmove (data->CurWritePtr, ptr, data->BytesRequested);
      data->CurWritePtr += data->BytesRequested;
      size -= data->BytesRequested;
      ptr += data->BytesRequested;
      data->BytesRequested = 0;
      fprintf (stderr, "saving %d bytes of excess data\n", size);
      memmove (data->ExcessData, ptr, size);
      data->ExcessDataSize = size;
      
      debug ("signalling successful read\n");
      pthread_mutex_unlock (&data->ReadDataMutex);
      pthread_cond_signal (&data->ReadDoneCondition);
    }

  debug ("buffer write done.\n");
  return size;
}

size_t BufferWriteFunction (void *ptr, size_t size, size_t nmemb, void *arg)
{
  size_t written = 0;
  while (nmemb > 0)
    {
      written += BufferWriteChunk (ptr, size, arg);
      nmemb--;
    }
  return written;
}

int CurlProgressFunction (void *arg, size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow)
{
  debug ("curlprogressfunction\n");
  return 0;
}

void CurlSetopts (CURL* handle, buf_t *buf, InputOpts_t inputOpts)
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
    curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, inputOpts.FollowLocation);
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

void* CurlGo (void *arg)
{
  buf_t *buf = arg;
  StreamInputBufferData_t *data = buf->data;
  CURLcode ret;
  fprintf (stderr, "CurlGo\n");
  ret = curl_easy_perform ((CURL*) data->CurlHandle);
  debug ("curl done.\n");
  data->EOFAt = buf->curfill;
  return (void*) ret;
}

buf_t *InitStream (InputOpts_t inputOpts)
{
  StreamInputBufferData_t *data = malloc (sizeof (StreamInputBufferData_t));
  buf_t *buf;

  debug ("InitStream\n");
  debug (" Allocated data\n");

  if (!data)
    {
      perror ("malloc");
      exit(1);
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
      exit(1);
    }

  debug (" start buffer\n");
  buf = StartBuffer (inputOpts.BufferSize, inputOpts.Prebuffer, data, BufferWriteFunction, NULL, NULL);

  if (!buf)
    {
      perror ("StartBuffer");
      exit(1);
    }

  debug (" set curl opts\n");
  CurlSetopts (data->CurlHandle, buf, inputOpts);

  pthread_create (&data->CurlThread, NULL, CurlGo, buf);

  debug ("returning.\n");
  return buf;
}

size_t StreamBufferRead (void *ptr, size_t size, size_t nmemb, void *arg)
{
  StreamInputBufferData_t *data = arg;
  size_t ret;

  ret = size *= nmemb; /* makes things simpler and run smoother */

  debug ("StreamBufferRead %d bytes\n", ret);

  pthread_mutex_lock (&data->ReadDataMutex);

  if (size <= data->ExcessDataSize)
    {
      debug ("reading out of excess data\n");
      memmove (ptr, data->ExcessData, size);
      data->ExcessDataSize -= size;
      if (size < data->ExcessDataSize)
	memmove (data->ExcessData, data->ExcessData + size, data->ExcessDataSize);
      pthread_mutex_unlock (&data->ReadDataMutex);
    }
  else
    {
      debug ("Using %d bytes of excess data.\n", data->ExcessDataSize);
      memmove (ptr, data->ExcessData, data->ExcessDataSize);
      ptr += data->ExcessDataSize;
      size -= data->ExcessDataSize;
      data->ExcessDataSize = 0;
      
      data->BytesRequested = size;
      data->WriteTarget = data->CurWritePtr = ptr;
      
      pthread_mutex_unlock (&data->ReadDataMutex);
      pthread_cond_signal (&data->ReadRequestedCondition);
      pthread_mutex_lock (&data->ReadDataMutex);
      
      while (data->BytesRequested > 0) {
	debug ("Waiting for %d bytes of data to be read.\n", data->BytesRequested);
	pthread_cond_wait (&data->ReadDoneCondition, &data->ReadDataMutex);
      }
      pthread_mutex_unlock (&data->ReadDataMutex);
    }
  debug ("buffer read done.\n");
  return ret;
}

/* These are no-ops for now. */
int StreamBufferSeek (void *arg, ogg_int64_t offset, int whence)
{
  debug ("StreamBufferSeek\n");
  return -1;
}

int StreamBufferClose (void *arg)
{
  buf_t *buf = arg;
  StreamInputBufferData_t *data = buf->data;

  pthread_kill (data->CurlThread, SIGTERM);
  pthread_join (data->CurlThread, NULL);
  data->CurlThread = 0;
  buffer_flush (buf);
  buffer_shutdown (buf);
  return 0;
}

long StreamBufferTell (void *arg)
{
  return 0;
}

