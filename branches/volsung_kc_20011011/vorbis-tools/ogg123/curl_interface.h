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
 
 last mod: $Id: curl_interface.h,v 1.1.2.7.2.1 2001/10/17 16:58:14 volsung Exp $
 
********************************************************************/

#ifndef __CURL_INTERFACE_H
#define __CURL_INTERFACE_H

#include <pthread.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <vorbis/vorbisfile.h>

#include "buffer.h"

typedef struct StreamInputBufferData_s {
  buf_t *buf;

  pthread_t CurlThread;

  CURL * CurlHandle;

  FILE *SavedStream;
} StreamInputBufferData_t;

typedef struct InputOpts_s {
  StreamInputBufferData_t *data;

  /* Input buffer options */
  long BufferSize;
  long Prebuffer;
  char seekable;
  double totalTime;
  ogg_int64_t totalSamples;

  char *SaveStream;
  
  /* libcurl options */
  char *URL;
  int ProxyPort;
  char *ProxyHost;
  char ProxyTunnel;
  char Netrc;
  char FollowLocation;
  char *Referer;
  char *UserAgent;
  char *Cookie;
  char *CookieFile;
} InputOpts_t;

#define VORBIS_CHUNKIN_SIZE (8500)

StreamInputBufferData_t *InitStream (InputOpts_t inputOpts);
size_t StreamBufferRead (void *ptr, size_t size, size_t nmemb, void *arg);
int StreamBufferSeek (void *arg, ogg_int64_t offset, int whence);
int StreamBufferClose (void *arg);
long StreamBufferTell (void *arg);
void StreamInputCleanup (buf_t *buf);

#endif /* __CURL_INTERFACE_H */
