#include <stdlib.h>
#include <string.h>
#include "internal.h"

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



/*Initializes the buffers used for reconstructed frames.
  These buffers are padded with 16 extra pixels on each side, to allow
   unrestricted motion vectors without special casing the boundary.
  If chroma is decimated in either direction, the padding is reduced by a
   factor of 2 on the appropriate sides.
  _enc: The encoding context to store the buffers in.*/
static void oc_state_init_ref_bufs(oc_theora_state *_state){
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
  int            i;
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
  for(i=0;i<3;i++){
    _state->ref_frame_bufs[i][0].data=ref_frame_data+yoffset;
    ref_frame_data+=yplane_sz;
    _state->ref_frame_bufs[i][1].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    _state->ref_frame_bufs[i][2].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
  }
}

int oc_state_init(oc_theora_state *_state,theora_info *_info){
  /*First validate the parameters.*/
  if(_info==NULL)return OC_FAULT;
  if((_info->width&0xF)||(_info->height&0xF)||
   _info->offset_x+_info->frame_width>_info->width||
   _info->offset_y+_info->frame_height>_info->height){
    return OC_EINVAL;
  }
  memset(_state,0,sizeof(_state));
  memcpy(&_state->info,_info,sizeof(*_info));
  oc_frarray_init(&_state->frarray,_info);
  oc_state_init_ref_bufs(_state);
  return 0;
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
  while(apix<epix){
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
  while(apix>epix){
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

const int OC_DCT_TOKEN_EXTRA_BITS[OC_NDCT_TOKENS]={
  0,0,0,2,3,4,12,3,6,
  0,0,0,0,
  1,1,1,1,2,3,4,5,6,10,
  1,1,1,1,1,3,4,
  2,3
};

void **oc_alloc_2d(size_t _height,size_t _width,size_t _sz){
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
