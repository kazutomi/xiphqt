/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: format.h,v 1.1.2.2 2001/12/11 05:29:08 volsung Exp $

 ********************************************************************/

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "transport.h"


typedef struct audio_format_t {
  int big_endian;
  int word_size;
  int signed_sample;
  int rate;
  int channels;
} audio_format_t;


typedef struct decoder_stats_t {
  double total_time;  /* seconds */
  double current_time;   /* seconds */
  long   instant_bitrate;
  long   avg_bitrate;
} decoder_stats_t;


/* Severity constants */
enum { ERROR, WARNING, INFO };

typedef struct decoder_callbacks_t {
  void (* printf_error) (void *arg, int severity, char *message, ...);
  void (* printf_metadata) (void *arg, int verbosity, char *message, ...);
} decoder_callbacks_t;


struct format_t;

typedef struct decoder_t {
  data_source_t *source;
  audio_format_t request_fmt;
  audio_format_t actual_fmt;
  struct format_t *format;
  decoder_callbacks_t *callbacks;
  void *callback_arg;
  void *private;
} decoder_t;

typedef struct format_t {
  char *name;

  int (* can_decode) (data_source_t *source);
  decoder_t* (* init) (data_source_t *source, audio_format_t *audio_fmt,
		       decoder_callbacks_t *callbacks, void *callback_arg);
  int (* read) (decoder_t *decoder, void *ptr, int nbytes, int *eos, 
		audio_format_t *audio_fmt);
  decoder_stats_t* (* statistics) (decoder_t *decoder);
  void (* cleanup) (decoder_t *decoder);
} format_t;

format_t *get_format_by_name (char *name);
format_t *select_format (data_source_t *source);
int audio_format_equal (audio_format_t *a, audio_format_t *b);
void audio_format_copy  (audio_format_t *source, audio_format_t *dest);

decoder_stats_t *malloc_decoder_stats (decoder_stats_t *to_copy);

#endif /* __FORMAT_H__ */
