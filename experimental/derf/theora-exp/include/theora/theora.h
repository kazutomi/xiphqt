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
  last mod: $Id: theora.h,v 1.8 2004/03/15 22:17:32 derf Exp $

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

/*The currently defined pixel format tags.
  See the Theora specification, Section 4.4, for details on the precise sample
   locations.*/
typedef enum{
  /*Chroma decimation by 2 in both the X and Y directions.*/
  OC_PF_420,
  /*Currently reserved.*/
  OC_PF_RSVD,
  /*Chroma decimation by 2 in the X direction.*/
  OC_PF_422,
  /*No chroma decimation.*/
  OC_PF_444,
  /*The total number of currently defined pixel formats.*/
  OC_PF_NFORMATS
}theora_pixel_fmt;

/*theora_enc_ctl codes.
  By convention, these are even, to distinguish them from decoder control
   codes.
  Keep any experimental or vendor-specific values above 0x8000.*/

/*Sets the Huffman tables to use.
  The tables are copied, not of stored by reference, so they can be freed after
   this call.
  NULL may be specified to revert to the default tables.

  arg: [in]  theora_huff_code[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]
  Return: OC_FAULT:  _enc_ctx is NULL.
          OC_EINVAL: Encodng has already begun or one or more of the given
                      tables is not full or prefix-free, _buf is NULL and
                      _buf_sz is not zero, or _buf is non-NULL and _buf_sz is
                      not sizeof(theora_huff_code)*OC_NHUFFMAN_TABLES*
                      OC_NDCT_TOKENS.
          OC_IMPL:   Not supported by this implementation. */
#define OC_ENCCTL_SET_HUFFMAN_CODES (0)
/*Sets the quantization parameters to use.
  The parameters are copied, not stored by reference, so they can be freed
   after this call.
  NULL may be specified to revert to the default parameters.
  For the current encoder, scale[ci!=0][qi] must be <= scale[ci!=0][qi-1] and
   base[qti][pli][qi][ci] must be <= base[qti][pli][qi-1][ci].
  These two conditions ensure that the actual quantizer for a given qti, pli
   and ci does not increase as qi increases.

  arg: [in]  theora_quant_info
  Return: OC_FAULT:  _enc_ctx is NULL.
          OC_EINVAL: Encodng has already begun or the quantization parameters
                      does not meet one of the above stated conditions, _buf is
                      NULL and _buf_sz is not zero, or _buf is non-NULL and
                      _buf_sz is not sizeof(theora_quant_info).
          OC_IMPL:   Not supported by this implementation. */
#define OC_ENCCTL_SET_QUANT_PARAMS (2)
/*Sets the maximum distance between key frames.
  This can be changed during an encode, but will be bounded by
   1<<keyframe_granule_shift.
  If it is set before encoding begins, keyframe_granule_shift will be enlarged
   appropriately.

  arg: [in]  ogg_uint32_t: The maximum distance between key frames.
       [out] ogg_uint32_t: The actual maximum distance set.
  Return: OC_FAULT:  _enc_ctx or _buf is NULL.
          OC_EINVAL: _buf_sz is not sizeof(ogg_uint32_t).
          OC_IMPL:   Not supported by this implementation.*/
#define OC_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE (4)

/*theora_dec_ctl codes.
  By convention, these are odd, to distinguish them from encoder control codes.
  Keep any experimental or vendor-specific values above 0x8000.*/

/*Gets the maximum post-processing level.

  arg: [out] int: The maximum post-processing level.
  Return: OC_FAULT:  _dec_ctx or _buf is NULL.
          OC_EINVAL: _buf_sz is not sizeof(int).
          OC_IMPL:   Not supported by this implementation.*/
#define OC_DECCTL_GET_PPLEVEL_MAX (1)
/*Sets the post-processing level.
  By default, post-processing is disabled.

  arg: [in]  int: The new post-processing level.
       0 to disable; larger values use more CPU.
  Return: OC_FAULT:  _dec_ctx or _buf is NULL.
          OC_EINVAL: _buf_sz is not sizeof(int), or the post-processing level
                      is out of bounds.
          OC_IMPL:   Not supported by this implementation. */
#define OC_DECCTL_SET_PPLEVEL (3)
/*Sets the granule position.
  Call this after a seek, before decoding the first frame, to ensure that the
   proper granule position is returned for all subsequent frames.

  arg: [in]  ogg_int64_t: The granule position of the next frames.
  Return: OC_FAULT:  _dec_ctx or _buf is NULL.
          OC_EINVAL: _buf_sz is not sizeof(ogg_int64_t), or the granpos is
                      negative.*/
#define OC_DECCTL_SET_GRANPOS (5)

/*A single color plane in an image.*/
typedef struct{
  int            width;
  int            height;
  int            ystride;
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
    If either value is zero, the aspect ratio is undefined.
    If not specified by any external means, 1:1 should be assumed.*/
  ogg_uint32_t       aspect_numerator;
  ogg_uint32_t       aspect_denominator;
  /*The color space.*/
  theora_colorspace  colorspace;
  /*The pixel format.*/
  theora_pixel_fmt   pixel_fmt;
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
     position.
    This can be at most 31.
    The maximum distance between keyframes is 1<<keyframe_granule_shift.
    If you leave this at zero and do not set the keyframe frequency with
     OC_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, every frame will be encoded as a
     keyframe.*/
  int                keyframe_granule_shift;
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

/*The Huffman tables used by VP3.*/
const theora_huff_code OC_VP31_HUFF_CODES[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS];

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
  ogg_uint16_t        dc_scale[64];
  ogg_uint16_t        ac_scale[64];
  unsigned char       loop_filter_limits[64];
  theora_quant_ranges qi_ranges[2][3];
}theora_quant_info;

/*The quantization parameters used by VP3.*/
const theora_quant_info OC_VP31_QUANT_INFO;

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
extern int theora_encode_ctl(theora_enc_ctx *_enc,int _req,void *_buf,
 size_t _buf_sz);
extern int theora_encode_flushheader(theora_enc_ctx *_enc,
 theora_comment *_comments,ogg_packet *_op);
extern int theora_encode_ycbcr_in(theora_enc_ctx *_enc,
 theora_ycbcr_buffer _ycbcr);
extern int theora_encode_packetout(theora_enc_ctx *_enc,int _last,
 ogg_packet *_op);
extern void theora_encode_free(theora_enc_ctx *_enc);

extern int theora_decode_headerin(theora_info *_info,theora_comment *_tc,
 theora_setup_info **_setup,ogg_packet *_op);
extern theora_dec_ctx *theora_decode_alloc(const theora_info *_info,
 const theora_setup_info *_setup);
extern int theora_decode_packetin(theora_dec_ctx *_dec,const ogg_packet *_op,
 ogg_int64_t *_granpos);
extern int theora_decode_ycbcr_out(theora_dec_ctx *_dec,
 theora_ycbcr_buffer _ycbcr);
extern int theora_decode_ctl(theora_dec_ctx *_dec,int _req,void *_buf,
 size_t _buf_sz);
extern void theora_decode_free(theora_dec_ctx *_dec);

extern double theora_granule_time(void *_encdec,ogg_int64_t _granpos);


#if defined(__cplusplus)
}
#endif

#endif
