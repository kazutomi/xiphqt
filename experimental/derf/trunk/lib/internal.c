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



int oc_ilog(unsigned _v){
  int ret;
  for(ret=0;_v;ret++)_v>>=1;
  return ret;
}

/*Initializes the buffers used for reconstructed frames.
  These buffers are padded with 16 extra pixels on each side, to allow
   unrestricted motion vectors without special casing the boundary.
  If chroma is decimated in either direction, the padding is reduced by a
   factor of 2 on the appropriate sides.
  _enc: The encoding context to store the buffers in.*/
static void oc_state_ref_bufs_init(oc_theora_state *_state){
  theora_info   *info;
  unsigned char *ref_frame_data;
  size_t         yplane_sz;
  size_t         cplane_sz;
  int            yhstride;
  int            yvstride;
  int            chstride;
  int            cvstride;
  int            yoffset;
  int            coffset;
  int            rfi;
  info=&_state->info;
  /*Compute the image buffer parameters for each plane.*/
  yhstride=info->width+2*OC_UMV_PADDING;
  yvstride=info->height+2*OC_UMV_PADDING;
  chstride=info->chroma_x_decimated?yhstride>>1:yhstride;
  cvstride=info->chroma_y_decimated?yvstride>>1:yvstride;
  yplane_sz=(size_t)yhstride*yvstride;
  cplane_sz=(size_t)chstride*cvstride;
  yoffset=OC_UMV_PADDING+OC_UMV_PADDING*yhstride;
  coffset=(info->chroma_x_decimated?OC_UMV_PADDING>>1:OC_UMV_PADDING)+
   (info->chroma_y_decimated?OC_UMV_PADDING>>1:OC_UMV_PADDING)*chstride;
  _state->ref_frame_data=ref_frame_data=_ogg_malloc(3*(yplane_sz+2*cplane_sz));
  /*Set up the width, height and stride for the YUV buffers.*/
  _state->ref_frame_bufs[0][0].width=info->width;
  _state->ref_frame_bufs[0][0].height=info->height;
  _state->ref_frame_bufs[0][0].stride=yhstride;
  _state->ref_frame_bufs[0][1].width=_state->ref_frame_bufs[0][2].width=
   info->chroma_x_decimated?info->width>>1:info->width;
  _state->ref_frame_bufs[0][1].height=_state->ref_frame_bufs[0][2].height=
   info->chroma_x_decimated?info->height>>1:info->height;
  _state->ref_frame_bufs[0][1].stride=_state->ref_frame_bufs[0][2].stride=
   chstride;
  memcpy(_state->ref_frame_bufs[1],_state->ref_frame_bufs[0],
   sizeof(_state->ref_frame_bufs[0]));
  memcpy(_state->ref_frame_bufs[2],_state->ref_frame_bufs[0],
   sizeof(_state->ref_frame_bufs[0]));
  /*Set up the data pointers for the YUV buffers.*/
  for(rfi=0;rfi<3;rfi++){
    _state->ref_frame_bufs[rfi][0].data=ref_frame_data+yoffset;
    ref_frame_data+=yplane_sz;
    _state->ref_frame_bufs[rfi][1].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    _state->ref_frame_bufs[rfi][2].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    /*Flip the buffer upside down.*/
    oc_yuv_buffer_flip(_state->ref_frame_bufs[rfi],
     _state->ref_frame_bufs[rfi]);
    /*Initialize the fragment pointers into this buffer.*/
    oc_frarray_fill_buffer_ptrs(&_state->frarray,rfi,
     _state->ref_frame_bufs[rfi]);
  }
  /*Initialize the reference frame indexes.*/
  _state->ref_frame_idx[OC_FRAME_GOLD]=
   _state->ref_frame_idx[OC_FRAME_PREV]=
   _state->ref_frame_idx[OC_FRAME_SELF]=-1;
}

static void oc_state_ref_bufs_clear(oc_theora_state *_state){
  _ogg_free(_state->ref_frame_data);
}

int oc_state_init(oc_theora_state *_state,theora_info *_info){
  /*First validate the parameters.*/
  if(_info==NULL)return OC_FAULT;
  /*The width and height of the encoded frame must be multiples of 16.
    They must also, when divided by 16, fit into a 16-bit unsigned integer.
    The displayable frame offset coordinates must fit into an 8-bit unsigned
     integer (only because we only write 8 bits to the bitstream: why?).
    The displayable frame must fit inside the encoded frame.
    The color space must be one known by the encoder.*/
  if((_info->width&0xF)||(_info->height&0xF)||
   _info->width>=0x100000||_info->height>=0x100000||
   _info->offset_x>255||_info->offset_y>255||
   _info->offset_x+_info->frame_width>_info->width||
   _info->offset_y+_info->frame_height>_info->height||
   _info->colorspace<0||_info->colorspace>=OC_CS_NSPACES){
    return OC_EINVAL;
  }
  memset(_state,0,sizeof(_state));
  memcpy(&_state->info,_info,sizeof(*_info));
  _state->frame_type=OC_UNKWN_FRAME;
  oc_frarray_init(&_state->frarray,_info);
  oc_state_ref_bufs_init(_state);
  if(_info->keyframe_frequency_force==0){
    _info->keyframe_frequency_force=0xFFFFFFFF;
  }
#if defined(OC_COMPOSITE_GRANPOS)
  _state->keyframe_granule_shift=oc_ilog(_info->keyframe_frequency_force-1);
#else
  _state->keyframe_granule_shift=0;
#endif
  _state->keyframe_num=0;
  _state->curframe_num=-1;
  return 0;
}

void oc_state_clear(oc_theora_state *_state){
  oc_state_ref_bufs_clear(_state);
  oc_frarray_clear(&_state->frarray);
}

/*Duplicates the pixels on the border of the image plane out into the
   surrounding padding for use by unrestricted motion vectors.
  _iplane:   The image plane to fill the borders of.
  _hpadding: The amount of padding on each side in the horizontal direction.
  _vpadding: The amount of padding on each side in the vertical direction.*/
static void oc_iplane_borders_fill(oc_img_plane *_iplane,
 int _hpadding,int _vpadding){
  unsigned char *apix;
  unsigned char *bpix;
  unsigned char *epix;
  int            fullw;
  /*First extend the vertical edges.*/
  apix=_iplane->data;
  bpix=_iplane->data+_iplane->width-1;
  epix=_iplane->data+_iplane->height*_iplane->stride;
  /*Note the use of != instead of <, which allows stride to be negative.*/
  while(apix!=epix){
    memset(apix-_hpadding,apix[0],_hpadding);
    memset(bpix+1,bpix[0],_hpadding);
    apix+=_iplane->stride;
    bpix+=_iplane->stride;
  }
  /*Next extend the horizontal edges.*/
  fullw=_iplane->width+(_hpadding<<1);
  apix=_iplane->data-_hpadding;
  bpix=_iplane->data+(_iplane->height-1)*_iplane->stride;
  epix=apix-_iplane->stride*_vpadding;
  while(apix!=epix){
    memcpy(apix-_iplane->stride,apix,fullw);
    memcpy(bpix+_iplane->stride,bpix,fullw);
    apix-=_iplane->stride;
    bpix+=_iplane->stride;
  }
}

/*Duplicates the pixels on the border of the given reference image out into
   the surrounding padding for use by unrestricted motion vectors.
  _state: The context containing the reference buffers.
  _refi:  The index of the reference buffer to pad.*/
void oc_state_borders_fill(oc_theora_state *_state,int _refi){
  static const int PL_PADDING[3][2]={
    {OC_UMV_PADDING,OC_UMV_PADDING},
    {OC_UMV_PADDING,OC_UMV_PADDING/2},
    {OC_UMV_PADDING,OC_UMV_PADDING/2}
  };
  int pli;
  for(pli=0;pli<3;pli++){
    oc_iplane_borders_fill(&_state->ref_frame_bufs[_refi][pli],
     PL_PADDING[pli][_state->info.chroma_x_decimated],
     PL_PADDING[pli][_state->info.chroma_y_decimated]);
  }
}

void oc_frag_recon(oc_theora_state *_state,const oc_fragment *_frag,int _pli,
 ogg_int16_t _dct_coeffs[64],int _ncoefs,const ogg_uint16_t _iquant[64]){
  ogg_int16_t dct_buf[64];
  ogg_int16_t res_buf[64];
  int dst_framei;
  int dst_ystride;
  int zzi;
  int ci;
  /*Special case only having a DC component.*/
  if(_ncoefs<2){
    ogg_int16_t p;
    /*Why is the iquant product rounded in this case and no others?
      Who knows.*/
    p=(ogg_int16_t)((ogg_int32_t)_frag->dc*_iquant[0]+15>>5);
    for(ci=0;ci<64;ci++)res_buf[ci]=p;
  }
  else{
    /*First, dequantize the coefficients.*/
    dct_buf[0]=(ogg_int16_t)((ogg_int32_t)_frag->dc*_iquant[0]);
    for(zzi=1;zzi<_ncoefs;zzi++){
      int ci;
      ci=OC_FZIG_ZAG[zzi];
      dct_buf[ci]=(ogg_int16_t)((ogg_int32_t)_dct_coeffs[ci]*_iquant[ci]);
    }
    /*Then, fill in the remainder of the coefficients with 0's, and perform
       the iDCT.*/
    if(_ncoefs<10){
      for(;zzi<10;zzi++)dct_buf[OC_FZIG_ZAG[zzi]]=0;
      oc_idct8x8_10(res_buf,dct_buf);
    }
    else{
      for(;zzi<64;zzi++)dct_buf[OC_FZIG_ZAG[zzi]]=0;
      oc_idct8x8(res_buf,dct_buf);
    }
  }
  /*Fill in the target buffer.*/
  dst_framei=_state->ref_frame_idx[OC_FRAME_SELF];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].stride;
  /*For now stride values in all ref frames assumed to be equal.*/
  if(_frag->mbmode==OC_MODE_INTRA){
    oc_frag_recon_intra(_frag->buffer[dst_framei],dst_ystride,res_buf);
  }
  else{
    int src_framei;
    int src_ystride;
    int src_offset1;
    src_framei=_state->ref_frame_idx[OC_FRAME_FOR_MODE[_frag->mbmode]];
    src_ystride=_state->ref_frame_bufs[src_framei][_pli].stride;
    if((_frag->mv[0]&1)||(_frag->mv[1]&1)){
      int src_offset2;
      int dy;
      src_offset1=OC_DIV2(_frag->mv[0]);
      src_offset2=_frag->mv[0]-src_offset1;
      dy=OC_DIV2(_frag->mv[1]);
      src_offset1+=dy*src_ystride;
      dy=_frag->mv[1]-dy;
      src_offset2+=dy*src_ystride;
      oc_frag_recon_inter2(_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[src_framei]+src_offset1,src_ystride,
       _frag->buffer[src_framei]+src_offset2,src_ystride,res_buf);
    }
    else{
      src_offset1=OC_DIV2(_frag->mv[0])+OC_DIV2(_frag->mv[1])*src_ystride;
      oc_frag_recon_inter(_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[src_framei]+src_offset1,src_ystride,res_buf);
    }
  }
}



const int OC_DCT_TOKEN_EXTRA_BITS[OC_NDCT_TOKENS]={
  0,0,0,2,3,4,12,3,6,
  0,0,0,0,
  1,1,1,1,2,3,4,5,6,10,
  1,1,1,1,1,3,4,
  2,3
};


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

/*Fills in a YUV buffer with a pointer to the image data in the first buffer,
   but with the opposite vertical orientation.
  _dst: The destination buffer.
        This can be the same as _src.
  _src: The source buffer.*/
void oc_yuv_buffer_flip(oc_yuv_buffer _dst,const oc_yuv_buffer _src){
  int pli;
  for(pli=0;pli<3;pli++){
    _dst[pli].width=_src[pli].width;
    _dst[pli].height=_src[pli].height;
    _dst[pli].data=_src[pli].data+(_src[pli].height-1)*_src[pli].stride;
    _dst[pli].stride=-_src[pli].stride;
  }
}

const char *theora_version_string(void){
  return OC_VENDOR_STRING;
}

ogg_uint32_t theora_version_number(void){
  return (OC_VERSION_MAJOR<<16)+(OC_VERSION_MINOR<<8)+(OC_VERSION_SUB);
}

