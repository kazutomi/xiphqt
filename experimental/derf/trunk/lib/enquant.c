#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "enquant.h"


/*The default quantization parameters used by VP3.1.*/
const theora_quant_info OC_VP31_QUANT_INFO={
  {
    500,450,400,370,340,310,285,265,
    245,225,210,195,185,180,170,160,
    150,145,135,130,125,115,110,107,
    100, 96, 93, 89, 85, 82, 75, 74,
     70, 68, 64, 60, 57, 56, 52, 50,
     49, 45, 44, 43, 40, 38, 37, 35,
     33, 32, 30, 29, 28, 25, 24, 22,
     21, 19, 18, 17, 15, 13, 12, 10
  },
  {
    220,200,190,180,170,170,160,160,
    150,150,140,140,130,130,120,120,
    110,110,100,100, 90, 90, 90, 80,
     80, 80, 70, 70, 70, 60, 60, 60,
     60, 50, 50, 50, 50, 40, 40, 40,
     40, 40, 30, 30, 30, 30, 30, 30,
     30, 20, 20, 20, 20, 20, 20, 20,
     20, 10, 10, 10, 10, 10, 10, 10
  },
  {
     16, 11, 10, 16, 24,  40, 51, 61,
     12, 12, 14, 19, 26,  58, 60, 55,
     14, 13, 16, 24, 40,  57, 69, 56,
     14, 17, 22, 29, 51,  87, 80, 62,
     18, 22, 37, 58, 68, 109,103, 77,
     24, 35, 55, 64, 81, 104,113, 92,
     49, 64, 78, 87,103, 121,120,101,
     72, 92, 95, 98,112, 100,103, 99
  },
  {
     17, 18, 24, 47, 99, 99, 99, 99,
     18, 21, 26, 66, 99, 99, 99, 99,
     24, 26, 56, 99, 99, 99, 99, 99,
     47, 66, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99
  },
  {
     16, 16, 16, 20, 24, 28, 32, 40,
     16, 16, 20, 24, 28, 32, 40, 48,
     16, 20, 24, 28, 32, 40, 48, 64,
     20, 24, 28, 32, 40, 48, 64, 64,
     24, 28, 32, 40, 48, 64, 64, 64,
     28, 32, 40, 48, 64, 64, 64, 96,
     32, 40, 48, 64, 64, 64, 96,128,
     40, 48, 64, 64, 64, 96,128,128
  }
};



void oc_quant_params_pack(oggpack_buffer *_opb,
 const theora_quant_info *_qinfo){
  int x;
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->ac_scale[x],16);
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->dc_scale[x],16);
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->luma_coeffs[x],8);
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->chroma_coeffs[x],8);
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->inter_coeffs[x],8);
}


void oc_enquant_table_init(oc_quant_tables *_enquant,
 const oc_quant_tables *_dequant){
  ogg_uint32_t q;
  int          ci;
  for(ci=0;ci<64;ci++){
    q=_dequant->intra_y[ci];
    _enquant->intra_y[ci]=(ogg_uint16_t)((2*OC_FQUANT_SCALE+q)/(q<<1));
    q=_dequant->intra_c[ci];
    _enquant->intra_c[ci]=(ogg_uint16_t)((2*OC_FQUANT_SCALE+q)/(q<<1));
    q=_dequant->inter[ci];
    _enquant->inter[ci]=(ogg_uint16_t)((2*OC_FQUANT_SCALE+q)/(q<<1));
  }
}
