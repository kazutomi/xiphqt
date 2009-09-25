#if !defined(_huffenc_H)
# define _huffenc_H (1)
# include "huffman.h"



extern const th_huff_code
 TH_VP31_HUFF_CODES[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];



int oc_huff_codes_pack(oggpack_buffer *_opb,
 const th_huff_code _codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS]);



#endif
