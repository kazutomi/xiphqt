#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"

ogg_uint16_t OC_RES_BITRATES[64][3][OC_NMODES][16];

#if defined(OC_BITRATE_STATS)
static ogg_uint32_t OC_RES_BITRATE_ACCUM[64][3][OC_NMODES][16];
static int          OC_RES_BITRATE_SAMPLES[64][3][OC_NMODES][16];

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
  ogg_uint32_t  eob_bits[64];
  int           eob_runs[64];
  int           pli;
  int          *coded_fragi;
  int          *coded_fragi_end;
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
  for(zzi=1;zzi<64;zzi++){
    /*If we've found the last coefficient spanned by a multi-coefficient EOB
       run...*/
    if(ti[zzi]&&(ti[zzi]<_enc->ndct_tokens[zzi]||zzi>=63||!ti[zzi+1])){
      int zzj;
      int zzk;
      /*Find the portion of the EOB run that used to be in this coefficient
         and in any other coefficients after the first one spanned by the
         run.*/
      token=_enc->dct_tokens[zzi][0];
      eb=OC_DCT_TOKEN_EXTRA_BITS[token]?_enc->extra_bits[zzi][0]:0;
      skip=-oc_dct_token_skip(token,eb);
      for(zzj=zzi-1;ti[zzj]>=_enc->ndct_tokens[zzj];zzj--){
        if(ti[zzj]){
          token=_enc->dct_tokens[zzj][0];
          eb=OC_DCT_TOKEN_EXTRA_BITS[token]?_enc->extra_bits[zzj][0]:0;
          skip-=oc_dct_token_skip(token,eb);
        }
      }
      /*Find the first non-trivial coefficient in the EOB run.*/
      for(zzk=zzj+1;!ti[zzk];zzk++);
      /*Find the total length of the EOB run.*/
      token=_enc->dct_tokens[zzj][_enc->ndct_tokens[zzj]-1];
      eb=OC_DCT_TOKEN_EXTRA_BITS[token]?
       _enc->extra_bits[zzj][_enc->nextra_bits[zzj]-1]:0;
      eob_runs[zzk]=-oc_dct_token_skip(token,eb);
      /*Find the Huffman group index and the color plane index where the EOB
         run was actually coded.*/
      hgi=OC_HUFF_GROUP_IDXS[zzj];
      for(pli=0;pli<2;pli++){
        if(_enc->dct_token_offs[pli+1][zzj]>=_enc->ndct_tokens[zzj])break;
      }
      eob_bits[zzk]=OC_DCT_TOKEN_EXTRA_BITS[token]+
       _enc->huff_codes[_huff_idxs[hgi][pli]][token].nbits<<8;
      /*Compute the bits that will be left to be distributed when we start
         each non-trivial coefficient spanned by the run after the one the run
         is started in.*/
      for(;;){
        while(eob_runs[zzk]>skip){
          eob_bits[zzk]-=
           ((eob_bits[zzk]<<1)+eob_runs[zzi])/(eob_runs[zzk]<<1);
          eob_runs[zzk]--;
        }
        if(zzk>=zzi)break;
        zzj=zzk;
        /*Skip to the next non-trivial coefficient.*/
        for(zzk++;!ti[zzk];zzk++);
        eob_bits[zzk]=eob_bits[zzj];
        eob_runs[zzk]=eob_runs[zzj];
        /*And remove the EOB tokens that belonged to this coefficient.*/
        token=_enc->dct_tokens[zzj][0];
        eb=OC_DCT_TOKEN_EXTRA_BITS[token]?_enc->extra_bits[zzj][0]:0;
        skip+=oc_dct_token_skip(token,eb);
      }
    }
  }
  /*Now, eob_bits and eob_runs are initialized exactly as they would be if we
     were going through a coefficient at a time (as things are ordered in the
     bitstream) instead of a block at a time.
    That setup was a little complicated, but it lets us now go through in
     block order, maintaining a counter for each coefficient, instead of
     traversing each block for each coefficient and maintaining a counter and
     bit count per block.*/
  /*Go through the actual encoded tokens and assign the bits used by each to
     the fragment(s) they came from.*/
  coded_fragi_end=coded_fragi=_enc->state.coded_fragis;
  for(pli=0;pli<3;pli++){
    coded_fragi_end+=_enc->state.ncoded_fragis[pli];
    for(;coded_fragi<coded_fragi_end;coded_fragi++){
      oc_fragment *frag;
      int          fragi;
      fragi=*coded_fragi;
      frag=_enc->state.frags+fragi;
      if(frag->coded){
        oc_fragment_enc_info *efrag;
        ogg_uint32_t          frag_bits;
        int                   err_bin;
        efrag=_enc->frinfo+fragi;
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
             _enc->huff_codes[_huff_idxs[hgi][pli]][token].nbits<<8;
          }
          else{
            frag_bits+=OC_DCT_TOKEN_EXTRA_BITS[token]+
             _enc->huff_codes[_huff_idxs[hgi][pli]][token].nbits<<8;
            zzi+=skip;
          }
        }
        err_bin=efrag->eerror>>(frag->mbmode==OC_MODE_INTRA?8:6);
        err_bin=OC_MINI(15,err_bin);
        OC_RES_BITRATE_ACCUM[
         frag->invalid?0:frag->qi][pli][frag->mbmode][err_bin]+=frag_bits;
        OC_RES_BITRATE_SAMPLES[
         frag->invalid?0:frag->qi][pli][frag->mbmode][err_bin]++;
      }
    }
  }
  /*Update the current bitrate statistics in use.*/
  {
    int qi;
    for(qi=0;qi<64;qi++){
      for(pli=0;pli<3;pli++){
        int modei;
        for(modei=0;modei<OC_NMODES;modei++){
          int erri;
          for(erri=0;erri<16;erri++){
            int n;
            n=OC_RES_BITRATE_SAMPLES[qi][pli][modei][erri];
            if(!n)continue;
            OC_RES_BITRATES[qi][pli][modei][erri]=(ogg_uint16_t)OC_MINI(65535,
             ((OC_RES_BITRATE_ACCUM[qi][pli][modei][erri]<<1)+n)/(n<<1));
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


/*\rho domain rate-distortion analysis, after He \cite{He2001}
  TODO: Is this worth the speed hit?

  @PHDTHESIS{He2001,
    author="Zhihai He",
    title="\rho-Domain Rate-Distortion Analysis and Rate-Control for Visual
     Coding and Communication",
    school="University of California at Santa Barbara",
    year=2001
  }*/


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
  mb=_enc->state.mbs+_mbi;
  fquant=_enc->enquant_tables[_enc->state.qi].intra_y;
  iquant=_enc->state.dequant_tables[_enc->state.qi].intra_y;
  bits=nc_bits=0;
  for(bi=0;bi<4;bi++){
    oc_fragment_enc_info *efrag;
    ogg_int32_t           frag_bits;
    int                   zzi;
    int                   zrun;
    efrag=_enc->frinfo+mb->map[0][bi];
    q_nz=q_z=rho=zrun=0;
    for(zzi=0;zzi<64;zzi++){
      int c_abs;
      int ci;
      ci=OC_FZIG_ZAG[zzi];
      c_abs=abs(efrag->dct_coeffs[ci]);
      if(c_abs<=OC_MAXI(efrag->tols[ci],iquant[ci]>>1)){
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
    if(_enc->state.frags[mb->map[0][bi]].coded)bits+=frag_bits;
    else nc_bits=frag_bits;
  }
  /*TODO: Chroma fragments.*/
  *_nc_bits=(int)(nc_bits+128>>8);
  return (int)(bits+128>>8);
}
#endif
