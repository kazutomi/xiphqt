

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "huffman.h"


#define MAX_FREQ ((1 << 14) - 1)


#define LEAF(x) ((x) | (1 << 5))

HuffmanTree32 default_huffman_tree = {
   { {~0, 0, 1, 2 },
     { 0, 0, 3, 4 },
     { 0, 0, 5, 6 },
     { 1, 0, 7, 8 },
     { 1, 0, 9, 10 },
     { 2, 0, 11, 12 },
     { 2, 0, 13, 14 },
     { 3, 0, 15, 16 },
     { 3, 0, 17, 18 },
     { 4, 0, 19, 20 },
     { 4, 0, 21, 22 },
     { 5, 0, 23, 24 },
     { 5, 0, 25, 26 },
     { 6, 0, 27, 28 },
     { 6, 0, 29, 30 },
     { 7, 0, LEAF(0), LEAF(1) },
     { 7, 0, LEAF(2), LEAF(3) },
     { 8, 0, LEAF(4), LEAF(5) },
     { 8, 0, LEAF(6), LEAF(7) },
     { 9, 0, LEAF(8), LEAF(9) },
     { 9, 0, LEAF(10), LEAF(11) },
     { 10, 0, LEAF(12), LEAF(13) },
     { 10, 0, LEAF(14), LEAF(15) },
     { 11, 0, LEAF(16), LEAF(17) },
     { 11, 0, LEAF(18), LEAF(19) },
     { 12, 0, LEAF(20), LEAF(21) },
     { 12, 0, LEAF(22), LEAF(23) },
     { 13, 0, LEAF(24), LEAF(25) },
     { 13, 0, LEAF(26), LEAF(27) },
     { 14, 0, LEAF(28), LEAF(29) },
     { 14, 0, LEAF(30), LEAF(31) }
   },
   {
     { 15, 0 },
     { 15, 0 },
     { 16, 0 },
     { 16, 0 },
     { 17, 0 },
     { 17, 0 },
     { 18, 0 },
     { 18, 0 },
     { 19, 0 },
     { 19, 0 },
     { 20, 0 },
     { 20, 0 },
     { 21, 0 },
     { 21, 0 },
     { 22, 0 },
     { 22, 0 },
     { 23, 0 },
     { 23, 0 },
     { 24, 0 },
     { 24, 0 },
     { 25, 0 },
     { 25, 0 },
     { 26, 0 },
     { 26, 0 },
     { 27, 0 },
     { 27, 0 },
     { 28, 0 },
     { 28, 0 },
     { 29, 0 },
     { 29, 0 },
     { 30, 0 },
     { 30, 0 }
   }
};



static
void huffman_swap_nodes (HuffmanTree32 *t, unsigned int n1, unsigned int n2);



/**
 *   be careful, this will return leafs if the is_leaf bit is set!!
 */
static inline
HuffmanNode* _huffman_get_node (HuffmanTree32 *t, unsigned int node)
{
   if (node & (1 << 5))
      return (HuffmanNode*) &t->leaf[node & 0x1f];
   else
      return &t->node[node];
}




/**
 *  searches above for a node with lower frequency and returns this
 *  or the node left above. 
 */
static inline
unsigned int _huffman_get_usuccessor (HuffmanTree32 *t, unsigned int node,
                                      int freq_limit)
{
   HuffmanNode *root = _huffman_get_node (t, node);
   HuffmanNode *n;
   int level = 0;

   while (root->parent != ~0) {
      root = &t->node[root->parent];
      level++;
   }

   n = _huffman_get_node (t, root->child0);

   if (root->child0 & (1 << 5))
      return root->child0;

   while (level > 2 && n->freq > freq_limit) {
      int is_leaf = n->child0 & (1 << 5);
      if (is_leaf)
         break;
      n = _huffman_get_node (t, n->child0);
      level--;
   };

   return n->child0;
}


/**
 * search for the right successor. Can return NULL!
 */
static inline
unsigned int _huffman_get_rsuccessor (HuffmanTree32 *t, unsigned int node,
                                      int freq_limit)
{
   HuffmanNode *n, *parent;
   int level = 0;

   n = _huffman_get_node (t, node);

   while ((parent = &t->node[n->parent])->child1 == node) {
      node = n->parent;
      if (node == ~0)                   /*  no right successor */
         return ~0;
      n = _huffman_get_node (t, node);
      level++;
   }

   n = _huffman_get_node (t, parent->child1);

   if (parent->child1 & (1 << 5))
      return parent->child1;

   while (level > 1 && n->freq > freq_limit) {
      int is_leaf = n->child0 & (1 << 5);
      if (is_leaf)
         break;
      n = _huffman_get_node (t, n->child0);
      level--;
   }

   return n->child0;
}


static inline
void huffman_update (HuffmanTree32 *t, unsigned int node)
{
   HuffmanNode *n = _huffman_get_node (t, node);
   unsigned int r = node, u = node;

   do {
      r = _huffman_get_rsuccessor (t, r, n->freq);
   } while (r != ~0 && _huffman_get_node(t, r)->freq > n->freq);

   if (_huffman_get_node(t, r)->freq > n->freq)
      huffman_swap_nodes (t, node, r);

   do {
      u = _huffman_get_usuccessor (t, u, n->freq);
   } while (u != ~0 && _huffman_get_node(t, u)->freq > n->freq);

   if (_huffman_get_node(t, u)->freq > n->freq)
      huffman_swap_nodes (t, node, u);
}


static inline
void huffman_update_freq (HuffmanTree32 *t, unsigned int node)
{
   while (node != ~0) {
      HuffmanNode *n = _huffman_get_node (t, node);

      if (!(node & (1 << 5))) {
         HuffmanNode *c0 = _huffman_get_node (t, n->child0);
         HuffmanNode *c1 = _huffman_get_node (t, n->child1);
         n->freq = c0->freq + c1->freq;

         if (n->freq > MAX_FREQ) {
            int i;
            for (i=0; i<31; i++)
               t->node[i].freq /= 2;
            for (i=0; i<32; i++)
               t->leaf[i].freq /= 2;
         }
      }

      huffman_update (t, node);
      node = n->parent;
   };
}


#define ISWAP(a,b)  do { unsigned int tmp = a; a = b; b = tmp; } while (0)


static
void huffman_swap_nodes (HuffmanTree32 *t, unsigned int n1, unsigned int n2)
{
   HuffmanNode *node1 = _huffman_get_node (t, n1);
   HuffmanNode *node2 = _huffman_get_node (t, n2);

   if (node1->parent == node2->parent) {
      HuffmanNode *parent =  &t->node[node1->parent];
      ISWAP(parent->child0, parent->child1);
   } else {
      HuffmanNode *parent1 = &t->node[node1->parent];
      HuffmanNode *parent2 = &t->node[node2->parent];

      if (parent1->child0 == n1) parent1->child0 = n2;
      else if (parent1->child0 == n2) parent1->child0 = n1;

      if (parent1->child1 == n1) parent1->child1 = n2;
      else if (parent1->child1 == n2) parent1->child1 = n1;

      if (parent2->child0 == n1) parent2->child0 = n2;
      else if (parent2->child0 == n2) parent2->child0 = n1;

      if (parent2->child1 == n1) parent2->child1 = n2;
      else if (parent2->child1 == n2) parent2->child1 = n1;

      ISWAP(node1->parent, node2->parent);

      huffman_update_freq (t, node1->parent);
      huffman_update_freq (t, node2->parent);
   }
}





void huffman_init (HuffmanTree32 *tree)
{
   memcpy (tree, &default_huffman_tree, sizeof(HuffmanTree32));
}



void huffman_encode_symbol (BitCoderState *bitcoder, HuffmanTree32 *t, unsigned int symbol)
{
   HuffmanNode *node, *parent;
   unsigned int n = symbol;
   uint32_t code = 0;
   int bit = 0;

   assert (symbol < 32);

   node = (HuffmanNode*) &t->leaf[symbol];

   do {
      parent = _huffman_get_node (t, node->parent);
      code |= (parent->child1 == n) << bit++;
      n = node->parent;
      node = parent;
   } while (n != ~0);
   
   while (bit)
      bitcoder_write_bit (bitcoder, code >> --bit);

   t->leaf[symbol].freq++;
   huffman_update (t, symbol | (1 << 5));
}




unsigned int huffman_decode_symbol (BitCoderState *bitcoder, HuffmanTree32 *t)
{
   unsigned int n = ~0;

   do {
      HuffmanNode *node = _huffman_get_node (t, n);
      if (bitcoder_read_bit (bitcoder))
         n = node->child1;
      else
         n = node->child0;
   } while (!n & (1 << 5));

   t->leaf[n & 0x1f].freq++;
   huffman_update (t, n);

   return (n & 0x1f);
}


