/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id: theora.h,v 1.1 2003/09/29 04:00:41 derf Exp $

 ********************************************************************/

#if !defined(_O_THEORA_H_)
# define _O_THEORA_H_ (1)
# include <ogg/ogg.h>

/*Use a composite (absolute key frame/relative delta frame) granule pos.*/
#define OC_COMPOSITE_GRANPOS (1)

typedef struct{
  int            width;
  int            height;
  int            stride;
  unsigned char *data;
}oc_img_plane;

typedef oc_img_plane oc_yuv_buffer[3];

typedef enum{
  OC_CS_UNSPECIFIED,
  OC_CS_ITU_REC_470M,
  OC_CS_ITU_REC_470BG,
  /*The total number of currently defined color spaces:*/
  OC_CS_NSPACES
}theora_colorspace;

typedef struct{
  unsigned char      version_major;
  unsigned char      version_minor;
  unsigned char      version_subminor;
  /*The encoded width: must be a multiple of 16, and less than 1048576.*/
  ogg_uint32_t       width;
  /*The encoded height: must be a multiple of 16, and less than 1048576.*/
  ogg_uint32_t       height;
  /*Whether or not chroma components are decimated in the x direction.*/
  int                chroma_x_decimated;
  /*Whether or not chroma components are decimated in the y direction.*/
  int                chroma_y_decimated;
  /*The displayed width: must be no larger than width.*/
  ogg_uint32_t       frame_width;
  /*The displayed height: must be no larger than height.*/
  ogg_uint32_t       frame_height;
  /*The x offset of the displayed frame: must be no larger than
     width-frame_width.*/
  ogg_uint32_t       offset_x;
  /*The y offset of the displayed frame: must be no larger than
     height-frame_height.*/
  ogg_uint32_t       offset_y;
  ogg_uint32_t       fps_numerator;
  ogg_uint32_t       fps_denominator;
  /*The aspect ratio of the pixels.
    If either value is zero, 1:1 is assumed.*/
  ogg_uint32_t       aspect_numerator;
  ogg_uint32_t       aspect_denominator;
  theora_colorspace  colorspace;
  /*Also used for decode init to get granpos shift correct.*/
  ogg_uint32_t       keyframe_frequency_force;
  /*TODO: Units?*/
  int                target_bitrate;
  /*TODO: Quantify.*/
  int                quality;
  /*Information stored from the codec setup header.*/
  void              *codec_setup;

/*TODO: None of these really belong here.
  They are control data, not informational data.*/
  /*TODO: what does this control? "quick encode/decode."
    Answer: This controls motion search complexity during encoding.
    It has no effect on decoding.
    It is my opinion that such things should be replaced by a general
     speed/size trade-off parameter for encoding only.*/
  int            quick_p;
  /*TODO: What do these control?*/
  int            dropframes_p;
  int            keyframe_auto_p;
  ogg_uint32_t   keyframe_frequency;
  ogg_uint32_t   keyframe_data_target_bitrate;
  ogg_int32_t    keyframe_auto_threshold;
  ogg_uint32_t   keyframe_mindistance;
  ogg_int32_t    noise_sensitivity;
}theora_info;

#define OC_NHUFFMAN_TABLES (80)
#define OC_NDCT_TOKENS     (32)

/*A Huffman code for a Theora DCT token.
  Each set of Huffman codes in a given table must form a complete, prefix-free
   code.
  There is no requirement that all the tokens in a table have a valid code,
   but the current encoder is not optimized to take advantage of this.
  If there is not at least one table with a code for every token in each of
   the five groups of 16 tables, then the encoder may fail to encode certain
   frames.*/
typedef struct{
  /*The bit pattern for the code, with the LSbit of the pattern aligned in
     the LSbit of the word.*/
  ogg_uint32_t bits;
  /*The number of bits in the code.
    This must be between 0 and 32, inclusive.*/
  int          len;
}theora_huff_code;

/*The quantization parameters.
  The quantizer for each coefficient is calculated as:
    Q=MAX(MIN(qmin[i],scale[i]*coeff[i]/100),1024).
  For Quality Index qi, scale[i] is dc_scale[qi] for i==0, and
   ac_scale[qi] otherwise.
  For the current encoder, ac_scale[qi] must be >= ac_scale[qi-1], and
   dc_scale[qi] must be >= dc_scale[qi-1].
  That is, scale values must be monotonically non-increasing as the quality
   index increases.
  This is not required by the decoder.
  For intra frames, coeff[i] is either luma_coeffs[i] for the Y' plane or
   chroma_coeffs[i] for the Cb and Cr planes.
  For inter frames, coeff[i] is inter_coeffs[i].
  For intra frames, qmin[i] is 4 for i==0, and 2 otherwise.
  For inter frames, qmin[i] is 8 for i==0, and 4 otherwise.
  Coefficients indices correspond to the normal 2D DCT block ordering--
   row-major with low frequencies first--NOT zig-zag order.*/
typedef struct{
  ogg_uint16_t  ac_scale[64];
  ogg_uint16_t  dc_scale[64];
  unsigned char luma_coeffs[64];
  unsigned char chroma_coeffs[64];
  unsigned char inter_coeffs[64];
}theora_quant_info;


typedef struct theora_comment{
  char **user_comments;
  int   *comment_lengths;
  int    comments;
  char  *vendor;
}theora_comment;

#define OC_FAULT     (-1)
#define OC_EINVAL    (-10)
#define OC_BADHEADER (-20)
#define OC_NOTFORMAT (-21)
#define OC_VERSION   (-22)
#define OC_IMPL      (-23)
#define OC_BADPACKET (-24)

typedef struct theora_enc_ctx theora_enc_ctx;

extern const char *theora_version_string(void);
extern ogg_uint32_t theora_version_number(void);

extern void theora_info_init(theora_info *_info);
extern void theora_info_clear(theora_info *_info);

extern void theora_comment_init(theora_comment *_tc);
extern void theora_comment_add(theora_comment *_tc, char *_comment);
extern void theora_comment_add_tag(theora_comment *_tc,char *_tag,char *_val);
extern char *theora_comment_query(theora_comment *_tc,char *_tag,int _count);
extern int theora_comment_query_count(theora_comment *_tc,char *_tag);
extern void theora_comment_clear(theora_comment *_tc);

extern theora_enc_ctx *theora_encode_alloc(theora_info *_info);
extern int theora_encode_set_huffman_codes(theora_enc_ctx *_enc,
 const theora_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]);
extern int theora_encode_set_quant_params(theora_enc_ctx *_enc,
 const theora_quant_info *_qinfo);
extern int theora_encode_flushheader(theora_enc_ctx *_enc,
 theora_comment *_comments,ogg_packet *_op);
extern int theora_encode_yuv_in(theora_enc_ctx *_enc,oc_yuv_buffer _yuv);
extern int theora_encode_packetout(theora_enc_ctx *_enc,int _last,
 ogg_packet *_op);
extern void theora_encode_free(theora_enc_ctx *_enc);

/*
extern int theora_decode_header(theora_info *ci, theora_comment *cc,
                                ogg_packet *op);
extern int theora_decode_init(theora_state *th, theora_info *c);
extern int theora_decode_packetin(theora_state *th,ogg_packet *op);
extern int theora_decode_YUVout(theora_state *th,oc_yuv_buffer *yuv);
extern double theora_granule_time(theora_state *th,ogg_int64_t granulepos);
extern void theora_clear(theora_state *t);*/

#endif
