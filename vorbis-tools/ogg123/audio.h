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
 
 last mod: $Id: audio.h,v 1.1.2.1 2001/12/08 23:59:24 volsung Exp $
 
********************************************************************/

/* ogg123's audio playback functions */

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <ao/ao.h>

#include "buffer.h"
#include "format.h"
#include "status.h"


/* For facilitating output to multiple devices */
typedef struct audio_device_t {
  int driver_id;
  ao_device *device;
  ao_option *options;
  char *filename;
  struct audio_device_t *next_device;
} audio_device_t;

/* Structures used by callbacks */

typedef struct audio_play_arg_t {
  stat_t *stats;
  audio_device_t *devices;
} audio_play_arg_t;

typedef struct audio_reopen_arg_t {
  audio_device_t *devices;
  audio_format_t *format;
} audio_reopen_arg_t;


audio_device_t *append_audio_device(audio_device_t *devices_list,
				     int driver_id,
				     ao_option *options, char *filename);
int audio_devices_write(audio_device_t *d, void *ptr, int nbytes);
int add_ao_option(ao_option **op_h, const char *optstring);
void close_audio_devices (audio_device_t *devices);
void free_audio_devices (audio_device_t *devices);
void ao_onexit (void *arg);

int audio_play_callback (void *ptr, int nbytes, int eos, void *arg);
void audio_reopen_callback (buf_t *buf, void *arg);

audio_reopen_arg_t *new_audio_reopen_arg (audio_device_t *devices,
					  audio_format_t *fmt);

#endif /* __AUDIO_H__ */
