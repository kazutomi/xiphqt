/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2003             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: byte-aligned access; array-like abstraction over buffers
  last mod: $Id: bytewise.c,v 1.1.2.1 2003/03/20 23:06:39 xiphmont Exp $

 ********************************************************************/

#include <string.h>
#include <stdlib.h>
#include "ogginternal.h"

/* this is an internal abstraction, and it is not guarded from misuse
   or botching a fencepost. */

static void _positionB(oggbyte_buffer *b,int pos){
  if(pos<b->headpos){
    /* start at beginning, scan forward */
    b->headref=b->baseref;
    b->headpos=b->basepos;
    b->headend=b->headpos+b->headref->length;
  }
}

static void _positionF(oggbyte_buffer *b,int pos){
  /* scan forward for position */
  while(pos>=b->headend){
    /* just seek forward */
    b->headpos+=b->headref->length;
    pos-=b->headref->length;
    b->headref=b->headref->next;
    b->headend=b->headref->length+b->headpos;
  }
}

static void _positionFE(oggbyte_buffer *b,int pos){
  /* scan forward for position */
  while(pos>=b->headend){
    if(!b->headref->next){
      /* perhaps just need to extend length field... */
      if(pos-b->headpos < b->headref->buffer->size-b->headref->buffer->begin){

	/* yes, there's space here */
	b->headref->length=pos-b->headpos;

      }else{

	/* extend the array and span */
	pos-=b->headref->length;
	b->headpos+=b->headref->length;	
	b->headref=ogg_buffer_extend(b->headref,OGGPACK_CHUNKSIZE);
	b->headend=b->headref->buffer->size+b->headpos;

      }

    }else{

      /* just seek forward */
      b->headpos+=b->headref->length;
      pos-=b->headref->length;
      b->headref=b->headref->next;
      b->headend=b->headref->length+b->headpos;
    }
  }
}

static int oggbyte_init(oggbyte_buffer *b,ogg_reference *or,long base,
			ogg_buffer_state *bs){
  memset(b,0,sizeof(*b));
    
  if(!or){
    if(base || !bs)return -1;
    or=ogg_buffer_alloc(bs,OGGPACK_CHUNKSIZE);
  }

  /* cheat and use the _position code */
  b->owner=bs;
  b->baseref=or;
  b->basepos=0;
  b->headpos=base+1; /* force _position to scan from beginning */
  _positionB(b,base);
  _positionFE(b,base);
  b->baseref=b->headref;
  b->headpos=b->basepos=b->headpos-base;
  b->headend-=base;
  
  return(0);
}

static void oggbyte_set1(oggbyte_buffer *b,unsigned char val,int pos){
  _positionB(b,pos);
  _positionFE(b,pos);
  b->headptr[pos-b->headpos]=val;
}

static void oggbyte_set2(oggbyte_buffer *b,int val,int pos){
  _positionB(b,pos);
  _positionFE(b,pos);
  b->headptr[pos-b->headpos]=val;
  _positionFE(b,++pos);
  b->headptr[pos-b->headpos]=val>>8;
}

static void oggbyte_set4(oggbyte_buffer *b,ogg_int32_t val,int pos){
  int i;
  _positionB(b,pos);
  for(i=0;i<4;i++){
    _positionFE(b,pos);
    b->headptr[pos-b->headpos]=val;
    val>>=8;
    ++pos;
  }
}

static void oggbyte_set8(oggbyte_buffer *b,ogg_int64_t val,int pos){
  int i;
  _positionB(b,pos);
  for(i=0;i<8;i++){
    _positionFE(b,pos+i);
    b->headptr[pos-b->headpos]=val;
    val>>=8;
    ++pos;
  }
}
 
static unsigned char oggbyte_read1(oggbyte_buffer *b,int pos){
  _positionB(b,pos);
  _positionF(b,pos);
  return b->headptr[pos-b->headpos];
}

static int oggbyte_read2(oggbyte_buffer *b,int pos){
  int ret;
  _positionB(b,pos);
  _positionF(b,pos);
  ret=b->headptr[pos-b->headpos];
  _positionF(b,++pos);
  ret|=b->headptr[pos-b->headpos]<<8;
}

static ogg_int32_t oggbyte_read4(oggbyte_buffer *b,int pos){
  ogg_int32_t ret;
  _positionB(b,pos);
  _positionF(b,pos);
  ret=b->headptr[pos-b->headpos];
  _positionF(b,++pos);
  ret|=b->headptr[pos-b->headpos]<<8;
  _positionF(b,++pos);
  ret|=b->headptr[pos-b->headpos]<<16;
  _positionF(b,++pos);
  ret|=b->headptr[pos-b->headpos]<<24;
  return ret;
}

static ogg_int32_t oggbyte_read8(oggbyte_buffer *b,int pos){
  ogg_int64_t ret;
  unsigned char t[7];
  int i;
  _positionB(b,pos);
  for(i=0;i<7;i++){
    _positionF(b,pos);
    t[i]=b->headptr[pos++ -b->headpos];
  }

  _positionF(b,pos);
  ret=b->headptr[pos-b->headpos];

  for(i=6;i>=0;--i)
    ret= ret<<8 | t[i];

  return ret;
}

