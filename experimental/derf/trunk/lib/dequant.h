#if !defined(_dequant_H)
# define _dequant_H (1)
# include "theora/theora.h"
# include "quant.h"

void oc_quant_params_unpack(oggpack_buffer *_opb,
 theora_quant_info *_qinfo);
void oc_quant_params_clear(theora_quant_info *_qinfo);

#endif
