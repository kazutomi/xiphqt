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
  last mod: $Id: theora.h,v 1.5 2004/03/01 01:49:34 derf Exp $

 ********************************************************************/

#if !defined(_O_THEORA_H_)
# define _O_THEORA_H_ (1)
# include <ogg/ogg.h>

#if defined(__cplusplus)
extern "C" {
#endif



/*Possible error codes.*/
/*An invalid pointer was provided.*/
#define OC_FAULT     (-1)
/*An invalid argument was provided.*/
#define OC_EINVAL    (-10)
/*The contents of the header were incomplete, invalid, or unexpected.*/
#define OC_BADHEADER (-20)
/*The header does not belong to a Theora stream.*/
#define OC_NOTFORMAT (-21)
/*The bitstream version is too high.*/
#define OC_VERSION   (-22)
/*The specified function is not implemented.*/
#define OC_IMPL      (-23)
/*There were errors in the video data packet.*/
#define OC_BADPACKET (-24)


/*The currently defined color space tags.
  See doc/color.html for exact details on the meaning of each of these color
   spaces.*/
typedef enum{
  OC_CS_UNSPECIFIED,
  OC_CS_ITU_REC_470M,
  OC_CS_ITU_REC_470BG,
  /*The total number of currently defined color spaces:*/
  OC_CS_NSPACES
}theora_colorspace;

/*A single color plane in an image.*/
typedef struct{
  int            width;
  int            height;
  int            stride;
  unsigned char *data;
}theora_img_plane;

/*A complete image buffer.
  Note: The term YUV often used to describe a colorspace is ambiguous.
  The exact parameters of the RGB<->YUV conversion process aside, in many
   contexts the U and V channels actually have opposite meanings.
  To avoid this confusion, we are explicit: the name of the color channels is
   Y'CbCr, and they appear in that order, always.
  The prime symbol denotes that the Y channel is non-linear.
  Cb and Cr stand for "Chroma blue" and "Chroma red" respectively.*/
typedef theora_img_plane theora_ycbcr_buffer[3];

typedef struct{
  /*Bitstream version information.*/
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
     width-frame_width or 255, whichever is smaller.*/
  ogg_uint32_t       offset_x;
  /*The y offset of the displayed frame: must be no larger than
     height-frame_height, and height-frame_height-offset_y must be no larger
     than 255.*/
  ogg_uint32_t       offset_y;
  /*The frame rate, as a fraction.
    If either is 0, the frame rate is undefined.*/
  ogg_uint32_t       fps_numerator;
  ogg_uint32_t       fps_denominator;
  /*The aspect ratio of the pixels.
    If either value is zero, 1:1 is assumed.*/
  ogg_uint32_t       aspect_numerator;
  ogg_uint32_t       aspect_denominator;
  /*The color space.*/
  theora_colorspace  colorspace;
  /*The target bit-rate in bps.
    TODO: Current encoder does not support CBR mode, or anything like it.
    We also don't really know what nominal rate each quality level
     corresponds to yet.*/
  int                target_bitrate;
  /*Currently this is set so that a qi of 0 corresponds to distortions of 24
     times the JND, and each increase by 16 halves that value.
    This gives us fine discrimination at low qualities, yet effective rate
     control at high qualities.
    The qi value 63 is special, however.
    For this, the highest quality, we use one half of a JND for our threshold.
    Due to the lower bounds placed on allowable quantizers in Theora, we will
     not actually be able to achieve quality this good, but this should
     provide as close to visually lossless quality as Theora is capable of.
    We could lift the quantizer restrictions without breaking VP3.1
     compatibility, but this would result in quantized coefficients that are
     too large for the current bitstream to be able to store.
    We'd have to redesign the token syntax to store these large coefficients,
     which would make transcoding complex.*/
  int                quality;
  /*The amount to shift to extract the last keyframe number from the granule
     position.*/
  int                keyframe_granule_shift;
/*TODO: This doesn't belong here.
  It is control data, not informational data.
  We should create a generic ioctl-like interface that can be used to set
   these implementation-specific things.*/
  /*Also used for decode init to get granpos shift correct.*/
  ogg_uint32_t       keyframe_frequency_force;
  int                pp_level;
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
   frames.
  The complete table in the first group of 16 does not have to be in the same
   place as the complete table in the other groups, but the complete tables in
   the remaining four groups must all be in the same place.*/
typedef struct{
  /*The bit pattern for the code, with the LSbit of the pattern aligned in
     the LSbit of the word.*/
  ogg_uint32_t pattern;
  /*The number of bits in the code.
    This must be between 0 and 32, inclusive.*/
  int          nbits;
}theora_huff_code;

/*The quantization parameters.
  The quantizer for each coefficient is calculated as:
    Q=MAX(MIN(qmin[qti][ci!=0],scale[ci!=0][qi]*base[qti][pli][qi][ci]/100),
     1024).
  qti is the quantization type index: 0 for intra, 1 for inter.
  ci!=0 is 0 for the DC coefficient and 1 for the AC coefficient.
  qi is the quality index, ranging between 0 (low quality) and 63 (high
   quality).
  pli is the color plane index: 0 for Y', 1 for Cb, 2 for Cr.
  ci is the DCT coefficient index.
  Coefficient indices correspond to the normal 2D DCT block ordering--
   row-major with low frequencies first--NOT zig-zag order.

  Minimum quantizers are constant, and are given by:
  qmin[2][2]={{4,2},{8,4}};

  Parameters that can be stored in the bitstream are as follows:
    The two scale matrices ac_scale and dc_scale.
      scale={dc_scale,ac_scale}.
    The base matrices for each qi, qti and pli (up to 384 in all).
      In order to avoid storing a full 384 base matrices, only a sparse set of
       matrices is stored, and the rest are linearly interpolated.
      This is done as follows.
      For each qti and pli, a series of n qi ranges is defined.
      The size of each qi range can vary arbitrarily, but they must sum to 63.
      Then, n+1 matrices are specified, one for each endpoint of the ranges.
      For interpolation purposes, each range's endpoints are the first qi
       value it contains and one past the last qi value it contains.
      Fractional values are rounded to the nearest integer, with ties rounded
       away from zero.

      Base matrices are stored by reference, so if the same matrices are used
       multiple times, they will only appear once in the bitstream.
      The bitstream is also capable of omitting an entire set of ranges and
       its associated matrices if they are the same as either the previous
       set (indexed in row-major order) or if the inter set is the same as the
       inter set.

  For the current encoder, scale[ci!=0][qi] must be <= scale[ci!=0][qi-1] and
   base[qti][pli][qi][ci] must be <= base[qti][pli][qi-1][ci].
  These two conditions ensure that the actual quantizer for a given qti, pli
   and ci does not increase as qi increases.
  This is not required by the decoder.*/

/*A single base matrix.*/
typedef unsigned char theora_quant_base[64];

/*A set of qi ranges.*/
typedef struct{
  int                      nranges;
  const int               *sizes;
  const theora_quant_base *base_matrices;
}theora_quant_ranges;

/*The complete set of quantization parameters.*/
typedef struct{
  ogg_uint16_t        ac_scale[64];
  ogg_uint16_t        dc_scale[64];
  theora_quant_ranges qi_ranges[2][3];
}theora_quant_info;

/*The comment information.*/
typedef struct theora_comment{
  char **user_comments;
  int   *comment_lengths;
  int    comments;
  char  *vendor;
}theora_comment;

/*These contents of the following structures are not publicly defined by this
   API.
  Referring to their internals directly is unsupported, and may break without
   warning.*/
typedef struct theora_enc_ctx    theora_enc_ctx;
typedef struct theora_dec_ctx    theora_dec_ctx;
typedef struct theora_setup_info theora_setup_info;

extern const char *theora_version_string(void);
extern ogg_uint32_t theora_version_number(void);

extern void theora_info_init(theora_info *_info);
extern void theora_info_clear(theora_info *_info);

extern void theora_setup_free(theora_setup_info *_setup);

extern void theora_comment_init(theora_comment *_tc);
extern void theora_comment_add(theora_comment *_tc, char *_comment);
extern void theora_comment_add_tag(theora_comment *_tc,char *_tag,char *_val);
extern char *theora_comment_query(theora_comment *_tc,char *_tag,int _count);
extern int theora_comment_query_count(theora_comment *_tc,char *_tag);
extern void theora_comment_clear(theora_comment *_tc);

extern theora_enc_ctx *theora_encode_alloc(const theora_info *_info);
extern int theora_encode_set_huffman_codes(theora_enc_ctx *_enc,
 const theora_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]);
extern int theora_encode_set_quant_params(theora_enc_ctx *_enc,
 const theora_quant_info *_qinfo);
extern int theora_encode_flushheader(theora_enc_ctx *_enc,
 theora_comment *_comments,ogg_packet *_op);
extern int theora_encode_ycbcr_in(theora_enc_ctx *_enc,
 theora_ycbcr_buffer _ycbcr);
extern int theora_encode_packetout(theora_enc_ctx *_enc,int _last,
 ogg_packet *_op);
extern void theora_encode_free(theora_enc_ctx *_enc);


extern int theora_decode_header(theora_info *_info,theora_comment *_tc,
 theora_setup_info **_setup,ogg_packet *_op);
extern theora_dec_ctx *theora_decode_alloc(const theora_info *_info,
 const theora_setup_info *_setup);
extern int theora_decode_packetin(theora_dec_ctx *_dec,ogg_packet *_op);
extern int theora_decode_ycbcr_out(theora_dec_ctx *_dec,
 theora_ycbcr_buffer _ycbcr);
extern void theora_decode_free(theora_dec_ctx *_dec);

extern double theora_granule_time(void *_encdec,ogg_int64_t _granpos);


#if defined(__cplusplus)
}
#endif

#endif
