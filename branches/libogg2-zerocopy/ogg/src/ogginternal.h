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
 last mod: $Id: ogginternal.h,v 1.1.2.1 2002/12/31 01:18:02 xiphmont Exp $

 ********************************************************************/

#ifndef _OGGI_H
#define _OGGI_H

typedef struct _ogg_sync_state * ogg_sync_state;
typedef struct _ogg_stream_state * ogg_stream_state;
typedef struct ogg_lbuffer fragmented_reference;

#include <ogg/ogg.h>

typedef struct{
  fragmented_reference *segment;
  int                   cursor;
} fragmented_cursor;

/* an internal type meant to be struct-compatable with fragmented_reference */
typedef struct fragmented_buffer {
  unsigned char            *data;
  int                       used;
  struct fragmented_buffer *next;
  int                       size;
} fragmented_buffer;

typedef struct ogg_packet_chain {
  fragmented_reference    *packet;
  long                     bytes;
  ogg_int64_t              granulepos;

  struct ogg_packet_chain *next;
} ogg_packet_chain;


typedef struct {

  /* stream buffers */
  fragmented_buffer *unused_fifo;
  fragmented_buffer *fifo_head;
  fragmented_buffer *fifo_tail;
  fragmented_buffer *fifo_returned;
  int                fifo_returned_pos;
  int                fifo_fill;

  /* stream sync management */
  int                unsynced;
  int                headerbytes;
  int                bodybytes;

  /* page/packet tracking and management */
  ogg_packet_chain  *unused_packet; /* contain allocated ->data
                                              storage */
  struct _ogg_stream_state *stream_list;

} _ogg_sync_state;

typedef struct ogg_stream_state {
  ogg_packet_chain *head;
  ogg_packet_chain *tail;
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
  struct _ogg_sync_state   *sync;
  struct _ogg_stream_state *next;

} _ogg_stream_state;

#endif
