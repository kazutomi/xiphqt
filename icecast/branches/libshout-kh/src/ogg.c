/* vorbis.c: Ogg Vorbis data handlers for libshout 
 *
 * Copyright(c) 2003 Karl Heyes <karl@xiph.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; See the file COPYING; 
 * if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */



#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#ifdef HAVE_THEORA
#include <theora/theora.h>
#endif

#include <shout/shout.h>
#include "shout_private.h"


typedef struct _ogg_codec_tag
{
    char *name;
    ogg_stream_state os;
    unsigned headers;
    uint64_t senttime;
    void *specific;
    struct _ogg_codec_tag *next;
    int (*process_page)(shout_t *shout, struct _ogg_codec_tag *codec, ogg_page *page);
    void (*codec_free)(struct _ogg_codec_tag *codec);
} ogg_codec_t;


typedef struct _ogg_info_tag
{
    ogg_sync_state oy;

    ogg_codec_t *codecs;
    int headers_completed;
    int not_in_bos;
} ogg_info_t;


typedef struct _vorbis_codec_tag
{
    vorbis_info vi;
    vorbis_comment vc;
    int prevW;
} vorbis_codec_t;



/* -- static prototypes -- */
static int send_oggpage (shout_t *self, const void *ptr,  unsigned len);
static int send_ogg (shout_t *self, const void *ptr, unsigned len);
static void close_ogg (shout_t *self);


static void free_ogg_codecs (ogg_info_t *ogg_info)
{
    ogg_codec_t *codec;

    if (ogg_info == NULL)
       return;
    // printf ("codec handlers are being cleared\n");
    codec = ogg_info->codecs;
    while (codec)
    {
        ogg_codec_t *next = codec->next;
        codec->codec_free (codec);
        codec = next;
    }
    ogg_info->codecs = NULL;
    ogg_info->headers_completed = 0;
}


static void vorbis_codec_free (ogg_codec_t *codec)
{
    vorbis_codec_t *vorbis = codec->specific;

    vorbis_info_clear (&vorbis->vi);
    vorbis_comment_clear (&vorbis->vc);
    ogg_stream_clear (&codec->os);
    free (vorbis);
    free (codec);
}


int shout_open_vorbis(shout_t *self)
{
    if (self->format == SHOUT_FORMAT_VORBISPAGE)
        self->send = send_oggpage;
    else
    {
        ogg_info_t *ogg_info;

        self->error = SHOUTERR_MALLOC;
        ogg_info = (ogg_info_t *)calloc (1, sizeof (ogg_info_t));
        if (ogg_info == NULL)
            return -1;
        self->format_data = ogg_info;

        ogg_sync_init (&ogg_info->oy);

        self->send = send_ogg;
    }
	self->close = close_ogg;
    self->mime_type = "application/ogg";

    self->error = SHOUTERR_SUCCESS;
	return 0;
}


static int blocksize (vorbis_codec_t *vd, ogg_packet *p)
{
    int this = vorbis_packet_blocksize(&vd->vi, p);
    int ret = (this + vd->prevW)/4;

    if(!vd->prevW) {
        vd->prevW = this;
        return 0;
    }

    vd->prevW = this;
    return ret;
}

static int process_vorbis_page (shout_t *shout, ogg_codec_t *codec, ogg_page *page)
{
    ogg_packet packet;
    vorbis_codec_t *vorbis = codec->specific;
    int ret;

    ogg_stream_pagein (&codec->os, page);
    if (codec->headers < 3)
    {

        while (ogg_stream_packetout (&codec->os, &packet) > 0)
        {
           if (vorbis_synthesis_headerin (&vorbis->vi, &vorbis->vc, &packet) < 0)
           {
               /* set some error code */
               return 0;
           }
           codec->headers++;
        }
    }
    else
    {
        /* work out the number of samples in each page */
        unsigned samples = 0;

        while (ogg_stream_packetout (&codec->os, &packet) == 1)
        {
            unsigned size = blocksize (vorbis, &packet);
            samples += size;
        }
        codec->senttime += (((uint64_t)samples * 1000000) / vorbis->vi.rate);
        if (shout->senttime < codec->senttime)
            shout->senttime = codec->senttime;
#if 0
        else
            if (shout->senttime - codec->senttime > 300000)
                printf ("vorbis lag of %lld\n", shout->senttime - codec->senttime);
        printf ("vorbis sentime %lld\n", codec->senttime);
#endif
    }
    ret = shout_send_raw (shout, page->header, page->header_len);
    if (ret < page->header_len)
        return -1;
    ret = shout_send_raw (shout, page->body, page->body_len);
    if (ret < page->body_len)
        return -1;

    return page->header_len + page->body_len;
}


static ogg_codec_t *initial_vorbis_page (shout_t *shout, ogg_page *page)
{
    ogg_codec_t *codec = calloc (1, sizeof (ogg_codec_t));
    ogg_packet packet;

    vorbis_codec_t *vorbis_codec = calloc (1, sizeof (vorbis_codec_t));

    ogg_stream_init (&codec->os, ogg_page_serialno (page));
    ogg_stream_pagein (&codec->os, page);

    vorbis_info_init (&vorbis_codec->vi);
    vorbis_comment_init (&vorbis_codec->vc);

    ogg_stream_packetout (&codec->os, &packet);

    codec->name = "Vorbis";
    do
    {
        int ret;

        if (vorbis_synthesis_headerin (&vorbis_codec->vi, &vorbis_codec->vc, &packet) < 0)
        {
            break;
        }
        codec->specific = vorbis_codec;
        codec->process_page = process_vorbis_page;
        codec->codec_free = vorbis_codec_free;
        codec->headers = 1;
        codec->senttime = shout->senttime;

        ret = shout_send_raw (shout, page->header, page->header_len);
        if (ret < 0)
            break;
        ret = shout_send_raw (shout, page->body, page->body_len);
        if (ret < 0)
            break;

        return codec;
    } while (0);

    ogg_stream_clear (&codec->os);
    vorbis_info_clear (&vorbis_codec->vi);
    vorbis_comment_clear (&vorbis_codec->vc);
    free (vorbis_codec);
    free (codec);
    return NULL;
}

#ifdef HAVE_THEORA
typedef struct _theora_codec_tag
{
    theora_info ti;
    theora_comment tc;
    uint32_t granule_shift;
    double prev_time;
} theora_codec_t;


static void theora_codec_free (ogg_codec_t *codec)
{   
    theora_codec_t *theora = codec->specific;

    theora_info_clear (&theora->ti);
    theora_comment_clear (&theora->tc);
    ogg_stream_clear (&codec->os);
    free (theora);
    free (codec);
}   


static int _ilog (unsigned int v)
{
    int ret=0;
    while(v){
        ret++;
        v>>=1;
    }
    return ret;
}


static int process_theora_page (shout_t *shout, ogg_codec_t *codec, ogg_page *page)
{
    theora_codec_t *theora = codec->specific;
    ogg_packet packet;
    int ret;

    ogg_stream_pagein (&codec->os, page);
    if (ogg_page_granulepos (page) == 0)
    {
        while (ogg_stream_packetout (&codec->os, &packet) > 0)
        {
           if (theora_decode_header (&theora->ti, &theora->tc, &packet) < 0)
           {
               /* set some error code */
               // fprintf (stderr, "error from theora decode header\n");
               return -1;
           }
           codec->headers++;
        }
        if (codec->headers == 3)
        {
            theora->prev_time = 0.0;
            theora->granule_shift = _ilog (theora->ti.keyframe_frequency_force - 1);
            // printf ("fps values %ld, %ld\n", theora->ti.fps_denominator , theora->ti.fps_numerator);
        }
    }
    else
    {
        // double fps = theora->ti.fps_numerator / theora->ti.fps_denominator;
        double per_frame = (double)theora->ti.fps_denominator / theora->ti.fps_numerator * 1000000;
        double duration;
#if 0
        double from_start;
        from_start = theora_granule_time (&theora->ti, ogg_page_granulepos (page));
        duration = (from_start - theora->prev_time) * 1000000;
#endif
#if 0
        int packets = ogg_page_packets(page);

        duration = (packets / fps) * 1000000.0;
        printf ("packets %d, fps is %.2f, duration %.3f\n", packets, fps, duration);
        codec->senttime += (uint64_t)(duration + 0.5);
#endif
        ogg_int64_t granulepos = ogg_page_granulepos (page);
        if (granulepos > 0)
        {
            ogg_int64_t iframe = granulepos >> theora->granule_shift;
            ogg_int64_t pframe = granulepos - (iframe << theora->granule_shift);

            uint64_t frames = iframe + pframe;
            double new_time = (frames  * per_frame);
            duration = new_time - theora->prev_time;
            // printf ("frames %lld (%.2f), new %.2f, prev %.2f\n", frames, per_frame, new_time, theora->prev_time);
            theora->prev_time = new_time;
            
            codec->senttime += (uint64_t)(duration + 0.5);
            if (shout->senttime < codec->senttime)
                shout->senttime = codec->senttime;
#if 0
            else
                if (shout->senttime - codec->senttime > 300000)
                    printf ("theora lag of %lld\n", shout->senttime - codec->senttime);
#endif
        }
        // printf ("theora sentime %lld\n", codec->senttime);
    }
    // printf ("page no is %ld\n", ogg_page_pageno (page));
    ret = shout_send_raw (shout, page->header, page->header_len);
    if (ret < page->header_len)
        return -1;
    ret = shout_send_raw (shout, page->body, page->body_len);
    if (ret < page->body_len)
        return -1;

    return page->header_len + page->body_len;
}


static ogg_codec_t *initial_theora_page (shout_t *shout, ogg_page *page)
{
    ogg_codec_t *codec = calloc (1, sizeof (ogg_codec_t));
    ogg_packet packet;

    theora_codec_t *theora_codec = calloc (1, sizeof (theora_codec_t));

    ogg_stream_init (&codec->os, ogg_page_serialno (page));
    ogg_stream_pagein (&codec->os, page);

    theora_info_init (&theora_codec->ti);
    theora_comment_init (&theora_codec->tc);

    ogg_stream_packetout (&codec->os, &packet);

    do 
    {
        int ret;

        if (theora_decode_header (&theora_codec->ti, &theora_codec->tc, &packet) < 0)
        {
            break;
        }
        codec->specific = theora_codec;
        codec->process_page = process_theora_page;
        codec->codec_free = theora_codec_free;
        codec->headers = 1;
        codec->senttime = shout->senttime;

        ret = shout_send_raw (shout, page->header, page->header_len);
        if (ret < page->header_len)
            break;
        ret = shout_send_raw (shout, page->body, page->body_len);
        if (ret < page->body_len)
            break;

        codec->name = "Theora";

        return codec;
    } while (0);

    ogg_stream_clear (&codec->os);
    theora_info_clear (&theora_codec->ti);
    theora_comment_clear (&theora_codec->tc);
    free (theora_codec);
    free (codec);
    return NULL;
}
#endif

/* catch all for unknown codecs */

static void unhandled_codec_free (ogg_codec_t *codec)
{
    ogg_stream_clear (&codec->os);
    free (codec);
}


static int process_unhandled_page (shout_t *shout, ogg_codec_t *codec, ogg_page *page)
{
    int ret;

    ret = shout_send_raw (shout, page->header, page->header_len);
    if (ret < 0)
        return SHOUTERR_SOCKET;
    ret = shout_send_raw (shout, page->body, page->body_len);

    return ret;
}


static ogg_codec_t *initial_unhandled_page (shout_t *shout, ogg_page *page)
{
    ogg_codec_t *codec = calloc (1, sizeof (ogg_codec_t));
    codec->process_page = process_unhandled_page;
    codec->codec_free = unhandled_codec_free;
    codec->name = "not timed";
    ogg_stream_init (&codec->os, ogg_page_serialno (page));

    do
    {
        int ret = shout_send_raw (shout, page->header, page->header_len);
        if (ret < 0)
            break;
        ret = shout_send_raw (shout, page->body, page->body_len);
        if (ret < 0)
            break;
        return codec;
    } while (0);

    ogg_stream_clear (&codec->os);
    free (codec);
    return NULL;
}


static int process_initial_page (shout_t *shout, ogg_page *page)
{
	ogg_info_t *ogg_info = (ogg_info_t *)shout->format_data;
    ogg_codec_t *codec;
    int ret = SHOUTERR_SUCCESS;

    if (ogg_info->not_in_bos)
    {
        free_ogg_codecs (ogg_info);
    }
    ogg_info->not_in_bos = 0;
    do
    {
        codec = initial_vorbis_page (shout, page);
        if (codec)
            break;
#ifdef HAVE_THEORA
        codec = initial_theora_page (shout, page);
        if (codec)
            break;
#endif
        /* any others */
        codec = initial_unhandled_page (shout, page);
        if (codec)
            break;
        shout->error = SHOUTERR_INSANE;
        ret = -1;

    } while (0);

    if (codec)
    {
        /* add codec to list */
        /* printf ("Adding codec handler (%s) to list\n", codec->name); */
        codec->next = ogg_info->codecs;
        ogg_info->codecs = codec;
    }
    return ret;
}


static int send_ogg (shout_t *self, const void *ptr, unsigned len)
{
	ogg_info_t *ogg_info = (ogg_info_t *)self->format_data;
	ogg_page page;
    void *buffer;

    /* lets chop up the data into pages */
	buffer = ogg_sync_buffer (&ogg_info->oy, len);
	memcpy (buffer, ptr, len);
	ogg_sync_wrote (&ogg_info->oy, len);

	while (ogg_sync_pageout (&ogg_info->oy, &page) == 1)
    {
        ogg_codec_t *codec;

        if (ogg_page_bos (&page))
        {
            /* printf ("serial of bos page is %d\n", ogg_page_serialno (&page)); */
            process_initial_page (self, &page);
            return self->error;
        }
        ogg_info->not_in_bos = 1;

        codec = ogg_info->codecs;
        while (codec)
        {
            if (ogg_page_serialno (&page) == codec->os.serialno)
            {
                // printf ("page handler is %s, %lld", codec->name, codec->senttime);
                codec->process_page (self, codec, &page);
                // printf ("\n");
                return self->error;
            }

            codec = codec->next;
        }
    }
    return 0;
}


static void close_ogg(shout_t *self)
{
   free_ogg_codecs (self->format_data);
}


static int send_oggpage (shout_t *self, const void *ptr, unsigned unused)
{
    const ogg_page *page = ptr;
    unsigned len = page->header_len + page->body_len;

    if (self->pending_queue == NULL)
    {
        struct iovec vecs [2];
        int ret;

        vecs[0] . iov_base = page->header;
        vecs[0] . iov_len  = page->header_len;
        vecs[1] . iov_base = page->body;
        vecs[1] . iov_len  = page->body_len;

        ret = sock_writev (self->socket, vecs, 2);
        if (ret == (int)len)
            return self->error = SHOUTERR_SUCCESS;
        if (ret < 0)
        {
            if (sock_recoverable (sock_error()))
                ret = 0;
            else
            {
                self->state = SHOUT_NOCONNECT;
                return self->error = SHOUTERR_SOCKET;
            }
        }
        if (ret < page->header_len)
        {
            if (shout_queue_raw (self, page->header+ret, page->header_len-ret) < 0)
                return self->error;
            ret = 0;
        }
        else
            ret -= page->header_len;
        if (shout_queue_raw (self, page->body+ret, page->body_len-ret) < 0)
            return self->error;
           
        return self->error = SHOUTERR_SUCCESS;
    }
    if (shout_queue_raw (self, page->header, page->header_len) < 0)
        return self->error;
    if (shout_queue_raw (self, page->body, page->body_len) < 0)
        return self->error;

    if (shout_send_pending (self) < 0)
        return self->error;

    return self->error = SHOUTERR_SUCCESS;
}

