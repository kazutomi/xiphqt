#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "fdct.h"
#if defined(OC_DUMP_IMAGES)
# include <stdio.h>
# include "png.h"
#endif



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
      Additional vectors are used, so there will always be at least 3, except
       for in the upper-left most macro block.*/
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
    if(mb->mode==OC_MODE_INVALID)continue;
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
      if(frarray->mbs[nmbi].mode==OC_MODE_INVALID)continue;
      emb->cneighbors[emb->ncneighbors++]=nmbi;
    }
    /*Fill in the neighbors with previous motion vectors available.*/
    for(i=0;i<4;i++){
      nmbx=mbx+PDX[i];
      nmby=mby+PDY[i];
      if(nmbx<0||nmbx>=frarray->nhmbs||nmby<0||nmby>=frarray->nvmbs)continue;
      nmbi=oc_frarray_mbi_for_pos(frarray,nmbx,nmby);
      if(frarray->mbs[nmbi].mode==OC_MODE_INVALID)continue;
      emb->pneighbors[emb->npneighbors++]=nmbi;
    }
  }
}

/*Select a quantizer for the given quality.*/
static void oc_enc_quant_sel_quality(oc_enc_ctx *_enc,int _intra_only){
  unsigned qmax[2][3];
  int      qti;
  int      pli;
  int      qi;
  int      ci;
  /*This quantizer selection really bugs me.
    In order to guarantee a minimum error value, we must select a quantizer
     no larger than twice the smallest allowable error for each coefficient.
    In reality, most fragments will not require such a small quantizer, but
     Theora only supports per-frame quantizer selection.*/
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    qmax[qti][pli]=OC_MAXI(2U*_enc->tol_mins[pli][0],OC_DC_QUANT_MIN[qti]);
  }
  for(qi=ci=0;qi<63;qi++){
    for(;;){
      if(_enc->state.dequant_tables[0][0][0][qi][ci]>qmax[0][0]||
       _enc->state.dequant_tables[0][1][0][qi][ci]>qmax[0][1]||
       _enc->state.dequant_tables[0][2][0][qi][ci]>qmax[0][2]||
       !_intra_only&&(
       _enc->state.dequant_tables[1][0][0][qi][ci]>qmax[1][0]||
       _enc->state.dequant_tables[1][1][0][qi][ci]>qmax[1][1]||
       _enc->state.dequant_tables[1][2][0][qi][ci]>qmax[1][2])){
        break;
      }
      if(++ci>=64)break;
      for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
        qmax[qti][pli]=
         OC_MAXI(2U*_enc->tol_mins[pli][ci],OC_AC_QUANT_MIN[qti]);
      }
    }
    if(ci==64)break;
  }
  _enc->state.qi=qi;
}

static void oc_enc_frame_header_pack(oc_enc_ctx *_enc){
  /*Mark this packet as a data packet.*/
  oggpackB_write(&_enc->opb,0,1);
  /*Write out the frame type (I or P).*/
  oggpackB_write(&_enc->opb,_enc->state.frame_type,1);
  /*Write out the current QI.*/
  oggpackB_write(&_enc->opb,_enc->state.qi,6);
  if(_enc->state.frame_type==OC_INTRA_FRAME){
    /*Keyframes have 3 unused configuration bits, holdovers from VP3 days.
      Most of the other unused bits in the VP3 headers were eliminated.
      I don't know why these remain.*/
    oggpackB_write(&_enc->opb,0,3);
  }
}

/*Mark all fragments as coded and in OC_MODE_INTRA.*/
static void oc_enc_mark_all_intra(oc_enc_ctx *_enc){
  oc_fragment *frag;
  oc_fragment *frag_end;
  frag=_enc->state.frarray.frags;
  frag_end=frag+_enc->state.frarray.nfrags;
  for(;frag<frag_end;frag++){
    frag->coded=1;
    frag->mbmode=OC_MODE_INTRA;
#if defined(OC_BITRATE_STATS)
    /*Compute the error function used for intra mode fragments.
      This function can only use information known at mode decision time, and
       so excludes the DC component.*/
    {
      oc_fragment_enc_info *frinfo;
      int                   ci;
      int                   eerror;
      frinfo=_enc->frinfo+(frag-_enc->state.frarray.frags);
      eerror=0;
      for(ci=1;ci<64;ci++)eerror+=abs(frinfo->dct_coeffs[ci]);
      frinfo->eerror=eerror;
    }
#endif
  }
}

/*The number of different DCT coefficient values that can be stored by each
   of the different DCT value category tokens.*/
static const int OC_DCT_VAL_CAT_SIZES[6]={2,4,8,16,32,512};
/*The number of bits to shift the sign of the DCT coefficient over by for each
   of the different DCT value category tokens.*/
static const int OC_DCT_VAL_CAT_SHIFTS[6]={1,2,3,4,5,9};

/*Quantize and predict the DC coefficients.
  This is done in a separate step because the prediction of DC coefficients
   occurs in image order, not in the Hilbert-curve order, unlike the rest of
   the encoding process.*/
static void oc_enc_quant_dc(oc_enc_ctx *_enc){
  oc_fragment_enc_info *frinfo;
  oc_fragment          *frag;
  int                   pli;
  frag=_enc->state.frarray.frags;
  frinfo=_enc->frinfo;
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *fplane;
    unsigned           fquant;
    unsigned           iquant;
    int                pred_last[3];
    int                fragx;
    int                fragy;
    pred_last[OC_FRAME_GOLD]=0;
    pred_last[OC_FRAME_PREV]=0;
    pred_last[OC_FRAME_SELF]=0;
    fplane=_enc->state.frarray.frplanes+pli;
    for(fragy=0;fragy<fplane->nvfrags;fragy++){
      for(fragx=0;fragx<fplane->nhfrags;fragx++,frag++,frinfo++){
        int qc_pred;
        int qc;
        if(!frag->coded)continue;
        qc_pred=oc_frag_pred_dc(frag,fplane,fragx,fragy,pred_last);
        /*Fragments outside the displayable region must still be coded in key
           frames.
          To minimize wasted bits, just use the predicted DC value.
          TODO: We might do a better job in the lower-left hand corner by
           propagating over the DC value of the first actually coded fragment,
           but for the moment this is not done.*/
        if(frag->invalid)qc=0;
        else{
          int c;
          int c_abs;
          int qti;
          /*We now center the DC coefficient range around the predicted value
             and perform token bits optimization based on the HVS-determined
             tolerance range.
            For more details, see oc_enc_frag_quant_tokenize().*/
          qti=frag->mbmode!=OC_MODE_INTRA;
          iquant=_enc->state.dequant_tables[qti][pli][0][_enc->state.qi][0];
          c=frinfo->dct_coeffs[0]-qc_pred*iquant;
          c_abs=abs(c);
          if(c_abs<=frinfo->tols[0])qc=0;
          else{
            int qc_max;
            int qc_min;
            int qc_offs;
            int c_sign;
            int c_min;
            int c_recon;
            int cati;
            fquant=_enc->enquant_tables[qti][pli][0][_enc->state.qi][0];
            qc_max=(ogg_int32_t)c_abs*fquant+OC_FQUANT_ROUND>>OC_FQUANT_SHIFT;
            c_sign=c<0;
            c_recon=(qc_max-1)*iquant;
            c_min=OC_MAXI(0,c_abs-frinfo->tols[0]);
            for(qc_min=qc_max;c_recon>=c_min;qc_min--)c_recon-=iquant;
            if(qc_min<3+OC_NDCT_VAL_CAT2_SIZE)qc=qc_min;
            else{
              qc_offs=3+OC_NDCT_VAL_CAT2_SIZE;
              for(cati=0;cati<5&&qc_min>=qc_offs+OC_DCT_VAL_CAT_SIZES[cati];
               cati++){
                qc_offs+=OC_DCT_VAL_CAT_SIZES[cati];
              }
              qc=OC_MINI(qc_offs+OC_DCT_VAL_CAT_SIZES[cati]-1,qc_max);
            }
            qc*=(-c_sign<<1)+1;
          }
        }
        pred_last[OC_FRAME_FOR_MODE[frag->mbmode]]=frag->dc=qc+qc_pred;
        frinfo->dct_coeffs[0]=(ogg_int16_t)qc;
      }
    }
  }
}

static int oc_enc_frag_quant_tokenize(oc_enc_ctx *_enc,
 oc_fragment_enc_info *_frinfo,const ogg_uint16_t _fquant[64],
 const ogg_uint16_t _iquant[64]){
  int zzi;
  int zrun;
  int qc;
  int qc_offs;
  int c_sign;
  int cati;
  int tli;
  /*The DC coefficient is already quantized (it had to be for prediction).
    Here we just tokenize it.*/
  if(_frinfo->dct_coeffs[0]){
    qc=abs(_frinfo->dct_coeffs[0]);
    c_sign=_frinfo->dct_coeffs[0]<0;
    _enc->dct_tok_fragi[0][_enc->ndct_tokens[0]]=_frinfo-_enc->frinfo;
    switch(qc){
      case 1:{
        _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
         (unsigned char)(OC_ONE_TOKEN+c_sign);
      }break;
      case 2:{
        _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
         (unsigned char)(OC_TWO_TOKEN+c_sign);
      }break;
      default:{
        if(qc-3<OC_NDCT_VAL_CAT2_SIZE){
          _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
           (unsigned char)(OC_DCT_VAL_CAT2+qc-3);
          _enc->extra_bits[0][_enc->nextra_bits[0]++]=(ogg_uint16_t)c_sign;
        }
        else{
          qc_offs=3+OC_NDCT_VAL_CAT2_SIZE;
          for(cati=0;qc>=qc_offs+OC_DCT_VAL_CAT_SIZES[cati];cati++){
            qc_offs+=OC_DCT_VAL_CAT_SIZES[cati];
          }
          _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
           (unsigned char)(OC_DCT_VAL_CAT3+cati);
          _enc->extra_bits[0][_enc->nextra_bits[0]++]=
           (ogg_uint16_t)((c_sign<<OC_DCT_VAL_CAT_SHIFTS[cati])+qc-qc_offs);
        }
      }
    }
    zrun=0;
  }
  else zrun=1;
  /*Now we quantize and tokenize each AC coefficient.*/
  for(zzi=1;zzi<64;zzi++){
    int qc_max;
    int qc_min;
    int c_sign;
    int c_abs;
    int c_min;
    int c_recon;
    int ci;
    ci=OC_FZIG_ZAG[zzi];
    c_abs=abs(_frinfo->dct_coeffs[ci]);
    /*Best case: we can encode this as a zero.*/
    if(c_abs<=_frinfo->tols[ci]){
      zrun++;
      _frinfo->dct_coeffs[ci]=0;
    }
    else{
      c_sign=_frinfo->dct_coeffs[ci]<0;
      /*qc_max is the most accurate quantized value.
        This is the largest possible (absolute) value we will use.*/
      qc_max=(ogg_int32_t)c_abs*_fquant[ci]+OC_FQUANT_ROUND>>OC_FQUANT_SHIFT;
      /*qc_min is the smallest possible (absolute) quantized value whose
         dequantized value is within the HVS-determined tolerance range.*/
      /*TODO: qc_min could be computed by a division (we do not want to allow
         the rounding errors that are possible with the mul+shift quantization
         used for qc_max), which would allow qc_max to be calculated only if
         needed below.
        Is this faster?
        Who knows.*/
      c_recon=(qc_max-1)*_iquant[ci];
      c_min=c_abs-_frinfo->tols[ci];
      for(qc_min=qc_max;c_recon>=c_min;qc_min--)c_recon-=_iquant[ci];
      /*We now proceed to find a token that is as close to qc_max as possible,
         but does not use any more bits than would be required for qc_min.
        The general assumption we make is that encoding a value closer to 0
         always uses fewer bits.
        qc_min can still reach 0 here despite the test above, if the quantizer
         value is larger than the tolerance (which can happen for very small
         tolerances; the quantizer value has a minimum it cannot go below).*/
      if(qc_min==0){
        zrun++;
        _frinfo->dct_coeffs[ci]=0;
      }
      else{
        /*If we have an outstanding zero run, code it now.*/
        if(zrun>0){
          /*The zero run tokens appear on the list for the first zero in the
             run.*/
          tli=zzi-zrun;
          _enc->dct_tok_fragi[tli][_enc->ndct_tokens[tli]]=_frinfo-_enc->frinfo;
          /*Second assumption: coding a combined run/value token always uses
             fewer bits than coding them separately.*/
          /*CAT1 run/value tokens: the value is 1.*/
          if(qc_min==1&&zrun<=17){
            if(zrun<=5){
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               (unsigned char)(OC_DCT_RUN_CAT1A+(zrun-1));
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)c_sign;
            }
            else if(zrun<=9){
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_RUN_CAT1B;
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)((c_sign<<2)+zrun-6);
            }
            else{
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_RUN_CAT1C;
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)((c_sign<<3)+zrun-10);
            }
            _frinfo->dct_coeffs[ci]=(ogg_int16_t)((-c_sign<<1)+1);
            zrun=0;
            /*Skip coding the DCT value below.*/
            continue;
          }
          /*CAT2 run/value tokens: the value is 2-3.*/
          else if(qc_min<=3&&zrun<=3){
            if(zrun==1){
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_RUN_CAT2A;
              qc=OC_MINI(1,qc_max-2);
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)((c_sign<<1)+qc);
            }
            else{
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_RUN_CAT2B;
              qc=OC_MINI(1,qc_max-2);
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)((c_sign<<2)+(qc<<1)+zrun-2);
            }
            _frinfo->dct_coeffs[ci]=(ogg_int16_t)(((-c_sign<<1)+1)*qc_min);
            zrun=0;
            /*Skip coding the DCT value below.*/
            continue;
          }
          /*The run is too long or the quantized value too large: code them
             separately.*/
          else{
            /*This is stupid: non-short ZRL tokens are never used for run
               values less than 9, but codewords are reserved for them,
               wasting bits.
              Yes, yes, this would've meant a non-constant number of extra
               bits for this token, but even so.*/
            if(zrun<=8){
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_SHORT_ZRL_TOKEN;
            }
            else{
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_ZRL_TOKEN;
            }
            _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
             (ogg_uint16_t)(zrun-1);
            zrun=0;
          }
        }
        /*No zero run, or the run and the qc value are being coded
           separately.*/
        _enc->dct_tok_fragi[zzi][_enc->ndct_tokens[zzi]]=_frinfo-_enc->frinfo;
        switch(qc_min){
          case 1:{
            _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
             (unsigned char)(OC_ONE_TOKEN+c_sign);
            _frinfo->dct_coeffs[ci]=(ogg_int16_t)((-c_sign<<1)+1);
          }break;
          case 2:{
            _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
             (unsigned char)(OC_TWO_TOKEN+c_sign);
            _frinfo->dct_coeffs[ci]=(ogg_int16_t)((-c_sign<<2)+2);
          }break;
          default:{
            if(qc_min-3<OC_NDCT_VAL_CAT2_SIZE){
              _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
               (unsigned char)(OC_DCT_VAL_CAT2+qc_min-3);
              _enc->extra_bits[zzi][_enc->nextra_bits[zzi]++]=
               (ogg_uint16_t)c_sign;
              _frinfo->dct_coeffs[ci]=(ogg_int16_t)(((-c_sign<<1)+1)*qc_min);
            }
            else{
              qc_offs=3+OC_NDCT_VAL_CAT2_SIZE;
              for(cati=0;cati<5&&qc_min>=qc_offs+OC_DCT_VAL_CAT_SIZES[cati];
               cati++){
                qc_offs+=OC_DCT_VAL_CAT_SIZES[cati];
              }
              /*qc_min can be encoded in this category.
                Since all DCT values in the category use the same number of
                 bits, we encode the closest value to qc_max.
                This is either qc_max itself, if it is in the category's
                 range, or the largest value in the category.*/
              qc=OC_MINI(qc_offs+OC_DCT_VAL_CAT_SIZES[cati]-1,qc_max);
              _frinfo->dct_coeffs[ci]=(ogg_int16_t)(((-c_sign<<1)+1)*qc);
              _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
               (unsigned char)(OC_DCT_VAL_CAT3+cati);
              _enc->extra_bits[zzi][_enc->nextra_bits[zzi]++]=(ogg_uint16_t)
               ((c_sign<<OC_DCT_VAL_CAT_SHIFTS[cati])+qc-qc_offs);
            }
          }
        }
      }
    }
  }
  /*If there's a trailing zero run, code an EOB token.*/
  if(zrun>0){
    int old_tok;
    int toki;
    int ebi;
    tli=64-zrun;
    toki=_enc->ndct_tokens[tli]-1;
    if(toki>=0)old_tok=_enc->dct_tokens[tli][toki];
    else old_tok=-1;
    /*Try to extend an EOB run.*/
    switch(old_tok){
      case OC_DCT_EOB1_TOKEN:
      case OC_DCT_EOB2_TOKEN:{
        _enc->dct_tokens[tli][toki]++;
      }break;
      case OC_DCT_EOB3_TOKEN:{
        _enc->dct_tokens[tli][toki]++;
        _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=0;
      }break;
      case OC_DCT_REPEAT_RUN0_TOKEN:{
        ebi=_enc->nextra_bits[tli]-1;
        if(_enc->extra_bits[tli][ebi]<3)_enc->extra_bits[tli][ebi]++;
        else{
          _enc->dct_tokens[tli][toki]++;
          _enc->extra_bits[tli][ebi]=0;
        }
      }break;
      case OC_DCT_REPEAT_RUN1_TOKEN:{
        ebi=_enc->nextra_bits[tli]-1;
        if(_enc->extra_bits[tli][ebi]<7)_enc->extra_bits[tli][ebi]++;
        else{
          _enc->dct_tokens[tli][toki]++;
          _enc->extra_bits[tli][ebi]=0;
        }
      }break;
      case OC_DCT_REPEAT_RUN2_TOKEN:{
        ebi=_enc->nextra_bits[tli]-1;
        if(_enc->extra_bits[tli][ebi]<15)_enc->extra_bits[tli][ebi]++;
        else{
          _enc->dct_tokens[tli][toki]++;
          /*Again stupid: we could encode runs up to 4127, but inexplicably
             they don't subtract the bottom of the range here, so we can only
             go to 4095 (unless we want to deal with wraparound).*/
          _enc->extra_bits[tli][ebi]=32;
        }
      }break;
      case OC_DCT_REPEAT_RUN3_TOKEN:{
        ebi=_enc->nextra_bits[tli]-1;
        if(_enc->extra_bits[tli][ebi]<4095){
          _enc->extra_bits[tli][ebi]++;
          break;
        }
        /*else fall through.*/
      }
      /*Start a new EOB run.*/
      default:{
        _enc->dct_tok_fragi[tli][_enc->ndct_tokens[tli]]=_frinfo-_enc->frinfo;
        _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=OC_DCT_EOB1_TOKEN;
      }
    }
  }
  /*Return the number of coefficients before the final zero run.*/
  return 64-zrun;
}

static void oc_enc_residual_tokenize(oc_enc_ctx *_enc){
  oc_sb *sb;
  oc_sb *sb_end;
  int    pli;
  int    zzi;
  /*Clear any existing DCT tokens.*/
  for(zzi=0;zzi<64;zzi++){
    _enc->ndct_tokens[zzi]=_enc->nextra_bits[zzi]=0;
    _enc->extra_bits_offs[zzi]=0;
  }
  sb=sb_end=_enc->state.frarray.sbs;
  for(pli=0;pli<3;pli++){
    oc_fragment_plane  *fplane;
    fplane=_enc->state.frarray.frplanes+pli;
    sb_end+=fplane->nsbs;
    memcpy(_enc->dct_token_offs[pli],_enc->ndct_tokens,
     sizeof(_enc->dct_token_offs[pli]));
    for(;sb<sb_end;sb++){
      int mbi;
      for(mbi=0;mbi<4;mbi++)if(sb->mb_valid&1<<mbi){
        int bi;
        for(bi=0;bi<4;bi++)if(sb->map[mbi][bi]>=0){
          oc_fragment *frag;
          frag=_enc->state.frarray.frags+sb->map[mbi][bi];
          if(frag->coded){
            oc_fragment_enc_info *frinfo;
            int                   qti;
            int                   nnzc;
            qti=frag->mbmode!=OC_MODE_INTRA;
            frinfo=_enc->frinfo+sb->map[mbi][bi];
            nnzc=oc_enc_frag_quant_tokenize(_enc,frinfo,
             _enc->enquant_tables[qti][pli][0][_enc->state.qi],
             _enc->state.dequant_tables[qti][pli][0][_enc->state.qi]);
            oc_frag_recon(&_enc->state,frag,pli,frinfo->dct_coeffs,nnzc,
             _enc->state.dequant_tables[qti][pli][0][_enc->state.qi]);
          }
        }
      }
    }
  }
#if defined(OC_DUMP_IMAGES)
  /*Dump a PNG of the reconstructed image.*/
  {
    png_structp    png;
    png_infop      info;
    png_bytep     *image;
    FILE          *fp;
    char           fname[16];
    unsigned char *y_row;
    unsigned char *u_row;
    unsigned char *v_row;
    unsigned char *y;
    unsigned char *u;
    unsigned char *v;
    int            y_stride;
    int            u_stride;
    int            v_stride;
    int            framei;
    int            width;
    int            height;
    int            imgi;
    int            imgj;
    width=_enc->state.info.width;
    height=_enc->state.info.height;
    sprintf(fname,"%08irec.png",_enc->state.curframe_num);
    fp=fopen(fname,"wb");
    if(fp==NULL)return;
    image=(png_bytep *)oc_malloc_2d(height,6*width,sizeof(image[0][0]));
    png=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if(png==NULL){
      oc_free_2d(image);
      fclose(fp);
      return;
    }
    info=png_create_info_struct(png);
    if(info==NULL){
      png_destroy_write_struct(&png,NULL);
      oc_free_2d(image);
      fclose(fp);
      return;
    }
    if(setjmp(png_jmpbuf(png))){
      png_destroy_write_struct(&png,&info);
      oc_free_2d(image);
      fclose(fp);
      return;
    }
    framei=_enc->state.ref_frame_idx[OC_FRAME_SELF];
    y_row=_enc->state.ref_frame_bufs[framei][0].data;
    u_row=_enc->state.ref_frame_bufs[framei][1].data;
    v_row=_enc->state.ref_frame_bufs[framei][2].data;
    y_stride=_enc->state.ref_frame_bufs[framei][0].stride;
    u_stride=_enc->state.ref_frame_bufs[framei][1].stride;
    v_stride=_enc->state.ref_frame_bufs[framei][2].stride;
    for(imgi=height;imgi-->0;){
      int dc;
      y=y_row;
      u=u_row;
      v=v_row;
      for(imgj=0;imgj<6*width;){
        float    yval;
        float    uval;
        float    vval;
        unsigned rval;
        unsigned gval;
        unsigned bval;
        yval=(*y-16)*(1.0F/219);
        uval=(*u-128)*(2*(1-0.114F)/224);
        vval=(*v-128)*(2*(1-0.299F)/224);
        rval=OC_CLAMPI(0,(int)(65535*(yval+vval)+0.5F),65535);
        gval=OC_CLAMPI(0,(int)(65535*(
         yval-uval*(0.114F/0.587F)-vval*(0.299F/0.587F))+0.5F),65535);
        bval=OC_CLAMPI(0,(int)(65535*(yval+uval)+0.5F),65535);
        image[imgi][imgj++]=(unsigned char)(rval>>8);
        image[imgi][imgj++]=(unsigned char)(rval&0xFF);
        image[imgi][imgj++]=(unsigned char)(gval>>8);
        image[imgi][imgj++]=(unsigned char)(gval&0xFF);
        image[imgi][imgj++]=(unsigned char)(bval>>8);
        image[imgi][imgj++]=(unsigned char)(bval&0xFF);
        dc=y-y_row&1;
        y++;
        u+=dc;
        v+=dc;
      }
      dc=-(height-1-imgi&1);
      y_row+=y_stride;
      u_row+=dc&u_stride;
      v_row+=dc&v_stride;
    }
    png_init_io(png,fp);
    png_set_compression_level(png,Z_BEST_COMPRESSION);
    png_set_IHDR(png,info,width,height,16,PNG_COLOR_TYPE_RGB,
     PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
    switch(_enc->state.info.colorspace){
      case OC_CS_ITU_REC_470M:{
        png_set_gAMA(png,info,2.2);
        png_set_cHRM_fixed(png,info,31006,31616,
         67000,32000,21000,71000,14000,8000);
      }break;
      case OC_CS_ITU_REC_470BG:{
        png_set_gAMA(png,info,2.67);
        png_set_cHRM_fixed(png,info,31271,32902,
         64000,33000,29000,60000,15000,6000);
      }break;
    }
    png_set_pHYs(png,info,_enc->state.info.aspect_numerator,
     _enc->state.info.aspect_denominator,0);
    png_set_rows(png,info,image);
    png_write_png(png,info,PNG_TRANSFORM_IDENTITY,NULL);
    png_write_end(png,info);
    png_destroy_write_struct(&png,&info);
    oc_free_2d(image);
    fclose(fp);
  }
#endif
  /*Merge the final EOB run of one coefficient list with the start of the
     next, if possible.*/
  for(zzi=1;zzi<64;zzi++){
    static const int OC_EOB_RANGE[OC_NDCT_EOB_TOKEN_MAX]={1,1,1,4,8,16,4096};
    static const int OC_EOB_OFFS[OC_NDCT_EOB_TOKEN_MAX]={1,2,3,4,8,16,0};
    int old_tok1;
    int old_tok2;
    int old_eb1;
    int old_eb2;
    int new_tok;
    int toki;
    int zzj;
    int ebi;
    int runl;
    /*Make sure this coefficient has tokens at all.*/
    if(_enc->ndct_tokens[zzi]<=0)continue;
    /*Ensure the first token is an EOB run.*/
    old_tok2=_enc->dct_tokens[zzi][0];
    if(old_tok2>=OC_NDCT_EOB_TOKEN_MAX)continue;
    /*Search for a previous coefficient that has any tokens at all.*/
    old_tok1=OC_NDCT_EOB_TOKEN_MAX;
    for(zzj=zzi;zzj-->0;){
      toki=_enc->ndct_tokens[zzj]-1;
      if(toki>=_enc->dct_token_offs[0][zzj]){
        old_tok1=_enc->dct_tokens[zzj][toki];
        break;
      }
    }
    /*Ensure its last token was an EOB run.*/
    if(old_tok1>=OC_NDCT_EOB_TOKEN_MAX)continue;
    /*Pull off the associated extra bits, if any, and decode the runs.*/
    ebi=_enc->nextra_bits[zzj];
    old_eb1=OC_DCT_TOKEN_EXTRA_BITS[old_tok1]?
     _enc->extra_bits[zzj][--ebi]:0;
    old_eb2=OC_DCT_TOKEN_EXTRA_BITS[old_tok2]?
     old_eb2=_enc->extra_bits[zzi][0]:0;
    runl=OC_EOB_OFFS[old_tok1]+old_eb1+OC_EOB_OFFS[old_tok2]+old_eb2;
    /*We can't possibly combine these into one run.
      It might be possible to split them more optimally, but we'll just leave
       them as is.*/
    if(runl>=4096)continue;
    /*We CAN combine them into one run.*/
    for(new_tok=OC_DCT_EOB1_TOKEN;
     runl-OC_EOB_OFFS[new_tok]>=OC_EOB_RANGE[new_tok];new_tok++);
    _enc->dct_tokens[zzj][toki]=(unsigned char)new_tok;
    /*Update the two token lists.*/
    if(OC_DCT_TOKEN_EXTRA_BITS[new_tok]){
      _enc->extra_bits[zzj][ebi++]=(ogg_uint16_t)(
       runl-OC_EOB_OFFS[new_tok]);
    }
    _enc->nextra_bits[zzj]=ebi;
    _enc->dct_token_offs[0][zzi]++;
    /*Note: We don't bother to update the offsets for planes 1 and 2 if
       planes 0 or 1 don't have any tokens.
      This turns out not to matter due to the way we use the offsets later.*/
    if(OC_DCT_TOKEN_EXTRA_BITS[old_tok2])_enc->extra_bits_offs[zzi]++;
  }
}

/*Counts the tokens of each type used for the given range of coefficient
   indices in zig-zag order.
  _enc: The encoding context.
  _zzi_start:      The first zig-zag index to include.
  _zzi_end:        The first zig-zag index to not include.
  _token_counts_y: Returns the token counts for the Y' plane.
  _token_counts_c: Returns the token counts for the Cb and Cr planes.*/
static void oc_enc_count_tokens(oc_enc_ctx *_enc,int _zzi_start,int _zzi_end,
 int _token_counts_y[32],int _token_counts_c[32]){
  int zzi;
  int ti;
  memset(_token_counts_y,0,sizeof(_token_counts_y[0])*32);
  memset(_token_counts_c,0,sizeof(_token_counts_c[0])*32);
  for(zzi=_zzi_start;zzi<_zzi_end;zzi++){
    for(ti=_enc->dct_token_offs[0][zzi];ti<_enc->dct_token_offs[1][zzi];ti++){
      _token_counts_y[_enc->dct_tokens[zzi][ti]]++;
    }
    /*Note: don't reset ti; dct_token_offs might be non-monotonic.*/
    for(;ti<_enc->ndct_tokens[zzi];ti++){
      _token_counts_c[_enc->dct_tokens[zzi][ti]]++;
    }
  }
}

/*Computes the number of bits used for each of the potential Huffman codes for
   the given list of token counts.
  The bits are added to whatever the current bit counts are.*/
static void oc_enc_count_bits(oc_enc_ctx *_enc,int _hgi,const int _token_counts[32],
 int _bit_counts[16]){
  int huffi;
  int huff_base;
  int token;
  huff_base=_hgi<<4;
  for(huffi=huff_base;huffi<huff_base+16;huffi++){
    for(token=0;token<32;token++){
      _bit_counts[huffi-huff_base]+=
       _token_counts[token]*_enc->huff_codes[huffi][token].len;
    }
  }
}

/*Returns the Huffman index using the fewest number of bits.*/
static int oc_enc_select_huffi(int _bit_counts[16]){
  int best_huffi;
  int huffi;
  best_huffi=0;
  for(huffi=1;huffi<16;huffi++)if(_bit_counts[huffi]<_bit_counts[best_huffi]){
    best_huffi=huffi;
  }
  return best_huffi;
}

/*Packs the DCT tokens for the given range of coefficient indices in zig-zag
   order using the given Huffman tables.*/
static void oc_enc_pack_huff_group(oc_enc_ctx *_enc,int _zzi_start,
 int _zzi_end,int _huff_idxs[3]){
  int zzi;
  for(zzi=_zzi_start;zzi<_zzi_end;zzi++){
    int pli;
    int ti;
    int ebi;
    ti=_enc->dct_token_offs[0][zzi];
    ebi=_enc->extra_bits_offs[zzi];
    for(pli=0;pli<3;pli++){
      const theora_huff_code *huff_codes;
      int                     token;
      int                     ti_end;
      /*Step 2: Write the tokens using these tables.*/
      huff_codes=_enc->huff_codes[_huff_idxs[pli]];
      /*Note: dct_token_offs[3] is really the ndct_tokens table.
        Yes, this seems like a horrible hack, yet it's strangely elegant.*/
      ti_end=_enc->dct_token_offs[pli+1][zzi];
      /*Note: don't reset ti; dct_token_offs might be non-monotonic.*/
      for(;ti<ti_end;ti++){
        token=_enc->dct_tokens[zzi][ti];
        oggpackB_write(&_enc->opb,huff_codes[token].bits,
         huff_codes[token].len);
        if(OC_DCT_TOKEN_EXTRA_BITS[token]){
          oggpackB_write(&_enc->opb,_enc->extra_bits[zzi][ebi++],
           OC_DCT_TOKEN_EXTRA_BITS[token]);
        }
      }
    }
  }
}

static void oc_enc_pack_residual_tokens(oc_enc_ctx *_enc){
  static const int OC_HUFF_LIST_MIN[5]={0,1,6,15,28};
  static const int OC_HUFF_LIST_MAX[5]={1,6,15,28,64};
  int bits_y[16];
  int bits_c[16];
  int token_counts_y[32];
  int token_counts_c[32];
  int huff_idxs[5][3];
  int huffi_y;
  int huffi_c;
  int hgi;
  /*Step 1a: Select Huffman tables for the DC token list.*/
  memset(bits_y,0,sizeof(bits_y));
  memset(bits_c,0,sizeof(bits_c));
  oc_enc_count_tokens(_enc,0,1,token_counts_y,token_counts_c);
  oc_enc_count_bits(_enc,0,token_counts_y,bits_y);
  oc_enc_count_bits(_enc,0,token_counts_c,bits_c);
  huffi_y=oc_enc_select_huffi(bits_y);
  huffi_c=oc_enc_select_huffi(bits_c);
  huff_idxs[0][0]=huffi_y;
  huff_idxs[0][1]=huff_idxs[0][2]=huffi_c;
  /*Step 1b: Write the DC token list with the chosen tables.*/
  oggpackB_write(&_enc->opb,huffi_y,4);
  oggpackB_write(&_enc->opb,huffi_c,4);
  oc_enc_pack_huff_group(_enc,0,1,huff_idxs[0]);
  /*Step 2a: Select Huffman tables for the AC token lists.*/
  memset(bits_y,0,sizeof(bits_y));
  memset(bits_y,0,sizeof(bits_c));
  for(hgi=1;hgi<5;hgi++){
    oc_enc_count_tokens(_enc,OC_HUFF_LIST_MIN[hgi],OC_HUFF_LIST_MAX[hgi],
     token_counts_y,token_counts_c);
    oc_enc_count_bits(_enc,hgi,token_counts_y,bits_y);
    oc_enc_count_bits(_enc,hgi,token_counts_c,bits_c);
  }
  huffi_y=oc_enc_select_huffi(bits_y);
  huffi_c=oc_enc_select_huffi(bits_c);
  /*Step 2b: Write the AC token lists using the chosen tables.*/
  oggpackB_write(&_enc->opb,huffi_y,4);
  oggpackB_write(&_enc->opb,huffi_c,4);
  for(hgi=1;hgi<5;hgi++){
    huff_idxs[hgi][0]=huffi_y+(hgi<<4);
    huff_idxs[hgi][1]=huff_idxs[hgi][2]=huffi_c+(hgi<<4);
    oc_enc_pack_huff_group(_enc,OC_HUFF_LIST_MIN[hgi],OC_HUFF_LIST_MAX[hgi],
     huff_idxs[hgi]);
  }
#if defined(OC_BITRATE_STATS)
  oc_bitrate_update_stats(_enc,huff_idxs);
#endif
}

static int oc_enc_keyframe(oc_enc_ctx *_enc){
  _enc->state.frame_type=OC_INTRA_FRAME;
  oc_enc_quant_sel_quality(_enc,1);
  oc_enc_mark_all_intra(_enc);
  oc_enc_quant_dc(_enc);
  oc_enc_residual_tokenize(_enc);
  oggpackB_reset(&_enc->opb);
  oc_enc_frame_header_pack(_enc);
  oc_enc_pack_residual_tokens(_enc);
  /*Success: Mark the packet as ready to be flushed.*/
  _enc->packet_state=OC_PACKET_READY;
  return 0;
}

static void oc_enc_choose_mbmodes(oc_enc_ctx *_enc){
  oc_fragment_enc_info *frinfo;
  oc_fragment          *frag;
  oc_fragment          *frag_end;
  int                   pli;
  /*Step 1: Select which fragments need to be coded.*/
  frinfo=_enc->frinfo;
  frag_end=frag=_enc->state.frarray.frags;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    int                      stride;
    int                      prev_refi;
    fplane=_enc->state.frarray.frplanes+pli;
    frag_end+=fplane->nfrags;
    prev_refi=_enc->state.ref_frame_idx[OC_FRAME_PREV];
    stride=_enc->state.ref_frame_bufs[prev_refi][pli].stride;
    for(;frag<frag_end;frag++,frinfo++){
      if(frag->invalid)frag->coded=0;
      else{
        ogg_int16_t dct_buf[64];
        int         ci;
        /*Check to see if the fragment can be skipped.
          It is assumed that a skipped fragment always takes fewer bits than a
           coded fragment, though this may not necessarily be true.*/
        oc_enc_frag_fdct(_enc,frag,dct_buf,stride,prev_refi);
        for(ci=0;ci<64;ci++){
          if(abs(dct_buf[ci]-frinfo->dct_coeffs[ci])>frinfo->tols[ci])break;
        }
        frag->coded=ci<64;
      }
    }
  }

  /*TODO: Select a MB mode to code each macro block in.*/
}

static int oc_enc_deltaframe(oc_enc_ctx *_enc){
  _enc->state.frame_type=OC_INTER_FRAME;
  oc_enc_quant_sel_quality(_enc,0);
  oc_enc_choose_mbmodes(_enc);
  oc_enc_quant_dc(_enc);
  oc_enc_residual_tokenize(_enc);
  oggpackB_reset(&_enc->opb);
  oc_enc_frame_header_pack(_enc);
  /*TODO: pack frag coded flags/mb modes.*/
  oc_enc_pack_residual_tokens(_enc);
  /*Success: Mark the packet as ready to be flushed.*/
  _enc->packet_state=OC_PACKET_READY;
  return 0;
}

static int oc_enc_init(oc_enc_ctx *_enc,theora_info *_info){
  int bii;
  int ret;
  _info->frame_width-=6;
  _info->frame_height-=10;
  _info->offset_x+=3;
  _info->offset_y+=5;
  ret=oc_state_init(&_enc->state,_info);
  if(ret<0)return ret;
  oggpackB_writeinit(&_enc->opb);
  _enc->frinfo=_ogg_calloc(_enc->state.frarray.nfrags,
   sizeof(_enc->frinfo[0]));
  _enc->mbinfo=_ogg_calloc(_enc->state.frarray.nmbs,sizeof(_enc->mbinfo[0]));
  oc_enc_init_mbinfo(_enc);
  for(bii=0;bii<_enc->state.frarray.nborders;bii++){
    oc_border_enc_info_init(_enc->border_info+bii,
     _enc->state.frarray.borders+bii);
  }
  _enc->mcenc=oc_mcenc_alloc(_enc);
  _enc->impmap=oc_impmap_alloc(_enc);
  _enc->psych=oc_psych_alloc(_enc);
  _enc->packet_state=OC_PACKET_INFO_HDR;
  /*TODO: Revisit qscale.*/
  /*Currently this is set so that a qi of 0 corresponds to distortions of 24
     times the JND, and each increase by 16 halves that value.
    This gives us fine discrimination at low qualities, yet effective rate
     control at high qualities.
    A qi of 63 is speical, however.
    For this, the highest quality, we use one half of a JND for our threshold.
    Due to the lower bounds placed on admissible quantizers, we will not
     actually be able to achieve quality this good, but this should provide as
     close to visually lossless quality as Theora is capable of.
    Now that we have a more powerful way to specify quantization matrices, we
     could lift the quantizer restrictions without breaking VP3.1
     compatibility, but this would result in quantized coefficients that are
     too large for the current bitstream to be able to store.*/
  _enc->qscale=_info->quality>=63?0.5F:
   1.5F*OC_POWF(2,0.0625F*(64-_info->quality));
  memcpy(_enc->huff_codes,OC_VP31_HUFF_CODES,sizeof(_enc->huff_codes));
  theora_encode_set_quant_params(_enc,NULL);
  _enc->dct_tokens=(unsigned char **)oc_malloc_2d(64,
   _enc->state.frarray.nfrags,sizeof(_enc->dct_tokens[0][0]));
  _enc->dct_tok_fragi=(int **)oc_malloc_2d(64,
   _enc->state.frarray.nfrags,sizeof(_enc->dct_tok_fragi[0][0]));
  _enc->extra_bits=(ogg_uint16_t **)oc_malloc_2d(64,
   _enc->state.frarray.nfrags,sizeof(_enc->extra_bits[0][0]));
  return 0;
}

static void oc_enc_clear(oc_enc_ctx *_enc){
  oc_psych_free(_enc->psych);
  oc_impmap_free(_enc->impmap);
  oc_mcenc_free(_enc->mcenc);
  _ogg_free(_enc->mbinfo);
  _ogg_free(_enc->frinfo);
  oc_free_2d(_enc->dct_tokens);
  oc_free_2d(_enc->dct_tok_fragi);
  oc_free_2d(_enc->extra_bits);
  oc_state_clear(&_enc->state);
}



/*Performs an fDCT on a given fragment.
  _frag:     The fragment to perform the 2D DCT on.
  _dct_vals: The output buffer for the DCT coefficients.
  _stride:   The stride of the plane the fragment belongs to.
  _ref_idx:  The picture buffer to perform the DCT on.
             Use OC_FRAME_IO for the current input frame.*/
void oc_enc_frag_fdct(oc_enc_ctx *_enc,const oc_fragment *_frag,
 ogg_int16_t _dct_vals[64],int _stride,int _ref_idx){
  ogg_int16_t    pix_buf[64];
  unsigned char *pixels;
  int            pixi;
  int            y;
  int            x;
  /*NOTE: 128 is subtracted from each pixel value to make it signed.
    The original VP3 source claimed that, "this reduces the internal precision
     requirments [sic] in the DCT transform."
    This is of course not actually true.
    The transform must still support input in the range [-255,255] to code
     predicted fragments, since the same transform is used for both.
    This actually _reduces_ the precision of the results, because larger
     values would have fewer significant bits chopped off when rounding.
    We're stuck with it, however.
    At least it might claim to reduce bias towards 0 when coding unpredicted
     DC coefficients.*/
  pixels=_frag->buffer[_ref_idx];
  /*For border fragments, only copy pixels that are in the displayable
     region of the image.
    The DCT function will compute optimal padding values for the other
     pixels.*/
  if(_frag->border!=NULL){
    ogg_int64_t mask;
    int         bii;
    mask=_frag->border->mask;
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++){
        pix_buf[pixi]=(ogg_int16_t)(((int)mask&1)?pixels[x]-128:0);
        /*This branchless code is equivalent to the previous line:
        int pmask;
        pmask=-(int)_mask&1;
        pix_buf[pixi]=(ogg_int16_t)(pmask&pixels[x]);
        We don't use this code to allow the user to pass in a buffer that is
         the exact size of the displayed image, not the size padded to a
         multiple of 16.
        In the latter case, we might segfault on pixels[x] if it is not mapped
         to a valid page.*/
        mask>>=1;
      }
      pixels+=_stride;
    }
    bii=_frag->border-_enc->state.frarray.borders;
    oc_fdct8x8_border(_frag->border,_enc->border_info+bii,_dct_vals,pix_buf);
  }
  /*Otherwise, copy all the pixels in the fragment and do a normal DCT.*/
  else{
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++)pix_buf[pixi]=(ogg_int16_t)(pixels[x]-128);
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
  int qti;
  int pli;
  int qri;
  int qi;
  int ci;
  if(_enc==NULL)return OC_FAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return OC_EINVAL;
  if(_qinfo==NULL)_qinfo=&OC_VP31_QUANT_INFO;
  /*Verify that, for a given qti, pli and ci, the actual quantizer will never
     increase as qi increases.
    This is required for efficient quantizer selection.*/
  for(qi=64;qi-->1;){
    if(_qinfo->ac_scale[qi]>_qinfo->ac_scale[qi-1])return OC_EINVAL;
    if(_qinfo->dc_scale[qi]>_qinfo->dc_scale[qi-1])return OC_EINVAL;
  }
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    if(_qinfo->qi_ranges[qti][pli].nranges<=0)return OC_EINVAL;
    for(qi=qri=0;qri<_qinfo->qi_ranges[qti][pli].nranges;qri++){
      qi+=_qinfo->qi_ranges[qti][pli].sizes[qri];
      if(qi<64||_qinfo->qi_ranges[qti][pli].sizes[qri]>1){
        for(ci=0;ci<64;ci++){
          if(_qinfo->qi_ranges[qti][pli].base_matrices[qri+1][ci]>
           _qinfo->qi_ranges[qti][pli].base_matrices[qri][ci]){
            return OC_EINVAL;
          }
        }
      }
    }
  }
  /*TODO: Analyze for packing purposes instead of just doing a shallow copy.*/
  memcpy(&_enc->qinfo,_qinfo,sizeof(_enc->qinfo));
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    _enc->state.dequant_tables[qti][pli]=
     _enc->state.dequant_table_data[qti]+pli;
    _enc->enquant_tables[qti][pli]=_enc->enqaunt_table_data[qti]+pli;
  }
  oc_enquant_tables_init(_enc->state.dequant_tables,_enc->enquant_tables,
   _qinfo);
  return 0;
}


int theora_encode_ycbcr_in(theora_enc_ctx *_enc,theora_ycbcr_buffer _img){
  theora_ycbcr_buffer img;
  int                 cwidth;
  int                 cheight;
  int                 ret;
  int                 rfi;
  int                 mbi;
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
  /*Flip the input buffer upside down.*/
  oc_ycbcr_buffer_flip(img,_img);
  /*Step 2: Update state.*/
  if(_enc->state.ref_frame_idx[OC_FRAME_SELF]>=0){
    /*Fill in the borders from the reconstructed version of the last encoded
       frame.*/
    oc_state_borders_fill(&_enc->state,
     _enc->state.ref_frame_idx[OC_FRAME_SELF]);
    /*Use this frame data for the appropriate reference frames, depending on
       the type.*/
    if(_enc->state.frame_type==OC_INTRA_FRAME){
      _enc->state.keyframe_num=_enc->state.curframe_num;
      _enc->state.ref_frame_idx[OC_FRAME_GOLD]=
       _enc->state.ref_frame_idx[OC_FRAME_PREV]=
       _enc->state.ref_frame_idx[OC_FRAME_SELF];
    }
    else{
     _enc->state.ref_frame_idx[OC_FRAME_PREV]=
      _enc->state.ref_frame_idx[OC_FRAME_SELF];
    }
  }
  /*Select a free buffer to use for the reconstructed version of this frame.*/
  for(rfi=0;rfi==_enc->state.ref_frame_idx[OC_FRAME_GOLD]||
   rfi==_enc->state.ref_frame_idx[OC_FRAME_PREV];rfi++);
  _enc->state.ref_frame_idx[OC_FRAME_SELF]=rfi;
  /*Move the motion vector predictors back a frame.*/
  for(mbi=_enc->state.frarray.frplanes[0].nsbs<<2;mbi-->0;){
    oc_mb_enc_info *emb;
    emb=_enc->mbinfo+mbi;
    memmove(emb->mvs[1][0],emb->mvs[0][0],sizeof(emb->mvs[0]));
  }
  _enc->state.curframe_num++;
  /*Step 3: Fill the fragment array with pointers into the user buffer.*/
  oc_frarray_fill_buffer_ptrs(&_enc->state.frarray,OC_FRAME_IO,img);
  /*Step 4: Analyze motion.*/
  oc_mcenc_analyze(_enc->mcenc);
  /*Step 5: Compute importance map.*/
  oc_impmap_fill(_enc->impmap,
   _enc->state.info.fps_denominator/(float)_enc->state.info.fps_numerator);
  /*Step 6: Compute coefficient tolerances.*/
  oc_psych_scan(_enc->psych,0);
  /*Step 7: Encode!*/
  if(_enc->state.curframe_num==1||
   _enc->state.curframe_num-_enc->state.keyframe_num>=
   _enc->state.info.keyframe_frequency_force){
    ret=oc_enc_keyframe(_enc);
    if(ret<0)return ret;
  }
  else{
    /*ret=oc_enc_deltaframe(_enc);*/
    ret=oc_enc_keyframe(_enc);
    if(ret<0)return ret;
  }
  /*Note: All buffer management, etc., that is done after a frame is encoded
     is delayed until the next frame is encoded.
    This allows for a future API that would let an encoding application
     examine the decompressed output and attempt to re-encode the same frame
     again with different settings if it disapproved.
    Here we just update the granpos needed for the output packet and return.*/
  if(_enc->state.frame_type==OC_INTRA_FRAME){
    _enc->state.granpos=
     _enc->state.curframe_num<<_enc->state.keyframe_granule_shift;
  }
  else{
    _enc->state.granpos=
     (_enc->state.keyframe_num<<_enc->state.keyframe_granule_shift)+
     (_enc->state.curframe_num-_enc->state.keyframe_num);
  }
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
  _op->granulepos=_enc->state.granpos;
  if(_last)_enc->packet_state=OC_PACKET_DONE;
  else _enc->packet_state=OC_PACKET_EMPTY;
  return 1;
}

double theora_granule_time(void *_encdec,ogg_int64_t _granpos){
  oc_theora_state *state;
  state=(oc_theora_state *)_encdec;
  if(_granpos>=0){
    ogg_int64_t iframe;
    ogg_int64_t pframe;
    iframe=_granpos>>state->keyframe_granule_shift;
    pframe=_granpos-(iframe<<state->keyframe_granule_shift);
    return (iframe+pframe)*(
     (double)state->info.fps_denominator/state->info.fps_numerator);
  }
  return(-1);
}