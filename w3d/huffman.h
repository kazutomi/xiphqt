
#include "bitcoder.h"


typedef struct {
   unsigned int parent : 5;
   unsigned int freq : 15;
   unsigned int child0 : 6;   /* MSB is the is_leaf bit! */
   unsigned int child1 : 6;   /* MSB is the is_leaf bit! */
} HuffmanNode;


typedef struct {
   unsigned int parent : 5;
   unsigned int freq : 15;
} HuffmanLeaf;


typedef struct {
   HuffmanNode node[31];
   HuffmanLeaf leaf[32];
} HuffmanTree32;



extern 
void huffman_init (HuffmanTree32 *tree);

extern 
void huffman_encode_symbol (BitCoderState *bitcoder, HuffmanTree32 *t,
                            unsigned int symbol);
extern 
unsigned int huffman_decode_symbol (BitCoderState *bitcoder, HuffmanTree32 *t);


