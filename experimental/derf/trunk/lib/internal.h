#if !defined(_internal_H)
# define _internal_H (1)
# include <stdlib.h>
# include "theora/theora.h"
# include "ocintrin.h"
# include "huffman.h"
# include "quant.h"

/*Thank you Microsoft, I know the order of operations.*/
#pragma warning(disable:4554)

/*This library's version.*/
# define OC_VENDOR_STRING "derf's experimental encoder library " __DATE__

/*Theora bitstream version.*/
# define OC_VERSION_MAJOR (3)
# define OC_VERSION_MINOR (2)
# define OC_VERSION_SUB   (0)

/*A keyframe.*/
#define OC_INTRA_FRAME (0)
/*A predicted frame.*/
#define OC_INTER_FRAME (1)
/*A frame of unknown type (frame type decision has not yet been made).*/
#define OC_UNKWN_FRAME (-1)

/*The amount of padding to add to the reconstructed frame buffers on all
   sides.
  This is used to allow unrestricted motion vectors without special casing.
  This must be a multiple of 2.*/
#define OC_UMV_PADDING (16)

/*Frame classification indices.*/
/*The previous golden frame.*/
#define OC_FRAME_GOLD (0)
/*The previous frame.*/
#define OC_FRAME_PREV (1)
/*The current frame.*/
#define OC_FRAME_SELF (2)

/*The input or output buffer.*/
#define OC_FRAME_IO   (3)

/*Macroblock modes.*/
/*Macro block is invalid: It is never coded.*/
#define OC_MODE_INVALID        (-1)

/*Encoded with no motion compensated prediction.*/
#define OC_MODE_INTRA          (0)
/*Encoded difference from the same macro block in the previous frame.*/
#define OC_MODE_INTER_NOMV     (1)
/*Encoded difference from the previous frame offset by the given motion
   vector.*/
#define OC_MODE_INTER_MV       (2)
/*Encoded difference from the previous frame offset by the last coded motion
   vector.*/
#define OC_MODE_INTER_MV_LAST  (3)
/*Encoded difference from the previous frame offset by the second to last
   coded motion vector.*/
#define OC_MODE_INTER_MV_LAST2 (4)
/*Encoded difference from the previous frame offset by the individual motion
   vectors given for each block.*/
#define OC_MODE_INTER_MV_FOUR  (5)
/*Encoded difference from the same macro block in the previous golden frame.*/
#define OC_MODE_GOLDEN_NOMV    (6)
/*Encoded difference from the previous golden frame offset by the given motion
   vector.*/
#define OC_MODE_GOLDEN_MV      (7)
/*The number of (coded) modes.*/
#define OC_NMODES              (8)

/*Macro block is not coded.*/
#define OC_MODE_NOT_CODED      (8)

/*Predictor bit flags.*/
/*Left.*/
#define OC_PL  (1)
/*Upper-left.*/
#define OC_PUL (2)
/*Up.*/
#define OC_PU  (4)
/*Upper-right.*/
#define OC_PUR (8)

/*Constants for the packet state machine common between encoder and decoder.*/

/*Next packet to emit/read: Codec info header.*/
#define OC_PACKET_INFO_HDR    (-3)
/*Next packet to emit/read: Comment header.*/
#define OC_PACKET_COMMENT_HDR (-2)
/*Next packet to emit/read: Codec setup header.*/
#define OC_PACKET_SETUP_HDR   (-1)
/*No more packets to emit/read.*/
#define OC_PACKET_DONE        (INT_MAX)




/*A map from a super block to fragment numbers.*/
typedef int oc_sb_map[4][4];
/*A map from a macro block to fragment numbers.*/
typedef int oc_mb_map[3][4];



/*Super block information.
  Super blocks are 32x32 segments of pixels in a single color plane indexed
   in image order.
  Internally, super blocks are broken up into a 2x2 pattern of macro blocks,
   each of which contains a 2x2 pattern of blocks, each of which is an 8x8
   block of pixels.
  Macro blocks, and the blocks within them, are indexed in a special order
   called a "Hilbert curve" within the super block.

  In order to differentiate between the Hilbert-curve indexing strategy and
   the regular image order indexing strategy, blocks indexed in image order
   are called "fragments".
  Fragments are indexed in image order, left to right, then bottom to top,
   from Y plane to Cb plane to Cr plane.*/
typedef struct{
  unsigned  coded_fully:1;
  unsigned  coded_partially:1;
  unsigned  mb_valid:4;
  oc_sb_map map;
}oc_sb;



/*Macro block information.
  Only macro blocks in the Y plane matter.
  Fragments in the chroma planes inherit their mode and motion vectors from
   the co-located macro block in the Y plane.
  Therefore macro block information is kept in a separate array from super
   blocks, to avoid unused space in the other planes.*/
typedef struct{
  /*The current macro block mode.
    A negative number indicates the macro block lies entirely outside the
     coded frame.*/
  int           mode;
  /*The X location of the macro block's upper-left hand pixel.*/
  int           x;
  /*The Y location of the macro block's upper-right hand pixel.*/
  int           y;
  /*The fragments that belong to this macro block in each color plane.
    Fragments are stored in image order (left to right then top to bottom).
    When chroma components are decimated, the extra fragments have an index of
     -1.*/
  oc_mb_map     map;
}oc_mb;



/*Information about a fragment which intersects the border of the displayable
   region.
  This marks which pixels belong to the displayable region, and is used to
   ensure that pixels outside of this region are never referenced.
  This allows applications to pass in buffers that are really the size of the
   displayable region without causing a seg fault.*/
typedef struct{
  /*A bit mask marking which pixels are in the displayable region.
    Pixel (x,y) corresponds to bit (y<<3|x).*/
  ogg_int64_t mask;
  /*The number of pixels in the displayable region.
    This is always positive, and always less than 64.*/
  int         npixels;
}oc_border_info;



/*Fragment information.*/
typedef struct{
  /*A flag indicating whether or not this fragment is coded.*/
  unsigned        coded:1;
  /*A flag indicating that all of this fragment lies outside the displayable
     region of the frame.
    Note the contrast with an invalid macro block, which is outside the coded
     frame, not just the displayable one.*/
  unsigned        invalid:1;
  /*The mode of the macroblock this fragment belongs to.*/
  int             mbmode:6;
  /*The prediction-corrected DC component.*/
  int             dc:16;
  /*A pointer to the portion of an image covered by this fragment in several
     images.
    The first three are reconstructed frame buffers, while the last is the
     input image buffer.
    The appropriate stride value is determined by the color plane the fragment
     belongs in.*/
  unsigned char  *buffer[4];
  /*Information for fragments which lie partially outside the displayable
     region.
    For fragments completely inside or outside this region, this is NULL.*/
  oc_border_info *border;
  /*The motion vector used for this fragment.*/
  char            mv[2];
}oc_fragment;



/*A description of each fragment plane.*/
typedef struct{
  /*The number of fragments in the horizontal direction.*/
  int nhfrags;
  /*The number of fragments in the vertical direction.*/
  int nvfrags;
  /*The offset of the first fragment in the plane.*/
  int froffset;
  /*The total number of fragments in the plane.*/
  int nfrags;
  /*The number of super blocks in the horizontal direction.*/
  int nhsbs;
  /*The number of super blocks in the vertical direction.*/
  int nvsbs;
  /*The offset of the first super block in the plane.*/
  int sboffset;
  /*The total number of super blocks in the plane.*/
  int nsbs;
}oc_fragment_plane;



/*Common state information between the encoder and decoder.*/
typedef struct{
  /*The stream information.*/
  theora_info           info;
  /*The chroma subsampling method.
    The lowe-order bit indicates X-axis decimation, while the next bit
     indicates Y-axis decimation.*/
  int                   chroma_type;
  /*The fragment plane descriptions.*/
  oc_fragment_plane     fplanes[3];
  /*The total number of fragments in a single frame.*/
  int                   nfrags;
  /*The list of fragments, indexed in image order.*/
  oc_fragment          *frags;
  /*The total number of super blocks in a single frame.*/
  int                   nsbs;
  /*The list of super blocks, indexed in image order.*/
  oc_sb                *sbs;
  /*The number of macro blocks in the X direction.*/
  int                   nhmbs;
  /*The number of macro blocks in the Y direction.*/
  int                   nvmbs;
  /*The total number of macro blocks.*/
  int                   nmbs;
  /*The list of macro blocks, indexed in super block order.
    That is, the macro block corresponding to the macro block mbi in super
     block sbi is (sbi<<2|mbi).*/
  oc_mb                *mbs;
  /*The list of coded fragments, in coded order.*/
  int                  *coded_fragis;
  /*The number of coded fragments in each plane.*/
  int                   ncoded_fragis[3];
  /*The list of uncoded fragments.
    This just past the end of the list, which is in reverse order, and
     uses the same block of allocated storage as the coded_fragis list.*/
  int                  *uncoded_fragis;
  /*The number of uncoded fragments in each plane.*/
  int                   nuncoded_fragis[3];
  /*The list of coded macro blocks in the Y plane, in coded order.*/
  int                  *coded_mbis;
  /*The number of coded macro blocks in the Y plane.*/
  int                   ncoded_mbis;
  /*A copy of the image data used to fill the input pointers in each fragment.
    If the data pointers or strides change, these input pointers must be
     re-populated.*/
  theora_ycbcr_buffer   input;
  /*The number of unique border patterns.*/
  int                   nborders;
  /*The storage for the border info for all border fragments.
    This data is pointed to from the appropriate fragments.*/
  oc_border_info        borders[16];
  /*The index of the buffers being used for each OC_FRAME_* reference frame.*/
  int                   ref_frame_idx[3];
  /*The actual buffers used for the previously decoded frames.*/
  theora_ycbcr_buffer   ref_frame_bufs[3];
  /*The storage for the reference frame buffers.*/
  unsigned char        *ref_frame_data;
  /*The frame number of the last keyframe.*/
  ogg_int64_t           keyframe_num;
  /*The frame number of the current frame.*/
  ogg_int64_t           curframe_num;
  /*The granpos of the current frame.*/
  ogg_int64_t           granpos;
  /*The type of the current frame.*/
  int                   frame_type;
  /*The quality index of the current frame.*/
  int                   qi;
  /*The dequantization tables.*/
  oc_quant_tables      *dequant_tables[2][3];
  oc_quant_tables       dequant_table_data[2][3];
  /*Loop filter strength parameters.*/
  unsigned char         loop_filter_limits[64];
}oc_theora_state;



/*The function type used to fill in the chroma plane motion vectors for a
   macro block when 4 different motion vectors are specified in the luma
   plane.
  _cbmvs: The chroma block-level motion vectors to fill in.
  _lmbmv: The luma macro-block level motion vector to fill in for use in
           prediction.
  _lbmvs: The luma block-level motion vectors.*/
typedef void (*oc_set_chroma_mvs_func)(char _cbmvs[4][2],
 const char _lbmvs[4][2]);



/*A map from the index in the zig zag scan to the coefficient number in a
   block.
  The extra 64 entries send out of bounds indexes to index 64.
  This is used to safely ignore invalid zero runs when decoding
   coefficients.*/
extern const int OC_FZIG_ZAG[128];
/*A map from the coefficient number in a block to its index in the zig zag
   scan.*/
extern const int OC_IZIG_ZAG[64];
/*The predictor frame to use for each macro block mode.*/
extern const int OC_FRAME_FOR_MODE[OC_NMODES];
/*A map from physical macro block ordering to bitstream macro block
   ordering within a super block.*/
extern const int OC_MB_MAP[2][2];
/*A list of the indices in the oc_mb.map array that can be valid for each of
   the various chroma decimation types.*/
extern const int OC_MB_MAP_IDXS[4][12];
/*The number of indices in the oc_mb.map array that can be valid for each of
   the various chroma decimation types.*/
extern const int OC_MB_MAP_NIDXS[4];
/*A table of functions used to fill in the Cb,Cr plane motion vectors for a
   macro block when 4 different motion vectors are specified in the luma
   plane.*/
extern const oc_set_chroma_mvs_func OC_SET_CHROMA_MVS_TABLE[4];



int oc_ilog(unsigned _v);
void **oc_malloc_2d(size_t _height,size_t _width,size_t _sz);
void **oc_calloc_2d(size_t _height,size_t _width,size_t _sz);
void oc_free_2d(void **_ptr);

void oc_ycbcr_buffer_flip(theora_ycbcr_buffer _dst,
 const theora_ycbcr_buffer _src);

int oc_dct_token_skip(int _token,int _extra_bits);

void oc_frag_recon_intra(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t *_residue);
void oc_frag_recon_inter(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter2(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue);
int oc_frag_pred_dc(const oc_fragment *_frag,
 const oc_fragment_plane *_fplane,int _x,int _y,int _pred_last[3]);

int oc_state_init(oc_theora_state *_state,const theora_info *_info);
void oc_state_clear(oc_theora_state *_state);
void oc_state_borders_fill(oc_theora_state *_state,int _refi);
void oc_state_fill_buffer_ptrs(oc_theora_state *_state,int _buf_idx,
 theora_ycbcr_buffer _img);
int oc_state_mbi_for_pos(oc_theora_state *_state,int _mbx,int _mby);
int oc_state_get_mv_offsets(oc_theora_state *_state,int *_offset0,
 int *_offset1,int _dx,int _dy,int _ystride,int _pli);
void oc_state_frag_recon(oc_theora_state *_state,const oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[64],int _ncoefs,
 const ogg_uint16_t _iquant[64]);
void oc_state_frag_copy(const oc_theora_state *_state,const int *_fragis[3],
 int _nfragis[3],int _dst_frame,int _src_frame);
void oc_state_loop_filter(oc_theora_state *_state,int _frame);
#if defined(OC_DUMP_IMAGES)
int oc_state_dump_frame(const oc_theora_state *_state,int _frame);
#endif

#endif
