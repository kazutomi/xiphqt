#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "dequant.h"


static const unsigned char OC_VP31_LOOP_FILTER_LIMITS[64]={
  30,25,20,20,15,15,14,14,
  13,13,12,12,11,11,10,10,
   9, 9, 8, 8, 7, 7, 7, 7,
   6, 6, 6, 6, 5, 5, 5, 5,
   4, 4, 4, 4, 3, 3, 3, 3,
   2, 2, 2, 2, 2, 2, 2, 2,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0
};


void oc_quant_params_unpack(oggpack_buffer *_opb,
 theora_quant_info *_qinfo){
  theora_quant_base *intra_y_base;
  theora_quant_base *intra_c_base;
  theora_quant_base *inter_base;
  int *sizes;
  int  x;
  /*TODO: Update for alpha 3.*/
  for(x=0;x<64;x++)_qinfo->ac_scale[x]=(ogg_uint16_t)oggpackB_read(_opb,16);
  for(x=0;x<64;x++)_qinfo->dc_scale[x]=(ogg_uint16_t)oggpackB_read(_opb,16);
  _qinfo->qi_ranges[1][2].nranges=_qinfo->qi_ranges[0][2].nranges=
   _qinfo->qi_ranges[1][1].nranges=_qinfo->qi_ranges[0][1].nranges=
   _qinfo->qi_ranges[1][0].nranges=_qinfo->qi_ranges[0][0].nranges=1;
  sizes=(int *)_ogg_malloc(sizeof(sizes[0]));
  sizes[0]=63;
  _qinfo->qi_ranges[1][2].sizes=_qinfo->qi_ranges[0][2].sizes=
   _qinfo->qi_ranges[1][1].sizes=_qinfo->qi_ranges[0][1].sizes=
   _qinfo->qi_ranges[1][0].sizes=_qinfo->qi_ranges[0][0].sizes=sizes;
  intra_y_base=(theora_quant_base *)_ogg_malloc(2*sizeof(intra_y_base[0]));
  _qinfo->qi_ranges[0][0].base_matrices=intra_y_base;
  intra_c_base=(theora_quant_base *)_ogg_malloc(2*sizeof(intra_c_base[0]));
  _qinfo->qi_ranges[0][2].base_matrices=_qinfo->qi_ranges[0][1].base_matrices=
   intra_c_base;
  inter_base=(theora_quant_base *)_ogg_malloc(2*sizeof(inter_base[0]));
  _qinfo->qi_ranges[1][2].base_matrices=_qinfo->qi_ranges[1][1].base_matrices=
   _qinfo->qi_ranges[1][0].base_matrices=inter_base;
  for(x=0;x<64;x++){
    intra_y_base[1][x]=intra_y_base[0][x]=
     (unsigned char)oggpackB_read(_opb,8);
  }
  for(x=0;x<64;x++){
    intra_c_base[1][x]=intra_c_base[0][x]=
     (unsigned char)oggpackB_read(_opb,8);
  }
  for(x=0;x<64;x++){
    inter_base[1][x]=inter_base[0][x]=
     (unsigned char)oggpackB_read(_opb,8);
  }
  memcpy(_qinfo->loop_filter_limits,OC_VP31_LOOP_FILTER_LIMITS,
   sizeof(_qinfo->loop_filter_limits));
}

void oc_quant_params_clear(theora_quant_info *_qinfo){
  int pli;
  int qti;
  for(pli=3;pli-->0;)for(qti=2;qti-->0;){
    /*Clear any duplicate pointer references.*/
    if(pli>0){
      if(_qinfo->qi_ranges[qti][pli].sizes==
       _qinfo->qi_ranges[qti][pli-1].sizes){
        _qinfo->qi_ranges[qti][pli].sizes=NULL;
      }
      if(_qinfo->qi_ranges[qti][pli].base_matrices==
       _qinfo->qi_ranges[qti][pli-1].base_matrices){
        _qinfo->qi_ranges[qti][pli].base_matrices=NULL;
      }
    }
    if(qti>0){
      if(_qinfo->qi_ranges[1][pli].sizes==
       _qinfo->qi_ranges[0][pli].sizes){
        _qinfo->qi_ranges[1][pli].sizes=NULL;
      }
      if(_qinfo->qi_ranges[1][pli].base_matrices==
       _qinfo->qi_ranges[0][pli].base_matrices){
        _qinfo->qi_ranges[1][pli].base_matrices=NULL;
      }
    }
    /*Now free all the non-duplicate storage.*/
    _ogg_free((void *)_qinfo->qi_ranges[qti][pli].sizes);
    _ogg_free((void *)_qinfo->qi_ranges[qti][pli].base_matrices);
  }
}

