#if !defined(_huffdec_H)
# define _huffdec_H (1)
# include "huffman.h"



typedef struct oc_huff_node oc_huff_node;

/*A node in the Huffman tree.
  Instead of storing every branching in the tree, sub-trees which are complete
   are collapsed into one node.
  This allows more than one bit to be read at a time, and avoids following all
   the intermediate branches with next to no increased code complexity once
   the collapsed tree has been built.*/
struct oc_huff_node{
  /*The number of bits of the code needed to descend through this node.
    0 indicates a leaf node.
    Otherwise there are 1<<nbits nodes in the nodes table, which can be
     indexed by reading nbits bits from the stream.*/
  unsigned char  nbits;
  /*The value of a token stored in a leaf node.
    The value in non-leaf nodes is undefined.*/
  unsigned char  token;
  /*The table of child nodes.
    The ACTUAL size of this array is 1<<nbits, despite what the declaration
     below claims.
    The exception is that for leaf nodes the size is 0.*/
  oc_huff_node  *nodes[1];
};



int oc_huff_trees_unpack(oggpack_buffer *_opb,
 oc_huff_node *_nodes[OC_NHUFFMAN_TABLES]);
void oc_huff_trees_copy(oc_huff_node *_dst[OC_NHUFFMAN_TABLES],
 const oc_huff_node *_src[OC_NHUFFMAN_TABLES]);
void oc_huff_trees_clear(oc_huff_node *_nodes[OC_NHUFFMAN_TABLES]);
int oc_huff_token_decode(oggpack_buffer *_opb,const oc_huff_node *_node);


#endif