#include <limits.h>
#if !defined(_encint_H)
# define _encint_H (1)
# include "theora/theora.h"
# include "internal.h"
# include "fdct.h"
# include "huffenc.h"
# include "enquant.h"

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
typedef struct{
  /*The DCT coefficients for coding the fragment in intra mode.
    These are computed in advance by the psycho-visual model, and then reused
     during mode decision.*/
  ogg_int16_t     dct_coeffs[64];
  /*The perceptual weight of each coefficient.
    This is an 8.8 fixed point number that should be used to scale the squared
     error of each coefficient.*/
  float           weights[64];
  /*The weight derived from the importance of this fragment.*/
  float           imp_weight;
}oc_fragment_enc_info;

/*Macro block information specific to encoding.*/
typedef struct{
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
  /*Motion vectors for a frame against the previous frame and
     against the golden frame.
    Each is a set of 3 vectors, for the co-located block in this
     frame and the previous two frames, which can be used to judge
     constant velocity and constant acceleration.
    Uninitialized MVs are (0,0).*/
  char          mvs[2][3][2];
  /*Per-block motion vectors for this frame against the previous frame.*/
  char          bmvs[4][2];
  /*Minimum motion estimation error from the analysis stage.*/
  int           aerror;
  /*Minimum 4V motion estimation error from the analysis stage.*/
  int           a4mverror;
  /*Minimum motion estimation error from the estimation stage.*/
  int           eerror;
}oc_mb_enc_info;

typedef struct oc_impmap_ctx oc_impmap_ctx;
typedef struct oc_mcenc_ctx  oc_mcenc_ctx;
typedef struct oc_psych_ctx  oc_psych_ctx;

typedef struct theora_enc_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state       state;
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and goes to 1
     when a frame has been processed and a data packet is ready.*/
  int                   packet_state;
  /*Buffer in which to assemble packets.*/
  oggpack_buffer        opb;
  /*Encoder-specific fragment information.*/
  oc_fragment_enc_info *frinfo;
  /*Encoder-specific macro block information.*/
  oc_mb_enc_info       *mbinfo;
  /*Minimum psychovisual weights for each coefficient in each plane.*/
  float                 weight_mins[3][64];
  /*Context information used to perform motion estimation.*/
  oc_mcenc_ctx         *mcenc;
  /*Context information used to generate the importance map.*/
  oc_impmap_ctx        *impmap;
  /*Context information used to generate low-level perceptual weightings.*/
  oc_psych_ctx         *psych;
  /*The huffman tables in use.*/
  theora_huff_code      huff_codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS];
  /*The scale factor for the current quality setting.*/
  float                 qscale;
  /*The quantization parameters in use.*/
  theora_quant_info     qinfo;
  /*The quantization tables.*/
  oc_quant_tables       enquant_tables[64];
  /*The number of DCT tokens for each coefficient.*/
  int                   ndct_tokens[64];
  /*The offset of the first DCT token for each coefficient.*/
  int                   dct_token_offs[64];
  /*The DCT token lists.*/
  unsigned char       **dct_tokens;
  /*The number of extra bits entries for each coefficient.*/
  int                   nextra_bits[64];
  /*The offset of the first extra bits entry for each coefficient.*/
  int                   extra_bits_offs[64];
  /*The extra bits associated with DCT tokens.*/
  ogg_uint16_t        **extra_bits;
}oc_enc_ctx;

void oc_frag_fdct(const oc_fragment *_frag,ogg_int16_t _dct_vals[64],
 int _stride);

oc_mcenc_ctx *oc_mcenc_alloc(oc_enc_ctx *_enc);
void oc_mcenc_free(oc_mcenc_ctx *_mcenc);
void oc_mcenc_analyze(oc_mcenc_ctx *_mcenc);

oc_impmap_ctx *oc_impmap_alloc(oc_enc_ctx *_enc);
void oc_impmap_free(oc_impmap_ctx *_impmap);
void oc_impmap_fill(oc_impmap_ctx *_impmap,float _duration);

oc_psych_ctx *oc_psych_alloc(oc_enc_ctx *_enc);
void oc_psych_free(oc_psych_ctx *_psych);
void oc_psych_scan(oc_psych_ctx *_psych,float _contrast);

#endif