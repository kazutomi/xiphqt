#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"
#include "psych.h"

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
   regions with, we choose the merger that yields the largest BIC increase.

  In the code, all three steps are combined.
  As soon as a leaf node is reached in the splitting process, it is merged
   with its neighbors.
  If that fails and it is below the pixel count threshold, then it is
   immediately merged with its largest neighbor.
  This could cause slightly different results from the method described by
   Osberger, since not all neighbors nor the final size of existing neighbors
   are known when the largest is selected for merging, but in practice should
   not cause too many problems.
  The advantage of this method is that the merging decision is made as soon as
   the region is created, so there is no need for a relabeling scheme or
   complex region management.

  The variances are computed using a reduced-resolution table of sums.
  This table contains accumulated sums from the upper-left corner of the
   image sampled every OC_SEG_MOMENT_RES pixels.
  The reduced resolution is designed to reduce memory requirements, since
   otherwise they would require 8 bytes per pixel.
  With a resolution of 4 pixels, this is reduced to 1/8 byte per pixel.
  Once recursion descends below this resolution level, a local moment table is
   computed for the remaining lookups.
  Because the local moment table is small (648 bytes), it easily fits in
   cache. Thus even though this method requires scanning the input image as
   much as two times, it yields a vast reduction in the amount of memory
   outside of cache that is accessed (2.125 bytes read, 0.125 bytes written
   per pixel as compared to 10 bytes read, 8 bytes written).*/

/*The log base 2 of the resolution of the global moment table.
  TODO: should this be 4? That gives a 2.25kB table, but aligns rows with
   cache lines on some hardware (assuming the picture starts on a multiple of
   16 boundary).*/
#define OC_SEG_MOMENT_RES_LOG           (3)
/*The resolution of the global moment table.*/
#define OC_SEG_MOMENT_RES               (1<<OC_SEG_MOMENT_RES_LOG)
/*A mask for the bits below the resolution of the moment table.*/
#define OC_SEG_MOMENT_RES_MASK          (OC_SEG_MOMENT_RES-1)

/*The minimum size of a segmented region.
  Imposing this limit gives us an upper bound on the number of regions that
   can be created, which allows us to allocate storage in advance.*/
#define OC_SEG_REGION_SZ_MIN            (16)



typedef struct oc_seg_neighbor oc_seg_neighbor;
typedef struct oc_seg_region   oc_seg_region;
typedef struct oc_seg_ctx      oc_seg_ctx;



/*An entry in the linked list of region neighbors.*/
struct oc_seg_neighbor{
  /*The region number of the neighbor.*/
  int              region;
  /*The next neighbor in the list.*/
  oc_seg_neighbor *next;
};

/*A segmented region of the image.
  All determinations of borders and connectedness use 4-connectivity.*/
struct oc_seg_region{
  /*The number of pixels in the region.*/
  int              npixels;
  /*The sum of the pixel values in the region.*/
  float            sumx;
  /*The sum of the squared pixel values in the region.*/
  float            sumx2;
  /*The variance of the pixel values in the region.*/
  float            var;
  /*The number of pixels on the region's border.*/
  int              nborder;
  /*The number of pixels on the region's edge.*/
  int              nedge;
  /*The number of pixels in the central 25% of the image.*/
  int              ncenter;
  /*The list of neighboring regions.*/
  oc_seg_neighbor *neighbors;
  /*The weight given to the region because of its contrast.*/
  float            weights[OC_IMPMAP_NSEG_WEIGHTS];
  /*The final importance of the region.*/
  float            importance;
};

/*Buffers and tables needed to drive the segmentation process.*/
struct oc_seg_ctx{
  /*The minimum power of 2 greater than or equal to the image width and
     height.*/
  int              level_max;
  /*The global first moment table.*/
  float          **sumx;
  /*The global second moment table.*/
  float          **sumx2;
  /*The local first moment table.*/
  float          **local_sumx;
  /*The local second moment table.*/
  float          **local_sumx2;
  /*The region labels for each pixel.*/
  int            **labels;
  /*The number of regions in the segmentation.*/
  int              nregions;
  /*The list of regions.*/
  oc_seg_region   *regions;
  /*The number of neighbors used in region lists.*/
  int              nneighbors;
  /*The storage for the neighbor lists for each region.*/
  oc_seg_neighbor *neighbors;
};

/*Importance map context information.*/
struct oc_impmap_ctx{
  /*Segmentation information for the Y plane.
    Some day we may also wish to segment the U and V planes, or derive a
     segmentation using all 3 planes at once.*/
  oc_seg_ctx seg;
  /*The reciprocal of the maximum region size to count towards a region's
     importance.
    This cap prevents very large regions from dominating the scene.*/
  float      inv_region_sz_max;
  /*The reciprocal of the maximum number of edge pixels required to consider a
     region completely in the background.
    This value is used when computing the foreground weight of a region.*/
  float      inv_edge_sz_max;
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
  _sums:  The pre-computed moment table.
  _tx:    The X coordinate in the table of the upper-left hand corner of the
           region.
  _ty:    The Y coordinate in the table of the upper-right hand corner of the
           region.
  _tsize: The width and height of the region in the table.
  Return: The sum of the appropriate value over the given region.*/
static float oc_seg_sum_region(const float **_sums,int _tx,int _ty,
 int _tsize){
  return _sums[_ty][_tx]-_sums[_ty][_tx+_tsize]-
   _sums[_ty+_tsize][_tx]+_sums[_ty+_tsize][_tx+_tsize];
}

/*The number of recent regions to keep track of when merging.*/
#define OC_SEG_MERGE_HISTORY_SZ (2)

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
static void oc_seg_merge(oc_seg_ctx *_seg,int _x0,int _y0,int _width,
 int _height,int _area,float _sumx,float _sumx2,float _var){
  oc_seg_region *reg;
  float          sumx;
  float          sumx2;
  float          var;
  float          bic;
  float          rsumx;
  float          rsumx2;
  float          rvar;
  float          rbic;
  float          bic0;
  int            lastr[OC_SEG_MERGE_HISTORY_SZ];
  int            bestr;
  int            regi;
  int            area;
  int            rarea;
  int            xend;
  int            yend;
  int            x;
  int            y;
  int            i;
  xend=_x0+_width;
  yend=_y0+_height;
  /*Step 1: Search for a region to merge with.*/
  memset(lastr,0xFF,sizeof(lastr));
  bestr=-1;
  rbic=FLT_MAX;
  rvar=rsumx=rsumx2=0;
  rarea=-1;
  bic0=0.5F*_area*(float)log(_var);
  /*Scan the left edge, if present, for adjacent regions.*/
  if(_x0>0)for(y=_y0;y<yend;y++){
    regi=_seg->labels[y][_x0-1];
    for(i=0;i<OC_SEG_MERGE_HISTORY_SZ&&lastr[i]!=regi;i++);
    if(i<OC_SEG_MERGE_HISTORY_SZ){
      memmove(lastr+1,lastr,sizeof(lastr[0])*i);
      lastr[0]=regi;
      continue;
    }
    reg=_seg->regions+regi;
    sumx=reg->sumx+_sumx;
    sumx2=reg->sumx2+_sumx2;
    area=reg->npixels+_area;
    var=(sumx2-sumx*sumx/area)/area;
    bic=0.5F*(area*(float)log(var)-reg->npixels*(float)log(reg->var))-
     (float)log(area);
    if(bic<rbic){
      bestr=regi;
      rbic=bic;
      rvar=var;
      rsumx=sumx;
      rsumx2=sumx2;
      rarea=area;
    }
    lastr[OC_SEG_MERGE_HISTORY_SZ-1]=regi;
  }
  /*Scan the top edge, if present, for adjacent regions.*/
  if(_y0>0)for(x=_x0;x<xend;x++){
    regi=_seg->labels[_y0-1][x];
    for(i=0;i<OC_SEG_MERGE_HISTORY_SZ&&lastr[i]!=regi;i++);
    if(i<OC_SEG_MERGE_HISTORY_SZ){
      memmove(lastr+1,lastr,sizeof(lastr[0])*i);
      lastr[0]=regi;
      continue;
    }
    reg=_seg->regions+regi;
    sumx=reg->sumx+_sumx;
    sumx2=reg->sumx2+_sumx2;
    area=reg->npixels+_area;
    var=(sumx2-sumx*sumx/area)/area;
    bic=0.5F*(area*(float)log(var)-reg->npixels*(float)log(reg->var))-
     (float)log(area);
    if(bic<rbic){
      bestr=regi;
      rbic=bic;
      rvar=var;
      rsumx=sumx;
      rsumx2=sumx2;
      rarea=area;
    }
    lastr[OC_SEG_MERGE_HISTORY_SZ-1]=regi;
  }
  /*If we found a region which was suitable for merging, merge with it.*/
  if(rbic<=bic0||bestr>=0&&_area<OC_SEG_REGION_SZ_MIN){
    regi=bestr;
    reg=_seg->regions+regi;
    reg->var=rvar;
    reg->sumx=rsumx;
    reg->sumx2=rsumx2;
    reg->npixels=rarea;
  }
  /*Otherwise create a new region.*/
  else{
    regi=_seg->nregions++;
    reg=_seg->regions+regi;
    reg->sumx=_sumx;
    reg->sumx2=_sumx2;
    reg->var=_var;
    reg->npixels=_area;
    reg->nborder=0;
    reg->nedge=0;
    reg->ncenter=0;
    reg->neighbors=NULL;
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
    for(i=0;i<4;i++){
      sumx[i]=oc_seg_sum_region(_seg->local_sumx,_dx+dx[i],_dy+dy[i],size_2);
      sumx2[i]=oc_seg_sum_region(_seg->local_sumx2,_dx+dx[i],_dy+dy[i],
       size_2);
      var[i]=(sumx2[i]*area-sumx[i]*sumx[i])/(area*area);
    }
    bic=0.5F*(_area*(float)log(_var)-area*((float)log(var[0])+
     (float)log(var[1])+(float)log(var[2])+(float)log(var[3])))-
     3*(float)log(_area);
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
  oc_seg_merge(_seg,_x0+_dx,_y0+_dy,size,size,_area,_sumx,_sumx2,_var);
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
    bic=0.5F*_area*(float)log(_var)+log_area;
    for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
      width[i]=(b&OC_SEG_BORDER_XMASKS[i])?_width-dx[i]:size;
      height[i]=(b&OC_SEG_BORDER_YMASKS[i])?_height-dy[i]:size;
      area[i]=width[i]*height[i];
      sumx[i]=oc_seg_sum_region(_seg->local_sumx,_dx+dx[i],_dy+dy[i],size);
      sumx2[i]=oc_seg_sum_region(_seg->local_sumx2,_dx+dx[i],_dy+dy[i],size);
      var[i]=(sumx2[i]*area[i]-sumx[i]*sumx[i])/(area[i]*area[i]);
      bic-=0.5F*area[i]*(float)log(var[i])+log_area;
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
  oc_seg_merge(_seg,_x0+_dx,_y0+_dy,_width,_height,_area,_sumx,_sumx2,_var);
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
  oc_seg_splitmerge_il(_seg,_x0,_y0,0,0,_level,_area,_sumx,_sumx2,_var);
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
  oc_seg_splitmerge_bl(_seg,_x0,_y0,0,0,_width,_height,_level,
   _area,_sumx,_sumx2,_var);
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
  for(i=0;i<4;i++){
    sumx[i]=oc_seg_sum_region(_seg->sumx,tx+tdx[i],ty+tdy[i],tsize);
    sumx2[i]=oc_seg_sum_region(_seg->sumx2,tx+tdx[i],ty+tdy[i],tsize);
    var[i]=(sumx2[i]-sumx[i]*sumx[i]/area)/area;
  }
  bic=0.5F*(_area*(float)log(_var)-area*((float)log(var[0])+
   (float)log(var[1])+(float)log(var[2])+(float)log(var[3])))-
   3*(float)log(_area);
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
    oc_seg_merge(_seg,_x0,_y0,size,size,_area,_sumx,_sumx2,_var);
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
  int   tsize;
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
  tsize=1<<_level-OC_SEG_MOMENT_RES_LOG;
  tdx[0]=tdx[2]=tdy[0]=tdy[1]=0;
  tdx[1]=tdx[3]=tdy[2]=tdy[3]=tsize;
  b=(size>_width)|((size>_height)<<1)|
   ((size_2>_width)<<2)|((size_2>_height)<<3)|
   ((size_2==_width)<<4)|((size_2==_height)<<5);
  log_area=(float)log(_area);
  bic=0.5F*_area*(float)log(_var)+log_area;
  for(i=0;i<4;i++)if(!(b&OC_SEG_CLIP_MASKS[i])){
    width[i]=(b&OC_SEG_BORDER_XMASKS[i])?_width-dx[i]:size;
    height[i]=(b&OC_SEG_BORDER_YMASKS[i])?_height-dy[i]:size;
    area[i]=width[i]*height[i];
    sumx[i]=oc_seg_sum_region(_seg->sumx,tx+tdx[i],ty+tdy[i],tsize);
    sumx2[i]=oc_seg_sum_region(_seg->sumx2,tx+tdx[i],ty+tdy[i],tsize);
    var[i]=(sumx2[i]-sumx[i]*sumx[i]/area[i])/area[i];
    bic-=0.5F*area[i]*(float)log(var[i])+log_area;
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
  else oc_seg_merge(_seg,_x0,_y0,_width,_height,_area,_sumx,_sumx2,_var);
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
  int nregions;
  int nneighbors;
  for(level_max=0,size=1;size<_width||size<_height;size<<=1)level_max++;
  _seg->level_max=level_max;
  twidth=(_width>>OC_SEG_MOMENT_RES_LOG)+1;
  theight=(_height>>OC_SEG_MOMENT_RES_LOG)+1;
  _seg->sumx=(float **)oc_alloc_2d(theight,twidth,sizeof(_seg->sumx[0][0]));
  _seg->sumx2=(float **)oc_alloc_2d(theight,twidth,sizeof(_seg->sumx2[0][0]));
  _seg->local_sumx=(float **)oc_alloc_2d(OC_SEG_MOMENT_RES+1,
   OC_SEG_MOMENT_RES+1,sizeof(_seg->local_sumx[0][0]));
  _seg->local_sumx2=(float **)oc_alloc_2d(OC_SEG_MOMENT_RES+1,
   OC_SEG_MOMENT_RES+1,sizeof(_seg->local_sumx2[0][0]));
  _seg->labels=(int **)oc_alloc_2d(_height,_width,sizeof(_seg->labels[0][0]));
  /*The maximum number of regions in the segmentation:*/
  nregions=_width*_height/OC_SEG_REGION_SZ_MIN+1;
  /*The maximum number of edges in a planar graph is 3n-6, where n is the
     number of vertices and n>=3 (from Euler's formula).
    This needs to be doubled to store both directions of an edge separately.*/
  nneighbors=6*nregions-12;
  if(nneighbors<2)nneighbors=2;
  _seg->nregions=0;
  _seg->regions=(oc_seg_region *)_ogg_malloc(
   nregions*sizeof(_seg->regions[0]));
  _seg->nneighbors=0;
  _seg->neighbors=(oc_seg_neighbor *)_ogg_malloc(
   nneighbors*sizeof(_seg->neighbors[0]));
}

/*Frees all memory used by the given segmentation context.
  _seg: The segmentation context to clear.*/
static void oc_seg_clear(oc_seg_ctx *_seg){
  oc_free_2d(_seg->sumx);
  oc_free_2d(_seg->sumx2);
  oc_free_2d(_seg->local_sumx);
  oc_free_2d(_seg->local_sumx2);
  oc_free_2d(_seg->labels);
  _ogg_free(_seg->regions);
  _ogg_free(_seg->neighbors);
}

/*Adds a neighbor to a region if it is not already there.
  _to is added to _from's neighbor list, but _from is not added to _to's list.
  This is because it is already necessary to check all directions from every
   pixel in order to accurately count the number that border at least one
   different region.
  Therefore this function will be called again when the other pixel is
   eventually checked.
  _seg:  The segmentation context.
  _from: The region to add the neighbor information to.
  _to:   The neighboring region to add.*/
static void oc_seg_add_neighbor(oc_seg_ctx *_seg,int _from,int _to){
  oc_seg_region    *rfrom;
  oc_seg_neighbor  *nneighbor;
  oc_seg_neighbor **pneighbor;
  rfrom=_seg->regions+_from;
  for(pneighbor=&rfrom->neighbors,nneighbor=rfrom->neighbors;
   nneighbor!=NULL;nneighbor=nneighbor->next){
    if(nneighbor->region==_to){
      /*If we found this neighbor in the list, move it to the head.*/
      *pneighbor=nneighbor->next;
      nneighbor->next=rfrom->neighbors;
      rfrom->neighbors=nneighbor;
      return;
    }
  }
  /*We found a new neighbor: add it to the head of the list.*/
  nneighbor=_seg->neighbors+_seg->nneighbors++;
  nneighbor->region=_to;
  nneighbor->next=rfrom->neighbors;
  rfrom->neighbors=nneighbor;
}

/*Scans a newly created segmentation to enumerate neighbors, count border
   pixels, count edge pixels, and count pixels near the center of the image.
  _seg:    The segmentation context.
  _width:  The width of the image.
  _height: The height of the image.*/
static void oc_seg_scan(oc_seg_ctx *_seg,int _width,int _height){
  int width_2;
  int height_2;
  int xend;
  int yend;
  int region;
  int neighbor;
  int x;
  int y;
  _seg->nneighbors=0;
  /*First: handle the corner(s).*/
  region=_seg->labels[0][0];
  _seg->regions[region].nborder++;
  _seg->regions[region].nedge++;
  if(_width>1){
    neighbor=_seg->labels[0][1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    region=_seg->labels[0][_width-1];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[0][_width-2];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  if(_height>1){
    region=_seg->labels[0][0];
    neighbor=_seg->labels[1][0];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    region=_seg->labels[_height-1][0];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[_height-2][0];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  if(_width>1&&_height>1){
    region=_seg->labels[0][_width-1];
    neighbor=_seg->labels[1][_width-1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    region=_seg->labels[_height-1][0];
    neighbor=_seg->labels[_height-1][1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    region=_seg->labels[_height-1][_width-1];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[_height-1][_width-2];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    neighbor=_seg->labels[_height-2][_width-1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  /*Second: Handle the interior edges.*/
  for(x=1;x<_width-2;x++){
    region=_seg->labels[0][x];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[0][x-1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    neighbor=_seg->labels[0][x+1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  for(y=1;y<_height-2;y++){
    region=_seg->labels[y][0];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[y-1][0];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    neighbor=_seg->labels[y+1][0];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  if(_height>1)for(x=1;x<_width-2;x++){
    region=_seg->labels[_height-1][x];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[_height-1][x-1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    neighbor=_seg->labels[_height-1][x+1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  if(_width>1)for(y=1;y<_height-2;y++){
    region=_seg->labels[y][_width-1];
    _seg->regions[region].nborder++;
    _seg->regions[region].nedge++;
    neighbor=_seg->labels[y-1][_width-1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
    neighbor=_seg->labels[y+1][_width-1];
    if(region!=neighbor)oc_seg_add_neighbor(_seg,region,neighbor);
  }
  /*Third: handle the interior.*/
  for(y=1;y<_height-2;y++)for(x=1;x<_width-2;x++){
    int border;
    region=_seg->labels[y][x];
    border=0;
    neighbor=_seg->labels[y][x-1];
    if(region!=neighbor){
      oc_seg_add_neighbor(_seg,region,neighbor);
      border=1;
    }
    neighbor=_seg->labels[y][x+1];
    if(region!=neighbor){
      oc_seg_add_neighbor(_seg,region,neighbor);
      border=1;
    }
    neighbor=_seg->labels[y-1][x];
    if(region!=neighbor){
      oc_seg_add_neighbor(_seg,region,neighbor);
      border=1;
    }
    neighbor=_seg->labels[y+1][x];
    if(region!=neighbor){
      oc_seg_add_neighbor(_seg,region,neighbor);
      border=1;
    }
    _seg->regions[region].nborder+=border;
  }
  /*Fourth: Handle the central 25% of the image.*/
  width_2=_width>>1;
  height_2=_height>>1;
  xend=(width_2>>1)+width_2;
  yend=(height_2>>1)+height_2;
  for(y=height_2>>1;y<yend;y++)for(x=width_2>>1;x<xend;x++){
    _seg->regions[_seg->labels[y][x]].ncenter++;
  }
}

/*Perform segmentation over the given image plane.
  _seg:   The segmentation context.
  _plane: The image plane to segment.*/
static void oc_seg_plane(oc_seg_ctx *_seg,const oc_img_plane *_plane){
  float sumx;
  float sumx2;
  float var;
  int   area;
  int   tsize;
  /*Build the moment tables.*/
  oc_seg_sum_moments(_seg,_plane);
  /*Perform the splitting and merging to produce the segmentation.*/
  _seg->nregions=0;
  area=_plane->width*_plane->height;
  if(_seg->level_max>OC_SEG_MOMENT_RES_LOG){
    tsize=1<<_seg->level_max-OC_SEG_MOMENT_RES_LOG;
  }
  else tsize=1;
  sumx=oc_seg_sum_region(_seg->sumx,0,0,tsize);
  sumx2=oc_seg_sum_region(_seg->sumx2,0,0,tsize);
  var=(sumx2-sumx*sumx/area)/area;
  if(_seg->level_max>OC_SEG_MOMENT_RES_LOG){
    oc_seg_splitmerge_bt(_seg,0,0,_plane->width,_plane->height,
     _seg->level_max,area,sumx,sumx2,var,_plane);
  }
  else{
    oc_seg_splitmerge_bs(_seg,0,0,_plane->width,_plane->height,
     _seg->level_max,area,sumx,sumx2,var,_plane);
  }
  /*Accumulate statistics over the segmented image for use in building the
     importance map.*/
  oc_seg_scan(_seg,_plane->width,_plane->height);
}

oc_impmap_ctx *oc_impmap_alloc(const oc_yuv_buffer _format){
  oc_impmap_ctx *impmap;
  int            edge_sz;
  int            width;
  int            height;
  width=_format[0].width;
  height=_format[0].height;
  impmap=(oc_impmap_ctx *)_ogg_malloc(sizeof(*impmap));
  oc_seg_init(&impmap->seg,width,height);
  impmap->inv_region_sz_max=100.0F/(width*height);
  edge_sz=width>1?height>1?(width-2<<1)+(height<<1):width:height;
  impmap->inv_edge_sz_max=2.0F/edge_sz;
  return impmap;
}

void oc_impmap_free(oc_impmap_ctx *_impmap){
  if(_impmap!=NULL){
    oc_seg_clear(&_impmap->seg);
    _ogg_free(_impmap);
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

void oc_impmap_fill(oc_impmap_ctx *_impmap,const oc_yuv_buffer _img,
 float _duration){
  float weight_mins[OC_IMPMAP_NSEG_RESCALED_WEIGHTS];
  float weight_maxs[OC_IMPMAP_NSEG_RESCALED_WEIGHTS];
  float weight_scales[OC_IMPMAP_NSEG_RESCALED_WEIGHTS];
  float imp_max;
  float imp_scale;
  float mot_limits[4];
  oc_seg_region *reg;
  int            regi;
  int            i;
  /*Segment the Y plane.*/
  oc_seg_plane(&_impmap->seg,&_img[0]);
  /*Compute the importance values for each region:*/
  for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
    weight_mins[i]=FLT_MAX;
    weight_maxs[i]=-FLT_MAX;
  }
  /*Step 1: Compute several weighting factors for each region.*/
  for(regi=0;regi<_impmap->seg.nregions;regi++){
    oc_seg_neighbor *neighbor;
    float            nsumx;
    int              narea;
    reg=_impmap->seg.regions+regi;
    /*Contrast weight: measure contrast with neighboring regions.*/
    nsumx=0;
    narea=0;
    for(neighbor=reg->neighbors;neighbor!=NULL;neighbor=neighbor->next){
      nsumx+=_impmap->seg.regions[neighbor->region].sumx;
      narea+=_impmap->seg.regions[neighbor->region].npixels;
    }
    reg->weights[OC_IMPMAP_WEIGHT_CONTRAST]=
     reg->sumx/reg->npixels-nsumx/narea;
    /*Size weight: measure the size of the region.*/
    reg->weights[OC_IMPMAP_WEIGHT_SIZE]=
     reg->npixels*_impmap->inv_region_sz_max;
    if(reg->weights[OC_IMPMAP_WEIGHT_SIZE]>1){
      reg->weights[OC_IMPMAP_WEIGHT_SIZE]=1;
    }
    /*Shape weight: measure unusual (long/thin) shapes.*/
    reg->weights[OC_IMPMAP_WEIGHT_SHAPE]=
     (float)pow(reg->nborder,1.75)/reg->npixels;
    /*Location weight: weight regions in the image center.*/
    reg->weights[OC_IMPMAP_WEIGHT_LOCATION]=
     reg->ncenter/(float)reg->npixels;
    /*Foreground weight: weight regions in the image foreground.
      As per Osberger, this is defined as regions which do not intersect the
       border of the image.*/
    reg->weights[OC_IMPMAP_WEIGHT_FOREGROUND]=
     1-reg->nedge*_impmap->inv_edge_sz_max;
    if(reg->weights[OC_IMPMAP_WEIGHT_SIZE]<0){
      reg->weights[OC_IMPMAP_WEIGHT_SIZE]=0;
    }
    for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
      if(reg->weights[i]>weight_maxs[i])weight_maxs[i]=reg->weights[i];
      if(reg->weights[i]<weight_mins[i])weight_mins[i]=reg->weights[i];
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
    reg=_impmap->seg.regions+regi;
    for(i=0;i<OC_IMPMAP_NSEG_RESCALED_WEIGHTS;i++){
      reg->weights[i]=(reg->weights[i]+weight_mins[i])*weight_scales[i];
    }
    reg->importance=0;
    for(i=0;i<OC_IMPMAP_NSEG_WEIGHTS;i++){
      reg->importance+=reg->weights[i]*reg->weights[i];
    }
    if(reg->importance>imp_max)imp_max=reg->importance;
  }
  /*Rescale the regions' importances into the range [0,1].*/
  if(imp_max>0){
    imp_scale=1/imp_max;
    for(regi=0;regi<_impmap->seg.nregions;regi++){
      _impmap->seg.regions[regi].importance*=imp_scale;
    }
  }
  /*Rescale motion limits from deg/sec to half-pixels/frame.*/
  mot_limits[0]=2*OC_IMPMAP_MOT_MIN*OC_PIXELS_PER_DEGREE*_duration;
  mot_limits[1]=2*OC_IMPMAP_MOT_P1*OC_PIXELS_PER_DEGREE*_duration;
  mot_limits[2]=2*OC_IMPMAP_MOT_P2*OC_PIXELS_PER_DEGREE*_duration;
  mot_limits[3]=2*OC_IMPMAP_MOT_MAX*OC_PIXELS_PER_DEGREE*_duration;
}
