#include <limits.h>
#if !defined(_decint_H)
# define _decint_H (1)
# include "theora/theora.h"
# include "internal.h"

typedef struct theora_setup_info oc_setup_info;
typedef struct theora_dec_ctx    oc_dec_ctx;

# include "idct.h"
# include "huffdec.h"
# include "dequant.h"

/*Constants for the packet-in state machine specific to the decoder.*/

/*Next packet to read: Data packet.*/
#define OC_PACKET_DATA (0)



struct theora_setup_info{
  /*The Huffman codes.*/
  oc_huff_node      *huff_tables[OC_NHUFFMAN_TABLES];
  /*The quantization parameters.*/
  theora_quant_info  qinfo;
};



struct theora_dec_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state          state;
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and goes to 1
     when a frame has been processed and a data packet is ready.*/
  int                      packet_state;
  /*Buffer in which to assemble packets.*/
  oggpack_buffer           opb;
  /*Huffman decode trees.*/
  oc_huff_node            *huff_tables[OC_NHUFFMAN_TABLES];
  /*The number of DCT tokens for each coefficient for each plane.*/
  int                      ndct_tokens[3][64];
  /*The DCT token lists.*/
  unsigned char          **dct_tokens;
  /*The extra bits associated with DCT tokens.*/
  ogg_uint16_t           **extra_bits;
  /*The number of outstanding EOB runs at the start of each coefficient.*/
  int                      eob_runs[64];
  /*The out-of-loop post-processing level.*/
  int                      pp_level;
  /*The DC scale used for out-of-loop deblocking.*/
  int                      pp_dc_scale[64];
};

#endif