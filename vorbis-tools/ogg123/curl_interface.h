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
 
 last mod: $Id: curl_interface.h,v 1.1.2.2 2001/08/11 02:55:37 kcarnold Exp $
 
********************************************************************/

#ifndef __CURL_INTERFACE_H
#define __CURL_INTERFACE_H

#include <pthread.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <vorbis/vorbisfile.h>

#include "buffer.h"

typedef struct InputOpts_s {
  /* Input buffer options */
  long BufferSize;
  long Prebuffer;
  
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

typedef struct StreamInputBufferData_s {
  pthread_t CurlThread;
  pthread_mutex_t ReadDataMutex;
  pthread_cond_t ReadRequestedCondition;
  pthread_cond_t ReadDoneCondition;

  CURL * CurlHandle;

  char EOS;

  size_t BytesRequested;
  unsigned char *WriteTarget;
  unsigned char *CurWritePtr;
  unsigned char ExcessData[TARGET_WRITE_SIZE];
  int ExcessDataSize;
} StreamInputBufferData_t;

buf_t *InitStream (InputOpts_t inputOpts);
size_t StreamBufferRead (void *ptr, size_t size, size_t nmemb, void *arg);
int StreamBufferSeek (void *arg, ogg_int64_t offset, int whence);
int StreamBufferClose (void *arg);
long StreamBufferTell (void *arg);

#endif /* __CURL_INTERFACE_H */
