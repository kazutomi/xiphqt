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

#endif
