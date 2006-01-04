/* reencode.c
 * - runtime reencoding of vorbis audio (usually to lower bitrates).
 *
 * $Id: reencode.c,v 1.6 2002/08/03 14:41:10 msmith Exp $
 *
 * Copyright (c) 2001   Michael Smith <msmith@labyrinth.net.au>
 * Copyright (c) 2002-4 Karl Heyes <karl@xiph.org>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "cfgparse.h"
#include "reencode.h"
#include "stream.h"
#include "encode.h"
#include "audio.h"

#define MODULE "reencode/"
#include "logging.h"

extern ogg_packet *copy_ogg_packet (ogg_packet *packet);

struct reencode *reencode_new_init (struct encoder_settings *settings)
{
    struct reencode *reenc = calloc (1, sizeof (struct reencode));
    if (reenc)
    {
        reenc->encoder = encode_create();
        if (reenc->encoder == NULL)
        {
            free (reenc);
            return NULL;
        }

        vorbis_info_init (&reenc->vi);
        vorbis_comment_init (&reenc->vc);
        reenc->settings = settings;
        reenc->need_headers = 3;
        LOG_DEBUG0("Reencoder setup complete");
    }
    return reenc;
}



void reencode_free(struct reencode *s)
{
    if (s)
    {
        vorbis_block_clear (&s->vb);
        vorbis_dsp_clear (&s->vd);
        vorbis_comment_clear (&s->vc);
        vorbis_info_clear (&s->vi);
        downmix_clear (s->downmix);
        resample_clear (s->resamp);
        encode_free (s->encoder);
        free (s);
    }
}



static int reencode_vorbis_header (struct reencode *s, ogg_packet *op)
{
    if (s->need_headers)
    {
        /* LOG_DEBUG0("processing vorbis header"); */
        if (vorbis_synthesis_headerin (&s->vi, &s->vc, op) < 0)
        {
            LOG_ERROR1 ("Failed to process Vorbis headers (%d)", 4-s->need_headers);
            return -1;
        }
        s->need_headers--;
        if (s->need_headers == 0)
        {
            char **comment;
            int channels = s->vi.channels;

            vorbis_synthesis_init (&s->vd, &s->vi);
            vorbis_block_init (&s->vd, &s->vb);

            s->settings->encode_channels = s->vi.channels;
            if (s->settings->channels && s->settings->channels != s->vi.channels)
            {
                s->downmix = downmix_initialise();
                s->settings->encode_channels = 1;
            }
            
            s->settings->encode_rate = s->vi.rate;
            if (s->settings->samplerate && s->settings->samplerate != s->vi.rate)
            {
                s->settings->encode_rate = s->settings->samplerate;
                s->resamp = resample_initialise (s->settings->encode_channels, s->vi.rate, s->settings->samplerate);
            }

            comment=s->vc.user_comments;
            while (*comment)
            {
                encode_comment (s->encoder, *comment);
                ++comment;
            }

            if (encode_setup (s->encoder, s->settings) < 0)
                return -1;
        }

        return 0;
    }
    LOG_WARN0("function called when not expecting header");
    return -1;
}


int reencode_packetin (struct reencode *s, ogg_packet *packet)
{
    int ret = 0;
    float **pcm;
    int samples;

    if (s->need_headers == 0)
    {
        if (vorbis_synthesis (&s->vb, packet) == 0)
        {
            vorbis_synthesis_blockin (&s->vd, &s->vb);
        }

        /* NOTE: we could expose pcm float buffer from encoder so that copies can be reduced further */
        while ((samples = vorbis_synthesis_pcmout (&s->vd, &pcm)) > 0)
        {
            if (s->downmix)
            {
                downmix_buffer_float(s->downmix, pcm, samples, s->vi.channels);
                if(s->resamp)
                {
                    resample_buffer_float(s->resamp, &s->downmix->buffer, samples);
                    encode_data_float(s->encoder, s->resamp->buffers, s->resamp->buffill);
                }
                else 
                    encode_data_float(s->encoder, &s->downmix->buffer, samples);
            }
            else if (s->resamp)
            {
                resample_buffer_float(s->resamp, pcm, samples);
                encode_data_float(s->encoder, s->resamp->buffers,
                        s->resamp->buffill);
            }
            else
            {
                encode_data_float(s->encoder, pcm, samples);
            }
            vorbis_synthesis_read(&s->vd, samples);
            ret = 1;
        }
        if (packet->e_o_s)
            encode_endstream (s->encoder);
        return ret;
    }
    return reencode_vorbis_header (s, packet);
}




int reencode_packetout(struct reencode *s, ogg_packet *op)
{
    if (s)
    {
        if (s->need_headers)
            return 0;
        return encode_packetout (s->encoder, op);
    }
    return -1;
}

