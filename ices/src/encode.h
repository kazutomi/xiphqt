/* encode.h
 * - encoding functions
 *
 * $Id: encode.h,v 1.3.2.1 2002/02/07 09:11:11 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __ENCODE_H
#define __ENCODE_H

#include <ogg/ogg.h>
#include <vorbis/codec.h>

typedef struct {
    int initialised;

    int managed;
    int min_br;
    int nom_br;
    int max_br;
    double quality;
    double max_page_time;

    int serial;
    
	ogg_stream_state os;
    vorbis_comment vc;
	vorbis_block vb;
	vorbis_dsp_state vd;
	vorbis_info vi;

    int samples_in_current_page;
    int samplerate;
    ogg_int64_t prevgranulepos;
	int in_header;
} encoder_state;

int encode_open_module(process_chain_element *mod, module_param_t *params);

#endif

