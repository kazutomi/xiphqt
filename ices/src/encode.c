/* encode.c
 * - runtime encoding of PCM data.
 *
 * $Id: encode.c,v 1.6.2.1 2002/02/07 09:11:10 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#include "config.h"
#include "encode.h"
#include "process.h"

#define MODULE "encode/"
#include "logging.h"

static void close_module(process_chain_element *mod) 
{
    encoder_state *s = mod->priv_data;

	if(s)
	{
	    LOG_DEBUG0("Clearing encoder engine");
		ogg_stream_clear(&s->os);
		vorbis_block_clear(&s->vb);
		vorbis_dsp_clear(&s->vd);
		vorbis_info_clear(&s->vi);
		free(s);
	}
}

static int event_handler(process_chain_element *self, event_type ev, 
        void *param)
{
    switch(ev)
    {
        case EVENT_SHUTDOWN:
            close_module(self);
            break;
            /* FIXME: handle next track event */
        default:
            return -1;
    }

    return 0;
}

static void encode_finish(encoder_state *s)
{
	ogg_packet op;
	vorbis_analysis_wrote(&s->vd, 0);

	while(vorbis_analysis_blockout(&s->vd, &s->vb)==1)
	{
		vorbis_analysis(&s->vb, &op);
		ogg_stream_packetin(&s->os, &op);
	}

}

static void append_page(ref_buffer *buf, ogg_page *page)
{
    int old = buf->len;
    buf->len += page->header_len + page->body_len;
    buf->buf = realloc(buf->buf, buf->len);
    memcpy(buf->buf + old, page->header, page->header_len);
    memcpy(buf->buf + old + page->header_len, page->body, page->body_len);
}

static int encode_flush(encoder_state *s, ogg_page *og)
{
	int result = ogg_stream_pageout(&s->os, og);

	if(s->in_header)
	{
		LOG_ERROR0("Unhandled case: flushing stream before headers have been "
				  "output. Behaviour may be unpredictable.");
	}

	if(result<=0)
		return 0;
	else
		return 1;
}

/* Requires little endian data (currently) */
static void encode_data(encoder_state *s, signed char *buf, int bytes, 
        int bigendian)
{
	float **buffer;
	int i,j;
	int channels = s->vi.channels;
	int samples = bytes/(2*channels);

	buffer = vorbis_analysis_buffer(&s->vd, samples);

	if(bigendian)
	{
		for(i=0; i < samples; i++)
		{
			for(j=0; j < channels; j++)
			{
				buffer[j][i]=((buf[2*(i*channels + j)]<<8) |
						      (0x00ff&(int)buf[2*(i*channels + j)+1]))/32768.f;
			}
		}
	}
	else
	{
		for(i=0; i < samples; i++)
		{
			for(j=0; j < channels; j++)
			{
				buffer[j][i]=((buf[2*(i*channels + j) + 1]<<8) |
						      (0x00ff&(int)buf[2*(i*channels + j)]))/32768.f;
			}
		}
	}

	vorbis_analysis_wrote(&s->vd, samples);

    s->samples_in_current_page += samples;
}


/* Returns:
 *   0     No output at this time
 *   >0    Page produced
 *
 * Caller should loop over this to ensure that we don't end up with
 * excessive buffering in libvorbis.
 */
static int encode_dataout(encoder_state *s, ogg_page *og)
{
	ogg_packet op;
	int result;

	if(s->in_header)
	{
		result = ogg_stream_flush(&s->os, og);
		if(result==0) 
		{
			s->in_header = 0;
			return encode_dataout(s,og);
		}
		else
			return 1;
	}
	else
	{
		while(vorbis_analysis_blockout(&s->vd, &s->vb)==1)
		{
			vorbis_analysis(&s->vb, NULL);
            vorbis_bitrate_addblock(&s->vb);

            while(vorbis_bitrate_flushpacket(&s->vd, &op)) 
    			ogg_stream_packetin(&s->os, &op);
		}

        /* We don't want to buffer too many samples in one page when doing
         * live encoding - that's fine for non-live encoding, but breaks
         * badly when doing things live. 
         * So, we flush the stream if we have too many samples buffered
         */
        if(s->samples_in_current_page > (int)(s->samplerate * s->max_page_time))
        {
            LOG_DEBUG1("Forcing flush: Too many samples in current page (%d)", 
                    s->samples_in_current_page);
            result = ogg_stream_flush(&s->os, og);
        }
        else
		    result = ogg_stream_pageout(&s->os, og);

		if(result==0)
			return 0;
		else /* Page found! */
        {
            s->samples_in_current_page -= ogg_page_granulepos(og) - 
                    s->prevgranulepos;
            s->prevgranulepos = ogg_page_granulepos(og);
			return 1;
        }
	}
}

static int encode_initialise(encoder_state *s, int channels, int rate)
{
	ogg_packet h1,h2,h3;
    int ret;

    /* FIXME: Return values from this need checking! */
    if(s->managed)
        LOG_INFO5("Encoder initialising with bitrate management: %d channels, "
                 "%d Hz, minimum bitrate %d, nominal %d, maximum %d",
                 channels, rate, s->min_br, s->nom_br, s->max_br);
    else
	    LOG_INFO3("Encoder initialising at %d channels, %d Hz, quality %f", 
		    	channels, rate, s->quality);

	vorbis_info_init(&s->vi);

    if(s->managed)
	    ret = vorbis_encode_init(&s->vi, channels, rate, s->max_br, 
                s->nom_br, s->min_br);
    else
        ret = vorbis_encode_init_vbr(&s->vi, channels, rate, s->quality*0.1);

    if(ret) {
        LOG_DEBUG1("Encoder init failed: returned %d", ret);
        return ret;
    }

	vorbis_analysis_init(&s->vd, &s->vi);
	vorbis_block_init(&s->vd, &s->vb);

	ogg_stream_init(&s->os, s->serial);

	vorbis_analysis_headerout(&s->vd, &s->vc, &h1,&h2,&h3);
	ogg_stream_packetin(&s->os, &h1);
	ogg_stream_packetin(&s->os, &h2);
	ogg_stream_packetin(&s->os, &h3);

	s->in_header = 1;
    s->samplerate = rate;
    s->samples_in_current_page = 0;
    s->prevgranulepos = 0;

    return 0;
}

static void encode_data_float(encoder_state *s, float **pcm, int samples)
{
	float **buf;
	int i;

	buf = vorbis_analysis_buffer(&s->vd, samples); 

	for(i=0; i < s->vi.channels; i++)
	{
		memcpy(buf[i], pcm[i], samples*sizeof(float));
	}

	vorbis_analysis_wrote(&s->vd, samples);

    s->samples_in_current_page += samples;
}

static int encode_chunk(instance_t *instance, void *self, 
        ref_buffer *in, ref_buffer **out)
{
    encoder_state *enc = self;
    ogg_page page;

    *out = NULL;

    if(in->flags & FLAG_BOS) {
        if(enc->initialised) {
            encode_finish(enc);
            while(encode_flush(enc, &page) > 0) {
                if(*out == NULL)
                    *out = new_ref_buffer(MEDIA_VORBIS, NULL, 0);
                append_page(*out, &page);
            }
            /* FIXME: rewrite the below func. 
            encode_clear(enc);
            */
        }
        encode_initialise(enc, in->channels, in->rate);
        enc->initialised = 1;
    }

    if(!enc->initialised)
        LOG_ERROR0("encode called without being initialised, absent BOS flag?");

    if(in->subtype == SUBTYPE_PCM_FLOAT) {
        float **pcm = (float **)in->buf;
        encode_data_float(enc, pcm, in->len);
    }
    else {
        signed char *buf = in->buf;
        encode_data(enc, buf, in->len, in->subtype == SUBTYPE_PCM_BE_16);
    }

    while(encode_dataout(enc, &page) > 0) {
        if(*out == NULL)
            *out = new_ref_buffer(MEDIA_VORBIS, NULL, 0);
        append_page(*out, &page);
    }

    release_buffer(in);

    return ((*out)?(*out)->len:0);
}

int encode_open_module(process_chain_element *mod, module_param_t *params)
{
    encoder_state *enc = calloc(1, sizeof(encoder_state));
    module_param_t *paramstart = params;

    mod->name = "process-encode";
    mod->input_type = MEDIA_PCM;
    mod->output_type = MEDIA_VORBIS;

    mod->process = encode_chunk;
    mod->event_handler = event_handler;

    mod->priv_data = enc;

    enc->managed = 0;
    enc->quality = 0.3;

    enc->nom_br = 128000;
    enc->min_br = enc->max_br = -1;
    enc->max_page_time = 2.0; /* Seconds */

    while(params) {

        if(!strcmp(params->name, "quality")) {
            /* FIXME: Ensure this is bounded to [0,10] */
            enc->managed = 0;
            enc->quality = atof(params->value) * 0.1;
        }
        else if(!strcmp(params->name, "nominal-bitrate")) {
            enc->managed = 1;
            enc->nom_br = atoi(params->value);
        }
        else if(!strcmp(params->name, "minimum-bitrate")) {
            enc->managed = 1;
            enc->min_br = atoi(params->value);
        }
        else if(!strcmp(params->name, "maximum-bitrate")) {
            enc->managed = 1;
            enc->max_br = atoi(params->value);
        }
        else if(!strcmp(params->name, "page-length-threshold")) {
            enc->max_page_time = atof(params->value);
        }
        else
            LOG_ERROR1("Unrecognised module parameter \"%s\"", params->name);

        params = params->next;
    }

    if(enc->managed)
        LOG_INFO3("Bitrate management engine enabled: "
                "min = %d, nom = %d, max = %d", enc->min_br, 
                enc->nom_br, enc->max_br);
    else
        LOG_INFO1("Full VBR encoding: quality level %f", enc->quality);

    config_free_params(paramstart);

    return 0;
}




