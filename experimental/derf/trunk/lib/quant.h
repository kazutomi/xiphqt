#if !defined(_quant_H)
# define _quant_H (1)
# include "theora/theora.h"
# include "ocintrin.h"

typedef ogg_uint16_t oc_quant_tables[64][64];



/*Maximum scaled quantizer value.*/
#define OC_QUANT_MAX          (1024<<2)



/*Minimum scaled DC coefficient frame quantizer value for intra and inter
   modes.*/
extern unsigned OC_DC_QUANT_MIN[2];
/*Minimum scaled AC coefficient frame quantizer value for intra and inter
   modes.*/
extern unsigned OC_AC_QUANT_MIN[2];



void oc_dequant_tables_init(oc_quant_tables *_dequant[2][3],
 const theora_quant_info *_qinfo);

#endif
