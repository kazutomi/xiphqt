#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "fdct.h"

int oc_enc_init(oc_enc_ctx *_enc,theora_info *_info){
  int ret;
  ret=oc_state_init(&_enc->state,_info);
  if(ret<0)return ret;
  _enc->frinfo=_ogg_calloc(_enc->state.frarray.nfrags,
   sizeof(_enc->frinfo[0]));
  _enc->mbinfo=_ogg_calloc(_enc->state.frarray.nmbs,sizeof(_enc->mbinfo[0]));
  return 0;
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
