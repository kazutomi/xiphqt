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
  last mod: $Id: buffer.c,v 1.1.2.4 2003/02/10 18:05:46 xiphmont Exp $

 ********************************************************************/

#ifdef OGGBUFFER_DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>
#include "ogginternal.h"

/* basic, centralized Ogg memory management.

   We trade in recycled, refcounted memory blocks belonging to a
   specific pool.

   Buffers are passed around and consumed primarily in reference form. */

void ogg_buffer_init(ogg_buffer_state *bs){
  memset(bs,0,sizeof(*bs));
  ogg_mutex_init(&bs->mutex);
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
  ret->refcount=1;
  return ret;
}

static void _ogg_buffer_release(ogg_buffer *ob,ogg_buffer_state *bs){
  ob->refcount--;
  if(ob->refcount==0){
    ob->next=bs->unused_pool;
    bs->unused_pool=ob;
  }
#ifdef OGGBUFFER_DEBUG
  if(ob->refcount<0){
    fprintf(stderr,"ERROR: Too many release()es on ogg_buffer.\n");
    exit(1);
  }
#endif
}

void ogg_buffer_release(ogg_buffer *ob,ogg_buffer_state *bs){
  ogg_mutex_lock(&bs->mutex);
  _ogg_buffer_release(ob,bs);
  ogg_mutex_unlock(&bs->mutex);
}

void ogg_reference_mark(ogg_buffer_reference *or){
  long bytes;
  ogg_buffer *ob;
  ogg_mutex_lock(&or->owner->mutex);
  
  bytes=or->begin+or->length;
  ob=or->buffer;

  while(bytes>0){
#ifdef OGGBUFFER_DEBUG
    if(ob==NULL){
      fprintf(stderr,"ERROR: ogg_reference_mark ran off the end of buffer chain.\n");
      exit(1);
    }
#endif

    ob->refcount++;
    bytes-=ob->used;
    ob=ob->next;
  }

  ogg_mutex_unlock(&or->owner->mutex);
}

void ogg_reference_release(ogg_buffer_reference *or){
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
