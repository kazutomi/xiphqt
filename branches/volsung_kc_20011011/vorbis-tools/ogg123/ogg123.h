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

 last mod: $Id: ogg123.h,v 1.7.2.12.2.7 2001/12/11 18:46:23 volsung Exp $

 ********************************************************************/

#ifndef __OGG123_H__
#define __OGG123_H__

#include <ogg/os_types.h>
#include "audio.h"

/* Compatibility fixes */

#ifdef __sun
#include <alloca.h>
#endif

/* SunOS 4 does on_exit() and everything else does atexit() */
#ifdef HAVE_ATEXIT
#define ATEXIT(x) (atexit(x))
#else
#ifdef HAVE_ON_EXIT
#define ATEXIT(x) (on_exit( (void (*)(int, void*))(x) , NULL)
#else
#define ATEXIT(x)
#warning "Neither atexit() nor on_exit() is present.  Bad things may happen when the application terminates."
#endif
#endif
 

typedef struct ogg123_options_t {
  long int verbosity;         /* Verbose output if > 1, quiet if 0 */

  int shuffle;               /* Should we shuffle playing? */
  int delay;                  /* delay for skip to next song */
  int nth;                    /* Play every nth chunk */
  int ntimes;                 /* Play every chunk n times */
  double seekpos;             /* Position in file to seek to */

  long buffer_size;           /* Size of audio buffer */
  float prebuffer;            /* Percent of buffer to fill before playing */
  char *default_device;       /* Name of default driver to use */

  audio_device_t *devices;    /* Audio devices to use */

  double status_freq;         /* Number of status updates per second */
} ogg123_options_t;

typedef struct signal_request_t {
  int skipfile;
  int exit;
  int pause;
  ogg_int64_t ticks;
} signal_request_t;

#endif /* __OGG123_H__ */
