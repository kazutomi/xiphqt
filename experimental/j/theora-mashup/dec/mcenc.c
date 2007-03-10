#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "encint.h"

struct oc_mcenc_ctx{
  oc_enc_ctx        *enc;
  oc_enc_pipe_stage  pipe;
  int                candidates[12][2];
  int                setb0;
  int                ncandidates;
  ogg_int32_t        mvapw1[2];
  ogg_int32_t        mvapw2[2];
};

/*The maximum Y plane SAD value for accepting the median predictor.*/
#define OC_YSAD_THRESH1            (256)
/*The amount to right shift the minimum error by when inflating it for
   computing the second maximum Y plane SAD threshold.*/
#define OC_YSAD_THRESH2_SCALE_BITS (3)
/*The amount to add to the second maximum Y plane threshold when inflating
   it.*/
#define OC_YSAD_THRESH2_OFFSET     (128)

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
  /* -15.5<dx<31,       -15.5<dy<15(.5)*/
  {0,1,2,3,5,6,7,8},
  /*-15.5==dx,          -15.5<dy<15(.5)*/
  {1,2,5,7,8},
  /*     dx==15(.5),    -15.5<dy<15(.5)*/
  {0,1,3,6,7},
  /*-15.5==dx==15(.5),  -15.5<dy<15(.5)*/
  {-1},
  /* -15.5<dx<15(.5),  -15.5==dy*/
  {3,5,6,7,8},
  /*-15.5==dx,         -15.5==dy*/
  {5,7,8},
  /*     dx==15(.5),   -15.5==dy*/
  {3,6,7},
  /*-15.5==dx==15(.5), -15.5==dy*/
  {-1},
  /*-15.5dx<15(.5),           dy==15(.5)*/
  {0,1,2,3,5},
  /*-15.5==dx,                dy==15(.5)*/
  {1,2,5},
  /*       dx==15(.5),        dy==15(.5)*/
  {0,1,3}
};



static void oc_mcenc_find_candidates(oc_mcenc_ctx *_mcenc,int _mbi,
 int _which_frame){
  oc_mb_enc_info *nemb;
  oc_mb_enc_info *emb;
  ogg_int32_t     mvapw1;
  ogg_int32_t     mvapw2;
  int             a[3][2];
  int             ncandidates;
  int             i;
  emb=_mcenc->enc->mbinfo+_mbi;
  if(emb->ncneighbors>0){
    /*Fill in the first part of set A: the last motion vectors used and the
       vectors from adjacent blocks.*/
    /*Skip a position to store the median predictor in.*/
    ncandidates=1;
    for(i=0;i<emb->ncneighbors;i++){
      nemb=_mcenc->enc->mbinfo+emb->cneighbors[i];
      _mcenc->candidates[ncandidates][0]=nemb->mvs[0][_which_frame][0];
      _mcenc->candidates[ncandidates][1]=nemb->mvs[0][_which_frame][1];
      ncandidates++;
    }
    /*Add a few additional vectors to set A: the vector used in the previous
       frame and the (0,0) vector.*/
    _mcenc->candidates[ncandidates][0]=emb->mvs[1][_which_frame][0];
    _mcenc->candidates[ncandidates][1]=emb->mvs[1][_which_frame][1];
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
  }
  /*The upper-left most macro block has no neighbors at all
    We just use 0,0 as the median predictor and its previous motion vector
     for set A.*/
  else{
    _mcenc->candidates[0][0]=0;
    _mcenc->candidates[0][1]=0;
    _mcenc->candidates[1][0]=emb->mvs[1][_which_frame][0];
    _mcenc->candidates[1][1]=emb->mvs[1][_which_frame][1];
    ncandidates=2;
  }
  /*Fill in set B: accelerated predictors for this and adjacent macro
     blocks.*/
  _mcenc->setb0=ncandidates;
  mvapw1=_mcenc->mvapw1[_which_frame];
  mvapw2=_mcenc->mvapw2[_which_frame];
  /*The first time through the loop use the current macro block.*/
  nemb=emb;
  for(i=0;;i++){
    _mcenc->candidates[ncandidates][0]=
     OC_DIV_ROUND_POW2(nemb->mvs[1][_which_frame][0]*mvapw1-
     nemb->mvs[2][_which_frame][0]*mvapw2,16,0x8000);
    _mcenc->candidates[ncandidates][1]=
     OC_DIV_ROUND_POW2(nemb->mvs[1][_which_frame][1]*mvapw1-
     nemb->mvs[2][_which_frame][1]*mvapw2,16,0x8000);
    _mcenc->candidates[ncandidates][0]=OC_CLAMPI(-31,
     _mcenc->candidates[ncandidates][0],31);
    _mcenc->candidates[ncandidates][1]=OC_CLAMPI(-31,
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

static int oc_sad16_halfpel(const oc_fragment *_frags,const int _fragis[4],
 int _cur_ystride,int _ref_ystride,int _mvoffset0,int _mvoffset1,
 int  _ref_framei){
  int err;
  int i;
  err=0;
  for(i=0;i<4;i++){
    const oc_fragment *frag;
    frag=_frags+_fragis[i];
    if(frag->border==NULL){
      if(!frag->invalid){
        err+=oc_sad8_halfpel(frag->buffer[OC_FRAME_IO],_cur_ystride,
         frag->buffer[_ref_framei]+_mvoffset0,
         frag->buffer[_ref_framei]+_mvoffset1,_ref_ystride);
      }
    }
    else{
      err+=oc_sad8_halfpel_border(frag->buffer[OC_FRAME_IO],_cur_ystride,
       frag->buffer[_ref_framei]+_mvoffset0,
       frag->buffer[_ref_framei]+_mvoffset1,_ref_ystride,frag->border->mask);
    }
  }
  return err;
}

static int oc_mcenc_sad_check_mbcandidate_fullpel(oc_mcenc_ctx *_mcenc,
 int _mbi,int _dx,int _dy,int _pli,int _ref_framei,int _block_err[4]){
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  int                     *mb_map;
  int                      cur_ystride;
  int                      ref_ystride;
  int                      mvoffset;
  int                      err;
  int                      bi;
  /*TODO: customize error function for speed/(quality+size) tradeoff.*/
  fplane=_mcenc->enc->state.fplanes+_pli;
  cur_ystride=_mcenc->enc->state.input[_pli].ystride;
  ref_ystride=_mcenc->enc->state.ref_frame_bufs[_ref_framei][_pli].ystride;
  frags=_mcenc->enc->state.frags;
  mb_map=_mcenc->enc->state.mbs[_mbi].map[_pli];
  mvoffset=_dx+_dy*ref_ystride;
  err=0;
  for(bi=0;bi<4;bi++)if(mb_map[bi]>=0){
    const oc_fragment *frag;
    frag=frags+mb_map[bi];
    if(frag->border==NULL){
      if(!frag->invalid){
        _block_err[bi]=oc_sad8_fullpel(frag->buffer[OC_FRAME_IO],cur_ystride,
         frag->buffer[_ref_framei]+mvoffset,ref_ystride);
      }
    }
    else{
      _block_err[bi]=oc_sad8_fullpel_border(frag->buffer[OC_FRAME_IO],
       cur_ystride,frag->buffer[_ref_framei]+mvoffset,ref_ystride,
       frag->border->mask);
    }
    err+=_block_err[bi];

  }
  return err;
}

static int oc_mcenc_ysad_check_mbcandidate_fullpel(oc_mcenc_ctx *_mcenc,
 int _mbi,int _dx,int _dy,int _ref_framei,int _block_err[4]){
  return oc_mcenc_sad_check_mbcandidate_fullpel(_mcenc,_mbi,_dx,_dy,0,
   _ref_framei,_block_err);
}

static int oc_mcenc_ysad_halfpel_mbrefine(oc_mcenc_ctx *_mcenc,
 int _mbi,int _vec[2],int _best_err,int _ref_framei){
  int                      offset_y[9];
  const oc_fragment_plane *fplane;
  const oc_fragment       *frags;
  int                     *mb_map;
  int                      cur_ystride;
  int                      ref_ystride;
  int                      mvoffset_base;
  int                      best_site;
  int                      sitei;
  int                      err;
  fplane=_mcenc->enc->state.fplanes+0;
  cur_ystride=_mcenc->enc->state.input[0].ystride;
  ref_ystride=_mcenc->enc->state.ref_frame_bufs[_ref_framei][0].ystride;
  frags=_mcenc->enc->state.frags;
  mb_map=_mcenc->enc->state.mbs[_mbi].map[0];
  mvoffset_base=_vec[0]+_vec[1]*ref_ystride;
  offset_y[0]=offset_y[1]=offset_y[2]=-ref_ystride;
  offset_y[3]=offset_y[5]=0;
  offset_y[6]=offset_y[7]=offset_y[8]=ref_ystride;
  err=_best_err;
  best_site=4;
  for(sitei=0;sitei<8;sitei++){
    int site;
    int xmask;
    int ymask;
    int dx;
    int dy;
    int mvoffset0;
    int mvoffset1;
    site=OC_SQUARE_SITES[0][sitei];
    dx=OC_SQUARE_DX[site];
    dy=OC_SQUARE_DY[site];
    /*The following code SHOULD be equivalent to
      oc_state_get_mv_offsets(&_mcenc->enc.state,&mvoffset0,&mvoffset1,
       (_vec[0]<<1)+dx,(_vec[1]<<1)+dy,ref_ystride,0);
      However, it should also be much faster, as it involves no multiplies and
       doesn't have to handle chroma vectors.*/
    xmask=((_vec[0]<<1)+OC_SQUARE_DX[site]>>6)^(dx>>1);
    ymask=((_vec[1]<<1)+OC_SQUARE_DY[site]>>6)^(dy>>1);
    mvoffset0=mvoffset_base+(dx&xmask)+(offset_y[site]&ymask);
    mvoffset1=mvoffset_base+(dx&~xmask)+(offset_y[site]&~ymask);
    err=oc_sad16_halfpel(frags,mb_map,cur_ystride,ref_ystride,mvoffset0,
     mvoffset1,_ref_framei);
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
 int _mbi,int _bi,int _vec[2],int _best_err,int _ref_framei){
  int                      offset_y[9];
  const oc_fragment_plane *fplane;
  const oc_fragment       *frag;
  int                      cur_ystride;
  int                      ref_ystride;
  int                      mvoffset_base;
  int                      best_site;
  int                      sitei;
  int                      err;
  fplane=_mcenc->enc->state.fplanes+0;
  cur_ystride=_mcenc->enc->state.input[0].ystride;
  ref_ystride=_mcenc->enc->state.ref_frame_bufs[_ref_framei][0].ystride;
  frag=_mcenc->enc->state.frags+_mcenc->enc->state.mbs[_mbi].map[0][_bi];
  if(frag->invalid)return _best_err;
  mvoffset_base=_vec[0]+_vec[1]*ref_ystride;
  offset_y[0]=offset_y[1]=offset_y[2]=-ref_ystride;
  offset_y[3]=offset_y[5]=0;
  offset_y[6]=offset_y[7]=offset_y[8]=ref_ystride;
  err=_best_err;
  best_site=4;
  for(sitei=0;sitei<8;sitei++){
    int site;
    int xmask;
    int ymask;
    int dx;
    int dy;
    int mvoffset0;
    int mvoffset1;
    site=OC_SQUARE_SITES[0][sitei];
    dx=OC_SQUARE_DX[site];
    dy=OC_SQUARE_DY[site];
    /*The following code SHOULD be equivalent to
      oc_state_get_mv_offsets(&_mcenc->enc.state,&mvoffset0,&mvoffset1,
       (_vec[0]<<1)+dx,(_vec[1]<<1)+dy,ref_ystride,0);
      However, it should also be much faster, as it involves no multiplies and
       doesn't have to handle chroma vectors.*/
    xmask=((_vec[0]<<1)+OC_SQUARE_DX[site]>>6)^(dx>>1);
    ymask=((_vec[1]<<1)+OC_SQUARE_DY[site]>>6)^(dy>>1);
    mvoffset0=mvoffset_base+(dx&xmask)+(offset_y[site]&ymask);
    mvoffset1=mvoffset_base+(dx&~xmask)+(offset_y[site]&~ymask);
    if(frag->border==NULL){
      err=oc_sad8_halfpel(frag->buffer[OC_FRAME_IO],cur_ystride,
       frag->buffer[_ref_framei]+mvoffset0,
       frag->buffer[_ref_framei]+mvoffset1,ref_ystride);
    }
    else{
      err=oc_sad8_halfpel_border(frag->buffer[OC_FRAME_IO],cur_ystride,
       frag->buffer[_ref_framei]+mvoffset0,
       frag->buffer[_ref_framei]+mvoffset1,ref_ystride,frag->border->mask);
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

/*Perform a motion vector search for this macro block against a single
   reference frame.
  As a bonus, individual block motion vectors are computed as well, as much of
   the work can be shared.
  The actual motion vector is stored in the appropriate place in the
   oc_mb_enc_info structure.
  _mcenc:    The motion compensation context.
  _mbi:      The macro block index.
  _frame:    The frame to search, either OC_FRAME_PREV or OC_FRAME_GOLD.
  _bmvs:     Returns the individual block motion vectors.
  _error:    Returns the prediction error for the macro block motion vector.
  _error4mv: Returns sum of the prediction error for the individual block
              motion vectors.*/
static void oc_mcenc_search(oc_mcenc_ctx *_mcenc,int _mbi,int _frame,
 char _bmvs[4][2],int *_error,int *_error4mv){
  oc_mb_enc_info *embs;
  oc_mb_enc_info *emb;
  oc_mb          *mb;
  ogg_int32_t     hit_cache[31];
  ogg_int32_t     hitbit;
  int             block_err[4];
  int             best_block_err[4];
  int             best_block_vec[4][2];
  int             best_vec[2];
  int             best_err;
  int             candx;
  int             candy;
  int             ref_framei;
  int             bi;
  /*TODO: customize error function for speed/(quality+size) tradeoff.*/
  ref_framei=_mcenc->enc->state.ref_frame_idx[_frame];
  mb=_mcenc->enc->state.mbs+_mbi;
  embs=_mcenc->enc->mbinfo;
  emb=embs+_mbi;
  /*Find some candidate motion vectors.*/
  oc_mcenc_find_candidates(_mcenc,_mbi,_frame);
  /*Clear the cache of locations we've examined.*/
  memset(hit_cache,0,sizeof(hit_cache));
  /*Start with the median predictor.*/
  candx=_mcenc->candidates[0][0];
  candy=_mcenc->candidates[0][1];
  hit_cache[candy+15]|=(ogg_int32_t)1<<candx+15;
  best_err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,candx,candy,
   ref_framei,block_err);
  best_vec[0]=candx;
  best_vec[1]=candy;
  for(bi=0;bi<4;bi++){
    best_block_err[bi]=block_err[bi];
    best_block_vec[bi][0]=candx;
    best_block_vec[bi][1]=candy;
  }
  /*If this predictor fails, move on to set A.*/
  if(best_err>OC_YSAD_THRESH1){
    int err;
    int ci;
    int ncs;
    int t2;
    /*Compute the early termination threshold for set A.*/
    t2=emb->aerror;
    ncs=OC_MINI(3,emb->ncneighbors);
    for(ci=0;ci<ncs;ci++)t2=OC_MAXI(t2,embs[emb->cneighbors[ci]].aerror);
    t2=t2+(t2>>OC_YSAD_THRESH2_SCALE_BITS)+OC_YSAD_THRESH2_OFFSET;
    /*Examine the candidates in set A.*/
    for(ci=1;ci<_mcenc->setb0;ci++){
      candx=_mcenc->candidates[ci][0];
      candy=_mcenc->candidates[ci][1];
      /*If we've already examined this vector, then we would be using it if it
         was better than what we are using.*/
      hitbit=(ogg_int32_t)1<<candx+15;
      if(hit_cache[candy+15]&hitbit)continue;
      hit_cache[candy+15]|=hitbit;
      err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,candx,candy,
       ref_framei,block_err);
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
        hitbit=(ogg_int32_t)1<<candx+15;
        if(hit_cache[candy+15]&hitbit)continue;
        hit_cache[candy+15]|=hitbit;
        err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,candx,candy,
         ref_framei,block_err);
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
          b=OC_DIV16(-best_vec[0]+1)|OC_DIV16(best_vec[0]+1)<<1|
           OC_DIV16(-best_vec[1]+1)<<2|OC_DIV16(best_vec[1]+1)<<3;
          nsites=OC_SQUARE_NSITES[b];
          for(sitei=0;sitei<nsites;sitei++){
            site=OC_SQUARE_SITES[b][sitei];
            candx=best_vec[0]+OC_SQUARE_DX[site];
            candy=best_vec[1]+OC_SQUARE_DY[site];
            hitbit=(ogg_int32_t)1<<candx+15;
            if(hit_cache[candy+15]&hitbit)continue;
            hit_cache[candy+15]|=hitbit;
            err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,
             candx,candy,ref_framei,block_err);
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
          /*Square pattern search.
            We do this in a slightly interesting manner.
            We continue to check the SAD of all four blocks in the macro
             block.
            This gives us two things:
             1) We can continue to use the hit_cache to avoid duplicate
                 checks.
                Otherwise we could continue to read it, but not write to it
                 without saving and restoring it for each block.
                Note that we could still eliminate a large number of
                 duplicate checks by taking into account the site we came
                 from when choosing the site list.
                We can still do that to avoid extra hit_cache queries, and it
                 might even be a speed win.
             2) It gives us a slightly better chance of escaping local minima.
                We would not be here if we weren't doing a fairly bad job in
                 finding a good vector, and checking these vectors can save us
                 from 100 to several thousand points off our SAD 1 in 15
                 times.
            TODO: Is this a good idea?
            Who knows.
            It needs more testing.*/
          for(;;){
            int bestx;
            int besty;
            int bj;
            bestx=best_block_vec[bi][0];
            besty=best_block_vec[bi][1];
            /*Compose the bit flags for boundary conditions.*/
            b=OC_DIV16(-bestx+1)|OC_DIV16(bestx+1)<<1|
             OC_DIV16(-besty+1)<<2|OC_DIV16(besty+1)<<3;
            nsites=OC_SQUARE_NSITES[b];
            for(sitei=0;sitei<nsites;sitei++){
              site=OC_SQUARE_SITES[b][sitei];
              candx=bestx+OC_SQUARE_DX[site];
              candy=besty+OC_SQUARE_DY[site];
              hitbit=(ogg_int32_t)1<<candx+15;
              if(hit_cache[candy+15]&hitbit)continue;
              hit_cache[candy+15]|=hitbit;
              err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,
               candx,candy,ref_framei,block_err);
              if(err<best_err){
                best_err=err;
                best_vec[0]=candx;
                best_vec[1]=candy;
              }
              for(bj=0;bj<4;bj++)if(block_err[bj]<best_block_err[bj]){
                best_block_err[bj]=block_err[bj];
                best_block_vec[bj][0]=candx;
                best_block_vec[bj][1]=candy;
              }
            }
            if(best_block_vec[bi][0]==bestx&&best_block_vec[bi][1]==besty){
              break;
            }
          }
        }
      }
    }
  }
  *_error=oc_mcenc_ysad_halfpel_mbrefine(_mcenc,_mbi,best_vec,best_err,
   ref_framei);
  emb->mvs[0][_frame][0]=(char)best_vec[0];
  emb->mvs[0][_frame][1]=(char)best_vec[1];
  *_error4mv=0;
  for(bi=0;bi<4;bi++){
    (*_error4mv)+=oc_mcenc_ysad_halfpel_brefine(_mcenc,_mbi,bi,
     best_block_vec[bi],best_block_err[bi],ref_framei);
    _bmvs[bi][0]=(char)best_block_vec[bi][0];
    _bmvs[bi][1]=(char)best_block_vec[bi][1];
  }
}

/*Perform a motion vector search for this macro block against a single
   reference frame.
  The actual motion vector is stored in the appropriate place in the
   oc_mb_enc_info structure.
  This is like the above oc_mcenc_search() routine, except that block-level
   motion vectors are not computed.
  _mcenc:    The motion compensation context.
  _mbi:      The macro block index.
  _frame:    The frame to search, either OC_FRAME_PREV or OC_FRAME_GOLD.
  Returns: The prediction error for the macro block motion vector.*/
int oc_mcenc_search_1mv(oc_mcenc_ctx *_mcenc,int _mbi,int _frame){
  oc_mb_enc_info *embs;
  oc_mb_enc_info *emb;
  oc_mb          *mb;
  ogg_int32_t     hit_cache[31];
  ogg_int32_t     hitbit;
  int             block_err[4];
  int             best_vec[2];
  int             best_err;
  int             candx;
  int             candy;
  int             ref_framei;
  /*TODO: customize error function for speed/(quality+size) tradeoff.*/
  ref_framei=_mcenc->enc->state.ref_frame_idx[_frame];
  mb=_mcenc->enc->state.mbs+_mbi;
  embs=_mcenc->enc->mbinfo;
  emb=embs+_mbi;
  /*Find some candidate motion vectors.*/
  oc_mcenc_find_candidates(_mcenc,_mbi,_frame);
  /*Clear the cache of locations we've examined.*/
  memset(hit_cache,0,sizeof(hit_cache));
  /*Start with the median predictor.*/
  candx=_mcenc->candidates[0][0];
  candy=_mcenc->candidates[0][1];
  hit_cache[candy+15]|=(ogg_int32_t)1<<candx+15;
  best_err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,candx,candy,
   ref_framei,block_err);
  best_vec[0]=candx;
  best_vec[1]=candy;
  /*If this predictor fails, move on to set A.*/
  if(best_err>OC_YSAD_THRESH1){
    int err;
    int ci;
    int ncs;
    int t2;
    /*Compute the early termination threshold for set A.*/
    t2=emb->aerror;
    ncs=OC_MINI(3,emb->ncneighbors);
    for(ci=0;ci<ncs;ci++)t2=OC_MAXI(t2,embs[emb->cneighbors[ci]].aerror);
    t2=t2+(t2>>OC_YSAD_THRESH2_SCALE_BITS)+OC_YSAD_THRESH2_OFFSET;
    /*Examine the candidates in set A.*/
    for(ci=1;ci<_mcenc->setb0;ci++){
      candx=_mcenc->candidates[ci][0];
      candy=_mcenc->candidates[ci][1];
      /*If we've already examined this vector, then we would be using it if it
         was better than what we are using.*/
      hitbit=(ogg_int32_t)1<<candx+15;
      if(hit_cache[candy+15]&hitbit)continue;
      hit_cache[candy+15]|=hitbit;
      err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,candx,candy,
       ref_framei,block_err);
      if(err<best_err){
        best_err=err;
        best_vec[0]=candx;
        best_vec[1]=candy;
      }
    }
    if(best_err>t2){
      /*Examine the candidates in set B.*/
      for(;ci<_mcenc->ncandidates;ci++){
        candx=_mcenc->candidates[ci][0];
        candy=_mcenc->candidates[ci][1];
        hitbit=(ogg_int32_t)1<<candx+15;
        if(hit_cache[candy+15]&hitbit)continue;
        hit_cache[candy+15]|=hitbit;
        err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,candx,candy,
         ref_framei,block_err);
        if(err<best_err){
          best_err=err;
          best_vec[0]=candx;
          best_vec[1]=candy;
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
          b=OC_DIV16(-best_vec[0]+1)|OC_DIV16(best_vec[0]+1)<<1|
           OC_DIV16(-best_vec[1]+1)<<2|OC_DIV16(best_vec[1]+1)<<3;
          nsites=OC_SQUARE_NSITES[b];
          for(sitei=0;sitei<nsites;sitei++){
            site=OC_SQUARE_SITES[b][sitei];
            candx=best_vec[0]+OC_SQUARE_DX[site];
            candy=best_vec[1]+OC_SQUARE_DY[site];
            hitbit=(ogg_int32_t)1<<candx+15;
            if(hit_cache[candy+15]&hitbit)continue;
            hit_cache[candy+15]|=hitbit;
            err=oc_mcenc_ysad_check_mbcandidate_fullpel(_mcenc,_mbi,
             candx,candy,ref_framei,block_err);
            if(err<best_err){
              best_err=err;
              best_site=site;
            }
          }
          if(best_site==4)break;
          best_vec[0]+=OC_SQUARE_DX[best_site];
          best_vec[1]+=OC_SQUARE_DY[best_site];
        }
      }
    }
  }
  best_err=oc_mcenc_ysad_halfpel_mbrefine(_mcenc,_mbi,best_vec,best_err,
   ref_framei);
  emb->mvs[0][_frame][0]=(char)best_vec[0];
  emb->mvs[0][_frame][1]=(char)best_vec[1];
  return best_err;
}

/*A pipe to perform a motion vector search for each macro block.*/

static int oc_mcenc_pipe_start(oc_enc_pipe_stage *_stage){
  oc_enc_ctx  *enc;
  ogg_int64_t  nframes;
  int          pli;
  int          mbi;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=0;
  /*Move the motion vector predictors back a frame.
    We could pipeline this, too, but it's probably not worth it.*/
  enc=_stage->enc;
  for(mbi=enc->state.fplanes[0].nsbs<<2;mbi-->0;){
    oc_mb_enc_info *emb;
    emb=enc->mbinfo+mbi;
    memmove(emb->mvs+1,emb->mvs,2*sizeof(emb->mvs[0]));
  }
  /*Set up the accelerated MV weights for previous frame prediction.*/
  enc->mcenc->mvapw1[OC_FRAME_PREV]=(ogg_int32_t)1<<17;
  enc->mcenc->mvapw2[OC_FRAME_PREV]=(ogg_int32_t)1<<16;
  /*Set up the accelerated MV weights for golden frame prediction.*/
  nframes=enc->state.curframe_num-enc->state.keyframe_num;
  enc->mcenc->mvapw1[OC_FRAME_GOLD]=(ogg_int32_t)(
   nframes!=1?(nframes<<17)/(nframes-1):0);
  enc->mcenc->mvapw2[OC_FRAME_GOLD]=(ogg_int32_t)(
   nframes!=2?(nframes<<16)/(nframes-2):0);
  return _stage->next!=NULL?(*_stage->next->pipe_start)(_stage->next):0;
}

static int oc_mcenc_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  oc_mcenc_ctx   *mcenc;
  int             pli;
  mcenc=_stage->enc->mcenc;
  /*For now we ignore the chroma planes.*/
  for(pli=1;pli<3;pli++)_stage->y_procd[pli]=_y_avail[pli];
  /*Only do motion analysis if there is a previous frame; otherwise every
     vector has already been initialized to (0,0).*/
  if(mcenc->enc->state.ref_frame_idx[OC_FRAME_PREV]>=0){
    int             y_avail;
    y_avail=_y_avail[0];
    /*Round to a super-block row, except for the last one, which may be
       incomplete.*/
    if(y_avail<(int)mcenc->enc->state.info.frame_height)y_avail&=~31;
    while(_stage->y_procd[0]<y_avail){
      oc_mb_enc_info *embs;
      oc_mb          *mbs;
      int             mbi;
      int             mbi_end;
      mbi=(_stage->y_procd[0]>>4)*mcenc->enc->state.fplanes[0].nhsbs;
      mbi_end=mbi+mcenc->enc->state.fplanes[0].nhsbs<<1;
      mbs=mcenc->enc->state.mbs;
      embs=mcenc->enc->mbinfo;
      for(;mbi<mbi_end;mbi++)if(mbs[mbi].mode!=OC_MODE_INVALID){
        oc_mb_enc_info *emb;
        emb=embs+mbi;
        oc_mcenc_search(mcenc,mbi,OC_FRAME_PREV,emb->bmvs,&emb->aerror,
         &emb->aerror4mv);
      }
      /*Chain to the next stage.*/
      _stage->y_procd[0]=OC_MINI(_stage->y_procd[0]+32,y_avail);
      if(_stage->next!=NULL){
        int ret;
        ret=_stage->next->pipe_proc(_stage->next,_stage->y_procd);
        if(ret<0)return ret;
      }
    }
  }
  else{
    _stage->y_procd[0]=_y_avail[0];
    if(_stage->next!=NULL){
      return _stage->next->pipe_proc(_stage->next,_stage->y_procd);
    }
  }
  return 0;
}

static int oc_mcenc_pipe_end(oc_enc_pipe_stage *_stage){
  return _stage->next!=NULL?(*_stage->next->pipe_end)(_stage->next):0;
}

/*Initialize the motion vector search stage of the pipeline.
  _enc: The encoding context.*/
static void oc_mcenc_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_mcenc_pipe_start;
  _stage->pipe_proc=oc_mcenc_pipe_process;
  _stage->pipe_end=oc_mcenc_pipe_end;
}

oc_mcenc_ctx *oc_mcenc_alloc(oc_enc_ctx *_enc){
  oc_mcenc_ctx *mcenc;
  mcenc=_ogg_calloc(1,sizeof(*mcenc));
  mcenc->enc=_enc;
  oc_mcenc_pipe_init(&mcenc->pipe,_enc);
  return mcenc;
}

void oc_mcenc_free(oc_mcenc_ctx *_mcenc){
  _ogg_free(_mcenc);
}

oc_enc_pipe_stage *oc_mcenc_prepend_to_pipe(oc_mcenc_ctx *_mcenc,
 oc_enc_pipe_stage *_next){
  _mcenc->pipe.next=_next;
  return &_mcenc->pipe;
}
