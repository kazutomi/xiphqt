#include <stdlib.h>
#include <ogg/ogg.h>
#include "theora/theora.h"
#include "huffdec.h"
#include "decint.h"


/*The ANSI offsetof macro is broken on some platforms (e.g., older DECs).*/
#define _ogg_offsetof(_type,_field)\
 ((size_t)((char *)&((_type *)0)->_field-(char *)0))



/*Allocates a Huffman tree node that represents a subtree of depth _nbits.
  _nbits: The depth of the subtree.
          If this is 0, the node is a leaf node.
          Otherwise 1<<_nbits pointers are allocated for children.
  Return: The newly allocated and fully initialized Huffman tree node.*/
static oc_huff_node *oc_huff_node_alloc(int _nbits){
  oc_huff_node *ret;
  size_t        size;
  size=_ogg_offsetof(oc_huff_node,nodes);
  if(_nbits>0)size+=sizeof(oc_huff_node *)*(1<<_nbits);
  ret=_ogg_calloc(1,size);
  ret->nbits=(unsigned char)_nbits;
  return ret;
}

/*Frees a Huffman tree node allocated with oc_huf_node_alloc.
  _node: The node to free.
         This may be NULL.*/
static void oc_huff_node_free(oc_huff_node *_node){
  free(_node);
}

/*Frees the memory used by a Huffman tree.
  _node: The Huffman tree to free.
         This may be NULL.*/
static void oc_huff_tree_free(oc_huff_node *_node){
  if(_node==NULL)return;
  if(_node->nbits){
    int nchildren;
    int i;
    nchildren=1<<_node->nbits;
    for(i=0;i<nchildren;i++)oc_huff_tree_free(_node->nodes[i]);
  }
  oc_huff_node_free(_node);
}

/*Unpacks a sub-tree from the given buffer.
  _opb:    The buffer to unpack from.
  _binode: The location to store a pointer to the sub-tree in.
  _depth:  The current depth of the tree.
           This is used to prevent infinite recursion.
  Return: 0 on success, or a negative value on error.*/
static int oc_huff_tree_unpack(oggpack_buffer *_opb,
 oc_huff_node **_binode,int _depth){
  oc_huff_node *binode;
  long          bits;
  /*Prevent infinite recursion.*/
  if(++_depth>32)return OC_BADHEADER;
  if(theora_read1(_opb,&bits)<0)return OC_BADHEADER;
  /*Read an internal node:*/
  if(!bits){
    int ret;
    binode=oc_huff_node_alloc(1);
    ret=oc_huff_tree_unpack(_opb,binode->nodes,_depth);
    if(ret>=0)ret=oc_huff_tree_unpack(_opb,binode->nodes+1,_depth);
    if(ret<0){
      oc_huff_tree_free(binode);
      return ret;
    }
  }
  /*Read a leaf node:*/
  else{
    if(theora_read(_opb,OC_NDCT_TOKEN_BITS,&bits)<0)return OC_BADHEADER;
    binode=oc_huff_node_alloc(0);
    binode->token=(unsigned char)bits;
  }
  *_binode=binode;
  return 0;
}

/*Finds the depth of shortest branch of the given sub-tree.
  The tree must be binary.
  _binode: The root of the given sub-tree.
           _binode->nbits must be 0 or 1.
  Return: The smallest depth of a leaf node in this sub-tree.
          0 indicates this sub-tree is a leaf node.*/
static int oc_huff_tree_mindepth(oc_huff_node *_binode){
  int depth0;
  int depth1;
  if(_binode->nbits==0)return 0;
  depth0=oc_huff_tree_mindepth(_binode->nodes[0]);
  depth1=oc_huff_tree_mindepth(_binode->nodes[1]);
  return OC_MINI(depth0,depth1)+1;
}

/*Fills the given nodes table with all the children in the sub-tree at the
   given depth.
  The nodes in the sub-tree with a depth less than that stored in the table
   are freed.
  The sub-tree must be binary and complete up until the given depth.
  _nodes:  The nodes table to fill.
  _binode: The root of the sub-tree to fill it with.
           _binode->nbits must be 0 or 1.
  _depth:  The depth of the nodes to fill the table with.
           0 indicates the current node should be stored in the table.*/
static void oc_huff_node_fill(oc_huff_node **_nodes,
 oc_huff_node *_binode,int _depth){
  if(_depth--<=0)_nodes[0]=_binode;
  else{
    oc_huff_node_fill(_nodes,_binode->nodes[0],_depth);
    oc_huff_node_fill(_nodes+(1<<_depth),_binode->nodes[1],_depth);
    oc_huff_node_free(_binode);
  }
}

/*Finds the largest complete sub-tree rooted at the current node and collapses
   it into a single node.
  This procedure is then applied recursively to all the children of that node.
  _binode: The root of the sub-tree to collapse.
           _binode->nbits must be 0 or 1.
  Return: The new root of the collapsed sub-tree.*/
static oc_huff_node *oc_huff_tree_collapse(oc_huff_node *_binode){
  oc_huff_node *root;
  int           nchildren;
  int           mindepth;
  int           i;
  mindepth=oc_huff_tree_mindepth(_binode);
  if(mindepth<=1)return _binode;
  root=oc_huff_node_alloc(mindepth);
  oc_huff_node_fill(root->nodes,_binode,mindepth);
  nchildren=1<<mindepth;
  for(i=0;i<nchildren;i++){
    root->nodes[i]=oc_huff_tree_collapse(root->nodes[i]);
  }
  return root;
}

/*Makes a copy of the given Huffman tree.
  _node: The Huffman tree to copy.
  Return: The copy of the Huffman tree.*/
static oc_huff_node *oc_huff_tree_copy(const oc_huff_node *_node){
  oc_huff_node *ret;
  ret=oc_huff_node_alloc(_node->nbits);
  if(_node->nbits){
    int nchildren;
    int i;
    nchildren=1<<_node->nbits;
    for(i=0;i<nchildren;i++)ret->nodes[i]=oc_huff_tree_copy(_node->nodes[i]);
  }
  else ret->token=_node->token;
  return ret;
}


/*Unpacks a set of Huffman trees, and reduces them to a collapsed
   representation.
  _opb:   The buffer to unpack the trees from.
  _nodes: The table to fill with the Huffman trees.
  Return: 0 on success, or a negative value on error.*/
int oc_huff_trees_unpack(oggpack_buffer *_opb,
 oc_huff_node *_nodes[OC_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<OC_NHUFFMAN_TABLES;i++){
    int ret;
    ret=oc_huff_tree_unpack(_opb,_nodes+i,0);
    if(ret<0)return ret;
    _nodes[i]=oc_huff_tree_collapse(_nodes[i]);
  }
  return 0;
}

/*Makes a copy of the given set of Huffman trees.
  _dst: The array to store the copy in.
  _src: The array of trees to copy.*/
void oc_huff_trees_copy(oc_huff_node *_dst[OC_NHUFFMAN_TABLES],
 const oc_huff_node *const _src[OC_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<OC_NHUFFMAN_TABLES;i++)_dst[i]=oc_huff_tree_copy(_src[i]);
}

/*Frees the memory used by a set of Huffman trees.
  _nodes: The array of trees to free.*/
void oc_huff_trees_clear(oc_huff_node *_nodes[OC_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<OC_NHUFFMAN_TABLES;i++)oc_huff_tree_free(_nodes[i]);
}

/*Unpacks a single token using the given Huffman tree.
  _opb:  The buffer to unpack the token from.
  _node: The tree to unpack the token with.
  Return: The token value.*/
int oc_huff_token_decode(oggpack_buffer *_opb,const oc_huff_node *_node){
  long bits;
  while(_node->nbits!=0){
    theora_read(_opb,_node->nbits,&bits);
    _node=_node->nodes[bits];
  }
  return _node->token;
}
