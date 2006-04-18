#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>
#include "encint.h"



/*The VLC code used for mode schemes 0-6.*/
static const th_huff_code OC_MODE_CODESA[OC_NMODES]={
  {0x00,1},{0x02,2},{0x06,3},{0x0E,4},{0x1E,5},{0x3E,6},{0x7E,7},{0x7F,7}
};

/*The CLC code used for mode scheme 7.*/
static const th_huff_code OC_MODE_CODESB[OC_NMODES]={
  {0x00,3},{0x01,3},{0x02,3},{0x03,3},{0x04,3},{0x05,3},{0x06,3},{0x07,3}
};



/*Initialize the mode scheme chooser.
  This need only be called once per encoder.
  This is probably the best place to describe the various schemes Theora uses
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
void oc_mode_scheme_chooser_init(oc_mode_scheme_chooser *_chooser){
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
void oc_mode_scheme_chooser_reset(oc_mode_scheme_chooser *_chooser){
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
int oc_mode_scheme_chooser_cost(oc_mode_scheme_chooser *_chooser,int _mode){
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
void oc_mode_scheme_chooser_update(oc_mode_scheme_chooser *_chooser,
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

/*Update the count for each mode by the given amounts, and then re-rank the
   schemes appropriately.
  This allows fewer (e.g. 1) updates to be done, at the cost of a more
   expensive update.
  _mode_counts: The amount to add to each mode count.*/
void oc_mode_scheme_chooser_add(oc_mode_scheme_chooser *_chooser,
 int _mode_counts[OC_NMODES]){
  int mi;
  int mj;
  int ri;
  int rj;
  int si;
  for(mi=0;mi<OC_NMODES;mi++){
    _chooser->mode_counts[mi]+=_mode_counts[mi];
  }
  /*Re-order the scheme0 mode list if necessary.*/
  for(ri=1;ri<OC_NMODES;ri++){
    mi=_chooser->scheme0_list[ri];
    rj=ri;
    do{
      mj=_chooser->scheme0_list[rj-1];
      if(_chooser->mode_counts[mj]>=_chooser->mode_counts[mi])break;
      _chooser->scheme0_ranks[mj]++;
      _chooser->scheme0_list[rj]=mj;
    }
    while(--rj>0);
    _chooser->scheme0_ranks[mi]=rj;
    _chooser->scheme0_list[rj]=mi;
  }
  /*Now recompute the bit cost for each scheme.*/
  for(si=0;si<8;si++){
    _chooser->scheme_bits[si]=0;
    for(mi=0;mi<8;mi++){
      _chooser->scheme_bits[si]+=
       _chooser->mode_codes[si][_chooser->mode_ranks[si][mi]].nbits*
        _chooser->mode_counts[mi];
    }
  }
  /*Scheme 0 starts with 24 bits to store the mode list in.*/
  _chooser->scheme_bits[0]+=24;
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
