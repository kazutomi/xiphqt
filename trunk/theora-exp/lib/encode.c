#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "fdct.h"



/*The mode orderings for the various mode coding schemes.
  Scheme 0 uses a custom alphabet, which is not stored in this table.*/
const int OC_MODE_SCHEMES[7][OC_NMODES]={
  /*Last MV dominates.*/
  /*L P M N I G GM 4*/
  {4,3,2,0,1,7,5,6},
  /*L P N M I G GM 4*/
  {4,2,3,0,1,7,5,6},
  /*L M P N I G GM 4*/
  {4,3,1,0,2,7,5,6},
  /*L M N P I G GM 4*/
  {4,2,1,0,3,7,5,6},
  /* No MV dominates.*/
  /*N L P M I G GM 4*/
  {4,0,3,1,2,7,5,6},
  /*N G L P M I GM 4*/
  {5,0,4,2,3,7,1,6},
  /*Default ordering.*/
  /*N I M L P G GM 4*/
  {1,0,2,3,4,7,5,6}
};

/*The number of different DCT coefficient values that can be stored by each
   of the different DCT value category tokens.*/
const int OC_DCT_VAL_CAT_SIZES[6]={2,4,8,16,32,512};

/*The number of bits to shift the sign of the DCT coefficient over by for each
   of the different DCT value category tokens.*/
const int OC_DCT_VAL_CAT_SHIFTS[6]={1,2,3,4,5,9};

/*The Huffman codes used for motion vectors.*/
const th_huff_code OC_MV_CODES[2][63]={
  /*Scheme 1: VLC code.*/
  {
             {0xFF,8},{0xFD,8},{0xFB,8},{0xF9,8},{0xF7,8},{0xF5,8},{0xF3,8},
    {0xF1,8},{0xEF,8},{0xED,8},{0xEB,8},{0xE9,8},{0xE7,8},{0xE5,8},{0xE3,8},
    {0xE1,8},{0x6F,7},{0x6D,7},{0x6B,7},{0x69,7},{0x67,7},{0x65,7},{0x63,7},
    {0x61,7},{0x2F,6},{0x2D,6},{0x2B,6},{0x29,6},{0x09,4},{0x07,4},{0x02,3},
    {0x00,3},
    {0x01,3},{0x06,4},{0x08,4},{0x28,6},{0x2A,6},{0x2C,6},{0x2E,6},{0x60,7},
    {0x62,7},{0x64,7},{0x66,7},{0x68,7},{0x6A,7},{0x6C,7},{0x6E,7},{0xE0,8},
    {0xE2,8},{0xE4,8},{0xE6,8},{0xE8,8},{0xEA,8},{0xEC,8},{0xEE,8},{0xF0,8},
    {0xF2,8},{0xF4,8},{0xF6,8},{0xF8,8},{0xFA,8},{0xFC,8},{0xFE,8}
  },
  /*Scheme 2: (5 bit magnitude),(1 bit sign).
    This wastes a code word (0x01, negative zero), or a bit (0x00, positive
     zero, requires only 5 bits to uniquely decode).*/
  {
             {0x3F,6},{0x3D,6},{0x3B,6},{0x39,6},{0x37,6},{0x35,6},{0x33,6},
    {0x31,6},{0x2F,6},{0x2D,6},{0x2B,6},{0x29,6},{0x27,6},{0x25,6},{0x23,6},
    {0x21,6},{0x1F,6},{0x1D,6},{0x1B,6},{0x19,6},{0x17,6},{0x15,6},{0x13,6},
    {0x11,6},{0x0F,6},{0x0D,6},{0x0B,6},{0x09,6},{0x07,6},{0x05,6},{0x03,6},
    {0x00,6},
    {0x02,6},{0x04,6},{0x06,6},{0x08,6},{0x0A,6},{0x0C,6},{0x0E,6},{0x10,6},
    {0x12,6},{0x14,6},{0x16,6},{0x18,6},{0x1A,6},{0x1C,6},{0x1E,6},{0x20,6},
    {0x22,6},{0x24,6},{0x26,6},{0x28,6},{0x2A,6},{0x2C,6},{0x2E,6},{0x30,6},
    {0x32,6},{0x34,6},{0x36,6},{0x38,6},{0x3A,6},{0x3C,6},{0x3E,6}
  }
};



int oc_sad8_fullpel(const unsigned char *_cur,int _cur_ystride,
 const unsigned char *_ref,int _ref_ystride){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++)err+=abs((int)_cur[j]-_ref[j]);
    _cur+=_cur_ystride;
    _ref+=_ref_ystride;
  }
  return err;
}

int oc_sad8_fullpel_border(const unsigned char *_cur,int _cur_ystride,
 const unsigned char *_ref,int _ref_ystride,ogg_int64_t _mask){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++){
      if(_mask&1)err+=abs((int)_cur[j]-_ref[j]);
      _mask>>=1;
    }
    _cur+=_cur_ystride;
    _ref+=_ref_ystride;
  }
  return err;
}

int oc_sad8_halfpel(const unsigned char *_cur,int _cur_ystride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_ystride){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++)err+=abs(_cur[j]-((int)_ref0[j]+_ref1[j]>>1));
    _cur+=_cur_ystride;
    _ref0+=_ref_ystride;
    _ref1+=_ref_ystride;
  }
  return err;
}

int oc_sad8_halfpel_border(const unsigned char *_cur,int _cur_ystride,
 const unsigned char *_ref0,const unsigned char *_ref1,int _ref_ystride,
 ogg_int64_t _mask){
  int i;
  int j;
  int err;
  err=0;
  for(i=0;i<8;i++){
    for(j=0;j<8;j++){
      if(_mask&1)err+=abs(_cur[j]-((int)_ref0[j]+_ref1[j])>>1);
      _mask>>=1;
    }
    _cur+=_cur_ystride;
    _ref0+=_ref_ystride;
    _ref1+=_ref_ystride;
  }
  return err;
}


/*Writes the bit pattern for the run length of a run to the given
   oggpack_buffer.
  _opb:         The buffer to write to.
  _value:       The length of the run.
                This must be positive, and no more than the maximum value of a
                 run that can be stored with the given prefix code.
  _val_min:     The minimum value that can be coded in each interval, plus an
                 extra entry for one past the last interval.
  _code_prefix: The prefix code that is prepended to the value for
                 each interval.
  _code_nbits:   The total number of bits in the bit pattern encoded for each
                 interval.
  Return: The number of bits written.*/
static int oc_run_pack(oggpack_buffer *_opb,int _value,const int _val_min[],
 const int _code_prefix[],const int _code_nbits[]){
  int i;
  for(i=0;_value>=_val_min[i+1];i++);
  oggpackB_write(_opb,_code_prefix[i]+(_value-_val_min[i]),_code_nbits[i]);
  return _code_nbits[i];
}

/*Writes the bit pattern for the run length of a super block run to the given
   oggpack_buffer.
  _opb:   The buffer to write to.
  _value: The length of the run.
          This must be positive, and no more than 4129.*/
static int oc_sb_run_pack(oggpack_buffer *_opb,int _value){
  /*Coding scheme:
       Codeword            Run Length
     0                       1
     10x                     2-3
     110x                    4-5
     1110xx                  6-9
     11110xxx                10-17
     111110xxxx              18-33
     111111xxxxxxxxxxxx      34-4129*/
  static const int VAL_MIN[8]={1,2,4,6,10,18,34,4130};
  static const int CODE_PREFIX[7]={0,4,0xC,0x38,0xF0,0x3E0,0x3F000};
  static const int CODE_NBITS[7]={1,3,4,6,8,10,18};
  return oc_run_pack(_opb,_value,VAL_MIN,CODE_PREFIX,CODE_NBITS);
}

/*Writes the bit pattern for the run length of a block run to the given
   oggpack_buffer.
  _opb:   The buffer to write to.
  _value: The length of the run.
          This must be positive, and no more than 30.*/
static int oc_block_run_pack(oggpack_buffer *_opb,int _value){
  /*Coding scheme:
     Codeword             Run Length
     0x                      1-2
     10x                     3-4
     110x                    5-6
     1110xx                  7-10
     11110xx                 11-14
     11111xxxx               15-30*/
  static const int VAL_MIN[7]={1,3,5,7,11,15,31};
  static const int CODE_PREFIX[6]={0,4,0xC,0x38,0x78,0x1F0};
  static const int CODE_NBITS[6]={2,3,4,6,7,9};
  return oc_run_pack(_opb,_value,VAL_MIN,CODE_PREFIX,CODE_NBITS);
}



/*Initializes the macro block neighbor lists.
  This assumes that the entire mbinfo memory region has been initialized with
   zeros.
  _enc: The encoding context.*/
static void oc_enc_init_mbinfo(oc_enc_ctx *_enc){
  oc_theora_state *state;
  int              nmbs;
  int              mbi;
  state=&_enc->state;
  /*Loop through the Y plane super blocks.*/
  nmbs=state->nmbs;
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
    mb=state->mbs+mbi;
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
      if(nmbx<0||nmbx>=state->nhmbs||nmby<0||nmby>=state->nvmbs)continue;
      nmbi=oc_state_mbi_for_pos(state,nmbx,nmby);
      if(state->mbs[nmbi].mode==OC_MODE_INVALID)continue;
      emb->cneighbors[emb->ncneighbors++]=nmbi;
    }
    /*Fill in the neighbors with previous motion vectors available.*/
    for(i=0;i<4;i++){
      nmbx=mbx+PDX[i];
      nmby=mby+PDY[i];
      if(nmbx<0||nmbx>=state->nhmbs||nmby<0||nmby>=state->nvmbs)continue;
      nmbi=oc_state_mbi_for_pos(state,nmbx,nmby);
      if(state->mbs[nmbi].mode==OC_MODE_INVALID)continue;
      emb->pneighbors[emb->npneighbors++]=nmbi;
    }
  }
}

/*Sets the Huffman codes to use for the DCT tokens.
  This may only be called before the setup header is written.
  If it is called multiple times, only the last call has any effect.
  _codes: An array of 80 Huffman tables with 32 elements each.
          This may be NULL, in which case the default Huffman codes will be
           used.
  Return: 0 on success, or a negative value on error.
          TH_EFAULT if _enc is NULL
          TH_EINVAL if the setup header has already been written, the code is
           not prefix free, or does not form a full binary tree.*/
static int oc_enc_set_huffman_codes(oc_enc_ctx *_enc,
 const th_huff_code _codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS]){
  int ret;
  if(_enc==NULL)return TH_EFAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return TH_EINVAL;
  if(_codes==NULL)_codes=TH_VP31_HUFF_CODES;
  /*Validate the codes.*/
  oggpackB_reset(&_enc->opb);
  ret=oc_huff_codes_pack(&_enc->opb,_codes);
  if(ret<0)return ret;
  memcpy(_enc->huff_codes,_codes,sizeof(_enc->huff_codes));
  return 0;
}

/*Sets the quantization parameters to use.
  This may only be called before the setup header is written.
  If it is called multiple times, only the last call has any effect.
  _qinfo: The quantization parameters.
          These are described in more detail in theoraenc.h.
          This can be NULL, in which case the default quantization parameters
           will be used.*
  Return: 0 on success, or a negative value on error.
          TH_EFAULT if _enc is NULL.
          TH_EINVAL if the setup header has already been written, or it cannot
           be verified that the quantization level of for a particular qti,
           pli, and ci never increases as qi increases.*/
static int oc_enc_set_quant_params(th_enc_ctx *_enc,
 const th_quant_info *_qinfo){
  int qti;
  int pli;
  int qri;
  int qi;
  int ci;
  if(_enc==NULL)return TH_EFAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return TH_EINVAL;
  if(_qinfo==NULL)_qinfo=OC_DEF_QUANT_INFO+_enc->state.info.pixel_fmt;
  /*Verify that, for a given qti, pli and ci, the actual quantizer will never
     increase as qi increases.
    This is required for efficient quantizer selection.*/
  for(qi=64;qi-->1;){
    if(_qinfo->ac_scale[qi]>_qinfo->ac_scale[qi-1])return TH_EINVAL;
    if(_qinfo->dc_scale[qi]>_qinfo->dc_scale[qi-1])return TH_EINVAL;
  }
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    if(_qinfo->qi_ranges[qti][pli].nranges<=0)return TH_EINVAL;
    for(qi=qri=0;qri<_qinfo->qi_ranges[qti][pli].nranges;qri++){
      qi+=_qinfo->qi_ranges[qti][pli].sizes[qri];
      for(ci=0;ci<64;ci++){
        if(_qinfo->qi_ranges[qti][pli].base_matrices[qri+1][ci]>
         _qinfo->qi_ranges[qti][pli].base_matrices[qri][ci]){
          return TH_EINVAL;
        }
      }
    }
    if(qi!=63)return TH_EINVAL;
  }
  /*TODO: Analyze for packing purposes instead of just doing a shallow copy.*/
  memcpy(&_enc->qinfo,_qinfo,sizeof(_enc->qinfo));
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    _enc->state.dequant_tables[qti][pli]=
     _enc->state.dequant_table_data[qti][pli];
    _enc->enquant_tables[qti][pli]=_enc->enqaunt_table_data[qti][pli];
  }
  oc_enquant_tables_init(_enc->state.dequant_tables,_enc->enquant_tables,
   _qinfo);
  memcpy(_enc->state.loop_filter_limits,_qinfo->loop_filter_limits,
   sizeof(_enc->state.loop_filter_limits));
  return 0;
}

static void oc_enc_frame_header_pack(oc_enc_ctx *_enc){
  /*Mark this packet as a data packet.*/
  oggpackB_write(&_enc->opb,0,1);
  /*Write out the frame type (I or P).*/
  oggpackB_write(&_enc->opb,_enc->state.frame_type,1);
  /*Write out the current qi list.*/
  oggpackB_write(&_enc->opb,_enc->state.qis[0],6);
  if(_enc->state.nqis>1){
    oggpackB_write(&_enc->opb,1,1);
    oggpackB_write(&_enc->opb,_enc->state.qis[1],6);
    if(_enc->state.nqis>2){
      oggpackB_write(&_enc->opb,1,1);
      oggpackB_write(&_enc->opb,_enc->state.qis[2],6);
    }
    else oggpackB_write(&_enc->opb,0,1);
  }
  else oggpackB_write(&_enc->opb,0,1);
  if(_enc->state.frame_type==OC_INTRA_FRAME){
    /*Keyframes have 3 unused configuration bits, holdovers from VP3 days.
      Most of the other unused bits in the VP3 headers were eliminated.
      I don't know why these remain.*/
    oggpackB_write(&_enc->opb,0,3);
  }
}

static void oc_enc_block_qis_pack(oc_enc_ctx *_enc){
  int *coded_fragi;
  int *coded_fragi_end;
  int  ncoded_fragis;
  int  flag;
  int  nqi0;
  int  qii;
  int  run_count;
  ncoded_fragis=_enc->state.ncoded_fragis[0]+
   _enc->state.ncoded_fragis[1]+_enc->state.ncoded_fragis[2];
  if(_enc->state.nqis==1||ncoded_fragis<=0)return;
  coded_fragi=_enc->state.coded_fragis;
  coded_fragi_end=coded_fragi+ncoded_fragis;
  flag=!!_enc->frinfo[*coded_fragi].qii;
  oggpackB_write(&_enc->opb,flag,1);
  nqi0=0;
  while(coded_fragi<coded_fragi_end){
    for(run_count=0;coded_fragi<coded_fragi_end;coded_fragi++){
      if(!!_enc->frinfo[*coded_fragi].qii!=flag)break;
      run_count++;
      nqi0+=!flag;
    }
    while(run_count>4129){
      oc_sb_run_pack(&_enc->opb,4129);
      run_count-=4129;
      oggpackB_write(&_enc->opb,flag,1);
    }
    oc_sb_run_pack(&_enc->opb,run_count);
    flag=!flag;
    if(run_count==4129&&coded_fragi<coded_fragi_end){
      oggpackB_write(&_enc->opb,flag,1);
    }
  }
  if(_enc->state.nqis!=3||nqi0>=ncoded_fragis)return;
  coded_fragi=_enc->state.coded_fragis;
  for(;!_enc->frinfo[*coded_fragi].qii;coded_fragi++);
  flag=_enc->frinfo[*coded_fragi].qii-1;
  oggpackB_write(&_enc->opb,flag,1);
  while(coded_fragi<coded_fragi_end){
    for(run_count=0;coded_fragi<coded_fragi_end;coded_fragi++){
      qii=_enc->frinfo[*coded_fragi].qii;
      if(!qii)continue;
      if(qii-1!=flag)break;
      run_count++;
    }
    while(run_count>4129){
      oc_sb_run_pack(&_enc->opb,4129);
      run_count-=4129;
      oggpackB_write(&_enc->opb,flag,1);
    }
    oc_sb_run_pack(&_enc->opb,run_count);
    flag=!flag;
    if(run_count==4129&&coded_fragi<coded_fragi_end){
      oggpackB_write(&_enc->opb,flag,1);
    }
  }
}

/*Performs an fDCT on a given fragment.
  _frag:     The fragment to perform the 2D DCT on.
  _dct_vals: The output buffer for the DCT coefficients.
  _pli:      The color plane the fragment belongs to.*/
static void oc_enc_frag_inter_fdct(oc_enc_ctx *_enc,const oc_fragment *_frag,
 ogg_int16_t _dct_vals[64],int _pli){
  ogg_int16_t    pix_buf[64];
  unsigned char *src;
  unsigned char *ref0;
  unsigned char *ref1;
  int            pixi;
  int            src_ystride;
  int            ref_ystride;
  int            ref_framei;
  int            mvoffset0;
  int            mvoffset1;
  int            y;
  int            x;
  src_ystride=_enc->state.input[_pli].ystride;
  ref_framei=_enc->state.ref_frame_idx[OC_FRAME_FOR_MODE[_frag->mbmode]];
  ref_ystride=_enc->state.ref_frame_bufs[ref_framei][_pli].ystride;
  src=_frag->buffer[OC_FRAME_IO];
  if(oc_state_get_mv_offsets(&_enc->state,&mvoffset0,&mvoffset1,
   _frag->mv[0],_frag->mv[1],ref_ystride,_pli)>1){
    ref0=_frag->buffer[ref_framei]+mvoffset0;
    ref1=_frag->buffer[ref_framei]+mvoffset1;
    if(_frag->border!=NULL){
      ogg_int64_t mask;
      mask=_frag->border->mask;
      for(pixi=y=0;y<8;y++){
        for(x=0;x<8;x++,pixi++){
          pix_buf[pixi]=(ogg_int16_t)(((int)mask&1)?
           src[x]-((int)ref0[x]+ref1[x]>>1):0);
          mask>>=1;
        }
        src+=src_ystride;
        ref0+=ref_ystride;
        ref1+=ref_ystride;
      }
      oc_fdct8x8_border(_frag->border,_dct_vals,pix_buf);
    }
    else{
      for(pixi=y=0;y<8;y++){
        for(x=0;x<8;x++,pixi++){
          pix_buf[pixi]=(ogg_int16_t)(src[x]-((int)ref0[x]+ref1[x]>>1));
        }
        src+=src_ystride;
        ref0+=ref_ystride;
        ref1+=ref_ystride;
      }
      oc_fdct8x8(_dct_vals,pix_buf);
    }
  }
  else{
    ref0=_frag->buffer[ref_framei]+mvoffset0;
    if(_frag->border!=NULL){
      ogg_int64_t mask;
      mask=_frag->border->mask;
      for(pixi=y=0;y<8;y++){
        for(x=0;x<8;x++,pixi++){
          pix_buf[pixi]=(ogg_int16_t)(((int)mask&1)?src[x]-(int)ref0[x]:0);
          mask>>=1;
        }
        src+=src_ystride;
        ref0+=ref_ystride;
      }
      oc_fdct8x8_border(_frag->border,_dct_vals,pix_buf);
    }
    else{
      for(pixi=y=0;y<8;y++){
        for(x=0;x<8;x++,pixi++){
          pix_buf[pixi]=(ogg_int16_t)(src[x]-(int)ref0[x]);
        }
        src+=src_ystride;
        ref0+=ref_ystride;
      }
      oc_fdct8x8(_dct_vals,pix_buf);
    }
  }
}

/*Merge the final EOB run of each coefficient list with the start of the next,
   if possible.
  This assumes that dct_token_offs[0][zzi] is 0 for each zzi, and will
   increase it as appropriate if an EOB run is merged with that of a previous
   token index.*/
void oc_enc_merge_eob_runs(oc_enc_ctx *_enc){
  int    zzi;
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

/*Counts the tokens of each type used for the given range of coefficient
   indices in zig-zag order.
  _enc: The encoding context.
  _zzi_start:      The first zig-zag index to include.
  _zzi_end:        The first zig-zag index to not include.
  _token_counts_y: Returns the token counts for the Y' plane.
  _token_counts_c: Returns the token counts for the Cb and Cr planes.*/
static void oc_enc_count_tokens(oc_enc_ctx *_enc,int _zzi_start,int _zzi_end,
 int _token_counts_y[TH_NDCT_TOKENS],int _token_counts_c[TH_NDCT_TOKENS]){
  int zzi;
  int ti;
  memset(_token_counts_y,0,sizeof(_token_counts_y[0])*TH_NDCT_TOKENS);
  memset(_token_counts_c,0,sizeof(_token_counts_c[0])*TH_NDCT_TOKENS);
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
static void oc_enc_count_bits(oc_enc_ctx *_enc,int _hgi,
 const int _token_counts[TH_NDCT_TOKENS],int _bit_counts[16]){
  int huffi;
  int huff_base;
  int token;
  huff_base=_hgi<<4;
  for(huffi=huff_base;huffi<huff_base+16;huffi++){
    for(token=0;token<TH_NDCT_TOKENS;token++){
      _bit_counts[huffi-huff_base]+=
       _token_counts[token]*_enc->huff_codes[huffi][token].nbits;
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
static void oc_enc_huff_group_pack(oc_enc_ctx *_enc,int _zzi_start,
 int _zzi_end,int _huff_idxs[3]){
  int zzi;
  for(zzi=_zzi_start;zzi<_zzi_end;zzi++){
    int pli;
    int ti;
    int ebi;
    ti=_enc->dct_token_offs[0][zzi];
    ebi=_enc->extra_bits_offs[zzi];
    for(pli=0;pli<3;pli++){
      const th_huff_code *huff_codes;
      int                 token;
      int                 ti_end;
      /*Step 2: Write the tokens using these tables.*/
      huff_codes=_enc->huff_codes[_huff_idxs[pli]];
      /*Note: dct_token_offs[3] is really the ndct_tokens table.
        Yes, this seems like a horrible hack, yet it's strangely elegant.*/
      ti_end=_enc->dct_token_offs[pli+1][zzi];
      /*Note: don't reset ti; dct_token_offs might be non-monotonic.*/
      for(;ti<ti_end;ti++){
        token=_enc->dct_tokens[zzi][ti];
        oggpackB_write(&_enc->opb,huff_codes[token].pattern,
         huff_codes[token].nbits);
        if(OC_DCT_TOKEN_EXTRA_BITS[token]){
          oggpackB_write(&_enc->opb,_enc->extra_bits[zzi][ebi++],
           OC_DCT_TOKEN_EXTRA_BITS[token]);
        }
      }
    }
  }
}

static void oc_enc_residual_tokens_pack(oc_enc_ctx *_enc){
  static const int  OC_HUFF_LIST_MIN[6]={0,1,6,15,28,64};
  static const int *OC_HUFF_LIST_MAX=OC_HUFF_LIST_MIN+1;
  int bits_y[16];
  int bits_c[16];
  int token_counts_y[TH_NDCT_TOKENS];
  int token_counts_c[TH_NDCT_TOKENS];
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
  oc_enc_huff_group_pack(_enc,0,1,huff_idxs[0]);
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
    oc_enc_huff_group_pack(_enc,OC_HUFF_LIST_MIN[hgi],OC_HUFF_LIST_MAX[hgi],
     huff_idxs[hgi]);
  }
#if defined(OC_BITRATE_STATS)
  oc_bitrate_update_stats(_enc,huff_idxs);
#endif
}


static void oc_enc_mb_modes_pack(oc_enc_ctx *_enc){
  const th_huff_code *codes;
  const int          *mode_ranks;
  int                *coded_mbi;
  int                *coded_mbi_end;
  int                 scheme;
  scheme=_enc->mode_scheme_chooser.scheme_list[0];
  oggpackB_write(&_enc->opb,scheme,3);
  if(scheme==0){
    int ranks[8];
    int mi;
    /*The numbers associated with each mode in the stream are slightly
       different than what we use in the source.
      The lookup here converts between the two.*/
    for(mi=0;mi<OC_NMODES;mi++){
      ranks[OC_MODE_SCHEMES[6][mi]]=
       _enc->mode_scheme_chooser.scheme0_ranks[mi];
    }
    for(mi=0;mi<OC_NMODES;mi++)oggpackB_write(&_enc->opb,ranks[mi],3);
  }
  codes=_enc->mode_scheme_chooser.mode_codes[scheme];
  mode_ranks=_enc->mode_scheme_chooser.mode_ranks[scheme];
  coded_mbi=_enc->state.coded_mbis;
  coded_mbi_end=coded_mbi+_enc->state.ncoded_mbis;
  for(;coded_mbi<coded_mbi_end;coded_mbi++){
    const th_huff_code *code;
    oc_mb              *mb;
    mb=_enc->state.mbs+*coded_mbi;
    code=codes+mode_ranks[mb->mode];
    oggpackB_write(&_enc->opb,code->pattern,code->nbits);
  }
}

static void oc_enc_mv_pack(oc_enc_ctx *_enc,int _dx,int _dy){
  const th_huff_code *code;
  code=OC_MV_CODES[_enc->mv_scheme]+_dx+31;
  oggpackB_write(&_enc->opb,code->pattern,code->nbits);
  code=OC_MV_CODES[_enc->mv_scheme]+_dy+31;
  oggpackB_write(&_enc->opb,code->pattern,code->nbits);
}

static void oc_enc_mvs_pack(oc_enc_ctx *_enc){
  int *coded_mbi;
  int *coded_mbi_end;
  oggpackB_write(&_enc->opb,_enc->mv_scheme,1);
  coded_mbi=_enc->state.coded_mbis;
  coded_mbi_end=coded_mbi+_enc->state.ncoded_mbis;
  for(;coded_mbi<coded_mbi_end;coded_mbi++){
    oc_mb          *mb;
    oc_mb_enc_info *mbinfo;
    int             mbi;
    mbi=*coded_mbi;
    mb=_enc->state.mbs+mbi;
    switch(mb->mode){
      case OC_MODE_INTER_MV:
      case OC_MODE_GOLDEN_MV:{
        int which_frame;
        which_frame=OC_FRAME_FOR_MODE[mb->mode];
        mbinfo=_enc->mbinfo+mbi;
        oc_enc_mv_pack(_enc,mbinfo->mvs[0][which_frame][0],
         mbinfo->mvs[0][which_frame][1]);
      }break;
      case OC_MODE_INTER_MV_FOUR:{
        int bi;
        mbinfo=_enc->mbinfo+mbi;
        for(bi=0;bi<4;bi++){
          int fragi;
          fragi=mb->map[0][bi];
          if(fragi>=0&&_enc->state.frags[fragi].coded){
            oc_enc_mv_pack(_enc,mbinfo->bmvs[bi][0],mbinfo->bmvs[bi][1]);
          }
        }
      }break;
    }
  }
}

static void oc_enc_enable_default_mode(oc_enc_ctx *_enc){
  /*TODO: Right now we always use VBR mode.
    When a CBR mode is available, we should use that by default if the user
     specifies a bitrate, but not a quality, in the th_info struct.*/
  if(_enc->vbr==NULL)_enc->vbr=oc_enc_vbr_alloc(_enc);
  oc_enc_vbr_enable(_enc->vbr,NULL);
}

/*A pipeline stage for copying uncoded fragments.*/

static int oc_copy_pipe_start(oc_enc_pipe_stage *_stage){
  int pli;
  for(pli=0;pli<3;pli++){
    _stage->y_procd[pli]=0;
    _stage->enc->uncoded_fragii[pli]=0;
  }
  return _stage->next!=NULL?(*_stage->next->pipe_start)(_stage->next):0;
}

static int oc_copy_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  int        *uncoded_fragis;
  oc_enc_ctx *enc;
  int         pli;
  enc=_stage->enc;
  uncoded_fragis=enc->state.uncoded_fragis;
  for(pli=0;pli<3;pli++){
    int y_avail;
    y_avail=_y_avail[pli];
    /*Process in units of super block rows, with the possible exception of the
       last, partial super block row.*/
    if(y_avail<enc->state.input[pli].height)y_avail&=~31;
    if(y_avail>_stage->y_procd[pli]){
      if(enc->uncoded_fragii[pli]<enc->state.nuncoded_fragis[pli]){
        oc_fragment_plane *fplane;
        int                fragi_end;
        int                fragii;
        fplane=enc->state.fplanes+pli;
        fragi_end=(y_avail>>3)*fplane->nhfrags+fplane->froffset;
        /*Count the uncoded fragments that belong in these super block rows.*/
        for(fragii=enc->uncoded_fragii[pli];
         fragii<enc->state.nuncoded_fragis[pli]&&
         *(uncoded_fragis-fragii)<fragi_end;fragii++);
        /*And copy them.*/
        oc_state_frag_copy(&enc->state,uncoded_fragis-fragii,
         fragii-enc->uncoded_fragii[pli],OC_FRAME_SELF,OC_FRAME_PREV,pli);
        enc->uncoded_fragii[pli]=fragii;
      }
      _stage->y_procd[pli]=y_avail;
      if(_stage->next!=NULL){
        int ret;
        ret=(*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
        if(ret<0)return ret;
      }
    }
    uncoded_fragis-=enc->state.nuncoded_fragis[pli];
  }
  return 0;
}

static int oc_copy_pipe_end(oc_enc_pipe_stage *_stage){
  return _stage->next!=NULL?(*_stage->next->pipe_end)(_stage->next):0;
}

/*Initialize the uncoded fragment copying stage of the pipeline.
  _enc: The encoding context.*/
static void oc_copy_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_copy_pipe_start;
  _stage->pipe_proc=oc_copy_pipe_process;
  _stage->pipe_end=oc_copy_pipe_end;
}

/*A pipeline stage for applying the loop filter.*/

static int oc_loop_pipe_start(oc_enc_pipe_stage *_stage){
  oc_enc_ctx *enc;
  int         pli;
  enc=_stage->enc;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=0;
  enc->loop_filter_enabled=enc->ncoded_frags>0&&
   !oc_state_loop_filter_init(&enc->state,enc->bounding_values+256);
  return _stage->next!=NULL?(*_stage->next->pipe_start)(_stage->next):0;
}

static int oc_loop_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  oc_enc_ctx *enc;
  int         pli;
  enc=_stage->enc;
  if(enc->loop_filter_enabled){
    int refi;
    refi=enc->state.ref_frame_idx[OC_FRAME_SELF];
    for(pli=0;pli<3;pli++){
      int delay;
      int fragy0;
      int fragy_end;
      fragy0=_stage->y_procd[pli]+1>>3;
      /*Add a 2 pixel delay for the vertical filter, except in the last row.*/
      delay=(_y_avail[pli]<enc->state.ref_frame_bufs[refi][pli].height);
      fragy_end=_y_avail[pli]-(delay<<1)>>3;
      if(fragy_end>fragy0){
        oc_state_loop_filter_frag_rows(&enc->state,enc->bounding_values+256,
         refi,pli,fragy0,fragy_end);
        /*We also add a 1 pixel delay to the next stage, since the vertical
           filter for the next fragment row can still change the last row of
           pixels from this fragment row.*/
        _stage->y_procd[pli]=(fragy_end<<3)-delay;
        if(_stage->next!=NULL){
          int ret;
          ret=(*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
          if(ret<0)return ret;
        }
      }
    }
  }
  else{
    for(pli=0;pli<3;pli++)_stage->y_procd[pli]=_y_avail[pli];
    if(_stage->next!=NULL){
      return (*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
    }
  }
  return 0;
}

static int oc_loop_pipe_end(oc_enc_pipe_stage *_stage){
  return _stage->next!=NULL?(*_stage->next->pipe_end)(_stage->next):0;
}

/*Initialize the loop filter stage of the pipeline.
  _enc: The encoding context.*/
static void oc_loop_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_loop_pipe_start;
  _stage->pipe_proc=oc_loop_pipe_process;
  _stage->pipe_end=oc_loop_pipe_end;
}

/*A pipeline stage for filling in the image border.*/

static int oc_fill_pipe_start(oc_enc_pipe_stage *_stage){
  int pli;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=0;
  return _stage->next!=NULL?(*_stage->next->pipe_start)(_stage->next):0;
}

static int oc_fill_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  int pli;
  if(_stage->enc->ncoded_frags>0){
    oc_theora_state *state;
    int              refi;
    state=&_stage->enc->state;
    refi=state->ref_frame_idx[OC_FRAME_SELF];
    for(pli=0;pli<3;pli++){
      if(_stage->y_procd[pli]<_y_avail[pli]){
        oc_state_borders_fill_rows(state,refi,pli,_stage->y_procd[pli],
         _y_avail[pli]);
        _stage->y_procd[pli]=_y_avail[pli];
        if(_stage->next!=NULL){
          int ret;
          ret=(*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
          if(ret<0)return ret;
        }
      }
    }
  }
  else{
    for(pli=0;pli<3;pli++)_stage->y_procd[pli]=_y_avail[pli];
    if(_stage->next!=NULL){
      return (*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
    }
  }
  return 0;
}

static int oc_fill_pipe_end(oc_enc_pipe_stage *_stage){
  oc_theora_state *state;
  int              refi;
  int              pli;
  state=&_stage->enc->state;
  refi=state->ref_frame_idx[OC_FRAME_SELF];
  for(pli=0;pli<3;pli++)oc_state_borders_fill_caps(state,refi,pli);
  return _stage->next!=NULL?(*_stage->next->pipe_end)(_stage->next):0;
}

/*Initialize the loop filter stage of the pipeline.
  _enc: The encoding context.*/
static void oc_fill_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_fill_pipe_start;
  _stage->pipe_proc=oc_fill_pipe_process;
  _stage->pipe_end=oc_fill_pipe_end;
}

/*A pipeline stage for storing the encoded frame contents in a packet.*/

static int oc_pack_pipe_start(oc_enc_pipe_stage *_stage){
  int pli;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=0;
  return 0;
}

static int oc_pack_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  int pli;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=_y_avail[pli];
  return 0;
}

static int oc_pack_pipe_end(oc_enc_pipe_stage *_stage){
  oc_enc_ctx *enc;
  int         ret;
  if(_stage->next!=NULL){
    ret=(*_stage->next->pipe_start)(_stage->next);
    if(ret<0)return ret;
  }
  enc=_stage->enc;
  oggpackB_reset(&enc->opb);
  /*Only proceed if we have some coded blocks.
    No coded blocks -> dropped frame -> 0 byte packet.*/
  if(enc->ncoded_frags>0){
    oc_enc_frame_header_pack(enc);
    if(enc->state.frame_type==OC_INTER_FRAME){
      oggpackB_writecopy(&enc->opb,
       oggpackB_get_buffer(&enc->opb_coded_flags),
       oggpackB_bits(&enc->opb_coded_flags));
      oc_enc_mb_modes_pack(enc);
      oc_enc_mvs_pack(enc);
    }
    oc_enc_block_qis_pack(enc);
    /*Pack the quantized DCT coefficients.*/
    oc_enc_residual_tokens_pack(enc);
  }
  /*Success: Mark the packet as ready to be flushed.*/
  enc->packet_state=OC_PACKET_READY;
  if(_stage->next!=NULL){
    ret=(*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
    if(ret<0)return ret;
    return (*_stage->next->pipe_end)(_stage->next);
  }
  return 0;
}

/*Initialize the loop filter stage of the pipeline.
  _enc: The encoding context.*/
static void oc_pack_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_pack_pipe_start;
  _stage->pipe_proc=oc_pack_pipe_process;
  _stage->pipe_end=oc_pack_pipe_end;
}


static int oc_enc_init(oc_enc_ctx *_enc,const th_info *_info){
  int ret;
  /*Initialize the shared encoder/decoder state.*/
  ret=oc_state_init(&_enc->state,_info);
  if(ret<0)return ret;
  _enc->block_coded_flags=_ogg_calloc(_enc->state.nfrags,
   sizeof(_enc->block_coded_flags[0]));
  /*Initialize our packet buffers.*/
  oggpackB_writeinit(&_enc->opb);
  oggpackB_writeinit(&_enc->opb_coded_flags);
  /*Allocate and initialize storage for encoder-specific fragment and macro
     block storage, as well as DCT token storage.*/
  _enc->frinfo=_ogg_calloc(_enc->state.nfrags,
   sizeof(_enc->frinfo[0]));
  _enc->mbinfo=_ogg_calloc(_enc->state.nmbs,sizeof(_enc->mbinfo[0]));
  _enc->dct_tokens=(unsigned char **)oc_malloc_2d(64,
   _enc->state.nfrags,sizeof(_enc->dct_tokens[0][0]));
  _enc->extra_bits=(ogg_uint16_t **)oc_malloc_2d(64,
   _enc->state.nfrags,sizeof(_enc->extra_bits[0][0]));
  oc_enc_init_mbinfo(_enc);
  /*Do one-time mode scheme chooser initialization.*/
  oc_mode_scheme_chooser_init(&_enc->mode_scheme_chooser);
  /*Set the maximum distance between key frames.*/
  _enc->keyframe_frequency_force=1<<_enc->state.info.keyframe_granule_shift;
  /*Initialize the motion compensation, high-level importance map, and
     low-level psychovisual model plug-ins.*/
  _enc->mcenc=oc_mcenc_alloc(_enc);
  /*Reset the packet-out state machine.*/
  _enc->packet_state=OC_PACKET_INFO_HDR;
  /*Mark us as not VP3-compatible.*/
  _enc->vp3_compatible=0;
  /*Set the Huffman codes and quantization parameters to the defaults.*/
  memcpy(_enc->huff_codes,TH_VP31_HUFF_CODES,sizeof(_enc->huff_codes));
  oc_enc_set_quant_params(_enc,NULL);
  /*Initialize the static pipeline stages.*/
  oc_fdct_pipe_init(&_enc->fdct_pipe,_enc);
  oc_copy_pipe_init(&_enc->copy_pipe,_enc);
  oc_loop_pipe_init(&_enc->loop_pipe,_enc);
  _enc->copy_pipe.next=&_enc->loop_pipe;
  oc_fill_pipe_init(&_enc->fill_pipe,_enc);
  _enc->loop_pipe.next=&_enc->fill_pipe;
  oc_pack_pipe_init(&_enc->pack_pipe,_enc);
  /*Delay initialization of the encoding pipeline until the application sets
     an encoding mode or the first frame is submitted.*/
  _enc->pipe=NULL;
  _enc->vbr=NULL;
  return 0;
}

static void oc_enc_clear(oc_enc_ctx *_enc){
  oc_enc_vbr_free(_enc->vbr);
  oc_mcenc_free(_enc->mcenc);
  oc_free_2d(_enc->extra_bits);
  oc_free_2d(_enc->dct_tokens);
  _ogg_free(_enc->mbinfo);
  _ogg_free(_enc->frinfo);
  oggpackB_writeclear(&_enc->opb_coded_flags);
  oggpackB_writeclear(&_enc->opb);
  _ogg_free(_enc->block_coded_flags);
  oc_state_clear(&_enc->state);
}



/*A default implementation of set_speed, to use when the encoding mode is not
   configurable.
  It does nothing.
  _speed: The encoding speed to use.*/
void oc_enc_set_speed_null(oc_enc_ctx *_enc,int _speed){}

/*Computes the SAD value of a fragment in the input image with respect to its
   motion compensated predictor..
  _frag:     The fragment to find the SAD of.
  _dx:       The X component of the motion vector.
  _dy:       The Y component of the motion vector.
  _pli:      The color plane the fragment belongs to.
  _frame:    The reference frame to predict from.*/
int oc_enc_frag_sad(oc_enc_ctx *_enc,oc_fragment *_frag,int _dx,
 int _dy,int _pli,int _frame){
  int cur_ystride;
  int ref_ystride;
  int ref_framei;
  int mvoffset0;
  int mvoffset1;
  cur_ystride=_enc->state.input[_pli].ystride;
  ref_framei=_enc->state.ref_frame_idx[_frame];
  ref_ystride=_enc->state.ref_frame_bufs[ref_framei][_pli].ystride;
  if(oc_state_get_mv_offsets(&_enc->state,&mvoffset0,&mvoffset1,_dx,_dy,
   ref_ystride,_pli)>1){
    if(_frag->border==NULL){
      return oc_sad8_halfpel(_frag->buffer[OC_FRAME_IO],cur_ystride,
       _frag->buffer[ref_framei]+mvoffset0,
       _frag->buffer[ref_framei]+mvoffset1,ref_ystride);
    }
    else{
      return oc_sad8_halfpel_border(_frag->buffer[OC_FRAME_IO],cur_ystride,
       _frag->buffer[ref_framei]+mvoffset0,
       _frag->buffer[ref_framei]+mvoffset1,ref_ystride,_frag->border->mask);
    }
  }
  else{
    if(_frag->border==NULL){
      return oc_sad8_fullpel(_frag->buffer[OC_FRAME_IO],cur_ystride,
       _frag->buffer[ref_framei]+mvoffset0,ref_ystride);
    }
    else{
      return oc_sad8_fullpel_border(_frag->buffer[OC_FRAME_IO],
       cur_ystride,_frag->buffer[ref_framei]+mvoffset0,ref_ystride,
       _frag->border->mask);
    }
  }
}

/*Writes the bit flags for whether or not each super block is partially coded
   or not.
  These flags are run-length encoded, with the flag value alternating between
   each run.
  Return: The number of bits written.*/
int oc_enc_partial_sb_flags_pack(oc_enc_ctx *_enc,oggpack_buffer *_opb){
  oc_sb    *sb;
  oc_sb    *sb_end;
  unsigned  flag;
  int       run_count;
  int       ret;
  /*Write the list of partially coded super block flags.*/
  flag=_enc->state.sbs[0].coded_partially;
  oggpackB_write(_opb,flag,1);
  ret=1;
  sb=_enc->state.sbs;
  sb_end=sb+_enc->state.nsbs;
  while(sb<sb_end){
    for(run_count=0;sb<sb_end;sb++){
      if(sb->coded_partially!=flag)break;
      run_count++;
    }
    /*The maximum run length we can encode is 4129.
      If we encode a run that long, we need to specify the bit value for the
       next run instead of being able to implicitly toggle it.
      Note that the original VP3 implementation did not consider this case,
       and would not write the extra bit for runs of 4129, and would write an
       invalid code for longer runs.*/
    /*First, encode runs until we have 4129 or fewer sbs left.*/
    while(run_count>4129){
      ret+=oc_sb_run_pack(_opb,4129);
      run_count-=4129;
      oggpackB_write(_opb,flag,1);
      ret++;
    }
    /*Encode the last run.*/
    ret+=oc_sb_run_pack(_opb,run_count);
    flag=!flag;
    /*If there are more sbs to come, and we had a run of 4129 exactly,
       encode the flipped bit.*/
    if(run_count==4129&&sb<sb_end){
      oggpackB_write(_opb,flag,1);
      ret++;
    }
  }
  return ret;
}

/*Writes the coded/not coded flags for each super block that is not partially
   coded.
  These flags are run-length encoded, with the flag value altenating between
   each run.
  Return: The number of bits written.*/
int oc_enc_coded_sb_flags_pack(oc_enc_ctx *_enc,oggpack_buffer *_opb){
  oc_sb    *sb;
  oc_sb    *sb_end;
  unsigned  flag;
  int       run_count;
  int       ret;
  /*Write the list of coded/not coded super block flags.*/
  /*Skip partially coded super blocks: their flags have already been coded.*/
  sb=_enc->state.sbs;
  sb_end=sb+_enc->state.nsbs;
  for(;;sb++){
    if(sb>=sb_end)return 0;
    if(!sb->coded_partially)break;
  }
  flag=sb->coded_fully;
  oggpackB_write(_opb,flag,1);
  ret=1;
  while(sb<sb_end){
    for(run_count=0;sb<sb_end;sb++){
      if(sb->coded_partially)continue;
      if(sb->coded_fully!=flag)break;
      run_count++;
    }
    /*The maximum run length we can encode is 4129.
      If we encode a run that long, we need to specify the bit value for the
       next run instead of being able to implicitly toggle it.
      Note that the original VP3 implementation did not consider this case,
       and would not write the extra bit for runs of 4129, and would write an
       invalid code for longer runs.*/
    /*First, encode runs until we have 4129 or fewer sbs left.*/
    while(run_count>4129){
      ret+=oc_sb_run_pack(_opb,4129);
      run_count-=4129;
      oggpackB_write(_opb,flag,1);
      ret++;
    }
    /*Encode the last run.*/
    ret+=oc_sb_run_pack(_opb,run_count);
    flag=!flag;
    if(run_count==4129&&sb<sb_end){
      oggpackB_write(_opb,flag,1);
      ret++;
    }
  }
  return ret;
}

/*Writes the coded/not coded flags for each block belonging to a partially
   coded super block.
  These flags are run-length encoded, with the flag value alternating between
   each run.
  Return: The number of bits written.*/
int oc_enc_coded_block_flags_pack(oc_enc_ctx *_enc,oggpack_buffer *_opb){
  int flag;
  int run_count;
  int bli;
  int ret;
  if(_enc->nblock_coded_flags<=0)return 0;
  flag=_enc->block_coded_flags[0];
  oggpackB_write(_opb,flag,1);
  ret=1;
  for(bli=0;bli<_enc->nblock_coded_flags;){
    for(run_count=0;bli<_enc->nblock_coded_flags;bli++){
      if(_enc->block_coded_flags[bli]!=flag)break;
      run_count++;
    }
    /*Since each super block must have a mix of coded and not coded blocks to
       get on this list, we are guaranteed a maximum run size of 30 (16 blocks
       per super block, with flags (1000 0000 0000 0000, 0000 0000 0000 0001)
       or its complement).
      This avoids the nastiness of the VLC not letting us encode runs long
       enough like above.*/
    ret+=oc_block_run_pack(_opb,run_count);
    flag=!flag;
  }
  return ret;
}

/*Performs a motion-compensated fDCT for each fragment coded in a mode other
   than INTRA.*/
void oc_enc_do_inter_dcts(oc_enc_ctx *_enc){
  int *coded_fragi;
  int *coded_fragi_end;
  int  pli;
  coded_fragi_end=coded_fragi=_enc->state.coded_fragis;
  for(pli=0;pli<3;pli++){
    coded_fragi_end+=_enc->state.ncoded_fragis[pli];
    for(;coded_fragi<coded_fragi_end;coded_fragi++){
      oc_fragment *frag;
      frag=_enc->state.frags+*coded_fragi;
      if(frag->mbmode!=OC_MODE_INTRA){
        oc_fragment_enc_info *efrag;
        efrag=_enc->frinfo+(frag-_enc->state.frags);
        oc_enc_frag_inter_fdct(_enc,frag,efrag->dct_coeffs,pli);
      }
    }
  }
}


th_enc_ctx *th_encode_alloc(const th_info *_info){
  oc_enc_ctx *enc;
  if(_info==NULL)return NULL;
  enc=_ogg_malloc(sizeof(*enc));
  if(oc_enc_init(enc,_info)<0){
    _ogg_free(enc);
    return NULL;
  }
  return enc;
}

void th_encode_free(th_enc_ctx *_enc){
  if(_enc!=NULL){
    oc_enc_clear(_enc);
    _ogg_free(_enc);
  }
}



int th_encode_ctl(th_enc_ctx *_enc,int _req,void *_buf,size_t _buf_sz){
  switch(_req){
    case TH_ENCCTL_SET_HUFFMAN_CODES:{
      if(_buf==NULL&&_buf_sz!=0||_buf!=NULL&&
       _buf_sz!=sizeof(th_huff_code)*TH_NHUFFMAN_TABLES*TH_NDCT_TOKENS){
        return TH_EINVAL;
      }
      return oc_enc_set_huffman_codes(_enc,(const th_huff_table *)_buf);
    }break;
    case TH_ENCCTL_SET_QUANT_PARAMS:{
      if(_buf==NULL&&_buf_sz!=0||
       _buf!=NULL&&_buf_sz!=sizeof(th_quant_info)){
        return TH_EINVAL;
      }
      return oc_enc_set_quant_params(_enc,(th_quant_info *)_buf);
    }break;
    case TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE:{
      ogg_uint32_t keyframe_frequency_force;
      if(_enc==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(ogg_uint32_t))return TH_EINVAL;
      keyframe_frequency_force=*(ogg_uint32_t *)_buf;
      if(_enc->packet_state==OC_PACKET_INFO_HDR){
        /*It's still early enough to enlarge keyframe_granule_shift.*/
        _enc->state.info.keyframe_granule_shift=
         OC_MAXI(_enc->state.info.keyframe_granule_shift,
         OC_MINI(31,oc_ilog(keyframe_frequency_force-1)));
      }
      _enc->keyframe_frequency_force=OC_MINI(keyframe_frequency_force,
       1U<<_enc->state.info.keyframe_granule_shift);
      *(ogg_uint32_t *)_buf=_enc->keyframe_frequency_force;
      return 0;
    }break;
    case TH_ENCCTL_SET_VP3_COMPATIBLE:{
      int vp3_compatible;
      int ret;
      if(_enc==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(int))return TH_EINVAL;
      vp3_compatible=*(int *)_buf;
      _enc->vp3_compatible=vp3_compatible;
      ret=oc_enc_set_huffman_codes(_enc,TH_VP31_HUFF_CODES);
      if(ret<0)vp3_compatible=0;
      ret=oc_enc_set_quant_params(_enc,&TH_VP31_QUANT_INFO);
      if(ret<0)vp3_compatible=0;
      if(_enc->state.info.pixel_fmt!=TH_PF_420||
       _enc->state.info.pic_width<_enc->state.info.frame_width||
       _enc->state.info.pic_height<_enc->state.info.frame_height||
      /*If we have more than 4095 super blocks, VP3's RLE coding might
         overflow.
        We could overcome this by ensuring we flip the coded/not-coded flags on
         at least one super block in the frame, but we pick the simple solution
         of just marking the stream incompatible instead.
        It's unlikely the old VP3 codec would be able to decode streams at this
         resolution in real time in the first place.*/
       _enc->state.nsbs>4095){
        vp3_compatible=0;
      }
      *(int *)_buf=vp3_compatible;
      return 0;
    }break;
    case TH_ENCCTL_GET_SPLEVEL_MAX:{
      if(_enc==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(int))return TH_EINVAL;
      /*We can only manipulate speed in the context of a given encoding mode.
        Ensure one is selected if the user has not already done so.*/
      if(_enc->set_speed==NULL)oc_enc_enable_default_mode(_enc);
      *(int *)_buf=_enc->speed_max;
      return 0;
    }break;
    case TH_ENCCTL_SET_SPLEVEL:{
      int speed;
      if(_enc==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(int))return TH_EINVAL;
      speed=*(int *)_buf;
      /*We can only manipulate speed in the context of a given encoding mode.
        Ensure one is selected if the user has not already done so.*/
      if(_enc->set_speed==NULL)oc_enc_enable_default_mode(_enc);
      if(speed<0||speed>_enc->speed_max)return TH_EINVAL;
      (*_enc->set_speed)(_enc,speed);
      return 0;
    }break;
    case TH_ENCCTL_SETUP_VBR:{
      if(_enc==NULL)return TH_EFAULT;
      if(_buf==NULL&&_buf_sz!=0||_buf!=NULL&&_buf_sz!=sizeof(th_vbr_cfg)){
        return TH_EINVAL;
      }
      if(_enc->vbr==NULL)_enc->vbr=oc_enc_vbr_alloc(_enc);
      return oc_enc_vbr_enable(_enc->vbr,(th_vbr_cfg *)_buf);
    }break;
    default:return TH_EIMPL;
  }
}

int th_encode_flushheader(th_enc_ctx *_enc,th_comment *_tc,ogg_packet *_op){
  if(_enc==NULL)return TH_EFAULT;
  return oc_state_flushheader(&_enc->state,&_enc->packet_state,&_enc->opb,
   &_enc->qinfo,(const th_huff_table *)_enc->huff_codes,th_version_string(),
   _tc,_op);
}

int th_encode_ycbcr_in(th_enc_ctx *_enc,th_ycbcr_buffer _img){
  th_ycbcr_buffer img;
  int             y_avail[3];
  int             cwidth;
  int             cheight;
  int             ret;
  int             rfi;
  int             pli;
  /*Step 1: validate parameters.*/
  if(_enc==NULL||_img==NULL)return TH_EFAULT;
  if(_enc->packet_state==OC_PACKET_DONE)return TH_EINVAL;
  if(_img[0].width!=(int)_enc->state.info.frame_width||
   _img[0].height!=(int)_enc->state.info.frame_height){
    return TH_EINVAL;
  }
  cwidth=_enc->state.info.frame_width>>!(_enc->state.info.pixel_fmt&1);
  cheight=_enc->state.info.frame_height>>!(_enc->state.info.pixel_fmt&2);
  if(_img[1].width!=cwidth||_img[2].width!=cwidth||
   _img[1].height!=cheight||_img[2].height!=cheight){
    return TH_EINVAL;
  }
  /*Flip the input buffer upside down.*/
  oc_ycbcr_buffer_flip(img,_img);
  /*Step 2: Update buffer state.*/
  if(_enc->state.ref_frame_idx[OC_FRAME_SELF]>=0){
    _enc->state.ref_frame_idx[OC_FRAME_PREV]=
     _enc->state.ref_frame_idx[OC_FRAME_SELF];
    if(_enc->state.frame_type==OC_INTRA_FRAME){
      /*The new frame becomes both the previous and gold reference frames.*/
      _enc->state.keyframe_num=_enc->state.curframe_num;
      _enc->state.ref_frame_idx[OC_FRAME_GOLD]=
       _enc->state.ref_frame_idx[OC_FRAME_SELF];
    }
  }
  /*If no encoding mode has been explicitly enabled by the application,
     enable the default encoding mode with a default configuration.*/
  else if(_enc->pipe==NULL)oc_enc_enable_default_mode(_enc);
  /*Select a free buffer to use for the reconstructed version of this frame.*/
  for(rfi=0;rfi==_enc->state.ref_frame_idx[OC_FRAME_GOLD]||
   rfi==_enc->state.ref_frame_idx[OC_FRAME_PREV];rfi++);
  _enc->state.ref_frame_idx[OC_FRAME_SELF]=rfi;
  _enc->state.curframe_num++;
  /*Fill the fragment array with pointers into the user buffer.*/
  oc_state_fill_buffer_ptrs(&_enc->state,OC_FRAME_IO,img);
  /*Reset the encoding pipeline.*/
  ret=(*_enc->pipe->pipe_start)(_enc->pipe);
  if(ret<0)return ret;
  /*Push the image into the pipeline.*/
  for(pli=0;pli<3;pli++)y_avail[pli]=_img[pli].height;
  ret=(*_enc->pipe->pipe_proc)(_enc->pipe,y_avail);
  if(ret<0)return ret;
  /*Flush the results through.*/
  ret=(*_enc->pipe->pipe_end)(_enc->pipe);
  if(ret<0)return ret;
  /*Note: All buffer management, etc., that is done after a frame is encoded
     is delayed until the next frame is encoded.
    This allows for a future API that would let an encoding application
     examine the decompressed output and attempt to re-encode the same frame
     again with different settings if it disapproved.
    Here we just update the granpos needed for the output packet and return.*/
  if(_enc->state.frame_type==OC_INTRA_FRAME){
    _enc->state.granpos=
     _enc->state.curframe_num<<_enc->state.info.keyframe_granule_shift;
  }
  else{
    _enc->state.granpos=
     (_enc->state.keyframe_num<<_enc->state.info.keyframe_granule_shift)+
     (_enc->state.curframe_num-_enc->state.keyframe_num);
  }
#if defined(OC_DUMP_IMAGES)
  /*This is done after the granpos update, because that's what it uses to name
     the output file.*/
  oc_state_dump_frame(&_enc->state,OC_FRAME_SELF,"rec");
#endif
  return 0;
}

int th_encode_packetout(th_enc_ctx *_enc,int _last,ogg_packet *_op){
  if(_enc==NULL||_op==NULL)return TH_EFAULT;
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
