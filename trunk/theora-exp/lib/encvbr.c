#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>
#include "encvbr.h"
#include "fdct.h"



/*Returns the number of bits used by the given motion vector with the VLC
   motion vector codes (as opposed to the CLC codes, which always use 12 bits).
  _dx: The X component of the vector, in half-pel units.
  _dy: The Y component of the vector, in half-pel units.
  Return: The number of bits required to store the vector with the VLC codes.*/
static int oc_mvbitsa(int _dx,int _dy){
  return OC_MV_CODES[0][_dx+31].nbits+OC_MV_CODES[0][_dy+31].nbits;
}



/*Select the set of quantizers to use for the current frame for each possible
   frame type (intra or inter).
  This does not assign a quantizer to each fragment, as that depends on the
   quantizer type used and thus is done during mode decision.*/
static void oc_enc_vbr_quant_sel_quality(oc_enc_ctx *_enc,int _intra_only){
  unsigned              qmax[2][3];
  int                   qi_min[2];
  int                   qi_max[2];
  int                   fti;
  int                   qti;
  int                   pli;
  int                   dc_qi[2];
  qi_min[0]=_enc->vbr->cfg.kf_qi_min;
  qi_min[1]=_enc->vbr->cfg.df_qi_min;
  qi_max[0]=_enc->vbr->cfg.kf_qi_max;
  qi_max[1]=_enc->vbr->cfg.df_qi_max;
  /*The first quantizer value is used for DC coefficients.
    Select one that allows us to meet our quality requirements.*/
  for(qti=0;qti<1+!_intra_only;qti++)for(pli=0;pli<3;pli++){
    qmax[qti][pli]=OC_MAXI(2U*_enc->vbr->dc_tol_mins[pli],
     OC_DC_QUANT_MIN[qti]);
  }
  /*For intra frames...(containing just INTRA fragments)*/
  for(dc_qi[0]=qi_min[0];dc_qi[0]<qi_max[0];dc_qi[0]++){
    if(_enc->state.dequant_tables[0][0][dc_qi[0]][0]<=qmax[0][0]&&
     _enc->state.dequant_tables[0][1][dc_qi[0]][0]<=qmax[0][1]&&
     _enc->state.dequant_tables[0][2][dc_qi[0]][0]<=qmax[0][2]){
      break;
    }
  }
  /*For inter frames...(containing both INTER and INTRA fragments)*/
  if(!_intra_only){
    for(dc_qi[1]=OC_CLAMPI(qi_min[1],dc_qi[0],qi_max[1]);dc_qi[1]<qi_max[1];
     dc_qi[1]++){
      if(_enc->state.dequant_tables[1][0][dc_qi[1]][0]<=qmax[1][0]&&
       _enc->state.dequant_tables[1][1][dc_qi[1]][0]<=qmax[1][1]&&
       _enc->state.dequant_tables[1][2][dc_qi[1]][0]<=qmax[1][2]){
        break;
      }
    }
  }
  /*Now we select a full qi list for each frame type.*/
  for(fti=0;fti<1+!_intra_only;fti++){
    oc_fragment_enc_info *efrag;
    int                   ncoded_fragis;
    int                   nqis[64];
    int                   qi;
    int                   qi0;
    int                   qi1;
    int                   qi2;
    /*Here we count up the number of fragments that can use each qi value.
      Unless we know this is an intra frame, we don't know what quantizer type
       will be used for each fragment, so we just count both of them.*/
    memset(nqis,0,sizeof(nqis));
    if(fti){
      int *coded_fragi;
      int *coded_fragi_end;
      coded_fragi=_enc->state.coded_fragis;
      ncoded_fragis=_enc->state.ncoded_fragis[0]+
       _enc->state.ncoded_fragis[1]+_enc->state.ncoded_fragis[2];
      coded_fragi_end=coded_fragi+ncoded_fragis;
      for(;coded_fragi<coded_fragi_end;coded_fragi++){
        efrag=_enc->frinfo+*coded_fragi;
        for(qti=0;qti<2;qti++)nqis[efrag->qi_min[qti]]++;
      }
    }
    else{
      oc_fragment_enc_info *efrag_end;
      ncoded_fragis=_enc->state.nfrags;
      efrag=_enc->frinfo;
      efrag_end=efrag+ncoded_fragis;
      for(;efrag<efrag_end;efrag++)nqis[efrag->qi_min[0]]++;
    }
    /*We'll now choose the qi values that divide the fragments into equally
       sized groups, or as close as we can make it.
      We account for the DC coefficients by adding an extra amount to the qi
       value they require.
      Since there are usually many more DC coefficients coded than any one AC
       coefficient, we use 1/8 of the number of fragments, instead of 1/64.*/
    nqis[dc_qi[fti]]+=(ncoded_fragis<<fti)+7>>3;
    /*Convert this into a moment table.*/
    for(qi=63;qi-->0;)nqis[qi]+=nqis[qi+1];
    /*If we have a lower limit on the QI range, promote and fragments with a
       smaller QI, to ensure they're counted.*/
    if(qi_min[fti]>0)nqis[qi_min[fti]]=nqis[0];
    /*Select our first quantizer.*/
    for(qi0=qi_max[fti]+1;qi0-->qi_min[fti]&&nqis[qi0]<=0;);
    for(qi1=qi0-1;qi1>=qi_min[fti]&&nqis[qi1]<=nqis[qi0];qi1--);
    /*Test to make sure there are even two unique quantizers.*/
    if(qi1>=qi_min[fti]){
      ogg_int64_t best_metric;
      ogg_int64_t metric;
      int         best_qi1;
      int         best_qi2;
      int         qii;
      for(qi2=qi1-1;qi2>=qi_min[fti]&&nqis[qi2]<=nqis[qi1];qi2--);
      /*Test to make sure there are three unique quantizers.*/
      if(qi2>=0){
        best_metric=(ogg_int64_t)(nqis[0]-nqis[qi2+1])*
         (nqis[qi2+1]-nqis[qi1+1])*nqis[qi1+1];
        best_qi1=qi1;
        best_qi2=qi2;
        for(;nqis[qi1]<nqis[1];qi1--){
          for(qi2=qi1-1;nqis[qi2]<nqis[0];qi2--){
            metric=(ogg_int64_t)(nqis[0]-nqis[qi2+1])*
             (nqis[qi2+1]-nqis[qi1+1])*nqis[qi1+1];
            if(metric>=best_metric){
              best_qi1=qi1;
              best_qi2=qi2;
              best_metric=metric;
            }
          }
        }
        _enc->qis[fti][0]=qi0;
        _enc->qis[fti][1]=best_qi1;
        _enc->qis[fti][2]=best_qi2;
        _enc->nqis[fti]=3;
      }
      else{
        best_metric=(ogg_int64_t)(nqis[0]-nqis[qi1+1])*nqis[qi1+1];
        best_qi1=qi1;
        if(qi1>0)for(qi1--;nqis[qi1]<nqis[0];qi1--){
          metric=(ogg_int64_t)(nqis[0]-nqis[qi1+1])*nqis[qi1+1];
          if(metric>best_metric){
            best_qi1=qi1;
            best_metric=metric;
          }
        }
        _enc->qis[fti][0]=qi0;
        _enc->qis[fti][1]=best_qi1;
        _enc->nqis[fti]=2;
      }
      /*Right now qis[0] is the largest.
        We want to use the smallest that is still large enough for our DC
         coefficients.*/
      for(qii=1;qii<_enc->nqis[fti];qii++)if(_enc->qis[fti][qii]>=dc_qi[fti]){
        qi0=_enc->qis[fti][0];
        _enc->qis[fti][0]=_enc->qis[fti][qii];
        _enc->qis[fti][qii]=qi0;
      }
    }
    else{
      _enc->qis[fti][0]=qi0;
      _enc->nqis[fti]=1;
    }
    /*If we're in VP3 compatibility mode, just use the first quantizer.*/
    if(_enc->vp3_compatible)_enc->nqis[fti]=1;
  }
}

/*Mark all fragments as coded and in OC_MODE_INTRA.
  This also selects a quantizer value for each fragment and builds up the
   coded fragment list (in coded order) and clears the uncoded fragment list.
  It does not update the coded macro block list, as that is not used when
   coding INTRA frames.*/
static void oc_enc_vbr_mark_all_intra(oc_enc_ctx *_enc){
  oc_sb *sb;
  oc_sb *sb_end;
  int    pli;
  int    qii;
  int    ncoded_fragis;
  int    prev_ncoded_fragis;
  /*Select the quantizer list for INTRA frames.*/
  _enc->state.nqis=_enc->nqis[OC_INTRA_FRAME];
  for(qii=0;qii<_enc->state.nqis;qii++){
    _enc->state.qis[qii]=_enc->qis[OC_INTRA_FRAME][qii];
  }
  prev_ncoded_fragis=ncoded_fragis=0;
  sb=sb_end=_enc->state.sbs;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    fplane=_enc->state.fplanes+pli;
    sb_end+=fplane->nsbs;
    for(;sb<sb_end;sb++){
      int quadi;
      for(quadi=0;quadi<4;quadi++)if(sb->quad_valid&1<<quadi){
        int bi;
        for(bi=0;bi<4;bi++)if(sb->map[quadi][bi]>=0){
          oc_fragment_enc_info *efrag;
          oc_fragment          *frag;
          int                   fragi;
          int                   best_qii;
          fragi=sb->map[quadi][bi];
          frag=_enc->state.frags+fragi;
          frag->coded=1;
          frag->mbmode=OC_MODE_INTRA;
          efrag=_enc->frinfo+fragi;
          best_qii=0;
          for(qii=1;qii<_enc->state.nqis;qii++){
            if(efrag->qi_min[0]<=_enc->state.qis[qii]&&
             (_enc->state.qis[best_qii]<efrag->qi_min[0]||
             _enc->state.qis[qii]<_enc->state.qis[best_qii])){
              best_qii=qii;
            }
          }
          efrag->qii=(unsigned char)best_qii;
          frag->qi=_enc->state.qis[best_qii];
          _enc->state.coded_fragis[ncoded_fragis++]=fragi;
#if defined(OC_BITRATE_STATS)
          /*Compute the error function used for intra mode fragments.
            This function can only use information known at mode decision time, and
             so excludes the DC component.
            TODO: Separate this out somewhere more useful.*/
          {
            oc_fragment_enc_info *efrag;
            int                   ci;
            int                   eerror;
            efrag=_enc->frinfo+fragi;
            eerror=0;
            for(ci=1;ci<64;ci++)eerror+=abs(efrag->dct_coeffs[ci]);
            efrag->eerror=eerror;
          }
#endif
        }
      }
    }
    _enc->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
    _enc->state.nuncoded_fragis[pli]=0;
  }
  _enc->ncoded_frags=ncoded_fragis;
}



/*Quantize and predict the DC coefficients.
  This is done in a separate step because the prediction of DC coefficients
   occurs in image order, not in the Hilbert-curve order, unlike the rest of
   the encoding process.*/
static void oc_enc_vbr_quant_dc(oc_enc_ctx *_enc){
  oc_fragment_enc_info *efrag;
  oc_fragment          *frag;
  int                   pli;
  frag=_enc->state.frags;
  efrag=_enc->frinfo;
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
    fplane=_enc->state.fplanes+pli;
    for(fragy=0;fragy<fplane->nvfrags;fragy++){
      for(fragx=0;fragx<fplane->nhfrags;fragx++,frag++,efrag++){
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
            For more details, see oc_enc_vbr_frag_quant_tokenize().*/
          qti=frag->mbmode!=OC_MODE_INTRA;
          iquant=_enc->state.dequant_tables[qti][pli][_enc->state.qis[0]][0];
          c=efrag->dct_coeffs[0]-qc_pred*iquant;
          c_abs=abs(c);
          if(c_abs<=efrag->tols[0])qc=0;
          else{
            int qc_signed[2];
            int qc_max;
            int qc_min;
            int qc_offs;
            int c_sign;
            int c_min;
            int c_recon;
            int cati;
            fquant=_enc->enquant_tables[qti][pli][_enc->state.qis[0]][0];
            qc_max=(ogg_int32_t)c_abs*fquant+OC_FQUANT_ROUND>>OC_FQUANT_SHIFT;
            c_sign=c<0;
            c_recon=(qc_max-1)*iquant;
            c_min=OC_MAXI(0,c_abs-efrag->tols[0]);
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
            qc_signed[0]=qc;
            qc_signed[1]=-qc;
            qc=qc_signed[c_sign];
          }
        }
        pred_last[OC_FRAME_FOR_MODE[frag->mbmode]]=frag->dc=qc+qc_pred;
        efrag->dct_coeffs[0]=(ogg_int16_t)qc;
      }
    }
  }
}

/*Quantize and tokenize the given fragment.
  _efrag:   The encoder information for the fragment to quantize.
  _qcoeffs: The quantized coefficients, in zig-zag order.
  _fquant:  The forward quantization matrix to use.
  _iquant:  The inverse quantization matrix to use.
  Return: The number of coefficients before any final zero run.*/
static int oc_enc_vbr_frag_quant_tokenize(oc_enc_ctx *_enc,
 oc_fragment_enc_info *_efrag,ogg_int16_t _qcoeffs[64],
 const ogg_uint16_t _fquant[64],const ogg_uint16_t _iquant[64]){
  int zzi;
  int zrun;
  int qc;
  int qc_offs;
  int c_sign;
  int cati;
  int tli;
  /*The DC coefficient is already quantized (it had to be for DC prediction).
    Here we just tokenize it.*/
  if(_efrag->dct_coeffs[0]){
    qc=abs(_efrag->dct_coeffs[0]);
    c_sign=_efrag->dct_coeffs[0]<0;
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
    int qc_signed[2];
    int qc_max;
    int qc_min;
    int c_sign;
    int c_abs;
    int c_min;
    int c_recon;
    int ci;
    ci=OC_FZIG_ZAG[zzi];
    c_abs=abs(_efrag->dct_coeffs[ci]);
    /*Best case: we can encode this as a zero.*/
    if(c_abs<=_efrag->tols[ci]){
      zrun++;
      _qcoeffs[zzi]=0;
    }
    else{
      c_sign=_efrag->dct_coeffs[ci]<0;
      /*qc_max is the most accurate quantized value.
        This is the largest possible (absolute) value we will use.*/
      qc_max=(ogg_int32_t)c_abs*_fquant[ci]+OC_FQUANT_ROUND>>OC_FQUANT_SHIFT;
      /*qc_min is the smallest possible (by absolute value) quantized value
         whose dequantized value is within the HVS-determined tolerance
         range.*/
      /*TODO: qc_min could be computed by a division (we do not want to allow
         the rounding errors that are possible with the mul+shift quantization
         used for qc_max), which would allow qc_max to be calculated only if
         needed below.
        Is this faster?
        Who knows.*/
      c_recon=(qc_max-1)*_iquant[ci];
      c_min=c_abs-_efrag->tols[ci];
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
        _qcoeffs[zzi]=0;
      }
      else{
        /*If we have an outstanding zero run, code it now.*/
        if(zrun>0){
          /*The zero run tokens appear on the list for the first zero in the
             run.*/
          tli=zzi-zrun;
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
            qc_signed[0]=1;
            qc_signed[1]=-1;
            _qcoeffs[zzi]=(ogg_int16_t)qc_signed[c_sign];
            zrun=0;
            /*Skip coding the DCT value below.*/
            continue;
          }
          /*CAT2 run/value tokens: the value is 2-3.*/
          else if(qc_min<=3&&zrun<=3){
            if(zrun==1){
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_RUN_CAT2A;
              qc=OC_MINI(3,qc_max);
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)((c_sign<<1)+qc-2);
            }
            else{
              _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=
               OC_DCT_RUN_CAT2B;
              qc=OC_MINI(3,qc_max);
              _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
               (ogg_uint16_t)((c_sign<<2)+(qc-2<<1)+zrun-2);
            }
            qc_signed[0]=qc;
            qc_signed[1]=-qc;
            _qcoeffs[zzi]=(ogg_int16_t)qc_signed[c_sign];
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
        switch(qc_min){
          case 1:{
            _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
             (unsigned char)(OC_ONE_TOKEN+c_sign);
            _qcoeffs[zzi]=(ogg_int16_t)((-c_sign<<1)+1);
          }break;
          case 2:{
            _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
             (unsigned char)(OC_TWO_TOKEN+c_sign);
            _qcoeffs[zzi]=(ogg_int16_t)((-c_sign<<2)+2);
          }break;
          default:{
            if(qc_min-3<OC_NDCT_VAL_CAT2_SIZE){
              _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
               (unsigned char)(OC_DCT_VAL_CAT2+qc_min-3);
              _enc->extra_bits[zzi][_enc->nextra_bits[zzi]++]=
               (ogg_uint16_t)c_sign;
              qc_signed[0]=qc_min;
              qc_signed[1]=-qc_min;
              _qcoeffs[zzi]=(ogg_int16_t)qc_signed[c_sign];
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
              qc_signed[0]=qc;
              qc_signed[1]=-qc;
              _qcoeffs[zzi]=(ogg_int16_t)qc_signed[c_sign];
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
             go to 4095 (unless we want to change the spec to deal with
             wrap-around).*/
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
        _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=OC_DCT_EOB1_TOKEN;
      }
    }
  }
  /*Return the number of coefficients before the final zero run.*/
  return 64-zrun;
}

static void oc_enc_vbr_residual_tokenize(oc_enc_ctx *_enc){
  int *coded_fragi;
  int *coded_fragi_end;
  int    pli;
  int    zzi;
  /*Clear any existing DCT tokens.*/
  for(zzi=0;zzi<64;zzi++){
    _enc->ndct_tokens[zzi]=_enc->nextra_bits[zzi]=0;
    _enc->extra_bits_offs[zzi]=0;
  }
  coded_fragi_end=coded_fragi=_enc->state.coded_fragis;
  for(pli=0;pli<3;pli++){
    memcpy(_enc->dct_token_offs[pli],_enc->ndct_tokens,
     sizeof(_enc->dct_token_offs[pli]));
    coded_fragi_end+=_enc->state.ncoded_fragis[pli];
    for(;coded_fragi<coded_fragi_end;coded_fragi++){
      oc_quant_table       *iquants;
      oc_fragment          *frag;
      oc_fragment_enc_info *efrag;
      ogg_int16_t           qcoeffs[64];
      int                   fragi;
      int                   qti;
      int                   nnzc;
      fragi=*coded_fragi;
      frag=_enc->state.frags+fragi;
      efrag=_enc->frinfo+fragi;
      qti=frag->mbmode!=OC_MODE_INTRA;
      iquants=_enc->state.dequant_tables[qti][pli];
      nnzc=oc_enc_vbr_frag_quant_tokenize(_enc,efrag,qcoeffs,
       _enc->enquant_tables[qti][pli][frag->qi],iquants[frag->qi]);
      /*While we're here and things are in cache, reconstruct the quantized
         fragment.*/
      oc_state_frag_recon(&_enc->state,frag,pli,qcoeffs,nnzc,nnzc,
       iquants[_enc->state.qis[0]][0],iquants[frag->qi]);
    }
  }
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
    zzj=zzi-1;
    do{
      toki=_enc->ndct_tokens[zzj]-1;
      if(toki>=_enc->dct_token_offs[0][zzj]){
        old_tok1=_enc->dct_tokens[zzj][toki];
        break;
      }
    }
    while(zzj-->0);
    /*Ensure its last token was an EOB run.*/
    if(old_tok1>=OC_NDCT_EOB_TOKEN_MAX)continue;
    /*Pull off the associated extra bits, if any, and decode the runs.*/
    ebi=_enc->nextra_bits[zzj];
    old_eb1=OC_DCT_TOKEN_EXTRA_BITS[old_tok1]?_enc->extra_bits[zzj][--ebi]:0;
    old_eb2=OC_DCT_TOKEN_EXTRA_BITS[old_tok2]?_enc->extra_bits[zzi][0]:0;
    runl=OC_EOB_OFFS[old_tok1]+old_eb1+OC_EOB_OFFS[old_tok2]+old_eb2;
    /*We can't possibly combine these into one run.
      It might be possible to split them more optimally, but we'll just leave
       them as is.*/
    if(runl>=4096)continue;
    /*We CAN combine them into one run.*/
    for(new_tok=OC_DCT_EOB1_TOKEN;
     runl-OC_EOB_OFFS[new_tok]>=OC_EOB_RANGE[new_tok];new_tok++);
    /*toki is always initialized.
      If your compiler thinks otherwise, it is dumb.*/
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

/*Marks each fragment as coded or not, based on the coefficient-level
   thresholds computed in the psychovisual stage.
  The MB mode of the fragments are not set, as they will be computed in
   oc_enc_choose_mbmodes().
  This also builds up the coded fragment and uncoded fragment lists.
  The coded MB list is not built up.
  That is done during mode decision.*/
static void oc_enc_vbr_mark_coded(oc_enc_ctx *_enc){
  oc_sb *sb;
  oc_sb *sb_end;
  int    pli;
  int    bli;
  int    ncoded_fragis;
  int    prev_ncoded_fragis;
  int    nuncoded_fragis;
  int    prev_nuncoded_fragis;
  _enc->nblock_coded_flags=bli=0;
  prev_ncoded_fragis=ncoded_fragis=prev_nuncoded_fragis=nuncoded_fragis=0;
  sb=sb_end=_enc->state.sbs;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    int                      ystride;
    int                      prev_refi;
    fplane=_enc->state.fplanes+pli;
    sb_end+=fplane->nsbs;
    prev_refi=_enc->state.ref_frame_idx[OC_FRAME_PREV];
    ystride=_enc->state.ref_frame_bufs[prev_refi][pli].ystride;
    for(;sb<sb_end;sb++){
      int quadi;
      sb->coded_fully=1;
      sb->coded_partially=0;
      for(quadi=0;quadi<4;quadi++)if(sb->quad_valid&1<<quadi){
        int bi;
        for(bi=0;bi<4;bi++){
          int fragi;
          fragi=sb->map[quadi][bi];
          if(fragi>=0){
            oc_fragment *frag;
            int          flag;
            frag=_enc->state.frags+fragi;
            if(frag->invalid){
              frag->coded=0;
              *(_enc->state.uncoded_fragis-++nuncoded_fragis)=fragi;
            }
            else{
              oc_fragment_enc_info *efrag;
              ogg_int16_t           dct_buf[64];
              int                   ci;
              /*Check to see if the fragment can be skipped.
                It is assumed that a skipped fragment always takes fewer bits
                 than a coded fragment, though this may not necessarily be true.
                A single skipped fragment could take up to 34 bits to encode
                 its location in the RLE scheme Theora uses */
              oc_frag_intra_fdct(frag,dct_buf,ystride,prev_refi);
              efrag=_enc->frinfo+fragi;
              /*The comparison against OC_DC_QUANT_MIN and OC_AC_QUANT_MIN
                 ensures we mark a fragment as skipped if it would be quantized
                 to all zeros in OC_MODE_INTER_NOMV.
                These minimum quantizers represent the maximum quality the
                 format is capable of, and can be larger than our tolerances.
                The minimum for INTER modes is twice the minimum for INTRA
                 modes, so technically if the tolerances are below this
                 threshold, we might be able to do a better job representing
                 this fragment by coding it in INTRA mode.
                But the number of extra bits required to do that would be
                 ridiculous, so we give up our devotion to minimum quality just
                 this once.

                Note: OC_DC_QUANT_MIN[0] should actually be
                 OC_DC_QUANT_MIN[1]>>1, but in this case those are
                 equivalent.*/
              ci=0;
              if((unsigned)abs(dct_buf[0]-efrag->dct_coeffs[0])<=
               OC_MAXI(efrag->tols[0],OC_DC_QUANT_MIN[0])){
                for(ci++;ci<64;ci++){
                  if((unsigned)abs(dct_buf[ci]-efrag->dct_coeffs[ci])>
                   OC_MAXI(efrag->tols[ci],OC_AC_QUANT_MIN[0])){
                    break;
                  }
                }
              }
              if(ci>=64){
                frag->coded=0;
                *(_enc->state.uncoded_fragis-++nuncoded_fragis)=fragi;
              }
              else{
                frag->coded=1;
                _enc->state.coded_fragis[ncoded_fragis++]=fragi;
              }
            }
            flag=frag->coded;
            sb->coded_fully&=flag;
            sb->coded_partially|=flag;
            _enc->block_coded_flags[bli++]=(char)flag;
          }
        }
      }
      /*If this is a partially coded super block, keep the entries just added
         to the code block flag list.*/
      if(!sb->coded_fully&&sb->coded_partially){
        _enc->nblock_coded_flags=bli;
      }
      /*Otherwise, discard these entries from the list, as they are
         implicit.*/
      else{
        sb->coded_partially=0;
        bli=_enc->nblock_coded_flags;
      }
    }
    _enc->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
    _enc->state.nuncoded_fragis[pli]=nuncoded_fragis-prev_nuncoded_fragis;
    prev_nuncoded_fragis=nuncoded_fragis;
  }
  _enc->ncoded_frags=ncoded_fragis;
}

/*Selects an appropriate coding mode for each macro block.
  A mode is chosen for the macro blocks with at least one coded fragment.
  A bit cost estimate for coding the frame with the selected modes is made,
   and a similar estimate is made for coding the frame as a key frame.
  These estimates are used to select the optimal frame type.
  Return: The frame type to encode with: OC_INTER_FRAME or OC_INTRA_FRAME.*/
static int oc_enc_choose_mbmodes(oc_enc_ctx *_enc){
  oc_set_chroma_mvs_func  set_chroma_mvs;
  oc_fragment_enc_info   *efrag;
  oc_fragment            *frag;
  oc_mb                  *mb;
  oc_mb_enc_info         *mbinfo;
  oc_mv                   last_mv[2];
  int                    *uncoded_fragi;
  int                    *uncoded_fragi_end;
  int                     best_qii;
  int                     qii;
  int                     qi;
  int                     pli;
  int                     mbi;
  int                     fragi;
  int                     ci;
  int                     nmbs;
  int                     mvbitsa;
  int                     mvbitsb;
  int                     intra_bits;
  int                     inter_bits;
  nmbs=_enc->state.nmbs;
  set_chroma_mvs=OC_SET_CHROMA_MVS_TABLE[_enc->state.info.pixel_fmt];
  oc_mode_scheme_chooser_reset(&_enc->mode_scheme_chooser);
  memset(last_mv,0,sizeof(last_mv));
  mbinfo=_enc->mbinfo;
  mvbitsa=mvbitsb=0;
  inter_bits=2+7*_enc->state.nqis-(_enc->state.nqis==3);
  intra_bits=inter_bits+3;
  _enc->state.ncoded_mbis=0;
  for(mbi=0;mbi<nmbs;mbi++){
    mb=_enc->state.mbs+mbi;
    if(mb->mode!=OC_MODE_INVALID){
      oc_fragment_enc_info *efrag;
      oc_mv                 bmvs[2][4];
      oc_mv                 mbmv;
      int                   err[OC_NMODES][12];
      int                   bits[OC_NMODES];
      int                   coded[13];
      int                   frag_qii[12][2][2];
      int                   ncoded;
      int                   ncoded_luma;
      int                   mapii;
      int                   mapi;
      int                   modei;
      int                   codedi;
      int                   mbintrabits;
      int                   mbpmvbitsa;
      int                   mbgmvbitsa;
      int                   mb4mvbitsa;
      int                   mb4mvbitsb;
      int                   mode;
      int                   fti;
      int                   qti;
      int                   bi;
      mbinfo=_enc->mbinfo+mbi;
      /*Build up a list of coded fragments.*/
      ncoded=0;
      for(mapii=0;mapii<OC_MB_MAP_NIDXS[_enc->state.info.pixel_fmt];mapii++){
        mapi=OC_MB_MAP_IDXS[_enc->state.info.pixel_fmt][mapii];
        fragi=mb->map[mapi>>2][mapi&3];
        if(fragi>=0&&_enc->state.frags[fragi].coded)coded[ncoded++]=mapi;
      }
      /*If we don't find any, mark this MB not coded and move on.*/
      if(ncoded<=0){
        mb->mode=OC_MODE_NOT_CODED;
        /*Don't bother to do a MV search against the golden frame.
          Just re-use the last vector, which should match well since the
           contents of the MB haven't changed much.*/
        mbinfo->mvs[0][OC_FRAME_GOLD][0]=mbinfo->mvs[1][OC_FRAME_GOLD][0];
        mbinfo->mvs[0][OC_FRAME_GOLD][1]=mbinfo->mvs[1][OC_FRAME_GOLD][1];
        continue;
      }
      /*Count the number of coded blocks that are luma blocks, and replace the
         block MVs for not-coded blocks with (0,0).*/
      memcpy(bmvs[0],mbinfo->bmvs,sizeof(bmvs[0]));
      /*Mark the end of the list so we don't go past it below.*/
      coded[ncoded]=-1;
      for(mapi=ncoded_luma=0;mapi<4;mapi++){
        if(coded[ncoded_luma]==mapi)ncoded_luma++;
        else bmvs[0][mapi][0]=bmvs[0][mapi][1]=0;
      }
      /*Select a qi value for each coded fragment for each frame type and
         quantizer type.*/
      for(codedi=0;codedi<ncoded;codedi++){
        mapi=coded[codedi];
        efrag=_enc->frinfo+mb->map[mapi>>2][mapi&3];
        for(fti=0;fti<2;fti++)for(qti=0;qti<=fti;qti++){
          best_qii=0;
          for(qii=1;qii<_enc->nqis[fti];qii++){
            if(efrag->qi_min[qti]<=_enc->qis[fti][qii]&&
             (_enc->qis[fti][qii]<_enc->qis[fti][best_qii]||
             _enc->qis[fti][best_qii]<efrag->qi_min[qti])){
              best_qii=qii;
            }
          }
          frag_qii[codedi][fti][qti]=best_qii;
        }
      }
      /*Special case: If no luma blocks are coded, but some chroma blocks are,
         then the macro block defaults to OC_MODE_INTER_NOMV, and no mode need
         be explicitly coded for it.*/
      if(ncoded_luma<=0){
        mb->mode=OC_MODE_NOT_CODED;
        /*Don't bother to do a MV search against the golden frame.*/
        mbinfo->mvs[0][OC_FRAME_GOLD][0]=mbinfo->mvs[0][OC_FRAME_GOLD][1]=0;
        /*We do collect bitrate stats for frame type decision.*/
        mbintrabits=bits[OC_MODE_INTER_NOMV]=0;
        for(codedi=0;codedi<ncoded;codedi++){
          mapi=coded[codedi];
          pli=mapi>>2;
          fragi=mb->map[pli][mapi&3];
          frag=_enc->state.frags+fragi;
          efrag=_enc->frinfo+fragi;
          /*Set the MB mode and MV in the fragment.*/
          frag->mbmode=OC_MODE_INTER_NOMV;
          frag->mv[0]=frag->mv[1]=0;
          /*Calculate the bitrate estimates.*/
          err[OC_MODE_INTRA][mapi]=0;
          for(ci=1;ci<64;ci++){
            err[OC_MODE_INTRA][mapi]+=abs(efrag->dct_coeffs[ci]);
          }
          err[OC_MODE_INTER_NOMV][mapi]=oc_enc_frag_sad(_enc,frag,0,0,pli,
           OC_FRAME_PREV);
          qi=_enc->qis[OC_INTRA_FRAME][frag_qii[codedi][OC_INTRA_FRAME][0]];
          mbintrabits+=OC_RES_BITRATES[qi][pli][OC_MODE_INTRA][
           OC_MINI(err[OC_MODE_INTRA][mapi]>>8,15)];
          qi=_enc->qis[OC_INTER_FRAME][frag_qii[codedi][OC_INTER_FRAME][1]];
          bits[OC_MODE_INTER_NOMV]+=OC_RES_BITRATES[qi][pli][
           OC_MODE_INTER_NOMV][OC_MINI(err[OC_MODE_INTER_NOMV][mapi]>>6,15)];
          /*Also mark this fragment with the selected INTER qi.
            It will be reset if we eventually code this as an INTRA frame.*/
#if defined(OC_BITRATE_STATS)
          efrag->eerror=err[OC_MODE_INTER_NOMV][mapi];
#endif
          efrag->qii=(unsigned char)frag_qii[codedi][OC_INTER_FRAME][1];
          frag->qi=qi;
        }
        intra_bits+=mbintrabits+(1<<OC_BIT_SCALE-1)>>OC_BIT_SCALE;
        inter_bits+=bits[OC_MODE_INTER_NOMV]+(1<<OC_BIT_SCALE-1)>>OC_BIT_SCALE;
        continue;
      }
      /*Otherwise, add this to the coded MB list.*/
      _enc->state.coded_mbis[_enc->state.ncoded_mbis++]=mbi;
      /*Compute the chroma MVs for the 4MV mode.*/
      (*set_chroma_mvs)(bmvs[1],(const oc_mv *)bmvs[0]);
      /*Do a MV search against the golden frame.*/
      oc_mcenc_search_1mv(_enc->mcenc,mb-_enc->state.mbs,OC_FRAME_GOLD);
      /*We are now ready to do mode decision for this macro block.
        Mode decision is done by exhaustively examining all potential choices.
        Since we use a minimum-quality encoding strategy, this amounts to
         simply selecting the mode which uses the smallest number of bits,
         since the minimum quality will be met in any mode.
        Obviously, doing the motion compensation, fDCT, tokenization, and then
         counting the bits each token uses is computationally expensive.
        Theora's EOB runs can also split the cost of these tokens across
         multiple fragments, and naturally we don't know what the optimal
         choice of Huffman codes will be until we know all the tokens we're
         going to encode in all the fragments.

        So we use a simple approach to estimating the bit cost of each mode
         based upon the SAD value of the residual.
        The mathematics behind the technique are outlined by Kim \cite{Kim03},
         but the process is very simple.
        For each quality index and SAD value, we have a table containing the
         average number of bits needed to code a fragment.
        The SAD values are placed into a small number of bins (currently 16).
        The bit counts are obtained by examining actual encoded frames, with
         optimal Huffman codes selected and EOB bits appropriately divided
         among all the blocks they involve.
        A separate QIxSAD table is kept for each mode and color plane.
        It may be possible to combine many of these, but only experimentation
         will tell which ones truly represent the same distribution.

        @ARTICLE{Kim03,
          author="Hyun Mun Kim",
          title="Adaptive Rate Control Using Nonlinear Regression",
          journal="IEEE Transactions on Circuits and Systems for Video
           Technology",
          volume=13,
          number=5,
          pages="432--439",
          month="May",
          year=2003
        }*/
      memset(bits,0,sizeof(bits));
      mbintrabits=0;
      /*Find the SAD values for each coded fragment for each possible mode.*/
      for(codedi=0;codedi<ncoded;codedi++){
        mapi=coded[codedi];
        pli=mapi>>2;
        bi=mapi&3;
        fragi=mb->map[pli][bi];
        frag=_enc->state.frags+fragi;
        efrag=_enc->frinfo+fragi;
        err[OC_MODE_INTRA][mapi]=0;
        for(ci=1;ci<64;ci++){
          err[OC_MODE_INTRA][mapi]+=abs(efrag->dct_coeffs[ci]);
        }
        err[OC_MODE_INTER_NOMV][mapi]=oc_enc_frag_sad(_enc,frag,0,0,pli,
         OC_FRAME_PREV);
        err[OC_MODE_INTER_MV][mapi]=oc_enc_frag_sad(_enc,frag,
         mbinfo->mvs[0][OC_FRAME_PREV][0],mbinfo->mvs[0][OC_FRAME_PREV][1],
         pli,OC_FRAME_PREV);
        err[OC_MODE_INTER_MV_LAST][mapi]=oc_enc_frag_sad(_enc,frag,
         last_mv[0][0],last_mv[0][1],pli,OC_FRAME_PREV);
        err[OC_MODE_INTER_MV_LAST2][mapi]=oc_enc_frag_sad(_enc,frag,
         last_mv[1][0],last_mv[1][1],pli,OC_FRAME_PREV);
        err[OC_MODE_INTER_MV_FOUR][mapi]=oc_enc_frag_sad(_enc,frag,
         bmvs[!!pli][bi][0],bmvs[!!pli][bi][1],pli,OC_FRAME_PREV);
        err[OC_MODE_GOLDEN_NOMV][mapi]=oc_enc_frag_sad(_enc,frag,
         0,0,pli,OC_FRAME_GOLD);
        err[OC_MODE_GOLDEN_MV][mapi]=oc_enc_frag_sad(_enc,frag,
         mbinfo->mvs[0][OC_FRAME_GOLD][0],mbinfo->mvs[0][OC_FRAME_GOLD][1],
         pli,OC_FRAME_GOLD);
        /*Using these distortion values, estimate the number of bits needed to
           code this fragment in each mode.*/
        qi=_enc->qis[OC_INTRA_FRAME][frag_qii[codedi][OC_INTRA_FRAME][0]];
        mbintrabits+=OC_RES_BITRATES[qi][pli][OC_MODE_INTRA][
         OC_MINI(err[OC_MODE_INTRA][mapi]>>8,15)];
        qi=_enc->qis[OC_INTER_FRAME][frag_qii[codedi][OC_INTER_FRAME][0]];
        bits[OC_MODE_INTRA]+=OC_RES_BITRATES[qi][pli][OC_MODE_INTRA][
         OC_MINI(err[OC_MODE_INTRA][mapi]>>8,15)];
        qi=_enc->qis[OC_INTER_FRAME][frag_qii[codedi][OC_INTER_FRAME][1]];
        for(modei=OC_MODE_INTRA+1;modei<OC_NMODES;modei++){
          bits[modei]+=OC_RES_BITRATES[qi][pli][modei][
           OC_MINI(err[modei][mapi]>>6,15)];
        }
      }
      /*Bit costs are stored in the table with extra precision.
        Round them down to whole bits here.*/
      for(modei=0;modei<OC_NMODES;modei++){
        bits[modei]=bits[modei]+(1<<OC_BIT_SCALE-1)>>OC_BIT_SCALE;
      }
      /*Estimate the cost of coding the label for each mode.
        See comments at oc_mode_scheme_chooser_cost() for a description of the
         method.*/
      for(modei=0;modei<OC_NMODES;modei++){
        bits[modei]+=oc_mode_scheme_chooser_cost(&_enc->mode_scheme_chooser,
         modei);
      }
      /*Add the motion vector bits for each mode that requires them.*/
      mbpmvbitsa=oc_mvbitsa(mbinfo->mvs[0][OC_FRAME_PREV][0],
       mbinfo->mvs[0][OC_FRAME_PREV][1]);
      mbgmvbitsa=oc_mvbitsa(mbinfo->mvs[1][OC_FRAME_GOLD][0],
       mbinfo->mvs[0][OC_FRAME_GOLD][1]);
      mb4mvbitsa=mb4mvbitsb=0;
      for(codedi=0;codedi<ncoded_luma;codedi++){
        mb4mvbitsa=oc_mvbitsa(bmvs[0][coded[codedi]][0],
         bmvs[0][coded[codedi]][1]);
        mb4mvbitsb+=12;
      }
      /*We use the same opportunity cost method of estimating the cost of
         coding the motion vectors with the two different schemes as we do for
         estimating the cost of the mode labels.
        However, because there are only two schemes and they're both pretty
         simple, this can just be done inline.*/
      bits[OC_MODE_INTER_MV]+=OC_MINI(mvbitsa+mbpmvbitsa,mvbitsb+12)-
       OC_MINI(mvbitsa,mvbitsb);
      bits[OC_MODE_GOLDEN_MV]+=OC_MINI(mvbitsa+mbgmvbitsa,mvbitsb+12)-
       OC_MINI(mvbitsa,mvbitsb);
      bits[OC_MODE_INTER_MV_FOUR]+=OC_MINI(mvbitsa+mb4mvbitsa,
       mvbitsb+mb4mvbitsb)-OC_MINI(mvbitsa,mvbitsb);
      /*Finally, pick the mode with the cheapest estimated bit cost.*/
      mode=0;
      for(modei=1;modei<OC_NMODES;modei++)if(bits[modei]<bits[mode]){
        /*Do not select 4MV mode when not all the luma blocks are coded when
           we're in VP3 compatibility mode.*/
        if(_enc->vp3_compatible&&modei==OC_MODE_INTER_MV_FOUR&&ncoded_luma<4){
          continue;
        }
        mode=modei;
      }
#if defined(OC_BITRATE_STATS)
      /*Remember the error for the mode we selected in each fragment.*/
      for(codedi=0;codedi<ncoded;codedi++){
        mapi=coded[codedi];
        fragi=mb->map[mapi>>2][mapi&3];
        efrag=_enc->frinfo+fragi;
        efrag->eerror=err[mode][mapi];
      }
#endif
      /*Go back and store the selected qi index corresponding to the selected
         mode in each fragment.*/
      for(codedi=0;codedi<ncoded;codedi++){
        mapi=coded[codedi];
        fragi=mb->map[mapi>>2][mapi&3];
        frag=_enc->state.frags+fragi;
        efrag=_enc->frinfo+fragi;
        efrag->qii=(unsigned char)
         frag_qii[codedi][OC_INTER_FRAME][mode!=0];
        frag->qi=_enc->qis[OC_INTER_FRAME][efrag->qii];
      }
      inter_bits+=bits[mode];
      intra_bits+=mbintrabits+(1<<OC_BIT_SCALE-1)>>OC_BIT_SCALE;
      oc_mode_scheme_chooser_update(&_enc->mode_scheme_chooser,mode);
      mb->mode=mode;
      switch(mode){
        case OC_MODE_INTER_MV:{
          mvbitsa+=mbpmvbitsa;
          mvbitsb+=12;
          last_mv[1][0]=last_mv[0][0];
          last_mv[1][1]=last_mv[0][1];
          mbmv[0]=last_mv[0][0]=mbinfo->mvs[0][OC_FRAME_PREV][0];
          mbmv[1]=last_mv[0][1]=mbinfo->mvs[0][OC_FRAME_PREV][1];
        }break;
        case OC_MODE_INTER_MV_LAST:{
          mbmv[0]=last_mv[0][0];
          mbmv[1]=last_mv[0][1];
        }break;
        case OC_MODE_INTER_MV_LAST2:{
          mbmv[0]=last_mv[1][0];
          mbmv[1]=last_mv[1][1];
          last_mv[1][0]=last_mv[0][0];
          last_mv[1][1]=last_mv[0][1];
          last_mv[0][0]=mbmv[0];
          last_mv[0][1]=mbmv[1];
        }break;
        case OC_MODE_INTER_MV_FOUR:{
          mvbitsa+=mb4mvbitsa;
          mvbitsb+=mb4mvbitsb;
          if(ncoded_luma>0){
            /*After 4MV mode, the last MV is the one from the last coded luma
               block.*/
            last_mv[1][0]=last_mv[0][0];
            last_mv[1][1]=last_mv[0][1];
            last_mv[0][0]=bmvs[0][coded[ncoded_luma-1]][0];
            last_mv[0][1]=bmvs[0][coded[ncoded_luma-1]][1];
          }
        }break;
        case OC_MODE_GOLDEN_MV:{
          mvbitsa+=mbgmvbitsa;
          mvbitsb+=12;
          mbmv[0]=mbinfo->mvs[0][OC_FRAME_GOLD][0];
          mbmv[1]=mbinfo->mvs[0][OC_FRAME_GOLD][1];
        }break;
        default:mbmv[0]=mbmv[1]=0;break;
      }
      /*Special case 4MV mode.
        MVs are stored in bmvs.*/
      if(mode==OC_MODE_INTER_MV_FOUR){
        for(codedi=0;codedi<ncoded;codedi++){
          mapi=coded[codedi];
          pli=mapi>>2;
          bi=mapi&3;
          fragi=mb->map[pli][bi];
          frag=_enc->state.frags+fragi;
          frag->mbmode=mode;
          frag->mv[0]=bmvs[!!pli][bi][0];
          frag->mv[1]=bmvs[!!pli][bi][1];
        }
      }
      /*For every other mode, the MV is stored in mbmv.*/
      else{
        for(codedi=0;codedi<ncoded;codedi++){
          mapi=coded[codedi];
          fragi=mb->map[mapi>>2][mapi&3];
          frag=_enc->state.frags+fragi;
          frag->mbmode=mode;
          frag->mv[0]=mbmv[0];
          frag->mv[1]=mbmv[1];
        }
      }
    }
  }
  /*Finally, compare the cost of an INTER frame and an INTRA frame.*/
  if(mvbitsb<mvbitsa){
    _enc->mv_scheme=1;
    inter_bits+=mvbitsb;
  }
  else{
    _enc->mv_scheme=0;
    inter_bits+=mvbitsa;
  }
  inter_bits+=_enc->mode_scheme_chooser.scheme_bits[
   _enc->mode_scheme_chooser.scheme_list[0]];
  /*The easiest way to count the bits needed for coded/not coded fragments is
     to code them.
    We need to do this anyway, might as well do it now.*/
  oggpackB_reset(&_enc->opb_coded_flags);
  inter_bits+=oc_enc_partial_sb_flags_pack(_enc,&_enc->opb_coded_flags);
  inter_bits+=oc_enc_coded_sb_flags_pack(_enc,&_enc->opb_coded_flags);
  inter_bits+=oc_enc_coded_block_flags_pack(_enc,&_enc->opb_coded_flags);
  /*Select the quantizer list for INTER frames.*/
  _enc->state.nqis=_enc->nqis[OC_INTER_FRAME];
  for(qii=0;qii<_enc->state.nqis;qii++){
    _enc->state.qis[qii]=_enc->qis[OC_INTER_FRAME][qii];
  }
  if(intra_bits>inter_bits){
    _enc->vbr->est_bits=inter_bits;
    return OC_INTER_FRAME;
  }
  /*All INTRA mode is smaller, but we haven't counted up the cost of all the
     not coded fragments we will now have to code.*/
  uncoded_fragi_end=uncoded_fragi=_enc->state.uncoded_fragis;
  for(pli=0;pli<3;pli++){
    uncoded_fragi_end-=_enc->state.nuncoded_fragis[pli];
    while(uncoded_fragi-->uncoded_fragi_end){
      fragi=*uncoded_fragi;
      frag=_enc->state.frags+fragi;
      /*Assume a very small bit cost for invalid fragments.*/
      if(frag->invalid)intra_bits+=OC_RES_BITRATES[0][pli][OC_MODE_INTRA][0];
      else{
        int eerror;
        eerror=0;
        efrag=_enc->frinfo+fragi;
        for(ci=1;ci<64;ci++)eerror+=abs(efrag->dct_coeffs[ci]);
#if defined(OC_BITRATE_STATS)
        efrag->eerror=eerror;
#endif
        qi=_enc->qis[OC_INTRA_FRAME][0];
        for(qii=1;qii<_enc->nqis[OC_INTRA_FRAME];qii++){
          if(_enc->qis[OC_INTRA_FRAME][qii]<qi&&
           efrag->qi_min[0]<=_enc->qis[OC_INTRA_FRAME][qii]){
            qi=_enc->qis[OC_INTRA_FRAME][qii];
          }
        }
        intra_bits+=OC_RES_BITRATES[qi][pli][OC_MODE_INTRA][
         OC_MINI(eerror>>8,15)];
        /*If it turns out INTRA mode was more expensive, we're done.*/
        if(intra_bits>inter_bits){
          _enc->vbr->est_bits=inter_bits;
          return OC_INTER_FRAME;
        }
      }
    }
  }
  /*So, we've compared the full cost estimates, and INTRA is still better.
    Code an INTRA frame instead.*/
  oc_enc_vbr_mark_all_intra(_enc);
  _enc->vbr->est_bits=intra_bits;
  return OC_INTRA_FRAME;
}

/*A pipeline stage for transforming, quantizing, and tokenizing the frame.*/

static int oc_vbr_pipe_start(oc_enc_pipe_stage *_stage){
  int pli;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=0;
  return 0;
}

static int oc_vbr_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  int pli;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=_y_avail[pli];
  return 0;
}

static int oc_vbr_pipe_end(oc_enc_pipe_stage *_stage){
  oc_enc_ctx *enc;
  int         ret;
  enc=_stage->enc;
  if(enc->state.curframe_num==0||
   enc->state.curframe_num-enc->state.keyframe_num>=
   enc->keyframe_frequency_force){
    enc->state.frame_type=OC_INTRA_FRAME;
    oc_enc_vbr_quant_sel_quality(enc,1);
    oc_enc_vbr_mark_all_intra(enc);
  }
  else{
    oc_enc_vbr_mark_coded(enc);
    /*Only proceed if we have some coded blocks.
      No coded blocks -> dropped frame -> 0 byte packet.*/
    if(enc->ncoded_frags>0){
      oc_enc_vbr_quant_sel_quality(enc,0);
      enc->state.frame_type=oc_enc_choose_mbmodes(enc);
      if(enc->state.frame_type==OC_INTER_FRAME)oc_enc_do_inter_dcts(enc);
    }
  }
  /*Only initialize subsequent stages after we know how many fragments will be
     encoded, and at what quality (so the loop filter can be set up
     properly).*/
  if(_stage->next!=NULL){
    ret=(*_stage->next->pipe_start)(_stage->next);
    if(ret<0)return ret;
  }
  if(enc->ncoded_frags>0){
    /*TODO: These stages could be pipelined with reconstruction.*/
    oc_enc_vbr_quant_dc(enc);
    oc_enc_vbr_residual_tokenize(enc);
  }
  if(_stage->next!=NULL){
    ret=(*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
    if(ret<0)return ret;
    return (*_stage->next->pipe_end)(_stage->next);
  }
  return 0;
}

/*Initialize the transform, quantization, and tokenization stage of the
   pipeline.
  _enc: The encoding context.*/
static void oc_vbr_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_vbr_pipe_start;
  _stage->pipe_proc=oc_vbr_pipe_process;
  _stage->pipe_end=oc_vbr_pipe_end;
}


static int oc_enc_vbr_init(oc_enc_vbr_ctx *_vbr,oc_enc_ctx *_enc){
  _vbr->cfg.qi=_enc->state.info.quality;
  _vbr->cfg.kf_qi_min=_vbr->cfg.df_qi_min=0;
  _vbr->cfg.kf_qi_max=_vbr->cfg.df_qi_max=63;
  _vbr->enc=_enc;
  _vbr->impmap=oc_impmap_alloc(_enc);
  _vbr->psych=oc_psych_alloc(_enc);
  oc_vbr_pipe_init(&_vbr->pipe,_enc);
  return 0;
}

static void oc_enc_vbr_clear(oc_enc_vbr_ctx *_vbr){
  oc_psych_free(_vbr->psych);
  oc_impmap_free(_vbr->impmap);
}

static int oc_enc_vbr_cfg(oc_enc_vbr_ctx *_vbr,th_vbr_cfg *_cfg){
  if(_cfg->qi<0||_cfg->qi>63||_cfg->kf_qi_min<0||_cfg->kf_qi_min>63||
   _cfg->kf_qi_max<_cfg->kf_qi_min||_cfg->kf_qi_max>63||
   _cfg->df_qi_min<0||_cfg->df_qi_min>63||
   _cfg->df_qi_max<_cfg->df_qi_min||_cfg->df_qi_max>63){
    return TH_EINVAL;
  }
  memcpy(&_vbr->cfg,_cfg,sizeof(_vbr->cfg));
  return 0;
}

static oc_enc_pipe_stage *oc_enc_vbr_create_pipe(oc_enc_vbr_ctx *_vbr){
  oc_enc_pipe_stage *pipe;
  _vbr->enc->fill_pipe.next=&_vbr->enc->pack_pipe;
  _vbr->pipe.next=&_vbr->enc->copy_pipe;
  /*TODO: Disable spatial masking and CSF filtering based on
     application-specified speed level.*/
  pipe=oc_psych_prepend_to_pipe(_vbr->psych,&_vbr->pipe);
  _vbr->enc->fdct_pipe.next=pipe;
  /*TODO: Disable impmap based on application-specified speed level.*/
  pipe=oc_impmap_prepend_to_pipe(_vbr->impmap,&_vbr->enc->fdct_pipe);
  pipe=oc_mcenc_prepend_to_pipe(_vbr->enc->mcenc,pipe);
  return pipe;
}


oc_enc_vbr_ctx *oc_enc_vbr_alloc(oc_enc_ctx *_enc){
  oc_enc_vbr_ctx *vbr;
  vbr=(oc_enc_vbr_ctx *)_ogg_malloc(sizeof(*vbr));
  oc_enc_vbr_init(vbr,_enc);
  return vbr;
}

void oc_enc_vbr_free(oc_enc_vbr_ctx *_vbr){
  if(_vbr!=NULL){
    oc_enc_vbr_clear(_vbr);
    _ogg_free(_vbr);
  }
}

int oc_enc_vbr_enable(oc_enc_vbr_ctx *_vbr,th_vbr_cfg *_cfg){
  if(_cfg!=NULL){
    int ret;
    ret=oc_enc_vbr_cfg(_vbr,_cfg);
    if(ret<0)return ret;
  }
  /*Map the qi to a multiple of JND values.*/
  _vbr->qscale=_vbr->cfg.qi>=63?0.5F:1.5F*OC_POWF(2,0.0625F*(64-_vbr->cfg.qi));
  _vbr->enc->pipe=oc_enc_vbr_create_pipe(_vbr);
  /*TODO: Implement a real speed level.*/
  _vbr->enc->speed_max=0;
  _vbr->enc->set_speed=oc_enc_set_speed_null;
  return 0;
}
