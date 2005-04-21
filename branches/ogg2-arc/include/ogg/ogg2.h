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

 function: toplevel libogg include
 last mod: $Id: ogg.h 9024 2005-03-03 12:56:20Z arc $

 ********************************************************************/
#ifndef _OGG2_H
#define _OGG2_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <ogg2/os_types.h>
  
struct ogg2_buffer;
struct ogg2_reference;
struct ogg2_buffer_state;
struct ogg2_sync_state;
struct ogg2_stream_state;
struct ogg2pack_buffer;

typedef struct ogg2_buffer ogg2_buffer;
typedef struct ogg2_reference ogg2_reference;
typedef struct ogg2_buffer_state ogg2_buffer_state;
typedef struct ogg2_sync_state ogg2_sync_state;
typedef struct ogg2_stream_state ogg2_stream_state;
typedef struct ogg2pack_buffer ogg2pack_buffer;

typedef struct {
  ogg2_reference *packet;
  long           bytes;
  long           b_o_s;
  long           e_o_s;
  ogg_int64_t    top_granule;
  ogg_int64_t    end_granule;
  ogg_int64_t    packetno;     /* sequence number for decode; the framing
				  knows where there's a hole in the data,
				  but we need coupling so that the codec
				  (which is in a seperate abstraction
				  layer) also knows about the gap */
} ogg2_packet;

typedef struct {
  ogg2_reference *header;
  int            header_len;
  ogg2_reference *body;
  int            body_len;
} ogg2_page;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

extern ogg2_buffer_state *ogg2_buffer_create(void);
extern int   ogg2pack_buffersize(void);
extern void  ogg2pack_writeinit(ogg2pack_buffer *b,ogg2_buffer_state *bs);
extern ogg2_reference *ogg2pack_writebuffer(ogg2pack_buffer *b);
extern void  ogg2pack_writealign(ogg2pack_buffer *b);
extern void  ogg2pack_writeclear(ogg2pack_buffer *b);
extern void  ogg2pack_readinit(ogg2pack_buffer *b,ogg2_reference *r);
extern void  ogg2pack_write(ogg2pack_buffer *b,unsigned long value,int bits);
extern int   ogg2pack_look(ogg2pack_buffer *b,int bits,unsigned long *ret);
extern long  ogg2pack_look1(ogg2pack_buffer *b);
extern void  ogg2pack_adv(ogg2pack_buffer *b,int bits);
extern void  ogg2pack_adv1(ogg2pack_buffer *b);
extern int   ogg2pack_read(ogg2pack_buffer *b,int bits,unsigned long *ret);
extern long  ogg2pack_read1(ogg2pack_buffer *b);
extern long  ogg2pack_bytes(ogg2pack_buffer *b);
extern long  ogg2pack_bits(ogg2pack_buffer *b);
extern int   ogg2pack_eop(ogg2pack_buffer *b);

extern void  ogg2packB_writeinit(ogg2pack_buffer *b,ogg2_buffer_state *bs);
extern ogg2_reference *ogg2packB_writebuffer(ogg2pack_buffer *b);
extern void  ogg2packB_writealign(ogg2pack_buffer *b);
extern void  ogg2packB_writeclear(ogg2pack_buffer *b);
extern void  ogg2packB_readinit(ogg2pack_buffer *b,ogg2_reference *r);
extern void  ogg2packB_write(ogg2pack_buffer *b,unsigned long value,int bits);
extern int   ogg2packB_look(ogg2pack_buffer *b,int bits,unsigned long *ret);
extern long  ogg2packB_look1(ogg2pack_buffer *b);
extern void  ogg2packB_adv(ogg2pack_buffer *b,int bits);
extern void  ogg2packB_adv1(ogg2pack_buffer *b);
extern int   ogg2packB_read(ogg2pack_buffer *b,int bits,unsigned long *ret);
extern long  ogg2packB_read1(ogg2pack_buffer *b);
extern long  ogg2packB_bytes(ogg2pack_buffer *b);
extern long  ogg2packB_bits(ogg2pack_buffer *b);
extern int   ogg2packB_eop(ogg2pack_buffer *b);

/* Ogg BITSTREAM PRIMITIVES: encoding **************************/
extern long     ogg2_sync_bufferout(ogg2_sync_state *oy, unsigned char **buffer);
extern int      ogg2_sync_pagein(ogg2_sync_state *oy,ogg2_page *og);
extern int      ogg2_sync_read(ogg2_sync_state *oy,long bytes);

extern int      ogg2_stream_packetin(ogg2_stream_state *os, ogg2_packet *op);
extern int      ogg2_stream_pageout(ogg2_stream_state *os, ogg2_page *og);
extern int      ogg2_stream_flush(ogg2_stream_state *os, ogg2_page *og);

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

extern ogg2_sync_state *ogg2_sync_create(void);
extern int      ogg2_sync_destroy(ogg2_sync_state *oy);
extern int      ogg2_sync_reset(ogg2_sync_state *oy);

extern unsigned char *ogg2_sync_bufferin(ogg2_sync_state *oy, long size);
extern int      ogg2_sync_wrote(ogg2_sync_state *oy, long bytes);
extern long     ogg2_sync_pageseek(ogg2_sync_state *oy,ogg2_page *og);
extern int      ogg2_sync_pageout(ogg2_sync_state *oy, ogg2_page *og);
extern int      ogg2_stream_pagein(ogg2_stream_state *os, ogg2_page *og);
extern int      ogg2_stream_packetout(ogg2_stream_state *os,ogg2_packet *op);
extern int      ogg2_stream_packetpeek(ogg2_stream_state *os,ogg2_packet *op);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

extern ogg2_stream_state *ogg2_stream_create(int serialno);
extern int      ogg2_stream_destroy(ogg2_stream_state *os);
extern int      ogg2_stream_reset(ogg2_stream_state *os);
extern int      ogg2_stream_reset_serialno(ogg2_stream_state *os,int serialno);
extern int      ogg2_stream_eos(ogg2_stream_state *os);
extern int	ogg2_stream_setdiscont(ogg2_stream_state *os);

extern int      ogg2_page_version(ogg2_page *og);
extern int      ogg2_page_continued(ogg2_page *og);
extern int      ogg2_page_bos(ogg2_page *og);
extern int      ogg2_page_eos(ogg2_page *og);
extern ogg_int64_t  ogg2_page_granulepos(ogg2_page *og);
extern ogg_uint32_t ogg2_page_serialno(ogg2_page *og);
extern ogg_uint32_t ogg2_page_pageno(ogg2_page *og);
extern int      ogg2_page_packets(ogg2_page *og);

extern void     ogg2_page_set_continued(ogg2_page *og, int value);
extern void     ogg2_page_set_bos(ogg2_page *og, int value);
extern void     ogg2_page_set_eos(ogg2_page *og, int value);
extern void     ogg2_page_set_granulepos(ogg2_page *og, ogg_int64_t value);
extern void     ogg2_page_set_serialno(ogg2_page *og, ogg_uint32_t value);
extern void     ogg2_page_set_pageno(ogg2_page *og, ogg_uint32_t value);

extern int      ogg2_packet_release(ogg2_packet *op);
extern int      ogg2_page_release(ogg2_page *og);

/* Ogg BITSTREAM PRIMITIVES: return codes ***************************/

#define  OGG2_SUCCESS   0

#define  OGG2_HOLE     -10
#define  OGG2_SPAN     -11
#define  OGG2_EVERSION -12
#define  OGG2_ESERIAL  -13
#define  OGG2_EINVAL   -14
#define  OGG2_EEOS     -15

#ifdef __cplusplus
}
#endif

#endif  /* _OGG2_H */
