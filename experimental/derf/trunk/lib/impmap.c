#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "psych.h"

/*This is where we attempt to model high-level vision processes, such as
   object tracking and visual attention.*/

/*Importance map calculations.

  The importance map isolates "regions of interest" or areas of the image that
   are likely to draw viewer attention.
  Because the visual accuity of the fovea is so much higher than in a viewer's
   peripheral vision, bits can be taken from uninteresting areas and allocated
   to areas that the viewer is actually looking at.

  The algorithm here is mostly that which was described by Osberger in his
   Ph.D. thesis \cite{Osb99}.
  This document was once available online, but as both the student and his
   supervising professor have since left the university, it does not appear to
   be available any longer.
  The algorithm has the advantages of being simple (and thus fairly fast),
   requiring no tuning parameters, and having demonstrable correlation with
   actual human fixations (for still images only).
  It is also well described in the thesis, which makes a re-implementation
   from scratch fairly easy.

  It is, however, a fairly ad-hoc algorithm.
  While its design decisions are in fact made by taking into account real
   properties of the human visual system, they are not necessarily the only
   decisions that could be made, or even the best.
  Thus experimentation could potentially produce a better method, but this
   would require collecting our own eye-tracking data to validate the model
   against.

  Also, its extension to video has not been experimentally validated.
  Although correlation was demonstrated against actual human fixation points
   for still images, a similar experiment was not carried out for video
   sequences.

  @PHDTHESIS{Osb99,
    author="Wilfried Osberger",
    title="Perceptual Vision Models for Picture Quality Assessment and
     Compression Applications",
    school="Queensland University of Technology",
    address="Brisbane, Australia",
    month="Mar.",
    year=1999
  }*/

/*The number of weighting factors assigned to each segmented region.*/
#define OC_IMPMAP_NSEG_WEIGHTS          (5)

/*The number of weighting factors which must be rescaled into the interval
   [0,1]*/
#define OC_IMPMAP_NSEG_RESCALED_WEIGHTS (2)

/*The list of weighting factors assigned to each segmented region.*/
enum{
  /*Contrast with surrounding regions.*/
  OC_IMPMAP_WEIGHT_CONTRAST,
  /*Unusualness of the region's shape.*/
  OC_IMPMAP_WEIGHT_SHAPE,
  /*Note: These three come last, since they do not need to be rescaled.*/
  /*Size of the region.*/
  OC_IMPMAP_WEIGHT_SIZE,
  /*Closeness of the region to the image center.*/
  OC_IMPMAP_WEIGHT_LOCATION,
  /*Degree to which the region is in the foreground vs. background.*/
  OC_IMPMAP_WEIGHT_FOREGROUND
};

/*The limits for assessing motion as an attractor of attention, in deg/sec.
  Importance increases linearly from 0 to 1 over the range [min,p1], stays
   constant over [p1,p2], and decreases linearly over the range [p2,max].
  It might be wise to disable the upper limit, so that moving farther away
   from the screen does not make the image look worse as faster motions are
   slowed down enough to become visible.*/

/*Minimum important motion.*/
#define OC_IMPMAP_MOT_MIN (0.0F)
/*First peak important motion.*/
#define OC_IMPMAP_MOT_P1  (5.0F)
/*Second peak important motion.*/
#define OC_IMPMAP_MOT_P2  (10.0F)
/*Maximum important motion.*/
#define OC_IMPMAP_MOT_MAX (15.0F)

/*Image segmentation code.
  The image segmentation drives the rest of the importance map calculations.
  Improving the segmentation process improves the entire rest of the process.
  The segmentation algorithm used here roughly corresponds to the one
   described by Osberger, since minimal research (a few hours on the web) did
   not turn up anything that looked to have a better speed-quality tradeoff.

  The algorithm is a standard split-merge variant.
  The split phase starts with one region that covers the whole image.
  So long as its variance is above a threshold, it is recursively subdivided
   into four sub-regions.
  After the split phase, a merge phase combines neighboring sub-regions so
   long as the variance of the combined region stays below a threshold.
  Finally, all regions smaller than a fixed number of pixels are combined with
   their largest neighbor.
  All determinations of borders, neighbors and connectedness use
   4-connectivity.

  Although Osberger used a fixed threshold to determine whether or not to
   split or merge regions, we use the Bayesian Information Criterion instead.
  This removes the only adjustable parameter from Osberger's model.
  The BIC for a model is:

  BIC(model)=log P(data|model)-1/2(model dimension)log(data sample count).

  Using a Gaussian distribution as the model, this reduces to
   (1/2)(data sample count)log(variance)-log(data sample count).
  The last term is typically weighted by a parameter \lambda to penalize small
   samples, but that weighting is omitted here.
  Instead, like Osberger, a minimal region size is enforced.
  The minimum size allows us to place a hard upper bound on the number of
   regions in order to allocate storage once ahead of time.
  Unlike Osberger, instead of choosing the largest neighbor to merge small
   regions with, we choose the merger that yields the largest BIC increase
   (which may be negative).

  The variances are computed using a reduced-resolution table of sums.
  This table contains accumulated sums from the upper-left corner of the
   image sampled every OC_SEG_MOMENT_RES pixels.
  The initial row and column of the table contain all zeros, so that
   sum[width][height] works as expected even if width or height are 0.
  The reduced resolution is designed to reduce memory requirements, since
   otherwise they would require 8 bytes per pixel.
  With a resolution of 16 pixels, this is reduced to 1/32 byte per pixel.
  Once recursion descends below this resolution level, a local moment table is
   computed for the remaining lookups.
  Because the local moment table is small (2312 bytes), it easily fits in
   cache.
  Thus even though this method requires scanning the input image as
   much as two times, it yields a vast reduction in the amount of memory
   outside of cache that is accessed (2.125 bytes read, 0.03125 bytes written
   per pixel as compared to 10 bytes read, 8 bytes written).

  Osberger never specifies how the next region pair to consider merging is
   chosen, despite this order playing a large role in the final output.
  Our strategy is as follows:
  As soon as a region is created, its existing neighbors are enumerated and
   any potential merge pairs are put on a heap.
  The heap is then emptied by merging the pair at the top.
  The newly created region then checks its neighbors and adds those pairs to
   the heap if necessary, and the process continues until the heap is empty.
  
  First local regions are created in a local block, OC_SEG_MOMENT_RES by
   OC_SEG_MOMENT_RES pixels.
  These are given temporary, negative labels starting at -2 (-1 is reserved
   to indicate no region).
  Once all the regions in a local block are established, all the pairs
   involving a local region that does not satisfy the minimum pixel size are
   placed on the heap, and merging continues until the heap is empty.
  Thus, because small regions can only accumulate in a local block, the total
   number of regions present at any one time can be reasonably bounded.
  Otherwise, the best bound available would be the number of pixels in the
   image.
  This also removes overhead early on required to deal with one pixel regions
   that often accumulate on region borders.*/

/*The log base 2 of the resolution of the global moment table.*/
#define OC_SEG_MOMENT_RES_LOG   (4)
/*The resolution of the global moment table.*/
#define OC_SEG_MOMENT_RES       (1<<OC_SEG_MOMENT_RES_LOG)
/*A mask for the bits below the resolution of the moment table.*/
#define OC_SEG_MOMENT_RES_MASK  (OC_SEG_MOMENT_RES-1)

/*The maximum number of local regions.*/
#define OC_SEG_NLOCAL_REGS      (OC_SEG_MOMENT_RES*OC_SEG_MOMENT_RES)
/*The maximum number of local pairs.*/
#define OC_SEG_NLOCAL_PAIRS     (OC_SEG_MOMENT_RES*(3*OC_SEG_MOMENT_RES+2)-6)

/*The minimum size of a segmented region.
  Imposing this limit gives us an upper bound on the number of regions that
   can be created, which allows us to allocate storage in advance.*/
#define OC_SEG_REGION_SZ_MIN    (16)



typedef struct oc_seg_region       oc_seg_region;
typedef struct oc_seg_link         oc_seg_link;
typedef struct oc_seg_pair         oc_seg_pair;
typedef struct oc_seg_ctx          oc_seg_ctx;



/*Called when the BIC factor of a candidate merge pair has been updated to
   add it to, remove it from, or update its position in the heap.
  _seg:     The segmentation context.
  _pair:    The pair that was updated.
  _old_bic: The old BIC factor.*/
typedef void (*OCSegPairReheapFunc)(oc_seg_ctx *_seg,oc_seg_pair *_pair,
 float _old_bic);



/*Region pairs belong to two doubly linked lists: one for each region they
   belong to.
  This is a link used in those lists.
  The lists are circular, to minimize special cases, with the head/tail link
   pointing to a NULL pair contained in the region itself.*/
struct oc_seg_link{
  /*The region index of the other region in this pair.
    For the link stored within a region itself, this index is the region label
     to use after merging.*/
  int          regi;
  /*The pair this link is contained in, or NULL if it is the head/tail link.*/
  oc_seg_pair *pair;
  /*The next link in the chain.*/
  oc_seg_link *next;
  /*The previous link in the chain.*/
  oc_seg_link *prev;
};

struct oc_seg_region{
  /*The list of pairs to which this region belongs.*/
  oc_seg_link link;
  /*The number of pixels in the region.*/
  int         npixels;
  /*The sum of the pixel values in the region.*/
  float       sumx;
  /*The sum of the squared pixel values in the region.*/
  float       sumx2;
  /*The base BIC level used to drive merge decisions.*/
  float       bic;
};

/*A pair of neighboring regions.*/
struct oc_seg_pair{
  /*The links for this pair.*/
  oc_seg_link   links[2];
  /*The number of pixels in the region.*/
  int           npixels;
  /*The sum of the pixel values in the region.*/
  float         sumx;
  /*The sum of the squared pixel values in the region.*/
  float         sumx2;
  /*The base BIC level used to drive merge decisions.*/
  float         bic;
  /*The location in the merge heap.*/
  int           heapi;
  /*The BIC delta for keeping the regions separate.*/
  float         dbic;
};

/*Buffers and tables needed to drive the segmentation process.*/
struct oc_seg_ctx{
  /*The minimum power of 2 greater than or equal to the image width and
     height.*/
  int                   level_max;
  /*The global first moment table.*/
  float               **sumx;
  /*The global second moment table.*/
  float               **sumx2;
  /*The local first moment table.*/
  float               **local_sumx;
  /*The local second moment table.*/
  float               **local_sumx2;
  /*The region labels for each pixel.*/
  int                 **labels;
  /*The number of local regions in a local segmentation.
    This count starts at -2 and goes down as each region is added.*/
  int                   nlocal_regions;
  /*The number of regions in the segmentation.*/
  int                   nregions;
  /*The total number of regions allocated.*/
  int                   cregions;
  /*The list of regions.*/
  oc_seg_region        *regions;
  /*The number of pairs in use.*/
  int                   npairs;
  /*The size of the global heap.*/
  int                   nheap;
  /*The storage for the neighbor lists for each region.*/
  oc_seg_pair          *pairs;
  /*The list of available pairs.*/
  oc_seg_link          *free_pairs;
  /*The global merge heap.*/
  oc_seg_pair         **heap;
  /*The reheaping function.*/
  OCSegPairReheapFunc   reheap;
};



typedef struct oc_impmap_region   oc_impmap_region;



/*The per-region information gathered for the importance map.*/
struct oc_impmap_region{
  /*The number of pixels on the region's border.*/
  int              nborder;
  /*The number of pixels on the region's edge.*/
  int              nedge;
  /*The number of pixels in the central 25% of the image.*/
  int              ncenter;
  /*The weight given to the region because of its contrast.*/
  float            weights[OC_IMPMAP_NSEG_WEIGHTS];
  /*The final importance of the region.*/
  float            importance;
};

/*Importance map context information.*/
struct oc_impmap_ctx{
  /*Segmentation information for the Y plane.
    Some day we may also wish to segment the U and V planes, or derive a
     segmentation using all 3 planes at once.*/
  oc_seg_ctx        seg;
  /*The reciprocal of the maximum region size to count towards a region's
     importance.
    This cap prevents very large regions from dominating the scene.*/
  float             inv_region_sz_max;
  /*The reciprocal of the maximum number of edge pixels required to consider a
     region completely in the background.
    This value is used when computing the foreground weight of a region.*/
  float             inv_edge_sz_max;
  /*The per-region information gathered for the importance map.*/
  oc_impmap_region *regions;
};



/*The following masks are used to test quad-tree child nodes for boundary
   conditions.
  The bits they are tested against are the following:
  0: size>width
  1: size>height
  2: (size/2)>width
  3: (size/2)>height
  4: (size/2)==width
  5: (size/2)==height
  Here "size" is the size of quadtree nodes at the parent's level, and "width"
   and "height" are the dimensions of the part of the parent node that lies
   inside the image.

  The quadtree nodes are organized as:
  +---+---+
  | 0 | 1 |
  +---+---+
  | 2 | 3 |
  +---+---+
*/

/*If any of the bits in these masks are set, the corresponding child node
   is completely clipped by the border.*/
static const int OC_SEG_CLIP_MASKS[4]={0x0,0x14,0x28,0x3C};
/*If the bit in these masks is set, the width of the child node is partially
   clipped by the border.*/
static const int OC_SEG_BORDER_XMASKS[4]={0x4,0x1,0x4,0x1};
/*If the bit in these masks is set, the height of the child node is partially
   clipped by the border.*/
static const int OC_SEG_BORDER_YMASKS[4]={0x8,0x8,0x2,0x2};
/*If any of the bits in these masks are set, either the width or height of the
   child node is partially clipped by the border.*/
static const int OC_SEG_BORDER_MASKS[4]={0xC,0x9,0x6,0x3};



/*Computes moments for an image row, saving samples every
   OC_SEG_MOMENT_RES pixels.
  These moments are added to the previous row value.
  _xrow:       The storage for the first moments.
  _x2row:      The storage for the second moments.
  _xrow_prev:  The previous first moments for the row.
  _x2row_prev: The previous second moments for the row.
  _irow:       The image row to accumulate.
  _width:      The width of the row in pixels.*/
static void oc_seg_sum_moment_row(float *_xrow,float *_x2row,
 const float *_xrow_prev,const float *_x2row_prev,
 const unsigned char *_irow,int _width){
  float xsum;
  float x2sum;
  int   xmax;
  int   x;
  int i;
  xmax=_width>>OC_SEG_MOMENT_RES_LOG;
  xsum=x2sum=0;
  for(x=1;x<=xmax;x++){
    for(i=0;i<OC_SEG_MOMENT_RES;i++){
      xsum+=*_irow;
      x2sum+=*_irow**_irow;
      _irow++;
    }
    _xrow[x]=xsum+_xrow_prev[x];
    _x2row[x]=x2sum+_x2row_prev[x];
  }
  i=_width&OC_SEG_MOMENT_RES_MASK;
  if(i>0){
    do{
      xsum+=*_irow;
      x2sum+=*_irow**_irow;
      _irow++;
    }
    while(--i>0);
    _xrow[x]=xsum+_xrow_prev[x];
    _x2row[x]=x2sum+_x2row_prev[x];
  }
}

/*Sum up 2-dimensional moments over the whole image.
  These are used for fast variance calculations over rectangular
   regions of the image during the split phase.
  The value at each position is the moment summed over the image
   region above and to the left.
  _seg:   The image segmentation context.
  _plane: The image plane to sum the moments over.
          This should already be adjusted to cover only the visible
           frame, and not the full encoded frame.*/
static void oc_seg_sum_moments(oc_seg_ctx *_seg,
 const oc_img_plane *_plane){
  const unsigned char *ypix;
  int                  ymax;
  int                  y;
  int                  i;
  ypix=_plane->data;
  ymax=_plane->height>>OC_SEG_MOMENT_RES_LOG;
  for(y=1;y<=ymax;y++){
    oc_seg_sum_moment_row(_seg->sumx[y],_seg->sumx2[y],
     _seg->sumx[y-1],_seg->sumx2[y-1],ypix,_plane->width);
    ypix+=_plane->stride;
    for(i=1;i<OC_SEG_MOMENT_RES;i++){
      oc_seg_sum_moment_row(_seg->sumx[y],_seg->sumx2[y],
       _seg->sumx[y],_seg->sumx2[y],ypix,_plane->width);
      ypix+=_plane->stride;
    }
  }
  i=_plane->height&OC_SEG_MOMENT_RES_MASK;
  if(i>0){
    oc_seg_sum_moment_row(_seg->sumx[y],_seg->sumx2[y],
     _seg->sumx[y-1],_seg->sumx2[y-1],ypix,_plane->width);
    ypix+=_plane->stride;
    while(--i>0){
      oc_seg_sum_moment_row(_seg->sumx[y],_seg->sumx2[y],
       _seg->sumx[y],_seg->sumx2[y],ypix,_plane->width);
      ypix+=_plane->stride;
    }
  }
}

/*Computes the sum over the given region using the given moment
   table.
  Note that coordinates are those of the table, not of the image from which
   the moments were computed.
  _sums:    The pre-computed moment table.
  _tx:      The X coordinate in the table of the upper-left hand corner of the
             region.
  _ty:      The Y coordinate in the table of the upper-right hand corner of
             the region.
  _twidth:  The width of the region in the table.
  _theight: The height of the region in the table.
  Return: The sum of the appropriate value over the given region.*/
static float oc_seg_sum_region(const float **_sums,int _tx,int _ty,
 int _twidth,int _theight){
  return _sums[_ty][_tx]-_sums[_ty][_tx+_twidth]-
   _sums[_ty+_theight][_tx]+_sums[_ty+_theight][_tx+_twidth];
}

static int oc_seg_get_label(oc_seg_ctx *_seg,int _label){
  oc_seg_region *reg;
  reg=_seg->regions+_label;
  if(reg->link.regi!=_label){
    return reg->link.regi=oc_seg_get_label(_seg,reg->link.regi);
  }
  return _label;
}

static oc_seg_pair *oc_seg_pair_alloc(oc_seg_ctx *_seg){
  oc_seg_pair *ret;
  if(_seg->free_pairs!=NULL){
    ret=_seg->free_pairs->pair;
    _seg->free_pairs=_seg->free_pairs->next;
  }
  else ret=_seg->pairs+_seg->npairs++;
  return ret;
}

static void oc_seg_pair_free(oc_seg_ctx *_seg,oc_seg_pair *_pair){
  _pair->links[0].next=_seg->free_pairs;
  _seg->free_pairs=_pair->links+0;
}

static void oc_seg_pair_heap_up(oc_seg_ctx *_seg,int _heapi){
  oc_seg_pair **heap;
  int           p;
  p=_heapi;
  heap=_seg->heap;
  while(p>0){
    oc_seg_pair *t;
    int          q;
    q=p;
    p=(q+1>>1)-1;
    if(heap[p]->dbic<=heap[q]->dbic)break;
    heap[p]->heapi=q;
    heap[q]->heapi=p;
    t=heap[p];
    heap[p]=heap[q];
    heap[q]=t;
  }
}

static void oc_seg_pair_heap_down(oc_seg_ctx *_seg,int _heapi){
  oc_seg_pair **heap;
  int           l;
  int           r;
  int           p;
  heap=_seg->heap;
  l=_seg->nheap>>1;
  r=_seg->nheap-1;
  p=_heapi;
  while(p<l){
    oc_seg_pair *t;
    int          q;
    q=(p<<1)+1;
    if(q<r&&heap[q]->dbic>=heap[q+1]->dbic)q++;
    if(heap[p]->dbic<=heap[q]->dbic)break;
    heap[p]->heapi=q;
    heap[q]->heapi=p;
    t=heap[p];
    heap[p]=heap[q];
    heap[q]=t;
    p=q;
  }
}

static void oc_seg_pair_heapify(oc_seg_ctx *_seg){
  oc_seg_pair **heap;
  int           l;
  int           r;
  int           i;
  heap=_seg->heap;
  l=_seg->nheap>>1;
  r=_seg->nheap-1;
  for(i=l;i-->0;){
    int p;
    p=i;
    do{
      oc_seg_pair *t;
      int          q;
      q=(p<<1)+1;
      if(q<r&&heap[q]->dbic>=heap[q+1]->dbic)q++;
      if(heap[p]->dbic<=heap[q]->dbic)break;
      heap[p]->heapi=q;
      heap[q]->heapi=p;
      t=heap[p];
      heap[p]=heap[q];
      heap[q]=t;
      p=q;
    }
    while(p<l);
  }
}

static void oc_seg_pair_heap_add(oc_seg_ctx *_seg,oc_seg_pair *_pair){
  _seg->heap[_seg->nheap]=_pair;
  _pair->heapi=_seg->nheap++;
  oc_seg_pair_heap_up(_seg,_pair->heapi);
}

static void oc_seg_pair_heap_delhead(oc_seg_ctx *_seg){
  if(--_seg->nheap>0){
    _seg->heap[0]=_seg->heap[_seg->nheap];
    _seg->heap[0]->heapi=0;
    oc_seg_pair_heap_down(_seg,0);
  }
}

static void oc_seg_pair_heap_del(oc_seg_ctx *_seg,oc_seg_pair *_pair){
  int heapi;
  heapi=_pair->heapi;
  if(heapi>=0){
    _seg->nheap--;
    if(_seg->nheap>0){
      _seg->heap[heapi]=_seg->heap[_seg->nheap];
      _seg->heap[heapi]->heapi=heapi;
      if(_seg->heap[heapi]->dbic<_pair->dbic)oc_seg_pair_heap_up(_seg,heapi);
      else oc_seg_pair_heap_down(_seg,heapi);
    }
  }
}

static void oc_seg_pair_reheap(oc_seg_ctx *_seg,oc_seg_pair *_pair,
 float _old_bic){
  /*Place this pair on the merge heap if necessary.*/
  if(_pair->dbic<=0){
    if(_pair->heapi<0)oc_seg_pair_heap_add(_seg,_pair);
    else if(_old_bic<_pair->dbic)oc_seg_pair_heap_down(_seg,_pair->heapi);
    else oc_seg_pair_heap_up(_seg,_pair->heapi);
  }
  else if(_pair->heapi>=0){
    oc_seg_pair_heap_del(_seg,_pair);
    _pair->heapi=-1;
  }
}

static void oc_seg_pair_reheap_small(oc_seg_ctx *_seg,oc_seg_pair *_pair,
 float _old_bic){
  /*Place this pair on the merge heap if necessary.*/
  if(_pair->dbic<=0||
   (_seg->regions+_pair->links[0].regi)->npixels<OC_SEG_REGION_SZ_MIN||
   (_seg->regions+_pair->links[1].regi)->npixels<OC_SEG_REGION_SZ_MIN){
    if(_pair->heapi<0)oc_seg_pair_heap_add(_seg,_pair);
    else if(_old_bic<_pair->dbic)oc_seg_pair_heap_down(_seg,_pair->heapi);
    else oc_seg_pair_heap_up(_seg,_pair->heapi);
  }
  else if(_pair->heapi>=0){
    oc_seg_pair_heap_del(_seg,_pair);
    _pair->heapi=-1;
  }
}

static void oc_seg_pair_update(oc_seg_ctx *_seg,oc_seg_pair *_pair){
  oc_seg_region *reg0;
  oc_seg_region *reg1;
  float          area;
  float          var;
  float          old_bic;
  /*Compute the statistics of the merged region.*/
  reg0=_seg->regions+_pair->links[0].regi;
  reg1=_seg->regions+_pair->links[1].regi;
  _pair->npixels=reg0->npixels+reg1->npixels;
  _pair->sumx=reg0->sumx+reg1->sumx;
  _pair->sumx2=reg0->sumx2+reg1->sumx2;
  area=(float)_pair->npixels;
  var=(_pair->sumx2*area-_pair->sumx*_pair->sumx)/(area*area);
  if(var>0)_pair->bic=0.5F*area*(float)log(var);
  else _pair->bic=0;
  old_bic=_pair->dbic;
  _pair->dbic=_pair->bic-reg0->bic-reg1->bic-(float)log(area);
  _seg->reheap(_seg,_pair,old_bic);
}

static void oc_seg_create_pair(oc_seg_ctx *_seg,int _regi0,int _regi1){
  oc_seg_region *reg0;
  oc_seg_region *reg1;
  oc_seg_link   *link;
  oc_seg_pair   *pair;
  reg1=_seg->regions+_regi1;
  if(reg1->link.next->regi==_regi0)return;
  for(link=reg1->link.next->next;link->pair!=NULL;link=link->next){
    if(link->regi==_regi0){
      link->prev->next=link->next;
      link->next->prev=link->prev;
      link->next=reg1->link.next;
      link->prev=&reg1->link;
      link->next->prev=link;
      reg1->link.next=link;
      return;
    }
  }
  /*Add the pair to the regions' lists.*/
  reg0=_seg->regions+_regi0;
  pair=oc_seg_pair_alloc(_seg);
  pair->links[0].regi=_regi0;
  pair->links[0].pair=pair;
  pair->links[0].next=reg1->link.next;
  pair->links[0].prev=&reg1->link;
  reg1->link.next->prev=pair->links+0;
  reg1->link.next=pair->links+0;
  pair->links[1].regi=_regi1;
  pair->links[1].pair=pair;
  pair->links[1].next=reg0->link.next;
  pair->links[1].prev=&reg0->link;
  reg0->link.next->prev=pair->links+1;
  reg0->link.next=pair->links+1;
  pair->heapi=-1;
  oc_seg_pair_update(_seg,pair);
}

/*Merges the list of pairs in region _srci into the list in region _dsti,
   relabling _srci to _dsti.
  After merging, any pairs that were already in region _dsti (after relabling)
   are left in _srci's list.
  The (_dsti,_srci) pair is discarded from both lists.
  _seg:  The segmentation context.
  _dsti: The destination region number.
  _srci: The source region number.*/
static void oc_seg_merge_pair_lists(oc_seg_ctx *_seg,int _dsti,int _srci){
  oc_seg_region  *dst;
  oc_seg_region  *src;
  oc_seg_link    *dlink;
  oc_seg_link    *dhead;
  oc_seg_link    *slink;
  oc_seg_link    *slink2;
  oc_seg_link    *snext;
  dst=_seg->regions+_dsti;
  src=_seg->regions+_srci;
  dhead=dst->link.next;
  for(slink=src->link.next;slink->pair!=NULL;slink=snext){
    snext=slink->next;
    /*Grab a pointer to the other link in the pair.*/
    slink2=slink->pair->links+(slink->pair->links+1-slink);
    for(dlink=dhead;;dlink=dlink->next){
      /*If we've reached the end of the list, then the pair was not found in
         dst, so add it.*/
      if(dlink->pair==NULL){
        slink->next=dst->link.next;
        slink->prev=&dst->link;
        dst->link.next->prev=slink;
        dst->link.next=slink;
        /*Change the region index of the other link.*/
        slink2->regi=_dsti;
        break;
      }
      /*If we found a duplicate pair, move the duplicate off the part of the
         list we're scanning, and delete the original from the other list it
         is in.*/
      else if(dlink->regi==slink->regi){
        if(dlink==dhead)dhead=dlink->next;
        else{
          dlink->next->prev=dlink->prev;
          dlink->prev->next=dlink->next;
          dlink->next=dst->link.next;
          dlink->prev=&dst->link;
          dst->link.next->prev=dlink;
          dst->link.next=dlink;
        }
        slink2->next->prev=slink2->prev;
        slink2->prev->next=slink2->next;
        /*And remove the pair from the merge heap if necessary.*/
        oc_seg_pair_heap_del(_seg,slink->pair);
        /*And finally free it.*/
        oc_seg_pair_free(_seg,slink->pair);
        break;
      }
    }
  }
}

static void oc_seg_merge(oc_seg_ctx *_seg){
  /*Resolve all merge candidates.*/
  while(_seg->nheap>0){
    oc_seg_pair   *pair;
    oc_seg_link   *link;
    oc_seg_region *reg[2];
    int            dst;
    pair=_seg->heap[0];
    /*We want to be sure we always prefer any global region over a local one,
       but otherwise we use the inherent order of the pair, which is usually
       a good one.*/
    dst=pair->links[0].regi<0&&0<=pair->links[1].regi;
    reg[0]=_seg->regions+pair->links[0].regi;
    reg[1]=_seg->regions+pair->links[1].regi;
    pair->links[0].next->prev=pair->links[0].prev;
    pair->links[0].prev->next=pair->links[0].next;
    pair->links[1].next->prev=pair->links[1].prev;
    pair->links[1].prev->next=pair->links[1].next;
    oc_seg_pair_heap_delhead(_seg);
    oc_seg_merge_pair_lists(_seg,pair->links[dst].regi,
     pair->links[1-dst].regi);
    oc_seg_pair_free(_seg,pair);
    reg[dst]->npixels=pair->npixels;
    reg[dst]->sumx=pair->sumx;
    reg[dst]->sumx2=pair->sumx2;
    reg[dst]->bic=pair->bic;
    reg[1-dst]->link.regi=reg[dst]->link.regi;
    for(link=reg[dst]->link.next;link->pair!=NULL;link=link->next){
      oc_seg_pair_update(_seg,link->pair);
    }
  }
}

/*Attempts to merge a block that can't be split anymore with existing,
   neighboring local regions.
  If a successful candidate cannot be found, a new local region is created.
  _seg:    The segmentation context.
  _x0:     The X coordinate of the upper-left hand corner of the local block.
  _y0:     The Y coordinate of the upper-left hand corner of the local block.
  _dx:     The X offset of the local region in the local block.
  _dy:     The Y offset of the local region in the lcoal block.
  _width:  The width of the local region.
  _height: The height of the local region.
  _area:   The area of the local region.
  _sumx:   The sum of the pixel values over the local region.
  _sumx2   The sum of the squared pixel values over the local region.
  _var:    The variance of the local region.*/
static void oc_seg_merge_l(oc_seg_ctx *_seg,int _x0,int _y0,int _width,
 int _height,int _area,float _sumx,float _sumx2,float _var){
  oc_seg_region *reg;
  int            regi;
  int            xend;
  int            yend;
  int            x;
  int            y;
  xend=_x0+_width;
  yend=_y0+_height;
  /*Set up the region structure.*/
  regi=_seg->nlocal_regions--;
  reg=_seg->regions+regi;
  reg->link.regi=regi;
  reg->link.pair=NULL;
  reg->link.next=&reg->link;
  reg->link.prev=&reg->link;
  reg->npixels=_area;
  reg->sumx=_sumx;
  reg->sumx2=_sumx2;
  reg->bic=_var>0?0.5F*_area*(float)log(_var):0;
  /*Scan the left edge, if present, for adjacent regions.*/
  if(_x0>0)for(y=_y0;y<yend;y++){
    oc_seg_create_pair(_seg,oc_seg_get_label(_seg,_seg->labels[y][_x0-1]),
     regi);
  }
  /*Scan the top edge, if present, for adjacent regions.*/
  if(_y0>0)for(x=_x0;x<xend;x++){
    oc_seg_create_pair(_seg,oc_seg_get_label(_seg,_seg->labels[_y0-1][x]),
     regi);
  }
  /*Merge all mergeable regions.*/
  oc_seg_merge(_seg);
  /*If we merged away the region we just created, set it up for re-use.*/
  if(reg->link.regi!=regi){
    _seg->nlocal_regions++;
    regi=oc_seg_get_label(_seg,regi);
  }
  /*Fill the block with the new region label.*/
  for(y=_y0;y<yend;y++)for(x=_x0;x<xend;x++)_seg->labels[y][x]=regi;
}

/*Attempts to merge the local regions with neighboring global regions.
  If a successful candidate cannot be found, and the local region is too
   small, it is merged with its largest neighbor, local or not.
  Otherwise, a new region is created.
  _seg:    The segmentation context.
  _x0:     The X coordinate of the upper-left hand corner of the local block.
  _y0:     The Y coordinate of the upper-left hand corner of the local block.
  _width:  The width of the block.
  _height: The height of the block.*/
static void oc_seg_merge_s(oc_seg_ctx *_seg,int _x0,int _y0,int _width,
 int _height){
  oc_seg_region *reg;
  oc_seg_region *lreg;
  oc_seg_link   *link;
  oc_seg_pair   *pair;
  int            regi;
  int            lregi;
  int            xend;
  int            yend;
  int            x;
  int            y;
  /*Step 1: Add all the small region pairs to the heap.*/
  for(lregi=-1;--lregi>_seg->nlocal_regions;){
    lreg=_seg->regions+lregi;
    /*If this region was not already merged with another and is small...*/
    if(lreg->link.regi==lregi&&lreg->npixels<OC_SEG_REGION_SZ_MIN){
      /*Add its pairs to the merge heap, regardless of whether or not they're
         beneficial.*/
      for(link=lreg->link.next;link->pair!=NULL;link=link->next){
        pair=link->pair;
        if(pair->heapi<0){
          pair->heapi=_seg->nheap++;
          _seg->heap[pair->heapi]=pair;
        }
      }
    }
  }
  oc_seg_pair_heapify(_seg);
  /*Step 2: Merge all the small regions.*/
  _seg->reheap=oc_seg_pair_reheap_small;
  oc_seg_merge(_seg);
  _seg->reheap=oc_seg_pair_reheap;
  /*Step 3: Allocate global regions for each local region that survived.*/
  for(lregi=-1;--lregi>_seg->nlocal_regions;){
    lreg=_seg->regions+lregi;
    /*If this region was not already merged with another...*/
    if(lreg->link.regi==lregi){
      /*Create a new global region out of it.*/
      regi=_seg->nregions++;
      reg=_seg->regions+regi;
      *&reg->link=*&lreg->link;
      reg->link.regi=regi;
      reg->npixels=lreg->npixels;
      reg->sumx=lreg->sumx;
      reg->sumx2=lreg->sumx2;
      lreg->link.regi=regi;
      /*Relabel the region in all of its merge pairs.*/
      for(link=reg->link.next;link->pair!=NULL;link=link->next){
        pair=link->pair;
        pair->links[(pair->links+1-link)].regi=regi;
      }
    }
  }
  /*Step 4: Update all pixel labels to refer to the global region labels.*/
  for(lregi=-1;--lregi>_seg->nlocal_regions;)oc_seg_get_label(_seg,lregi);
  xend=_x0+_width;
  yend=_y0+_height;
  for(y=_y0;y<yend;y++)for(x=_x0;x<xend;x++){
    _seg->labels[y][x]=(_seg->regions+_seg->labels[y][x])->link.regi;
  }
}

/*Attempts to merge a block that can't be split anymore with existing,
   neighboring regions.
  If a successful candidate cannot be found, a new region is created.
  _seg:    The segmentation context.
  _x0:     The X coordinate of the upper-left hand corner of the block.
  _y0:     The Y coordinate of the upper-left hand corner of the block.
  _width:  The width of the block.
  _height: The height of the block.
  _area:   The area of the block.
  _sumx:   The sum of the pixel values over the block.
  _sumx2   The sum of the squared pixel values over the block.
  _var:    The variance of the block.*/
static void oc_seg_merge_t(oc_seg_ctx *_seg,int _x0,int _y0,int _width,
 int _height,int _area,float _sumx,float _sumx2,float _var){
  oc_seg_region *reg;
  int            regi;
  int            xend;
  int            yend;
  int            x;
  int            y;
  xend=_x0+_width;
  yend=_y0+_height;
  /*Set up the region structure.*/
  regi=_seg->nregions++;
  reg=_seg->regions+regi;
  reg->link.regi=regi;
  reg->link.pair=NULL;
  reg->link.next=&reg->link;
  reg->link.prev=&reg->link;
  reg->npixels=_area;
  reg->sumx=_sumx;
  reg->sumx2=_sumx2;
  reg->bic=_var>0?0.5F*_area*(float)log(_var):0;
  /*Scan the left edge, if present, for adjacent regions.*/
  if(_x0>0)for(y=_y0;y<yend;y++){
    oc_seg_create_pair(_seg,oc_seg_get_label(_seg,_seg->labels[y][_x0-1]),
     regi);
  }
  /*Scan the top edge, if present, for adjacent regions.*/
  if(_y0>0)for(x=_x0;x<xend;x++){
    oc_seg_create_pair(_seg,oc_seg_get_label(_seg,_seg->labels[_y0-1][x]),
     regi);
  }
  /*Merge all mergeable regions.*/
  oc_seg_merge(_seg);
  /*If we merged away the region we just created, set it up for re-use.*/
  if(reg->link.regi!=regi){
    _seg->nregions--;
    regi=oc_seg_get_label(_seg,regi);
  }
  /*Fill the block with the new region label.*/
  for(y=_y0;y<yend;y++)for(x=_x0;x<xend;x++)_seg->labels[y][x]=regi;
}

/*Recursive split-merge step.
  Version for interior regions with local table lookups.
  _seg:    The segmentation context.
  _x0:     The X coordinate of the upper-left hand corner of the local region.
  _y0:     The Y coordinate of the upper-left hand corner of the local region.
  _dx:     The X coordinate of the upper-left hand corner of the block
            relative to the local region.
  _dy:     The Y coordinate of the upper-left hand corner of the block
            relative to the local region.
  _level:  The log base 2 of the block size.
  _area:   The area of the block in pixels^2.
  _sumx:   The sum of the pixel values over the block.
  _sumx2:  The sum of the pixel values squared over the block.
  _var:    The variance of the block.*/
static void oc_seg_splitmerge_il(oc_seg_ctx *_seg,int _x0,int _y0,
 int _dx,int _dy,int _level,int _area,float _sumx,float _sumx2,float _var){
  int size;
  if(_level>0){
    float var[4];
    float sumx[4];
    float sumx2[4];
    float bic;
    int   dx[4];
    int   dy[4];
    int   area;
    int   size_2;
    int   i;
    _level--;
    area=1<<(_level<<1);
    size_2=1<<_level;
    dx[0]=dx[2]=dy[0]=dy[1]=0;
    dx[1]=dx[3]=dy[2]=dy[3]=size_2;
    bic=_var>0?0.5F*_area*(float)log(_var):0;
    for(i=0;i<4;i++){
      sumx[i]=oc_seg_sum_region(_seg->local_sumx,_dx+dx[i],_dy+dy[i],size_2,
       size_2);
      sumx2[i]=oc_seg_sum_region(_seg->local_sumx2,_dx+dx[i],_dy+dy[i],
       size_2,size_2);
      var[i]=(sumx2[i]*area-sumx[i]*sumx[i])/(area*area);
      if(var[i]>0)bic-=0.5F*area*(float)log(var[i]);
    }
    bic-=3*(float)log(_area);
    if(bic>=0){
      for(i=0;i<4;i++){
        oc_seg_splitmerge_il(_seg,_x0,_y0,_dx+dx[i],_dy+dy[i],_level,area,
         sumx[i],sumx2[i],var[i]);
      }
      return;
    }
    _level++;
  }
  size=1<<_level;
  oc_seg_merge_l(_seg,_x0+_dx,_y0+_dy,size,size,_area,_sumx,_sumx2,_var);
}

/*Recursive split-merge step.
  Version for border regions with local table lookups.
  _seg:    The segmentation context.
  _x0:     The X coordinate of the upper-left hand corner of the local region.
  _y0:     The Y coordinate of the upper-left hand corner of the local region.
  _dx:     The X coordinate of the upper-left hand corner of the block
            relative to the local region.
  _dy:     The Y coordinate of the upper-left hand corner of the block
            relative to the local region.
  _width:  The width of the block in pixels.
  _height: The height of the block in pixels.
  _level:  The log base 2 of the block size.
  _area:   The area of the block in pixels^2.
  _sumx:   The sum of the pixel values over the block.
  _sumx2:  The sum of the pixel values squared over the block.
  _var:    The variance of the block.*/
static void oc_seg_splitmerge_bl(oc_seg_ctx *_seg,
 int _x0,int _y0,int _dx,int _dy,int _width,int _height,int _level,
 int _area,float _sumx,float _sumx2,float _var){
  int size;
  size=1<<_level;
  if(_level>0){
    float var[4];
    float sumx[4];
    float sumx2[4];
    float bic;
    float log_area;
    int   dx[4];
    int   dy[4];
    int   area[4];
    int   width[4];
    int   height[4];
    int   size_2;
    int   b;
    int   i;
    _level--;
    size_2=1<<_level;
    dx[0]=dx[2]=dy[0]=dy[1]=0;
    dx[1]=dx[3]=dy[2]=dy[3]=size_2;
    b=(size>_width)|((size>_height)<<1)|
     ((size_2>_width)<<2)|((size_2>_height)<<3)|
     ((size_2==_width)<<4)|((size_2==_height)<<5);
    log_area=(float)log(_area);
    bic=_var>0?0.5F*_area*(float)log(_var):0;
    bic+=log_area;
    for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
      width[i]=(b&OC_SEG_BORDER_XMASKS[i])?_width-dx[i]:size;
      height[i]=(b&OC_SEG_BORDER_YMASKS[i])?_height-dy[i]:size;
      area[i]=width[i]*height[i];
      sumx[i]=oc_seg_sum_region(_seg->local_sumx,_dx+dx[i],_dy+dy[i],size,
       size);
      sumx2[i]=oc_seg_sum_region(_seg->local_sumx2,_dx+dx[i],_dy+dy[i],size,
       size);
      var[i]=(sumx2[i]*area[i]-sumx[i]*sumx[i])/(area[i]*area[i]);
      if(var[i]>0)bic-=0.5F*area[i]*(float)log(var[i]);
      bic-=log_area;
    }
    if(bic>=0){
      for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
        if(b&OC_SEG_BORDER_MASKS[i]){
          oc_seg_splitmerge_bl(_seg,_x0,_y0,_dx+dx[i],_dy+dy[i],
           width[i],height[i],_level,area[i],sumx[i],sumx2[i],var[i]);
        }
        else{
          oc_seg_splitmerge_il(_seg,_x0,_y0,_dx+dx[i],_dy+dy[i],
           _level,area[i],sumx[i],sumx2[i],var[i]);
        }
      }
      return;
    }
  }
  oc_seg_merge_l(_seg,_x0+_dx,_y0+_dy,_width,_height,_area,_sumx,_sumx2,_var);
}

/*Recursive split-merge step.
  Version for interior regions with manual summing.
  _seg:   The segmentation context.
  _x0:    The X coordinate of the upper-left hand corner of the block.
  _y0:    The Y coordinate of the upper-left hand corner of the block.
  _level: The log base 2 of the block size.
  _area:   The area of the block in pixels^2.
  _sumx:   The sum of the pixel values over the block.
  _sumx2:  The sum of the pixel values squared over the block.
  _var:    The variance of the block.
  _plane:  The image plane the block is contained in.*/
static void oc_seg_splitmerge_is(oc_seg_ctx *_seg,
 int _x0,int _y0,int _level,int _area,float _sumx,float _sumx2,float _var,
 const oc_img_plane *_plane){
  const unsigned char *ypix;
  int                  size;
  int                  y;
  int                  x;
  size=1<<_level;
  /*Compute a pixel-level moment table for this local region.*/
  ypix=_plane->data+_y0*_plane->stride+_x0;
  for(y=1;y<=size;y++){
    const unsigned char *xpix;
    float                xsum;
    float                x2sum;
    xpix=ypix;
    xsum=x2sum=0;
    for(x=1;x<=size;x++){
      xsum+=*xpix;
      x2sum+=*xpix**xpix;
      _seg->local_sumx[y][x]=_seg->local_sumx[y-1][x]+xsum;
      _seg->local_sumx2[y][x]=_seg->local_sumx2[y-1][x]+x2sum;
      xpix++;
    }
    ypix+=_plane->stride;
  }
  _seg->nlocal_regions=-2;
  oc_seg_splitmerge_il(_seg,_x0,_y0,0,0,_level,_area,_sumx,_sumx2,_var);
  oc_seg_merge_s(_seg,_x0,_y0,size,size);
}

/*Recursive split-merge step.
  Version for border regions with manual summing.
  _seg:   The segmentation context.
  _x0:    The X coordinate of the upper-left hand corner of the block.
  _y0:    The Y coordinate of the upper-left hand corner of the block.
  _width:  The width of the block in pixels.
  _height: The height of the block in pixels.
  _level: The log base 2 of the block size.
  _area:   The area of the block in pixels^2.
  _sumx:   The sum of the pixel values over the block.
  _sumx2:  The sum of the pixel values squared over the block.
  _var:    The variance of the block.
  _plane:  The image plane the block is contained in.*/
static void oc_seg_splitmerge_bs(oc_seg_ctx *_seg,
 int _x0,int _y0,int _width,int _height,int _level,
 int _area,float _sumx,float _sumx2,float _var,const oc_img_plane *_plane){
  const unsigned char *ypix;
  int                  size;
  int                  y;
  int                  x;
  size=1<<_level;
  /*Compute a pixel-level moment table for this local region.*/
  ypix=_plane->data+_y0*_plane->stride+_x0;
  for(y=1;y<=_height;y++){
    const unsigned char *xpix;
    float                xsum;
    float                x2sum;
    xpix=ypix;
    xsum=x2sum=0;
    for(x=1;x<=_width;x++){
      xsum+=*xpix;
      x2sum+=*xpix**xpix;
      _seg->local_sumx[y][x]=_seg->local_sumx[y-1][x]+xsum;
      _seg->local_sumx2[y][x]=_seg->local_sumx2[y-1][x]+x2sum;
      xpix++;
    }
    for(;x<=size;x++){
      _seg->local_sumx[y][x]=_seg->local_sumx[y-1][x]+xsum;
      _seg->local_sumx2[y][x]=_seg->local_sumx2[y-1][x]+x2sum;
    }
    ypix+=_plane->stride;
  }
  for(;y<=size;y++){
    for(x=_width+1;x<=size;x++){
      _seg->local_sumx[y][x]=_seg->local_sumx[y-1][x];
      _seg->local_sumx2[y][x]=_seg->local_sumx2[y-1][x];
    }
  }
  _seg->nlocal_regions=-2;
  oc_seg_splitmerge_bl(_seg,_x0,_y0,0,0,_width,_height,_level,
   _area,_sumx,_sumx2,_var);
  oc_seg_merge_s(_seg,_x0,_y0,_width,_height);
}

/*Recursive split-merge step.
  Version for interior regions with reduced resolution table lookups.
  _seg:   The segmentation context.
  _x0:    The X coordinate of the upper-left hand corner of the block.
  _y0:    The Y coordinate of the upper-left hand corner of the block.
  _level: The log base 2 of the block size.
  _area:   The area of the block in pixels^2.
  _sumx:   The sum of the pixel values over the block.
  _sumx2:  The sum of the pixel values squared over the block.
  _var:    The variance of the block.
  _plane:  The image plane the block is contained in.*/
static void oc_seg_splitmerge_it(oc_seg_ctx *_seg,
 int _x0,int _y0,int _level,int _area,float _sumx,float _sumx2,float _var,
 const oc_img_plane *_plane){
  float var[4];
  float sumx[4];
  float sumx2[4];
  float bic;
  int   tdx[4];
  int   tdy[4];
  int   area;
  int   tsize;
  int   tx;
  int   ty;
  int   i;
  _level--;
  area=1<<(_level<<1);
  tx=_x0>>OC_SEG_MOMENT_RES_LOG;
  ty=_y0>>OC_SEG_MOMENT_RES_LOG;
  tsize=1<<_level-OC_SEG_MOMENT_RES_LOG;
  tdx[0]=tdx[2]=tdy[0]=tdy[1]=0;
  tdx[1]=tdx[3]=tdy[2]=tdy[3]=tsize;
  bic=_var>0?0.5F*_area*(float)log(_var):0;
  for(i=0;i<4;i++){
    sumx[i]=oc_seg_sum_region(_seg->sumx,tx+tdx[i],ty+tdy[i],tsize,tsize);
    sumx2[i]=oc_seg_sum_region(_seg->sumx2,tx+tdx[i],ty+tdy[i],tsize,tsize);
    var[i]=(sumx2[i]-sumx[i]*sumx[i]/area)/area;
    if(var[i]>0)bic-=0.5F*area*(float)log(var[i]);
  }
  bic-=3*(float)log(_area);
  if(bic>=0){
    int dx[4];
    int dy[4];
    int size_2;
    size_2=1<<_level;
    dx[0]=dx[2]=dy[0]=dy[1]=0;
    dx[1]=dx[3]=dy[2]=dy[3]=size_2;
    if(_level>OC_SEG_MOMENT_RES_LOG)for(i=0;i<4;i++){
      oc_seg_splitmerge_it(_seg,_x0+dx[i],_y0+dy[i],_level,area,
       sumx[i],sumx2[i],var[i],_plane);
    }
    else for(i=0;i<4;i++){
      oc_seg_splitmerge_is(_seg,_x0+dx[i],_y0+dy[i],_level,area,
       sumx[i],sumx2[i],var[i],_plane);
    }
  }
  else{
    int size;
    _level++;
    size=1<<_level;
    oc_seg_merge_t(_seg,_x0,_y0,size,size,_area,_sumx,_sumx2,_var);
  }
}

/*Recursive split-merge step.
  Version for border regions with reduced resolution table lookups.
  _seg:    The segmentation context.
  _x0:     The X coordinate of the upper-left hand corner of the block.
  _y0:     The Y coordinate of the upper-left hand corner of the block.
  _width:  The width of the block in pixels.
  _height: The height of the block in pixels.
  _level:  The log base 2 of the block size.
  _area:   The area of the block in pixels^2.
  _sumx:   The sum of the pixel values over the block.
  _sumx2:  The sum of the pixel values squared over the block.
  _var:    The variance of the block.
  _plane:  The image plane the block is contained in.*/
static void oc_seg_splitmerge_bt(oc_seg_ctx *_seg,
 int _x0,int _y0,int _width,int _height,int _level,
 int _area,float _sumx,float _sumx2,float _var,const oc_img_plane *_plane){
  float var[4];
  float sumx[4];
  float sumx2[4];
  float bic;
  float log_area;
  int   dx[4];
  int   dy[4];
  int   tdx[4];
  int   tdy[4];
  int   area[4];
  int   width[4];
  int   height[4];
  int   size;
  int   size_2;
  int   b;
  int   tx;
  int   ty;
  int   i;
  size=1<<_level;
  _level--;
  size_2=1<<_level;
  dx[0]=dx[2]=dy[0]=dy[1]=0;
  dx[1]=dx[3]=dy[2]=dy[3]=size_2;
  tx=_x0>>OC_SEG_MOMENT_RES_LOG;
  ty=_y0>>OC_SEG_MOMENT_RES_LOG;
  tdx[0]=tdx[2]=tdy[0]=tdy[1]=0;
  tdx[1]=tdx[3]=tdy[2]=tdy[3]=1<<_level-OC_SEG_MOMENT_RES_LOG;
  b=(size>_width)|((size>_height)<<1)|
   ((size_2>_width)<<2)|((size_2>_height)<<3)|
   ((size_2==_width)<<4)|((size_2==_height)<<5);
  log_area=(float)log(_area);
  bic=_var>0?0.5F*_area*(float)log(_var):0;
  bic+=log_area;
  for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
    int twidth;
    int theight;
    width[i]=(b&OC_SEG_BORDER_XMASKS[i])?_width-dx[i]:size_2;
    height[i]=(b&OC_SEG_BORDER_YMASKS[i])?_height-dy[i]:size_2;
    area[i]=width[i]*height[i];
    twidth=width[i]+OC_SEG_MOMENT_RES-1>>OC_SEG_MOMENT_RES_LOG;
    theight=height[i]+OC_SEG_MOMENT_RES-1>>OC_SEG_MOMENT_RES_LOG;
    sumx[i]=oc_seg_sum_region(_seg->sumx,tx+tdx[i],ty+tdy[i],twidth,theight);
    sumx2[i]=oc_seg_sum_region(_seg->sumx2,tx+tdx[i],ty+tdy[i],twidth,theight);
    var[i]=(sumx2[i]-sumx[i]*sumx[i]/area[i])/area[i];
    if(var[i]>0)bic-=0.5F*area[i]*(float)log(var[i]);
    bic-=log_area;
  }
  if(bic>=0){
    if(_level>OC_SEG_MOMENT_RES_LOG){
      for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
        if(b&OC_SEG_BORDER_MASKS[i]){
          oc_seg_splitmerge_bt(_seg,_x0+dx[i],_y0+dy[i],width[i],height[i],
           _level,area[i],sumx[i],sumx2[i],var[i],_plane);
        }
        else{
          oc_seg_splitmerge_it(_seg,_x0+dx[i],_y0+dy[i],
           _level,area[i],sumx[i],sumx2[i],var[i],_plane);
        }
      }
    }
    else{
      for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
        if(b&OC_SEG_BORDER_MASKS[i]){
          oc_seg_splitmerge_bs(_seg,_x0+dx[i],_y0+dy[i],width[i],height[i],
           _level,area[i],sumx[i],sumx2[i],var[i],_plane);
        }
        else{
          oc_seg_splitmerge_is(_seg,_x0+dx[i],_y0+dy[i],
           _level,area[i],sumx[i],sumx2[i],var[i],_plane);
        }
      }
    }
  }
  else oc_seg_merge_t(_seg,_x0,_y0,_width,_height,_area,_sumx,_sumx2,_var);
}


/*Initialize a segmentation context.
  This pre-allocates all memory needed by the segmentation process.
  _seg:    The segmentation context.
  _width:  The width of the image to segment, in pixels.
  _height: The height of the image to segment, in pixels.*/
static void oc_seg_init(oc_seg_ctx *_seg,int _width,int _height){
  int size;
  int level_max;
  int twidth;
  int theight;
  int cregions;
  int cpairs;
  int x;
  int y;
  /*Find the maximum recursion depth.*/
  for(level_max=0,size=1;size<_width||size<_height;size<<=1)level_max++;
  _seg->level_max=level_max;
  /*Allocate space for the moment tables.*/
  twidth=(_width+OC_SEG_MOMENT_RES-1>>OC_SEG_MOMENT_RES_LOG)+1;
  theight=(_height+OC_SEG_MOMENT_RES-1>>OC_SEG_MOMENT_RES_LOG)+1;
  _seg->sumx=(float **)oc_alloc_2d(theight,twidth,sizeof(_seg->sumx[0][0]));
  _seg->sumx2=(float **)oc_alloc_2d(theight,twidth,sizeof(_seg->sumx2[0][0]));
  _seg->local_sumx=(float **)oc_alloc_2d(OC_SEG_MOMENT_RES+1,
   OC_SEG_MOMENT_RES+1,sizeof(_seg->local_sumx[0][0]));
  _seg->local_sumx2=(float **)oc_alloc_2d(OC_SEG_MOMENT_RES+1,
   OC_SEG_MOMENT_RES+1,sizeof(_seg->local_sumx2[0][0]));
  /*Zero out the initial row and column of each moment table.*/
  for(x=0;x<twidth;x++)_seg->sumx[0][x]=_seg->sumx2[0][x]=0;
  for(y=1;y<theight;y++)_seg->sumx[y][0]=_seg->sumx2[y][0]=0;
  for(x=0;x<=OC_SEG_MOMENT_RES;x++)_seg->local_sumx[0][x]=_seg->local_sumx2[0][x]=0;
  for(y=1;y<=OC_SEG_MOMENT_RES;y++)_seg->local_sumx[y][0]=_seg->local_sumx2[y][0]=0;
  _seg->labels=(int **)oc_alloc_2d(_height,_width,sizeof(_seg->labels[0][0]));
  /*Compute the maximum number of global regions in the segmentation:*/
  cregions=(_width*_height+OC_SEG_REGION_SZ_MIN-1)/OC_SEG_REGION_SZ_MIN;
  /*Compute the maximum number of pairs.
    The maximum number of edges in a planar graph is 3n-6, where n is the
     number of vertices and n>=3 (from Euler's formula).*/
  cpairs=3*cregions-6+OC_SEG_NLOCAL_PAIRS;
  if(cpairs<2)cpairs=2;
  /*Allocate space for the region information and the neighbor lists.*/
  _seg->nregions=0;
  _seg->cregions=cregions;
  _seg->regions=(oc_seg_region *)_ogg_malloc(
   (OC_SEG_NLOCAL_REGS+1+cregions)*sizeof(_seg->regions[0]));
  /*Local regions have negative labels.
    However, to avoid lots of special casing when retreiving the region data
     for a given label, we store them all in a single table.
    Thus, we offset the regions pointer so that adding the region label always
     points to a valid entry in the table.
    The extra 1 entry is there to account for the special label -1, which
     indicates no region.*/
  _seg->regions+=OC_SEG_NLOCAL_REGS+1;
  _seg->npairs=0;
  _seg->nheap=0;
  _seg->pairs=(oc_seg_pair *)_ogg_malloc(cpairs*sizeof(_seg->pairs[0]));
  _seg->free_pairs=NULL;
  _seg->heap=(oc_seg_pair **)_ogg_malloc(cpairs*sizeof(_seg->heap[0]));
  _seg->reheap=oc_seg_pair_reheap;
}

/*Frees all memory used by the given segmentation context.
  _seg: The segmentation context to clear.*/
static void oc_seg_clear(oc_seg_ctx *_seg){
  oc_free_2d(_seg->sumx);
  oc_free_2d(_seg->sumx2);
  oc_free_2d(_seg->local_sumx);
  oc_free_2d(_seg->local_sumx2);
  oc_free_2d(_seg->labels);
  _ogg_free(_seg->regions-OC_SEG_NLOCAL_REGS-1);
  _ogg_free(_seg->pairs);
  _ogg_free(_seg->heap);
}


/*Perform segmentation over the given image plane.
  _seg:   The segmentation context.
  _plane: The image plane to segment.*/
static void oc_seg_plane(oc_seg_ctx *_seg,const oc_img_plane *_plane){
  float sumx;
  float sumx2;
  float var;
  int   area;
  int   twidth;
  int   theight;
  int   regi;
  /*Build the moment tables.*/
  oc_seg_sum_moments(_seg,_plane);
  /*Perform the splitting and merging to produce the segmentation.*/
  _seg->nregions=0;
  area=_plane->width*_plane->height;
  if(_seg->level_max>OC_SEG_MOMENT_RES_LOG){
    twidth=_plane->width+OC_SEG_MOMENT_RES-1>>OC_SEG_MOMENT_RES_LOG;
    theight=_plane->height+OC_SEG_MOMENT_RES-1>>OC_SEG_MOMENT_RES_LOG;
  }
  else twidth=theight=1;
  sumx=oc_seg_sum_region(_seg->sumx,0,0,twidth,theight);
  sumx2=oc_seg_sum_region(_seg->sumx2,0,0,twidth,theight);
  var=(sumx2-sumx*sumx/area)/area;
  if(_seg->level_max>OC_SEG_MOMENT_RES_LOG){
    oc_seg_splitmerge_bt(_seg,0,0,_plane->width,_plane->height,
     _seg->level_max,area,sumx,sumx2,var,_plane);
  }
  else{
    oc_seg_splitmerge_bs(_seg,0,0,_plane->width,_plane->height,
     _seg->level_max,area,sumx,sumx2,var,_plane);
  }
  /*Make sure all the region labels are up to date.*/
  for(regi=0;regi<_seg->nregions;regi++)oc_seg_get_label(_seg,regi);
}



/*Scans a newly created segmentation to enumerate neighbors, count border
   pixels, count edge pixels, and count pixels near the center of the image.
  _impmap: The importance map context.
  _width:  The width of the image.
  _height: The height of the image.*/
static void oc_impmap_scan(oc_impmap_ctx *_impmap,int _width,int _height){
  oc_seg_region  *sregions;
  int           **labels;
  int             width_2;
  int             height_2;
  int             xend;
  int             yend;
  int             region;
  int             neighbor;
  int             x;
  int             y;
  sregions=_impmap->seg.regions;
  labels=_impmap->seg.labels;
  /*Clear the region data.*/
  memset(_impmap->regions,0,
   sizeof(_impmap->regions[0])*_impmap->seg.nregions);
  /*First: handle the corner(s).*/
  region=sregions[labels[0][0]].link.regi;
  _impmap->regions[region].nborder++;
  _impmap->regions[region].nedge++;
  if(_width>1){
    region=sregions[labels[0][_width-1]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  if(_height>1){
    region=sregions[labels[_height-1][0]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  if(_width>1&&_height>1){
    region=sregions[labels[_height-1][_width-1]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  /*Second: Handle the interior edges.*/
  for(x=1;x<_width-2;x++){
    region=sregions[labels[0][x]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  for(y=1;y<_height-2;y++){
    region=sregions[labels[y][0]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  if(_height>1)for(x=1;x<_width-2;x++){
    region=sregions[labels[_height-1][x]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  if(_width>1)for(y=1;y<_height-2;y++){
    region=sregions[labels[y][_width-1]].link.regi;
    _impmap->regions[region].nborder++;
    _impmap->regions[region].nedge++;
  }
  /*Third: handle the interior.*/
  for(y=1;y<_height-2;y++)for(x=1;x<_width-2;x++){
    int border;
    region=sregions[labels[y][x]].link.regi;
    neighbor=sregions[labels[y][x-1]].link.regi;
    border=region^neighbor;
    neighbor=sregions[labels[y][x+1]].link.regi;
    border|=region^neighbor;
    neighbor=sregions[labels[y-1][x]].link.regi;
    border|=region^neighbor;
    neighbor=sregions[labels[y+1][x]].link.regi;
    border|=region^neighbor;
    _impmap->regions[region].nborder+=!!border;
  }
  /*Fourth: Handle the central 25% of the image.*/
  width_2=_width>>1;
  height_2=_height>>1;
  xend=(width_2>>1)+width_2;
  yend=(height_2>>1)+height_2;
  for(y=height_2>>1;y<yend;y++)for(x=width_2>>1;x<xend;x++){
    _impmap->regions[sregions[labels[y][x]].link.regi].ncenter++;
  }
}

/*Computes the importance of a given motion vector.
  _dx:         The horizontal motion, in half-pixels/frame.
  _dy:         The vertical motion, in half-pixels/frame.
  _mot_limits: The boundaries of each of the three piecewise sections of the
                importance function, pre-adjusted to half-pixels/frame.*/
static float oc_impmap_mot_weight(int _dx,int _dy,float _mot_limits[4]){
  float mot;
  /*This computation is perhaps too simple: In reality, it is motion which
     deviates from the average (3D) motion that attracts attention.
    Consider a camera that is moving straight forward.
    The motion with the largest magnitudes is on the edge of the image, but
     the most likely center of attention is the vanishing point in the center
     of the image.
    All of the motion in the image can be explained by one global motion of
     the camera, and so none of it in particular draws attention.
    However, the Theora codec does not currently have a form of global motion
     compensation, and computing it just for this measurement is awfully
     expensive.*/
  mot=(float)sqrt(_dx*_dx+_dy*_dy);
  if(mot<_mot_limits[0])return 0;
  else if(mot<_mot_limits[1]){
    return (mot-_mot_limits[0])/(_mot_limits[1]-_mot_limits[0]);
  }
  else if(mot<_mot_limits[2])return 1;
  else if(mot<_mot_limits[3]){
    return (_mot_limits[3]-mot)/(_mot_limits[3]-_mot_limits[2]);
  }
  else return 0;
}


oc_impmap_ctx *oc_impmap_alloc(oc_enc_ctx *_enc){
  oc_impmap_ctx *impmap;
  int            edge_sz;
  int            width;
  int            height;
  width=_enc->state.info.frame_width;
  height=_enc->state.info.frame_height;
  impmap=(oc_impmap_ctx *)_ogg_malloc(sizeof(*impmap));
  oc_seg_init(&impmap->seg,width,height);
  impmap->inv_region_sz_max=100.0F/(width*height);
  edge_sz=width>1?height>1?(width-2<<1)+(height<<1):width:height;
  impmap->inv_edge_sz_max=2.0F/edge_sz;
  /*Allocate space for the region stats and neighbor links.*/
  impmap->regions=(oc_impmap_region *)_ogg_malloc(
   impmap->seg.cregions*sizeof(impmap->regions[0]));
  return impmap;
}

void oc_impmap_free(oc_impmap_ctx *_impmap){
  if(_impmap!=NULL){
    oc_seg_clear(&_impmap->seg);
    _ogg_free(_impmap->regions);
    _ogg_free(_impmap);
  }
}

void oc_impmap_fill(oc_impmap_ctx *_impmap,const oc_yuv_buffer _img,
 float _duration){
  float weight_mins[OC_IMPMAP_NSEG_RESCALED_WEIGHTS];
  float weight_maxs[OC_IMPMAP_NSEG_RESCALED_WEIGHTS];
  float weight_scales[OC_IMPMAP_NSEG_RESCALED_WEIGHTS];
  float imp_max;
  float imp_scale;
  float mot_limits[4];
  oc_seg_region    *sreg;
  oc_impmap_region *ireg;
  int               regi;
  int               i;
  /*Segment the Y plane.*/
  oc_seg_plane(&_impmap->seg,&_img[0]);
  /*Gather statistics over the segmentation.*/
  oc_impmap_scan(_impmap,_img[0].width,_img[0].height);
  /*Compute the importance values for each region:*/
  for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
    weight_mins[i]=FLT_MAX;
    weight_maxs[i]=-FLT_MAX;
  }
  /*Step 1: Compute several weighting factors for each region.*/
  for(regi=0;regi<_impmap->seg.nregions;regi++){
    oc_seg_link *link;
    float        nsumx;
    int          narea;
    ireg=_impmap->regions+regi;
    sreg=_impmap->seg.regions+regi;
    /*Contrast weight: measure contrast with neighboring regions.*/
    nsumx=0;
    narea=0;
    for(link=sreg->link.next;link->pair!=NULL;link=link->next){
      nsumx+=_impmap->seg.regions[link->regi].sumx;
      narea+=_impmap->seg.regions[link->regi].npixels;
    }
    ireg->weights[OC_IMPMAP_WEIGHT_CONTRAST]=sreg->sumx/sreg->npixels;
    if(narea>0)ireg->weights[OC_IMPMAP_WEIGHT_CONTRAST]-=nsumx/narea;
    /*Size weight: measure the size of the region.*/
    ireg->weights[OC_IMPMAP_WEIGHT_SIZE]=
     sreg->npixels*_impmap->inv_region_sz_max;
    if(ireg->weights[OC_IMPMAP_WEIGHT_SIZE]>1){
      ireg->weights[OC_IMPMAP_WEIGHT_SIZE]=1;
    }
    /*Shape weight: measure unusual (long/thin) shapes.*/
    ireg->weights[OC_IMPMAP_WEIGHT_SHAPE]=
     (float)pow(ireg->nborder,1.75)/sreg->npixels;
    /*Location weight: weight regions in the image center.*/
    ireg->weights[OC_IMPMAP_WEIGHT_LOCATION]=
     ireg->ncenter/(float)sreg->npixels;
    /*Foreground weight: weight regions in the image foreground.
      As per Osberger, this is defined as regions which do not intersect the
       border of the image.*/
    ireg->weights[OC_IMPMAP_WEIGHT_FOREGROUND]=
     1-ireg->nedge*_impmap->inv_edge_sz_max;
    if(ireg->weights[OC_IMPMAP_WEIGHT_FOREGROUND]<0){
      ireg->weights[OC_IMPMAP_WEIGHT_FOREGROUND]=0;
    }
    for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
      if(ireg->weights[i]>weight_maxs[i])weight_maxs[i]=ireg->weights[i];
      if(ireg->weights[i]<weight_mins[i])weight_mins[i]=ireg->weights[i];
    }
  }
  /*Rescale the weighting factors into the range [0,1] as necessary.*/
  for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
    if(weight_maxs[i]>weight_mins[i]){
      weight_scales[i]=1/(weight_maxs[i]-weight_mins[i]);
    }
    else weight_scales[i]=1;
  }
  /*Step 2: Combine the weighting factors into a single importance value.*/
  imp_max=0;
  for(regi=0;regi<_impmap->seg.nregions;regi++){
    ireg=_impmap->regions+regi;
    for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
      ireg->weights[i]=(ireg->weights[i]-weight_mins[i])*weight_scales[i];
    }
    ireg->importance=0;
    for(i=0;i<OC_IMPMAP_NSEG_WEIGHTS;i++){
      ireg->importance+=ireg->weights[i]*ireg->weights[i];
    }
    if(ireg->importance>imp_max)imp_max=ireg->importance;
  }
  /*Rescale the regions' importances into the range [0,1].*/
  if(imp_max>0){
    imp_scale=1/imp_max;
    for(regi=0;regi<_impmap->seg.nregions;regi++){
      _impmap->regions[regi].importance*=imp_scale;
    }
  }
  /*Rescale motion limits from deg/sec to half-pixels/frame.*/
  mot_limits[0]=2*OC_IMPMAP_MOT_MIN*OC_PIXELS_PER_DEGREE*_duration;
  mot_limits[1]=2*OC_IMPMAP_MOT_P1*OC_PIXELS_PER_DEGREE*_duration;
  mot_limits[2]=2*OC_IMPMAP_MOT_P2*OC_PIXELS_PER_DEGREE*_duration;
  mot_limits[3]=2*OC_IMPMAP_MOT_MAX*OC_PIXELS_PER_DEGREE*_duration;
}
