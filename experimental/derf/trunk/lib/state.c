#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "idct.h"



/*Returns the fragment index of the top-left block in a macro block.
  This can be used to test whether or not the whole macro block is coded.
  _sb: The super block.
  _mb: The macro block number.
  Return: The index of the fragment of the upper left block in the macro
   block, or -1 if the block lies outside the coded frame.*/
static int oc_sb_mb_top_left_frag(const oc_sb *_sb,int _mb){
  /*It so happens that under the Hilbert curve ordering described below, the
     upper-left block in each macro block is at index 0, except in macro block
     3, where it is at index 2.*/
  return _sb->map[_mb][_mb&_mb<<1];
}

/*Fills in the mapping from block positions to fragment numbers for a single
   color plane.
  This function also fills in the "valid" flag of each macro block in a super
   block.
  _sbs:    The array of super blocks for the color plane.
  _frag0:  The index of the first fragment in the plane.
  _hfrags: The number of horizontal fragments in a coded frame.
  _vfrags: The number of vertical fragments in a coded frame.*/
static void oc_sb_create_plane_mapping(oc_sb _sbs[],int _frag0,int _hfrags,
 int _vfrags){
  /*Contains the (macro_block,block) indices for a 4x4 grid of
     fragments.
    The pattern is a 4x4 Hilbert space-filling curve.
    A Hilbert curve has the nice property that as the curve grows larger, its
     fractal dimension approaches 2.
    The intuition is that nearby blocks in the curve are also close spatially,
     with the previous element always an immediate neighbor, so that runs of
     blocks should be well correlated.*/
  static const int SB_MAP[4][4][2]={
    {{0,0},{0,1},{3,2},{3,3}},
    {{0,3},{0,2},{3,1},{3,0}},
    {{1,0},{1,3},{2,0},{2,3}},
    {{1,1},{1,2},{2,1},{2,2}}
  };
  oc_sb *sb;
  int    yfrag;
  int    y;
  sb=_sbs;
  yfrag=_frag0;
  for(y=0;;y+=4){
    int imax;
    int x;
    /*Figure out how many columns of blocks in this super block lie within the
       image.*/
    imax=_vfrags-y;
    if(imax>4)imax=4;
    else if(imax<=0)break;
    for(x=0;;x+=4,sb++){
      int    xfrag;
      int    jmax;
      int    mbi;
      int    i;
      /*Figure out how many rows of blocks in this super block lie within the
         image.*/
      jmax=_hfrags-x;
      if(jmax>4)jmax=4;
      else if(jmax<=0)break;
      /*By default, set all fragment indices to -1.*/
      memset(sb->map[0],0xFF,sizeof(sb->map));
      /*Fill in the fragment map for this super block.*/
      xfrag=yfrag+x;
      for(i=0;i<imax;i++){
        int j;
        for(j=0;j<jmax;j++){
          sb->map[SB_MAP[i][j][0]][SB_MAP[i][j][1]]=xfrag+j;
        }
        xfrag+=_hfrags;
      }
      /*Mark which macro blocks in this super block lie within the image.*/
      for(mbi=0;mbi<4;mbi++){
        sb->mb_valid|=(oc_sb_mb_top_left_frag(sb,mbi)>=0)<<mbi;
      }
    }
    yfrag+=_hfrags<<2;
  }
}

/*Fills in the Y plane fragment map for a macro block given the fragment
   indices of its upper-left hand corder.
  _mb:      The macro block to fill.
  _frplane: The description of the Y plane.
  _x:        The X location of the upper-left hand fragment in the Y plane.
  _y:        The Y location of the upper-left hand fragment in the Y plane.*/
static void oc_mb_fill_ymapping(oc_mb *_mb,const oc_fragment_plane *_frplane,
 int _x,int _y){
  int i;
  for(i=0;i<2;i++){
    int j;
    if(_y+i>=_frplane->nvfrags)break;
    for(j=0;j<2;j++){
      if(_x+j>=_frplane->nhfrags)break;
      _mb->map[0][i<<1|j]=(_y+i)*_frplane->nhfrags+_x+j;
    }
  }
}

/*Fills in the U,V plane fragment maps for a macro block.
  This version is for use with no chroma decimation.
  This uses the already filled-in Y plane values.
  _mb:       The macro block to fill.
  _frplanes: The descriptions of the fragment planes.*/
static void oc_mb_fill_cmapping00(oc_mb *_mb,
 const oc_fragment_plane _frplanes[3]){
  int k;
  for(k=0;k<4;k++){
    if(_mb->map[0][k]>=0){
      _mb->map[1][k]=_mb->map[0][k]+_frplanes[1].froffset;
      _mb->map[2][k]=_mb->map[0][k]+_frplanes[2].froffset;
    }
  }
}

/*Fills in the U,V plane fragment maps for a macro block.
  This version is for use with chroma decimated in the X direction.
  _mb:       The macro block to fill.
  _frplanes: The descriptions of the fragment planes.
  _x:        The X location of the upper-left hand fragment in the Y plane.
  _y:        The Y location of the upper-left hand fragment in the Y plane.*/
static void oc_mb_fill_cmapping01(oc_mb *_mb,
 const oc_fragment_plane _frplanes[3],int _x,int _y){
  int fragi;
  int i;
  _x>>=1;
  fragi=_y*_frplanes[1].nhfrags+_x;
  for(i=0;i<2;i++){
    if(_y+i>=_frplanes[1].nvfrags)break;
    _mb->map[1][i<<1]=fragi+_frplanes[1].froffset;
    _mb->map[2][i<<1]=fragi+_frplanes[2].froffset;
    fragi+=_frplanes[1].nhfrags;
  }
}

/*Fills in the U,V plane fragment maps for a macro block.
  This version is for use with chroma decimated in the Y direction.
  _mb:       The macro block to fill.
  _frplanes: The descriptions of the fragment planes.
  _x:        The X location of the upper-left hand fragment in the Y plane.
  _y:        The Y location of the upper-left hand fragment in the Y plane.*/
static void oc_mb_fill_cmapping10(oc_mb *_mb,
 const oc_fragment_plane _frplanes[3],int _x,int _y){
  int fragi;
  int j;
  _y>>=1;
  fragi=_y*_frplanes[1].nhfrags+_x;
  for(j=0;j<2;j++){
    if(_x+j>=_frplanes[1].nhfrags)break;
    _mb->map[1][j]=fragi+_frplanes[1].froffset;
    _mb->map[2][j]=fragi+_frplanes[2].froffset;
    fragi++;
  }
}

/*Fills in the U,V plane fragment maps for a macro block.
  This version is for use with chroma decimated in the X and Y directions.
  _mb:       The macro block to fill.
  _frplanes: The descriptions of the fragment planes.
  _x:        The X location of the upper-left hand fragment in the Y plane.
  _y:        The Y location of the upper-left hand fragment in the Y plane.*/
static void oc_mb_fill_cmapping11(oc_mb *_mb,
 const oc_fragment_plane _frplanes[3],int _x,int _y){
  int fragi;
  _x>>=1;
  _y>>=1;
  fragi=_y*_frplanes[1].nhfrags+_x;
  _mb->map[1][0]=fragi+_frplanes[1].froffset;
  _mb->map[2][0]=fragi+_frplanes[2].froffset;
}

/*The function type used to fill in the U,V plane fragment maps for a macro
   block.
  _mb:       The macro block to fill.
  _frplanes: The descriptions of the fragment planes.
  _x:        The X location of the upper-left hand fragment in the Y plane.
  _y:        The Y location of the upper-left hand fragment in the Y plane.*/
typedef void (*oc_mb_fill_cmapping_func)(oc_mb *_mb,
 const oc_fragment_plane _frplanes[3],int _xfrag0,int _yfrag0);

/*A table of functions used to fill in the U,V plane fragment maps for a macro
   block for each type of chrominance decimation.*/
static const oc_mb_fill_cmapping_func OC_MB_FILL_CMAPPING_TABLE[4]={
  (oc_mb_fill_cmapping_func)oc_mb_fill_cmapping00,
  oc_mb_fill_cmapping01,
  oc_mb_fill_cmapping10,
  oc_mb_fill_cmapping11
};

/*Fills in the mapping from macro blocks to their corresponding fragment
   numbers in each plane.
  _mbs:      The array of macro blocks.
  _frplanes: The descriptions of the fragment planes.
  _ctype:    The chroma decimation type.*/
static void oc_mb_create_mapping(oc_mb _mbs[],
 const oc_fragment_plane _frplanes[3],int _ctype){
  oc_mb_fill_cmapping_func  mb_fill_cmapping;
  oc_mb                    *mb0;
  int                       y;
  mb0=_mbs;
  mb_fill_cmapping=OC_MB_FILL_CMAPPING_TABLE[_ctype];
  /*Loop through the Y plane super blocks.*/
  for(y=0;y<_frplanes[0].nvfrags;y+=4){
    int x;
    for(x=0;x<_frplanes[0].nhfrags;x+=4,mb0+=4){
      int ymb;
      /*Loop through the macro blocks in each super block in display order.*/
      for(ymb=0;ymb<2;ymb++){
        int xmb;
        for(xmb=0;xmb<2;xmb++){
          oc_mb *mb;
          int    mbx;
          int    mby;
          mb=mb0+OC_MB_MAP[ymb][xmb];
          mbx=x|xmb<<1;
          mby=y|ymb<<1;
          mb->x=mbx<<3;
          mb->y=mby<<3;
          /*Initialize fragment indexes to -1.*/
          memset(mb->map,0xFF,sizeof(mb->map));
          /*Make sure this macro block is within the encoded region.*/
          if(mbx>=_frplanes[0].nhfrags||mby>=_frplanes[0].nvfrags){
            mb->mode=OC_MODE_INVALID;
            continue;
          }
          /*Fill in the fragment indices for the Y plane.*/
          oc_mb_fill_ymapping(mb,_frplanes,mbx,mby);
          /*Fill in the fragment indices for the U,V planes.*/
          (*mb_fill_cmapping)(mb,_frplanes,mbx,mby);
        }
      }
    }
  }
}

/*Marks the fragments which fall all or partially outside the displayable
   region of the frame.
  _state: The Theora state containing the fragments to be marked.*/
static void oc_state_border_init(oc_theora_state *_state){
  typedef struct{
    int x0;
    int y0;
    int xf;
    int yf;
  }oc_crop_rect;
  oc_fragment       *frag;
  oc_fragment       *yfrag_end;
  oc_fragment       *xfrag_end;
  oc_fragment_plane *fplane;
  oc_crop_rect      *crop;
  oc_crop_rect       crop_rects[3];
  int                pli;
  int                y;
  int                x;
  /*The method we use here is slow, but the code is dead simple and handles
     all the special cases easily.
    We only ever need to do it once.*/
  /*Loop through the fragments, marking those completely outside the
     displayable region and constructing a border mask for those that straddle
     the border.*/
  _state->nborders=0;
  yfrag_end=frag=_state->frags;
  for(pli=0;pli<3;pli++){
    fplane=_state->frplanes+pli;
    crop=crop_rects+pli;
    /*Set up the cropping rectangle for this plane.*/
    crop->x0=_state->info.offset_x;
    crop->xf=_state->info.offset_x+_state->info.frame_width;
    crop->y0=_state->info.offset_y;
    crop->yf=_state->info.offset_y+_state->info.frame_height;
    if(pli>0){
      if(_state->info.chroma_x_decimated){
        crop->x0=crop->x0>>1;
        crop->xf=crop->xf+1>>1;
      }
      if(_state->info.chroma_y_decimated){
        crop->y0=crop->y0>>1;
        crop->yf=crop->yf+1>>1;
      }
    }
    y=0;
    for(yfrag_end+=fplane->nfrags;frag<yfrag_end;y+=8){
      x=0;
      for(xfrag_end=frag+fplane->nhfrags;frag<xfrag_end;frag++,x+=8){
        /*First check to see if this fragment is completely outside the
           displayable region.*/
        /*Note the special checks for an empty cropping rectangle.
          This guarantees that if we count a fragment as straddling the
           border, at least one pixel in the fragment will be inside the
           displayable region.*/
        if(x+8<=crop->x0||crop->xf<=x||y+8<=crop->y0||crop->yf<=y||
         crop->x0>=crop->xf||crop->y0>=crop->yf){
          frag->invalid=1;
        }
        /*Otherwise, check to see if it overlaps the border.*/
        else if(x<crop->x0&&crop->x0<x+8||x<crop->xf&&crop->xf<x+8||
         y<crop->y0&&crop->y0<y+8||y<crop->yf&&crop->yf<y+8){
          ogg_int64_t mask;
          int         npixels;
          int         i;
          mask=npixels=0;
          for(i=0;i<8;i++){
            int j;
            for(j=0;j<8;j++){
              if(x+j>=crop->x0&&x+j<crop->xf&&y+i>=crop->y0&&y+i<crop->yf){
                mask|=(ogg_int64_t)1<<(i<<3|j);
                npixels++;
              }
            }
          }
          /*Search the fragment array for border info with the same pattern.
            In general, there will be at most 8 different patterns.*/
          for(i=0;;i++){
            if(i>=_state->nborders){
              _state->nborders++;
              _state->borders[i].mask=mask;
              _state->borders[i].npixels=npixels;
            }
            else if(_state->borders[i].mask!=mask)continue;
            frag->border=_state->borders+i;
            break;
          }
        }
      }
    }
  }
}

static void oc_state_frarray_init(oc_theora_state *_state){
  int yhfrags;
  int yvfrags;
  int chfrags;
  int cvfrags;
  int yfrags;
  int cfrags;
  int nfrags;
  int yhsbs;
  int yvsbs;
  int chsbs;
  int cvsbs;
  int ysbs;
  int csbs;
  int nsbs;
  int nmbs;
  int pli;
  /*Figure out the number of fragments in each plane.*/
  /*These parameters have already been validated to be multiples of 16.*/
  yhfrags=_state->info.width>>3;
  yvfrags=_state->info.height>>3;
  chfrags=_state->info.chroma_x_decimated?yhfrags+1>>1:yhfrags;
  cvfrags=_state->info.chroma_y_decimated?yvfrags+1>>1:yvfrags;
  yfrags=yhfrags*yvfrags;
  cfrags=chfrags*cvfrags;
  nfrags=yfrags+2*cfrags;
  /*Figure out the number of super blocks in each plane.*/
  yhsbs=yhfrags+3>>2;
  yvsbs=yvfrags+3>>2;
  chsbs=chfrags+3>>2;
  cvsbs=cvfrags+3>>2;
  ysbs=yhsbs*yvsbs;
  csbs=chsbs*cvsbs;
  nsbs=ysbs+2*csbs;
  nmbs=ysbs<<2;
  /*Initialize the fragment array.*/
  _state->frplanes[0].nhfrags=yhfrags;
  _state->frplanes[0].nvfrags=yvfrags;
  _state->frplanes[0].froffset=0;
  _state->frplanes[0].nfrags=yfrags;
  _state->frplanes[0].nhsbs=yhsbs;
  _state->frplanes[0].nvsbs=yvsbs;
  _state->frplanes[0].sboffset=0;
  _state->frplanes[0].nsbs=ysbs;
  _state->frplanes[1].nhfrags=_state->frplanes[2].nhfrags=chfrags;
  _state->frplanes[1].nvfrags=_state->frplanes[2].nvfrags=cvfrags;
  _state->frplanes[1].froffset=yfrags;
  _state->frplanes[2].froffset=yfrags+cfrags;
  _state->frplanes[1].nfrags=_state->frplanes[2].nfrags=cfrags;
  _state->frplanes[1].nhsbs=_state->frplanes[2].nhsbs=chsbs;
  _state->frplanes[1].nvsbs=_state->frplanes[2].nvsbs=cvsbs;
  _state->frplanes[1].sboffset=ysbs;
  _state->frplanes[2].sboffset=ysbs+csbs;
  _state->frplanes[1].nsbs=_state->frplanes[2].nsbs=csbs;
  _state->nfrags=nfrags;
  _state->frags=_ogg_calloc(nfrags,sizeof(oc_fragment));
  _state->nsbs=nsbs;
  _state->sbs=_ogg_calloc(nsbs,sizeof(oc_sb));
  _state->nhmbs=yhsbs<<1;
  _state->nvmbs=yvsbs<<1;
  _state->nmbs=nmbs;
  _state->mbs=_ogg_calloc(nmbs,sizeof(oc_mb));
  _state->block_coded_flags=_ogg_calloc(nfrags,
   sizeof(_state->block_coded_flags[0]));
  _state->coded_fragis=_ogg_malloc(nfrags*sizeof(_state->coded_fragis[0]));
  _state->uncoded_fragis=_state->coded_fragis+nfrags;
  _state->coded_mbis=_ogg_malloc(nmbs*sizeof(_state->coded_mbis[0]));
  /*Create the mapping from super blocks to fragments.*/
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *fplane;
    fplane=_state->frplanes+pli;
    oc_sb_create_plane_mapping(_state->sbs+fplane->sboffset,
     fplane->froffset,fplane->nhfrags,fplane->nvfrags);
  }
  /*Create the mapping from macro blocks to fragments.*/
  oc_mb_create_mapping(_state->mbs,_state->frplanes,_state->chroma_type);
  /*Initialize the invalid and border fields of each fragment.*/
  oc_state_border_init(_state);
}

static void oc_state_frarray_clear(oc_theora_state *_state){
  _ogg_free(_state->coded_mbis);
  _ogg_free(_state->coded_fragis);
  _ogg_free(_state->block_coded_flags);
  _ogg_free(_state->mbs);
  _ogg_free(_state->sbs);
  _ogg_free(_state->frags);
}


/*Initializes the buffers used for reconstructed frames.
  These buffers are padded with 16 extra pixels on each side, to allow
   unrestricted motion vectors without special casing the boundary.
  If chroma is decimated in either direction, the padding is reduced by a
   factor of 2 on the appropriate sides.
  _enc: The encoding context to store the buffers in.*/
static void oc_state_ref_bufs_init(oc_theora_state *_state){
  theora_info   *info;
  unsigned char *ref_frame_data;
  size_t         yplane_sz;
  size_t         cplane_sz;
  int            yhstride;
  int            yvstride;
  int            chstride;
  int            cvstride;
  int            yoffset;
  int            coffset;
  int            rfi;
  info=&_state->info;
  /*Compute the image buffer parameters for each plane.*/
  yhstride=info->width+2*OC_UMV_PADDING;
  yvstride=info->height+2*OC_UMV_PADDING;
  chstride=info->chroma_x_decimated?yhstride>>1:yhstride;
  cvstride=info->chroma_y_decimated?yvstride>>1:yvstride;
  yplane_sz=(size_t)yhstride*yvstride;
  cplane_sz=(size_t)chstride*cvstride;
  yoffset=OC_UMV_PADDING+OC_UMV_PADDING*yhstride;
  coffset=(info->chroma_x_decimated?OC_UMV_PADDING>>1:OC_UMV_PADDING)+
   (info->chroma_y_decimated?OC_UMV_PADDING>>1:OC_UMV_PADDING)*chstride;
  _state->ref_frame_data=ref_frame_data=_ogg_malloc(3*(yplane_sz+2*cplane_sz));
  /*Set up the width, height and stride for the image buffers.*/
  _state->ref_frame_bufs[0][0].width=info->width;
  _state->ref_frame_bufs[0][0].height=info->height;
  _state->ref_frame_bufs[0][0].stride=yhstride;
  _state->ref_frame_bufs[0][1].width=_state->ref_frame_bufs[0][2].width=
   info->chroma_x_decimated?info->width>>1:info->width;
  _state->ref_frame_bufs[0][1].height=_state->ref_frame_bufs[0][2].height=
   info->chroma_x_decimated?info->height>>1:info->height;
  _state->ref_frame_bufs[0][1].stride=_state->ref_frame_bufs[0][2].stride=
   chstride;
  memcpy(_state->ref_frame_bufs[1],_state->ref_frame_bufs[0],
   sizeof(_state->ref_frame_bufs[0]));
  memcpy(_state->ref_frame_bufs[2],_state->ref_frame_bufs[0],
   sizeof(_state->ref_frame_bufs[0]));
  /*Set up the data pointers for the image buffers.*/
  for(rfi=0;rfi<3;rfi++){
    _state->ref_frame_bufs[rfi][0].data=ref_frame_data+yoffset;
    ref_frame_data+=yplane_sz;
    _state->ref_frame_bufs[rfi][1].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    _state->ref_frame_bufs[rfi][2].data=ref_frame_data+coffset;
    ref_frame_data+=cplane_sz;
    /*Flip the buffer upside down.*/
    oc_ycbcr_buffer_flip(_state->ref_frame_bufs[rfi],
     _state->ref_frame_bufs[rfi]);
    /*Initialize the fragment pointers into this buffer.*/
    oc_state_fill_buffer_ptrs(_state,rfi,_state->ref_frame_bufs[rfi]);
  }
  /*Initialize the reference frame indexes.*/
  _state->ref_frame_idx[OC_FRAME_GOLD]=
   _state->ref_frame_idx[OC_FRAME_PREV]=
   _state->ref_frame_idx[OC_FRAME_SELF]=-1;
}

static void oc_state_ref_bufs_clear(oc_theora_state *_state){
  _ogg_free(_state->ref_frame_data);
}

int oc_state_init(oc_theora_state *_state,theora_info *_info){
  /*First validate the parameters.*/
  if(_info==NULL)return OC_FAULT;
  /*The width and height of the encoded frame must be multiples of 16.
    They must also, when divided by 16, fit into a 16-bit unsigned integer.
    The displayable frame offset coordinates must fit into an 8-bit unsigned
     integer (only because we only write 8 bits to the bitstream: why?).
    The displayable frame must fit inside the encoded frame.
    The color space must be one known by the encoder.*/
  if((_info->width&0xF)||(_info->height&0xF)||
   _info->width>=0x100000||_info->height>=0x100000||
   _info->offset_x>255||_info->offset_y>255||
   _info->offset_x+_info->frame_width>_info->width||
   _info->offset_y+_info->frame_height>_info->height||
   _info->colorspace<0||_info->colorspace>=OC_CS_NSPACES){
    return OC_EINVAL;
  }
  memset(_state,0,sizeof(_state));
  memcpy(&_state->info,_info,sizeof(*_info));
  /*Force chroma bits to 0 or 1, and make a combined index out of them.*/
  _state->info.chroma_x_decimated=!!_state->info.chroma_x_decimated;
  _state->info.chroma_y_decimated=!!_state->info.chroma_y_decimated;
  _state->chroma_type=_state->info.chroma_y_decimated<<1|
   _state->info.chroma_x_decimated;
  _state->frame_type=OC_UNKWN_FRAME;
  oc_state_frarray_init(_state);
  oc_state_ref_bufs_init(_state);
  if(_info->keyframe_frequency_force==0){
    _info->keyframe_frequency_force=0xFFFFFFFF;
  }
#if defined(OC_COMPOSITE_GRANPOS)
  _state->keyframe_granule_shift=oc_ilog(_info->keyframe_frequency_force-1);
#else
  _state->keyframe_granule_shift=0;
#endif
  _state->keyframe_num=0;
  _state->curframe_num=-1;
  return 0;
}

void oc_state_clear(oc_theora_state *_state){
  oc_state_ref_bufs_clear(_state);
  oc_state_frarray_clear(_state);
}


/*Duplicates the pixels on the border of the image plane out into the
   surrounding padding for use by unrestricted motion vectors.
  _iplane:   The image plane to fill the borders of.
  _hpadding: The amount of padding on each side in the horizontal direction.
  _vpadding: The amount of padding on each side in the vertical direction.*/
static void oc_iplane_borders_fill(theora_img_plane *_iplane,
 int _hpadding,int _vpadding){
  unsigned char *apix;
  unsigned char *bpix;
  unsigned char *epix;
  int            fullw;
  /*First extend the vertical edges.*/
  apix=_iplane->data;
  bpix=_iplane->data+_iplane->width-1;
  epix=_iplane->data+_iplane->height*_iplane->stride;
  /*Note the use of != instead of <, which allows stride to be negative.*/
  while(apix!=epix){
    memset(apix-_hpadding,apix[0],_hpadding);
    memset(bpix+1,bpix[0],_hpadding);
    apix+=_iplane->stride;
    bpix+=_iplane->stride;
  }
  /*Next extend the horizontal edges.*/
  fullw=_iplane->width+(_hpadding<<1);
  apix=_iplane->data-_hpadding;
  bpix=_iplane->data+(_iplane->height-1)*_iplane->stride;
  epix=apix-_iplane->stride*_vpadding;
  while(apix!=epix){
    memcpy(apix-_iplane->stride,apix,fullw);
    memcpy(bpix+_iplane->stride,bpix,fullw);
    apix-=_iplane->stride;
    bpix+=_iplane->stride;
  }
}

/*Duplicates the pixels on the border of the given reference image out into
   the surrounding padding for use by unrestricted motion vectors.
  _state: The context containing the reference buffers.
  _refi:  The index of the reference buffer to pad.*/
void oc_state_borders_fill(oc_theora_state *_state,int _refi){
  static const int PL_PADDING[3][2]={
    {OC_UMV_PADDING,OC_UMV_PADDING},
    {OC_UMV_PADDING,OC_UMV_PADDING/2},
    {OC_UMV_PADDING,OC_UMV_PADDING/2}
  };
  int pli;
  for(pli=0;pli<3;pli++){
    oc_iplane_borders_fill(&_state->ref_frame_bufs[_refi][pli],
     PL_PADDING[pli][_state->info.chroma_x_decimated],
     PL_PADDING[pli][_state->info.chroma_y_decimated]);
  }
}

/*Sets the buffer pointer in each fragment to point to the portion of the
   image buffer which it corresponds to.
  _state:   The Theora state to fill.
  _buf_idx: The index of the buffer pointer to fill.
            The first three correspond to our reconstructed frame buffers,
             while the last corresponds to the input image.
  _img:     The image buffer to fill the fragments with.*/
void oc_state_fill_buffer_ptrs(oc_theora_state *_state,int _buf_idx,
 theora_ycbcr_buffer _img){
  int pli;
  /*Special handling for the input image to give us the opportunity to skip
     some updates.
    The other buffers do not change throughout the encoding process.*/
  if(_buf_idx==OC_FRAME_IO){
     if(memcmp(_state->input,_img,sizeof(theora_ycbcr_buffer))==0)return;
     memcpy(_state->input,_img,sizeof(theora_ycbcr_buffer));
  }
  for(pli=0;pli<3;pli++){
    theora_img_plane  *iplane;
    oc_fragment_plane *fplane;
    oc_fragment       *frag;
    oc_fragment       *yfrag_end;
    unsigned char     *ypix;
    iplane=&_img[pli];
    fplane=&_state->frplanes[pli];
    ypix=iplane->data;
    frag=_state->frags+fplane->froffset;
    yfrag_end=frag+fplane->nfrags;
    while(frag<yfrag_end){
      oc_fragment   *xfrag_end;
      unsigned char *xpix;
      xpix=ypix;
      for(xfrag_end=frag+fplane->nhfrags;frag<xfrag_end;frag++){
        frag->buffer[_buf_idx]=xpix;
        xpix+=8;
      }
      ypix+=iplane->stride<<3;
    }
  }
}

/*Returns the macro block index of the macro block in the given position.
  _state: The Theora state the macro block is contained in.
  _mbx:   The X coordinate of the macro block (in macro blocks, not pixels).
  _mby:   The Y coordinate of the macro block (in macro blocks, not pixels).
  Return: The index of the macro block in the given position.*/
int oc_state_mbi_for_pos(oc_theora_state *_state,int _mbx,int _mby){
  return ((_mbx&~1)<<1)+(_mby&~1)*_state->nhmbs+OC_MB_MAP[_mby&1][_mbx&1];
}

/*Determines the offsets in an image buffer to use for motion compensation.
  _state:   The Theora state the offsets are to be computed with.
  _offset0: Returns the offset for the first buffer.
  _offset1: Returns the offset for the second buffer, if the motion vector
             has non-zero fractional components.
  _dx:      The X component of the motion vector.
  _dy:      The Y component of the motion vector.
  _ystride: The Y stride in the buffer the motion vector points into.
  _pli:     The color plane index.
  Return: The number of offsets returned: 1 or 2.*/
int oc_state_get_mv_offsets(oc_theora_state *_state,int *_offset0,
 int *_offset1,int _dx,int _dy,int _ystride,int _pli){
  static const int MV_SHIFT[6]={1,2};
  static const int MV_MASK[6]={1,3};
  int offset0;
  int offset1;
  int xtype;
  int ytype;
  int xsign;
  int ysign;
  int xfrac;
  int yfrac;
  /*Here is a brief description of how Theora handles motion vectors:
    Motion vector components are specified to half-pixel accuracy in
     undecimated directions of each plane, and quarter-pixel accuracy in
     decimated directions.
    Integer parts are extracted by dividing (not shifting) by the
     appropriate amount, with truncation towards zero.
    These integer values are used to calculate the first offset.

    If either of the fractional parts are non-zero, then a second offset is
     computed.
    No third or fourth offsets are computed, even if both components have
     non-zero fractional parts.
    The second offset is computed by dividing (not shifting) by the
     appropriate amount, always rounding _away_ from zero.*/
  /*These two variables decide whether we are in half- or quarter-pixel
     precision in each component.*/
  xtype=_state->info.chroma_x_decimated&!!_pli;
  ytype=_state->info.chroma_y_decimated&!!_pli;
  /*These two variables are either 0 for a non-negative vector or all 1's for
     a negative one.*/
  xsign=_dx>>6;
  ysign=_dy>>6;
  /*These two variables are either 0 if all the fractional bits are 0 or 1 if
     any of them are non-zero.*/
  xfrac=!!(_dx&MV_MASK[xtype]);
  yfrac=!!(_dy&MV_MASK[ytype]);
  /*This branchless code is equivalent to:
  if(_dx<0){
    if(_dy<0){
      offset0=-(-_dx>>MV_SHIFT[xtype])-(-_dy>>MV_SHIFT[ytype])*_ystride;
    }
    else{
      offset0=-(-_dx>>MV_SHIFT[xtype])+(_dy>>MV_SHIFT[ytype])*_ystride;
    }
  }
  else{
    if(_dy<0){
      offset0=(_dx>>MV_SHIFT[xtype])-(-_dy>>MV_SHIFT[ytype])*_ystride;
    }
    else{
      offset0=(_dx>>MV_SHIFT[xtype])+(_dy>>MV_SHIFT[ytype])*_ystride;
    }
  }*/
  *_offset0=offset0=(_dx>>MV_SHIFT[xtype])+(xfrac&xsign)+
   ((_dy>>MV_SHIFT[ytype])+(yfrac&ysign))*_ystride;
  if(xfrac||yfrac){
    int o[2];
    /*This branchless code is equivalent to:
    if(xfrac){
      if(_dx<0)offset1=offset0-1;
      else offset1=offset0+1;
    }
    else offset1=offset0;*/
    o[0]=offset0;
    o[1]=offset0+(xsign|1);
    offset1=o[xfrac];
    /*This branchless code is equivalent to:
    if(yfrac){
      if(_dy<0)offset1-=ref_stride;
      else offset1+=ref_stride;
    }*/
    o[0]=offset1;
    o[1]=offset1+(_ystride&~ysign)-(_ystride&ysign);
    *_offset1=o[yfrac];
    return 2;
  }
  return 1;
}

void oc_state_frag_recon(oc_theora_state *_state,const oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[64],int _ncoefs,
 const ogg_uint16_t _iquant[64]){
  ogg_int16_t dct_buf[64];
  ogg_int16_t res_buf[64];
  int dst_framei;
  int dst_ystride;
  int zzi;
  int ci;
  /*Special case only having a DC component.*/
  if(_ncoefs<2){
    ogg_int16_t p;
    /*Why is the iquant product rounded in this case and no others?
      Who knows.*/
    p=(ogg_int16_t)((ogg_int32_t)_frag->dc*_iquant[0]+15>>5);
    for(ci=0;ci<64;ci++)res_buf[ci]=p;
  }
  else{
    /*First, dequantize the coefficients.*/
    dct_buf[0]=(ogg_int16_t)((ogg_int32_t)_frag->dc*_iquant[0]);
    for(zzi=1;zzi<_ncoefs;zzi++){
      int ci;
      ci=OC_FZIG_ZAG[zzi];
      dct_buf[ci]=(ogg_int16_t)((ogg_int32_t)_dct_coeffs[ci]*_iquant[ci]);
    }
    /*Then, fill in the remainder of the coefficients with 0's, and perform
       the iDCT.*/
    if(_ncoefs<10){
      for(;zzi<10;zzi++)dct_buf[OC_FZIG_ZAG[zzi]]=0;
      oc_idct8x8_10(res_buf,dct_buf);
    }
    /*else{*/
      for(;zzi<64;zzi++)dct_buf[OC_FZIG_ZAG[zzi]]=0;
      oc_idct8x8(res_buf,dct_buf);
    /*}*/
  }
  /*Fill in the target buffer.*/
  dst_framei=_state->ref_frame_idx[OC_FRAME_SELF];
  dst_ystride=_state->ref_frame_bufs[dst_framei][_pli].stride;
  /*For now stride values in all ref frames assumed to be equal.*/
  if(_frag->mbmode==OC_MODE_INTRA){
    oc_frag_recon_intra(_frag->buffer[dst_framei],dst_ystride,res_buf);
  }
  else{
    int ref_framei;
    int ref_ystride;
    int mvoffset0;
    int mvoffset1;
    ref_framei=_state->ref_frame_idx[OC_FRAME_FOR_MODE[_frag->mbmode]];
    ref_ystride=_state->ref_frame_bufs[ref_framei][_pli].stride;
    if(oc_state_get_mv_offsets(_state,&mvoffset0,&mvoffset1,_frag->mv[0],
     _frag->mv[1],ref_ystride,_pli)>1){
      oc_frag_recon_inter2(_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[ref_framei]+mvoffset0,ref_ystride,
       _frag->buffer[ref_framei]+mvoffset1,ref_ystride,res_buf);
    }
    else{
      oc_frag_recon_inter(_frag->buffer[dst_framei],dst_ystride,
       _frag->buffer[ref_framei]+mvoffset0,ref_ystride,res_buf);
    }
  }
}
