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

 function: internal/hidden data representation structures
 last mod: $Id: ogginternal.h,v 1.1.2.6 2003/03/15 01:26:09 xiphmont Exp $

 ********************************************************************/

#ifndef _OGGI_H
#define _OGGI_H

#include <ogg2/ogg.h>
#include "mutex.h"

struct ogg_buffer_state{
  ogg_buffer    *unused_buffers;
  int            outstanding_buffers;

  ogg_reference *unused_references;
  int            outstanding_references;

  ogg_mutex_t mutex;
};

struct ogg_buffer {
  unsigned char      *data;
  long                size;
  int                 refcount;
  union {
    ogg_buffer_state *owner;
    ogg_buffer       *next;
  } ptr;
};

struct ogg_reference {
  struct ogg_buffer    *buffer;
  long                  begin;
  long                  length;
  struct ogg_reference *next;
};

struct oggpack_buffer {
  int            headbit;
  unsigned char *headptr;
  long           headend;

  /* memory management */
  ogg_reference *head;
  ogg_reference *tail;

  /* render the byte/bit counter API constant time */
  long count;              /* doesn't count the tail */
  ogg_buffer_state *owner; /* cache preferred pool for write lazy init */

};

typedef struct{
  ogg_reference *segment;
  int            cursor;
} ogg_buffer_cursor;

struct ogg_sync_state {
  /* encode/decode mem management */
  ogg_buffer_state  bufferpool;

  /* stream buffers */
  ogg_reference    *fifo_head;
  ogg_reference    *fifo_tail;
  
  long              fifo_cursor;
  long              fifo_fill;
  ogg_reference    *returned;

  /* stream sync management */
  int               unsynced;
  int               headerbytes;
  int               bodybytes;

};

struct ogg_stream_state {

  long              body_len;

  unsigned char    *header;       /* working space for header encode */
  int               header_fill;

  int     e_o_s;        /* set when we have buffered the last packet in the
                           logical bitstream */
  int     b_o_s;        /* set after we've written the initial page
                           of a logical bitstream */
  int     headers;      /* how many setup headers? */
  long    serialno;
  long    pageno;
  ogg_int64_t packetno; /* sequence number for decode; the framing
                           knows where there's a hole in the data,
                           but we need coupling so that the codec
                           (which is in a seperate abstraction
                           layer) also knows about the gap */

  /* for sync memory management use */
  struct ogg_sync_state   *sync;
  struct ogg_stream_state *next;

};

extern void           ogg_buffer_init(ogg_buffer_state *bs);
extern void           ogg_buffer_clear(ogg_buffer_state *bs);
extern ogg_reference *ogg_buffer_alloc(ogg_buffer_state *bs,long bytes);
extern ogg_reference *ogg_buffer_dup(ogg_reference *or,long begin,long length);
extern ogg_reference *ogg_buffer_extend(ogg_reference *or,long bytes);
extern void           ogg_buffer_mark(ogg_reference *or);
extern void           ogg_buffer_release(ogg_reference *or);
extern ogg_reference *ogg_buffer_pretruncate(ogg_reference *or,long pos);
extern void           ogg_buffer_posttruncate(ogg_reference *or,long pos);
extern void           ogg_buffer_cat(ogg_reference *tail, ogg_reference *head);

#endif
