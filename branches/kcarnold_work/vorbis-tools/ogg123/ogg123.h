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

 last mod: $Id: ogg123.h,v 1.7.2.8 2001/08/12 03:59:31 kcarnold Exp $

 ********************************************************************/

#ifndef __OGG123_H
#define __OGG123_H

/* Common includes */
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <ao/ao.h>

#ifdef __sun
#include <alloca.h>
#endif

#include "ao_interface.h"
#include "curl_interface.h"

typedef struct ogg123_options_s {
  char *read_file;            /* File to decode */
  char shuffle;               /* Should we shuffle playing? */
  long int verbose;           /* Verbose output if > 0, quiet if < 0 */
  long int quiet;             /* Be quiet (no title) */
  double seekpos;             /* Amount to seek by */
  FILE *instream;             /* Stream to read from. */
  char *default_device;       /* default device for playback */
  devices_t *outdevices;      /* Streams to write to. */
  long outbuffer_size;            /* Size of the buffer in chunks. */
  float outprebuffer;              /* number of chunks to prebuffer */
  int rate, channels;         /* playback params for opening audio devices */
  int delay;                  /* delay for skip to next song */
  int nth;                    /* Play every nth chunk */
  int ntimes;                 /* Play every chunk n times */
  InputOpts_t inputOpts;
} ogg123_options_t;

void usage(void);
void play_file(ogg123_options_t *opt);
int open_audio_devices(ogg123_options_t *opt, int rate, int channels);
void signal_quit (int ignored);
void ogg123_onexit (int exitcode, void *arg);

#endif /* !defined(__OGG123_H) */
