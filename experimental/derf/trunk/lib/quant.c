#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "quant.h"


/*Minimum scaled DC coefficient intra-frame quantizer value.*/
#define OC_INTRA_DC_QUANT_MIN (4<<2)
/*Minimum scaled AC coefficient intra-frame quantizer value.*/
#define OC_INTRA_AC_QUANT_MIN (2<<2)
/*Minimum scaled DC coefficient inter-frame quantizer value.*/
#define OC_INTER_DC_QUANT_MIN (8<<2)
/*Minimum scaled AC coefficient inter-frame quantizer value.*/
#define OC_INTER_AC_QUANT_MIN (4<<2)
/*Maximum scaled quantizer value.*/
#define OC_QUANT_MAX          (1024<<2)



void oc_quant_tables_init(oc_quant_tables *_dequant,
 const theora_quant_info *_qinfo,int _qi){
  ogg_uint32_t q;
  int          ci;
  /*In the original VP3.2 code, the rounding offset and the size of the dead
     zone around 0 were controlled by a "sharpness" parameter.
    The size of our dead zone is now controlled by the per-coefficient
     quality thresholds returned by our HVS module.
    We round down from a more accurate value when the quality of the
     reconstruction does not fall below our threshold and it saves bits.
    Hence, all of that VP3.2 code is gone from here, and the remaining
     floating point code has been implemented as integer code with exact
     precision.
    I.e., (ogg_uint16_t)(1.0/q*OC_QUANT_SCALE+0.5) has become
     (2*OC_QUANT_SCALE+q)/(q<<1).*/
  /*Scale DC coefficients from the proper table.*/
  /*Intra luma.*/
  q=((ogg_uint32_t)_qinfo->dc_scale[_qi]*_qinfo->luma_coeffs[0]/100)<<2;
  _dequant->intra_y[0]=
   (ogg_uint16_t)OC_CLAMPI(OC_INTRA_DC_QUANT_MIN,q,OC_QUANT_MAX);
  /*Intra chroma.*/
  q=((ogg_uint32_t)_qinfo->dc_scale[_qi]*_qinfo->chroma_coeffs[0]/100)<<2;
  _dequant->intra_c[0]=
   (ogg_uint16_t)OC_CLAMPI(OC_INTRA_DC_QUANT_MIN,q,OC_QUANT_MAX);
  /*Inter.*/
  q=((ogg_uint32_t)_qinfo->dc_scale[_qi]*_qinfo->inter_coeffs[0]/100)<<2;
  _dequant->inter[0]=
   (ogg_uint16_t)OC_CLAMPI(OC_INTER_DC_QUANT_MIN,q,OC_QUANT_MAX);
  /*Now scale AC coefficients from the proper table.*/
  for(ci=1;ci<64;ci++){
    /*Intra luma.*/
    q=((ogg_uint32_t)_qinfo->ac_scale[_qi]*_qinfo->luma_coeffs[ci]/100)<<2;
    _dequant->intra_y[ci]=
     (ogg_uint16_t)OC_CLAMPI(OC_INTRA_AC_QUANT_MIN,q,OC_QUANT_MAX);
    /*Intra chroma.*/
    q=((ogg_uint32_t)_qinfo->ac_scale[_qi]*_qinfo->chroma_coeffs[ci]/100)<<2;
    _dequant->intra_c[ci]=
     (ogg_uint16_t)OC_CLAMPI(OC_INTRA_AC_QUANT_MIN,q,OC_QUANT_MAX);
    /*Inter.*/
    q=((ogg_uint32_t)_qinfo->ac_scale[_qi]*_qinfo->inter_coeffs[ci]/100)<<2;
    _dequant->inter[ci]=
     (ogg_uint16_t)OC_CLAMPI(OC_INTER_AC_QUANT_MIN,q,OC_QUANT_MAX);
  }
}
