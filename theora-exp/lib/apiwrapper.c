#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>

/* libtheora header */
#include <theora/theora.h>

/* theora-exp header */
#include "theora/theoradec.h"

/* TODO: Use appropriate allocation functions? */

typedef struct {
  th_info info;

  th_setup_info *setup;

  th_dec_ctx *decode;

} th_api_wrapper;

const char *theora_version_string(void) {
    return th_version_string();
}

ogg_uint32_t theora_version_number(void) {
    return th_version_number();
}

void theora_info_init(theora_info *c)
{
    th_api_wrapper *api = calloc(1, sizeof(th_api_wrapper));

    memset(c, 0, sizeof(theora_info));

    c->codec_setup = api;

    th_info_init(&api->info);
}

void theora_info_clear (theora_info *c)
{
    th_api_wrapper *api = (th_api_wrapper *)c->codec_setup;

    if (api->setup)
        th_setup_free(api->setup);

    if (api->decode)
        th_decode_free(api->decode);

    th_info_clear(&api->info);

    free (c->codec_setup);
    
    memset(c, 0, sizeof(theora_info));
}

/* TODO: see if this is ok - which bits are we allowed to clear here? */
void theora_clear(theora_state *t)
{
    th_api_wrapper *api = (th_api_wrapper *)t->i->codec_setup;

    api->decode = NULL; /* Should we clear this here? Right now we do it in
                           info_clear. TODO */

}

int theora_decode_init (theora_state *th, theora_info *c)
{
    th_api_wrapper *api = (th_api_wrapper *)c->codec_setup;

    th->internal_encode = NULL;
    th->internal_decode = NULL; /* We don't need this */
    th->i = c;
    th->granulepos = 0;

    api->decode = th_decode_alloc(&api->info, api->setup);
}

int theora_decode_header(theora_info *ci, theora_comment *cc, ogg_packet *op)
{
    th_api_wrapper *api = (th_api_wrapper *)ci->codec_setup;
    th_info *info = &api->info;
    int ret;

    /* Rely on the fact that theora_comment and th_comment structures are
     * actually identical. Take care not to break this invariant unless you
     * change the code here as well! */

    ret = th_decode_headerin(info, (th_comment *)cc, &api->setup, op);

    if (ret < 0)
        return ret; // TODO: map_ret_val(ret);

    ci->width = info->frame_width;
    ci->height = info->frame_height;
    ci->frame_width = info->pic_width;
    ci->frame_height = info->pic_height;
    ci->offset_x = info->pic_x;
    ci->offset_y = info->pic_y;

    ci->fps_numerator = info->fps_numerator;
    ci->fps_denominator = info->fps_denominator;
    ci->aspect_numerator = info->aspect_numerator;
    ci->aspect_denominator = info->aspect_denominator;

    switch (info->colorspace) {
        case TH_CS_ITU_REC_470M:
            ci->colorspace = OC_CS_ITU_REC_470M;
            break;
        case TH_CS_ITU_REC_470BG:
            ci->colorspace = OC_CS_ITU_REC_470BG;
            break;
        default:
            ci->colorspace = OC_CS_UNSPECIFIED;
    }

    ci->target_bitrate = info->target_bitrate;
    ci->quality = info->quality;

    switch (info->pixel_fmt) {
        case TH_PF_420:
            ci->pixelformat = OC_PF_420;
            break;
        case TH_PF_422:
            ci->pixelformat = OC_PF_422;
            break;
        case TH_PF_444:
            ci->pixelformat = OC_PF_444;
            break;
        default:
            ci->pixelformat = OC_PF_RSVD;
    }

    ci->keyframe_frequency_force = 1 << info->keyframe_granule_shift;

    ci->version_major = info->version_major;
    ci->version_minor = info->version_minor;
    ci->version_subminor = info->version_subminor;

    return 0;
}

int theora_decode_packetin(theora_state *th, ogg_packet *op)
{
    th_api_wrapper *api = (th_api_wrapper *)th->i->codec_setup;
    int ret;
    ogg_int64_t gp;

    ret = th_decode_packetin(api->decode, op, &gp);

    if (ret)
        return OC_BADPACKET;

    th->granulepos = gp;

    return 0;
}

int theora_decode_YUVout(theora_state *th, yuv_buffer *yuv)
{
    th_api_wrapper *api = (th_api_wrapper *)th->i->codec_setup;
    int ret;
    th_ycbcr_buffer buf;

    ret = th_decode_ycbcr_out(api->decode, buf);

    if (!ret) {
        /* Convert... */
        yuv->y_width = buf[0].width;
        yuv->y_height = buf[0].height;
        yuv->y_stride = buf[0].ystride;

        yuv->uv_width = buf[1].width;
        yuv->uv_height = buf[1].height;
        yuv->uv_stride = buf[1].ystride;

        yuv->y = buf[0].data;
        yuv->u = buf[1].data;
        yuv->v = buf[2].data;
    }

    return ret;
}

int theora_packet_isheader(ogg_packet *op)
{
    return th_packet_isheader(op);
}

int theora_packet_iskeyframe(ogg_packet *op)
{
    return th_packet_iskeyframe(op);
}

int theora_granule_shift (theora_info *ti)
{
    return ((th_api_wrapper *)ti->codec_setup)->info.keyframe_granule_shift;
}

ogg_int64_t theora_granule_frame (theora_state *th, ogg_int64_t granulepos)
{
    return th_granule_frame (((th_api_wrapper *)th->i->codec_setup)->decode, 
            granulepos);
}

double theora_granule_time(theora_state *th, ogg_int64_t granulepos)
{
    return th_granule_time (((th_api_wrapper *)th->i->codec_setup)->decode, 
            granulepos);
}

void theora_comment_init(theora_comment *tc)
{
    th_comment_init((th_comment *)tc);
}

char *theora_comment_query(theora_comment *tc, char *tag, int count)
{
    return th_comment_query((th_comment *)tc, tag, count);
}

int theora_comment_query_count(theora_comment *tc, char *tag)
{
    return th_comment_query_count((th_comment *)tc, tag);
}

void theora_comment_clear(theora_comment *tc)
{
    th_comment_clear((th_comment *)tc);
}

