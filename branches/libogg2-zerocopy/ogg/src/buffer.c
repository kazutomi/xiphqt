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

  function: centralized fragment buffer management
  last mod: $Id: buffer.c,v 1.1.2.5 2003/03/06 23:13:36 xiphmont Exp $

 ********************************************************************/

#ifdef OGGBUFFER_DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>
#include "ogginternal.h"

/* basic, centralized Ogg memory management.

   We trade in recycled, refcounted [in decode] memory blocks
   belonging to a specific pool.

   During decode, buffers are passed around and consumed primarily in
   reference form. Encode uses the actual ogg_buffer as there's no
   multipath usage. */

void ogg_buffer_init(ogg_buffer_state *bs,int encode_decode_flag){
  memset(bs,0,sizeof(*bs));
  ogg_mutex_init(&bs->mutex);
  bs->encode_decode_flag=encode_decode_flag;
}

void ogg_buffer_clear(ogg_buffer_state *bs){
  if(bs->outstanding==0){
    ogg_mutex_clear(&bs->mutex);
    
    while(bs->unused_pool){
      ogg_buffer *b=bs->unused_pool;
      bs->unused_pool=b->next;
      if(b->data)_ogg_free(b->data);
      _ogg_free(b);
    }
  }
#ifdef OGGBUFFER_DEBUG
  else{
    
    fprintf(stderr,"ERROR: Freeing ogg_buffer_state with buffers outstanding.\n");
    exit(1);
    
  }
#endif
}

ogg_buffer *ogg_buffer_alloc(ogg_buffer_state *bs,long bytes){
  ogg_buffer *ret;
  ogg_mutex_lock(&bs->mutex);

  /* do we have an unused buffer sitting in the pool? */
  if(bs->unused_pool){
    ret=bs->unused_pool;
    bs->unused_pool=ret->next;
    ogg_mutex_unlock(&bs->mutex);

    /* if the unused buffer is too small, grow it */
    if(ret->size<bytes){
      ret->data=_ogg_realloc(ret->data,bytes);
      ret->size=bytes;
    }
  }else{
    ogg_mutex_unlock(&bs->mutex);

    /* allocate a new buffer */
    ret=_ogg_malloc(sizeof(*ret));
    ret->data=_ogg_malloc(bytes);
    ret->size=bytes;
  }

  ret->used=0;
  ret->next=0;
  if(bs->encode_decode_flag)
    ret->refbeg=1;
  else
    ret->refbeg=0;

  return ret;
}

/* this will succeed only if 
   a) the buffer is unused 
   b) with a refcount of one, or part of an encode-side pool

   currently used only by the sync fifo code to avoid the need
   for a doubly-linked list */

int ogg_buffer_realloc(ogg_buffer_state *bs,ogg_buffer *ob,long bytes){
  int ret=-1;
  ogg_mutex_lock(&bs->mutex);
  if(ob->used==0 && (ob->refbeg<=1 || bs->encode_decode_flag==0)){
    ret=0;
    if(bytes>ob->size){
      ob->data=_ogg_realloc(ob->data,bytes);
      ob->size=bytes;
    }
  }
  ogg_mutex_unlock(&bs->mutex);
  return ret;
}

static void _ogg_buffer_release(ogg_buffer *ob,ogg_buffer_state *bs){
  if(bs->encode_decode_flag){
    ob->refbeg--;
#ifdef OGGBUFFER_DEBUG
    if(ob->refbeg<0){
      fprintf(stderr,"ERROR: Too many release()es on ogg_buffer.\n");
      exit(1);
    }
#endif
  }
  if(!bs->encode_decode_flag || !ob->refbeg==0){
    ob->next=bs->unused_pool;
    bs->unused_pool=ob;
  }
}

/* offered only on encode-side */
ogg_buffer *ogg_buffer_pretruncate(ogg_buffer *ob,ogg_buffer_state *bs,
				   int bytes){

  if(bs->encode_decode_flag)return NULL;
  ogg_mutex_lock(&bs->mutex);
  
  while(ob && bytes>0){
    int current=ob->used-ob->refbeg;
#ifdef OGGBUFFER_DEBUG
    if(current<0){
      fprintf(stderr,"ERROR: Negative used ogg_buffer size.\n");
      exit(1);
    }
#endif

    if(current<=bytes){
      /* now completely unused; release the buffer */
      ogg_buffer *next=ob->next;
      _ogg_buffer_release(ob,bs);
      bytes-=current;
      ob=next;
    }else{
      /* trim the current buffer */
      ob->refbeg+=bytes;
      bytes=0;
    }
    
#ifdef OGGBUFFER_DEBUG
    if(ob==NULL && bytes>0){
      fprintf(stderr,"ERROR: requested pretruncate larger than ogg_buffer.\n");
      exit(1);
    }
#endif
    
  }

  ogg_mutex_unlock(&bs->mutex);  
  return ob;
}

void ogg_buffer_release(ogg_buffer *ob,ogg_buffer_state *bs){
  ogg_mutex_lock(&bs->mutex);
  _ogg_buffer_release(ob,bs);
  ogg_mutex_unlock(&bs->mutex);
}

void ogg_reference_mark(ogg_buffer_reference *or){
  ogg_mutex_lock(&or->owner->mutex);
  if(or->owner->encode_decode_flag){
    long bytes=or->begin+or->length;
    ogg_buffer *ob=or->buffer;
    
    while(bytes>0){
#ifdef OGGBUFFER_DEBUG
      if(ob==NULL){
	fprintf(stderr,"ERROR: ogg_reference_mark ran off the end of buffer chain.\n");
	exit(1);
      }
#endif
      
      ob->refbeg++;
      bytes-=ob->used;
      ob=ob->next;
    }
  }
  ogg_mutex_unlock(&or->owner->mutex);
}

void ogg_reference_clear(ogg_buffer_reference *or){
  or->buffer=0;
}

void ogg_reference_release(ogg_buffer_reference *or){
  if(or && or->buffer){
    long bytes;
    ogg_buffer *ob;
    ogg_mutex_lock(&or->owner->mutex);
    
    bytes=or->begin+or->length;
    ob=or->buffer;
    
    while(bytes>0){
#ifdef OGGBUFFER_DEBUG
      if(ob==NULL){
	fprintf(stderr,"ERROR: ogg_reference_release ran off the end of buffer chain.\n");
      exit(1);
      }
#endif
      {
	ogg_buffer *next=ob->next;
	bytes-=ob->used;
      _ogg_buffer_release(ob,or->owner);
      ob=next;
      }
    }
    
    ogg_mutex_unlock(&or->owner->mutex);
  }
}
