#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "idct.h"



/*A map from the index in the zig zag scan to the coefficient number in a
   block.*/
const int OC_FZIG_ZAG[64]={
   0, 1, 8,16, 9, 2, 3,10,
  17,24,32,25,18,11, 4, 5,
  12,19,26,33,40,48,41,34,
  27,20,13, 6, 7,14,21,28,
  35,42,49,56,57,50,43,36,
  29,22,15,23,30,37,44,51,
  58,59,52,45,38,31,39,46,
  53,60,61,54,47,55,62,63
};

/*A map from the coefficient number in a block to its index in the zig zag
   scan.*/
const int OC_IZIG_ZAG[64]={
   0, 1, 5, 6,14,15,27,28,
   2, 4, 7,13,16,26,29,42,
   3, 8,12,17,25,30,41,43,
   9,11,18,24,31,40,44,53,
  10,19,23,32,39,45,52,54,
  20,22,33,38,46,51,55,60,
  21,34,37,47,50,56,59,61,
  35,36,48,49,57,58,62,63
};

/*The predictor frame to use for each macro block mode.*/
const int OC_FRAME_FOR_MODE[8]={
  /*OC_MODE_INTRA*/
  OC_FRAME_SELF,
  /*OC_MODE_INTER*/
  OC_FRAME_PREV,
  /*OC_MODE_INTER_MV*/
  OC_FRAME_PREV,
  /*OC_MODE_INTER_MV_LAST*/
  OC_FRAME_PREV,
  /*OC_MODE_INTER_MV_LAST2*/
  OC_FRAME_PREV,
  /*OC_MODE_INTER_MV_FOUR*/
  OC_FRAME_PREV,
  /*OC_MODE_GOLDEN*/
  OC_FRAME_GOLD,
  /*OC_MODE_GOLDEN_MV*/
  OC_FRAME_GOLD,
};

/*A map from physical macro block ordering to bitstream macro block
   ordering within a super block.*/
const int OC_MB_MAP[2][2]={{0,3},{1,2}};

/*A list of the indices in the oc_mb.map array that can be valid for each of
   the various chroma decimation types.*/
const int OC_MB_MAP_IDXS[4][12]={
  {0,1,2,3,4,5,6,7,8,9,10,11},
  {0,1,2,3,4,6,8,10},
  {0,1,2,3,4,5,8,9},
  {0,1,2,3,4,8}
};

/*The number of indices in the oc_mb.map array that can be valid for each of
   the various chroma decimation types.*/
const int OC_MB_MAP_NIDXS[4]={12,8,8,6};

/*The number of extra bits that are coded with each of the DCT tokens.
  Each DCT token has some fixed number of additional bits (possibly 0) stored
   after the token itself, containing, for example, coefficient magnitude,
   sign bits, etc.*/
const int OC_DCT_TOKEN_EXTRA_BITS[OC_NDCT_TOKENS]={
  0,0,0,2,3,4,12,3,6,
  0,0,0,0,
  1,1,1,1,2,3,4,5,6,10,
  1,1,1,1,1,3,4,
  2,3
};



int oc_ilog(unsigned _v){
  int ret;
  for(ret=0;_v;ret++)_v>>=1;
  return ret;
}



/*Determines the number of blocks or coefficients to be skipped for a given
   token value.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: A positive value indicates that number of coefficients are to be
           skipped in the current block.
          Otherwise, the negative of the return value indicates that number of
           blocks are to be ended.*/
typedef (*oc_token_skip_func)(int _token,int _extra_bits);

/*Handles the end of block tokens.*/
static int oc_token_skip_eob(int _token,int _extra_bits){
  static const int NBLOCKS_ADJUST[OC_NDCT_EOB_TOKEN_MAX]={1,2,3,4,8,16,0};
  return -_extra_bits-NBLOCKS_ADJUST[_token];
}

/*Handles the pure zero run tokens.*/
static int oc_token_skip_zrl(int _token,int _extra_bits){
  return _extra_bits+1;
}

/*Handles a normal coefficient value token.*/
static int oc_token_skip_val(void){
  return 1;
}

/*Handles a category 1A zero run/coefficient value combo token.*/
static int oc_token_skip_run_cat1a(int _token){
  return _token-OC_DCT_RUN_CAT1A+2;
}

/*Handles category 1b and 2 zero run/coefficient value combo tokens.*/
static int oc_token_skip_run(int _token,int _extra_bits){
  static const int NCOEFFS_ADJUST[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    7,11,2,3
  };
  static const int NCOEFFS_MASK[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    3,7,0,1
  };
  _token-=OC_DCT_RUN_CAT1B;
  return (_extra_bits&NCOEFFS_MASK[_token])+NCOEFFS_ADJUST[_token];
}

/*A jump table for computing the number of coefficients or blocks to skip for
   a given token value.
  This reduces all the conditional branches, etc., needed to parse these token
   values down to one indirect jump.*/
static const oc_token_skip_func OC_TOKEN_SKIP_TABLE[OC_NDCT_TOKENS]={
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_zrl,
  oc_token_skip_zrl,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  oc_token_skip_run,
  oc_token_skip_run,
  oc_token_skip_run,
  oc_token_skip_run
};

/*Determines the number of blocks or coefficients to be skipped for a given
   token value.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: A positive value indicates that number of coefficients are to be
           skipped in the current block.
          Otherwise, the negative of the return value indicates that number of
           blocks are to be ended.*/
int oc_dct_token_skip(int _token,int _extra_bits){
  return (*OC_TOKEN_SKIP_TABLE[_token])(_token,_extra_bits);
}


/*The function type used to fill in the U,V plane motion vectors for a macro
   block when 4 different motion vectors are specified in the luma plane.
  This version is for use with no chroma decimation.
  _cbmvs: The chroma block-level motion vectors to fill in.
  _lmbmv: The luma macro-block level motion vector to fill in for use in
           prediction.
  _lbmvs: The luma block-level motion vectors.*/
static void oc_set_chroma_mvs00(int _cbmvs[4][2],int _lmbmv[2],
 const char _lbmvs[4][2]){
  int dx;
  int dy;
  memcpy(_cbmvs,_lbmvs,4*sizeof(_lbmvs[0]));
  dx=_lbmvs[0][0]+_lbmvs[1][0]+_lbmvs[2][0]+_lbmvs[3][0];
  dy=_lbmvs[0][1]+_lbmvs[1][1]+_lbmvs[2][1]+_lbmvs[3][1];
  _lmbmv[0]=OC_DIV_ROUND_POW2(dx,2,2);
  _lmbmv[1]=OC_DIV_ROUND_POW2(dy,2,2);
}

/*The function type used to fill in the U,V plane motion vectors for a macro
   block when 4 different motion vectors are specified in the luma plane.
  This version is for use with chroma decimated in the X direction.
  _cbmvs: The chroma block-level motion vectors to fill in.
  _lmbmv: The luma macro-block level motion vector to fill in for use in
           prediction.
  _lbmvs: The luma block-level motion vectors.*/
static void oc_set_chroma_mvs01(int _cbmvs[4][2],int _lmbmv[2],
 const char _lbmvs[4][2]){
  int dxa;
  int dya;
  int dxb;
  int dyb;
  dxa=_lbmvs[0][0]+_lbmvs[1][0];
  dya=_lbmvs[0][1]+_lbmvs[1][1];
  _cbmvs[0][0]=OC_DIV_ROUND_POW2(dxa,1,1);
  _cbmvs[0][1]=OC_DIV_ROUND_POW2(dya,1,1);
  dxb=_lbmvs[2][0]+_lbmvs[3][0];
  dyb=_lbmvs[2][1]+_lbmvs[3][1];
  _cbmvs[2][0]=OC_DIV_ROUND_POW2(dxb,1,1);
  _cbmvs[2][1]=OC_DIV_ROUND_POW2(dyb,1,1);
  dxa+=dxb;
  dxa+=dxb;
  _lmbmv[0]=OC_DIV_ROUND_POW2(dxa,2,2);
  _lmbmv[1]=OC_DIV_ROUND_POW2(dya,2,2);
}

/*The function type used to fill in the U,V plane motion vectors for a macro
   block when 4 different motion vectors are specified in the luma plane.
  This version is for use with chroma decimated in the Y direction.
  _cbmvs: The chroma block-level motion vectors to fill in.
  _lmbmv: The luma macro-block level motion vector to fill in for use in
           prediction.
  _lbmvs: The luma block-level motion vectors.*/
static oc_set_chroma_mvs10(int _cbmvs[4][2],int _lmbmv[2],
 const char _lbmvs[4][2]){
  int dxa;
  int dya;
  int dxb;
  int dyb;
  dxa=_lbmvs[0][0]+_lbmvs[2][0];
  dya=_lbmvs[0][1]+_lbmvs[2][1];
  _cbmvs[0][0]=OC_DIV_ROUND_POW2(dxa,1,1);
  _cbmvs[0][1]=OC_DIV_ROUND_POW2(dya,1,1);
  dxb=_lbmvs[1][0]+_lbmvs[3][0];
  dyb=_lbmvs[1][1]+_lbmvs[3][1];
  _cbmvs[1][0]=OC_DIV_ROUND_POW2(dxb,1,1);
  _cbmvs[1][1]=OC_DIV_ROUND_POW2(dyb,1,1);
  dxa+=dxb;
  dxa+=dxb;
  _lmbmv[0]=OC_DIV_ROUND_POW2(dxa,2,2);
  _lmbmv[1]=OC_DIV_ROUND_POW2(dya,2,2);
}

/*The function type used to fill in the U,V plane motion vectors for a macro
   block when 4 different motion vectors are specified in the luma plane.
  This version is for use with chroma decimated in the X and Y directions.
  _cbmvs: The chroma block-level motion vectors to fill in.
  _lmbmv: The luma macro-block level motion vector to fill in for use in
           prediction.
  _lbmvs: The luma block-level motion vectors.*/
static void oc_set_chroma_mvs11(int _cbmvs[4][2],int _lmbmv[2],
 const char _lbmvs[4][2]){
  int dx;
  int dy;
  dx=_lbmvs[0][0]+_lbmvs[1][0]+_lbmvs[2][0]+_lbmvs[3][0];
  dy=_lbmvs[0][1]+_lbmvs[1][1]+_lbmvs[2][1]+_lbmvs[3][1];
  _lmbmv[0]=_cbmvs[0][0]=OC_DIV_ROUND_POW2(dx,2,2);
  _lmbmv[1]=_cbmvs[0][1]=OC_DIV_ROUND_POW2(dy,2,2);
}

/*A table of functions used to fill in the U,V plane motion vectors for a
   macro block when 4 different motion vectors are specified in the luma
   plane.*/
const oc_set_chroma_mvs_func OC_SET_CHROMA_MVS_TABLE[4]={
  oc_set_chroma_mvs00,
  oc_set_chroma_mvs01,
  oc_set_chroma_mvs10,
  oc_set_chroma_mvs11
};



void **oc_malloc_2d(size_t _height,size_t _width,size_t _sz){
  size_t  colsz;
  size_t  datsz;
  char   *ret;
  colsz=_height*sizeof(void *);
  datsz=_sz*_width*_height;
  /*Alloc array and row pointers.*/
  ret=(char *)_ogg_malloc(datsz+colsz);
  /*Initialize the array.*/
  if(ret!=NULL){
    size_t   rowsz;
    size_t   i;
    void   **p;
    char    *datptr;
    p=(void **)ret;
    i=_height;
    rowsz=_sz*_width;
    for(datptr=ret+colsz;i-->0;p++,datptr+=rowsz)*p=(void *)datptr;
  }
  return (void **)ret;
}

void oc_free_2d(void **_ptr){
  _ogg_free(_ptr);
}

/*Fills in a Y'CbCr buffer with a pointer to the image data in the first
   buffer, but with the opposite vertical orientation.
  _dst: The destination buffer.
        This can be the same as _src.
  _src: The source buffer.*/
void oc_ycbcr_buffer_flip(theora_ycbcr_buffer _dst,
 const theora_ycbcr_buffer _src){
  int pli;
  for(pli=0;pli<3;pli++){
    _dst[pli].width=_src[pli].width;
    _dst[pli].height=_src[pli].height;
    _dst[pli].stride=-_src[pli].stride;
    _dst[pli].data=_src[pli].data+(1-_dst[pli].height)*_dst[pli].stride;
  }
}

const char *theora_version_string(void){
  return OC_VENDOR_STRING;
}

ogg_uint32_t theora_version_number(void){
  return (OC_VERSION_MAJOR<<16)+(OC_VERSION_MINOR<<8)+(OC_VERSION_SUB);
}
