#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ogg/ogg.h>
#include "encint.h"


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