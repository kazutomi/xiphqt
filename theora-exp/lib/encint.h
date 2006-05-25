#include <limits.h>
#if !defined(_encint_H)
# define _encint_H (1)
# include "theora/theoraenc.h"
# include "internal.h"

typedef struct oc_enc_pipe_stage      oc_enc_pipe_stage;
typedef struct oc_fragment_enc_info   oc_fragment_enc_info;
typedef struct oc_mb_enc_info         oc_mb_enc_info;
typedef struct oc_mode_scheme_chooser oc_mode_scheme_chooser;
typedef struct oc_enc_vbr_ctx         oc_enc_vbr_ctx;
typedef struct oc_mcenc_ctx           oc_mcenc_ctx;
typedef struct th_enc_ctx         oc_enc_ctx;

# include "fdct.h"
# include "huffenc.h"
# include "enquant.h"

#define OC_1_LN2 (1.4426950408889634073F)

/*Encoding modes.*/
#define OC_ENC_MODE_VBR (0)
#define OC_ENC_MODE_CQI (1)
/*
Not yet implemented:
#define OC_ENC_MODE_CBR (2)
#define OC_ENC_MODE_RDO (3)
*/

/*The function used to set the speed for the current encoding mode.
  _speed: The encoding speed to use.
          Higher values should provide faster encoding, at reduced
           rate-distortion performance.
          This will always be in the range [0..._enc->speed_max].*/
typedef void (*oc_enc_set_speed_func)(oc_enc_ctx *_enc,int _speed);

/*Constants for the packet-out state machine specific to the encoder.*/

/*Next packet to emit: Data packet, but none are ready yet.*/
#define OC_PACKET_EMPTY (0)
/*Next packet to emit: Data packet, and one is ready.*/
#define OC_PACKET_READY (1)



/*An encoder pipeline stage.*/
struct oc_enc_pipe_stage{
  /*The encoder this pipeline stage belongs to.*/
  oc_enc_ctx        *enc;
  /*The next stage in the pipeline.*/
  oc_enc_pipe_stage *next;
  /*The number of rows processed so far in each plane.*/
  int                y_procd[3];
  /*Called before processing the first stripe.
    This does not need to call the next stage's start function.
    Return: 0 on success, or a negative value on error.*/
  int (*pipe_start)(oc_enc_pipe_stage *_stage);
  /*Called for each stripe as it becomes available.
    This function is responsible for calling the next function in the chain.
    It may do so in smaller or larger stripes than are passed to it, at its
     discretion.
    _y_avail: Rows 0 through _y_avail[pli] in plane pli will be available for
               processing.
    Return: 0 on success, or a negative value on error.*/
  int (*pipe_proc)(oc_enc_pipe_stage *_stage,int _y_avail[3]);
  /*Called after processing the last stripe.
    This does not need to call the next stage's end function.
    Return: 0 on success, or a negative value on error.*/
  int (*pipe_end)(oc_enc_pipe_stage *_stage);
};

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
  /*The minimum quantizer allowed for each quantizer type.*/
  unsigned char   qi_min[2];
  /*The qi index selected for this fragment.*/
  unsigned char   qii;
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

/*A structure used to track the optimal mode coding scheme, for use in
   estimating the cost of coding each mode label during the mode selection
   process.*/
struct oc_mode_scheme_chooser{
  /*Pointers to the Huffman codes associated with each mode scheme.
    The first 7 are always OC_MODE_CODESA, and the last is always
     OC_MODE_CODESB.*/
  const th_huff_code *mode_codes[8];
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



struct th_enc_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state          state;
  /*The start of the encoder pipeline.*/
  oc_enc_pipe_stage       *pipe;
  /*The maximum speed setting for the current encoding mode.*/
  int                      speed_max;
  /*The function used to set the speed level for the current encoding mode.*/
  oc_enc_set_speed_func    set_speed;
  /*The INTRA fDCT pipe stage.*/
  oc_enc_pipe_stage        fdct_pipe;
  /*The uncoded fragment copying pipe stage.*/
  oc_enc_pipe_stage        copy_pipe;
  /*The loop filter pipe stage.*/
  oc_enc_pipe_stage        loop_pipe;
  /*The border filling pipe stage.*/
  oc_enc_pipe_stage        fill_pipe;
  /*The packet assembly pipe stage.*/
  oc_enc_pipe_stage        pack_pipe;
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and goes to 1
     when a frame has been processed and a data packet is ready.*/
  int                      packet_state;
  /*Buffer in which to assemble packets.*/
  oggpack_buffer           opb;
  /*The list of flags indicating which blocks are coded in all partially coded
     super blocks.*/
  char                    *block_coded_flags;
  /*The number of block coded flags in the list.
    This is 16 times the number of super blocks with their partially coded
     flag set.*/
  int                      nblock_coded_flags;
  /*Special buffer used for the coded fragment flags.*/
  oggpack_buffer           opb_coded_flags;
  /*Encoder-specific fragment information.*/
  oc_fragment_enc_info    *frinfo;
  /*Encoder-specific macro block information.*/
  oc_mb_enc_info          *mbinfo;
  /*Context information used to perform motion estimation.*/
  oc_mcenc_ctx            *mcenc;
  /*Context information used for VBR encoding.*/
  oc_enc_vbr_ctx          *vbr;
  /*The qi value lists selected for each potential frame type.*/
  int                      qis[2][3];
  /*The number of qi values in the list for each frame type.*/
  int                      nqis[2];
  /*The number of coded fragments.*/
  int                      ncoded_frags;
  /*The current uncoded_fragi index being copied to each plane.*/
  int                      uncoded_fragii[3];
  /*The macro-block mode scheme chooser.*/
  oc_mode_scheme_chooser   mode_scheme_chooser;
  /*The motion vector scheme chosen.*/
  int                      mv_scheme;
  /*The maximum distance between keyframes.*/
  ogg_uint32_t             keyframe_frequency_force;
  /*Whether or not VP3-compatibility is enabled.*/
  int                      vp3_compatible;
  /*Whether or not the loop filter is enabled.
    This is determined each frame, based on the quantizer it is encoded with.*/
  int                      loop_filter_enabled;
  /*The bounding value array used for the loop filter.*/
  int                      bounding_values[512];
  /*The huffman tables in use.*/
  th_huff_code         huff_codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];
  /*The quantization parameters in use.*/
  th_quant_info        qinfo;
  /*Pointers to the quantization tables in use.*/
  oc_quant_table          *enquant_tables[2][3];
  /*Storage for the actual quantization tables.*/
  oc_quant_tables          enqaunt_table_data[2][3];
  /*The offset of the first DCT token for each coefficient for each plane.*/
  int                      dct_token_offs[3][64];
  /*The number of DCT tokens for each coefficient.*/
  int                      ndct_tokens[64];
  /*The DCT token lists.*/
  unsigned char          **dct_tokens;
  /*The number of extra bits entries for each coefficient.*/
  int                      nextra_bits[64];
  /*The offset of the first extra bits entry for each coefficient.*/
  int                      extra_bits_offs[64];
  /*The extra bits associated with DCT tokens.*/
  ogg_uint16_t           **extra_bits;
};

extern const int OC_MODE_SCHEMES[7][OC_NMODES];
extern const int OC_DCT_VAL_CAT_SIZES[6];
extern const int OC_DCT_VAL_CAT_SHIFTS[6];
extern const int OC_MODE_HAS_MV[OC_NMODES];
extern const th_huff_code OC_MV_CODES[2][63];

/*The number of fractional bits in bitrate statistics.*/
#define OC_BIT_SCALE (7)
/*The number of fractional bits in distortion statistics.*/
#define OC_DIS_SCALE (9)

/*Estimated bits needed to code a residual given the: quality index, color
   plane, macro-block mode, and a SAD bin.
  SAD values for a block are divided by 256 for INTRA mode and 64 for INTER
   modes to find the appropriate bin.*/
extern ogg_uint16_t OC_RES_BITRATES[64][3][OC_NMODES][16];

#if defined(OC_BITRATE_STATS)
void oc_bitrate_update_stats(oc_enc_ctx *_enc,int _huff_idxs[5][3]);
#endif


int oc_sad8_fullpel(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref,int _ref_stride);
int oc_sad8_fullpel_border(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref,int _ref_stride,ogg_int64_t _mask);
int oc_sad8_halfpel(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_stride);
int oc_sad8_halfpel_border(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_stride,
 ogg_int64_t _mask);

void oc_mode_scheme_chooser_init(oc_mode_scheme_chooser *_chooser);
void oc_mode_scheme_chooser_reset(oc_mode_scheme_chooser *_chooser);
int oc_mode_scheme_chooser_cost(oc_mode_scheme_chooser *_chooser,int _mode);
void oc_mode_scheme_chooser_update(oc_mode_scheme_chooser *_chooser,
 int _mode);
void oc_mode_scheme_chooser_add(oc_mode_scheme_chooser *_chooser,
 int _mode_counts[OC_NMODES]);

oc_mcenc_ctx *oc_mcenc_alloc(oc_enc_ctx *_enc);
void oc_mcenc_free(oc_mcenc_ctx *_mcenc);
int oc_mcenc_search_1mv(oc_mcenc_ctx *_mcenc,int _mbi,int _frame);
oc_enc_pipe_stage *oc_mcenc_prepend_to_pipe(oc_mcenc_ctx *_mcenc,
 oc_enc_pipe_stage *_next);

oc_enc_vbr_ctx *oc_enc_vbr_alloc(oc_enc_ctx *_enc);
void oc_enc_vbr_free(oc_enc_vbr_ctx *_vbr);
int oc_enc_vbr_enable(oc_enc_vbr_ctx *_vbr,th_vbr_cfg *_cfg);

void oc_enc_set_speed_null(oc_enc_ctx *_enc,int _speed);
void oc_enc_frag_intra_fdct(oc_enc_ctx *_enc,const oc_fragment *_frag,
 ogg_int16_t _dct_vals[64],int _ystride,int _framei);
int oc_enc_frag_sad(oc_enc_ctx *_enc,oc_fragment *_frag,int _dx,
 int _dy,int _pli,int _frame);
int oc_enc_partial_sb_flags_pack(oc_enc_ctx *_enc,oggpack_buffer *_opb);
int oc_enc_coded_sb_flags_pack(oc_enc_ctx *_enc,oggpack_buffer *_opb);
int oc_enc_coded_block_flags_pack(oc_enc_ctx *_enc,oggpack_buffer *_opb);
void oc_enc_do_inter_dcts(oc_enc_ctx *_enc);
void oc_enc_merge_eob_runs(oc_enc_ctx *_enc);

#endif
