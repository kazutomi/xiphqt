#define OC_BITRATE_STATS (1)
#include <limits.h>
#if !defined(_encint_H)
# define _encint_H (1)
# include "theora/theora.h"
# include "internal.h"

typedef struct oc_fragment_enc_info   oc_fragment_enc_info;
typedef struct oc_mb_enc_info         oc_mb_enc_info;
typedef struct oc_border_enc_info     oc_border_enc_info;
typedef struct oc_mode_scheme_chooser oc_mode_scheme_chooser;
typedef struct oc_impmap_ctx          oc_impmap_ctx;
typedef struct oc_mcenc_ctx           oc_mcenc_ctx;
typedef struct oc_psych_ctx           oc_psych_ctx;
typedef struct theora_enc_ctx         oc_enc_ctx;

# include "fdct.h"
# include "huffenc.h"
# include "enquant.h"

#define OC_1_LN2 (1.4426950408889634073F)

#define OC_NTEST_MODES (1)

/*Next packet to emit: Codec info header.*/
#define OC_PACKET_INFO_HDR    (-3)
/*Next packet to emit: Comment header.*/
#define OC_PACKET_COMMENT_HDR (-2)
/*Next packet to emit: Codec setup header.*/
#define OC_PACKET_SETUP_HDR   (-1)
/*Next packet to emit: Data packet, but none are ready yet.*/
#define OC_PACKET_EMPTY       (0)
/*Next packet to emit: Data packet, and one is ready.*/
#define OC_PACKET_READY       (1)
/*No more packets to emit.*/
#define OC_PACKET_DONE        (INT_MAX)

/*Fragment information specific to encoding.*/
struct oc_fragment_enc_info{
  /*The DCT coefficients for coding the fragment in intra mode.
    These are computed in advance by the psycho-visual model, and then reused
     during mode decision.*/
  ogg_int16_t     dct_coeffs[64];
  /*The maximum allowed distortion allowed for this coefficient.
    The quantizer is free to choose any quantized value that does not move the
     reconstructed value more than this amount away from the true coefficient
     value.*/
  ogg_uint16_t    tols[64];
  /*The weight derived from the importance of this fragment.*/
  float           imp_weight;
  /*The actual error encoded by the residual after mode selection.*/
  int             eerror;
};

/*Macro block information specific to encoding.*/
struct oc_mb_enc_info{
  /*The neighboring macro blocks with motion vectors available for the current
     frame.*/
  int           cneighbors[4];
  /*The number of current neighbors available.*/
  int           ncneighbors;
  /*The neighboring macro blocks with motion vectors available for the
     previous frame.*/
  int           pneighbors[4];
  /*The number of previous neighbors available.*/
  int           npneighbors;
  /*Motion vectors for a macro block for the current frame and the previous
     two frames.
    Each is a set of 2 vectors against the previous frame and against the
     golden frame, which can be used to judge constant velocity and constant
     acceleration.
    Uninitialized MVs are (0,0).*/
  char          mvs[3][2][2];
  /*Per-block motion vectors for this frame against the previous frame.*/
  char          bmvs[4][2];
  /*Minimum motion estimation error from the analysis stage.*/
  int           aerror;
  /*Minimum 4V motion estimation error from the analysis stage.*/
  int           aerror4mv;
};

/*Information used to extend fragments which straddle the cropping border in
   an optimal manner.*/
struct oc_border_enc_info{
  /*The indices of pixels with valid values first and then padding pixels.*/
  unsigned char   pi[64];
  /*The amount to scale pixel values by before multiplying by the extension
     matrix.*/
  int             shift;
  /*The matrix used to extend the fragment into the padding region, in signed
     1.15 fixed-point format.*/
  ogg_int16_t   **ext_matrix;
};

/*A structure used to track the optimal mode coding scheme, for use in
   estimating the cost of coding each mode label during the mode selection
   process.*/
struct oc_mode_scheme_chooser{
  /*Pointers to the Huffman codes associated with each mode scheme.
    The first 7 are always OC_MODE_CODESA, and the last is always
     OC_MODE_CODESB.*/
  const theora_huff_code *mode_codes[8];
  /*Pointers to the a list containing the index of each mode in the mode
     alphabet used by each scheme.
    The first entry points to the dynamic scheme0_ranks, while the remaining
     7 point to the constant entries stored in OC_MODE_SCHEMES.*/
  const int              *mode_ranks[8];
  /*The ranks for each mode when coded with scheme 0.
    These are optimized so that the more frequent modes have lower ranks.*/
  int                     scheme0_ranks[OC_NMODES];
  /*The list of modes, sorted in descending order of frequency, that
     corresponds to the ranks above.*/
  int                     scheme0_list[OC_NMODES];
  /*The number of times each mode has been chosen so far.*/
  int                     mode_counts[OC_NMODES];
  /*The list of mode coding schemes, sorted in ascending order of bit cost.*/
  int                     scheme_list[8];
  /*The number of bits used by each mode coding scheme.*/
  int                     scheme_bits[8];
};



struct theora_enc_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state          state;
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and goes to 1
     when a frame has been processed and a data packet is ready.*/
  int                      packet_state;
  /*Buffer in which to assemble packets.*/
  oggpack_buffer           opb;
  /*Special buffer used for the coded fragment flags.*/
  oggpack_buffer           opb_coded_flags;
  /*Encoder-specific fragment information.*/
  oc_fragment_enc_info    *frinfo;
  /*Encoder-specific macro block information.*/
  oc_mb_enc_info          *mbinfo;
  /*Encoder-specific border information.*/
  oc_border_enc_info       border_info[16];
  /*Minimum psychovisual tolerances for each coefficient in each plane.*/
  ogg_uint16_t             tol_mins[3][64];
  /*The macro-block mode scheme chooser.*/
  oc_mode_scheme_chooser   mode_scheme_chooser;
  /*The motion vector scheme chosen.*/
  int                      mv_scheme;
  /*Context information used to perform motion estimation.*/
  oc_mcenc_ctx            *mcenc;
  /*Context information used to generate the importance map.*/
  oc_impmap_ctx           *impmap;
  /*Context information used to generate low-level perceptual weightings.*/
  oc_psych_ctx            *psych;
  /*The huffman tables in use.*/
  theora_huff_code         huff_codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS];
  /*The scale factor for the current quality setting.*/
  float                    qscale;
  /*The quantization parameters in use.*/
  theora_quant_info        qinfo;
  /*Pointers to the quantization tables in use.*/
  oc_quant_tables         *enquant_tables[2][3];
  /*Storage for the actual quantization tables.*/
  oc_quant_tables          enqaunt_table_data[2][3];
  /*The offset of the first DCT token for each coefficient for each plane.*/
  int                      dct_token_offs[3][64];
  /*The number of DCT tokens for each coefficient.*/
  int                      ndct_tokens[64];
  /*The DCT token lists.*/
  unsigned char          **dct_tokens;
  int                    **dct_tok_fragi;
  /*The number of extra bits entries for each coefficient.*/
  int                      nextra_bits[64];
  /*The offset of the first extra bits entry for each coefficient.*/
  int                      extra_bits_offs[64];
  /*The extra bits associated with DCT tokens.*/
  ogg_uint16_t           **extra_bits;
};

int oc_sad8_fullpel(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref,int _ref_stride);
int oc_sad8_fullpel_border(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref,int _ref_stride,ogg_int64_t _mask);
int oc_sad8_halfpel(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_stride);
int oc_sad8_halfpel_border(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_stride,
 ogg_int64_t _mask);

void oc_enc_frag_intra_fdct(oc_enc_ctx *_enc,const oc_fragment *_frag,
 ogg_int16_t _dct_vals[64],int _ystride,int _framei);

oc_mcenc_ctx *oc_mcenc_alloc(oc_enc_ctx *_enc);
void oc_mcenc_free(oc_mcenc_ctx *_mcenc);
void oc_mcenc_analyze(oc_mcenc_ctx *_mcenc);
int oc_mcenc_search_1mv(oc_mcenc_ctx *_mcenc,int _mbi,int _frame);

oc_impmap_ctx *oc_impmap_alloc(oc_enc_ctx *_enc);
void oc_impmap_free(oc_impmap_ctx *_impmap);
void oc_impmap_fill(oc_impmap_ctx *_impmap,float _duration);

oc_psych_ctx *oc_psych_alloc(oc_enc_ctx *_enc);
void oc_psych_free(oc_psych_ctx *_psych);
void oc_psych_scan(oc_psych_ctx *_psych,float _contrast);

/*Estimated bits needed to code a residual given the: quality index, color
   plane, macro-block mode, and a SAD bin.
  SAD values for a block are divided by 256 for INTRA mode and 64 for INTER
   modes to find the appropriate bin.*/
extern ogg_uint16_t OC_RES_BITRATES[64][3][OC_NMODES][16];

#if defined(OC_BITRATE_STATS)
void oc_bitrate_update_stats(oc_enc_ctx *_enc,int _huff_idxs[5][3]);
#endif


#endif