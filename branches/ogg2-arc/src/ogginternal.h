/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Reference Library SOURCE CODE.      *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Ogg Reference Library SOURCE CODE IS (C) COPYRIGHT 1994-2004 *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: internal/hidden data representation structures
 last mod: $Id$

 ********************************************************************/

#ifndef _OGG2I_H
#define _OGG2I_H

#include <ogg2/ogg.h>
#include "mutex.h"

struct ogg2_buffer_state{
  ogg2_buffer    *unused_buffers;
  ogg2_reference *unused_references;
  int            outstanding;

  ogg2_mutex_t mutex;
  int         shutdown;
};

struct ogg2_buffer {
  unsigned char      *data;
  long                size;
  int                 refcount;
  
  union {
    ogg2_buffer_state *owner;
    ogg2_buffer       *next;
  } ptr;
};

struct ogg2_reference {
  struct ogg2_buffer    *buffer;
  long                  begin;
  long                  length;

  struct ogg2_reference *next;
#ifdef OGG2BUFFER_DEBUG
  int                   used;
#endif
};

struct ogg2pack_buffer {
  int            headbit;
  unsigned char *headptr;
  long           headend;

  /* memory management */
  ogg2_reference *head;
  ogg2_reference *tail;

  /* render the byte/bit counter API constant time */
  long              count; /* doesn't count the tail */
  ogg2_buffer_state *owner; /* useful on encode side */
};

typedef struct ogg2byte_buffer {
  ogg2_reference *baseref;

  ogg2_reference *ref;
  unsigned char *ptr;
  long           pos;
  long           end;

  ogg2_buffer_state *owner; /* if it's to be extensible; encode side */
  int               external; /* did baseref come from outside? */ 
} ogg2byte_buffer;

struct ogg2_sync_state {
  /* decode memory management pool */
  ogg2_buffer_state *bufferpool;

  /* stream buffers */
  ogg2_reference    *fifo_head;
  ogg2_reference    *fifo_tail;
  long              fifo_fill;

  /* stream sync management */
  int               unsynced;
  int               headerbytes;
  int               bodybytes;

};

struct ogg2_stream_state {
  /* encode memory management pool */
  ogg2_buffer_state *bufferpool;

  ogg2_reference *header_head;
  ogg2_reference *header_tail;
  ogg2_reference *body_head;
  ogg2_reference *body_tail;

  int            e_o_s;    /* set when we have buffered the last
                              packet in the logical bitstream */
  int            b_o_s;    /* set after we've written the initial page
			      of a logical bitstream */
  long           serialno;
  long           pageno;
  ogg_int64_t    packetno; /* sequence number for decode; the framing
			      knows where there's a hole in the data,
			      but we need coupling so that the codec
			      (which is in a seperate abstraction
			      layer) also knows about the gap */
  ogg_int64_t    granulepos;
  int            mode;     /* 0 = continuous, 1 = discontinuous */

  int            lacing_fill;
  ogg_uint32_t   body_fill;

  /* encode-side header build */
  unsigned int   watermark;
  ogg2byte_buffer header_build;
  int            continued;
  int            packets;

  /* decode-side state data */
  int            holeflag;
  int            spanflag;
  int            clearflag;
  int            laceptr;
  ogg_uint32_t   body_fill_next;
  
};

extern ogg2_buffer_state *ogg2_buffer_create(void);
extern void              ogg2_buffer_destroy(ogg2_buffer_state *bs);
extern ogg2_reference    *ogg2_buffer_alloc(ogg2_buffer_state *bs,long bytes);
extern void              ogg2_buffer_realloc(ogg2_reference *or,long bytes);
extern ogg2_reference    *ogg2_buffer_sub(ogg2_reference *or,long begin,long length);
extern ogg2_reference    *ogg2_buffer_dup(ogg2_reference *or);
extern ogg2_reference    *ogg2_buffer_extend(ogg2_reference *or,long bytes);
extern void              ogg2_buffer_mark(ogg2_reference *or);
extern void              ogg2_buffer_release(ogg2_reference *or);
extern void              ogg2_buffer_release_one(ogg2_reference *or);
extern ogg2_reference    *ogg2_buffer_pretruncate(ogg2_reference *or,long pos);
extern void              ogg2_buffer_posttruncate(ogg2_reference *or,long pos);
extern ogg2_reference    *ogg2_buffer_cat(ogg2_reference *tail, ogg2_reference *head);
extern ogg2_reference    *ogg2_buffer_walk(ogg2_reference *or);
extern long              ogg2_buffer_length(ogg2_reference *or);
extern ogg2_reference    *ogg2_buffer_split(ogg2_reference **tail,
				       ogg2_reference **head,long pos);
extern void              ogg2_buffer_outstanding(ogg2_buffer_state *bs);

extern  int              ogg2byte_init(ogg2byte_buffer *b,ogg2_reference *or,
				   ogg2_buffer_state *bs);
extern void              ogg2byte_clear(ogg2byte_buffer *b);
extern ogg2_reference    *ogg2byte_return_and_reset(ogg2byte_buffer *b);
extern void              ogg2byte_set1(ogg2byte_buffer *b,unsigned char val,
				   int pos);
extern void              ogg2byte_set2(ogg2byte_buffer *b,int val,int pos);
extern void              ogg2byte_set4(ogg2byte_buffer *b,ogg_uint32_t val,int pos);
extern void              ogg2byte_set8(ogg2byte_buffer *b,ogg_int64_t val,int pos);
extern unsigned char     ogg2byte_read1(ogg2byte_buffer *b,int pos);
extern int               ogg2byte_read2(ogg2byte_buffer *b,int pos);
extern ogg_uint32_t      ogg2byte_read4(ogg2byte_buffer *b,int pos);
extern ogg_int64_t       ogg2byte_read8(ogg2byte_buffer *b,int pos);
extern int               ogg2_page_checksum_set(ogg2_page *og);

#ifdef _V_SELFTEST
#define OGG2PACK_CHUNKSIZE 3
#define OGG2PACK_MINCHUNKSIZE 1
#else
#define OGG2PACK_CHUNKSIZE 128
#define OGG2PACK_MINCHUNKSIZE 16
#endif

#endif



