#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "enquant.h"


/*The default quantization parameters used by VP3.1.*/
static const int OC_VP31_RANGE_SIZES[1]={63};
static const theora_quant_base OC_VP31_BASES_INTRA_Y[2]={
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
     16, 11, 10, 16, 24,  40, 51, 61,
     12, 12, 14, 19, 26,  58, 60, 55,
     14, 13, 16, 24, 40,  57, 69, 56,
     14, 17, 22, 29, 51,  87, 80, 62,
     18, 22, 37, 58, 68, 109,103, 77,
     24, 35, 55, 64, 81, 104,113, 92,
     49, 64, 78, 87,103, 121,120,101,
     72, 92, 95, 98,112, 100,103, 99
  }
};
static const theora_quant_base OC_VP31_BASES_INTRA_C[2]={
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
     17, 18, 24, 47, 99, 99, 99, 99,
     18, 21, 26, 66, 99, 99, 99, 99,
     24, 26, 56, 99, 99, 99, 99, 99,
     47, 66, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99
  }
};
static const theora_quant_base OC_VP31_BASES_INTER[2]={
  {
     16, 16, 16, 20, 24, 28, 32, 40,
     16, 16, 20, 24, 28, 32, 40, 48,
     16, 20, 24, 28, 32, 40, 48, 64,
     20, 24, 28, 32, 40, 48, 64, 64,
     24, 28, 32, 40, 48, 64, 64, 64,
     28, 32, 40, 48, 64, 64, 64, 96,
     32, 40, 48, 64, 64, 64, 96,128,
     40, 48, 64, 64, 64, 96,128,128
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
    30,25,20,20,15,15,14,14,
    13,13,12,12,11,11,10,10,
     9, 9, 8, 8, 7, 7, 7, 7,
     6, 6, 6, 6, 5, 5, 5, 5,
     4, 4, 4, 4, 3, 3, 3, 3,
     2, 2, 2, 2, 2, 2, 2, 2,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0
  },
  {
    {
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTRA_Y},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTRA_C},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTRA_C}
    },
    {
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTER},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTER},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTER}
    }
  }
};



void oc_quant_params_pack(oggpack_buffer *_opb,
 const theora_quant_info *_qinfo){
  int x;
  /*TODO: Update for alpha 3.*/
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->ac_scale[x],16);
  for(x=0;x<64;x++)oggpackB_write(_opb,_qinfo->dc_scale[x],16);
  for(x=0;x<64;x++){
    oggpackB_write(_opb,_qinfo->qi_ranges[0][0].base_matrices[0][x],8);
  }
  for(x=0;x<64;x++){
    oggpackB_write(_opb,_qinfo->qi_ranges[0][1].base_matrices[0][x],8);
  }
  for(x=0;x<64;x++){
    oggpackB_write(_opb,_qinfo->qi_ranges[1][0].base_matrices[0][x],8);
  }
}



/*See comments at oc_dequant_tables_init() for how the quantization tables'
   storage should be initialized.*/
void oc_enquant_tables_init(oc_quant_tables *_dequant[2][3],
 oc_quant_tables *_enquant[2][3],const theora_quant_info *_qinfo){
  int qti;
  int pli;
  /*Initialize the dequantization tables first.*/
  oc_dequant_tables_init(_dequant,NULL,_qinfo);
  /*Derive the quantization tables directly from the dequantization tables.*/
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    int qi;
    /*These simple checks help us improve cache coherency later.*/
    if(pli>0&&_dequant[qti][pli]==_dequant[qti][pli-1]){
      _enquant[qti][pli]=_enquant[qti][pli-1];
      continue;
    }
    if(qti>0&&_dequant[qti][pli]==_dequant[qti-1][pli]){
      _enquant[qti][pli]=_enquant[qti-1][pli];
      continue;
    }
    for(qi=0;qi<64;qi++){
      int ci;
      /*All of that VP3.2 floating point code has been implemented as integer
         code with exact precision.
        I.e., (ogg_uint16_t)(1.0/q*OC_FQUANT_SCALE+0.5) has become
         (2*OC_FQUANT_SCALE+q)/(q<<1).*/
      for(ci=0;ci<64;ci++){
        ogg_uint32_t q;
        q=_dequant[qti][pli][0][qi][ci];
        _enquant[qti][pli][0][qi][ci]=(ogg_uint16_t)(
         (2*OC_FQUANT_SCALE+q)/(q<<1));
      }
    }
  }
}
