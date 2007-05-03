#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "recode.h"
/*For th_setup_info, packet state, idct, huffdec, dequant.*/
#include "decint.h"
/*For oc_huff_codes_pack, oc_state_flushheader.*/
#include "encint.h"

typedef struct th_rec_ctx oc_rec_ctx;
typedef oc_tok_hist       oc_tok_hist_table[5];



/*Reading packet statistics.*/
#define OC_PACKET_ANALYZE    (1)
/*Waiting for Huffman tables to be set.*/
#define OC_PACKET_HUFFTABLES (2)
/*Rewriting data packets.*/
#define OC_PACKET_REWRITE    (0)



struct th_rec_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state      state;
  /*The next four fields must be in the given positions in order to be
     compatible with some encoder functions we call.*/
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and goes to 1
     when a frame has been processed and a data packet is ready.*/
  int                  packet_state;
  /*Buffer in which to assemble packets.*/
  oggpack_buffer       enc_opb;
  /*Huffman encode tables.*/
  th_huff_code         enc_huff_codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];
  /*Quantization parameters.*/
  th_quant_info        qinfo;
  /*The previous four fields must be in the given positions in order to be
     compatible with some encoder functions we call.*/
  /*Buffer from which to decode packets.*/
  oggpack_buffer       dec_opb;
  /*Huffman decode trees.*/
  oc_huff_node        *dec_huff_tables[TH_NHUFFMAN_TABLES];
  /*The index of one past the last token in each plane for each coefficient.
    The final entries are the total number of tokens for each coefficient.*/
  int                  ti0[3][64];
  /*The index of one past the last extra bits entry in each plane for each
     coefficient.
    The final entries are the total number of extra bits entries for each
     coefficient.*/
  int                  ebi0[3][64];
  /*The number of outstanding EOB runs at the start of each coefficient in each
     plane.*/
  int                  eob_runs[3][64];
  /*The DCT token lists.*/
  unsigned char      **dct_tokens;
  /*The extra bits associated with DCT tokens.*/
  ogg_uint16_t       **extra_bits;
  /*The DCT token counts for the last decoded frame.*/
  oc_tok_hist          tok_hist[2][5];
  /*The DCT token counts for all decoded frames.*/
  oc_frame_tok_hist   *tok_hists;
  long                 ntok_hists;
  long                 ctok_hists;
  /*The index of the set of token counts used for the current frame while
     rewriting.*/
  long                 cur_tok_histi;
};



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
  long bits;
  int ret;
  /*Coding scheme:
       Codeword            Run Length
     0                       1
     10x                     2-3
     110x                    4-5
     1110xx                  6-9
     11110xxx                10-17
     111110xxxx              18-33
     111111xxxxxxxxxxxx      34-4129*/
  theora_read1(_opb,&bits);
  if(bits==0)return 1;
  theora_read(_opb,2,&bits);
  if((bits&2)==0)return 2+(int)bits;
  else if((bits&1)==0){
    theora_read1(_opb,&bits);
    return 4+(int)bits;
  }
  theora_read(_opb,3,&bits);
  if((bits&4)==0)return 6+(int)bits;
  else if((bits&2)==0){
    ret=10+((bits&1)<<2);
    theora_read(_opb,2,&bits);
    return ret+(int)bits;
  }
  else if((bits&1)==0){
    theora_read(_opb,4,&bits);
    return 18+(int)bits;
  }
  theora_read(_opb,12,&bits);
  return 34+(int)bits;
}

static int oc_block_run_unpack(oggpack_buffer *_opb){
  long bits;
  long bits2;
  /*Coding scheme:
     Codeword             Run Length
     0x                      1-2
     10x                     3-4
     110x                    5-6
     1110xx                  7-10
     11110xx                 11-14
     11111xxxx               15-30*/
  theora_read(_opb,2,&bits);
  if((bits&2)==0)return 1+(int)bits;
  else if((bits&1)==0){
    theora_read1(_opb,&bits);
    return 3+(int)bits;
  }
  theora_read(_opb,2,&bits);
  if((bits&2)==0)return 5+(int)bits;
  else if((bits&1)==0){
    theora_read(_opb,2,&bits);
    return 7+(int)bits;
  }
  theora_read(_opb,3,&bits);
  if((bits&4)==0)return 11+bits;
  theora_read(_opb,2,&bits2);
  return 15+((bits&3)<<2)+bits2;
}

static void oc_quant_params_copy(th_quant_info *_qdst,
 const th_quant_info *_qsrc){
  int i;
  memcpy(_qdst,_qsrc,sizeof(*_qdst));
  for(i=0;i<6;i++){
    int qti;
    int pli;
    int qtj;
    int plj;
    qti=i/3;
    pli=i%3;
    qtj=(i-1)/3;
    plj=(i-1)%3;
    if(i>0&&_qsrc->qi_ranges[qti][pli].sizes==
     _qsrc->qi_ranges[qtj][plj].sizes){
      _qdst->qi_ranges[qti][pli].sizes=_qdst->qi_ranges[qtj][plj].sizes;
    }
    else if(qti>0&&_qsrc->qi_ranges[1][pli].sizes==
     _qsrc->qi_ranges[0][pli].sizes){
      _qdst->qi_ranges[1][pli].sizes=_qdst->qi_ranges[0][pli].sizes;
    }
    else{
      int *sizes;
      sizes=(int *)_ogg_malloc(
       _qsrc->qi_ranges[qti][pli].nranges*sizeof(*sizes));
      memcpy(sizes,_qsrc->qi_ranges[qti][pli].sizes,
       _qsrc->qi_ranges[qti][pli].nranges*sizeof(*sizes));
      _qdst->qi_ranges[qti][pli].sizes=sizes;
    }
    if(i>0&&_qsrc->qi_ranges[qti][pli].base_matrices==
     _qsrc->qi_ranges[qtj][plj].base_matrices){
      _qdst->qi_ranges[qti][pli].base_matrices=
       _qdst->qi_ranges[qtj][plj].base_matrices;
    }
    else if(qti>0&&_qsrc->qi_ranges[1][pli].base_matrices==
     _qsrc->qi_ranges[0][pli].base_matrices){
      _qdst->qi_ranges[1][pli].base_matrices=
       _qdst->qi_ranges[0][pli].base_matrices;
    }
    else{
      th_quant_base *base_matrices;
      base_matrices=(th_quant_base *)_ogg_malloc(
       (_qsrc->qi_ranges[qti][pli].nranges+1)*sizeof(*base_matrices));
      memcpy(base_matrices,_qsrc->qi_ranges[qti][pli].base_matrices,
       (_qsrc->qi_ranges[qti][pli].nranges+1)*sizeof(*base_matrices));
      _qdst->qi_ranges[qti][pli].base_matrices=
       (const th_quant_base *)base_matrices;
    }
  }
}


static int oc_rec_init(oc_rec_ctx *_rec,const th_info *_info,
 const th_setup_info *_setup){
  int ret;
  ret=oc_state_init(&_rec->state,_info);
  if(ret<0)return ret;
  oc_huff_trees_copy(_rec->dec_huff_tables,
   (const oc_huff_node *const *)_setup->huff_tables);
  /*Do a deep copy of the quant params, since we will need to refer to this
     data again (unlike in the normal decoder).*/
  oc_quant_params_copy(&_rec->qinfo,&_setup->qinfo);
  _rec->dct_tokens=(unsigned char **)oc_calloc_2d(64,
   _rec->state.nfrags,sizeof(_rec->dct_tokens[0][0]));
  _rec->extra_bits=(ogg_uint16_t **)oc_calloc_2d(64,
   _rec->state.nfrags,sizeof(_rec->extra_bits[0][0]));
  _rec->tok_hists=NULL;
  _rec->ntok_hists=_rec->ctok_hists=0;
  _rec->cur_tok_histi=0;
  _rec->packet_state=OC_PACKET_ANALYZE;
  oggpackB_writeinit(&_rec->enc_opb);
  return 0;
}

static void oc_rec_clear(oc_rec_ctx *_rec){
  _ogg_free(_rec->tok_hists);
  oc_free_2d(_rec->extra_bits);
  oc_free_2d(_rec->dct_tokens);
  oc_quant_params_clear(&_rec->qinfo);
  oc_huff_trees_clear(_rec->dec_huff_tables);
  oggpackB_writeclear(&_rec->enc_opb);
  oc_state_clear(&_rec->state);
}


static int oc_rec_frame_header_unpack(oc_rec_ctx *_rec){
  long val;
  /*Check to make sure this is a data packet.*/
  theora_read1(&_rec->dec_opb,&val);
  if(val!=0)return TH_EBADPACKET;
  /*Read in the frame type (I or P).*/
  theora_read1(&_rec->dec_opb,&val);
  _rec->state.frame_type=(int)val;
  /*Read in the current qi.*/
  theora_read(&_rec->dec_opb,6,&val);
  _rec->state.qis[0]=(int)val;
  theora_read1(&_rec->dec_opb,&val);
  if(!val)_rec->state.nqis=1;
  else{
    theora_read(&_rec->dec_opb,6,&val);
    _rec->state.qis[1]=(int)val;
    theora_read1(&_rec->dec_opb,&val);
    if(!val)_rec->state.nqis=2;
    else{
      theora_read(&_rec->dec_opb,6,&val);
      _rec->state.qis[2]=(int)val;
      _rec->state.nqis=3;
    }
  }
  if(_rec->state.frame_type==OC_INTRA_FRAME){
    /*Keyframes have 3 unused configuration bits, holdovers from VP3 days.
      Most of the other unused bits in the VP3 headers were eliminated.
      I don't know why these remain.*/
    theora_read(&_rec->dec_opb,3,&val);
    if(val!=0)return TH_EIMPL;
  }
  return 0;
}

/*Mark all fragments as coded and in OC_MODE_INTRA.
  This also builds up the coded fragment list (in coded order), and clears the
   uncoded fragment list.
  It does not update the coded macro block list, as that is not used when
   decoding INTRA frames.*/
static void oc_rec_mark_all_intra(oc_rec_ctx *_rec){
  oc_sb *sb;
  oc_sb *sb_end;
  int    pli;
  int    ncoded_fragis;
  int    prev_ncoded_fragis;
  prev_ncoded_fragis=ncoded_fragis=0;
  sb=sb_end=_rec->state.sbs;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    fplane=_rec->state.fplanes+pli;
    sb_end+=fplane->nsbs;
    for(;sb<sb_end;sb++){
      int quadi;
      for(quadi=0;quadi<4;quadi++)if(sb->quad_valid&1<<quadi){
        int bi;
        for(bi=0;bi<4;bi++){
          int fragi;
          fragi=sb->map[quadi][bi];
          if(fragi>=0){
            oc_fragment *frag;
            frag=_rec->state.frags+fragi;
            frag->coded=1;
            frag->mbmode=OC_MODE_INTRA;
            _rec->state.coded_fragis[ncoded_fragis++]=fragi;
          }
        }
      }
    }
    _rec->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
    _rec->state.nuncoded_fragis[pli]=0;
  }
}

/*Decodes the bit flags for whether or not each super block is partially coded
   or not.
  Return: The number of partially coded super blocks.*/
static int oc_rec_partial_sb_flags_unpack(oc_rec_ctx *_rec){
  oc_sb *sb;
  oc_sb *sb_end;
  long   val;
  int    flag;
  int    npartial;
  int    run_count;
  theora_read1(&_rec->dec_opb,&val);
  flag=(int)val;
  sb=_rec->state.sbs;
  sb_end=sb+_rec->state.nsbs;
  run_count=npartial=0;
  while(sb<sb_end){
    int full_run;
    run_count=oc_sb_run_unpack(&_rec->dec_opb);
    full_run=run_count>=4129;
    do{
      sb->coded_partially=flag;
      sb->coded_fully=0;
      npartial+=flag;
      sb++;
    }
    while(--run_count>0&&sb<sb_end);
    if(full_run&&sb<sb_end){
      theora_read1(&_rec->dec_opb,&val);
      flag=(int)val;
    }
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
static void oc_rec_coded_sb_flags_unpack(oc_rec_ctx *_rec){
  oc_sb *sb;
  oc_sb *sb_end;
  long   val;
  int    flag;
  int    run_count;
  sb=_rec->state.sbs;
  sb_end=sb+_rec->state.nsbs;
  /*Skip partially coded super blocks.*/
  for(;sb->coded_partially;sb++);
  theora_read1(&_rec->dec_opb,&val);
  flag=(int)val;
  while(sb<sb_end){
    int full_run;
    run_count=oc_sb_run_unpack(&_rec->dec_opb);
    full_run=run_count>=4129;
    for(;sb<sb_end;sb++){
      if(sb->coded_partially)continue;
      if(run_count--<=0)break;
      sb->coded_fully=flag;
    }
    if(full_run&&sb<sb_end){
      theora_read1(&_rec->dec_opb,&val);
      flag=(int)val;
    }
    else flag=!flag;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
}

static void oc_rec_coded_flags_unpack(oc_rec_ctx *_rec){
  oc_sb *sb;
  oc_sb *sb_end;
  long   val;
  int    npartial;
  int    pli;
  int    flag;
  int    run_count;
  int    ncoded_fragis;
  int    prev_ncoded_fragis;
  int    nuncoded_fragis;
  int    prev_nuncoded_fragis;
  npartial=oc_rec_partial_sb_flags_unpack(_rec);
  if(npartial<_rec->state.nsbs)oc_rec_coded_sb_flags_unpack(_rec);
  if(npartial>0){
    theora_read1(&_rec->dec_opb,&val);
    flag=!(int)val;
  }
  else flag=0;
  run_count=0;
  prev_ncoded_fragis=ncoded_fragis=prev_nuncoded_fragis=nuncoded_fragis=0;
  sb=sb_end=_rec->state.sbs;
  for(pli=0;pli<3;pli++){
    const oc_fragment_plane *fplane;
    fplane=_rec->state.fplanes+pli;
    sb_end+=fplane->nsbs;
    for(;sb<sb_end;sb++){
      int quadi;
      for(quadi=0;quadi<4;quadi++)if(sb->quad_valid&1<<quadi){
        int bi;
        for(bi=0;bi<4;bi++){
          int fragi;
          fragi=sb->map[quadi][bi];
          if(fragi>=0){
            oc_fragment *frag;
            frag=_rec->state.frags+fragi;
            if(sb->coded_fully)frag->coded=1;
            else if(!sb->coded_partially)frag->coded=0;
            else{
              if(run_count<=0){
                run_count=oc_block_run_unpack(&_rec->dec_opb);
                flag=!flag;
              }
              run_count--;
              frag->coded=flag;
            }
            if(frag->coded)_rec->state.coded_fragis[ncoded_fragis++]=fragi;
            else *(_rec->state.uncoded_fragis-++nuncoded_fragis)=fragi;
          }
        }
      }
    }
    _rec->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
    _rec->state.nuncoded_fragis[pli]=nuncoded_fragis-prev_nuncoded_fragis;
    prev_nuncoded_fragis=nuncoded_fragis;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
}



typedef int (*oc_mode_unpack_func)(oggpack_buffer *_opb);

static int oc_vlc_mode_unpack(oggpack_buffer *_opb){
  long val;
  int  i;
  for(i=0;i<7;i++){
    theora_read1(_opb,&val);
    if(!val)break;
  }
  return i;
}

static int oc_clc_mode_unpack(oggpack_buffer *_opb){
  long val;
  theora_read(_opb,3,&val);
  return (int)val;
}

/*Unpacks the list of macro block modes for INTER frames.*/
void oc_rec_mb_modes_unpack(oc_rec_ctx *_rec){
  oc_mode_unpack_func  mode_unpack;
  oc_mb               *mb;
  oc_mb               *mb_end;
  const int           *alphabet;
  long                 val;
  int                  scheme0_alphabet[8];
  int                  mode_scheme;
  theora_read(&_rec->dec_opb,3,&val);
  mode_scheme=(int)val;
  if(mode_scheme==0){
    int mi;
    /*Just in case, initialize the modes to something.
      If the bitstream doesn't contain each index exactly once, it's likely
       corrupt and the rest of the packet is garbage anyway, but this way we
       won't crash, and we'll decode SOMETHING.*/
    for(mi=0;mi<OC_NMODES;mi++)scheme0_alphabet[mi]=OC_MODE_INTER_NOMV;
    for(mi=0;mi<OC_NMODES;mi++){
      theora_read(&_rec->dec_opb,3,&val);
      scheme0_alphabet[val]=OC_MODE_ALPHABETS[6][mi];
    }
    alphabet=scheme0_alphabet;
  }
  else alphabet=OC_MODE_ALPHABETS[mode_scheme-1];
  if(mode_scheme==7)mode_unpack=oc_clc_mode_unpack;
  else mode_unpack=oc_vlc_mode_unpack;
  mb=_rec->state.mbs;
  mb_end=mb+_rec->state.nmbs;
  for(;mb<mb_end;mb++)if(mb->mode!=OC_MODE_INVALID){
    int bi;
    for(bi=0;bi<4;bi++){
      int fragi;
      fragi=mb->map[0][bi];
      if(fragi>=0&&_rec->state.frags[fragi].coded)break;
    }
    if(bi<4)mb->mode=alphabet[(*mode_unpack)(&_rec->dec_opb)];
    else mb->mode=OC_MODE_INTER_NOMV;
  }
}



typedef int (*oc_mv_comp_unpack_func)(oggpack_buffer *_opb);

static int oc_vlc_mv_comp_unpack(oggpack_buffer *_opb){
  long bits;
  int  mvsigned[2];
  theora_read(_opb,3,&bits);
  switch(bits){
    case 0:return 0;
    case 1:return 1;
    case 2:return -1;
    case 3:{
      mvsigned[0]=2;
      theora_read1(_opb,&bits);
    }break;
    case 4:{
      mvsigned[0]=3;
      theora_read1(_opb,&bits);
    }break;
    case 5:{
      theora_read(_opb,3,&bits);
      mvsigned[0]=4+(bits>>1);
      bits&=1;
    }break;
    case 6:{
      theora_read(_opb,4,&bits);
      mvsigned[0]=8+(bits>>1);
      bits&=1;
    }break;
    case 7:{
      theora_read(_opb,5,&bits);
      mvsigned[0]=16+(bits>>1);
      bits&=1;
    }break;
  }
  mvsigned[1]=-mvsigned[0];
  return mvsigned[bits];
}

static int oc_clc_mv_comp_unpack(oggpack_buffer *_opb){
  long bits;
  int  mvsigned[2];
  theora_read(_opb,6,&bits);
  mvsigned[0]=bits>>1;
  mvsigned[1]=-mvsigned[0];
  return mvsigned[bits&1];
}

/*Unpacks the list of motion vectors for INTER frames.
  Does not propagte the macro block modes and motion vectors to the individual
   fragments.
  The purpose of this function is solely to skip these bits in the packet.*/
static void oc_rec_mv_unpack(oc_rec_ctx *_rec){
  oc_mv_comp_unpack_func  mv_comp_unpack;
  oc_mb                  *mb;
  oc_mb                  *mb_end;
  const int              *map_idxs;
  long                    val;
  int                     map_nidxs;
  theora_read1(&_rec->dec_opb,&val);
  mv_comp_unpack=val?oc_clc_mv_comp_unpack:oc_vlc_mv_comp_unpack;
  map_idxs=OC_MB_MAP_IDXS[_rec->state.info.pixel_fmt];
  map_nidxs=OC_MB_MAP_NIDXS[_rec->state.info.pixel_fmt];
  mb=_rec->state.mbs;
  mb_end=mb+_rec->state.nmbs;
  for(;mb<mb_end;mb++)if(mb->mode!=OC_MODE_INVALID){
    int          coded[13];
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
      if(fragi>=0&&_rec->state.frags[fragi].coded)coded[ncoded++]=mapi;
    }
    while(++mapii<map_nidxs);
    if(ncoded<=0)continue;
    switch(mb->mode){
      case OC_MODE_INTER_MV_FOUR:{
        int  bi;
        /*Mark the tail of the list, so we don't accidentally go past it.*/
        coded[ncoded]=-1;
        for(bi=codedi=0;bi<4;bi++)if(coded[codedi]==bi){
          codedi++;
          (*mv_comp_unpack)(&_rec->dec_opb);
          (*mv_comp_unpack)(&_rec->dec_opb);
        }
      }break;
      case OC_MODE_INTER_MV:{
        (*mv_comp_unpack)(&_rec->dec_opb);
        (*mv_comp_unpack)(&_rec->dec_opb);
      }break;
      case OC_MODE_GOLDEN_MV:{
        (*mv_comp_unpack)(&_rec->dec_opb);
        (*mv_comp_unpack)(&_rec->dec_opb);
      }break;
    }
  }
}

static void oc_rec_block_qis_unpack(oc_rec_ctx *_rec){
  int *coded_fragi;
  int *coded_fragi_end;
  int  ncoded_fragis;
  ncoded_fragis=_rec->state.ncoded_fragis[0]+
   _rec->state.ncoded_fragis[1]+_rec->state.ncoded_fragis[2];
  if(ncoded_fragis<=0)return;
  coded_fragi=_rec->state.coded_fragis;
  coded_fragi_end=coded_fragi+ncoded_fragis;
  if(_rec->state.nqis>1){
    long val;
    int  flag;
    int  nqi0;
    int  run_count;
    /*If this frame has more than one qi value, we decode a qi index for each
      fragment, using two passes of the same binary RLE scheme used for
      super-block coded bits.
     The first pass marks each fragment as having a qii of 0 or greater than
      0, and the second pass (if necessary), distinguishes between a qii of
      1 and 2.
     We just store the qii in the fragment.*/
    theora_read1(&_rec->dec_opb,&val);
    flag=(int)val;
    run_count=nqi0=0;
    while(coded_fragi<coded_fragi_end){
      int full_run;
      run_count=oc_sb_run_unpack(&_rec->dec_opb);
      full_run=run_count>=4129;
      do{
        _rec->state.frags[*coded_fragi++].qi=flag;
        nqi0+=!flag;
      }
      while(--run_count>0&&coded_fragi<coded_fragi_end);
      if(full_run&&coded_fragi<coded_fragi_end){
        theora_read1(&_rec->dec_opb,&val);
        flag=(int)val;
      }
      else flag=!flag;
    }
    /*TODO: run_count should be 0 here.
      If it's not, we should issue a warning of some kind.*/
    /*If we have 3 different qi's for this frame, and there was at least one
       fragment with a non-zero qi, make the second pass.*/
    if(_rec->state.nqis==3&&nqi0<ncoded_fragis){
      /*Skip qii==0 fragments.*/
      for(coded_fragi=_rec->state.coded_fragis;
       _rec->state.frags[*coded_fragi].qi==0;coded_fragi++);
      theora_read1(&_rec->dec_opb,&val);
      flag=(int)val;
      while(coded_fragi<coded_fragi_end){
        int full_run;
        run_count=oc_sb_run_unpack(&_rec->dec_opb);
        full_run=run_count>=4129;
        for(;coded_fragi<coded_fragi_end;coded_fragi++){
          oc_fragment *frag;
          frag=_rec->state.frags+*coded_fragi;
          if(frag->qi==0)continue;
          if(run_count--<=0)break;
          frag->qi+=flag;
        }
        if(full_run&&coded_fragi<coded_fragi_end){
          theora_read1(&_rec->dec_opb,&val);
          flag=(int)val;
        }
        else flag=!flag;
      }
      /*TODO: run_count should be 0 here.
        If it's not, we should issue a warning of some kind.*/
    }
  }
}

/*Unpacks the DC coefficient tokens.
  Unlike when unpacking the AC coefficient tokens, we actually need to decode
   the DC coefficient values now so that we can do DC prediction.
  _huff_idx:   The index of the Huffman table to use for each color plane.
  _ntoks_left: The number of tokens left to be decoded in each color plane for
                each coefficient.
               This is updated as EOB tokens and zero run tokens are decoded.
  Return: The length of any outstanding EOB run.*/
static int oc_rec_dc_coeff_unpack(oc_rec_ctx *_rec,int _huff_idxs[3],
 int *_tok_hists[3],int _ntoks_left[3][64]){
  long  val;
  int  *coded_fragi;
  int  *coded_fragi_end;
  int   run_counts[64];
  int   cfi;
  int   eobi;
  int   eobs;
  int   ti;
  int   ebi;
  int   pli;
  int   rli;
  eobs=0;
  ti=ebi=0;
  coded_fragi_end=coded_fragi=_rec->state.coded_fragis;
  for(pli=0;pli<3;pli++){
    coded_fragi_end+=_rec->state.ncoded_fragis[pli];
    memset(run_counts,0,sizeof(run_counts));
    _rec->eob_runs[pli][0]=eobs;
    /*Continue any previous EOB run, if there was one.*/
    for(eobi=eobs;eobi-->0&&coded_fragi<coded_fragi_end;coded_fragi++);
    cfi=0;
    while(eobs<_ntoks_left[pli][0]-cfi){
      int token;
      int neb;
      int eb;
      int skip;
      cfi+=eobs;
      run_counts[63]+=eobs;
      token=oc_huff_token_decode(&_rec->dec_opb,
       _rec->dec_huff_tables[_huff_idxs[pli]]);
      _rec->dct_tokens[0][ti++]=(char)token;
      _tok_hists[pli][token]++;
      neb=OC_DCT_TOKEN_EXTRA_BITS[token];
      if(neb){
        theora_read(&_rec->dec_opb,neb,&val);
        eb=(int)val;
        _rec->extra_bits[0][ebi++]=(ogg_int16_t)eb;
      }
      else eb=0;
      skip=oc_dct_token_skip(token,eb);
      if(skip<0){
        eobs=eobi=-skip;
        while(eobi-->0&&coded_fragi<coded_fragi_end)coded_fragi++;
      }
      else{
        run_counts[skip-1]++;
        cfi++;
        eobs=0;
        coded_fragi++;
      }
    }
    _rec->ti0[pli][0]=ti;
    _rec->ebi0[pli][0]=ebi;
    /*Set the EOB count to the portion of the last EOB run which extends past
       this coefficient.*/
    eobs=eobs+cfi-_ntoks_left[pli][0];
    /*Add the portion of the last EOB which was included in this coefficient to
       to the longest run length.*/
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
  _eobs:       The length of any outstanding EOB run from previous
                coefficients.
  Return: The length of any outstanding EOB run.*/
static int oc_rec_ac_coeff_unpack(oc_rec_ctx *_rec,int _zzi,int _huff_idxs[3],
 int *_tok_hists[3],int _ntoks_left[3][64],int _eobs){
  long val;
  int  run_counts[64];
  int  cfi;
  int  ti;
  int  ebi;
  int  pli;
  int  rli;
  ti=ebi=0;
  for(pli=0;pli<3;pli++){
    memset(run_counts,0,sizeof(run_counts));
    _rec->eob_runs[pli][_zzi]=_eobs;
    cfi=0;
    while(_eobs<_ntoks_left[pli][_zzi]-cfi){
      int token;
      int neb;
      int eb;
      int skip;
      cfi+=_eobs;
      run_counts[63]+=_eobs;
      token=oc_huff_token_decode(&_rec->dec_opb,
       _rec->dec_huff_tables[_huff_idxs[pli]]);
      _rec->dct_tokens[_zzi][ti++]=(char)token;
      _tok_hists[pli][token]++;
      neb=OC_DCT_TOKEN_EXTRA_BITS[token];
      if(neb){
        theora_read(&_rec->dec_opb,neb,&val);
        eb=(int)val;
        _rec->extra_bits[_zzi][ebi++]=(ogg_int16_t)eb;
      }
      else eb=0;
      skip=oc_dct_token_skip(token,eb);
      if(skip<0)_eobs=-skip;
      else{
        run_counts[skip-1]++;
        cfi++;
        _eobs=0;
      }
    }
    _rec->ti0[pli][_zzi]=ti;
    _rec->ebi0[pli][_zzi]=ebi;
    /*Set the EOB count to the portion of the last EOB run which extends past
       this coefficient.*/
    _eobs=_eobs+cfi-_ntoks_left[pli][_zzi];
    /*Add the portion of the last EOB which was included in this coefficient to
       to the longest run length.*/
    run_counts[63]+=_ntoks_left[pli][_zzi]-cfi;
    /*And convert the run_counts array to a moment table.*/
    for(rli=63;rli-->0;)run_counts[rli]+=run_counts[rli+1];
    /*Finally, subtract off the number of coefficients that have been
       accounted for by runs started in this coefficient.*/
    for(rli=64-_zzi;rli-->0;)_ntoks_left[pli][_zzi+rli]-=run_counts[rli];
  }
  return _eobs;
}

/*Tokens describing the DCT coefficients that belong to each fragment are
   stored in the bitstream grouped by coefficient, not by fragment.
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
   determine where the next token goes.
  Actually, we use 3 counters per coefficient, one for each color plane, so we
   can decode all color planes simultaneously.
  This lets us color conversion, etc., be done as soon as a full MCU (one or
   two super block rows) is decoded, while the image data is still in cache.*/
static void oc_rec_residual_tokens_unpack(oc_rec_ctx *_rec){
  static const int OC_HUFF_LIST_MAX[5]={1,6,15,28,64};
  long  val;
  int   ntoks_left[3][64];
  int   huff_idxs[3];
  int  *tok_hists[3];
  int   pli;
  int   zzi;
  int   hgi;
  int   huffi_y;
  int   huffi_c;
  int   eobs;
  memset(_rec->tok_hist,0,sizeof(_rec->tok_hist));
  for(pli=0;pli<3;pli++)for(zzi=0;zzi<64;zzi++){
    ntoks_left[pli][zzi]=_rec->state.ncoded_fragis[pli];
  }
  theora_read(&_rec->dec_opb,4,&val);
  huffi_y=(int)val;
  theora_read(&_rec->dec_opb,4,&val);
  huffi_c=(int)val;
  huff_idxs[0]=huffi_y;
  huff_idxs[1]=huff_idxs[2]=huffi_c;
  tok_hists[0]=_rec->tok_hist[0][0];
  tok_hists[1]=tok_hists[2]=_rec->tok_hist[1][0];
  _rec->eob_runs[0][0]=0;
  eobs=oc_rec_dc_coeff_unpack(_rec,huff_idxs,tok_hists,ntoks_left);
  theora_read(&_rec->dec_opb,4,&val);
  huffi_y=(int)val;
  theora_read(&_rec->dec_opb,4,&val);
  huffi_c=(int)val;
  zzi=1;
  for(hgi=1;hgi<5;hgi++){
    huff_idxs[0]=huffi_y+(hgi<<4);
    huff_idxs[1]=huff_idxs[2]=huffi_c+(hgi<<4);
    tok_hists[0]=_rec->tok_hist[0][hgi];
    tok_hists[1]=tok_hists[2]=_rec->tok_hist[1][hgi];
    for(;zzi<OC_HUFF_LIST_MAX[hgi];zzi++){
      eobs=oc_rec_ac_coeff_unpack(_rec,zzi,huff_idxs,tok_hists,ntoks_left,eobs);
    }
  }
  /*TODO: eobs should be exactly zero, or 4096 or greater.
    The second case occurs when an EOB run of size zero is encountered, which
     gets treated as an infinite EOB run (where infinity is INT_MAX).
    If neither of these conditions holds, then a warning should be issued.*/
}

static int oc_rec_set_huffman_codes(oc_rec_ctx *_rec,
 const th_huff_code _codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS]){
  int ret;
  if(_rec==NULL)return TH_EFAULT;
  /*If we've already emitted the setup header, then don't let the user set the
     tables again.*/
  if(_rec->packet_state>=OC_PACKET_SETUP_HDR&&
   _rec->packet_state<=OC_PACKET_REWRITE){
    return TH_EINVAL;
  }
  if(_codes==NULL)_codes=TH_VP31_HUFF_CODES;
  /*Validate the codes.*/
  oggpackB_reset(&_rec->enc_opb);
  ret=oc_huff_codes_pack(&_rec->enc_opb,_codes);
  if(ret<0)return ret;
  memcpy(_rec->enc_huff_codes,_codes,sizeof(_rec->enc_huff_codes));
  _rec->packet_state=OC_PACKET_INFO_HDR;
  return 0;
}

/*Computes the number of bits used for each of the potential Huffman codes for
   the given list of token counts.
  The bits are added to whatever the current bit counts are.*/
static void oc_rec_count_bits(oc_rec_ctx *_rec,int _hgi,
 const int _token_counts[TH_NDCT_TOKENS],int _bit_counts[16]){
  int huffi;
  int huff_base;
  int token;
  huff_base=_hgi<<4;
  for(huffi=huff_base;huffi<huff_base+16;huffi++){
    for(token=0;token<TH_NDCT_TOKENS;token++){
      _bit_counts[huffi-huff_base]+=
       _token_counts[token]*_rec->enc_huff_codes[huffi][token].nbits;
    }
  }
}

/*Returns the Huffman index using the fewest number of bits.*/
static int oc_rec_select_huffi(int _bit_counts[16]){
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
static void oc_rec_huff_group_pack(oc_rec_ctx *_rec,int _zzi_start,
 int _zzi_end,int _huff_idxs[3]){
  int zzi;
  for(zzi=_zzi_start;zzi<_zzi_end;zzi++){
    int pli;
    int ti;
    int ebi;
    ti=0;
    ebi=0;
    for(pli=0;pli<3;pli++){
      const th_huff_code *huff_codes;
      int                 token;
      int                 ti_end;
      /*Step 2: Write the tokens using these tables.*/
      huff_codes=_rec->enc_huff_codes[_huff_idxs[pli]];
      /*Note: dct_token_offs[3] is really the ndct_tokens table.
        Yes, this seems like a horrible hack, yet it's strangely elegant.*/
      ti_end=_rec->ti0[pli][zzi];
      for(;ti<ti_end;ti++){
        token=_rec->dct_tokens[zzi][ti];
        oggpackB_write(&_rec->enc_opb,huff_codes[token].pattern,
         huff_codes[token].nbits);
        if(OC_DCT_TOKEN_EXTRA_BITS[token]){
          oggpackB_write(&_rec->enc_opb,_rec->extra_bits[zzi][ebi++],
           OC_DCT_TOKEN_EXTRA_BITS[token]);
        }
      }
    }
  }
}

static void oc_rec_residual_tokens_pack(oc_rec_ctx *_rec,
 const oc_tok_hist _tok_hist[2][5]){
  static const int  OC_HUFF_LIST_MIN[6]={0,1,6,15,28,64};
  static const int *OC_HUFF_LIST_MAX=OC_HUFF_LIST_MIN+1;
  int bits_y[16];
  int bits_c[16];
  int huff_idxs[5][3];
  int huffi_y;
  int huffi_c;
  int hgi;
  /*Step 1a: Select Huffman tables for the DC token list.*/
  memset(bits_y,0,sizeof(bits_y));
  memset(bits_c,0,sizeof(bits_c));
  oc_rec_count_bits(_rec,0,_tok_hist[0][0],bits_y);
  oc_rec_count_bits(_rec,0,_tok_hist[1][0],bits_c);
  huffi_y=oc_rec_select_huffi(bits_y);
  huffi_c=oc_rec_select_huffi(bits_c);
  huff_idxs[0][0]=huffi_y;
  huff_idxs[0][1]=huff_idxs[0][2]=huffi_c;
  /*Step 1b: Write the DC token list with the chosen tables.*/
  oggpackB_write(&_rec->enc_opb,huffi_y,4);
  oggpackB_write(&_rec->enc_opb,huffi_c,4);
  oc_rec_huff_group_pack(_rec,0,1,huff_idxs[0]);
  /*Step 2a: Select Huffman tables for the AC token lists.*/
  memset(bits_y,0,sizeof(bits_y));
  memset(bits_y,0,sizeof(bits_c));
  for(hgi=1;hgi<5;hgi++){
    oc_rec_count_bits(_rec,hgi,_tok_hist[0][hgi],bits_y);
    oc_rec_count_bits(_rec,hgi,_tok_hist[1][hgi],bits_c);
  }
  huffi_y=oc_rec_select_huffi(bits_y);
  huffi_c=oc_rec_select_huffi(bits_c);
  /*Step 2b: Write the AC token lists using the chosen tables.*/
  oggpackB_write(&_rec->enc_opb,huffi_y,4);
  oggpackB_write(&_rec->enc_opb,huffi_c,4);
  for(hgi=1;hgi<5;hgi++){
    huff_idxs[hgi][0]=huffi_y+(hgi<<4);
    huff_idxs[hgi][1]=huff_idxs[hgi][2]=huffi_c+(hgi<<4);
    oc_rec_huff_group_pack(_rec,OC_HUFF_LIST_MIN[hgi],OC_HUFF_LIST_MAX[hgi],
     huff_idxs[hgi]);
  }
}



th_rec_ctx *th_recode_alloc(const th_info *_info,const th_setup_info *_setup){
  oc_rec_ctx *dec;
  if(_info==NULL||_setup==NULL)return NULL;
  dec=_ogg_malloc(sizeof(*dec));
  if(oc_rec_init(dec,_info,_setup)<0){
    _ogg_free(dec);
    return NULL;
  }
  dec->state.curframe_num=0;
  return dec;
}

void th_recode_free(th_rec_ctx *_rec){
  if(_rec!=NULL){
    oc_rec_clear(_rec);
    _ogg_free(_rec);
  }
}

int th_recode_packetin(th_rec_ctx *_rec,const ogg_packet *_op,
 ogg_int64_t *_granpos){
  int ret;
  if(_rec==NULL||_op==NULL)return TH_EFAULT;
  /*If the user has already retrieved the statistics, we can't update them any
     longer.*/
  if(_rec->packet_state!=OC_PACKET_ANALYZE)return TH_EINVAL;
  /*A completely empty packet indicates a dropped frame and is treated exactly
     like an inter frame with no coded blocks.
    Only proceed if we have a non-empty packet.*/
  if(_op->bytes!=0){
    oc_frame_tok_hist *tok_hist;
    oggpackB_readinit(&_rec->dec_opb,_op->packet,_op->bytes);
    ret=oc_rec_frame_header_unpack(_rec);
    if(ret<0)return ret;
    if(_rec->state.frame_type==OC_INTRA_FRAME){
      oc_rec_mark_all_intra(_rec);
      _rec->state.keyframe_num=_rec->state.curframe_num;
    }
    else{
      oc_rec_coded_flags_unpack(_rec);
      oc_rec_mb_modes_unpack(_rec);
      oc_rec_mv_unpack(_rec);
    }
    oc_rec_block_qis_unpack(_rec);
    if(_rec->ntok_hists>=_rec->ctok_hists){
      _rec->ctok_hists=_rec->ctok_hists<<1|1;
      _rec->tok_hists=(oc_frame_tok_hist *)_ogg_realloc(_rec->tok_hists,
       _rec->ctok_hists*sizeof(*_rec->tok_hists));
    }
    tok_hist=_rec->tok_hists+_rec->ntok_hists++;
    tok_hist->pkt_sz=_op->bytes;
    tok_hist->dct_offs=oggpackB_bits(&_rec->dec_opb);
    memcpy(tok_hist->ncoded_fragis,_rec->state.ncoded_fragis,
     sizeof(tok_hist->ncoded_fragis));
    oc_rec_residual_tokens_unpack(_rec);
    /*Update granule position.*/
    _rec->state.granpos=
     (_rec->state.keyframe_num<<_rec->state.info.keyframe_granule_shift)+
     (_rec->state.curframe_num-_rec->state.keyframe_num);
    tok_hist->granpos=_rec->state.granpos;
    /*Save the statistics for this frame.*/
    memcpy(tok_hist->tok_hist,_rec->tok_hist,sizeof(tok_hist->tok_hist));
    _rec->state.curframe_num++;
    if(_granpos!=NULL)*_granpos=_rec->state.granpos;
    return 0;
  }
  else{
    /*Just update the granule position and return.*/
    _rec->state.granpos=
     (_rec->state.keyframe_num<<_rec->state.info.keyframe_granule_shift)+
     (_rec->state.curframe_num-_rec->state.keyframe_num);
    _rec->state.curframe_num++;
    if(_granpos!=NULL)*_granpos=_rec->state.granpos;
    return TH_DUPFRAME;
  }
}

int th_recode_ctl(th_rec_ctx *_rec,int _req,void *_buf,size_t _buf_sz){
  switch(_req){
    case TH_ENCCTL_SET_HUFFMAN_CODES:{
      if(_buf==NULL&&_buf_sz!=0||_buf!=NULL&&
       _buf_sz!=sizeof(th_huff_code)*TH_NHUFFMAN_TABLES*TH_NDCT_TOKENS){
        return TH_EINVAL;
      }
      return oc_rec_set_huffman_codes(_rec,(const th_huff_table *)_buf);
    }break;
    case TH_DECCTL_SET_GRANPOS:{
      ogg_int64_t granpos;
      if(_rec==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(ogg_int64_t))return TH_EINVAL;
      granpos=*(ogg_int64_t *)_buf;
      if(granpos<0)return TH_EINVAL;
      _rec->state.granpos=granpos;
      _rec->state.keyframe_num=
       granpos>>_rec->state.info.keyframe_granule_shift;
      _rec->state.curframe_num=_rec->state.keyframe_num+
       (granpos&(1<<_rec->state.info.keyframe_granule_shift)-1);
      return 0;
    }break;
    case TH_RECCTL_GET_TOK_NSTATS:{
      if(_rec==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(long))return TH_EINVAL;
      *((long *)_buf)=_rec->ntok_hists;
      return 0;
    }break;
    case TH_RECCTL_GET_TOK_STATS:{
      if(_rec==NULL||_buf==NULL)return TH_EFAULT;
      if(_buf_sz!=sizeof(const oc_frame_tok_hist **))return TH_EINVAL;
      if(_rec->packet_state<OC_PACKET_ANALYZE)return TH_EINVAL;
      /*Update the state to prevent us from invalidating this pointer.*/
      _rec->packet_state=OC_PACKET_HUFFTABLES;
      *((const oc_frame_tok_hist **)_buf)=_rec->tok_hists;
      return 0;
    }break;
    default:return TH_EIMPL;
  }
}

int th_recode_flushheader(th_rec_ctx *_rec,th_comment *_tc,ogg_packet *_op){
  return oc_state_flushheader(&_rec->state,&_rec->packet_state,&_rec->enc_opb,
   &_rec->qinfo,(const th_huff_table *)_rec->enc_huff_codes,_tc->vendor,
   _tc,_op);
}

#include <stdio.h>

int th_recode_packet_rewrite(th_rec_ctx *_rec,const ogg_packet *_op_in,
 ogg_packet *_op_out){
  int ret;
  if(_rec==NULL||_op_in==NULL||_op_out==NULL)return TH_EFAULT;
  /*If we've used all our decoded token histograms, please stop calling us.*/
  if(_rec->cur_tok_histi>=_rec->ntok_hists)return TH_EINVAL;
  /*A completely empty packet indicates a dropped frame and is treated exactly
     like an inter frame with no coded blocks.
    Only proceed if we have a non-empty packet.*/
  if(_op_in->bytes!=0){
    oc_frame_tok_hist *tok_hist;
    /*Read enough of the packet to figure out what kind of frame we have.
      This also validates the packet to be sure we can decode it, which is why
       we don't just use th_packet_iskeyframe().*/
    oggpackB_readinit(&_rec->dec_opb,_op_in->packet,_op_in->bytes);
    ret=oc_rec_frame_header_unpack(_rec);
    if(ret<0)return ret;
    /*Update granule position.*/
    if(_rec->state.frame_type==OC_INTRA_FRAME){
      _rec->state.keyframe_num=_rec->state.curframe_num;
    }
    _rec->state.granpos=
     (_rec->state.keyframe_num<<_rec->state.info.keyframe_granule_shift)+
     (_rec->state.curframe_num-_rec->state.keyframe_num);
    _rec->state.curframe_num++;
    /*Sanity checks to see if the next piece of frame data corresponds to this
       packet.
      This isn't a guarantee if someone rewrote the file out from under us, but
       it at least ensures that we have enough bytes in the packet.
      TODO: We could re-decode this packet to get the info we need, instead of
       failing, but that would be more code.*/
    tok_hist=_rec->tok_hists+_rec->cur_tok_histi;
    if(tok_hist->granpos!=_rec->state.granpos||
     tok_hist->pkt_sz!=_op_in->bytes){
      return TH_EBADPACKET;
    }
    _rec->cur_tok_histi++;
    /*Copy the contents of the input packet up to the DCT tokens.*/
    oggpackB_reset(&_rec->enc_opb);
    oggpackB_writecopy(&_rec->enc_opb,_op_in->packet,tok_hist->dct_offs);
    /*Read the DCT tokens using the old codes.*/
    oggpackB_readinit(&_rec->dec_opb,_op_in->packet,_op_in->bytes);
    oggpackB_adv(&_rec->dec_opb,tok_hist->dct_offs);
    memcpy(_rec->state.ncoded_fragis,tok_hist->ncoded_fragis,
     sizeof(_rec->state.ncoded_fragis));
    oc_rec_residual_tokens_unpack(_rec);
    /*Write the DCT tokens using the new codes.*/
    memcpy(_rec->state.ncoded_fragis,tok_hist->ncoded_fragis,
     sizeof(_rec->state.ncoded_fragis));
    oc_rec_residual_tokens_pack(_rec,
     (const oc_tok_hist_table *)tok_hist->tok_hist);
    ret=0;
  }
  else{
    oggpackB_reset(&_rec->enc_opb);
    /*Just update the granule position and return.*/
    _rec->state.granpos=
     (_rec->state.keyframe_num<<_rec->state.info.keyframe_granule_shift)+
     (_rec->state.curframe_num-_rec->state.keyframe_num);
    _rec->state.curframe_num++;
    ret=TH_DUPFRAME;
  }
  _op_out->packet=oggpackB_get_buffer(&_rec->enc_opb);
  _op_out->bytes=oggpackB_bytes(&_rec->enc_opb);
  _op_out->b_o_s=0;
  _op_out->e_o_s=_op_in->e_o_s;
  _op_out->packetno=_rec->state.curframe_num;
  _op_out->granulepos=_rec->state.granpos;
  if(_op_out->e_o_s)_rec->packet_state=OC_PACKET_DONE;
  return ret;
}
