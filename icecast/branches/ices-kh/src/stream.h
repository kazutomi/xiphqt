/* stream.h
 * - Core streaming functions/main loop.
 *
 * $Id: stream_shared.h,v 1.3 2001/09/25 12:04:22 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __STREAM_H
#define __STREAM_H

#include "runner.h"

struct codec_ops 
{
    void *data;
    int  (*process_buffer)(struct instance *stream, input_buffer *buffer);
    void (*flush_data)(struct instance *stream);
};

extern struct codec_ops passthru_ptks_ops;
extern struct codec_ops no_encoding_ops;
extern struct codec_ops reencode_ops;
extern struct codec_ops encode_ops;
extern struct codec_ops reencode_new_ops;
extern struct codec_ops reencode_packet_ops;


void process_critical (struct instance *stream, input_buffer *buffer);
void output_clear(struct output_state *state);

#endif


