#include <string.h>
#include "theora/theora.h"
#include "internal.h"

typedef struct oc_alphabet_chooser oc_alphabet_chooser;

/*The goal of the alphabet chooser is to be able to get an estimate of the
   number of bits required to encode a given token while deciding which tokens
   to actually encode.
  To know the exact cost would require knowing which alphabet is going to
   be used, but the optimal alphabet choice cannot be made until it is known
   how many of each token is going to be used.
  We can define an approximate cost for token t_n as
   min_{alphabets a_i}(sum_{tokens t_i}(cost(a_i,t_i))+cost(a_i,t_n))-
   min_{alphabets a_i}(sum_{tokens t_i}(cost(a_i,t_i)))
  I.e., we make decisions one at a time in a linear fashion using the list of
   tokens we've chosen so far, and never look back.*/
struct oc_alphabet_chooser{
  /*The number of alphabets to choose from.
    This must be at least 2.*/
  int   nalphabets;
  /*The number of tokens in each alphabet.*/
  int   ntokens;
  /*The number of bits used by each alphabet for each token.*/
  int **token_bits;
  /*The maximum number of bits used by a given token in any alphabet minus the
     minimum.*/
  int  *gain_max;
  /*The total number of bits used by the second best alphabet minus the total
     number of bits used by the best alphabet.*/
  int   gain_min;
  /*The total number of bits currently used by each alphabet.*/
  int  *total_bits;
  /*A list of alphabet indices sorted by total bits.*/
  int  *list;
};



/*Updates the sorted alphabet list after the total_bits values have changed.*/
static void oc_alphabet_chooser_update(oc_alphabet_chooser *_chooser){
  int li;
  int lj;
  int aj;
  /*Since our array is small and has high temporal coherency, use an insertion
     sort to update it.*/
  for(li=1;li<_chooser->nalphabets;li++){
    for(lj=li;lj>0&&_chooser->total_bits[_chooser->list[lj]]<
     _chooser->total_bits[_chooser->list[lj-1]];lj--){
      aj=_chooser->list[lj];
      _chooser->list[lj]=_chooser->list[lj-1];
      _chooser->list[lj-1]=aj;
    }
  }
  _chooser->gain_min=_chooser->total_bits[_chooser->list[1]]-
   _chooser->total_bits[_chooser->list[0]];
}


void oc_alphabet_chooser_init(oc_alphabet_chooser *_chooser,
 int _nalphabets,int _ntokens,const theora_huff_code *_codes,
 const int *_extra_bits){
  int ai;
  int ti;
  _chooser->nalphabets=_nalphabets;
  _chooser->ntokens=_ntokens;
  _chooser->token_bits=(int **)oc_malloc_2d(_nalphabets,_ntokens,
   sizeof(_chooser->token_bits[0][0]));
  _chooser->gain_max=(int *)_ogg_malloc(
   _ntokens*sizeof(_chooser->gain_max[0]));
  _chooser->total_bits=(int *)_ogg_malloc(
   _nalphabets*sizeof(_chooser->total_bits[0]));
  _chooser->list=(int *)_ogg_malloc(_nalphabets*sizeof(_chooser->list[0]));
  for(ai=0;ai<_nalphabets;ai++){
    for(ti=0;ti<_ntokens;ti++,_codes++){
      _chooser->token_bits[ai][ti]=_codes->nbits+_extra_bits[ti];
    }
    _chooser->total_bits[ai]=0;
    _chooser->list[ai]=ai;
  }
  for(ti=0;ti<_ntokens;ti++){
    int bits_min;
    int bits_max;
    bits_min=bits_max=_chooser->token_bits[0][ti];
    for(ai=1;ai<_nalphabets;ai++){
      bits_min=OC_MINI(bits_min,_chooser->token_bits[ai][ti]);
      bits_max=OC_MAXI(bits_max,_chooser->token_bits[ai][ti]);
    }
    _chooser->gain_max[ti]=bits_max-bits_min;
  }
  _chooser->gain_min=0;
}

/*Clears the bit totals for all alphabets.*/
void oc_alphabet_chooser_reset(oc_alphabet_chooser *_chooser){
  int ai;
  memset(_chooser->total_bits,0,
   sizeof(_chooser->total_bits[0])*_chooser->nalphabets);
  _chooser->gain_min=0;
  /*Reset the alphabet list: this just ensures ties are always broken the same
     way.*/
  for(ai=0;ai<_chooser->nalphabets;ai++)_chooser->list[ai]=ai;
}

int oc_alphabet_chooser_bitcost(oc_alphabet_chooser *_chooser,int _token){
  int a0;
  int b0;
  int li;
  int bits_min;
  int bits_max;
  int gain_max;
  /*Normal case: encoding with this token cannot possibly change the current
     alphabet.*/
  a0=_chooser->list[0];
  gain_max=_chooser->gain_max[_token];
  if(gain_max<_chooser->gain_min)return _chooser->token_bits[a0][_token];
  /*We're close to a decision point: scan the alphabet list to see if changing
     alphabets would give us a lower cost for this symbol.*/
  b0=_chooser->total_bits[a0];
  bits_min=b0+_chooser->token_bits[a0][_token];
  bits_max=b0+gain_max;
  for(li=1;li<_chooser->nalphabets;li++){
    int ai;
    ai=_chooser->list[li];
    if(_chooser->total_bits[ai]>=bits_max)break;
    bits_min=OC_MINI(bits_min,
     _chooser->total_bits[ai]+_chooser->token_bits[ai][_token]);
  }
  return bits_min-b0;
}

void oc_alphabet_chooser_token_add(oc_alphabet_chooser *_chooser,int _token){
  int ai;
  for(ai=0;ai<_chooser->nalphabets;ai++){
    _chooser->total_bits[ai]+=_chooser->token_bits[ai][_token];
  }
  oc_alphabet_chooser_update(_chooser);
}

void oc_alphabet_chooser_token_del(oc_alphabet_chooser *_chooser,int _token){
  int ai;
  for(ai=0;ai<_chooser->nalphabets;ai++){
    _chooser->total_bits[ai]-=_chooser->token_bits[ai][_token];
  }
  oc_alphabet_chooser_update(_chooser);
}

void oc_alphabet_chooser_token_dellist(oc_alphabet_chooser *_chooser,
 const int *_tokens,int _ntokens){
  int ai;
  int ti;
  for(ai=0;ai<_chooser->nalphabets;ai++){
    for(ti=0;ti<_ntokens;ti++){
      _chooser->total_bits[ai]-=_chooser->token_bits[ai][_tokens[ti]];
    }
  }
  oc_alphabet_chooser_update(_chooser);
}
