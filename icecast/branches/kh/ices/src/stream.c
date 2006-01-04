/* stream_shared.c
 * - Stream utility functions.
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include <thread/thread.h>

#include "cfgparse.h"
#include "inputmodule.h"
#include "stream.h"
#include "reencode.h"
#include "encode.h"
#include "audio.h"
#include "metadata.h"

#define MODULE "stream/"
#include "logging.h"


struct pcm2vorbis_encode
{
    struct encoder *enc;
    struct resample *resamp;
    struct downmix *downmix;
};


void output_clear(struct output_state *state)
{
    if (state == NULL)
        return;
    /* clear comment and info */
    if (state->info_in_use)
    {
        struct output_module *mod, *next;
        LOG_DEBUG0("Clearing up output state");
        vorbis_comment_clear (&state->vc);
        vorbis_info_clear (&state->vi);

        /* clear stored headers */
        free (state->packets[0].packet);
        /* ogg_packet_clear (&state->packets[1]); */
        free (state->packets[1].packet);
        free (state->packets[2].packet);

        /* should clear individual outputs */
        mod = state->head;
        while (mod)
        {
            next = mod->next;
            /* clear each output module */
            mod->output_clear (mod);
            free (mod);
            mod = next;
        }
        state->head = NULL;
        if (state->url)
        {
            xmlFree (state->url);
            state->url = NULL;
        }
        if (state->name)
        {
            xmlFree (state->name);
            state->name = NULL;
        }
        if (state->description)
        {
            xmlFree (state->description);
            state->description = NULL;
        }
        if (state->genre)
        {
            xmlFree (state->genre);
            state->genre = NULL;
        }
        state->info_in_use = 0;
    }
}


static int _output_oggpacket (struct instance *stream, ogg_packet *op, unsigned samples)
{
    struct output_state *state = &stream->output;
    struct output_module *mod;
    ogg_int64_t granulepos;

    if (op->b_o_s)
    {
        LOG_DEBUG0 ("seen new stream, better get headers");
        state->headers = 3;
    }
    if (op->e_o_s)    LOG_DEBUG0 ("packet marked with EOS seen");

    /* need to store vorbis headers */
    if (state->headers)
    {
        ogg_packet *header;
        void *ptr;

#ifdef DEBUG
        LOG_DEBUG1 ("header packet has packetno of %lld", op->packetno);
#endif
        if (state->headers == 3)
        {
            /* LOG_DEBUG1 ("intial header packet %d", stream->id); */
            if (state->info_in_use)
            {
                LOG_DEBUG0 ("Clearing output info/comment settings");
                vorbis_comment_clear (&state->vc);
                vorbis_info_clear (&state->vi);
            }
            /* LOG_DEBUG0("state vi/vc initialised"); */
            vorbis_info_init (&state->vi);
            vorbis_comment_init (&state->vc);
            state->info_in_use = 1;
        }
        /* LOG_DEBUG0("state vi/vc headerin"); */
        vorbis_synthesis_headerin (&state->vi, &state->vc, op);

        state->headers--;
        header = &state->packets [2-state->headers];

        switch (state->headers)
        {
#if  0   
            /* vorbis_commentheader_out leaks a small amount of mem in v1.0 */
            case 1:
                LOG_DEBUG0("processing comment header");
                if (header->packet)
                {
                    LOG_DEBUG0("clearing old header");
                    ogg_packet_clear (header);
                    header->packet = NULL;
                }
                vorbis_comment_add (&state->vc, "EncodedBy=" PACKAGE_STRING);
                vorbis_commentheader_out (&state->vc, header);
                break;
#endif

            case 0:
                LOG_DEBUG2 ("samplerate is %d, channels is %d", state->vi.rate, state->vi.channels);
                state->new_headers = 1;
                state->start_pos = 0;
                /* fall thru  we need to store all 3 headers */
            case 1:
            case 2:
                /* LOG_DEBUG1("header count is %d", state->headers); */
                if (header->packet)
                {
                    free (header->packet);
                    header->packet = NULL;
                }
                ptr = malloc (op->bytes);
                memcpy (header, op, sizeof (*header));
                memcpy (ptr, op->packet, op->bytes);
                header->packet = ptr;
                header->granulepos = 0;
                break;
                
            default:
                LOG_ERROR1("headers expected value is unexpected %d", state->headers);
                break;
        }
        return 0;
    }
    /* LOG_DEBUG1("granulepos is %lld", op->granulepos); */
    /* printf("granulepos is %lld\n", op->granulepos); */

    mod = state->head;
    granulepos = op->granulepos;
    while (mod)
    {
        mod->output_send (mod, op, samples);
        op->granulepos = granulepos;
        mod = mod->next;
    }

    state->new_headers = 0;

    return 0;
}


static void flush_vorbis_input (struct instance *stream)
{
    LOG_DEBUG1("Flushing ogg packets on %d", stream->id);
    reencode_free (stream->ops->data);
    free (stream->ops);
    stream->ops = NULL;
}



void flush_ogg_packets (struct instance *stream)
{
    int send_it = 1;
    ogg_packet op;
    struct pcm2vorbis_encode *s = stream->ops->data;

    if (encode_endstream(s->enc))
    {
        LOG_DEBUG1("Flushing out encoded ogg packets stream %d", stream->id);
        while (encode_packetout (s->enc, &op) > 0)
        {
            if (send_it && _output_oggpacket (stream, &op, encoder_pkt_samples (s->enc)) < 0)
                send_it = 0;
        }
    }
    encode_free (s->enc);
    downmix_clear (s->downmix);
    resample_clear (s->resamp);
    free (s);
    free (stream->ops);
    stream->ops = NULL;
}


static int encode_pcm (struct instance *stream, input_buffer *buffer)
{
    ogg_packet op;
    int ret = 0;
    struct pcm2vorbis_encode *s = stream->ops->data;

    if (buffer->samples == 0)
        return 0;
    if (stream->downmix)
    {
        downmix_buffer_float (s->downmix, buffer->buf, buffer->samples, buffer->channels);
        if (s->resamp)
        {
            resample_buffer_float (s->resamp, &s->downmix->buffer, buffer->samples);
            encode_data_float (s->enc, s->resamp->buffers, s->resamp->buffill);
        }
        else
            encode_data_float (s->enc, &s->downmix->buffer, buffer->samples);
    }
    else if (s->resamp)
    {
        resample_buffer_float (s->resamp, buffer->buf, buffer->samples);
        encode_data_float (s->enc, s->resamp->buffers, s->resamp->buffill);
    }
    else
    {
        encode_data_float (s->enc, buffer->buf, buffer->samples);
    }
    while (encode_packetout (s->enc, &op) > 0)
    {
        if (ret == 0 && _output_oggpacket (stream, &op, encoder_pkt_samples (s->enc)) < 0)
        {
            ret = -1;
            break;
        }
    }
    return ret;
}


static int process_encode_init (struct instance *stream, input_buffer *buffer)
{
    unsigned samplerate;
    int channels;
    ogg_packet op;
    struct pcm2vorbis_encode *s;
    struct encoder *enc = encode_create();

    do
    {
        struct codec_ops *ops = stream->ops;

        if (enc == NULL)
            break;
        s = calloc (1, sizeof (struct pcm2vorbis_encode));
        if (s == NULL)
            break;
        s->enc = enc;
        LOG_INFO1 ("Restarting encoder for PCM input on stream %d", stream->id);
        samplerate = buffer->samplerate;

        if (stream->resampleoutrate)
            samplerate = stream->resampleoutrate;

        channels = buffer->channels;
        if (stream->downmix && channels == 2)
        {
            s->downmix = downmix_initialise();
            if (s->downmix == NULL)
                break;
            channels = 1;
        }

        if (buffer->samplerate != samplerate)
        {
            s->resamp = resample_initialise (channels, buffer->samplerate, samplerate);
            if (s->resamp == NULL)
                break;
        }

        if (buffer->metadata)
        {
            char **md = buffer->metadata;
            while(*md)
            {
                LOG_INFO1 ("Adding comment %s", *md);
                encode_comment (enc, *md++);
            }
        }

        stream->encode_settings.encode_rate = samplerate;
        stream->encode_settings.encode_channels = channels;
        if (encode_setup (enc, &stream->encode_settings) < 0)
        {
            LOG_ERROR1("[%d] Failed to configure encoder", stream->id);
            break;
        }
        while (encode_packetout (enc, &op) > 0)
        {
            _output_oggpacket (stream, &op, encoder_pkt_samples (enc));
        }
        ops->data = s;
        ops->flush_data = flush_ogg_packets;
        ops->process_buffer = encode_pcm;

        return 0;
    } while (0);

    if (enc) encode_free (enc);
    if (s)
    {
        if (s->resamp) resample_clear (s->resamp);
        if (s->downmix) downmix_clear (s->downmix);
        free (s);
    }
    LOG_ERROR0("Encoder failed");
    return -1;
}



static int reencode_vorbis_packet (struct instance *stream, input_buffer *buffer)
{
    ogg_packet *packet = (ogg_packet *)buffer->buf;
    int ret = 0;

    if (reencode_packetin (stream->ops->data, packet) < 0)
    {
        LOG_ERROR1("[%d] Fatal reencoding error encountered", stream->id);
        return -1;
    }
    while (1)
    {
        ogg_packet reencoded_packet;
        struct reencode *reenc = stream->ops->data;

        if ((ret = reencode_packetout (stream->ops->data, &reencoded_packet)) > 0)
        {
            _output_oggpacket (stream, &reencoded_packet, encoder_pkt_samples (reenc->encoder));
        }
        else
        {
            if (ret < 0)
            {
                LOG_ERROR1("failed getting packet from re-encoder [%d]", stream->id);
                return -1;
            }
            else
                break;
        }
    }
    return ret;
}


static int process_ogg_init (struct instance *stream, input_buffer *buffer)
{
    if (stream->encode_settings.passthru)
        stream->ops = &passthru_ptks_ops;
    else
    {
        struct codec_ops *ops = stream->ops;

        if (stream->resampleoutrate)
            stream->encode_settings.samplerate = stream->resampleoutrate;
        if (stream->downmix)
            stream->encode_settings.channels = 1;
            
        if ((ops->data = reencode_new_init (&stream->encode_settings)) == NULL)
        {
            LOG_ERROR1("failed intialising re-encoder [%d]", stream->id);
            return -1;
        }
        ops->flush_data = flush_vorbis_input;
        ops->process_buffer = reencode_vorbis_packet;
    }
    return 0;
}


static int send_vorbis_packet (struct instance *stream, input_buffer *buffer)
{
    ogg_packet *packet = (ogg_packet *)buffer->buf;

    if (_output_oggpacket (stream, packet, buffer->samples) < 0)
        return -1;

    return 0;
}


/*
 * Process a critical buffer of data 
 */
void process_critical (struct instance *stream, input_buffer *buffer)
{
    struct codec_ops *ops;
    int ret = 0;
        
    if (stream->ops && stream->ops->flush_data)
    {
        LOG_DEBUG0("Stream has restarted but no EOS of previous seen");
        stream->ops->flush_data (stream);
        free (stream->ops);
        stream->ops = NULL;
    }

    ops = malloc (sizeof (struct codec_ops));
    if (ops == NULL)
    {
        LOG_DEBUG1("stream %d - codec ops allocation error", stream->id);
        return;
    }
    stream->ops = ops;

    switch (buffer->type)
    {
        case ICES_INPUT_VORBIS_PACKET:
            ret = process_ogg_init (stream, buffer);
            break;

        case ICES_INPUT_PCM:
            ret = process_encode_init (stream, buffer);
            break;

        default:
            LOG_ERROR2 ("[%d] No known buffer type %d", stream->id, buffer->type);
            break;
    }
    if (ret < 0)  /* failed initialiser */
    {
        free (stream->ops);
        stream->ops = NULL;
    }
}

struct codec_ops passthru_ptks_ops = 
{
    NULL,
    send_vorbis_packet,
    NULL
}; 

