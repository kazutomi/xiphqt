#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "encint.h"

struct oc_mcenc_ctx{
  oc_enc_ctx  *enc;
  ogg_int32_t  hit_cache[32];
  int          candidates[12][2];
  int          setb0;
  int          ncandidates;
  int          last[2][2];
};

/*The maximum Y plane SAD value for accepting the median predictor.*/
#define OC_YSAD_THRESH1            (256)
/*The amount to right shift the minimum error by when inflating it for
   computing the second maximum Y plane SAD threshold.*/
#define OC_YSAD_THRESH2_SCALE_BITS  (3)
/*The amount to add to the second maximum Y plane threshold when inflating
   it.*/
#define OC_YSAD_THRESH2_OFFSET      (128)

/*The vector offsets in the X direction for each search site in the square
   pattern.*/
static const int OC_SQUARE_DX[9]={-1,0,1,-1,0,1,-1,0,1};
/*The vector offsets in the Y direction for each search site in the square
   pattern.*/
static const int OC_SQUARE_DY[9]={-1,-1,-1,0,0,0,1,1,1};
/*The number of sites to search for each boundary condition in the square
   pattern.
  Bit flags for the boundary conditions are as follows:
  1: -16==dx
  2:      dx==15(.5)
  4: -16==dy
  8:      dy==15(.5)*/
static const int OC_SQUARE_NSITES[11]={8,5,5,0,5,3,3,0,5,3,3};
/*The list of sites to search for each boundary condition in the square
   pattern.*/
static const int OC_SQUARE_SITES[11][8]={
  /* -16<dx<31,       -16<dy<15(.5)*/
  {0,1,2,3,5,6,7,8},
  /*-16==dx,          -16<dy<15(.5)*/
  {1,2,3,7,8},
  /*     dx==15(.5),  -16<dy<15(.5)*/
  {0,1,3,6,7},
  /*-16==dx==15(.5),  -16<dy<15(.5)*/
  {-1},
  /* -16<dx<15(.5),  -16==dy*/
  {3,5,6,7,8},
  /*-16==dx,         -16==dy*/
  {5,7,8},
  /*     dx==15(.5), -16==dy*/
  {3,6,7},
  /*-16==dx==15(.5), -16==dy*/
  {-1},
  /* -16<dx<15(.5),       dy==15(.5)*/
  {0,1,2,3,5},
  /*-16==dx,              dy==15(.5)*/
  {1,2,5},
  /*     dx==15(.5),      dy==15(.5)*/
  {0,1,3}
};



static void oc_mcenc_find_candidates(oc_mcenc_ctx *_mcenc,int _mbi,
 int _which_frame){
  oc_mb_enc_info *nemb;
  oc_mb_enc_info *emb;
  int             a[3][2];
  int             ncandidates;
  int             i;
  emb=_mcenc->enc->mbinfo+_mbi;
  /*Fill in the first part of set A: the last motion vectors used and the
     vectors from adjacent blocks.*/
  /*Skip a position to store the median predictor in.*/
  ncandidates=1;
  for(i=0;i<emb->ncneighbors;i++){
    nemb=_mcenc->enc->mbinfo+emb->cneighbors[i];
    _mcenc->candidates[ncandidates][0]=nemb->mvs[_which_frame][0][0];
    _mcenc->candidates[ncandidates][1]=nemb->mvs[_which_frame][0][1];
    ncandidates++;
  }
  /*Add a few additional vectors to set A: the vector used in the previous
     frame and the (0,0) vector.*/
  _mcenc->candidates[ncandidates][0]=emb->mvs[_which_frame][1][0];
  _mcenc->candidates[ncandidates][1]=emb->mvs[_which_frame][1][1];
  ncandidates++;
  _mcenc->candidates[ncandidates][0]=0;
  _mcenc->candidates[ncandidates][1]=0;
  ncandidates++;
  /*Use the first three vectors of set A to find our best predictor: their
     median.*/
  memcpy(a,_mcenc->candidates+1,sizeof(a[0])*3);
  OC_SORT2I(a[0][0],a[1][0]);
  OC_SORT2I(a[0][1],a[1][1]);
  OC_SORT2I(a[1][0],a[2][0]);
  OC_SORT2I(a[1][1],a[2][1]);
  OC_SORT2I(a[0][0],a[1][0]);
  OC_SORT2I(a[0][1],a[1][1]);
  _mcenc->candidates[0][0]=a[1][0];
  _mcenc->candidates[0][1]=a[1][1];
  /*Fill in set B: accelerated predictors for this and adjacent macro
     blocks.
    TODO: These are wrong for golden frame vectors, but "correct" vectors may
     be ridiculously large.
    Is it even worth using a set B for golden frames?*/
  _mcenc->setb0=ncandidates;
  /*The first time through the loop use the current macro block.*/
  nemb=emb;
  for(i=0;;i++){
    _mcenc->candidates[ncandidates][0]=
     (nemb->mvs[_which_frame][1][0]<<1)-nemb->mvs[_which_frame][2][0];
    _mcenc->candidates[ncandidates][1]=
     (nemb->mvs[_which_frame][1][1]<<1)-nemb->mvs[_which_frame][2][1];
    _mcenc->candidates[ncandidates][0]=OC_CLAMPI(-32,
     _mcenc->candidates[ncandidates][0],31);
    _mcenc->candidates[ncandidates][1]=OC_CLAMPI(-32,
     _mcenc->candidates[ncandidates][1],31);
    ncandidates++;
    if(i>=emb->npneighbors)break;
    nemb=_mcenc->enc->mbinfo+emb->pneighbors[i];
  }
  /*Truncate to full-pel positions.*/
  for(i=0;i<ncandidates;i++){
    _mcenc->candidates[i][0]=OC_DIV2(_mcenc->candidates[i][0]);
    _mcenc->candidates[i][1]=OC_DIV2(_mcenc->candidates[i][1]);
  }
  _mcenc->ncandidates=ncandidates;
}

static int oc_sad8_fullpel(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref,int _ref_stride){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++)err+=abs((int)_cur[j]-_ref[j]);
    _cur+=_cur_stride;
    _ref+=_ref_stride;
  }
  return err;
}

static int oc_sad8_fullpel_border(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref,int _ref_stride,ogg_int64_t _mask){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++){
      int pmask;
      /*This branchless code is equivalent to:
      if(_mask&1)err+=abs((int)_cur[j]-_ref[j]);*/
      pmask=-(int)_mask&1;
      err+=pmask&abs((int)_cur[j]-_ref[j]);
      _mask>>=1;
    }
    _cur+=_cur_stride;
    _ref+=_ref_stride;
  }
  return err;
}

static int oc_sad8_halfpel(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_stride){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++)err+=abs(_cur[j]-((int)_ref0[j]+_ref1[j]>>1));
    _cur+=_cur_stride;
    _ref0+=_ref_stride;
    _ref1+=_ref_stride;
  }
  return err;
}

static int oc_sad8_halfpel_border(const unsigned char *_cur,int _cur_stride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_stride,
 ogg_int64_t _mask){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++){
      int pmask;
      /*This branchless code is equivalent to:
      if(_mask&1)err+=abs(_cur[j]-((int)_ref0[j]+_ref1[j])>>1));*/
      pmask=-(int)_mask&1;
      err+=pmask&abs(_cur[j]-((int)_ref0[j]+_ref1[j]>>1));
      _mask>>=1;
    }
    _cur+=_cur_stride;
    _ref0+=_ref_stride;
    _ref1+=_ref_stride;
  }
  return err;
}

static int oc_sad16_halfpel(const oc_fragment *_frags,const int _fragis[4],
 int _cur_stride,int _ref_stride,int _mvoffset0,int _mvoffset1,int  _ref_idx){
  int err;
  int i;
  err=0;
  for(i=0;i<4;i++){
    const oc_fragment *frag;
    frag=_frags+_fragis[i];
    if(frag->border==NULL){
      if(!frag->invalid){
        err+=oc_sad8_halfpel(frag->buffer[3],_cur_stride,
         frag->buffer[_ref_idx]+_mvoffset0,frag->buffer[_ref_idx]+_mvoffset1,
         _ref_stride);
      }
    }
    else{
      err+=oc_sad8_halfpel_border(frag->buffer[3],_cur_stride,
       frag->buffer[_ref_idx]+_mvoffset0,frag->buffer[_ref_idx]+_mvoffset1,
       _ref_stride,frag->border->mask);
    }
  }
  return err;
}

#if 0
  int                      xfrac;
  int                      yfrac;
  xfrac=_dx&1;
  yfrac=_dy&1;
  mvoffset0=(_dx>>1)+(_dy>>1)*ref_iplane->stride;
  if(xfrac||yfrac){
    int o[2];
    int xsign;
    int ysign;
    /*This branchless code is equivalent to:
    if(xfrac){
      if(_dx<0)mvoffset1=mvoffset0-1;
      else mvoffset1=mvoffset0+1;
    }
    else mvoffset1=mvoffset0;*/
    xsign=_dx>>6;
    o[0]=mvoffset0;
    o[1]=mvoffset0+(xsign|1);
    mvoffset1=o[xfrac];
    /*This branchless code is equivalent to:
    if(yfrac){
      if(_dy<0)mvoffset1-=ref_iplane->stride;
      else mvoffset1+=ref_iplane->stride;
    }*/
    ysign=_dy>>6;
    o[0]=mvoffset1;
    o[1]=mvoffset1+(ref_iplane->stride&-1-ysign)-(ref_iplane->stride&ysign);
    mvoffset1=o[yfrac];
    err=oc_sad16_2(frags,mb->map[_pli],cur_iplane->stride,ref_iplane->stride,
     mvoffset0,mvoffset1,_ref_idx);
  }
#endif

static int oc_mcenc_sad_check_mbcandidate_fullpel(oc_mcenc_ctx *_mcenc,
 int _mbi,int _dx,int _dy,int _pli,int _ref_idx,int _block_err[4]){
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  ogg_int32_t              hitbit;
  int                     *mb_map;
  int                      cur_stride;
  int                      ref_stride;
  int                      mvoffset;
  int                      err;
  int                      bi;
  hitbit=(ogg_int32_t)1<<_dx+16;
  /*If we already examined this vector, then we would be using it already if
     it was better than what we are using.*/
  if(_mcenc->hit_cache[_dy+16]&hitbit)return INT_MAX;
  _mcenc->hit_cache[_dy+16]|=hitbit;
  /*TODO: customize error function for speed/quality tradeoff.*/
  fplane=_mcenc->enc->state.frarray.frplanes+_pli;
  cur_stride=_mcenc->enc->state.frarray.input[_pli].stride;
  ref_stride=_mcenc->enc->state.ref_frame_bufs[_ref_idx][_pli].stride;
  frags=_mcenc->enc->state.frarray.frags;
  mb_map=_mcenc->enc->state.frarray.mbs[_mbi].map[_pli];
  mvoffset=_dx+_dy*ref_stride;
  err=0;
  for(bi=0;bi<4;bi++)if(mb_map[bi]>=0){
    const oc_fragment *frag;
    frag=frags+mb_map[bi];
    if(frag->border==NULL){
      if(!frag->invalid){
        _block_err[bi]=oc_sad8_fullpel(frag->buffer[3],cur_stride,
         frag->buffer[_ref_idx]+mvoffset,ref_stride);
      }
    }
    else{
      _block_err[bi]=oc_sad8_fullpel_border(frag->buffer[3],cur_stride,
       frag->buffer[_ref_idx]+mvoffset,ref_stride,frag->border->mask);
    }
    err+=_block_err[bi];

  }
  return err;
}

static int oc_mcenc_ysad_check_mbcandidate_fullpel(oc_mcenc_ctx *_mcenc,
 int _mbi,int _dx,int _dy,int _ref_idx,int _block_err[4]){
  return oc_mcenc_sad_check_mbcandidate_fullpel(_mcenc,_mbi,_dx,_dy,0,
   _ref_idx,_block_err);
}

static int oc_mcenc_sad_check_bcandidate_fullpel(oc_mcenc_ctx *_mcenc,
 int _mbi,int _bi,int _dx,int _dy,int _pli,int _ref_idx){
  const oc_fragment_plane *fplane;
  const oc_fragment       *frag;
  ogg_int32_t              hitbit;
  int                      cur_stride;
  int                      ref_stride;
  int                      mvoffset;
  int                      err;
  hitbit=(ogg_int32_t)1<<_dx+16;
  /*If we already examined this vector, then we would be using it already if
     it was better than what we are using.*/
  if(_mcenc->hit_cache[_dy+16]&hitbit)return INT_MAX;
  /*NOTE: We don't store the bit once we've checked it for single blocks.
    This is because we use the same cache for the whole macro block.
    Maybe it's worth copying that and restoring it for every block, but this
     is untested.
    For now we assume it isn't.*/
  /*TODO: customize error function for speed/quality tradeoff.*/
  fplane=_mcenc->enc->state.frarray.frplanes+_pli;
  cur_stride=_mcenc->enc->state.frarray.input[_pli].stride;
  ref_stride=_mcenc->enc->state.ref_frame_bufs[_ref_idx][_pli].stride;
  mvoffset=_dx+_dy*ref_stride;
  err=0;
  frag=_mcenc->enc->state.frarray.frags+
   _mcenc->enc->state.frarray.mbs[_mbi].map[_pli][_bi];
  if(frag->border==NULL){
    err=oc_sad8_fullpel(frag->buffer[3],cur_stride,
     frag->buffer[_ref_idx]+mvoffset,ref_stride);
  }
  else{
    err=oc_sad8_fullpel_border(frag->buffer[3],cur_stride,
     frag->buffer[_ref_idx]+mvoffset,ref_stride,frag->border->mask);
  }
  return err;
}

static int oc_mcenc_ysad_check_bcandidate_fullpel(oc_mcenc_ctx *_mcenc,
 int _mbi,int _bi,int _dx,int _dy,int _ref_idx){
  return oc_mcenc_sad_check_bcandidate_fullpel(_mcenc,_mbi,_bi,_dx,_dy,0,
   _ref_idx);
}

static int oc_mcenc_ysad_halfpel_mbrefine(oc_mcenc_ctx *_mcenc,
 int _mbi,int _vec[2],int _best_err,int _ref_idx){
  int                      offset_y[8];
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  int                     *mb_map;
  int                      cur_stride;
  int                      ref_stride;
  int                      mvoffset0;
  int                      best_site;
  int                      nsites;
  int                      sitei;
  int                      err;
  int                      b;
  fplane=_mcenc->enc->state.frarray.frplanes+0;
  cur_stride=_mcenc->enc->state.frarray.input[0].stride;
  ref_stride=_mcenc->enc->state.ref_frame_bufs[_ref_idx][0].stride;
  frags=_mcenc->enc->state.frarray.frags;
  mb_map=_mcenc->enc->state.frarray.mbs[_mbi].map[0];
  mvoffset0=_vec[0]+_vec[1]*ref_stride;
  offset_y[0]=offset_y[1]=offset_y[2]=-ref_stride;
  offset_y[5]=offset_y[6]=offset_y[7]=ref_stride;
  err=_best_err;
  best_site=4;
  /*Compose the bit flags for boundary conditions.*/
  b=-_vec[0]>>4|(-_vec[1]>>4)<<2;
  nsites=OC_SQUARE_NSITES[b];
  for(sitei=0;sitei<nsites;sitei++){
    int site;
    int mvoffset1;
    site=OC_SQUARE_SITES[b][sitei];
    mvoffset1=mvoffset0+OC_SQUARE_DX[site]+offset_y[site];
    err=oc_sad16_halfpel(frags,mb_map,cur_stride,ref_stride,mvoffset0,
     mvoffset1,_ref_idx);
    if(err<_best_err){
      _best_err=err;
      best_site=site;
    }
  }
  _vec[0]=(_vec[0]<<1)+OC_SQUARE_DX[best_site];
  _vec[1]=(_vec[1]<<1)+OC_SQUARE_DY[best_site];
  return _best_err;
}

static int oc_mcenc_ysad_halfpel_brefine(oc_mcenc_ctx *_mcenc,
 int _mbi,int _bi,int _vec[2],int _best_err,int _ref_idx){
  int                      offset_y[8];
  const oc_fragment_plane *fplane;
  const oc_fragment       *frag;
  int                      cur_stride;
  int                      ref_stride;
  int                      mvoffset0;
  int                      best_site;
  int                      nsites;
  int                      sitei;
  int                      err;
  int                      b;
  fplane=_mcenc->enc->state.frarray.frplanes+0;
  cur_stride=_mcenc->enc->state.frarray.input[0].stride;
  ref_stride=_mcenc->enc->state.ref_frame_bufs[_ref_idx][0].stride;
  frag=_mcenc->enc->state.frarray.frags+
   _mcenc->enc->state.frarray.mbs[_mbi].map[0][_bi];
  if(frag->invalid)return _best_err;
  mvoffset0=_vec[0]+_vec[1]*ref_stride;
  offset_y[0]=offset_y[1]=offset_y[2]=-ref_stride;
  offset_y[5]=offset_y[6]=offset_y[7]=ref_stride;
  err=_best_err;
  best_site=4;
  /*Compose the bit flags for boundary conditions.*/
  b=-_vec[0]>>4|(-_vec[1]>>4)<<2;
  nsites=OC_SQUARE_NSITES[b];
  for(sitei=0;sitei<nsites;sitei++){
    int site;
    int mvoffset1;
    site=OC_SQUARE_SITES[b][sitei];
    mvoffset1=mvoffset0+OC_SQUARE_DX[site]+offset_y[site];
    if(frag->border==NULL){
      err=oc_sad8_halfpel(frag->buffer[3],cur_stride,
       frag->buffer[_ref_idx]+mvoffset0,frag->buffer[_ref_idx]+mvoffset1,
       ref_stride);
    }
    else{
      err=oc_sad8_halfpel_border(frag->buffer[3],cur_stride,
       frag->buffer[_ref_idx]+mvoffset0,frag->buffer[_ref_idx]+mvoffset1,
       ref_stride,frag->border->mask);
    }
    if(err<_best_err){
      _best_err=err;
      best_site=site;
    }
  }
  _vec[0]=(_vec[0]<<1)+OC_SQUARE_DX[best_site];
  _vec[1]=(_vec[1]<<1)+OC_SQUARE_DY[best_site];
  return _best_err;
}

oc_mcenc_ctx *oc_mcenc_alloc(oc_enc_ctx *_enc){
  oc_mcenc_ctx *mcenc;
  mcenc=_ogg_calloc(1,sizeof(*mcenc));
  mcenc->enc=_enc;
  return mcenc;
}

void oc_mcenc_free(oc_mcenc_ctx *_mcenc){
  _ogg_free(_mcenc);
}

void oc_mcenc_analyze(oc_mcenc_ctx *_mcenc){
  oc_fragment_plane *fplane;
  oc_mb_enc_info    *embs;
  oc_mb             *mbs;
  int                ref_idx;
  int                nmbs;
  int                mbi;
  /*TODO: customize error function for speed/(quality+size) tradeoff.*/
  ref_idx=_mcenc->enc->state.ref_frame_idx[OC_FRAME_PREV];
  /*If there is no previous frame, then skip motion analysis: Every vector has
     been initialized to (0,0).*/
  if(ref_idx<=0)return;
  fplane=_mcenc->enc->state.frarray.frplanes+0;
  mbs=_mcenc->enc->state.frarray.mbs;
  embs=_mcenc->enc->mbinfo;
  nmbs=fplane->nsbs<<2;
  for(mbi=0;mbi<nmbs;mbi++)if(mbs[mbi].mode>=0){
    oc_mb_enc_info *emb;
    int             block_err[4];
    int             best_block_err[4];
    int             best_block_vec[4][2];
    int             best_vec[2];
    int             best_err;
    int             candx;
    int             candy;
    int             bi;
    emb=embs+mbi;
    /*Find some candidate motion vectors.*/
    oc_mcenc_find_candidates(_mcenc,mbi,OC_FRAME_PREV);
    /*Clear the cache of locations we've examined.*/
    memset(_mcenc->hit_cache,0,sizeof(_mcenc->hit_cache));
    /*Start with the median predictor.*/
    candx=_mcenc->candidates[0][0];
    candy=_mcenc->candidates[0][1];
    memset(best_block_err,0,sizeof(best_block_err));
    best_err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,mbi,candx,candy,
     ref_idx,best_block_err);
    best_vec[0]=candx;
    best_vec[1]=candy;
    for(bi=0;bi<4;bi++){
      best_block_vec[bi][0]=candx;
      best_block_vec[bi][1]=candy;
    }
    /*If this predictor fails, move on to set A.*/
    if(best_err>OC_YSAD_THRESH1){
      oc_mb_enc_info *nemb;
      int             err;
      int             ci;
      int             ncs;
      int             t2;
      /*Compute the early termination threshold for set A.*/
      t2=emb->aerror;
      ncs=OC_MINI(3,emb->ncneighbors);
      for(ci=0;ci<ncs;ci++){
        nemb=embs+emb->cneighbors[ci];
        if(t2>nemb->aerror)t2=nemb->aerror;
      }
      t2=t2+(t2>>OC_YSAD_THRESH2_SCALE_BITS)+OC_YSAD_THRESH2_OFFSET;
      /*Examine the candidates in set A.*/
      for(ci=1;ci<_mcenc->setb0;ci++){
        candx=_mcenc->candidates[ci][0];
        candy=_mcenc->candidates[ci][1];
        err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,mbi,candx,candy,
         ref_idx,block_err);
        if(err<best_err){
          best_err=err;
          best_vec[0]=candx;
          best_vec[1]=candy;
        }
        for(bi=0;bi<4;bi++)if(block_err[bi]<best_block_err[bi]){
          best_block_err[bi]=block_err[bi];
          best_block_vec[bi][0]=candx;
          best_block_vec[bi][1]=candy;
        }
      }
      if(best_err>t2){
        /*Examine the candidates in set B.*/
        for(;ci<_mcenc->ncandidates;ci++){
          candx=_mcenc->candidates[ci][0];
          candy=_mcenc->candidates[ci][1];
          err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,mbi,candx,candy,
           ref_idx,block_err);
          if(err<best_err){
            best_err=err;
            best_vec[0]=candx;
            best_vec[1]=candy;
          }
          for(bi=0;bi<4;bi++)if(block_err[bi]<best_block_err[bi]){
            best_block_err[bi]=block_err[bi];
            best_block_vec[bi][0]=candx;
            best_block_vec[bi][1]=candy;
          }
        }
        /*Use the same threshold for set B as in set A.*/
        if(best_err>t2){
          int best_site;
          int nsites;
          int sitei;
          int site;
          int b;
          /*Square pattern search.*/
          for(;;){
            best_site=4;
            /*Compose the bit flags for boundary conditions.*/
            b=-best_vec[0]>>4|(best_vec[0]+1>>4)<<1|
             (-best_vec[1]>>4)<<2|(best_vec[1]+1>>4)<<3;
            nsites=OC_SQUARE_NSITES[b];
            for(sitei=0;sitei<nsites;sitei++){
              site=OC_SQUARE_SITES[b][sitei];
              candx=best_vec[0]+OC_SQUARE_DX[site];
              candy=best_vec[1]+OC_SQUARE_DY[site];
              err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,mbi,
               candx,candy,ref_idx,block_err);
              if(err<best_err){
                best_err=err;
                best_site=site;
              }
              for(bi=0;bi<4;bi++)if(block_err[bi]<best_block_err[bi]){
                best_block_err[bi]=block_err[bi];
                best_block_vec[bi][0]=candx;
                best_block_vec[bi][1]=candy;
              }
            }
            if(best_site==4)break;
            best_vec[0]+=OC_SQUARE_DX[best_site];
            best_vec[1]+=OC_SQUARE_DY[best_site];
          }
          /*Final 4-MV search.*/
          /*Simply use 1/4 of the macro block set A and B threshold as the
             individual block threshold.*/
          t2>>=2;
          for(bi=0;bi<4;bi++)if(best_block_err[bi]>t2){
            /*Square pattern search.*/
            for(;;){
              int bestx;
              int besty;
              best_site=4;
              bestx=best_block_vec[bi][0];
              besty=best_block_vec[bi][1];
              /*Compose the bit flags for boundary conditions.*/
              b=-bestx>>4|(bestx+1>>4)<<1|(-besty>>4)<<2|(besty+1>>4)<<3;
              nsites=OC_SQUARE_NSITES[b];
              for(sitei=0;sitei<nsites;sitei++){
                site=OC_SQUARE_SITES[b][sitei];
                candx=bestx+OC_SQUARE_DX[site];
                candy=besty+OC_SQUARE_DY[site];
                err=oc_mcenc_ysad_check_bcandidate_fullpel(_mcenc,mbi,bi,
                 candx,candy,ref_idx);
                if(err<best_block_err[bi]){
                  best_block_err[bi]=err;
                  best_site=site;
                }
              }
              if(best_site==4)break;
              best_block_vec[bi][0]+=OC_SQUARE_DX[best_site];
              best_block_vec[bi][1]+=OC_SQUARE_DY[best_site];
            }
          }
        }
      }
    }
    emb->aerror=oc_mcenc_ysad_halfpel_mbrefine(_mcenc,mbi,best_vec,best_err,
     ref_idx);
    emb->mvs[OC_FRAME_PREV][0][0]=(char)best_vec[0];
    emb->mvs[OC_FRAME_PREV][0][1]=(char)best_vec[1];
    emb->a4mverror=0;
    for(bi=0;bi<4;bi++){
      emb->a4mverror+=oc_mcenc_ysad_halfpel_brefine(_mcenc,mbi,bi,
       best_block_vec[bi],best_block_err[bi],ref_idx);
      emb->bmvs[bi][0]=(char)best_block_vec[bi][0];
      emb->bmvs[bi][1]=(char)best_block_vec[bi][1];
    }
  }
}
