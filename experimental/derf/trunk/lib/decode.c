#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "decint.h"
#if defined(OC_DUMP_IMAGES)
# include <stdio.h>
# include "png.h"
#endif



/*The mode alphabets for the various mode coding schemes.
  Scheme 0 uses a custom alphabet, which is not stored in this table.*/
static const int OC_MODE_ALPHABETS[7][OC_NMODES]={
  /*Last MV dominates */
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV_LAST2,OC_MODE_INTER_MV,
    OC_MODE_INTER_NOMV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV_LAST2,OC_MODE_INTER_NOMV,
    OC_MODE_INTER_MV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV,OC_MODE_INTER_MV_LAST2,
    OC_MODE_INTER_NOMV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV,OC_MODE_INTER_NOMV,
    OC_MODE_INTER_MV_LAST2,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,
    OC_MODE_GOLDEN_MV,OC_MODE_INTER_MV_FOUR
  },
  /*No MV dominates.*/
  {
    OC_MODE_INTER_NOMV,OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV_LAST2,
    OC_MODE_INTER_MV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_NOMV,OC_MODE_GOLDEN_NOMV,OC_MODE_INTER_MV_LAST,
    OC_MODE_INTER_MV_LAST2,OC_MODE_INTER_MV,OC_MODE_INTRA,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  /*Default ordering.*/
  {
    OC_MODE_INTER_NOMV,OC_MODE_INTRA,OC_MODE_INTER_MV,OC_MODE_INTER_MV_LAST,
    OC_MODE_INTER_MV_LAST2,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  }
};



static int oc_sb_run_unpack(oggpack_buffer *_opb){
  int bits;
  /*Coding scheme:
       Codeword            Run Length
     0                       1
     10x                     2-3
     110x                    4-5
     1110xx                  6-9
     11110xxx                10-17
     111110xxxx              18-33
     111111xxxxxxxxxxxx      34-4129*/
  bits=oggpackB_read1(_opb);
  if(bits==0)return 1;
  bits=oggpackB_read(_opb,2);
  if((bits&2)==0)return 2+(bits&1);
  else if((bits&1)==0)return 4+(oggpackB_read1(_opb)&1);
  bits=oggpackB_read(_opb,3);
  if((bits&4)==0)return 6+(bits&3);
  else if((bits&2)==0)return 10+((bits&1)<<2)+(oggpackB_read(_opb,2)&3);
  else if((bits&1)==0)return 18+(oggpackB_read(_opb,4)&0xF);
  return 34+(oggpackB_read(_opb,12)&0xFFF);
}

static int oc_block_run_unpack(oggpack_buffer *_opb){
  int bits;
  /*Coding scheme:
     Codeword             Run Length
     0x                      1-2
     10x                     3-4
     110x                    5-6
     1110xx                  7-10
     11110xx                 11-14
     11111xxxx               15-30*/
  bits=oggpackB_read(_opb,2);
  if((bits&2)==0)return 1+(bits&1);
  else if((bits&1)==0)return 3+(oggpackB_read1(_opb)&1);
  bits=oggpackB_read(_opb,2);
  if((bits&2)==0)return 5+(bits&1);
  else if((bits&1)==0)return 7+(oggpackB_read(_opb,2)&3);
  if(!oggpackB_read1(_opb))return 11+(oggpackB_read(_opb,2)&3);
  return 15+(oggpackB_read(_opb,4)&0xF);
}



static void oc_dec_init(oc_dec_ctx *_dec,const theora_info *_info,
 const theora_setup_info *_setup){
  int qti;
  int pli;
  oc_state_init(&_dec->state,_info);
  oc_huff_trees_copy(_dec->huff_tables,_setup->huff_tables);
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    _dec->state.dequant_tables[qti][pli]=
     _dec->state.dequant_table_data[qti]+pli;
  }
  oc_dequant_tables_init(_dec->state.dequant_tables,_dec->pp_dc_scale,
   &_setup->qinfo);
  _dec->dct_tokens=(unsigned char **)oc_calloc_2d(64,
   _dec->state.nfrags,sizeof(_dec->dct_tokens[0][0]));
  _dec->extra_bits=(ogg_uint16_t **)oc_calloc_2d(64,
   _dec->state.nfrags,sizeof(_dec->extra_bits[0][0]));
  memcpy(_dec->state.loop_filter_limits,_setup->qinfo.loop_filter_limits,
   sizeof(_dec->state.loop_filter_limits));
}

static void oc_dec_clear(oc_dec_ctx *_dec){
  oc_free_2d(_dec->extra_bits);
  oc_free_2d(_dec->dct_tokens);
  oc_huff_trees_clear(_dec->huff_tables);
  oc_state_clear(&_dec->state);
}


static int oc_dec_frame_header_unpack(oc_dec_ctx *_dec){
  /*Check to make sure this is a data packet.*/
  if(oggpackB_read1(&_dec->opb)!=0)return OC_BADPACKET;
  /*Read in the frame type (I or P).*/
  _dec->state.frame_type=oggpackB_read1(&_dec->opb);
  /*Read in the current qi.*/
  _dec->state.qi=oggpackB_read(&_dec->opb,6);
  /*TODO: Support multiple qi's.*/
  if(oggpackB_read1(&_dec->opb)!=0)return OC_IMPL;
  if(_dec->state.frame_type==OC_INTRA_FRAME){
    /*Keyframes have 3 unused configuration bits, holdovers from VP3 days.
      Most of the other unused bits in the VP3 headers were eliminated.
      I don't know why these remain.*/
    if(oggpackB_read(&_dec->opb,3)!=0)return OC_IMPL;
  }
  return 0;
}

/*Mark all fragments as coded and in OC_MODE_INTRA.
  This also builds up the coded fragment list (in coded order), and clears the
   uncoded fragment list.
  It does not update the coded macro block list, as that is not used when
   decoding INTRA frames.*/
static void oc_dec_mark_all_intra(oc_dec_ctx *_dec){
  oc_sb *sb;
  oc_sb *sb_end;
  int    pli;
  int    ncoded_fragis;
  int    prev_ncoded_fragis;
  prev_ncoded_fragis=ncoded_fragis=0;
  sb=sb_end=_dec->state.sbs;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    fplane=_dec->state.fplanes+pli;
    sb_end+=fplane->nsbs;
    for(;sb<sb_end;sb++){
      int mbi;
      for(mbi=0;mbi<4;mbi++)if(sb->mb_valid&1<<mbi){
        int bi;
        for(bi=0;bi<4;bi++){
          int fragi;
          fragi=sb->map[mbi][bi];
          if(fragi>=0){
            oc_fragment *frag;
            frag=_dec->state.frags+fragi;
            frag->coded=1;
            frag->mbmode=OC_MODE_INTRA;
            _dec->state.coded_fragis[ncoded_fragis++]=fragi;
          }
        }
      }
    }
    _dec->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
    _dec->state.nuncoded_fragis[pli]=0;
  }
}

/*Decodes the bit flags for whether or not each super block is partially coded
   or not.
  Return: The number of partially coded super blocks.*/
static int oc_dec_partial_sb_flags_unpack(oc_dec_ctx *_dec){
  oc_sb *sb;
  oc_sb *sb_end;
  int   flag;
  int   npartial;
  int   run_count;
  flag=oggpackB_read1(&_dec->opb);
  sb=_dec->state.sbs;
  sb_end=sb+_dec->state.nsbs;
  run_count=npartial=0;
  while(sb<sb_end){
    int full_run;
    run_count=oc_sb_run_unpack(&_dec->opb);
    full_run=run_count>=4129;
    do{
      sb->coded_partially=flag;
      sb->coded_fully=0;
      npartial+=flag;
      sb++;
    }
    while(--run_count>0&&sb<sb_end);
    if(full_run&&sb<sb_end)flag=oggpackB_read1(&_dec->opb);
    else flag=!flag;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
  return npartial;
}

/*Decodes the bit flags for whether or not each non-partially-coded super
   block is fully coded or not.
  This function should only be called if there is at least one
   non-partially-coded super block.
  Return: The number of partially coded super blocks.*/
static void oc_dec_coded_sb_flags_unpack(oc_dec_ctx *_dec){
  oc_sb *sb;
  oc_sb *sb_end;
  int    flag;
  int    run_count;
  sb=_dec->state.sbs;
  sb_end=sb+_dec->state.nsbs;
  /*Skip partially coded super blocks.*/
  for(;sb->coded_partially;sb++);
  flag=oggpackB_read1(&_dec->opb);
  while(sb<sb_end){
    int full_run;
    run_count=oc_sb_run_unpack(&_dec->opb);
    full_run=run_count>=4129;
    for(;sb<sb_end;sb++){
      if(sb->coded_partially)continue;
      if(run_count--<=0)break;
      sb->coded_fully=flag;
    }
    if(full_run&&sb<sb_end)flag=oggpackB_read1(&_dec->opb);
    else flag=!flag;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
}

static void oc_dec_coded_flags_unpack(oc_dec_ctx *_dec){
  oc_sb *sb;
  oc_sb *sb_end;
  int    npartial;
  int    pli;
  int    flag;
  int    run_count;
  int    ncoded_fragis;
  int    prev_ncoded_fragis;
  int    nuncoded_fragis;
  int    prev_nuncoded_fragis;
  npartial=oc_dec_partial_sb_flags_unpack(_dec);
  if(npartial<_dec->state.nsbs)oc_dec_coded_sb_flags_unpack(_dec);
  if(npartial>0)flag=!oggpackB_read1(&_dec->opb);
  else flag=0;
  run_count=0;
  prev_ncoded_fragis=ncoded_fragis=prev_nuncoded_fragis=nuncoded_fragis=0;
  sb=sb_end=_dec->state.sbs;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    fplane=_dec->state.fplanes+pli;
    sb_end+=fplane->nsbs;
    for(;sb<sb_end;sb++){
      int mbi;
      for(mbi=0;mbi<4;mbi++)if(sb->mb_valid&1<<mbi){
        int bi;
        for(bi=0;bi<4;bi++){
          int fragi;
          fragi=sb->map[mbi][bi];
          if(fragi>=0){
            oc_fragment *frag;
            frag=_dec->state.frags+fragi;
            if(sb->coded_fully)frag->coded=1;
            else if(!sb->coded_partially)frag->coded=0;
            else{
              if(run_count<=0){
                run_count=oc_block_run_unpack(&_dec->opb);
                flag=!flag;
              }
              run_count--;
              frag->coded=flag;
            }
            if(frag->coded)_dec->state.coded_fragis[ncoded_fragis++]=fragi;
            else *(_dec->state.uncoded_fragis-++nuncoded_fragis)=fragi;
          }
        }
      }
    }
    _dec->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
    _dec->state.nuncoded_fragis[pli]=nuncoded_fragis-prev_nuncoded_fragis;
    prev_nuncoded_fragis=nuncoded_fragis;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
}



typedef int (*oc_mode_unpack_func)(oggpack_buffer *_opb);

static int oc_vlc_mode_unpack(oggpack_buffer *_opb){
  int i;
  for(i=0;i<7;i++)if(!oggpackB_read1(_opb))break;
  return i;
}

static int oc_clc_mode_unpack(oggpack_buffer *_opb){
  return oggpackB_read(_opb,3)&7;
}

/*Unpacks the list of macro block modes for INTER frames.*/
int oc_dec_mb_modes_unpack(oc_dec_ctx *_dec){
  oc_mode_unpack_func  mode_unpack;
  oc_mb               *mb;
  oc_mb               *mb_end;
  const int           *alphabet;
  int                  scheme0_alphabet[8];
  int                  mode_scheme;
  mode_scheme=oggpackB_read(&_dec->opb,3);
  if(mode_scheme<0)return OC_BADPACKET;
  if(mode_scheme==0){
    int mi;
    for(mi=0;mi<OC_NMODES;mi++){
      int idx;
      idx=oggpackB_read(&_dec->opb,3);
      if(idx<0)return OC_BADPACKET;
      scheme0_alphabet[idx]=OC_MODE_ALPHABETS[6][mi];
    }
    alphabet=scheme0_alphabet;
  }
  else alphabet=OC_MODE_ALPHABETS[mode_scheme-1];
  if(mode_scheme==7)mode_unpack=oc_clc_mode_unpack;
  else mode_unpack=oc_vlc_mode_unpack;
  mb=_dec->state.mbs;
  mb_end=mb+_dec->state.nmbs;
  for(;mb<mb_end;mb++)if(mb->mode!=OC_MODE_INVALID){
    int bi;
    for(bi=0;bi<4;bi++){
      int fragi;
      fragi=mb->map[0][bi];
      if(fragi>=0&&_dec->state.frags[fragi].coded)break;
    }
    if(bi<4)mb->mode=alphabet[(*mode_unpack)(&_dec->opb)];
    else mb->mode=OC_MODE_INTER_NOMV;
  }
  return 0;
}



typedef int (*oc_mv_comp_unpack_func)(oggpack_buffer *_opb);

static int oc_vlc_mv_comp_unpack(oggpack_buffer *_opb){
  int mvsigned[2];
  int bits;
  bits=oggpackB_read(_opb,3);
  switch(bits){
    case 0:return 0;
    case 1:return 1;
    case 2:return -1;
    case 3:{
      mvsigned[0]=2;
      bits=oggpackB_read1(_opb);
    }break;
    case 4:{
      mvsigned[0]=3;
      bits=oggpackB_read1(_opb);
    }break;
    case 5:{
      bits=oggpackB_read(_opb,3)&7;
      mvsigned[0]=4+(bits>>1);
    }break;
    case 6:{
      bits=oggpackB_read(_opb,4)&0xF;
      mvsigned[0]=8+(bits>>1);
    }break;
    case 7:{
      bits=oggpackB_read(_opb,5)&0x1F;
      mvsigned[0]=16+(bits>>1);
    }break;
  }
  mvsigned[1]=-mvsigned[0];
  return mvsigned[bits&1];
}

static int oc_clc_mv_comp_unpack(oggpack_buffer *_opb){
  int mvsigned[2];
  int bits;
  bits=oggpackB_read(_opb,6)&0x3F;
  mvsigned[0]=bits>>1;
  mvsigned[1]=-mvsigned[0];
  return mvsigned[bits&1];
}

/*Unpacks the list of motion vectors for INTER frames, and propagtes the macro
   block modes and motion vectors to the individual fragments.*/
static void oc_dec_mv_unpack_and_frag_modes_fill(oc_dec_ctx *_dec){
  oc_set_chroma_mvs_func  set_chroma_mvs;
  oc_mv_comp_unpack_func  mv_comp_unpack;
  oc_mb                  *mb;
  oc_mb                  *mb_end;
  const int              *map_idxs;
  int                     map_nidxs;
  char                    last_mv[2][2];
  char                    cbmvs[4][2];
  set_chroma_mvs=OC_SET_CHROMA_MVS_TABLE[_dec->state.chroma_type];
  mv_comp_unpack=oggpackB_read1(&_dec->opb)?
   oc_clc_mv_comp_unpack:oc_vlc_mv_comp_unpack;
  map_idxs=OC_MB_MAP_IDXS[_dec->state.chroma_type];
  map_nidxs=OC_MB_MAP_NIDXS[_dec->state.chroma_type];
  memset(last_mv,0,sizeof(last_mv));
  mb=_dec->state.mbs;
  mb_end=mb+_dec->state.nmbs;
  for(;mb<mb_end;mb++)if(mb->mode!=OC_MODE_INVALID){
    oc_fragment *frag;
    char         mbmv[2];
    int          coded[12];
    int          codedi;
    int          ncoded;
    int          mapi;
    int          mapii;
    int          fragi;
    /*Search for at least one coded fragment.*/
    ncoded=mapii=0;
    do{
      mapi=map_idxs[mapii];
      fragi=mb->map[mapi>>2][mapi&3];
      if(fragi>=0&&_dec->state.frags[fragi].coded)coded[ncoded++]=mapi;
    }
    while(++mapii<map_nidxs);
    if(ncoded<=0)continue;
    switch(mb->mode){
      case OC_MODE_INTER_MV_FOUR:{
        char bmvs[4][2];
        int  bi;
        for(bi=0;bi<4;bi++){
          bmvs[bi][0]=(char)(*mv_comp_unpack)(&_dec->opb);
          bmvs[bi][1]=(char)(*mv_comp_unpack)(&_dec->opb);
        }
        last_mv[1][0]=last_mv[0][0];
        last_mv[1][1]=last_mv[0][1];
        last_mv[0][0]=bmvs[3][0];
        last_mv[0][1]=bmvs[3][1];
        for(codedi=0;codedi<ncoded&&coded[codedi]<4;codedi++){
          bi=coded[codedi];
          fragi=mb->map[0][bi];
          frag=_dec->state.frags+fragi;
          frag->mbmode=mb->mode;
          frag->mv[0]=bmvs[bi][0];
          frag->mv[1]=bmvs[bi][1];
        }
        if(codedi<ncoded){
          (*set_chroma_mvs)(cbmvs,bmvs);
          for(;codedi<ncoded;codedi++){
            mapi=coded[codedi];
            bi=mapi&3;
            fragi=mb->map[mapi>>2][bi];
            frag=_dec->state.frags+fragi;
            frag->mbmode=mb->mode;
            frag->mv[0]=cbmvs[bi][0];
            frag->mv[1]=cbmvs[bi][1];
          }
        }
      }break;
      case OC_MODE_INTER_MV:{
        last_mv[1][0]=last_mv[0][0];
        last_mv[1][1]=last_mv[0][1];
        mbmv[0]=last_mv[0][0]=(char)(*mv_comp_unpack)(&_dec->opb);
        mbmv[1]=last_mv[0][1]=(char)(*mv_comp_unpack)(&_dec->opb);
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
      case OC_MODE_GOLDEN_MV:{
        mbmv[0]=(char)(*mv_comp_unpack)(&_dec->opb);
        mbmv[1]=(char)(*mv_comp_unpack)(&_dec->opb);
      }break;
      default:mbmv[0]=mbmv[1]=0;break;
    }
    /*4MV mode fills in the fragments itself.
      For all other modes we can use this common code.*/
    if(mb->mode!=OC_MODE_INTER_MV_FOUR){
      for(codedi=0;codedi<ncoded;codedi++){
        mapi=coded[codedi];
        fragi=mb->map[mapi>>2][mapi&3];
        frag=_dec->state.frags+fragi;
        frag->mbmode=mb->mode;
        frag->mv[0]=mbmv[0];
        frag->mv[1]=mbmv[1];
      }
    }
  }
}



/*Returns the decoded value of the given token.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: The decoded coefficient value.*/
typedef int (*oc_token_dec1val_func)(int _token,int _extra_bits);

/*Handles zero run tokens.*/
static int oc_token_dec1val_zrl(void){
  return 0;
}

/*Handles 1, -1, 2 and -2 tokens.*/
static int oc_token_dec1val_const(int _token){
  static const int CONST_VALS[4]={1,-1,2,-2};
  return CONST_VALS[_token-OC_NDCT_ZRL_TOKEN_MAX];
}

/*Handles DCT value tokens category 2.*/
static int oc_token_dec1val_cat2(int _token,int _extra_bits){
  int valsigned[2];
  valsigned[0]=_token-OC_DCT_VAL_CAT2+3;
  valsigned[1]=-valsigned[0];
  return valsigned[_extra_bits];
}

/*Handles DCT value tokens categories 3 through 8.*/
static int oc_token_dec1val_cati(int _token,int _extra_bits){
  static const int VAL_CAT_OFFS[6]={
    OC_NDCT_VAL_CAT2_SIZE+3,
    OC_NDCT_VAL_CAT2_SIZE+5,
    OC_NDCT_VAL_CAT2_SIZE+9,
    OC_NDCT_VAL_CAT2_SIZE+17,
    OC_NDCT_VAL_CAT2_SIZE+33,
    OC_NDCT_VAL_CAT2_SIZE+65
  };
  static const int VAL_CAT_MASKS[6]={
    0x001,0x003,0x007,0x00F,0x01F,0x1FF
  };
  static const int VAL_CAT_SHIFTS[6]={1,2,3,4,5,9};
  int valsigned[2];
  int cati;
  cati=_token-OC_NDCT_VAL_CAT2_MAX;
  valsigned[0]=VAL_CAT_OFFS[cati]+(_extra_bits&VAL_CAT_MASKS[cati]);
  valsigned[1]=-valsigned[0];
  return valsigned[_extra_bits>>VAL_CAT_SHIFTS[cati]&1];
}

/*A jump table for compute the first coefficient value the given token value
   represents.*/
static const oc_token_dec1val_func OC_TOKEN_DEC1VAL_TABLE[OC_NDCT_TOKENS-
 OC_NDCT_EOB_TOKEN_MAX]={
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cati,
  oc_token_dec1val_cati,
  oc_token_dec1val_cati,
  oc_token_dec1val_cati,
  oc_token_dec1val_cati,
  oc_token_dec1val_cati,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl
};

/*Returns the decoded value of the given token.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: The decoded coefficient value.*/
int oc_dct_token_dec1val(int _token,int _extra_bits){
  return (*OC_TOKEN_DEC1VAL_TABLE[_token-OC_NDCT_EOB_TOKEN_MAX])(_token,
   _extra_bits);
}

/*Unpacks the DC coefficient tokens.
  Unlike when unpacking the AC coefficient tokens, we actually need to decode
   the DC coefficient values now so that we can do DC prediction.
  _huff_idx:   The index of the Huffman table to use for each color plane.
  _ntoks_left: The number of tokens left to be decoded in each color plane for
                each coefficient.
               This is updated as EOB tokens and zero run tokens are decoded.
  Return: The length of any outstanding EOB run.*/
static int oc_dec_dc_coeff_unpack(oc_dec_ctx *_dec,int _huff_idxs[3],
 int _ntoks_left[3][64]){
  int *coded_fragi;
  int *coded_fragi_end;
  int  run_counts[64];
  int  cfi;
  int  eobi;
  int  eobs;
  int  ti;
  int  ebi;
  int  pli;
  int  rli;
  eobs=0;
  ti=ebi=0;
  coded_fragi_end=coded_fragi=_dec->state.coded_fragis;
  for(pli=0;pli<3;pli++){
    coded_fragi_end+=_dec->state.ncoded_fragis[pli];
    memset(run_counts,0,sizeof(run_counts));
    /*Continue any previous EOB run, if there was one.*/
    for(eobi=eobs;eobi-->0&&coded_fragi<coded_fragi_end;){
      _dec->state.frags[*coded_fragi++].dc=0;
    }
    cfi=0;
    while(cfi+eobs<_ntoks_left[pli][0]){
      int token;
      int neb;
      int eb;
      int skip;
      token=oc_huff_token_decode(&_dec->opb,
       _dec->huff_tables[_huff_idxs[pli]]);
      _dec->dct_tokens[0][ti++]=(char)token;
      neb=OC_DCT_TOKEN_EXTRA_BITS[token];
      if(neb){
        eb=oggpackB_read(&_dec->opb,neb)&(1<<neb)-1;
        _dec->extra_bits[0][ebi++]=(ogg_int16_t)eb;
      }
      else eb=0;
      skip=oc_dct_token_skip(token,eb);
      if(skip<0){
        eobi=-skip;
        eobs+=eobi;
        while(eobi-->0&&coded_fragi<coded_fragi_end){
          _dec->state.frags[*coded_fragi++].dc=0;
        }
      }
      else{
        run_counts[skip-1]++;
        cfi++;
        _dec->state.frags[*coded_fragi++].dc=oc_dct_token_dec1val(token,eb);
      }
    }
    _dec->ndct_tokens[pli][0]=ti;
    /*Set the EOB count to the portion of the last EOB run which extends past
       this coefficient.*/
    eobs=eobs+cfi-_ntoks_left[pli][0];
    /*Add the total number of EOBs decoded in this coefficient to the longest
       run length.*/
    run_counts[63]+=_ntoks_left[pli][0]-cfi;
    /*And convert the run_counts array to a moment table.*/
    for(rli=63;rli-->0;)run_counts[rli]+=run_counts[rli+1];
    /*Finally, subtract off the number of coefficients that have been
       accounted for by runs started in this coefficient.*/
    for(rli=64;rli-->0;)_ntoks_left[pli][rli]-=run_counts[rli];
  }
  return eobs;
}

/*Unpacks the AC coefficient tokens.
  This can completely discard coefficient values while unpacking, and so is
   somewhat simpler than unpacking the DC coefficient tokens.
  _huff_idx:   The index of the Huffman table to use for each color plane.
  _ntoks_left: The number of tokens left to be decoded in each color plane for
                each coefficient.
               This is updated as EOB tokens and zero run tokens are decoded.
  Return: The length of any outstanding EOB run.*/
static int oc_dec_ac_coeff_unpack(oc_dec_ctx *_dec,int _zzi,int _huff_idxs[3],
 int _ntoks_left[3][64]){
  int run_counts[64];
  int cfi;
  int eobs;
  int ti;
  int ebi;
  int pli;
  int rli;
  eobs=_dec->eob_runs[_zzi];
  ti=ebi=0;
  for(pli=0;pli<3;pli++){
    memset(run_counts,0,sizeof(run_counts));
    cfi=0;
    while(cfi+eobs<_ntoks_left[pli][_zzi]){
      int token;
      int neb;
      int eb;
      int skip;
      token=oc_huff_token_decode(&_dec->opb,
       _dec->huff_tables[_huff_idxs[pli]]);
      _dec->dct_tokens[_zzi][ti++]=(char)token;
      neb=OC_DCT_TOKEN_EXTRA_BITS[token];
      if(neb){
        eb=oggpackB_read(&_dec->opb,neb)&(1<<neb)-1;
        _dec->extra_bits[_zzi][ebi++]=(ogg_int16_t)eb;
      }
      else eb=0;
      skip=oc_dct_token_skip(token,eb);
      if(skip<0)eobs-=skip;
      else{
        run_counts[skip-1]++;
        cfi++;
      }
    }
    /*TODO: We don't actually use these values for anything, yet.*/
    _dec->ndct_tokens[pli][_zzi]=ti;
    /*Set the EOB count to the portion of the last EOB run which extends past
       this coefficient.*/
    eobs=eobs+cfi-_ntoks_left[pli][_zzi];
    /*Add the total number of EOBs decoded in this coefficient to the longest
       run length.*/
    run_counts[63]+=_ntoks_left[pli][_zzi]-cfi;
    /*And convert the run_counts array to a moment table.*/
    for(rli=63;rli-->0;)run_counts[rli]+=run_counts[rli+1];
    /*Finally, subtract off the number of coefficients that have been
       accounted for by runs started in this coefficient.*/
    for(rli=64-_zzi;rli-->0;)_ntoks_left[pli][_zzi+rli]-=run_counts[rli];
  }
  return eobs;
}

/*Tokens describing the DCT coefficients that belong to each fragment are
   stored in the bistream grouped by coefficient, not by fragment.
  This means that we either decode all the tokens in order, building up a
   separate coefficient list for each fragment as we go, and then go back and
   do the iDCT on each fragment, or we have to create separate lists of tokens
   for each coefficient, so that we can pull the next token required off the
   head of the appropriate list when decoding a specific fragment.
  The former was VP3's choice, and it meant 2*w*h extra storage for all the
   decoded coefficient values.
  We take the second option, which lets us store just one or three bytes per
   token (generally far fewer than the number of coefficients, due to EOB
   tokens and zero runs), and which requires us to only maintain a counter for
   each of the 64 coefficients, instead of a counter for every fragment to
   determine where the next token goes.*/
static void oc_dec_residual_tokens_unpack(oc_dec_ctx *_dec){
  static const int OC_HUFF_LIST_MAX[5]={1,6,15,28,64};
  int ntoks_left[3][64];
  int huff_idxs[3];
  int pli;
  int zzi;
  int hgi;
  int huffi_y;
  int huffi_c;
  int eobs;
  for(pli=0;pli<3;pli++)for(zzi=0;zzi<64;zzi++){
    ntoks_left[pli][zzi]=_dec->state.ncoded_fragis[pli];
  }
  huffi_y=oggpackB_read(&_dec->opb,4)&0xF;
  huffi_c=oggpackB_read(&_dec->opb,4)&0xF;
  huff_idxs[0]=huffi_y;
  huff_idxs[1]=huff_idxs[2]=huffi_c;
  _dec->eob_runs[0]=0;
  eobs=oc_dec_dc_coeff_unpack(_dec,huff_idxs,ntoks_left);
  huffi_y=oggpackB_read(&_dec->opb,4)&0xF;
  huffi_c=oggpackB_read(&_dec->opb,4)&0xF;
  zzi=1;
  for(hgi=1;hgi<5;hgi++){
    huff_idxs[0]=huffi_y+(hgi<<4);
    huff_idxs[1]=huff_idxs[2]=huffi_c+(hgi<<4);
    for(;zzi<OC_HUFF_LIST_MAX[hgi];zzi++){
      _dec->eob_runs[zzi]=eobs;
      eobs=oc_dec_ac_coeff_unpack(_dec,zzi,huff_idxs,ntoks_left);
    }
  }
}



/*Expands a single token into the given coefficient list.
  This fills in the zeros for zero runs as well as coefficient values, and
   updates the index of the current coefficient.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to expand.
  _extra_bits: The extra bits associated with the token.
  _dct_coeffs: The current list of coefficients, in zig-zag order.
  _zzi:        A pointer to the zig-zag index of the next coefficient to write
                to.
               This is updated before the function returns.*/
typedef void (*oc_token_expand_func)(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi);

/*Expands a zero run token.*/
void oc_token_expand_zrl(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi){
  int zzi;
  zzi=*_zzi;
  do _dct_coeffs[OC_FZIG_ZAG[zzi++]]=0;
  while(_extra_bits-->0);
  *_zzi=zzi;
}

/*Expands a constant, single-value token.*/
void oc_token_expand_const(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi){
  _dct_coeffs[OC_FZIG_ZAG[(*_zzi)++]]=
   (ogg_int16_t)oc_token_dec1val_const(_token);
}

/*Expands category 2 single-valued tokens.*/
void oc_token_expand_cat2(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi){
  _dct_coeffs[OC_FZIG_ZAG[(*_zzi)++]]=
   (ogg_int16_t)oc_token_dec1val_cat2(_token,_extra_bits);
}

/*Expands category 3 through 8 single-valued tokens.*/
void oc_token_expand_cati(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi){
  _dct_coeffs[OC_FZIG_ZAG[(*_zzi)++]]=
   (ogg_int16_t)oc_token_dec1val_cati(_token,_extra_bits);
}

/*Expands a category 1a zero run/value combo token.*/
void oc_token_expand_run_cat1a(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi){
  int zzi;
  int rl;
  zzi=*_zzi;
  for(rl=_token-OC_DCT_RUN_CAT1A+1;rl-->0;)_dct_coeffs[OC_FZIG_ZAG[zzi++]]=0;
  _dct_coeffs[OC_FZIG_ZAG[zzi++]]=(ogg_int16_t)(1-(_extra_bits<<1));
  *_zzi=zzi;
}

/*Expands all other zero run/value combo tokens.*/
void oc_token_expand_run(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[64],int *_zzi){
  static const int NZEROS_ADJUST[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    6,10,1,2
  };
  static const int NZEROS_MASK[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    3,7,0,1
  };
  static const int VALUE_SHIFT[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    0,0,0,1
  };
  static const int VALUE_MASK[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    0,0,1,1
  };
  static const int VALUE_ADJUST[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    1,1,2,2
  };
  static const int SIGN_SHIFT[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    2,3,1,2
  };
  int valsigned[2];
  int zzi;
  int rl;
  _token-=OC_DCT_RUN_CAT1B;
  rl=(_extra_bits&NZEROS_MASK[_token])+NZEROS_ADJUST[_token];
  zzi=*_zzi;
  while(rl-->0)_dct_coeffs[OC_FZIG_ZAG[zzi++]]=0;
  valsigned[0]=VALUE_ADJUST[_token]+
   (_extra_bits>>VALUE_SHIFT[_token]&VALUE_MASK[_token]);
  valsigned[1]=-valsigned[0];
  _dct_coeffs[OC_FZIG_ZAG[zzi++]]=(ogg_int16_t)valsigned[
   _extra_bits>>SIGN_SHIFT[_token]];
  *_zzi=zzi;
}

/*A jump table for expanding token values into coefficient values.
  This reduces all the conditional branches, etc., needed to parse these token
   values down to one indirect jump.*/
static const oc_token_expand_func OC_TOKEN_EXPAND_TABLE[OC_NDCT_TOKENS-
 OC_NDCT_EOB_TOKEN_MAX]={
  oc_token_expand_zrl,
  oc_token_expand_zrl,
  oc_token_expand_const,
  oc_token_expand_const,
  oc_token_expand_const,
  oc_token_expand_const,
  oc_token_expand_cat2,
  oc_token_expand_cat2,
  oc_token_expand_cat2,
  oc_token_expand_cat2,
  oc_token_expand_cati,
  oc_token_expand_cati,
  oc_token_expand_cati,
  oc_token_expand_cati,
  oc_token_expand_cati,
  oc_token_expand_cati,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run,
  oc_token_expand_run,
  oc_token_expand_run,
  oc_token_expand_run
};

/*Expands a single token into the given coefficient list.
  This fills in the zeros for zero runs as well as coefficient values, and
   updates the index of the current coefficient.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to expand.
  _extra_bits: The extra bits associated with the token.
  _dct_coeffs: The current list of coefficients, in zig-zag order.
  _zzi:        A pointer to the zig-zag index of the next coefficient to write
                to.
               This is updated before the function returns.*/
static void oc_dct_token_expand(int _token,int _extra_bits,
 ogg_int16_t *_dct_coeffs,int *_zzi){
  (*OC_TOKEN_EXPAND_TABLE[_token-OC_NDCT_EOB_TOKEN_MAX])(_token,
   _extra_bits,_dct_coeffs,_zzi);
}



/*Reconstructs all coded fragments.
  This requires that each coded fragment have a proper macro block mode and
   motion vector (if not in INTRA mode), and have it's raw DC value decoded.
  The token lists for each coefficient should also be filled in, along with
   initial EOB run counts.*/
static void oc_dec_frags_recon(oc_dec_ctx *_dec){
  oc_fragment *frag;
  int         *coded_fragi;
  int         *coded_fragi_end;
  int          eob_runs[64];
  int          ti[64];
  int          ebi[64];
  int          pli;
  /*First, undo the DC prediction.*/
  frag=_dec->state.frags;
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *fplane;
    int                pred_last[3];
    int                fragx;
    int                fragy;
    pred_last[OC_FRAME_GOLD]=0;
    pred_last[OC_FRAME_PREV]=0;
    pred_last[OC_FRAME_SELF]=0;
    fplane=_dec->state.fplanes+pli;
    for(fragy=0;fragy<fplane->nvfrags;fragy++){
      for(fragx=0;fragx<fplane->nhfrags;fragx++,frag++){
        if(!frag->coded)continue;
        pred_last[OC_FRAME_FOR_MODE[frag->mbmode]]=frag->dc+=
         oc_frag_pred_dc(frag,fplane,fragx,fragy,pred_last);
      }
    }
  }
  /*Now decode the rest of the coefficients.*/
  memset(ti,0,sizeof(ti));
  memset(ebi,0,sizeof(ebi));
  memcpy(eob_runs,_dec->eob_runs,sizeof(eob_runs));
  coded_fragi_end=coded_fragi=_dec->state.coded_fragis;
  for(pli=0;pli<3;pli++){
    coded_fragi_end+=_dec->state.ncoded_fragis[pli];
    for(;coded_fragi<coded_fragi_end;coded_fragi++){
      /*This array is made bigger than necessary so that an invalid zero run
         cannot cause a buffer overflow.*/
      ogg_int16_t  dct_coeffs[65];
      int          fragi;
      int          zzi;
      fragi=*coded_fragi;
      frag=_dec->state.frags+fragi;
      for(zzi=0;zzi<64;){
        int token;
        int eb;
        if(eob_runs[zzi]){
          eob_runs[zzi]--;
          break;
        }
        else{
          token=_dec->dct_tokens[zzi][ti[zzi]++];
          eb=OC_DCT_TOKEN_EXTRA_BITS[token]?
           _dec->extra_bits[zzi][ebi[zzi]++]:0;
          if(token<OC_NDCT_EOB_TOKEN_MAX){
            eob_runs[zzi]=-oc_dct_token_skip(token,eb);
          }
          else oc_dct_token_expand(token,eb,dct_coeffs,&zzi);
        }
      }
      zzi=OC_MINI(zzi,64);
      dct_coeffs[0]=(ogg_int16_t)frag->dc;
      oc_state_frag_recon(&_dec->state,frag,pli,dct_coeffs,zzi,
       _dec->state.dequant_tables[frag->mbmode!=OC_MODE_INTRA][pli][0][
       _dec->state.qi]);
    }
  }
}



theora_dec_ctx *theora_decode_alloc(const theora_info *_info,
 const theora_setup_info *_setup){
  oc_dec_ctx *dec;
  if(_info==NULL||_setup==NULL)return NULL;
  dec=_ogg_malloc(sizeof(*dec));
  oc_dec_init(dec,_info,_setup);
  dec->state.curframe_num=0;
  return dec;
}

void theora_decode_free(theora_dec_ctx *_dec){
  if(_dec!=NULL){
    oc_dec_clear(_dec);
    _ogg_free(_dec);
  }
}

int theora_decode_packetin(theora_dec_ctx *_dec,const ogg_packet *_op,
 ogg_int64_t *_granpos){
  int ret;
  if(_dec==NULL||_op==NULL)return OC_FAULT;
  oggpackB_readinit(&_dec->opb,_op->packet,_op->bytes);
  ret=oc_dec_frame_header_unpack(_dec);
  if(ret<0)return ret;
  /*Select a free buffer to use for the reconstructed version of this frame.*/
  if(_dec->state.frame_type!=OC_INTRA_FRAME&&
   (_dec->state.ref_frame_idx[OC_FRAME_GOLD]<0||
   _dec->state.ref_frame_idx[OC_FRAME_PREV]<0)){
    theora_info *info;
    size_t       yplane_sz;
    size_t       cplane_sz;
    int          yhstride;
    int          yvstride;
    int          chstride;
    int          cvstride;
    /*We're decoding an INTER frame, but have no initialized reference
       buffers (i.e., decoding did not start on a key frame).
      We initialize them to a solid gray here.*/
    _dec->state.ref_frame_idx[OC_FRAME_GOLD]=0;
    _dec->state.ref_frame_idx[OC_FRAME_PREV]=0;
    _dec->state.ref_frame_idx[OC_FRAME_SELF]=1;
    info=&_dec->state.info;
    yhstride=info->width+2*OC_UMV_PADDING;
    yvstride=info->height+2*OC_UMV_PADDING;
    chstride=info->chroma_x_decimated?yhstride>>1:yhstride;
    cvstride=info->chroma_y_decimated?yvstride>>1:yvstride;
    yplane_sz=(size_t)yhstride*yvstride;
    cplane_sz=(size_t)chstride*cvstride;
    memset(_dec->state.ref_frame_data,0x80,yplane_sz+2*cplane_sz);
  }
  else{
    int rfi;
    for(rfi=0;rfi==_dec->state.ref_frame_idx[OC_FRAME_GOLD]||
     rfi==_dec->state.ref_frame_idx[OC_FRAME_PREV];rfi++);
    _dec->state.ref_frame_idx[OC_FRAME_SELF]=rfi;
  }
  if(_dec->state.frame_type==OC_INTRA_FRAME){
    oc_dec_mark_all_intra(_dec);
    _dec->state.keyframe_num=_dec->state.curframe_num;
  }
  else{
    oc_dec_coded_flags_unpack(_dec);
    ret=oc_dec_mb_modes_unpack(_dec);
    if(ret<0)return ret;
    oc_dec_mv_unpack_and_frag_modes_fill(_dec);
  }
  oc_dec_residual_tokens_unpack(_dec);
  /*TODO: All of the rest of the operations -- reconstructing coded fragments,
     copying (coded or uncoded) fragments and post-processing -- should be
     pipelined.
    i.e., after sufficient numbers of super-block rows are reconstructed, copy
     back fragments, then after sufficient numbers of super-block rows are
     complete, post-process.
    This allows us to operate on the data while it is still in cache,
     resulting in big performance improvements.
    A application callback to allow it to process each super-block row as it
     is decoded is also a good idea.*/
  oc_dec_frags_recon(_dec);
  /*Right now the reconstructed frame has only the coded blocks in it.
    We either need to copy all the other blocks into it, or copy the
     reconstructed blocks back into the previous frame, whichever is faster.*/
  if(_dec->state.frame_type==OC_INTRA_FRAME){
    /*Intra frames always code all fragments, so there is nothing to copy.
      The new frame becomes both the previous and gold reference frames.*/
    _dec->state.keyframe_num=_dec->state.curframe_num;
    _dec->state.ref_frame_idx[OC_FRAME_GOLD]=
     _dec->state.ref_frame_idx[OC_FRAME_PREV]=
     _dec->state.ref_frame_idx[OC_FRAME_SELF];
  }
  else{
    int *plfragis[3];
    int  ncoded;
    int  nuncoded;
    ncoded=_dec->state.ncoded_fragis[0]+_dec->state.ncoded_fragis[1]+
     _dec->state.ncoded_fragis[2];
    nuncoded=_dec->state.nfrags-ncoded;
    if(ncoded<nuncoded){
      plfragis[0]=_dec->state.coded_fragis;
      plfragis[1]=plfragis[0]+_dec->state.ncoded_fragis[0];
      plfragis[2]=plfragis[1]+_dec->state.ncoded_fragis[1];
      oc_state_frag_copy(&_dec->state,plfragis,_dec->state.ncoded_fragis,
       OC_FRAME_PREV,OC_FRAME_SELF);
      _dec->state.ref_frame_idx[OC_FRAME_SELF]=
       _dec->state.ref_frame_idx[OC_FRAME_PREV];
    }
    else{
      plfragis[0]=_dec->state.uncoded_fragis-_dec->state.nuncoded_fragis[0];
      plfragis[1]=plfragis[0]-_dec->state.nuncoded_fragis[1];
      plfragis[2]=plfragis[1]-_dec->state.nuncoded_fragis[2];
      oc_state_frag_copy(&_dec->state,plfragis,_dec->state.nuncoded_fragis,
       OC_FRAME_SELF,OC_FRAME_PREV);
      _dec->state.ref_frame_idx[OC_FRAME_PREV]=
       _dec->state.ref_frame_idx[OC_FRAME_SELF];
    }
  }
  /*Filter block edges.*/
  oc_state_loop_filter(&_dec->state,OC_FRAME_PREV);
  /*Fill in the borders from the reconstructed version of the last encoded
     frame.*/
  if(_dec->state.ref_frame_idx[OC_FRAME_PREV]>=0){
    oc_state_borders_fill(&_dec->state,
     _dec->state.ref_frame_idx[OC_FRAME_PREV]);
  }
  _dec->state.granpos=
   (_dec->state.keyframe_num<<_dec->state.info.keyframe_granule_shift)+
   (_dec->state.curframe_num-_dec->state.keyframe_num);
  _dec->state.curframe_num++;
  if(_granpos!=NULL)*_granpos=_dec->state.granpos;
  return 0;
}

int theora_decode_ycbcr_out(theora_dec_ctx *_dec,theora_ycbcr_buffer _ycbcr){
  /*No post-processing for now.*/
  oc_ycbcr_buffer_flip(_ycbcr,_dec->state.ref_frame_bufs[
   _dec->state.ref_frame_idx[OC_FRAME_SELF]]);
#if defined(OC_DUMP_IMAGES)
  oc_state_dump_frame(&_dec->state,OC_FRAME_SELF);
#endif
  return 0;
}
