#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "fdct.h"


/*Initializes the macro block neighbor lists.
  This assumes that the entire mbinfo memory region has been initialized with
   zeros.
  _enc: The encoding context.*/
static void oc_enc_init_mbinfo(oc_enc_ctx *_enc){
  oc_fragment_array *frarray;
  int                nmbs;
  int                mbi;
  frarray=&_enc->state.frarray;
  /*Loop through the Y plane super blocks.*/
  nmbs=frarray->nmbs;
  for(mbi=0;mbi<nmbs;mbi++){
    /*Because of the Hilbert curve ordering the macro blocks are
       visited in, the available neighbors change depending on where in
       a super block the macro block is located.
      Only the first three vectors are used in the median calculation
       for the optimal predictor, and so the most important should be
       listed first.
      Additional vectors are used, so there will always be at least 3.*/
    /*The number of current neighbors for each macro block position.*/
    static const int NCNEIGHBORS[4]={4,3,2,4};
    /*The offset of each current neighbor in the X direction.*/
    static const int CDX[4][4]={
      {-1,0,1,-1},
      {-1,0,-1,},
      {-1,-1},
      {-1,0,0,1}
    };
    /*The offset of each current neighbor in the Y direction.*/
    static const int CDY[4][4]={
      {0,-1,-1,-1},
      {0,-1,-1},
      {0,-1},
      {0,-1,1,-1}
    };
    /*The offset of each previous neighbor in the X direction.*/
    static const int PDX[4]={-1,0,1,0};
    /*The offset of each previous neighbor in the Y direction.*/
    static const int PDY[4]={0,-1,0,1};
    oc_mb_enc_info *emb;
    oc_mb          *mb;
    int mbx;
    int mby;
    int mbn;
    int nmbx;
    int nmby;
    int nmbi;
    int i;
    mb=frarray->mbs+mbi;
    /*Make sure this macro block is within the encoded region.*/
    if(mb->mode<0)continue;
    emb=_enc->mbinfo+mbi;
    mbn=mbi&3;
    mbx=mb->x>>4;
    mby=mb->y>>4;
    /*Fill in the neighbors with current motion vectors available.*/
    for(i=0;i<NCNEIGHBORS[mbn];i++){
      nmbx=mbx+CDX[mbn][i];
      nmby=mby+CDY[mbn][i];
      if(nmbx<0||nmbx>=frarray->nhmbs||nmby<0||nmby>=frarray->nvmbs)continue;
      nmbi=oc_frarray_mbi_for_pos(frarray,nmbx,nmby);
      if(frarray->mbs[nmbi].mode<0)continue;
      emb->cneighbors[emb->ncneighbors++]=nmbi;
    }
    /*Fill in the neighbors with previous motion vectors available.*/
    for(i=0;i<4;i++){
      nmbx=mbx+PDX[i];
      nmby=mby+PDY[i];
      if(nmbx<0||nmbx>=frarray->nhmbs||nmby<0||nmby>=frarray->nvmbs)continue;
      nmbi=oc_frarray_mbi_for_pos(frarray,nmbx,nmby);
      if(frarray->mbs[nmbi].mode<0)continue;
      emb->pneighbors[emb->npneighbors++]=nmbi;
    }
  }
}

static int oc_enc_keyframe(oc_enc_ctx *_enc){
  return 0;
}

static int oc_enc_init(oc_enc_ctx *_enc,theora_info *_info){
  int ret;
  ret=oc_state_init(&_enc->state,_info);
  if(ret<0)return ret;
  oggpackB_writeinit(&_enc->opb);
  _enc->frinfo=_ogg_calloc(_enc->state.frarray.nfrags,
   sizeof(_enc->frinfo[0]));
  _enc->mbinfo=_ogg_calloc(_enc->state.frarray.nmbs,sizeof(_enc->mbinfo[0]));
  oc_enc_init_mbinfo(_enc);
  _enc->mcenc=oc_mcenc_alloc(_enc);
  _enc->impmap=oc_impmap_alloc(_enc);
  _enc->psych=oc_psych_alloc(_enc);
  _enc->packet_state=OC_PACKET_INFO_HDR;
  memcpy(_enc->huff_codes,OC_VP31_HUFF_CODES,sizeof(_enc->huff_codes));
  memcpy(&_enc->qinfo,&OC_VP31_QUANT_INFO,sizeof(_enc->qinfo));
  return 0;
}

static void oc_enc_clear(oc_enc_ctx *_enc){
  oc_impmap_free(_enc->impmap);
  oc_mcenc_free(_enc->mcenc);
  _ogg_free(_enc->mbinfo);
  _ogg_free(_enc->frinfo);
  oc_state_clear(&_enc->state);
}


#if 0
/*Performs an initial DCT on the data for each fragment before any attempt at
   motion compensation.
  This data is used to compute the HVS-modeled weights for each coefficient,
   as well as for the macro block mode decision.

  For now, this function is disabled, and DCT's are performed in psych.c where
   they are first used for better cache coherency.
  This may be changed to make separation clearner when the HVS stuff gets
   moved to plug-ins.

  _enc: The encoder context.
  _img: The input image.
        The fragment array must have already been filled with input pointers
         to this image.*/
static void oc_enc_init_dcts(oc_enc_ctx *_enc,oc_yuv_buffer _img){
  int pli;
  for(pli=0;pli<3;pli++){
    oc_img_plane      *iplane;
    oc_fragment_plane *frplane;
    int                frag_end;
    int                fragi;
    iplane=&_img[pli];
    frplane=_enc->state.frarray.frplanes+pli;
    frag_end=frplane->froffset+frplane->nfrags;
    for(fragi=frplane->froffset;fragi<frag_end;fragi++){
      oc_fragment          *frag;
      oc_fragment_enc_info *frinfo;
      ogg_int16_t           pix_buf[64];
      unsigned char        *pixels;
      int                   pixi;
      int                   y;
      frag=_enc->state.frarray.frags+fragi;
      /*Skip blocks that are not displayable.*/
      if(frag->invalid)continue;
      pixels=frag->buffer[3];
      frinfo=_enc->frinfo+fragi;
      /*For border fragments, only copy pixels that are in the displayable
         region of the image.
        The DCT function will compute optimal padding values for the other
         pixels.*/
      if(frag->border){
        for(pixi=y=0;y<8;y++){
          int x;
          for(x=0;x<8;x++,pixi++)if(frag->border->mask&1<<pixi){
            pix_buf[pixi]=pixels[x];
          }
          pixels+=iplane->stride;
        }
        oc_fdct8x8_border(frag->border,frinfo->dct_coeffs[0],pix_buf);
      }
      /*Otherwise, copy all the pixels in the fragment and do a normal DCT.*/
      else{
        pixels=frag->buffer[3];
        for(pixi=y=0;y<8;y++){
          int x;
          for(x=0;x<8;x++,pixi++)pix_buf[pixi]=pixels[x];
          pixels+=iplane->stride;
        }
        oc_fdct8x8(frinfo->dct_coeffs[0],pix_buf);
      }
    }
  }
}
#endif

/*Performs an fDCT on a given fragment.
  _frag:     The fragment to perform the 2D DCT on.
  _dct_vals: The output buffer for the DCT coefficients.
  _stride:   The stride of the plane the fragment belongs to.*/
void oc_frag_fdct(const oc_fragment *_frag,ogg_int16_t _dct_vals[64],
 int _stride){
  ogg_int16_t    pix_buf[64];
  unsigned char *pixels;
  int            pixi;
  int            y;
  int            x;
  pixels=_frag->buffer[3];
  /*For border fragments, only copy pixels that are in the displayable
     region of the image.
    The DCT function will compute optimal padding values for the other
     pixels.*/
  if(_frag->border!=NULL){
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++){
        pix_buf[pixi]=(ogg_int16_t)(
         (_frag->border->mask&(ogg_int64_t)1<<pixi)?pixels[x]:0);
      }
      pixels+=_stride;
    }
    oc_fdct8x8_border(_frag->border,_dct_vals,pix_buf);
  }
  /*Otherwise, copy all the pixels in the fragment and do a normal DCT.*/
  else{
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++)pix_buf[pixi]=pixels[x];
      pixels+=_stride;
    }
    oc_fdct8x8(_dct_vals,pix_buf);
  }
}



theora_enc_ctx *theora_encode_alloc(theora_info *_info){
  oc_enc_ctx *enc;
  if(_info==NULL)return NULL;
  enc=_ogg_malloc(sizeof(*enc));
  oc_enc_init(enc,_info);
  return enc;
}

void theora_encode_free(theora_enc_ctx *_enc){
  if(_enc!=NULL){
    oc_enc_clear(_enc);
    _ogg_free(_enc);
  }
}


int theora_encode_set_huffman_codes(theora_enc_ctx *_enc,
 const theora_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]){
  int ret;
  if(_enc==NULL)return OC_FAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return OC_EINVAL;
  if(_codes==NULL)_codes=OC_VP31_HUFF_CODES;
  /*Validate the codes.*/
  oggpackB_reset(&_enc->opb);
  ret=oc_huff_codes_pack(&_enc->opb,_codes);
  if(ret<0)return ret;
  memcpy(_enc->huff_codes,_codes,sizeof(_enc->huff_codes));
  return 0;
}

int theora_encode_set_quant_params(theora_enc_ctx *_enc,
 const theora_quant_info *_qinfo){
  int i;
  if(_enc==NULL)return OC_FAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return OC_EINVAL;
  if(_qinfo==NULL)_qinfo=&OC_VP31_QUANT_INFO;
  for(i=64;i-->1;){
    if(_qinfo->ac_scale[i]>_qinfo->ac_scale[i-1])return OC_EINVAL;
    if(_qinfo->dc_scale[i]>_qinfo->dc_scale[i-1])return OC_EINVAL;
  }
  memcpy(&_enc->qinfo,_qinfo,sizeof(_enc->qinfo));
  return 0;
}


int theora_encode_yuv_in(theora_enc_ctx *_enc,oc_yuv_buffer _img){
  int cwidth;
  int cheight;
  int ret;
  /*Step 1: validate parameters.*/
  if(_enc==NULL||_img==NULL)return OC_FAULT;
  if(_enc->packet_state==OC_PACKET_DONE)return OC_EINVAL;
  if(_img[0].width!=(int)_enc->state.info.width||
   _img[0].height!=(int)_enc->state.info.height){
    return OC_EINVAL;
  }
  cwidth=_enc->state.info.width;
  if(_enc->state.info.chroma_x_decimated)cwidth>>=1;
  cheight=_enc->state.info.height;
  if(_enc->state.info.chroma_y_decimated)cheight>>=1;
  if(_img[1].width!=cwidth||_img[2].width!=cwidth||
   _img[1].height!=cheight||_img[2].height!=cheight){
    return OC_EINVAL;
  }
  /*Step 2: Update state.*/
  _enc->state.curframe_num++;
  /*TODO: Move buffers around, etc.*/
  /*Step 3: Fill the fragment array with pointers into the user buffer.*/
  oc_frarray_fill_buffer_ptrs(&_enc->state.frarray,3,_img);
  /*Step 4: Analyze motion.*/
  oc_mcenc_analyze(_enc->mcenc);
  /*Step 5: Compute importance map.*/
  oc_impmap_fill(_enc->impmap,
   _enc->state.info.fps_denominator/(float)_enc->state.info.fps_numerator);
  /*Step 6: Compute coefficient weights.*/
  oc_psych_scan(_enc->psych,0);
  /*TODO: other things, like, maybe encoding.*/
  if(_enc->state.curframe_num==1||
   _enc->state.curframe_num-_enc->state.keyframe_num>=
   _enc->state.info.keyframe_frequency_force){
    ret=oc_enc_keyframe(_enc);
    if(ret<0)return ret;
  }
  _enc->state.granpos=
   (_enc->state.keyframe_num<<_enc->state.keyframe_granule_shift)+
   (_enc->state.curframe_num-_enc->state.keyframe_num);
  return 0;
}

int theora_encode_packetout(theora_enc_ctx *_enc,int _last,ogg_packet *_op){
  if(_enc==NULL||_op==NULL)return OC_FAULT;
  if(_enc->packet_state!=OC_PACKET_READY)return 0;
  _op->packet=oggpackB_get_buffer(&_enc->opb);
  _op->bytes=oggpackB_bytes(&_enc->opb);
  _op->b_o_s=0;
  _op->e_o_s=_last;
  _op->packetno=_enc->state.curframe_num;
  /*_op->packetno=cpi->CurrentFrame;*/
  /*_op->granulepos=t->granulepos;*/
  if(_last)_enc->packet_state=OC_PACKET_DONE;
  else _enc->packet_state=OC_PACKET_EMPTY;
  return 1;
}
