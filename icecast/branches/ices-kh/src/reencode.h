/* reencode.h
 * - reencoding functions
 *
 * $Id: reencode.h,v 1.4 2002/08/03 14:41:10 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __REENCODE_H
#define __REENCODE_H

#include <ogg/ogg.h>
#include <vorbis/codec.h>

typedef struct _reencode_tag reencode;

#include "encode.h"
#include "inputmodule.h"
#include "audio.h"

struct instance;

struct reencode
{
    struct encoder *encoder;
    struct encoder_settings *settings;

    int out_samplerate;
    int out_channels;

    int in_samplerate;
    int in_channels;

    int in_use;

    int need_headers;

    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;

    struct downmix *downmix;
    struct resample *resamp;

};

struct reencode *reencode_new_init ();

int reencode_init (struct instance *);
void reencode_free (struct reencode *s);

int reencode_pagein (struct reencode *s, ogg_page *og);
int reencode_pageout (struct reencode *s, ogg_page *og);
int reencode_packetout (struct reencode *s, ogg_packet *op);
int reencode_packetin (struct reencode *s, ogg_packet *packet);

void reencode_clear(struct reencode *s);
int reencode_send (struct instance *stream);
int reencode_flush (struct reencode *s, ogg_page *og);
void reencode_setup (struct reencode *s, long serial);
int reencode_ogg_header (struct reencode  *s, ogg_page *og, unsigned samplerate, unsigned channels);


#endif /* __REENCODE_H */

