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

 function: decode stream sync and memory management foundation code;
           takes in raw data, spits out packets
 last mod: $Id: sync.c,v 1.1.2.1 2002/12/31 01:18:02 xiphmont Exp $

 note: The CRC code is directly derived from public domain code by
 Ross Williams (ross@guest.adelaide.edu.au).  See docs/framing.html
 for details.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "ogginternal.h" /* proper way to suck in ogg/ogg.h from a
			    libogg compile */

/* A complete description of Ogg framing exists in docs/framing.html */

/* Below we have decode stream buffer and memory management which is
   handled by physical stream and centralized in the ogg_sync_state
   structure. Encode side buffer management is done by logical stream
   in the ogg_stream_state structure */

static void _fc_seek(fragmented_cursor *fc,
		     fragmented_reference *fb,
		     int position){
  fc->segment=fb;
  fc->cursor=position;
  while(fc->segment && fc->cursor>=fc->segment->used){
    fc->cursor-=fc->segment->used;
    fc->segment=fc->segment->next;
  }
}

static int _fc_get(fragmented_cursor *fc){
  if(!fc->segment)return -1;
  if(fc->cursor>=fc->segment->used){
    fc->cursor=0;
    fc->segment=fc->segment->next;
    if(!fc->segment)return -1;
  }
  return(fc->segment->data[fc->cursor++]);
}

static void _fc_set(fragmented_cursor *fc,unsigned char val){
  if(!fc->segment)return;
  if(fc->cursor>=fc->segment->used){
    fc->cursor=0;
    fc->segment=fc->segment->next;
    if(!fc->segment)return;
  }
  fc->segment->data[fc->cursor++]=val;
}

static ogg_int64_t _fc_get_int64(fragmented_cursor *fc){
  ogg_int64_t ret;
  unsigned char y[7];
  y[0]=_fc_get(fc);
  y[1]=_fc_get(fc);
  y[2]=_fc_get(fc);
  y[3]=_fc_get(fc);
  y[4]=_fc_get(fc);
  y[5]=_fc_get(fc);
  y[6]=_fc_get(fc);
  ret =_fc_get(fc);
  
  ret =(ret<<8)|y[6];
  ret =(ret<<8)|y[5];
  ret =(ret<<8)|y[4];
  ret =(ret<<8)|y[3];
  ret =(ret<<8)|y[2];
  ret =(ret<<8)|y[1];
  ret =(ret<<8)|y[0];
  
  return(ret);
}

static ogg_uint32_t _fc_get_uint32(fragmented_cursor *fc){
  ogg_uint32_t ret;
  unsigned char y[3];
  y[0]=_fc_get(fc);
  y[1]=_fc_get(fc);
  y[2]=_fc_get(fc);
  ret =_fc_get(fc);
  ret =(ret<<8)|y[2];
  ret =(ret<<8)|y[1];
  ret =(ret<<8)|y[0];
  
  return(ret);
}

/* if there is a head fragment with sufficient space for the expose,
     return it
   if the head fragment doesn't have enough space or no head fragment:
     if there is no unused_fifo fragment, allocate one and return it
     if the unused_fifo fragment is large enough for the expose, return it
     else realloc the fragment in the unused_fifo buffer (the only graceful
       failback if the app is calling for exposes with varying sizes)
*/

static unsigned char *_sync_expose_fifo(_ogg_sync_state *oy,long size){
  if(!oy->fifo_head || 
     oy->fifo_head->size-oy->fifo_head->used<size){

    /* need to add to the fifo */
    
    fragmented_buffer *new;
    if(!oy->unused_fifo){
      oy->unused_fifo=_ogg_malloc(sizeof(*oy->unused_fifo));
      oy->unused_fifo->data=_ogg_malloc(size);
      oy->unused_fifo->size=size;
      oy->unused_fifo->used=0;
    }
    if(oy->unused_fifo->size<size){
      oy->unused_fifo->data=_ogg_realloc(oy->unused_fifo->data,size);
      oy->unused_fifo->size=size;
    }

    new=oy->unused_fifo;
    oy->unused_fifo=new->next;
    new->next=0;
    oy->fifo_head->next=new;
    oy->fifo_head=new;
    if(!oy->fifo_tail){
      oy->fifo_tail=new;
      oy->fifo_returned=new;
      oy->fifo_returned_pos=0;
    }

    return new->data;
  }

  return oy->fifo_head->data+oy->fifo_head->used;
  
}

static int _sync_wrote_fifo(_ogg_sync_state *oy,long size){
  if(oy->fifo_head->size-oy->fifo_head->used<size)
    size=oy->fifo_head->size-oy->fifo_head->used;
  oy->fifo_head->used+=size;
  oy->fifo_fill+=size;
  return 0;
}

/* marks for release fifo memory up to the passed in ptr which must be
   a valid location somewhere in the fragment buffer data (clearing up
   to the point), or if it is not in a valid data area, clearing the
   whole fifo. The reason we mark but do not yet release is that a
   page read can be immediately followed by handing that page to a
   logical stream, in which case the memory is once again locked
   in-buffer */

static void _sync_mark_fifo(_ogg_sync_state *oy,long bytes){
  
  oy->fifo_returned_pos+=bytes;
  oy->fifo_fill-=bytes;
  
  while(oy->fifo_returned_pos>=oy->fifo_returned->used){
    oy->fifo_returned_pos-=oy->fifo_returned->used;
    oy->fifo_returned=oy->fifo_returned->next;
  }
}

/* release up to previously marked point, taking into account
   higher-layer buffer locking */
static void _sync_release_fifo(_ogg_sync_state *oy){
  
  while(oy->fifo_tail!=oy->fifo_returned){
    fragmented_buffer *next=oy->fifo_tail->next;

    oy->fifo_tail->used=0;
    oy->fifo_tail->next=oy->unused_fifo;
    oy->unused_fifo=oy->fifo_tail;
    
    oy->fifo_tail=next;
    if(!oy->fifo_tail){
      oy->fifo_head=NULL;
      return;
    }
  }  
}

int ogg_page_version(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,4);
  return(_fc_get(&fc));
}

int ogg_page_continued(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,5);
  return(_fc_get(&fc)&0x01);
}

int ogg_page_bos(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,5);
  return(_fc_get(&fc)&0x02);
}

int ogg_page_eos(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,5);
  return(_fc_get(&fc)&0x04);
}

ogg_int64_t ogg_page_granulepos(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,6);
  return _fc_get_int64(&fc);
}

ogg_uint32_t ogg_page_serialno(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,14);
  return _fc_get_uint32(&fc);
}
 
ogg_uint32_t ogg_page_pageno(ogg_page *og){
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,18);
  return _fc_get_uint32(&fc);
}


/* returns the number of packets that are completed on this page (if
   the leading packet is begun on a previous page, but ends on this
   page, it's counted */

/* NOTE:
If a page consists of a packet begun on a previous page, and a new
packet begun (but not completed) on this page, the return will be:
  ogg_page_packets(page)   ==1, 
  ogg_page_continued(page) !=0

If a page happens to be a single packet that was begun on a
previous page, and spans to the next page (in the case of a three or
more page packet), the return will be: 
  ogg_page_packets(page)   ==0, 
  ogg_page_continued(page) !=0
*/

int ogg_page_packets(ogg_page *og){
  int i;
  int n;
  int count=0;
  fragmented_cursor fc;
  _fc_seek(&fc,&og->header,26);
  
  n=_fc_get(&fc);
  for(i=0;i<n;i++)
    if(_fc_get(&fc)<255)count++;
  return(count);
}

/* Static CRC calculation table.  See older code in CVS for dead
   run-time initialization code. */

static ogg_uint32_t crc_lookup[256]={
  0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,
  0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
  0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,
  0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
  0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,
  0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
  0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,
  0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
  0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,
  0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
  0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,
  0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
  0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,
  0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
  0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,
  0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
  0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,
  0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
  0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,
  0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
  0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,
  0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
  0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,
  0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
  0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,
  0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
  0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,
  0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
  0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,
  0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
  0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,
  0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
  0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,
  0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
  0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,
  0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
  0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,
  0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
  0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,
  0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
  0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,
  0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
  0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,
  0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
  0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,
  0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
  0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,
  0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
  0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,
  0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
  0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,
  0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
  0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,
  0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
  0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,
  0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
  0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,
  0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
  0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,
  0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
  0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,
  0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
  0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,
  0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4};

/* DECODING PRIMITIVES: raw stream and page layer *******************/

/* This has two layers (split page decode and packet decode) to place
   more of the multi-serialno and paging control in the hands of
   higher layers (eg, OggFile).  First, we expose a data buffer using
   ogg_sync_buffer().  The app either copies into the buffer, or
   passes it directly to read(), etc.  We then call ogg_sync_wrote()
   to tell how many bytes we just added.

   Efficiency note: request the same buffer size each time if at all
   possible.

   Pages are returned (pointers into the buffer in ogg_sync_state)
   by ogg_sync_pageout().  */

/* initialize the struct to a known state */

/* why the double indirects?
   allows us to clear the pointer on clear/destroy
   holds more compatability with libogg1 interface */

int ogg_sync_init(ogg_sync_state *oyi){
  if(oyi){
    _ogg_sync_state *oy=_ogg_calloc(1,sizeof(_ogg_sync_state));
    *oyi=(ogg_sync_state)oy;
    memset(oy,0,sizeof(*oy));
  }
  return(0);
}

/* clear non-flat storage within */
int ogg_sync_clear(ogg_sync_state *oyi){
  if(oyi){
    _ogg_sync_state *oy=(_ogg_sync_state *)*oyi;
    if(oy){
      fragmented_buffer *p=oy->unused_fifo;
      while(p){
	fragmented_buffer *next=p->next;
	if(p->data)_ogg_free(p->data);
	_ogg_free(p);
	p=next;
      }
      
      p=oy->fifo_tail;
      while(p){
	fragmented_buffer *next=p->next;
	if(p->data)_ogg_free(p->data);
	_ogg_free(p);
	p=next;
      }
      
      _ogg_free(oy);
      *oyi=0;
    }
  }
  return(0);
}

unsigned char *ogg_sync_buffer(ogg_sync_state *oyi, long bytes){
  _ogg_sync_state *oy=(_ogg_sync_state *)*oyi;
  /* this is the API point where fragments marked to be cleared from
     the fifo and not claimed by the stream layer are released */
  _sync_release_fifo(oy);
  return _sync_expose_fifo(oy,bytes);  
}

int ogg_sync_wrote(ogg_sync_state *oyi, long bytes){ 
  _ogg_sync_state *oy=(_ogg_sync_state *)*oyi;
  return _sync_wrote_fifo(oy,bytes);
}

/* sync the stream.  This is meant to be useful for finding page
   boundaries.

   return values for this:
  -n) skipped n bytes
   0) page not ready; more data (no bytes skipped)
   n) page synced at current location; page length n bytes
   
*/

long ogg_sync_pageseek(ogg_sync_state *oyi,ogg_page *og){
  _ogg_sync_state *oy=(_ogg_sync_state *)*oyi;
  fragmented_cursor         page;
  ogg_page                  temppage;
  long                      bytes;

  bytes=oy->fifo_fill;

  if(oy->headerbytes==0){
    if(bytes<27)goto out_more; /* not enough for even a minimal header */
    
    /* verify capture pattern */
    _fc_seek(&page,(fragmented_reference *)&oy->fifo_returned,
	     oy->fifo_returned_pos);
    if(_fc_get(&page)!=(int)'O' ||
       _fc_get(&page)!=(int)'g' ||
       _fc_get(&page)!=(int)'g' ||
       _fc_get(&page)!=(int)'S'    ) goto sync_fail;

    _fc_seek(&page,(fragmented_reference *)&oy->fifo_returned,
	     oy->fifo_returned_pos+26);
    oy->headerbytes=_fc_get(&page)+27;
  }
  if(bytes<oy->headerbytes)goto out_more; /* not enough for header +
                                             seg table */
    
  if(oy->bodybytes==0){
    int i;
    /* count up body length in the segment table */
    _fc_seek(&page,(fragmented_reference *)&oy->fifo_returned,
	     oy->fifo_returned_pos+27);
    for(i=0;i<oy->headerbytes-27;i++)
      oy->bodybytes+=_fc_get(&page);
  }
  
  if(oy->bodybytes+oy->headerbytes>bytes)goto out_more;
  
  /* The whole test page is buffered.  Set up the page struct and
     verify the checksum */
  {
    /* Grab the checksum bytes */
    unsigned char chksum[4];    
    _fc_seek(&page,(fragmented_reference *)&oy->fifo_returned,
	     oy->fifo_returned_pos+22);
    chksum[0]=_fc_get(&page);
    chksum[1]=_fc_get(&page);
    chksum[2]=_fc_get(&page);
    chksum[3]=_fc_get(&page);
    
    /* set up page struct and recompute the checksum */
    {
      fragmented_buffer *fb=oy->fifo_returned;
      int pos=oy->fifo_returned_pos;
      if(!og)og=&temppage;
      
      og->header.data =fb->data+pos;
      og->header.used =fb->used-pos;
      og->header.next =(fragmented_reference *)fb->next;
      og->header_len=oy->headerbytes;
      
      pos+=oy->headerbytes;
      while(pos>=fb->used){
	pos-=fb->used;
	fb=fb->next;
      }

      og->body.data =fb->data+pos;
      og->body.used =fb->used-pos;
      og->body.next =(fragmented_reference *)fb->next;
      og->body_len=oy->bodybytes;
    }
    
    ogg_page_checksum_set(og);
    
    /* Compare checksums */
    _fc_seek(&page,(fragmented_reference *)&oy->fifo_returned,
	     oy->fifo_returned_pos+22);
    if(chksum[0]!=_fc_get(&page) ||
       chksum[1]!=_fc_get(&page) ||
       chksum[2]!=_fc_get(&page) ||
       chksum[3]!=_fc_get(&page)){
      /* D'oh.  Mismatch! Corrupt page (or miscapture and not a page
	 at all). replace the computed checksum with the one actually
	 read in */
      _fc_seek(&page,(fragmented_reference *)&oy->fifo_returned,
	       oy->fifo_returned_pos+22);
      _fc_set(&page,chksum[0]);
      _fc_set(&page,chksum[1]);
      _fc_set(&page,chksum[2]);
      _fc_set(&page,chksum[3]);
      
      /* Bad checksum. Lose sync */
      goto sync_fail;
    }
  }
  
  /* yes, have a whole page all ready to go */
  {
    long bytes=oy->headerbytes+oy->bodybytes;

    oy->unsynced=0;
    oy->headerbytes=0;
    oy->bodybytes=0;

    /* chase the return position forward */
    _sync_mark_fifo(oy,bytes);

    return(bytes);
  }
  
 sync_fail:

  {
    long count=0;
    oy->headerbytes=0;
    oy->bodybytes=0;
    
    /* search forward through fragments for possible capture */
    while(oy->fifo_returned){
      unsigned char *next=memchr(oy->fifo_returned->data+
				 oy->fifo_returned_pos,
				 'O',
				 oy->fifo_returned->used-
				 oy->fifo_returned_pos);
      
      if(next){
	long bytes=next-oy->fifo_returned->data-oy->fifo_returned_pos;
	count+=bytes;
	oy->fifo_returned_pos+=bytes;
	break;
      }else{
	oy->fifo_returned_pos=0;
	oy->fifo_returned=oy->fifo_returned->next;
      }
    }
    return(-count);
  }

 out_more:
  return(0);
}

/* sync the stream and get a page.  Keep trying until we find a page.
   Supress 'sync errors' after reporting the first.

   return values:
   -1) recapture (hole in data)
    0) need more data
    1) page returned

   Returns pointers into buffered data; invalidated by next call to
   _stream, _clear, _init, or _buffer */

int ogg_sync_pageout(ogg_sync_state *oyi, ogg_page *og){
  _ogg_sync_state *oy=(_ogg_sync_state *)*oyi;

  /* all we need to do is verify a page at the head of the stream
     buffer.  If it doesn't verify, we look for the next potential
     frame */

  while(1){
    long ret=ogg_sync_pageseek(oyi,og);
    if(ret>0){
      /* have a page */
      return(1);
    }
    if(ret==0){
      /* need more data */
      return(0);
    }
    
    /* head did not start a synced page... skipped some bytes */
    if(!oy->unsynced){
      oy->unsynced=1;
      return(-1);
    }

    /* loop. keep looking */

  }
}

/* clear things to an initial state.  Good to call, eg, before seeking */
int ogg_sync_reset(ogg_sync_state *oyi){
  _ogg_sync_state *oy=(_ogg_sync_state *)*oyi;
  oy->fifo_returned=0;
  oy->fifo_returned_pos=0;
  _sync_release_fifo(oy);

  oy->unsynced=0;
  oy->headerbytes=0;
  oy->bodybytes=0;
  return(0);
}

/* checksum the page; direct table CRC */

void ogg_page_checksum_set(ogg_page *og){
  if(og){
    fragmented_cursor fc;
    fragmented_reference *fb;
    ogg_uint32_t crc_reg=0;
    int i,j;

    /* safety; needed for API behavior, but not framing code */
    _fc_seek(&fc,&og->header,22);
    _fc_set(&fc,0);
    _fc_set(&fc,0);
    _fc_set(&fc,0);
    _fc_set(&fc,0);

    i=og->header_len;
    fb=&og->header;
    while(i){
      for(j=0;j<i && j<fb->used;j++)
	crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)&0xff)^fb->data[j]];
      i-=j;
      fb=fb->next;
    }
    i=og->body_len;
    fb=&og->body;
    while(i){
      for(j=0;j<i && j<fb->used;j++)
	crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)&0xff)^fb->data[j]];
      i-=j;
      fb=fb->next;
    }
    
    _fc_seek(&fc,&og->header,22);
    _fc_set(&fc,crc_reg);
    _fc_set(&fc,crc_reg>>8);
    _fc_set(&fc,crc_reg>>16);
    _fc_set(&fc,crc_reg>>24);

  }
}

/* ENCODING PRIMITIVES: raw stream and page layer *******************/

/* The encode sync layer is only a convenience; it eliminates the need
   to parse the fragmented buffer data structure */

extern int ogg_page_getbuffer(ogg_page *og, unsigned char **buffer){
  int bytes;

  if(og->header_len){
    bytes=og->header.used;
    if(bytes>og->header_len)bytes=og->header_len;
    *buffer=og->header.data;
    og->header_len-=bytes;

    og->header.data=og->header.next->data;
    og->header.used=og->header.next->used;
    og->header.next=og->header.next->next;

    return(bytes);
  }
    
  if(og->body_len){
    bytes=og->body.used;
    if(bytes>og->body_len)bytes=og->body_len;
    *buffer=og->body.data;
    og->body_len-=bytes;

    og->body.data=og->body.next->data;
    og->body.used=og->body.next->used;
    og->body.next=og->body.next->next;

    return(bytes);
  }
  return(0);
}
