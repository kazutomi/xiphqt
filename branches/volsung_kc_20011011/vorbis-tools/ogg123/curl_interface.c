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
 
 last mod: $Id: curl_interface.c,v 1.1.2.6.2.2 2001/10/17 16:58:14 volsung Exp $
 
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
  buffer_submit_data (buf, ptr, size, nmemb);
  Ogg123UpdateStats();
  return size * nmemb;
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
  StreamInputBufferData_t *data = (StreamInputBufferData_t *) arg;
  CURLcode ret;

  debug ("CurlGo\n");
  ret = curl_easy_perform ((CURL *) data->CurlHandle);
  debug ("curl done.\n");

  buffer_mark_eos (data->buf);

  curl_easy_cleanup (data->CurlHandle);
  data->CurlHandle = 0;

  return (void *) ret;
}

StreamInputBufferData_t *
InitStream (InputOpts_t inputOpts)
{
  StreamInputBufferData_t *data = malloc (sizeof (StreamInputBufferData_t));

  debug ("InitStream\n");
  debug (" Allocated data\n");

  if (!data)
    {
      perror ("malloc");
      exit (1);
    }

  data->SavedStream = NULL;

  debug (" curl init\n");
  data->CurlHandle = curl_easy_init ();
  if (!data->CurlHandle)
    {
      perror ("curl_easy_init");
      exit (1);
    }

  debug (" create buffer, prebuf = %ld\n", inputOpts.Prebuffer);
  inputOpts.Prebuffer = 0;
  inputOpts.BufferSize = 1024*500;
  data->buf = buffer_create (inputOpts.BufferSize, inputOpts.Prebuffer, NULL,
			     NULL, NULL, NULL, VORBIS_CHUNKIN_SIZE);
  /* Don't start the thread since we are going to be pulling data from
     the buffer instead. */
  
  if (!data->buf)
    {
      perror ("Create Buffer");
      exit (1);
    }

  debug (" set curl opts\n");
  CurlSetopts (data->CurlHandle, data->buf, inputOpts);

  debug (" init saving stream\n");
  if (inputOpts.SaveStream)
    data->SavedStream = fopen (inputOpts.SaveStream, "wb");

  pthread_create (&data->CurlThread, NULL, CurlGo, data);

  debug ("returning.\n");
  return data;
}

void StreamCleanup (StreamInputBufferData_t *data)
{
  free (data);
}

/* --------------- vorbisfile callbacks --------------- */

size_t StreamBufferRead (void *ptr, size_t size, size_t nmemb, void *arg)
{
  StreamInputBufferData_t *data = arg;
  size_t ret;

  debug ("StreamBufferRead %d bytes\n", size*nmemb);
  ret = buffer_get_data(data->buf, ptr, size, nmemb);

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

  pthread_kill (data->CurlThread, SIGTERM);
  pthread_join (data->CurlThread, NULL);

  buffer_destroy(data->buf);
  data->buf = NULL;

  if (data->SavedStream)
    fclose (data->SavedStream);

  return 0;
}

long
StreamBufferTell (void *arg)
{
  return 0;
}
