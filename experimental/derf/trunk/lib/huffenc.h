#if !defined(_huffenc_H)
# define _huffenc_H (1)
# include "huffman.h"



/*A Huffman code for a given token.*/
typedef struct{
  /*The bit pattern for the code.*/
  ogg_uint32_t bits;
  /*The number of bits in the code.*/
  int          len;
}oc_huff_code;



extern const oc_huff_code VP31_HUFF_CODES[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS];



void oc_huff_pack_trees(oggpack_buffer *_opb,
 const oc_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]);

/*
#define OC_DC_HUFF_OFFSET      (0)
#define OC_DC_HUFF_CHOICE_BITS (4)
#define OC_DC_HUFF_CHOICES     (1<<OC_DC_HUFF_CHOICE_BITS)
#define OC_AC_HUFF_OFFSET      (16)
#define OC_AC_HUFF_CHOICE_BITS (4)
#define OC_AC_HUFF_CHOICES     (1<<OC_AC_HUFF_CHOICE_BITS)
#define OC_AC_HUFF_THRESH1 (5)
#define OC_AC_HUFF_THRESH2 (14)
#define OC_AC_HUFF_THRESH3 (27)

#define OC_AC_TABLE_THRESH1 (5)
#define OC_AC_TABLE_THRESH2 (14)
#define OC_AC_TABLE_THRESH3 (27)

#define OC_SINGLE_TOKEN_VALUE_MAX (6)
#define OC_DCT_VAL_CAT2_MIN       (3)
#define OC_DCT_VAL_CAT3_MIN       (7)
#define OC_DCT_VAL_CAT4_MIN       (9)
#define OC_DCT_VAL_CAT5_MIN       (13)
#define OC_DCT_VAL_CAT6_MIN       (21)
#define OC_DCT_VAL_CAT7_MIN       (37)
#define OC_DCT_VAL_CAT8_MIN       (69)



static void pack_eob_run(oggpack_buffer *_opb,...){
}

static void pack_dc_token_list(oggpack_buffer *_opb,...){
}

*/
#endif