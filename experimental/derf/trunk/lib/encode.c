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

/*Select a quantizer for the given quality.*/
static void oc_enc_quant_sel_quality(oc_enc_ctx *_enc,int _intra_only){
  int qi;
  int ci;
  /*This quantizer selection really bugs me.
    In order to guarantee a minimum error value, we must select a quantizer
     no larger than twice the smallest allowable error for each coefficient.
    In reality, most fragments will not require such a small quantizer, but
     Theora only supports per-frame quantizer selection.*/
  for(qi=0;qi<63;qi++){
    for(ci=0;ci<64;ci++){
      float qmax;
      qmax=2*_enc->weight_mins[0][ci];
      if(_enc->state.dequant_tables[qi].intra_y[ci]>qmax||
       _enc->state.dequant_tables[qi].intra_c[ci]>qmax||
       _enc->state.dequant_tables[qi].intra_c[ci]>qmax||
       !_intra_only&&
       (_enc->state.dequant_tables[qi].inter[ci]>qmax||
       _enc->state.dequant_tables[qi].inter[ci]>qmax||
       _enc->state.dequant_tables[qi].inter[ci]>qmax)){
        break;
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
    if(frag->invalid)continue;
    frag->coded=1;
    frag->mbmode=OC_MODE_INTRA;
  }
}

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
    const ogg_uint16_t *quant[2];
    int                 pred_last[3];
    int                 fragx;
    int                 fragy;
    pred_last[OC_FRAME_GOLD]=0;
    pred_last[OC_FRAME_PREV]=0;
    pred_last[OC_FRAME_SELF]=0;
    fplane=_enc->state.frarray.frplanes+pli;
    quant[0]=pli>0?_enc->enquant_tables[_enc->state.qi].intra_c:
     _enc->enquant_tables[_enc->state.qi].intra_y;
    quant[1]=_enc->enquant_tables[_enc->state.qi].inter;
    for(fragy=0;fragy<fplane->nvfrags;fragy++){
      for(fragx=0;fragx<fplane->nhfrags;fragx++,frag++,frinfo++){
        if(!frag->coded)continue;
        /*Note: we don't do any special token bits minimization here, because
           we don't know what the previous encoded token was, since we're not
           processing blocks in the order we'll encode them in.*/
        frag->dc=(ogg_int32_t)frinfo->dct_coeffs[0]*
         quant[frag->mbmode!=OC_MODE_INTRA][0]+OC_FQUANT_ROUND>>
         OC_FQUANT_SHIFT;
        frinfo->dct_coeffs[0]=(ogg_int16_t)OC_CLAMPI(-OC_DCT_VAL_RANGE,
         frag->dc-oc_frag_pred_dc(frag,fplane,fragx,fragy,pred_last),
         OC_DCT_VAL_RANGE);
        pred_last[OC_FRAME_FOR_MODE[frag->mbmode]]=frag->dc;
      }
    }
  }
}

static void oc_enc_frag_quant_tokenize(oc_enc_ctx *_enc,
 oc_fragment_enc_info *_frinfo,const ogg_uint16_t _fquant[64],
 const ogg_uint16_t _iquant[64]){
  static const int OC_DCT_VAL_CAT_SIZES[6]={2,4,8,16,32,512};
  static const int OC_DCT_VAL_CAT_SHIFTS[6]={1,2,3,4,5,9};
  int zzi;
  int zrun;
  int qc;
  int c_sign;
  int cati;
  int tli;
  /*The DC coefficient is already quantized (it had to be for prediction).
    Here we just tokenize it.*/
  if(_frinfo->dct_coeffs[0]){
    qc=abs(_frinfo->dct_coeffs[0]);
    c_sign=_frinfo->dct_coeffs[0]<0;
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
        qc-=3;
        if(qc<OC_NDCT_VAL_CAT2_SIZE){
          _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
           (unsigned char)(OC_DCT_VAL_CAT2+qc);
          _enc->extra_bits[0][_enc->nextra_bits[0]++]=(ogg_uint16_t)c_sign;
        }
        else{
          for(cati=0;;){
            qc-=OC_DCT_VAL_CAT_SIZES[cati];
            cati++;
            if(qc<OC_DCT_VAL_CAT_SIZES[cati]){
              _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
               (unsigned char)(OC_DCT_VAL_CAT2+cati);
              _enc->extra_bits[0][_enc->nextra_bits[0]++]=
               (ogg_uint16_t)((c_sign<<OC_DCT_VAL_CAT_SHIFTS[cati])+qc);
            }
          }
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
    c_sign=_frinfo->dct_coeffs[ci]<0;
    /*qc_max is the most accurate quantized value.
      This is the largest possible (absolute) value we will use.*/
    qc_max=(ogg_int32_t)c_abs*_fquant[ci]+OC_FQUANT_ROUND>>OC_FQUANT_SHIFT;
    /*qc_min is the smallest possible (absolute) quantized value whose
       dequantized value is within the HVS-determined tolerance range.*/
    qc_min=qc_max-1;
    c_recon=qc_min*_iquant[ci];
    c_min=OC_MAXI(0,c_abs-(int)_frinfo->weights[ci]);
    for(;c_recon>=c_min;qc_min--)c_recon-=_iquant[ci];
    qc_min++;
    /*We now proceed to find a token that is as close to qc_max as possible,
       but does not use any more bits than would be required for qc_min.
      The general assumption we make is that encoding a value closer to 0
       always uses fewer bits.*/
    /*Best case: we can encode this as a zero.*/
    if(qc_min==0)zrun++;
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
            _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=OC_DCT_RUN_CAT1B;
            _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
             (ogg_uint16_t)((c_sign<<2)+zrun-6);
          }
          else{
            _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=OC_DCT_RUN_CAT1C;
            _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
             (ogg_uint16_t)((c_sign<<3)+zrun-10);
          }
          /*Skip coding the DCT value below.*/
          continue;
        }
        /*CAT2 run/value tokens: the value is 2-3.*/
        else if(qc_min<=3&&zrun<=3){
          if(zrun==1){
            _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=OC_DCT_RUN_CAT2A;
            qc=OC_MINI(1,qc_max-2);
            _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
             (ogg_uint16_t)((c_sign<<1)+qc);
          }
          else{
            _enc->dct_tokens[tli][_enc->ndct_tokens[tli]++]=OC_DCT_RUN_CAT2B;
            qc=OC_MINI(1,qc_max-2);
            _enc->extra_bits[tli][_enc->nextra_bits[tli]++]=
             (ogg_uint16_t)((c_sign<<2)+(qc<<1)+zrun-2);
          }
          /*Skip coding the DCT value below.*/
          continue;
        }
        /*The run is too long or the quantized value too large: code them
           separately.*/
        else{
          /*This is stupid: non-short ZRL tokens are never used for run values
             less than 9, but codewords are reserved for them, wasting bits.
            Yes, yes, this would've meant a non-constant number of extra bits
             for this token, but even so.*/
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
        }
      }
      /*No zero run, or the run and the qc value are being coded separately.*/
      switch(qc_min){
        case 1:{
          _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
           (unsigned char)(OC_ONE_TOKEN+c_sign);
        }break;
        case 2:{
          _enc->dct_tokens[0][_enc->ndct_tokens[0]++]=
           (unsigned char)(OC_TWO_TOKEN+c_sign);
        }break;
        default:{
          if(qc_min-3<OC_NDCT_VAL_CAT2_SIZE){
            _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
             (unsigned char)(OC_DCT_VAL_CAT2+qc_min-3);
            _enc->extra_bits[zzi][_enc->nextra_bits[zzi]++]=
             (ogg_uint16_t)c_sign;
          }
          else{
            int qc_offs;
            qc_offs=3+OC_NDCT_VAL_CAT2_SIZE;
            for(cati=0;;){
              qc_offs+=OC_DCT_VAL_CAT_SIZES[cati];
              cati++;
              if(qc_min-qc_offs<OC_DCT_VAL_CAT_SIZES[cati]){
                /*qc_min can be encoded in this category.
                  Since all DCT values in the category use the same number of
                   bits, we encode the closest value to qc_max.
                  This is either qc_max itself, if it is in the category's
                   range, or the largest value in the category.*/
                qc=OC_MINI(OC_DCT_VAL_CAT_SIZES[cati]-1,qc_max-qc_offs);
                _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
                 (unsigned char)(OC_DCT_VAL_CAT2+cati);
                _enc->extra_bits[zzi][_enc->nextra_bits[zzi]++]=
                 (ogg_uint16_t)((c_sign<<OC_DCT_VAL_CAT_SHIFTS[cati])+qc);
              }
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
        if(_enc->extra_bits[tli][ebi]<4)_enc->extra_bits[tli][ebi]++;
        else{
          _enc->dct_tokens[tli][toki]++;
          _enc->extra_bits[tli][ebi]=0;
        }
      }break;
      case OC_DCT_REPEAT_RUN1_TOKEN:{
        ebi=_enc->nextra_bits[tli]-1;
        if(_enc->extra_bits[tli][ebi]<8)_enc->extra_bits[tli][ebi]++;
        else{
          _enc->dct_tokens[tli][toki]++;
          _enc->extra_bits[tli][ebi]=0;
        }
      }break;
      case OC_DCT_REPEAT_RUN2_TOKEN:{
        ebi=_enc->nextra_bits[tli]-1;
        if(_enc->extra_bits[tli][ebi]<16)_enc->extra_bits[tli][ebi]++;
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
        if(_enc->extra_bits[tli][ebi]<4096){
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
}

static void oc_enc_residual(oc_enc_ctx *_enc){
  oc_sb *sb;
  oc_sb *sb_end;
  int    pli;
  int    zzi;
  /*Clear any existing DCT tokens.*/
  for(zzi=0;zzi<64;zzi++){
    _enc->ndct_tokens[zzi]=_enc->nextra_bits[zzi]=0;
    _enc->dct_token_offs[zzi]=_enc->extra_bits_offs[zzi]=0;
  }
  sb=sb_end=_enc->state.frarray.sbs;
  for(pli=0;pli<3;pli++){
    oc_fragment_plane  *fplane;
    const ogg_uint16_t *fquant[2];
    const ogg_uint16_t *iquant[2];
    fplane=_enc->state.frarray.frplanes+pli;
    sb_end+=fplane->nsbs;
    if(pli==0){
      fquant[0]=_enc->enquant_tables[_enc->state.qi].intra_y;
      iquant[0]=_enc->state.dequant_tables[_enc->state.qi].intra_y;
    }
    else{
      fquant[0]=_enc->enquant_tables[_enc->state.qi].intra_c;
      iquant[0]=_enc->state.dequant_tables[_enc->state.qi].intra_c;
    }
    fquant[1]=_enc->enquant_tables[_enc->state.qi].inter;
    iquant[1]=_enc->state.dequant_tables[_enc->state.qi].inter;
    for(;sb<sb_end;sb++){
      int mbi;
      for(mbi=0;mbi<4;mbi++)if(sb->mb_valid&1<<mbi){
        int bi;
        for(bi=0;bi<4;bi++){
          oc_fragment *frag;
          frag=_enc->state.frarray.frags+sb->map[mbi][bi];
          if(frag->coded){
            int qti;
            qti=frag->mbmode!=OC_MODE_INTRA;
            oc_enc_frag_quant_tokenize(_enc,_enc->frinfo+sb->map[mbi][bi],
             fquant[qti],iquant[qti]);
          }
        }
      }
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
    int ebi;
    int runl;
    /*Verify that we have tokens for each coefficient and that the previous
       coefficient ends with an EOB token while this coefficient begins with
       one.*/
    toki=_enc->ndct_tokens[zzi-1]-1;
    if(toki<0||_enc->ndct_tokens[zzi]<=0)continue;
    old_tok1=_enc->dct_tokens[zzi-1][toki];
    if(old_tok1>=OC_NDCT_EOB_TOKEN_MAX)continue;
    old_tok2=_enc->dct_tokens[zzi][0];
    if(old_tok2>=OC_NDCT_EOB_TOKEN_MAX)continue;
    /*Pull off the associated extra bits, if any, and decode the runs.*/
    ebi=_enc->nextra_bits[zzi-1];
    old_eb1=OC_DCT_TOKEN_EXTRA_BITS[old_tok1]?
     _enc->extra_bits[zzi-1][--ebi]:0;
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
    _enc->dct_tokens[zzi-1][toki]=(unsigned char)new_tok;
    /*Update the two token lists.*/
    if(OC_DCT_TOKEN_EXTRA_BITS[new_tok]){
      _enc->extra_bits[zzi-1][ebi++]=(ogg_uint16_t)(
       runl-OC_EOB_OFFS[new_tok]);
    }
    _enc->nextra_bits[zzi-1]=ebi;
    _enc->dct_token_offs[zzi]++;
    if(OC_DCT_TOKEN_EXTRA_BITS[old_tok2])_enc->extra_bits_offs[zzi]++;
  }
}

static int oc_enc_keyframe(oc_enc_ctx *_enc){
  _enc->state.frame_type=OC_INTRA_FRAME;
  oc_enc_quant_sel_quality(_enc,1);
  oggpackB_reset(&_enc->opb);
  oc_enc_frame_header_pack(_enc);
  oc_enc_mark_all_intra(_enc);
  oc_enc_quant_dc(_enc);
  oc_enc_residual(_enc);
  /*Success: Mark the packet as ready to be flushed.*/
  _enc->packet_state=OC_PACKET_READY;
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
  /*TODO: Revisit qscale.*/
  _enc->qscale=0.5F*_info->quality;
  memcpy(_enc->huff_codes,OC_VP31_HUFF_CODES,sizeof(_enc->huff_codes));
  theora_encode_set_quant_params(_enc,NULL);
  _enc->dct_tokens=(unsigned char **)oc_malloc_2d(64,
   _enc->state.frarray.nfrags,sizeof(_enc->dct_tokens[0][0]));
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
  int qi;
  if(_enc==NULL)return OC_FAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return OC_EINVAL;
  if(_qinfo==NULL)_qinfo=&OC_VP31_QUANT_INFO;
  for(qi=64;qi-->1;){
    if(_qinfo->ac_scale[qi]>_qinfo->ac_scale[qi-1])return OC_EINVAL;
    if(_qinfo->dc_scale[qi]>_qinfo->dc_scale[qi-1])return OC_EINVAL;
  }
  memcpy(&_enc->qinfo,_qinfo,sizeof(_enc->qinfo));
  for(qi=0;qi<64;qi++){
    oc_quant_tables_init(_enc->state.dequant_tables+qi,_qinfo,qi);
    oc_enquant_table_init(_enc->enquant_tables+qi,
     _enc->state.dequant_tables+qi);
  }
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
