#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"

#if defined(OC_BITRATE_STATS)
static ogg_uint32_t OC_RES_BITRATES[8][64][16];
static int          OC_RES_BITRATE_SAMPLES[8][64][16];

void oc_bitrate_update_stats(oc_enc_ctx *_enc,int _huff_idxs[5][3]){
  static OC_HUFF_GROUP_IDXS[64]={
    0,1,1,1,1,1,2,2,
    2,2,2,2,2,2,2,3,
    3,3,3,3,3,3,3,3,
    3,3,3,3,4,4,4,4,
    4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4
  };
  /*Go through the actual encoded tokens and assign the bits used to each to
     the fragment(s) they came from.*/
  oc_sb        *sb;
  oc_sb        *sb_end;
  ogg_uint32_t  eob_bits[64];
  int           eob_runs[64];
  int           pli;
  int           ti[64];
  int           ebi[64];
  int           zzi;
  int           hgi;
  int           token;
  int           eb;
  int           skip;
  memset(eob_runs,0,sizeof(eob_runs));
  memcpy(ti,_enc->dct_token_offs[0],sizeof(ti));
  memcpy(ebi,_enc->extra_bits_offs,sizeof(ebi));
  /*Set up for EOB runs that span coefficient boundaries.*/
  for(zzi=1;zzi<64;zzi++)if(ti[zzi]){
    /*Find the portion of the EOB run that used to be in this coefficient.*/
    token=_enc->dct_tokens[zzi][0];
    eb=OC_DCT_TOKEN_EXTRA_BITS[token]?_enc->extra_bits[zzi][0]:0;
    skip=-oc_dct_token_skip(token,eb);
    /*Find the portion of the EOB run that is now in the previous
       coefficient.*/
    token=_enc->dct_tokens[zzi-1][_enc->ndct_tokens[zzi-1]-1];
    eb=OC_DCT_TOKEN_EXTRA_BITS[token]?
     _enc->extra_bits[zzi-1][_enc->nextra_bits[zzi-1]-1]:0;
    eob_runs[zzi]=-oc_dct_token_skip(token,eb);
    /*Find the Huffman group index and the color plane index where the EOB
       run was actually coded.*/
    hgi=OC_HUFF_GROUP_IDXS[zzi];
    for(pli=0;pli<2;pli++){
      if(_enc->dct_token_offs[pli+1][zzi-1]>=_enc->ndct_tokens[zzi-1])break;
    }
    /*Compute the bits that will be left to be distributed when we start this
       coefficient.*/
    eob_bits[zzi]=OC_DCT_TOKEN_EXTRA_BITS[token]+
     _enc->huff_codes[_huff_idxs[hgi][2]][token].len<<8;
    while(eob_runs[zzi]>skip){
      eob_bits[zzi]-=((eob_bits[zzi]<<1)+eob_runs[zzi])/(eob_runs[zzi]<<1);
      eob_runs[zzi]--;
    }
  }
  sb=sb_end=_enc->state.frarray.sbs;
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *fplane;
    fplane=_enc->state.frarray.frplanes+pli;
    sb_end+=fplane->nsbs;
    for(;sb<sb_end;sb++){
      int mbi;
      for(mbi=0;mbi<4;mbi++)if(sb->mb_valid&1<<mbi){
        int bi;
        for(bi=0;bi<4;bi++)if(sb->map[mbi][bi]>=0){
          oc_fragment *frag;
          frag=_enc->state.frarray.frags+sb->map[mbi][bi];
          if(frag->coded){
            oc_fragment_enc_info *frinfo;
            ogg_uint32_t          frag_bits;
            int                   err_bin;
            frinfo=_enc->frinfo+sb->map[mbi][bi];
            frag_bits=0;
            for(zzi=0;zzi<64;){
              if(eob_runs[zzi]){
                ogg_uint32_t bits;
                /*Evenly divide the bits for an EOB run among the fragments
                   that it ends.
                  This incremental formulation ensures that the exact
                   correct number of bits gets distributed.*/
                bits=((eob_bits[zzi]<<1)+eob_runs[zzi])/(eob_runs[zzi]<<1);
                eob_bits[zzi]-=bits;
                eob_runs[zzi]--;
                frag_bits+=bits;
                break;
              }
              token=_enc->dct_tokens[zzi][ti[zzi]++];
              eb=OC_DCT_TOKEN_EXTRA_BITS[token]?
               _enc->extra_bits[zzi][ebi[zzi]++]:0;
              skip=oc_dct_token_skip(token,eb);
              hgi=OC_HUFF_GROUP_IDXS[zzi];
              if(skip<0){
                eob_runs[zzi]=-skip;
                eob_bits[zzi]=OC_DCT_TOKEN_EXTRA_BITS[token]+
                 _enc->huff_codes[_huff_idxs[hgi][pli]][token].len<<8;
              }
              else{
                frag_bits+=OC_DCT_TOKEN_EXTRA_BITS[token]+
                 _enc->huff_codes[_huff_idxs[hgi][pli]][token].len<<8;
                zzi+=skip;
              }
            }
            err_bin=frinfo->eerror>>(frag->mbmode==OC_MODE_INTRA?8:6);
            err_bin=OC_MINI(15,err_bin);
            OC_RES_BITRATES[frag->mbmode][_enc->state.qi][err_bin]+=frag_bits;
            OC_RES_BITRATE_SAMPLES[frag->mbmode][_enc->state.qi][err_bin]++;
          }
        }
      }
    }
  }
}
#endif











#if 0
#include <stdlib.h>
#include <string.h>
#include <limits.h>


/*256*(A(\rho), B(\rho) and C(\rho)) for \rho=0/64..64/64, where
   the bitrate for a macro block is approximated as
   Q_{nz}(\rho)*A(\rho)+Q_z(\rho)*B(\rho)+C(\rho).
  TODO: These numbers were extrapolated from numbers derived from JPEG.
  We should derive our own numbers at some point.*/
static const int OC_INTRA_RATE_PARAMS[65][3]={
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{311,-114,231},{311,-114,231},{311,-114,231},
  {311,-114,231},{305,-115,231},{272,-119,232},{240,-124,234},
  {207,-129,236},{194,-119,230},{181,-110,224},{168,-100,217},
  {161, -96,212},{156, -93,208},{150, -90,203},{146, -82,196},
  {145, -73,188},{143, -63,179},{137, -55,171},{125, -49,163},
  {113, -43,156},{104, -38,150},{104, -38,150},{104, -38,150},
  {104, -38,150}
};

/*Estimates the number of bits required to code this macro block in intra
   mode.
  _enc:     The encoding context.
  _mbi:     The macro block index.
  _nc_bits: Returns the number of bits required for the not-coded fragments.
            This is used for key frame decisions.
  Return: The number of bits required for the coded fragments.*/
int oc_enc_intra_bits_est(oc_enc_ctx *_enc,int _mbi,int *_nc_bits){
  oc_mb          *mb;
  oc_mb_enc_info *emb;
  ogg_uint16_t   *fquant;
  ogg_uint16_t   *iquant;
  ogg_int32_t     bits;
  ogg_int32_t     nc_bits;
  int             q_nz;
  int             q_z;
  int             rho;
  int             bi;
  emb=_enc->mbinfo+_mbi;
  mb=_enc->state.frarray.mbs+_mbi;
  fquant=_enc->enquant_tables[_enc->state.qi].intra_y;
  iquant=_enc->state.dequant_tables[_enc->state.qi].intra_y;
  bits=nc_bits=0;
  for(bi=0;bi<4;bi++){
    oc_fragment_enc_info *frinfo;
    ogg_int32_t           frag_bits;
    int                   zzi;
    int                   zrun;
    frinfo=_enc->frinfo+mb->map[0][bi];
    q_nz=q_z=rho=zrun=0;
    for(zzi=0;zzi<64;zzi++){
      int c_abs;
      int ci;
      ci=OC_FZIG_ZAG[zzi];
      c_abs=abs(frinfo->dct_coeffs[ci]);
      if(c_abs<=OC_MAXI(frinfo->tols[ci],iquant[ci]>>1)){
        zrun++;
        rho++;
      }
      else{
        if(zrun){
          q_z+=1+oc_ilog(zrun);
          zrun=0;
        }
        q_nz+=1+oc_ilog((ogg_int32_t)c_abs*fquant[ci]+OC_FQUANT_ROUND>>
         OC_FQUANT_SHIFT);
      }
    }
    if(zrun)q_z+=1+oc_ilog(zrun);
    frag_bits=OC_INTRA_RATE_PARAMS[rho][0]*(ogg_int32_t)q_nz+
     OC_INTRA_RATE_PARAMS[rho][1]*(ogg_int32_t)q_z+
     OC_INTRA_RATE_PARAMS[rho][2];
    if(_enc->state.frarray.frags[mb->map[0][bi]].coded)bits+=frag_bits;
    else nc_bits=frag_bits;
  }
  /*TODO: Chroma fragments.*/
  *_nc_bits=(int)(nc_bits+128>>8);
  return (int)(bits+128>>8);
}
#endif