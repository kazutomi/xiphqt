#if !defined(_quant_H)
# define _quant_H (1)
# include "theora/theora.h"
# include "ocintrin.h"

typedef struct oc_quant_tables oc_quant_tables;

struct oc_quant_tables{
  ogg_uint16_t intra_y[64];
  ogg_uint16_t intra_c[64];
  ogg_uint16_t inter[64];
};

void oc_quant_tables_init(oc_quant_tables *_dequant,
 const theora_quant_info *_qinfo,int _qi);

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

#endif
