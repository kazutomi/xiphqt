/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************
*/
#include "x86int.h"

/*Copies the fragments specified by the lists of fragment indices from one
   frame to another.
  _fragis:    A pointer to a list of fragment indices.
  _nfragis:   The number of fragment indices to copy.
  _dst_frame: The reference frame to copy to.
  _src_frame: The reference frame to copy from.
  _pli:       The color plane the fragments lie in.*/
void oc_state_frag_copy_mmx(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli){
  const int *fragi;
  const int *fragi_end;
  int        dst_framei;
  int        dst_ystride;
  int        src_framei;
  int        src_ystride;
  dst_framei=_state->ref_frame_idx[_dst_frame];
  src_framei=_state->ref_frame_idx[_src_frame];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].ystride;
  src_ystride=_state->ref_frame_bufs[src_framei][_pli].ystride;
  fragi_end=_fragis+_nfragis;
  for(fragi=_fragis;fragi<fragi_end;fragi++){
    oc_fragment   *frag;
    unsigned char *dst;
    unsigned char *src;
    frag=_state->frags+*fragi;
    dst=frag->buffer[dst_framei];
    src=frag->buffer[src_framei];
    __asm__ __volatile__(
     "  lea         (%3, %3, 2), %%esi   \n\t"  /* esi=src_stride*3 */
     "  movq        (%1),        %%mm0   \n\t"  /* src */
     "  lea         (%2, %2, 2), %%edi   \n\t"  /* edi=dst_stride*3 */
     "  movq        (%1, %3),    %%mm1   \n\t"  /* src+1x stride */
     "  movq        (%1, %3, 2), %%mm2   \n\t"  /* src+2x stride */
     "  movq        (%1, %%esi), %%mm3   \n\t"  /* src+3x stride */
     "  movq        %%mm0,       (%0)    \n\t"  /* dst */
     "  movq        %%mm1,       (%0, %2)\n\t"  /* dst+dst_stride */
     "  lea         (%1,%3,4),   %1      \n\t"  /* pointer to next 4 */
     "  movq        %%mm2,       (%0, %2, 2)      \n\t"  /*dst+2x dst_stride */
     "  movq        %%mm3,       (%0, %%edi)      \n\t"  /* 3x */
     "  lea         (%0,%2,4),   %0      \n\t"  /* pointer to next 4 */
     "  movq        (%1),        %%mm0   \n\t"  /* src */
     "  movq        (%1, %3),    %%mm1   \n\t"  /* src+1x stride */
     "  movq        (%1, %3, 2), %%mm2   \n\t"  /* src+2x stride */
     "  movq        (%1, %%esi), %%mm3   \n\t"  /* src+3x stride */
     "  movq        %%mm0,       (%0)    \n\t"  /* dst */
     "  movq        %%mm1,       (%0, %2)\n\t"  /* dst+dst_stride */
     "  movq        %%mm2,       (%0, %2, 2)     \n\t"  /* dst+2x dst_stride */
     "  movq        %%mm3,       (%0, %%edi)     \n\t"  /* 3x */
     :"+r" (dst) /* 0 */
     :"r" (src),  /* 1 */
      "r" (dst_ystride), /* 2 */
      "r" (src_ystride) /* 3 */
     :"memory", "esi","edi"
    );
  }
  /*This needs to be removed when decode specific functions are implemented:*/
  __asm__ __volatile__("emms\n\t");
}
