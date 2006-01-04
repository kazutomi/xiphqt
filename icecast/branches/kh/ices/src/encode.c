/* encode.c
 * - runtime encoding of PCM data.
 *
 * $Id: encode.c,v 1.12 2002/08/17 05:17:57 karl Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 * Copyright (c) 2002-3 Karl Heyes <karl@xiph.org>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#include "cfgparse.h"
#include "encode.h"

#define MODULE "encode/"
#include "logging.h"


struct encoder
{
    long magic;
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;

    unsigned samplerate;
    int in_use;
    int in_header;

    int flushed;
    ogg_packet headers[3];
    unsigned int samples;
    uint64_t prev_samples;
};


unsigned int encoder_pkt_samples (struct encoder *s)
{
    return s->samples;
}


void encode_free (struct encoder *s)
{
    if (s)
    {
        LOG_DEBUG0("Freeing encoder engine");
        
        vorbis_block_clear (&s->vb);
        vorbis_dsp_clear (&s->vd);
        vorbis_comment_clear (&s->vc);
        vorbis_info_clear (&s->vi);
        free (s);
    }
}


struct encoder *encode_create (void)
{
    struct encoder *s = calloc(1, sizeof(struct encoder));
    if (s == NULL)
        return NULL;
    s->magic=1;
    vorbis_comment_init (&s->vc);
    return s;
}


void encode_comment (struct encoder *s, char *str)
{
    if (s)
    {
        vorbis_comment_add (&s->vc, str);
    }
}


int encode_setup (struct encoder *s, struct encoder_settings *settings)
{
    float quality;
    long nom_br, max_br, min_br, rate, channels;

    /* do some sanity check */
    if (settings->quality > 10.0)
    {
        LOG_WARN1 ("Quality setting of %f is too high, setting to 10.0", settings->quality);
        settings->quality = 10.0;
    }

    nom_br = settings->nom_br;
    min_br = settings->min_br;
    max_br = settings->max_br;
    rate = settings->encode_rate;
    channels = settings->encode_channels;

    /* If none of these are set, it's obviously not supposed to be managed */
    if (settings->nom_br < 0 && min_br < 0 && max_br < 0)
        settings->managed = 0;

    if (settings->managed == 0 && nom_br > 0)
        if (min_br >= 0 || max_br > 0)
            settings->managed = 1;

    quality = settings->quality;

    /* Have vorbisenc choose a mode for us */
    vorbis_info_init (&s->vi);

    do
    {
        if (settings->managed)
        {
            LOG_INFO5("Encoder initialising with bitrate management: %d "
                    "channels, %d Hz, minimum bitrate %d, nominal %d, "
                    "maximum %d", channels, rate, min_br, nom_br, max_br);
            if (vorbis_encode_setup_managed (&s->vi, channels,
                        rate, max_br, nom_br, min_br))
                break;
        }
        else
        {
            if (nom_br < 0)
            {
                LOG_INFO3 ("Encoder initialising in VBR mode: %d channel(s), "
                        "%d Hz, quality %.2f", channels, rate, quality);
                if (min_br > 0 || max_br > 0)
                    LOG_WARN0 ("ignoring min/max bitrate, not supported in VBR "
                            "mode, use nominal-bitrate instead");
                if (vorbis_encode_setup_vbr (&s->vi, channels, rate, quality*0.1))
                    break;
            }
            else
            {
                LOG_INFO3 ("Encoder initialising in VBR mode: %d "
                        "channels, %d Hz, nominal %d", channels, rate, nom_br);
                if (vorbis_encode_setup_managed (&s->vi, channels,
                            rate, max_br, nom_br, max_br))
                    break;
                if (vorbis_encode_ctl (&s->vi, OV_ECTL_RATEMANAGE_SET, NULL))
                    break;
            }
        }

        if (vorbis_encode_setup_init (&s->vi))
            break;

        vorbis_analysis_init (&s->vd, &s->vi);
        vorbis_block_init (&s->vd, &s->vb);

        vorbis_comment_add (&s->vc, "EncodedBy=" PACKAGE_STRING);
        vorbis_analysis_headerout (&s->vd, &s->vc, &s->headers[0],&s->headers[1],&s->headers[2]);

        s->in_header = 3;
        s->samplerate = settings->samplerate;
        s->in_use = 1;
        s->prev_samples = 0;
        s->samples = 0;

        return 0;
    } while (0);

    LOG_INFO0("Failed to configure encoder, verify settings");
    vorbis_info_clear(&s->vi);

    return -1;
}


void encode_data_float(struct encoder *s, float **pcm, size_t samples)
{
    float **buf, **src, **dest;
    int i;
    unsigned size;

    if (samples == 0)
    {
        LOG_DEBUG0 ("request for encoding 0 samples");
        return;
    }
    if (s->magic != 1) printf ("structure has gone bad\n");

    buf = vorbis_analysis_buffer(&s->vd, samples); 

    i=s->vi.channels;
    src = pcm;
    dest = buf;
    size = samples*sizeof(float);
    for(i=0; i<s->vi.channels ; i++)
    {
        memcpy(*dest, *src, size);
        dest++;
        src++;
    }

    vorbis_analysis_wrote(&s->vd, samples);
}



int encode_packetout(struct encoder *s, ogg_packet *op)
{
    if (s->in_header)
    {
        memcpy (op, &s->headers[3-s->in_header], sizeof (*op));
#if 0
        if ((op->packet = malloc (op->bytes)) == NULL)
            return 0;
        memcpy (op->packet, s->headers[3-s->in_header].packet, op->bytes);
#endif
        s->in_header--;
        return 1;
    }
    while (vorbis_bitrate_flushpacket (&s->vd, op) == 0)
    {
        if (vorbis_analysis_blockout (&s->vd, &s->vb) == 0)
            return 0;

        vorbis_analysis (&s->vb, NULL);
        vorbis_bitrate_addblock (&s->vb);
    }
    s->samples = op->granulepos - s->prev_samples;
    s->prev_samples = op->granulepos;
    return 1;
}


int encode_endstream (struct encoder *s)
{
    vorbis_analysis_wrote(&s->vd, 0);
    return 1;
}


int parse_encode (xmlNodePtr node, void *x)
{
    struct encoder_settings *enc = x;
    struct cfg_tag encode_tags[] =
    {
        { "passthru",        get_xml_bool,  &enc->passthru},
        { "passthrough",     get_xml_bool,  &enc->passthru},
        { "nominal-bitrate", get_xml_int,   &enc->nom_br },
        { "quality",         get_xml_float, &enc->quality },
        { "minimum-bitrate", get_xml_int,   &enc->min_br },
        { "maximum-bitrate", get_xml_int,   &enc->max_br },
        { "managed",         get_xml_bool,  &enc->managed },
        { NULL , NULL, NULL }
    };

    enc->min_br = -1;
    enc->max_br = -1;
    enc->nom_br = -1;
    if (enc->passthru > 1) /* handle default setting */
        enc->passthru = 0;

    return parse_xml_tags ("encode", node->xmlChildrenNode, encode_tags);
}


