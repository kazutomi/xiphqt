/* encode.h
 * - encoding functions
 *
 * $Id: encode.h,v 1.3 2002/01/28 00:19:15 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 * Copyright (c) 2002-3 Karl Heyes <karl@karl@pts.tele2.co.uk>
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

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>


struct encoder_settings
{
    int passthru;
    int managed;
    int min_br;
    int max_br;
    int nom_br;
    float quality;
    unsigned samplerate;
    unsigned channels;
    unsigned encode_rate;
    unsigned encode_channels;
};


int parse_encode (xmlNodePtr node, void *x);

struct encoder *encode_initialise (int channels, int rate, int managed,
    int min_br, int nom_br, int max_br, float quality,
	int serial, vorbis_comment *vc);

int encode_setup (struct encoder *, struct encoder_settings *);
struct encoder *encode_create (void);

void encode_free (struct encoder *s);
void encode_clear (struct encoder *s);
void encode_comment (struct encoder *s, char *);
unsigned int encoder_pkt_samples (struct encoder *s);

int encode_endstream (struct encoder *s);
void encode_data_float (struct encoder *s, float **pcm, size_t samples);
int encode_packetout(struct encoder *s, ogg_packet *op);

int encode_dataout (struct encoder *s, ogg_page *og);
int encode_pageout (struct encoder *s, ogg_page *og);
void encode_finish (struct encoder *s);
int encode_flush (struct encoder *s, ogg_page *og);


#endif

