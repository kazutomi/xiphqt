#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "fdct.h"



/*The mode orderings for the various mode coding schemes.
  Scheme 0 uses a custom alphabet, which is not stored in this table.*/
static const int OC_MODE_SCHEMES[7][OC_NMODES]={
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

/*Whether or not each mode has a motion vector associated with it.
  Otherwise, the mode is assumed to use the 0,0 vector.*/
static const int OC_MODE_HAS_MV[OC_NMODES]={0,0,1,1,1,1,0,1};

/*The Huffman codes used for motion vectors.*/
static const theora_huff_code OC_MV_CODES[2][63]={
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



static int oc_mvbitsa(int _dx,int _dy){
  return OC_MV_CODES[0][_dx+31].nbits+OC_MV_CODES[0][_dy+31].nbits;
}



static const theora_huff_code OC_MODE_CODESA[OC_NMODES]={
  {0x00,1},{0x02,2},{0x06,3},{0x0E,4},{0x1E,5},{0x3E,6},{0x7E,7},{0x7F,7}
};

static const theora_huff_code OC_MODE_CODESB[OC_NMODES]={
  {0x01,3},{0x00,3},{0x02,3},{0x03,3},{0x04,3},{0x07,3},{0x06,3},{0x05,3}
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



/*Initialize the mode scheme chooser.
  This need only be called once per encoder.
  This is probably the best place to describe the various scheme's Theora uses
   to encode macro block modes.
  There are 8 possible schemes.
  Schemes 0-6 use a highly unbalanced Huffman code to code each of the modes.
  The same set of Huffman codes is used for each of these 7 schemes, but the
   mode assigned to each code varies.
  Schemes 1-6 have a fixed mapping from Huffman code to MB mode, while scheme
   0 writes a custom mapping to the bitstream before all the modes.
  Finally, scheme 7 just encodes each mode directly in 3 bits.
  Be warned that the number assigned to each mode is slightly different in the
   bitstream than in this implementation, so a translation needs to be done.

  Mode name:                 Source-code index;  Bit-stream index:
  OC_MODE_INTRA              0                   1
  OC_MODE_INTER_NOMV         1                   0
  OC_MODE_INTER_MV           2                   2
  OC_MODE_INTER_MV_LAST      3                   3
  OC_MODE_INTER_MV_LAST2     4                   4
  OC_MODE_INTER_MV_FOUR      5                   6
  OC_MODE_GOLDEN_NOMV        6                   7
  OC_MODE_GOLDEN_MV          7                   5

  The bit stream indices come from the constants assigned to each mode in the
   original VP3 source.*/
static void oc_mode_scheme_chooser_init(oc_mode_scheme_chooser *_chooser){
  int msi;
  _chooser->mode_ranks[0]=_chooser->scheme0_ranks;
  for(msi=0;msi<7;msi++){
    _chooser->mode_codes[msi]=OC_MODE_CODESA;
    _chooser->mode_ranks[msi+1]=OC_MODE_SCHEMES[msi];
  }
  _chooser->mode_codes[7]=OC_MODE_CODESB;
}

/*Reset the mode scheme chooser.
  This needs to be called once for each frame, including the first.*/
static void oc_mode_scheme_chooser_reset(oc_mode_scheme_chooser *_chooser){
  int i;
  memset(_chooser->mode_counts,0,sizeof(_chooser->mode_counts));
  /*Scheme 0 starts with 24 bits to store the mode list in.*/
  _chooser->scheme_bits[0]=24;
  memset(_chooser->scheme_bits+1,0,7*sizeof(_chooser->scheme_bits[1]));
  for(i=0;i<8;i++){
    /*Scheme 7 should always start first, and scheme 0 should always start
       last.*/
    _chooser->scheme_list[i]=7-i;
    _chooser->scheme0_list[i]=_chooser->scheme0_ranks[i]=i;
  }
}

/*This is the real purpose of this data structure: not actually selecting a
   mode scheme, but estimating the cost of coding a given mode given all the
   modes selected so far.
  This is done via opportunity cost: the cost is defined as the number of bits
   required to encode all the modes selected so far including the current one
   using the best possible scheme, minus the number of bits required to encode
   all the modes selected so far not including the current one using the best
   possible scheme.
  The computational expense of doing this probably makes it overkill.
  Just be happy we take a greedy approach instead of trying to solve the
   global mode-selection problem (which is NP-hard).
  _mode: The mode to determine the cost of.
  Return: The number of bits required to code this mode.*/
static int oc_mode_scheme_chooser_cost(oc_mode_scheme_chooser *_chooser,
 int _mode){
  int scheme0;
  int scheme1;
  int si;
  int scheme_bits;
  int best_bits;
  int mode_bits;
  scheme0=_chooser->scheme_list[0];
  scheme1=_chooser->scheme_list[1];
  best_bits=_chooser->scheme_bits[scheme0];
  mode_bits=_chooser->mode_codes[scheme0][
   _chooser->mode_ranks[scheme0][_mode]].nbits;
  /*Typical case: If the difference between the best scheme and the next best
     is greater than 6 bits, then adding just one mode cannot change which
     scheme we use.*/
  if(_chooser->scheme_bits[scheme1]-best_bits>6)return mode_bits;
  /*Otherwise, check to see if adding this mode selects a different scheme
     as the best.*/
  si=1;
  best_bits+=mode_bits;
  do{
    scheme1=_chooser->scheme_list[si];
    /*For any scheme except 0, we can just use the bit cost of the mode's rank
       in that scheme.*/
    if(scheme1!=0){
      scheme_bits=_chooser->scheme_bits[scheme1]+
       _chooser->mode_codes[scheme1][
       _chooser->mode_ranks[scheme1][_mode]].nbits;
    }
    else{
      int ri;
      /*For scheme 0, incrementing the mode count could potentially change the
         mode's rank.
        Find the index where the mode would be moved to in the optimal list,
         and use its bit cost instead of the one for the mode's current
         position in the list.*/
      for(ri=_chooser->scheme0_ranks[_mode];ri>0&&
       _chooser->mode_counts[_mode]>=
       _chooser->mode_counts[_chooser->scheme0_list[ri-1]];ri--);
      scheme_bits=_chooser->scheme_bits[0]+OC_MODE_CODESA[ri].nbits;
    }
    if(scheme_bits<best_bits)best_bits=scheme_bits;
    si++;
  }
  while(si<8&&_chooser->scheme_bits[_chooser->scheme_list[si]]-
   _chooser->scheme_bits[scheme0]<=6);
  return best_bits-_chooser->scheme_bits[scheme0];
}

/*Update the mode counts and per-scheme bit counts and re-order the scheme
   lists once a mode has been selected.
  _mode: The mode that was chosen.*/
static void oc_mode_scheme_chooser_update(oc_mode_scheme_chooser *_chooser,
 int _mode){
  int ri;
  int si;
  _chooser->mode_counts[_mode]++;
  /*Re-order the scheme0 mode list if necessary.*/
  for(ri=_chooser->scheme0_ranks[_mode];ri>0;ri--){
    int pmode;
    pmode=_chooser->scheme0_list[ri-1];
    if(_chooser->mode_counts[pmode]>=_chooser->mode_counts[_mode])break;
    _chooser->scheme0_ranks[pmode]++;
    _chooser->scheme0_list[ri]=pmode;
  }
  _chooser->scheme0_ranks[_mode]=ri;
  _chooser->scheme0_list[ri]=_mode;
  /*Now add the bit cost for the mode to each scheme.*/
  for(si=0;si<8;si++){
    _chooser->scheme_bits[si]+=
     _chooser->mode_codes[si][_chooser->mode_ranks[si][_mode]].nbits;
  }
  /*Finally, re-order the list of schemes.*/
  for(si=1;si<8;si++){
    int sj;
    int scheme0;
    int bits0;
    scheme0=_chooser->scheme_list[si];
    bits0=_chooser->scheme_bits[scheme0];
    sj=si;
    do{
      int scheme1;
      scheme1=_chooser->scheme_list[sj-1];
      if(bits0>=_chooser->scheme_bits[scheme1])break;
      _chooser->scheme_list[sj]=scheme1;
    }
    while(--sj>0);
    _chooser->scheme_list[sj]=scheme0;
  }
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
          OC_FAULT if _enc is NULL
          OC_EINVAL if the setup header has already been written, the code is
           not prefix free, or does not form a full binary tree.*/
static int oc_enc_set_huffman_codes(oc_enc_ctx *_enc,
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

/*Sets the quantization parameters to use.
  This may only be called before the setup header is written.
  If it is called multiple times, only the last call has any effect.
  _qinfo: The quantization parameters.
          These are described in more detail in theora.h.
          This can be NULL, in which case the default quantization parameters
           will be used.*
  Return: 0 on success, or a negative value on error.
          OC_FAULT if _enc is NULL.
          OC_EINVAL if the setup header has already been written, or it cannot
           be verified that the quantization level of for a particular qti,
           pli, and ci never increases as qi increases.*/
static int oc_enc_set_quant_params(theora_enc_ctx *_enc,
 const theora_quant_info *_qinfo){
  int qti;
  int pli;
  int qri;
  int qi;
  int ci;
  if(_enc==NULL)return OC_FAULT;
  if(_enc->packet_state>OC_PACKET_SETUP_HDR)return OC_EINVAL;
  if(_qinfo==NULL)_qinfo=OC_DEF_QUANT_INFO+_enc->state.info.pixel_fmt;
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
      for(ci=0;ci<64;ci++){
        if(_qinfo->qi_ranges[qti][pli].base_matrices[qri+1][ci]>
         _qinfo->qi_ranges[qti][pli].base_matrices[qri][ci]){
          return OC_EINVAL;
        }
      }
    }
    if(qi!=63)return OC_EINVAL;
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

/*Select the set of quantizers to use for the current frame for each possible
   frame type (intra or inter).
  This does not assign a quantizer to each fragment, as that depends on the
   quantizer type used and thus is done during mode decision.*/
static void oc_enc_quant_sel_quality(oc_enc_ctx *_enc,int _intra_only){
  unsigned              qmax[2][3];
  int                   fti;
  int                   qti;
  int                   pli;
  int                   dc_qi[2];
  /*The first quantizer value is used for DC coefficients.
    Select one that allows us to meet our quality requirements.*/
  for(qti=0;qti<1+!_intra_only;qti++)for(pli=0;pli<3;pli++){
    qmax[qti][pli]=OC_MAXI(2U*_enc->dc_tol_mins[pli],OC_DC_QUANT_MIN[qti]);
  }
  /*For intra frames...(containing just INTRA fragments)*/
  for(dc_qi[0]=0;dc_qi[0]<63;dc_qi[0]++){
    if(_enc->state.dequant_tables[0][0][dc_qi[0]][0]<=qmax[0][0]&&
     _enc->state.dequant_tables[0][1][dc_qi[0]][0]<=qmax[0][1]&&
     _enc->state.dequant_tables[0][2][dc_qi[0]][0]<=qmax[0][2]){
      break;
    }
  }
  /*For inter frames...(containing both INTER and INTRA fragments)*/
  if(!_intra_only){
    for(dc_qi[1]=dc_qi[0];dc_qi[1]<63;dc_qi[1]++){
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
    for(qi0=64;qi0-->0&&nqis[qi0]<=0;);
    for(qi1=qi0-1;qi1>=0&&nqis[qi1]<=nqis[qi0];qi1--);
    /*Test to make sure there are even two unique quantizers.*/
    if(qi1>=0){
      ogg_int64_t best_metric;
      ogg_int64_t metric;
      int         best_qi1;
      int         best_qi2;
      int         qii;
      for(qi2=qi1-1;qi2>=0&&nqis[qi2]<=nqis[qi1];qi2--);
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
  }
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

/*Mark all fragments as coded and in OC_MODE_INTRA.
  This also selects a quantizer value for each fragment and builds up the
   coded fragment list (in coded order) and clears the uncoded fragment list.
  It does not update the coded macro block list, as that is not used when
   coding INTRA frames.*/
static void oc_enc_mark_all_intra(oc_enc_ctx *_enc){
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
            if(_enc->state.qis[qii]<_enc->state.qis[best_qii]&&
             efrag->qi_min[0]<=_enc->state.qis[qii]){
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
  _ystride:  The Y stride of the plane the fragment belongs to.
  _framei:   The picture buffer index to perform the DCT on.
             Use OC_FRAME_IO for the current input frame.*/
void oc_enc_frag_intra_fdct(oc_enc_ctx *_enc,const oc_fragment *_frag,
 ogg_int16_t _dct_vals[64],int _ystride,int _framei){
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
     (absolute) values would have fewer significant bits chopped off when
     rounding.
    We're stuck with it, however.
    At least it might reduce bias towards 0 when coding unpredicted DC
     coefficients, but that's not what VP3 justified it with.*/
  pixels=_frag->buffer[_framei];
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
        /*This branchless code is (almost) equivalent to the previous line:
        int pmask;
        pmask=-(int)mask&1;
        pix_buf[pixi]=(ogg_int16_t)(pmask&pixels[x]);
        We don't use this code to allow the user to pass in a buffer that is
         the exact size of the displayed image, not the size padded to a
         multiple of 16.
        In the latter case, we might segfault on pixels[x] if it is not mapped
         to a valid page.*/
        mask>>=1;
      }
      pixels+=_ystride;
    }
    bii=_frag->border-_enc->state.borders;
    oc_fdct8x8_border(_frag->border,_enc->border_info+bii,_dct_vals,pix_buf);
  }
  /*Otherwise, copy all the pixels in the fragment and do a normal DCT.*/
  else{
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++)pix_buf[pixi]=(ogg_int16_t)(pixels[x]-128);
      pixels+=_ystride;
    }
    oc_fdct8x8(_dct_vals,pix_buf);
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
  int            bii;
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
      bii=_frag->border-_enc->state.borders;
      oc_fdct8x8_border(_frag->border,_enc->border_info+bii,_dct_vals,
       pix_buf);
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
      bii=_frag->border-_enc->state.borders;
      oc_fdct8x8_border(_frag->border,_enc->border_info+bii,_dct_vals,
       pix_buf);
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

/*Computes the SAD value of a fragment in the input image with respect to its
   motion compensated predictor..
  _frag:     The fragment to find the SAD of.
  _dx:       The X component of the motion vector.
  _dy:       The Y component of the motion vector.
  _pli:      The color plane the fragment belongs to.
  _frame:    The reference frame to predict from.*/
static int oc_enc_frag_sad(oc_enc_ctx *_enc,oc_fragment *_frag,int _dx,
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
            For more details, see oc_enc_frag_quant_tokenize().*/
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
  _efrag:  The encoder information for the fragment to quantize.
  _fquant: The forward quantization matrix to use.
  _iquant: The inverse quantization matrix to use.*/
static int oc_enc_frag_quant_tokenize(oc_enc_ctx *_enc,
 oc_fragment_enc_info *_efrag,const ogg_uint16_t _fquant[64],
 const ogg_uint16_t _iquant[64]){
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
      _efrag->dct_coeffs[ci]=0;
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
        _efrag->dct_coeffs[ci]=0;
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
            _efrag->dct_coeffs[ci]=(ogg_int16_t)qc_signed[c_sign];
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
            _efrag->dct_coeffs[ci]=(ogg_int16_t)qc_signed[c_sign];
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
            _efrag->dct_coeffs[ci]=(ogg_int16_t)((-c_sign<<1)+1);
          }break;
          case 2:{
            _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
             (unsigned char)(OC_TWO_TOKEN+c_sign);
            _efrag->dct_coeffs[ci]=(ogg_int16_t)((-c_sign<<2)+2);
          }break;
          default:{
            if(qc_min-3<OC_NDCT_VAL_CAT2_SIZE){
              _enc->dct_tokens[zzi][_enc->ndct_tokens[zzi]++]=
               (unsigned char)(OC_DCT_VAL_CAT2+qc_min-3);
              _enc->extra_bits[zzi][_enc->nextra_bits[zzi]++]=
               (ogg_uint16_t)c_sign;
              qc_signed[0]=qc_min;
              qc_signed[1]=-qc_min;
              _efrag->dct_coeffs[ci]=(ogg_int16_t)qc_signed[c_sign];
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
              _efrag->dct_coeffs[ci]=(ogg_int16_t)qc_signed[c_sign];
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

static void oc_enc_residual_tokenize(oc_enc_ctx *_enc){
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
      int                   fragi;
      int                   qti;
      int                   nnzc;
      fragi=*coded_fragi;
      frag=_enc->state.frags+fragi;
      efrag=_enc->frinfo+fragi;
      qti=frag->mbmode!=OC_MODE_INTRA;
      iquants=_enc->state.dequant_tables[qti][pli];
      nnzc=oc_enc_frag_quant_tokenize(_enc,efrag,
       _enc->enquant_tables[qti][pli][frag->qi],iquants[frag->qi]);
      /*While we're here and things are in cache, reconstruct the quantized
         fragment.*/
      oc_state_frag_recon(&_enc->state,frag,pli,efrag->dct_coeffs,nnzc,nnzc,
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

/*Encodes the current frame as a key frame.
  The result is stored in the opb field, and the packet state is updated to
   indicate a new packet is ready.
  Return: 0 on success, or a negative value on error.*/
static int oc_enc_keyframe(oc_enc_ctx *_enc){
  _enc->state.frame_type=OC_INTRA_FRAME;
  oc_enc_quant_sel_quality(_enc,1);
  oc_enc_mark_all_intra(_enc);
  oc_enc_quant_dc(_enc);
  oc_enc_residual_tokenize(_enc);
  oggpackB_reset(&_enc->opb);
  oc_enc_frame_header_pack(_enc);
  oc_enc_block_qis_pack(_enc);
  oc_enc_residual_tokens_pack(_enc);
  /*Success: Mark the packet as ready to be flushed.*/
  _enc->packet_state=OC_PACKET_READY;
  return 0;
}



/*Writes the bit flags for whether or not each super block is partially coded
   or not.
  These flags are run-length encoded, with the flag value alternating between
   each run.
  Return: The number of bits written.*/
static int oc_enc_partial_sb_flags_pack(oc_enc_ctx *_enc){
  oc_sb    *sb;
  oc_sb    *sb_end;
  unsigned  flag;
  int       run_count;
  int       ret;
  /*Write the list of partially coded super block flags.*/
  flag=_enc->state.sbs[0].coded_partially;
  oggpackB_write(&_enc->opb_coded_flags,flag,1);
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
      ret+=oc_sb_run_pack(&_enc->opb_coded_flags,4129);
      run_count-=4129;
      oggpackB_write(&_enc->opb_coded_flags,flag,1);
      ret++;
    }
    /*Encode the last run.*/
    ret+=oc_sb_run_pack(&_enc->opb_coded_flags,run_count);
    flag=!flag;
    /*If there are more sbs to come, and we had a run of 4129 exactly,
       encode the flipped bit.*/
    if(run_count==4129&&sb<sb_end){
      oggpackB_write(&_enc->opb_coded_flags,flag,1);
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
static int oc_enc_coded_sb_flags_pack(oc_enc_ctx *_enc){
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
  oggpackB_write(&_enc->opb_coded_flags,flag,1);
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
      ret+=oc_sb_run_pack(&_enc->opb_coded_flags,4129);
      run_count-=4129;
      oggpackB_write(&_enc->opb_coded_flags,flag,1);
      ret++;
    }
    /*Encode the last run.*/
    ret+=oc_sb_run_pack(&_enc->opb_coded_flags,run_count);
    flag=!flag;
    if(run_count==4129&&sb<sb_end){
      oggpackB_write(&_enc->opb_coded_flags,flag,1);
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
static int oc_enc_coded_block_flags_pack(oc_enc_ctx *_enc){
  int flag;
  int run_count;
  int bli;
  int ret;
  if(_enc->nblock_coded_flags<=0)return 0;
  flag=_enc->block_coded_flags[0];
  oggpackB_write(&_enc->opb_coded_flags,flag,1);
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
    ret+=oc_block_run_pack(&_enc->opb_coded_flags,run_count);
    flag=!flag;
  }
  return ret;
}

/*Marks each fragment as coded or not, based on the coefficient-level
   thresholds computed in the psychovisual stage.
  The MB mode of the fragments are not set, as they will be computed in
   oc_enc_choose_mbmodes().
  This also builds up the coded fragment and uncoded fragment lists.
  The coded MB list is not built up.
  That is done during mode decision.*/
static void oc_enc_mark_coded(oc_enc_ctx *_enc){
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
              oc_enc_frag_intra_fdct(_enc,frag,dct_buf,ystride,prev_refi);
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
              if((unsigned)abs(dct_buf[0]-efrag->dct_coeffs[0])>
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
  char                    last_mv[2][2];
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
      char                  bmvs[2][4][2];
      char                  mbmv[2];
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
            if(_enc->qis[fti][qii]<_enc->qis[fti][best_qii]&&
             efrag->qi_min[qti]<=_enc->qis[fti][qii]){
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
          efrag->qii=frag_qii[codedi][OC_INTER_FRAME][1];
          frag->qi=qi;
        }
        intra_bits+=mbintrabits+128>>8;
        inter_bits+=bits[OC_MODE_INTER_NOMV]+128>>8;
        continue;
      }
      /*Otherwise, add this to the coded MB list.*/
      _enc->state.coded_mbis[_enc->state.ncoded_mbis++]=mbi;
      /*Compute the chroma MVs for the 4MV mode.*/
      (*set_chroma_mvs)(bmvs[1],bmvs[0]);
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
      for(modei=0;modei<OC_NMODES;modei++)bits[modei]=bits[modei]+128>>8;
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
      mb->mode=0;
      for(modei=1;modei<OC_NMODES;modei++)if(bits[modei]<bits[mb->mode]){
        mb->mode=modei;
      }
#if defined(OC_BITRATE_STATS)
      /*Remember the error for the mode we selected in each fragment.*/
      for(codedi=0;codedi<ncoded;codedi++){
        mapi=coded[codedi];
        fragi=mb->map[mapi>>2][mapi&3];
        efrag=_enc->frinfo+fragi;
        efrag->eerror=err[mb->mode][mapi];
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
         frag_qii[codedi][OC_INTER_FRAME][mb->mode!=0];
        frag->qi=_enc->qis[OC_INTER_FRAME][efrag->qii];
      }
      inter_bits+=bits[mb->mode];
      intra_bits+=mbintrabits+128>>8;
      oc_mode_scheme_chooser_update(&_enc->mode_scheme_chooser,mb->mode);
      switch(mb->mode){
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
          /*TODO: Handle the case when less than 4 luma blocks are being
             coded.*/
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
      }
      if(OC_MODE_HAS_MV[mb->mode]){
        /*Special case 4MV mode.
          MVs are stored in bmvs.*/
        if(mb->mode==OC_MODE_INTER_MV_FOUR){
          for(codedi=0;codedi<ncoded;codedi++){
            mapi=coded[codedi];
            pli=mapi>>2;
            bi=mapi&3;
            fragi=mb->map[pli][bi];
            frag=_enc->state.frags+fragi;
            frag->mbmode=mb->mode;
            frag->mv[0]=bmvs[!!pli][bi][0];
            frag->mv[1]=bmvs[!!pli][bi][1];
          }
        }
        /*For every other mode with a MV, it is stored in mbmv.*/
        else{
          for(codedi=0;codedi<ncoded;codedi++){
            mapi=coded[codedi];
            fragi=mb->map[mapi>>2][mapi&3];
            frag=_enc->state.frags+fragi;
            frag->mbmode=mb->mode;
            frag->mv[0]=mbmv[0];
            frag->mv[1]=mbmv[1];
          }
        }
      }
      /*For modes with no MV, ensure 0,0 is stored in each fragment.*/
      else{
        for(codedi=0;codedi<ncoded;codedi++){
          mapi=coded[codedi];
          fragi=mb->map[mapi>>2][mapi&3];
          frag=_enc->state.frags+fragi;
          frag->mbmode=mb->mode;
          frag->mv[0]=frag->mv[1]=0;
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
   _enc->mode_scheme_chooser.scheme_bits[
   _enc->mode_scheme_chooser.scheme_list[0]];
  /*The easiest way to count the bits needed for coded/not coded fragments is
     to code them.
    We need to do this anyway, might as well do it now.*/
  oggpackB_reset(&_enc->opb_coded_flags);
  inter_bits+=oc_enc_partial_sb_flags_pack(_enc);
  inter_bits+=oc_enc_coded_sb_flags_pack(_enc);
  inter_bits+=oc_enc_coded_block_flags_pack(_enc);
  if(intra_bits>inter_bits){
    _enc->est_bits=inter_bits;
    /*Select the quantizer list for INTER frames.*/
    _enc->state.nqis=_enc->nqis[OC_INTER_FRAME];
    for(qii=0;qii<_enc->state.nqis;qii++){
      _enc->state.qis[qii]=_enc->qis[OC_INTER_FRAME][qii];
    }
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
          _enc->est_bits=inter_bits;
          return OC_INTER_FRAME;
        }
      }
    }
  }
  /*So, we've compared the full cost estimates, and INTRA is still better.
    Code an INTRA frame instead.*/
  oc_enc_mark_all_intra(_enc);
  _enc->est_bits=intra_bits;
  return OC_INTRA_FRAME;
}

/*Performs a motion-compensated fDCT for each fragment coded in a mode other
   than INTRA.*/
static void oc_enc_do_inter_dcts(oc_enc_ctx *_enc){
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

static void oc_enc_mb_modes_pack(oc_enc_ctx *_enc){
  const theora_huff_code *codes;
  const int              *mode_ranks;
  int                    *coded_mbi;
  int                    *coded_mbi_end;
  int                     scheme;
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
    const theora_huff_code *code;
    oc_mb                  *mb;
    mb=_enc->state.mbs+*coded_mbi;
    code=codes+mode_ranks[mb->mode];
    oggpackB_write(&_enc->opb,code->pattern,code->nbits);
  }
}

static void oc_enc_mv_pack(oc_enc_ctx *_enc,int _dx,int _dy){
  const theora_huff_code *code;
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

static int oc_enc_deltaframe(oc_enc_ctx *_enc){
  oc_enc_mark_coded(_enc);
  oc_enc_quant_sel_quality(_enc,0);
  _enc->state.frame_type=oc_enc_choose_mbmodes(_enc);
  if(_enc->state.frame_type==OC_INTER_FRAME)oc_enc_do_inter_dcts(_enc);
  oc_enc_quant_dc(_enc);
  oc_enc_residual_tokenize(_enc);
  oggpackB_reset(&_enc->opb);
  oc_enc_frame_header_pack(_enc);
  if(_enc->state.frame_type==OC_INTER_FRAME){
    oggpackB_writecopy(&_enc->opb,
     oggpackB_get_buffer(&_enc->opb_coded_flags),
     oggpackB_bits(&_enc->opb_coded_flags));
    oc_enc_mb_modes_pack(_enc);
    oc_enc_mvs_pack(_enc);
  }
  oc_enc_block_qis_pack(_enc);
  /*Pack the quantized DCT coefficients.*/
  oc_enc_residual_tokens_pack(_enc);
  /*Success: Mark the packet as ready to be flushed.*/
  _enc->packet_state=OC_PACKET_READY;
  return 0;
}


static int oc_enc_init(oc_enc_ctx *_enc,const theora_info *_info){
  int bii;
  int ret;
  /*Initialize the shared encoder/decoder state.*/
  ret=oc_state_init(&_enc->state,_info);
  if(ret<0)return ret;
  _enc->block_coded_flags=_ogg_calloc(_enc->state.nfrags,
   sizeof(_enc->block_coded_flags[0]));
  /*Compute the matrices needed to extend the frame into the border regions.*/
  for(bii=0;bii<_enc->state.nborders;bii++){
    oc_border_enc_info_init(_enc->border_info+bii,_enc->state.borders+bii);
  }
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
  /*Map the qi to a multiple of JND values.*/
  _enc->qscale=_info->quality>=63?0.5F:
   1.5F*OC_POWF(2,0.0625F*(64-_info->quality));
  /*Initialize the motion compensation, high-level importance map, and
     low-level psychovisual model plug-ins.*/
  _enc->mcenc=oc_mcenc_alloc(_enc);
  _enc->impmap=oc_impmap_alloc(_enc);
  _enc->psych=oc_psych_alloc(_enc);
  /*Reset the packet-out state machine.*/
  _enc->packet_state=OC_PACKET_INFO_HDR;
  /*Set the Huffman codes and quantization parameters to the defaults.*/
  memcpy(_enc->huff_codes,OC_VP31_HUFF_CODES,sizeof(_enc->huff_codes));
  oc_enc_set_quant_params(_enc,NULL);
  return 0;
}

static void oc_enc_clear(oc_enc_ctx *_enc){
  oc_psych_free(_enc->psych);
  oc_impmap_free(_enc->impmap);
  oc_mcenc_free(_enc->mcenc);
  oc_free_2d(_enc->extra_bits);
  oc_free_2d(_enc->dct_tokens);
  _ogg_free(_enc->mbinfo);
  _ogg_free(_enc->frinfo);
  _ogg_free(_enc->block_coded_flags);
  oc_state_clear(&_enc->state);
}



theora_enc_ctx *theora_encode_alloc(const theora_info *_info){
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



typedef theora_huff_code theora_huff_table[OC_NDCT_TOKENS];

int theora_encode_ctl(theora_enc_ctx *_enc,int _req,void *_buf,
 size_t _buf_sz){
  switch(_req){
    case OC_ENCCTL_SET_HUFFMAN_CODES:{
      if(_buf==NULL&&_buf_sz!=0||_buf!=NULL&&
       _buf_sz!=sizeof(theora_huff_code)*OC_NHUFFMAN_TABLES*OC_NDCT_TOKENS){
        return OC_EINVAL;
      }
      return oc_enc_set_huffman_codes(_enc,(theora_huff_table *)_buf);
    }break;
    case OC_ENCCTL_SET_QUANT_PARAMS:{
       if(_buf==NULL&&_buf_sz!=0||
        _buf!=NULL&&_buf_sz!=sizeof(theora_quant_info)){
         return OC_EINVAL;
       }
       return oc_enc_set_quant_params(_enc,(theora_quant_info *)_buf);
    }break;
    case OC_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE:{
       ogg_uint32_t keyframe_frequency_force;
       if(_enc==NULL||_buf==NULL)return OC_FAULT;
       if(_buf_sz!=sizeof(ogg_uint32_t))return OC_EINVAL;
       keyframe_frequency_force=*(ogg_uint32_t *)_buf;
       if(_enc->packet_state==OC_PACKET_INFO_HDR){
         /*It's still early enough to enlarge keyframe_granule_shift.*/
         _enc->state.info.keyframe_granule_shift=
          OC_MAXI(_enc->state.info.keyframe_granule_shift,
          OC_MINI(31,oc_ilog(keyframe_frequency_force-1)));
       }
       _enc->keyframe_frequency_force=OC_MINI(keyframe_frequency_force,
        1<<_enc->state.info.keyframe_granule_shift);
       (*(ogg_uint32_t *)_buf)=_enc->keyframe_frequency_force;
       return 0;
    }break;
    default:return OC_IMPL;
  }
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
  if(!(_enc->state.info.pixel_fmt&1))cwidth>>=1;
  cheight=_enc->state.info.height;
  if(!(_enc->state.info.pixel_fmt&2))cheight>>=1;
  if(_img[1].width!=cwidth||_img[2].width!=cwidth||
   _img[1].height!=cheight||_img[2].height!=cheight){
    return OC_EINVAL;
  }
  /*Flip the input buffer upside down.*/
  oc_ycbcr_buffer_flip(img,_img);
  /*Step 2: Update state.*/
  if(_enc->state.ref_frame_idx[OC_FRAME_SELF]>=0){
    /*Right now the reconstructed frame has only the coded blocks in it.
      We either need to copy all the other blocks into it, or copy the
       reconstructed blocks back into the previous frame, whichever is
       faster.*/
    if(_enc->state.frame_type==OC_INTRA_FRAME){
      /*Intra frames always code all fragments, so there is nothing to copy.
        The new frame becomes both the previous and gold reference frames.*/
      _enc->state.keyframe_num=_enc->state.curframe_num;
      _enc->state.ref_frame_idx[OC_FRAME_GOLD]=
       _enc->state.ref_frame_idx[OC_FRAME_PREV]=
       _enc->state.ref_frame_idx[OC_FRAME_SELF];
    }
    else{
      int *plfragis[3];
      int  ncoded;
      int  nuncoded;
      ncoded=_enc->state.ncoded_fragis[0]+_enc->state.ncoded_fragis[1]+
       _enc->state.ncoded_fragis[2];
      nuncoded=_enc->state.nfrags-ncoded;
      if(ncoded<nuncoded&&
       _enc->state.ref_frame_idx[OC_FRAME_PREV]!=
       _enc->state.ref_frame_idx[OC_FRAME_GOLD]){
        plfragis[0]=_enc->state.coded_fragis;
        plfragis[1]=plfragis[0]+_enc->state.ncoded_fragis[0];
        plfragis[2]=plfragis[1]+_enc->state.ncoded_fragis[1];
        oc_state_frag_copy(&_enc->state,plfragis,_enc->state.ncoded_fragis,
         OC_FRAME_PREV,OC_FRAME_SELF);
        _enc->state.ref_frame_idx[OC_FRAME_SELF]=
         _enc->state.ref_frame_idx[OC_FRAME_PREV];
      }
      else{
        plfragis[0]=_enc->state.uncoded_fragis-_enc->state.nuncoded_fragis[0];
        plfragis[1]=plfragis[0]-_enc->state.nuncoded_fragis[1];
        plfragis[2]=plfragis[1]-_enc->state.nuncoded_fragis[2];
        oc_state_frag_copy(&_enc->state,plfragis,_enc->state.nuncoded_fragis,
         OC_FRAME_SELF,OC_FRAME_PREV);
        _enc->state.ref_frame_idx[OC_FRAME_PREV]=
         _enc->state.ref_frame_idx[OC_FRAME_SELF];
      }
    }
    /*Filter block edges.*/
    oc_state_loop_filter(&_enc->state,OC_FRAME_PREV);
#if defined(OC_DUMP_IMAGES)
    oc_state_dump_frame(&_enc->state,OC_FRAME_PREV,"rec");
#endif
    /*Fill in the borders from the reconstructed version of the last encoded
       frame.*/
    oc_state_borders_fill(&_enc->state,
     _enc->state.ref_frame_idx[OC_FRAME_PREV]);
  }
  /*Select a free buffer to use for the reconstructed version of this frame.*/
  for(rfi=0;rfi==_enc->state.ref_frame_idx[OC_FRAME_GOLD]||
   rfi==_enc->state.ref_frame_idx[OC_FRAME_PREV];rfi++);
  _enc->state.ref_frame_idx[OC_FRAME_SELF]=rfi;
  /*Move the motion vector predictors back a frame.*/
  for(mbi=_enc->state.fplanes[0].nsbs<<2;mbi-->0;){
    oc_mb_enc_info *emb;
    emb=_enc->mbinfo+mbi;
    memmove(emb->mvs+1,emb->mvs,2*sizeof(emb->mvs[0]));
  }
  _enc->state.curframe_num++;
  /*Fill the fragment array with pointers into the user buffer.*/
  oc_state_fill_buffer_ptrs(&_enc->state,OC_FRAME_IO,img);
  /*Step 3: Analyze motion.*/
  oc_mcenc_analyze(_enc->mcenc);
  /*Step 4: Compute importance map.*/
  oc_impmap_fill(_enc->impmap,
   _enc->state.info.fps_denominator/(float)_enc->state.info.fps_numerator);
  /*Step 5: Compute coefficient tolerances.*/
  oc_psych_scan(_enc->psych,0);
  /*Step 6: Encode!*/
  if(_enc->state.curframe_num==0||
   _enc->state.curframe_num-_enc->state.keyframe_num>=
   _enc->keyframe_frequency_force){
    ret=oc_enc_keyframe(_enc);
    if(ret<0)return ret;
  }
  else{
    ret=oc_enc_deltaframe(_enc);
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
     _enc->state.curframe_num<<_enc->state.info.keyframe_granule_shift;
  }
  else{
    _enc->state.granpos=
     (_enc->state.keyframe_num<<_enc->state.info.keyframe_granule_shift)+
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
