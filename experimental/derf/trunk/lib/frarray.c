#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "encint.h"


/*Writes the bit pattern for the run length of a run to the given
   oggpack_buffer.
  _opb:         The buffer to write to.
  _value:       The length of the run.
                This must be positive, and no more than the maximum value of a
                 run that can be stored with the given prefix code.
  _val_min:     The minimum value that can be coded in each interval, plus an
                 extra entry for one past the last interval.
  _code_prefix: The prefix code that is prepended to the value for
                 each interval.
  _code_bits:   The total number of bits in the bit pattern encoded for each
                 interval.*/
static void oc_frarray_pack_run(oggpack_buffer *_opb,int _value,
 const int _val_min[],const int _code_prefix[],
 const int _code_bits[]){
  int i;
  for(i=0;_value<_val_min[i+1];i++);
  oggpackB_write(_opb,_code_prefix[i]+(_value-_val_min[i]),_code_bits[i]);
}

/*Writes the bit pattern for the run length of a super block run to the given
   oggpack_buffer.
  _opb:   The buffer to write to.
  _value: The length of the run.
          This must be positive, and no more than 4129.*/
static void oc_frarray_pack_sb_run(oggpack_buffer *_opb,int _value){
  /*Coding scheme:
       Codeword            Run Length
     0                       1
     10x                     2-3
     110x                    4-5
     1110xx                  6-9
     11110xxx                10-17
     111110xxxx              18-33
     111111xxxxxxxxxxxx      34-4129*/
  static const int VAL_MIN[8]={1,2,4,6,10,18,34,4130};
  static const int CODE_PREFIX[7]={0,4,0xC,0x38,0xF0,0x3E0,0x3F000};
  static const int CODE_BITS[7]={1,3,4,6,8,10,18};
  oc_frarray_pack_run(_opb,_value,VAL_MIN,CODE_PREFIX,CODE_BITS);
}

/*Writes the bit pattern for the run length of a block run to the given
   oggpack_buffer.
  _opb:   The buffer to write to.
  _value: The length of the run.
          This must be positive, and no more than 30.*/
static void oc_frarray_pack_block_run(oggpack_buffer *_opb,int _value){
  /*Coding scheme:
     Codeword             Run Length
     0x                      1-2
     10x                     3-4
     110x                    5-6
     1110xx                  7-10
     11110xx                 11-14
     11111xxxx               15-30*/
  static const int VAL_MIN[7]={1,3,5,7,11,15,31};
  static const int CODE_PREFIX[6]={0,4,0xC,0x38,0x78,0x1F0};
  static const int CODE_BITS[6]={2,3,4,6,7,9};
  oc_frarray_pack_run(_opb,_value,VAL_MIN,CODE_PREFIX,CODE_BITS);
}

/*Writes the bit flags for whether or not each super block is partially coded
   or not.
  These flags are run-length encoded, with the flag value altenating between
   each run.
  _opb:     The oggpack_buffer to write to.
  _frarray: The fragment array containing the flags.*/
static void oc_frarray_pack_partial_sb_flags(oggpack_buffer *_opb,
 oc_fragment_array *_frarray){
  unsigned flag;
  int      run_count;
  int      sbi;
  /*Write the list of partially coded super block flags.*/
  flag=_frarray->sbs[0].coded_partially;
  oggpackB_write(_opb,flag,1);
  for(sbi=0;sbi<_frarray->nsbs;sbi++){
    for(run_count=0;sbi<_frarray->nsbs;sbi++){
      if(_frarray->sbs[sbi].coded_partially!=flag)break;
      run_count++;
    }
    /*The maximum run length we can encode is 4129.
      If we encode a run that long, we need to specify the bit value for the
       next run instead of being able to implicitly toggle it.
      Note that the original VP3 implementation did not consider this case,
       and would not write the extra bit for runs of 4129, and would write an
       invalid code for longer runs.*/
    /*First, encode runs until we have 4129 or fewer sbs left.*/
    while(run_count>4129){
      oc_frarray_pack_sb_run(_opb,4129);
      run_count-=4129;
      oggpackB_write(_opb,flag,1);
    }
    /*Encode the last run.*/
    oc_frarray_pack_sb_run(_opb,run_count);
    flag=!flag;
    /*If there are more sbs to come, and we had a run of 4129 exactly,
       encode the flipped bit.*/
    if(run_count==4129&&sbi+1<_frarray->nsbs){
      oggpackB_write(_opb,flag,1);
    }
  }
}

/*Writes the coded/not coded flags for each super block that is not partially
   coded.
  These flags are run-length encoded, with the flag value altenating between
   each run.
  _opb:     The oggpack_buffer to write to.
  _frarray: The fragment array containing the flags.*/
static void oc_frarray_pack_coded_sb_flags(oggpack_buffer *_opb,
 oc_fragment_array *_frarray){
  unsigned flag;
  int      run_count;
  int      sbi;
  /*Write the list of coded/not coded super block flags.*/
  /*Skip partially coded super blocks: their flags have already been coded.*/
  while(_frarray->sbs[sbi].coded_partially){
    sbi++;
    if(sbi>=_frarray->nsbs)return;
  }
  flag=_frarray->sbs[sbi].coded_fully;
  oggpackB_write(_opb,flag,1);
  for(sbi=0;sbi<_frarray->nsbs;sbi++){
    for(run_count=0;sbi<_frarray->nsbs;sbi++){
      if(_frarray->sbs[sbi].coded_partially)continue;
      if(_frarray->sbs[sbi].coded_fully!=flag)break;
      run_count++;
    }
    /*The maximum run length we can encode is 4129.
      If we encode a run that long, we need to specify the bit value for the
       next run instead of being able to implicitly toggle it.
      Note that the original VP3 implementation did not consider this case,
       and would not write the extra bit for runs of 4129, and would write an
       invalid code for longer runs.*/
    /*First, encode runs until we have 4129 or fewer sbs left.*/
    while(run_count>4129){
      oc_frarray_pack_sb_run(_opb,4129);
      run_count-=4129;
      oggpackB_write(_opb,flag,1);
    }
    /*Encode the last run.*/
    oc_frarray_pack_sb_run(_opb,run_count);
    flag=!flag;
    if(run_count==4129&&sbi+1<_frarray->nsbs){
      oggpackB_write(_opb,flag,1);
    }
  }
}

/*Writes the coded/not coded flags for each block belonging to a partially
   coded super block.
  These flags are run-length encoded, with the flag value alternating between
   each run.
  _opb:     The oggpack_buffer to write to.
  _frarray: The fragment array containing the flags.*/
static void oc_frarray_pack_coded_block_flags(oggpack_buffer *_opb,
 oc_fragment_array *_frarray){
  int flag;
  int run_count;
  int bli;
  if(_frarray->nblock_coded_flags<=0)return;
  flag=_frarray->block_coded_flags[0];
  oggpackB_write(_opb,flag,1);
  for(bli=0;bli<_frarray->nblock_coded_flags;bli++){
    for(run_count=0;bli<_frarray->nblock_coded_flags;bli++){
      if(_frarray->block_coded_flags[bli]!=flag)break;
      run_count++;
    }
    /*Since each super block must have a mix of coded and not coded blocks to
       get on this list, we are guaranteed a maximum run size of 30 (16 blocks
       per super block, with flags (1000 0000 0000 0000) (0000 0000 0000 0001)
       or its complement).
      This avoids the nastiness of the VLC not letting us encode runs long
       enough like above.*/
    oc_frarray_pack_block_run(_opb,run_count);
    flag=!flag;
  }
}

/*Scans the list of fragments that belong to each super block to determine
   whether it is fully coded, partially coded, or not coded.
  The block_coded_flags list is populated using the blocks belonging to
   partially coded super blocks.
  _frarray: The fragment array containing the flags.*/
static void oc_frarray_update_sb_coded_bits(oc_fragment_array *_frarray){
  int flag;
  int sbi;
  int bli;
  _frarray->nblock_coded_flags=bli=0;
  for(sbi=0;sbi<_frarray->nsbs;sbi++){
    oc_sb *sb;
    int    mbi;
    sb=_frarray->sbs+sbi;
    sb->coded_fully=1;
    sb->coded_partially=0;
    /*Check the status of all the fragments coded in this super block.*/
    for(mbi=0;mbi<4;mbi++)if(sb->mb_valid&1<<mbi){
      int bi;
      for(bi=0;bi<4;bi++){
        int fragi;
        fragi=sb->map[mbi][bi];
        if(fragi>=0){
          flag=_frarray->frags[fragi].coded;
          sb->coded_fully&=flag;
          sb->coded_partially|=flag;
          _frarray->block_coded_flags[bli++]=flag;
        }
      }
    }
    /*If this is a partially coded block, keep the entries just added to the
       code block flag list.*/
    if(!sb->coded_fully&&sb->coded_partially){
      _frarray->nblock_coded_flags=bli;
    }
    /*Otherwise, discard these entries from the list, as they are implicit.*/
    else{
      sb->coded_partially=0;
      bli=_frarray->nblock_coded_flags;
    }
  }
}

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
      /*By default, set all fragment indices to -1.*/
      memset(sb->map[0],0xFF,sizeof(sb->map));
      /*Figure out how many rows of blocks in this super block lie within the
         image.*/
      jmax=_hfrags-x;
      if(jmax>4)jmax=4;
      else if(jmax<=0)break;
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
  int i;
  _x>>=1;
  for(i=0;i<2;i++){
    int fragi;
    if(_y+i>=_frplanes[1].nvfrags)break;
    fragi=(_y+i)*_frplanes[1].nhfrags+_x;
    _mb->map[1][i<<1]=fragi+_frplanes[1].froffset;
    _mb->map[2][i<<1]=fragi+_frplanes[2].froffset;
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
  int j;
  _y>>=1;
  for(j=0;j<2;j++){
    int fragi;
    if(_x+j>=_frplanes[1].nhfrags)break;
    fragi=_y*_frplanes[1].nhfrags+_x+j;
    _mb->map[1][j]=fragi+_frplanes[1].froffset;
    _mb->map[2][j]=fragi+_frplanes[2].froffset;
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
static const oc_mb_fill_cmapping_func MB_FILL_CMAPPING_TABLE[4]={
  (oc_mb_fill_cmapping_func)oc_mb_fill_cmapping00,
  oc_mb_fill_cmapping01,
  oc_mb_fill_cmapping10,
  oc_mb_fill_cmapping11
};

/*Fills in the mapping from macro blocks to their corresponding fragment
   numbers in each plane.
  _mbs:      The array of macro blocks.
  _frplanes: The descriptions of the fragment planes.
  _chdec:    Whether or not chroma is decimated horizontally.
  _cvdec     Whether or not chroma is decimated vertically.*/
static void oc_mb_create_mapping(oc_mb _mbs[],
 const oc_fragment_plane _frplanes[3],int _chdec,int _cvdec){
  oc_mb_fill_cmapping_func  mb_fill_cmapping;
  oc_mb                    *mb0;
  int                       y;
  mb0=_mbs;
  mb_fill_cmapping=MB_FILL_CMAPPING_TABLE[_cvdec<<1|_chdec];
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
          /*Initialize fragment indexes to -1.*/
          memset(mb->map,0xFF,sizeof(mb->map));
          /*Make sure this macro block is within the encoded region.*/
          if(mbx>=_frplanes[0].nhfrags||mby>=_frplanes[0].nvfrags){
            mb->mode=-1;
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
  _frarray: The fragment array containing the fragments to be marked.
  _info:    The encoding information describing the frame.*/
static void oc_frarray_init_border(oc_fragment_array *_frarray,
 const theora_info *_info){
  typedef struct{
    int x0;
    int xf;
    int y0;
    int yf;
  }oc_crop_rect;
  oc_fragment       *frag;
  oc_fragment       *yfrag_end;
  oc_fragment       *xfrag_end;
  oc_fragment_plane *fplane;
  oc_crop_rect      *crop;
  oc_crop_rect       crop_rects[3];
  int                nborder_frags;
  int                pli;
  int                y;
  int                x;
  /*The method we use here is slow, but the code is dead simple and handles
     all the special cases easily.
    We only ever need to do it once.*/
  /*Loop through the fragments, marking those completely outside the
     displayable region and counting those that straddle the border.*/
  nborder_frags=0;
  yfrag_end=frag=_frarray->frags;
  for(pli=0;pli<3;pli++){
    fplane=&_frarray->frplanes[pli];
    crop=&crop_rects[pli];
    /*Set up the cropping rectangle for this plane.*/
    crop->x0=_info->offset_x;
    crop->xf=_info->offset_x+_info->frame_width;
    crop->y0=_info->offset_y;
    crop->yf=_info->offset_y+_info->frame_height;
    if(pli>0){
      if(_info->chroma_x_decimated){
        crop->x0=crop->x0>>1;
        crop->xf=crop->xf+1>>1;
      }
      if(_info->chroma_y_decimated){
        crop->y0=crop->y0>>1;
        crop->yf=crop->yf+1>>1;
      }
    }
    y=0;
    for(yfrag_end+=fplane->nfrags;frag<yfrag_end;){
      x=0;
      for(xfrag_end=yfrag_end+fplane->nhfrags;frag<xfrag_end;frag++){
        /*Note the special checks for an empty cropping rectangle.
          This guarantees that if we count a fragment as straddling the
           border, at least one pixel in the fragment will be inside the
           displayable region.*/
        if(x+8<=crop->x0||crop->xf<=x||y+8<=crop->y0||crop->yf<=y||
         crop->x0>=crop->xf||crop->y0>=crop->yf){
          frag->invalid=1;
        }
        else if(x<crop->x0&&crop->x0<x+8||x<crop->xf&&crop->xf<x+8||
         y<crop->y0&&crop->y0<y+8||y<crop->yf&&crop->yf<y+8){
          nborder_frags++;
        }
        x+=8;
      }
    }
  }
  /*If we have no fragments straddling the boundary, there's nothing left to
     do.*/
  if(nborder_frags<=0)return;
  /*Allocate storage for the border fragments.*/
  _frarray->borders=_ogg_calloc(nborder_frags,sizeof(_frarray->borders[0]));
  /*Loop through the fragments again, this time filling in the border fragment
     info.*/
  nborder_frags=0;
  yfrag_end=frag=_frarray->frags;
  for(pli=0;pli<3;pli++){
    fplane=&_frarray->frplanes[pli];
    crop=&crop_rects[pli];
    y=0;
    for(yfrag_end+=fplane->nfrags;frag<yfrag_end;){
      x=0;
      for(xfrag_end=yfrag_end+fplane->nhfrags;frag<xfrag_end;frag++){
        /*We can skip the checks for an empty crop rectangle here, since we
           know we have at least one non-empty border fragment.*/
        if(x+8<=crop->x0||crop->xf<=x||y+8<=crop->y0||crop->yf<=y)continue;
        else if(x<crop->x0&&crop->x0<x+8||x<crop->xf&&crop->xf<x+8||
         y<crop->y0&&crop->y0<y+8||y<crop->yf&&crop->yf<y+8){
          int i;
          frag->border=&_frarray->borders[nborder_frags++];
          for(i=0;i<8;i++){
            int j;
            for(j=0;j<8;j++){
              if(x+j>=crop->x0&&x+j<crop->xf&&y+i>=crop->y0&&y+i<crop->yf){
                frag->border->mask|=i<<3|j;
                frag->border->npixels++;
              }
            }
          }
        }
        x+=8;
      }
    }
  }
}

void oc_frarray_init(oc_fragment_array *_frarray,theora_info *_info){
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
  yhfrags=_info->width>>3;
  yvfrags=_info->height>>3;
  chfrags=_info->chroma_x_decimated?yhfrags+1>>1:yhfrags;
  cvfrags=_info->chroma_y_decimated?yvfrags+1>>1:yvfrags;
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
  nmbs=nsbs<<2;
  /*Initialize the fragment array.*/
  memset(_frarray,0,sizeof(*_frarray));
  _frarray->frplanes[0].nhfrags=yhfrags;
  _frarray->frplanes[0].nvfrags=yvfrags;
  _frarray->frplanes[0].froffset=0;
  _frarray->frplanes[0].nfrags=yfrags;
  _frarray->frplanes[0].nhsbs=yhsbs;
  _frarray->frplanes[0].nvsbs=yvsbs;
  _frarray->frplanes[0].sboffset=0;
  _frarray->frplanes[0].nsbs=ysbs;
  _frarray->frplanes[1].nhfrags=_frarray->frplanes[2].nhfrags=chfrags;
  _frarray->frplanes[1].nvfrags=_frarray->frplanes[2].nvfrags=cvfrags;
  _frarray->frplanes[1].froffset=yfrags;
  _frarray->frplanes[2].froffset=yfrags+cfrags;
  _frarray->frplanes[1].nfrags=_frarray->frplanes[2].nfrags=cfrags;
  _frarray->frplanes[1].nhsbs=_frarray->frplanes[2].nhsbs=chsbs;
  _frarray->frplanes[1].nvsbs=_frarray->frplanes[2].nvsbs=cvsbs;
  _frarray->frplanes[1].sboffset=ysbs;
  _frarray->frplanes[2].sboffset=ysbs+csbs;
  _frarray->frplanes[1].nsbs=_frarray->frplanes[2].nsbs=csbs;
  _frarray->frags=_ogg_calloc(nfrags,sizeof(oc_fragment));
  _frarray->nfrags=nfrags;
  _frarray->sbs=_ogg_calloc(nsbs,sizeof(oc_sb));
  _frarray->nsbs=nsbs;
  _frarray->mbs=_ogg_calloc(nmbs,sizeof(oc_mb));
  _frarray->nmbs=nmbs;
  _frarray->block_coded_flags=_ogg_calloc(nfrags,
   sizeof(_frarray->block_coded_flags[0]));
  /*Create the mapping from super blocks to fragments.*/
  for(pli=0;pli<3;pli++){
    oc_fragment_plane *frplane;
    frplane=_frarray->frplanes+pli;
    oc_sb_create_plane_mapping(_frarray->sbs+frplane->sboffset,
     frplane->froffset,frplane->nhfrags,frplane->nvfrags);
  }
  /*Create the mapping from macro blocks to fragments.*/
  oc_mb_create_mapping(_frarray->mbs,_frarray->frplanes,
   _info->chroma_x_decimated,_info->chroma_y_decimated);
  /*Initialize the invalid and border fields of each fragment.*/
  oc_frarray_init_border(_frarray,_info);
}

/*Sets the buffer pointer in each fragment to point to the portion of the
   image buffer which it corresponds to.
  _frarray: The array of fragments to fill.
  _buf_idx: The index of the buffer pointer to fill.
            The first three correspond to our reconstructed frame buffers,
             while the last corresponds to the input image.
  _img:     The image buffer to fill the fragments with.*/
void oc_frarray_fill_buffer_ptrs(oc_fragment_array *_frarray,int _buf_idx,
 oc_yuv_buffer _img){
  int pli;
  /*Special handling for the input image to give us the opportunity to skip
     some updates.
    The other buffers do not change throughout the encoding process.*/
  if(_buf_idx==3){
     if(memcmp(_frarray->input,_img,sizeof(_img))==0)return;
     memcpy(_frarray->input,_img,sizeof(_img));
  }
  for(pli=0;pli<3;pli++){
    oc_img_plane      *iplane;
    oc_fragment_plane *frplane;
    oc_fragment       *frag;
    oc_fragment       *yfrag_end;
    unsigned char     *ypix;
    iplane=&_img[pli];
    frplane=&_frarray->frplanes[pli];
    ypix=iplane->data;
    frag=_frarray->frags+frplane->froffset;
    yfrag_end=frag+frplane->nfrags;
    while(frag<yfrag_end){
      oc_fragment   *xfrag_end;
      unsigned char *xpix;
      xpix=ypix;
      for(xfrag_end=frag+frplane->nhfrags;frag<xfrag_end;frag++){
        frag->buffer[_buf_idx]=xpix;
        xpix+=8;
      }
      ypix+=iplane->stride<<3;
    }
  }
}
