#if !defined(_enquant_H)
# define _enquant_H (1)
# include "theora/theora.h"

/*The default quantization parameters used by VP3.1.*/
extern const theora_quant_info OC_VP31_QUANT_INFO;

void oc_quant_params_pack(oggpack_buffer *_opb,
 const theora_quant_info *_qinfo);

#endif
