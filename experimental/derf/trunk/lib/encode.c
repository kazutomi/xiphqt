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
  oc_mb             *mbs;
  oc_mb_enc_info    *emb0;
  int                nhmbs;
  int                nvmbs;
  int                y;
  frarray=&_enc->state.frarray;
  mbs=frarray->mbs;
  emb0=_enc->mbinfo;
  nhmbs=frarray->frplanes[0].nhsbs<<1;
  nvmbs=frarray->frplanes[0].nvsbs<<1;
  /*Loop through the Y plane super blocks.*/
  for(y=0;y<nvmbs;y+=2){
    int x;
    for(x=0;x<nhmbs;x+=2,emb0+=4){
      int ymb;
      /*Loop through the macro blocks in each super block in display order.*/
      for(ymb=0;ymb<2;ymb++){
        int xmb;
        for(xmb=0;xmb<2;xmb++){
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
          int             mbx;
          int             mby;
          int             mbn;
          int             i;
          mbn=OC_MB_MAP[ymb][xmb];
          emb=emb0+mbn;
          mbx=x|xmb;
          mby=y|ymb;
          /*Make sure this macro block is within the encoded region.*/
          if(mbx>=nhmbs||mby>=nvmbs)continue;
          /*Fill in the neighbors with current motion vectors available.*/
          for(i=0;i<NCNEIGHBORS[mbn];i++){
            int nx;
            int ny;
            int nmb;
            nx=mbx+CDX[mbn][i];
            ny=mby+CDY[mbn][i];
            if(nx<0||nx>=nhmbs||ny<0||ny>=nvmbs)continue;
            nmb=nx+ny*nhmbs+OC_MB_MAP[ny&1][nx&1];
            if(mbs[nmb].mode<0)continue;
            emb->cneighbors[emb->ncneighbors++]=nmb;
          }
          /*Fill in the neighbors with previous motion vectors available.*/
          for(i=0;i<4;i++){
            int nx;
            int ny;
            int nmb;
            nx=mbx+PDX[i];
            ny=mby+PDY[i];
            if(nx<0||nx>=nhmbs||ny<0||ny>=nvmbs)continue;
            nmb=(nx&~1)+(ny&~1)*nhmbs+OC_MB_MAP[ny&1][nx&1];
            if(mbs[nmb].mode<0)continue;
            emb->pneighbors[emb->npneighbors++]=nmb;
          }
        }
      }
    }
  }
}

static int oc_enc_init(oc_enc_ctx *_enc,theora_info *_info){
  int ret;
  ret=oc_state_init(&_enc->state,_info);
  if(ret<0)return ret;
  _enc->frinfo=_ogg_calloc(_enc->state.frarray.nfrags,
   sizeof(_enc->frinfo[0]));
  _enc->mbinfo=_ogg_calloc(_enc->state.frarray.nmbs,sizeof(_enc->mbinfo[0]));
  oc_enc_init_mbinfo(_enc);
  _enc->mcenc=oc_mcenc_alloc(_enc);
  _enc->impmap=oc_impmap_alloc(_enc);
  return 0;
}

static void oc_enc_clear(oc_enc_ctx *_enc){
  oc_impmap_free(_enc->impmap);
  oc_mcenc_free(_enc->mcenc);
  _ogg_free(_enc->mbinfo);
  _ogg_free(_enc->frinfo);
  oc_state_clear(&_enc->state);
}


/*Performs an initial DCT on the data for each fragment before any attempt at
   motion compensation.
  This data is used to compute the HVS-modeled weights for each coefficient,
   as well as for the macro block mode decision.
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

static int oc_enc_yuv_in(oc_enc_ctx *_enc,oc_yuv_buffer _img){
  int cwidth;
  int cheight;
  /*Step 1: validate parameters.*/
  if(_enc==NULL||_img==NULL)return OC_FAULT;
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
  /*Step 2: Fill the fragment array with pointers into the user buffer.*/
  oc_frarray_fill_buffer_ptrs(&_enc->state.frarray,3,_img);
  /*Step 3: Analyze motion.*/
  oc_mcenc_analyze(_enc->mcenc);
  /*Step 4: Compute importance map.*/
  oc_impmap_fill(_enc->impmap,_img,
   _enc->state.info.fps_denominator/(float)_enc->state.info.fps_numerator);
  /*TODO: other things, like, maybe encoding.*/
  return 0;
}


void *theora_encode_alloc(theora_info *_info){
  oc_enc_ctx *enc;
  enc=_ogg_malloc(sizeof(*enc));
  oc_enc_init(enc,_info);
  return enc;
}

void theora_encode_free(void *_enc){
  if(_enc!=NULL){
    oc_enc_clear((oc_enc_ctx *)_enc);
    _ogg_free(_enc);
  }
}

int theora_encode_yuv_in(void *_enc,oc_yuv_buffer _img){
  return oc_enc_yuv_in((oc_enc_ctx *)_enc,_img);
}
