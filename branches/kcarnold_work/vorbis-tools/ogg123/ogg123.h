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

 last mod: $Id: ogg123.h,v 1.7.2.9 2001/08/13 00:43:20 kcarnold Exp $

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
#include "status.h"

typedef struct ogg123_options_s {
  struct {
    char *read_file;            /* File to decode */
    char shuffle;               /* Should we shuffle playing? */
    double seekpos;             /* Amount to seek by */
    int delay;                  /* delay for skip to next song */
    int nth;                    /* Play every nth chunk */
    int ntimes;                 /* Play every chunk n times */
  } playOpts;
  struct {
    long int verbose;           /* Verbose output if > 0, quiet if < 0 */
    long int quiet;             /* Be quiet (no title) */
    
    /* Status options:
     * stats[0] - currently playing file / stream
     * stats[1] - current playback time
     * stats[2] - remaining playback time
     * stats[3] - total playback time
     * stats[4] - instantaneous bitrate
     * stats[5] - average bitrate (not yet implemented)
     * stats[6] - input buffer fill %
     * stats[7] - input buffer status
     * stats[8] - output buffer fill %
     * stats[9] - output buffer status
     */
    Stat_t stats[10];
  } statOpts;
  InputOpts_t inputOpts;
  struct {
    buf_t *buffer;
    long BufferSize;
    float Prebuffer;
    int rate, channels;         /* playback params for opening audio devices */
    devices_t *devices;
    char *default_device;
  } outputOpts;
} ogg123_options_t;

void usage(void);
void play_file();
int open_audio_devices();
void signal_quit (int ignored);
void ogg123_onexit (int exitcode, void *arg);

#endif /* !defined(__OGG123_H) */
