/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef _STREAM_H
#define _STREAM_H

#include <ogg/ogg.h>
#include "output.h"

#define STREAM_TYPE_UNKNOWN   0x00
#define STREAM_TYPE_VORBIS    0x01
#define STREAM_TYPE_THEORA    0x02
#define STREAM_TYPE_SPEEX     0x03
#define STREAM_TYPE_FLAC      0x04

typedef struct {
  int    serial;
  int    type;

  output_t *op;
} stream_t;

typedef struct {
  stream_t *streams;
  int       streams_size;
  int       streams_used;
} stream_ctrl_t;

int       stream_ctrl_init(stream_ctrl_t *sc);
int       stream_ctrl_free(stream_ctrl_t *sc);
stream_t *stream_ctrl_stream_new(stream_ctrl_t *sc, ogg_page *og);
stream_t *stream_ctrl_stream_get(stream_ctrl_t *sc, int serial);
int       stream_ctrl_stream_free(stream_ctrl_t *sc, int serial);

const char* stream_type_name(stream_t *stream);

#endif /* _STREAM_H */
