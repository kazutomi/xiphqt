#if !defined(_huffenc_H)
# define _huffenc_H (1)
# include "huffman.h"



extern const theora_huff_code
 OC_VP31_HUFF_CODES[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS];



int oc_huff_codes_pack(oggpack_buffer *_opb,
 const theora_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]);



#endif
