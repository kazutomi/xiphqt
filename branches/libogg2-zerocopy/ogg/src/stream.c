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

 function: code raw packets into framed Ogg stream and
           decode Ogg streams back into raw packets
 last mod: $Id: stream.c,v 1.1.2.2 2003/03/15 01:26:09 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "ogginternal.h" /* proper way to suck in ogg/ogg.h from a
			    libogg compile */

/* A complete description of Ogg framing exists in docs/framing.html */

int ogg_stream_init(ogg_stream_state *os,int serialno){
  if(os){
    memset(os,0,sizeof(*os));
    os->body_storage=16*1024;
    os->body_data=_ogg_malloc(os->body_storage*sizeof(*os->body_data));

    os->lacing_storage=1024;
    os->lacing_vals=_ogg_malloc(os->lacing_storage*sizeof(*os->lacing_vals));
    os->granule_vals=_ogg_malloc(os->lacing_storage*sizeof(*os->granule_vals));

    os->serialno=serialno;

    return(0);
  }
  return(-1);
} 

/* _clear does not free os, only the non-flat storage within */
int ogg_stream_clear(ogg_stream_state *os){
  if(os){
    if(os->body_data)_ogg_free(os->body_data);
    if(os->lacing_vals)_ogg_free(os->lacing_vals);
    if(os->granule_vals)_ogg_free(os->granule_vals);

    memset(os,0,sizeof(*os));    
  }
  return(0);
} 

/* submit data to the internal buffer of the framing engine */
int ogg_stream_packetin(ogg_stream_state *os,ogg_packet *op){
  int lacing_vals=op->bytes/255+1,i;

  if(os->body_returned){
    /* advance packet data according to the body_returned pointer. We
       had to keep it around to return a pointer into the buffer last
       call */
    
    os->body_fill-=os->body_returned;
    if(os->body_fill)
      memmove(os->body_data,os->body_data+os->body_returned,
	      os->body_fill);
    os->body_returned=0;
  }
 
  /* make sure we have the buffer storage */
  _os_body_expand(os,op->bytes);
  _os_lacing_expand(os,lacing_vals);

  /* Copy in the submitted packet.  Yes, the copy is a waste; this is
     the liability of overly clean abstraction for the time being.  It
     will actually be fairly easy to eliminate the extra copy in the
     future */

  memcpy(os->body_data+os->body_fill,op->packet,op->bytes);
  os->body_fill+=op->bytes;

  /* Store lacing vals for this packet */
  for(i=0;i<lacing_vals-1;i++){
    os->lacing_vals[os->lacing_fill+i]=255;
    os->granule_vals[os->lacing_fill+i]=os->granulepos;
  }
  os->lacing_vals[os->lacing_fill+i]=(op->bytes)%255;
  os->granulepos=os->granule_vals[os->lacing_fill+i]=op->granulepos;

  /* flag the first segment as the beginning of the packet */
  os->lacing_vals[os->lacing_fill]|= 0x100;

  os->lacing_fill+=lacing_vals;

  /* for the sake of completeness */
  os->packetno++;

  if(op->e_o_s)os->e_o_s=1;

  return(0);
}

/* This will flush remaining packets into a page (returning nonzero),
   even if there is not enough data to trigger a flush normally
   (undersized page). If there are no packets or partial packets to
   flush, ogg_stream_flush returns 0.  Note that ogg_stream_flush will
   try to flush a normal sized page like ogg_stream_pageout; a call to
   ogg_stream_flush does not guarantee that all packets have flushed.
   Only a return value of 0 from ogg_stream_flush indicates all packet
   data is flushed into pages.

   since ogg_stream_flush will flush the last page in a stream even if
   it's undersized, you almost certainly want to use ogg_stream_pageout
   (and *not* ogg_stream_flush) unless you specifically need to flush 
   an page regardless of size in the middle of a stream. */

int ogg_stream_flush(ogg_stream_state *os,ogg_page *og){
  int i;
  int vals=0;
  int maxvals=(os->lacing_fill>255?255:os->lacing_fill);
  int bytes=0;
  long acc=0;
  ogg_int64_t granule_pos=os->granule_vals[0];

  if(maxvals==0)return(0);
  
  /* construct a page */
  /* decide how many segments to include */
  
  /* If this is the initial header case, the first page must only include
     the initial header packet */
  if(os->b_o_s==0){  /* 'initial header page' case */
    granule_pos=0;
    for(vals=0;vals<maxvals;vals++){
      if((os->lacing_vals[vals]&0x0ff)<255){
	vals++;
	break;
      }
    }
  }else{
    for(vals=0;vals<maxvals;vals++){
      if(acc>4096)break;
      acc+=os->lacing_vals[vals]&0x0ff;
      granule_pos=os->granule_vals[vals];
    }
  }
  
  /* construct the header in temp storage */
  memcpy(os->header,"OggS",4);
  
  /* stream structure version */
  os->header[4]=0x00;
  
  /* continued packet flag? */
  os->header[5]=0x00;
  if((os->lacing_vals[0]&0x100)==0)os->header[5]|=0x01;
  /* first page flag? */
  if(os->b_o_s==0)os->header[5]|=0x02;
  /* last page flag? */
  if(os->e_o_s && os->lacing_fill==vals)os->header[5]|=0x04;
  os->b_o_s=1;

  /* 64 bits of PCM position */
  for(i=6;i<14;i++){
    os->header[i]=(granule_pos&0xff);
    granule_pos>>=8;
  }

  /* 32 bits of stream serial number */
  {
    long serialno=os->serialno;
    for(i=14;i<18;i++){
      os->header[i]=(serialno&0xff);
      serialno>>=8;
    }
  }

  /* 32 bits of page counter (we have both counter and page header
     because this val can roll over) */
  if(os->pageno==-1)os->pageno=0; /* because someone called
				     stream_reset; this would be a
				     strange thing to do in an
				     encode stream, but it has
				     plausible uses */
  {
    long pageno=os->pageno++;
    for(i=18;i<22;i++){
      os->header[i]=(pageno&0xff);
      pageno>>=8;
    }
  }
  
  /* zero for computation; filled in later */
  os->header[22]=0;
  os->header[23]=0;
  os->header[24]=0;
  os->header[25]=0;
  
  /* segment table */
  os->header[26]=vals&0xff;
  for(i=0;i<vals;i++)
    bytes+=os->header[i+27]=(os->lacing_vals[i]&0xff);
  
  /* set pointers in the ogg_page struct */
  og->header=os->header;
  og->header_len=os->header_fill=vals+27;
  og->body=os->body_data+os->body_returned;
  og->body_len=bytes;
  
  /* advance the lacing data and set the body_returned pointer */
  
  os->lacing_fill-=vals;
  memmove(os->lacing_vals,os->lacing_vals+vals,os->lacing_fill*sizeof(*os->lacing_vals));
  memmove(os->granule_vals,os->granule_vals+vals,os->lacing_fill*sizeof(*os->granule_vals));
  os->body_returned+=bytes;
  
  /* calculate the checksum */
  
  ogg_page_checksum_set(og);

  /* done */
  return(1);
}


/* This constructs pages from buffered packet segments.  The pointers
returned are to static buffers; do not free. The returned buffers are
good only until the next call (using the same ogg_stream_state) */

int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og){

  if((os->e_o_s&&os->lacing_fill) ||          /* 'were done, now flush' case */
     os->body_fill-os->body_returned > 4096 ||/* 'page nominal size' case */
     os->lacing_fill>=255 ||                  /* 'segment table full' case */
     (os->lacing_fill&&!os->b_o_s)){          /* 'initial header page' case */
        
    return(ogg_stream_flush(os,og));
  }
  
  /* not enough data to construct a page and not end of stream */
  return(0);
}

int ogg_stream_eos(ogg_stream_state *os){
  return os->e_o_s;
}

/* DECODING PRIMITIVES: packet streaming layer **********************/

/* add the incoming page to the stream state; we decompose the page
   into packet segments here as well. */

int ogg_stream_pagein(ogg_stream_state *os, ogg_page *og){
  unsigned char *header=og->header;
  unsigned char *body=og->body;
  long           bodysize=og->body_len;
  int            segptr=0;

  int version=ogg_page_version(og);
  int continued=ogg_page_continued(og);
  int bos=ogg_page_bos(og);
  int eos=ogg_page_eos(og);
  ogg_int64_t granulepos=ogg_page_granulepos(og);
  int serialno=ogg_page_serialno(og);
  long pageno=ogg_page_pageno(og);
  int segments=header[26];
  
  /* clean up 'returned data' */
  {
    long lr=os->lacing_returned;
    long br=os->body_returned;

    /* body data */
    if(br){
      os->body_fill-=br;
      if(os->body_fill)
	memmove(os->body_data,os->body_data+br,os->body_fill);
      os->body_returned=0;
    }

    if(lr){
      /* segment table */
      if(os->lacing_fill-lr){
	memmove(os->lacing_vals,os->lacing_vals+lr,
		(os->lacing_fill-lr)*sizeof(*os->lacing_vals));
	memmove(os->granule_vals,os->granule_vals+lr,
		(os->lacing_fill-lr)*sizeof(*os->granule_vals));
      }
      os->lacing_fill-=lr;
      os->lacing_packet-=lr;
      os->lacing_returned=0;
    }
  }

  /* check the serial number */
  if(serialno!=os->serialno)return(-1);
  if(version>0)return(-1);

  _os_lacing_expand(os,segments+1);

  /* are we in sequence? */
  if(pageno!=os->pageno){
    int i;

    /* unroll previous partial packet (if any) */
    for(i=os->lacing_packet;i<os->lacing_fill;i++)
      os->body_fill-=os->lacing_vals[i]&0xff;
    os->lacing_fill=os->lacing_packet;

    /* make a note of dropped data in segment table */
    if(os->pageno!=-1){
      os->lacing_vals[os->lacing_fill++]=0x400;
      os->lacing_packet++;
    }

    /* are we a 'continued packet' page?  If so, we'll need to skip
       some segments */
    if(continued){
      bos=0;
      for(;segptr<segments;segptr++){
	int val=header[27+segptr];
	body+=val;
	bodysize-=val;
	if(val<255){
	  segptr++;
	  break;
	}
      }
    }
  }
  
  if(bodysize){
    _os_body_expand(os,bodysize);
    memcpy(os->body_data+os->body_fill,body,bodysize);
    os->body_fill+=bodysize;
  }

  {
    int saved=-1;
    while(segptr<segments){
      int val=header[27+segptr];
      os->lacing_vals[os->lacing_fill]=val;
      os->granule_vals[os->lacing_fill]=-1;
      
      if(bos){
	os->lacing_vals[os->lacing_fill]|=0x100;
	bos=0;
      }
      
      if(val<255)saved=os->lacing_fill;
      
      os->lacing_fill++;
      segptr++;
      
      if(val<255)os->lacing_packet=os->lacing_fill;
    }
  
    /* set the granulepos on the last granuleval of the last full packet */
    if(saved!=-1){
      os->granule_vals[saved]=granulepos;
    }

  }

  if(eos){
    os->e_o_s=1;
    if(os->lacing_fill>0)
      os->lacing_vals[os->lacing_fill-1]|=0x200;
  }

  os->pageno=pageno+1;

  return(0);
}

int ogg_stream_reset(ogg_stream_state *os){
  os->body_fill=0;
  os->body_returned=0;

  os->lacing_fill=0;
  os->lacing_packet=0;
  os->lacing_returned=0;

  os->header_fill=0;

  os->e_o_s=0;
  os->b_o_s=0;
  os->pageno=-1;
  os->packetno=0;
  os->granulepos=0;

  return(0);
}

int ogg_stream_reset_serialno(ogg_stream_state *os,int serialno){
  ogg_stream_reset(os);
  os->serialno=serialno;
  return(0);
}

static int _packetout(ogg_stream_state *os,ogg_packet *op,int adv){

  /* The last part of decode. We have the stream broken into packet
     segments.  Now we need to group them into packets (or return the
     out of sync markers) */

  int ptr=os->lacing_returned;

  if(os->lacing_packet<=ptr)return(0);

  if(os->lacing_vals[ptr]&0x400){
    /* we need to tell the codec there's a gap; it might need to
       handle previous packet dependencies. */
    os->lacing_returned++;
    os->packetno++;
    return(-1);
  }

  if(!op && !adv)return(1); /* just using peek as an inexpensive way
                               to ask if there's a whole packet
                               waiting */

  /* Gather the whole packet. We'll have no holes or a partial packet */
  {
    int size=os->lacing_vals[ptr]&0xff;
    int bytes=size;
    int eos=os->lacing_vals[ptr]&0x200; /* last packet of the stream? */
    int bos=os->lacing_vals[ptr]&0x100; /* first packet of the stream? */

    while(size==255){
      int val=os->lacing_vals[++ptr];
      size=val&0xff;
      if(val&0x200)eos=0x200;
      bytes+=size;
    }

    if(op){
      op->e_o_s=eos;
      op->b_o_s=bos;
      op->packet=os->body_data+os->body_returned;
      op->packetno=os->packetno;
      op->granulepos=os->granule_vals[ptr];
      op->bytes=bytes;
    }

    if(adv){
      os->body_returned+=bytes;
      os->lacing_returned=ptr+1;
      os->packetno++;
    }
  }
  return(1);
}

int ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op){
  return _packetout(os,op,1);
}

int ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op){
  return _packetout(os,op,0);
}

void ogg_packet_clear(ogg_packet *op) {
  _ogg_free(op->packet);
  memset(op, 0, sizeof(*op));
}

#ifdef _V_SELFTEST
#include <stdio.h>

ogg_stream_state os_en, os_de;
ogg_sync_state oy;

void checkpacket(ogg_packet *op,int len, int no, int pos){
  long j;
  static int sequence=0;
  static int lastno=0;

  if(op->bytes!=len){
    fprintf(stderr,"incorrect packet length!\n");
    exit(1);
  }
  if(op->granulepos!=pos){
    fprintf(stderr,"incorrect packet position!\n");
    exit(1);
  }

  /* packet number just follows sequence/gap; adjust the input number
     for that */
  if(no==0){
    sequence=0;
  }else{
    sequence++;
    if(no>lastno+1)
      sequence++;
  }
  lastno=no;
  if(op->packetno!=sequence){
    fprintf(stderr,"incorrect packet sequence %ld != %d\n",
	    (long)(op->packetno),sequence);
    exit(1);
  }

  /* Test data */
  for(j=0;j<op->bytes;j++)
    if(op->packet[j]!=((j+no)&0xff)){
      fprintf(stderr,"body data mismatch (1) at pos %ld: %x!=%lx!\n\n",
	      j,op->packet[j],(j+no)&0xff);
      exit(1);
    }
}

void check_page(unsigned char *data,const int *header,ogg_page *og){
  long j;
  /* Test data */
  for(j=0;j<og->body_len;j++)
    if(og->body[j]!=data[j]){
      fprintf(stderr,"body data mismatch (2) at pos %ld: %x!=%x!\n\n",
	      j,data[j],og->body[j]);
      exit(1);
    }

  /* Test header */
  for(j=0;j<og->header_len;j++){
    if(og->header[j]!=header[j]){
      fprintf(stderr,"header content mismatch at pos %ld:\n",j);
      for(j=0;j<header[26]+27;j++)
	fprintf(stderr," (%ld)%02x:%02x",j,header[j],og->header[j]);
      fprintf(stderr,"\n");
      exit(1);
    }
  }
  if(og->header_len!=header[26]+27){
    fprintf(stderr,"header length incorrect! (%ld!=%d)\n",
	    og->header_len,header[26]+27);
    exit(1);
  }
}

void print_header(ogg_page *og){
  int j;
  fprintf(stderr,"\nHEADER:\n");
  fprintf(stderr,"  capture: %c %c %c %c  version: %d  flags: %x\n",
	  og->header[0],og->header[1],og->header[2],og->header[3],
	  (int)og->header[4],(int)og->header[5]);

  fprintf(stderr,"  granulepos: %d  serialno: %d  pageno: %ld\n",
	  (og->header[9]<<24)|(og->header[8]<<16)|
	  (og->header[7]<<8)|og->header[6],
	  (og->header[17]<<24)|(og->header[16]<<16)|
	  (og->header[15]<<8)|og->header[14],
	  ((long)(og->header[21])<<24)|(og->header[20]<<16)|
	  (og->header[19]<<8)|og->header[18]);

  fprintf(stderr,"  checksum: %02x:%02x:%02x:%02x\n  segments: %d (",
	  (int)og->header[22],(int)og->header[23],
	  (int)og->header[24],(int)og->header[25],
	  (int)og->header[26]);

  for(j=27;j<og->header_len;j++)
    fprintf(stderr,"%d ",(int)og->header[j]);
  fprintf(stderr,")\n\n");
}

void copy_page(ogg_page *og){
  unsigned char *temp=_ogg_malloc(og->header_len);
  memcpy(temp,og->header,og->header_len);
  og->header=temp;

  temp=_ogg_malloc(og->body_len);
  memcpy(temp,og->body,og->body_len);
  og->body=temp;
}

void error(void){
  fprintf(stderr,"error!\n");
  exit(1);
}

/* 17 only */
const int head1_0[] = {0x4f,0x67,0x67,0x53,0,0x06,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0x15,0xed,0xec,0x91,
		       1,
		       17};

/* 17, 254, 255, 256, 500, 510, 600 byte, pad */
const int head1_1[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0x59,0x10,0x6c,0x2c,
		       1,
		       17};
const int head2_1[] = {0x4f,0x67,0x67,0x53,0,0x04,
		       0x07,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0x89,0x33,0x85,0xce,
		       13,
		       254,255,0,255,1,255,245,255,255,0,
		       255,255,90};

/* nil packets; beginning,middle,end */
const int head1_2[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0xff,0x7b,0x23,0x17,
		       1,
		       0};
const int head2_2[] = {0x4f,0x67,0x67,0x53,0,0x04,
		       0x07,0x28,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0x5c,0x3f,0x66,0xcb,
		       17,
		       17,254,255,0,0,255,1,0,255,245,255,255,0,
		       255,255,90,0};

/* large initial packet */
const int head1_3[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0x01,0x27,0x31,0xaa,
		       18,
		       255,255,255,255,255,255,255,255,
		       255,255,255,255,255,255,255,255,255,10};

const int head2_3[] = {0x4f,0x67,0x67,0x53,0,0x04,
		       0x07,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0x7f,0x4e,0x8a,0xd2,
		       4,
		       255,4,255,0};


/* continuing packet test */
const int head1_4[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0xff,0x7b,0x23,0x17,
		       1,
		       0};

const int head2_4[] = {0x4f,0x67,0x67,0x53,0,0x00,
		       0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0x34,0x24,0xd5,0x29,
		       17,
		       255,255,255,255,255,255,255,255,
		       255,255,255,255,255,255,255,255,255};

const int head3_4[] = {0x4f,0x67,0x67,0x53,0,0x05,
		       0x07,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,2,0,0,0,
		       0xc8,0xc3,0xcb,0xed,
		       5,
		       10,255,4,255,0};


/* page with the 255 segment limit */
const int head1_5[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0xff,0x7b,0x23,0x17,
		       1,
		       0};

const int head2_5[] = {0x4f,0x67,0x67,0x53,0,0x00,
		       0x07,0xfc,0x03,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0xed,0x2a,0x2e,0xa7,
		       255,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10,10,
		       10,10,10,10,10,10,10};

const int head3_5[] = {0x4f,0x67,0x67,0x53,0,0x04,
		       0x07,0x00,0x04,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,2,0,0,0,
		       0x6c,0x3b,0x82,0x3d,
		       1,
		       50};


/* packet that overspans over an entire page */
const int head1_6[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0xff,0x7b,0x23,0x17,
		       1,
		       0};

const int head2_6[] = {0x4f,0x67,0x67,0x53,0,0x00,
		       0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0x3c,0xd9,0x4d,0x3f,
		       17,
		       100,255,255,255,255,255,255,255,255,
		       255,255,255,255,255,255,255,255};

const int head3_6[] = {0x4f,0x67,0x67,0x53,0,0x01,
		       0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,2,0,0,0,
		       0xbd,0xd5,0xb5,0x8b,
		       17,
		       255,255,255,255,255,255,255,255,
		       255,255,255,255,255,255,255,255,255};

const int head4_6[] = {0x4f,0x67,0x67,0x53,0,0x05,
		       0x07,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,3,0,0,0,
		       0xef,0xdd,0x88,0xde,
		       7,
		       255,255,75,255,4,255,0};

/* packet that overspans over an entire page */
const int head1_7[] = {0x4f,0x67,0x67,0x53,0,0x02,
		       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,0,0,0,0,
		       0xff,0x7b,0x23,0x17,
		       1,
		       0};

const int head2_7[] = {0x4f,0x67,0x67,0x53,0,0x00,
		       0x07,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,1,0,0,0,
		       0x3c,0xd9,0x4d,0x3f,
		       17,
		       100,255,255,255,255,255,255,255,255,
		       255,255,255,255,255,255,255,255};

const int head3_7[] = {0x4f,0x67,0x67,0x53,0,0x05,
		       0x07,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
		       0x01,0x02,0x03,0x04,2,0,0,0,
		       0xd4,0xe0,0x60,0xe5,
		       1,0};

void test_pack(const int *pl, const int **headers){
  unsigned char *data=_ogg_malloc(1024*1024); /* for scripted test cases only */
  long inptr=0;
  long outptr=0;
  long deptr=0;
  long depacket=0;
  long granule_pos=7,pageno=0;
  int i,j,packets,pageout=0;
  int eosflag=0;
  int bosflag=0;

  ogg_stream_reset(&os_en);
  ogg_stream_reset(&os_de);
  ogg_sync_reset(&oy);

  for(packets=0;;packets++)if(pl[packets]==-1)break;

  for(i=0;i<packets;i++){
    /* construct a test packet */
    ogg_packet op;
    int len=pl[i];
    
    op.packet=data+inptr;
    op.bytes=len;
    op.e_o_s=(pl[i+1]<0?1:0);
    op.granulepos=granule_pos;

    granule_pos+=1024;

    for(j=0;j<len;j++)data[inptr++]=i+j;

    /* submit the test packet */
    ogg_stream_packetin(&os_en,&op);

    /* retrieve any finished pages */
    {
      ogg_page og;
      
      while(ogg_stream_pageout(&os_en,&og)){
	/* We have a page.  Check it carefully */

	fprintf(stderr,"%ld, ",pageno);

	if(headers[pageno]==NULL){
	  fprintf(stderr,"coded too many pages!\n");
	  exit(1);
	}

	check_page(data+outptr,headers[pageno],&og);

	outptr+=og.body_len;
	pageno++;

	/* have a complete page; submit it to sync/decode */

	{
	  ogg_page og_de;
	  ogg_packet op_de,op_de2;
	  char *buf=ogg_sync_buffer(&oy,og.header_len+og.body_len);
	  memcpy(buf,og.header,og.header_len);
	  memcpy(buf+og.header_len,og.body,og.body_len);
	  ogg_sync_wrote(&oy,og.header_len+og.body_len);

	  while(ogg_sync_pageout(&oy,&og_de)>0){
	    /* got a page.  Happy happy.  Verify that it's good. */
	    
	    check_page(data+deptr,headers[pageout],&og_de);
	    deptr+=og_de.body_len;
	    pageout++;

	    /* submit it to deconstitution */
	    ogg_stream_pagein(&os_de,&og_de);

	    /* packets out? */
	    while(ogg_stream_packetpeek(&os_de,&op_de2)>0){
	      ogg_stream_packetpeek(&os_de,NULL);
	      ogg_stream_packetout(&os_de,&op_de); /* just catching them all */
	      
	      /* verify peek and out match */
	      if(memcmp(&op_de,&op_de2,sizeof(op_de))){
		fprintf(stderr,"packetout != packetpeek! pos=%ld\n",
			depacket);
		exit(1);
	      }

	      /* verify the packet! */
	      /* check data */
	      if(memcmp(data+depacket,op_de.packet,op_de.bytes)){
		fprintf(stderr,"packet data mismatch in decode! pos=%ld\n",
			depacket);
		exit(1);
	      }
	      /* check bos flag */
	      if(bosflag==0 && op_de.b_o_s==0){
		fprintf(stderr,"b_o_s flag not set on packet!\n");
		exit(1);
	      }
	      if(bosflag && op_de.b_o_s){
		fprintf(stderr,"b_o_s flag incorrectly set on packet!\n");
		exit(1);
	      }
	      bosflag=1;
	      depacket+=op_de.bytes;
	      
	      /* check eos flag */
	      if(eosflag){
		fprintf(stderr,"Multiple decoded packets with eos flag!\n");
		exit(1);
	      }

	      if(op_de.e_o_s)eosflag=1;

	      /* check granulepos flag */
	      if(op_de.granulepos!=-1){
		fprintf(stderr," granule:%ld ",(long)op_de.granulepos);
	      }
	    }
	  }
	}
      }
    }
  }
  _ogg_free(data);
  if(headers[pageno]!=NULL){
    fprintf(stderr,"did not write last page!\n");
    exit(1);
  }
  if(headers[pageout]!=NULL){
    fprintf(stderr,"did not decode last page!\n");
    exit(1);
  }
  if(inptr!=outptr){
    fprintf(stderr,"encoded page data incomplete!\n");
    exit(1);
  }
  if(inptr!=deptr){
    fprintf(stderr,"decoded page data incomplete!\n");
    exit(1);
  }
  if(inptr!=depacket){
    fprintf(stderr,"decoded packet data incomplete!\n");
    exit(1);
  }
  if(!eosflag){
    fprintf(stderr,"Never got a packet with EOS set!\n");
    exit(1);
  }
  fprintf(stderr,"ok.\n");
}

int main(void){

  ogg_stream_init(&os_en,0x04030201);
  ogg_stream_init(&os_de,0x04030201);
  ogg_sync_init(&oy);

  /* Exercise each code path in the framing code.  Also verify that
     the checksums are working.  */

  {
    /* 17 only */
    const int packets[]={17, -1};
    const int *headret[]={head1_0,NULL};
    
    fprintf(stderr,"testing single page encoding... ");
    test_pack(packets,headret);
  }

  {
    /* 17, 254, 255, 256, 500, 510, 600 byte, pad */
    const int packets[]={17, 254, 255, 256, 500, 510, 600, -1};
    const int *headret[]={head1_1,head2_1,NULL};
    
    fprintf(stderr,"testing basic page encoding... ");
    test_pack(packets,headret);
  }

  {
    /* nil packets; beginning,middle,end */
    const int packets[]={0,17, 254, 255, 0, 256, 0, 500, 510, 600, 0, -1};
    const int *headret[]={head1_2,head2_2,NULL};
    
    fprintf(stderr,"testing basic nil packets... ");
    test_pack(packets,headret);
  }

  {
    /* large initial packet */
    const int packets[]={4345,259,255,-1};
    const int *headret[]={head1_3,head2_3,NULL};
    
    fprintf(stderr,"testing initial-packet lacing > 4k... ");
    test_pack(packets,headret);
  }

  {
    /* continuing packet test */
    const int packets[]={0,4345,259,255,-1};
    const int *headret[]={head1_4,head2_4,head3_4,NULL};
    
    fprintf(stderr,"testing single packet page span... ");
    test_pack(packets,headret);
  }

  /* page with the 255 segment limit */
  {

    const int packets[]={0,10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,50,-1};
    const int *headret[]={head1_5,head2_5,head3_5,NULL};
    
    fprintf(stderr,"testing max packet segments... ");
    test_pack(packets,headret);
  }

  {
    /* packet that overspans over an entire page */
    const int packets[]={0,100,9000,259,255,-1};
    const int *headret[]={head1_6,head2_6,head3_6,head4_6,NULL};
    
    fprintf(stderr,"testing very large packets... ");
    test_pack(packets,headret);
  }

  {
    /* term only page.  why not? */
    const int packets[]={0,100,4080,-1};
    const int *headret[]={head1_7,head2_7,head3_7,NULL};
    
    fprintf(stderr,"testing zero data page (1 nil packet)... ");
    test_pack(packets,headret);
  }



  {
    /* build a bunch of pages for testing */
    unsigned char *data=_ogg_malloc(1024*1024);
    int pl[]={0,100,4079,2956,2057,76,34,912,0,234,1000,1000,1000,300,-1};
    int inptr=0,i,j;
    ogg_page og[5];
    
    ogg_stream_reset(&os_en);

    for(i=0;pl[i]!=-1;i++){
      ogg_packet op;
      int len=pl[i];
      
      op.packet=data+inptr;
      op.bytes=len;
      op.e_o_s=(pl[i+1]<0?1:0);
      op.granulepos=(i+1)*1000;

      for(j=0;j<len;j++)data[inptr++]=i+j;
      ogg_stream_packetin(&os_en,&op);
    }

    _ogg_free(data);

    /* retrieve finished pages */
    for(i=0;i<5;i++){
      if(ogg_stream_pageout(&os_en,&og[i])==0){
	fprintf(stderr,"Too few pages output building sync tests!\n");
	exit(1);
      }
      copy_page(&og[i]);
    }

    /* Test lost pages on pagein/packetout: no rollback */
    {
      ogg_page temp;
      ogg_packet test;

      fprintf(stderr,"Testing loss of pages... ");

      ogg_sync_reset(&oy);
      ogg_stream_reset(&os_de);
      for(i=0;i<5;i++){
	memcpy(ogg_sync_buffer(&oy,og[i].header_len),og[i].header,
	       og[i].header_len);
	ogg_sync_wrote(&oy,og[i].header_len);
	memcpy(ogg_sync_buffer(&oy,og[i].body_len),og[i].body,og[i].body_len);
	ogg_sync_wrote(&oy,og[i].body_len);
      }

      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);
      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);
      ogg_sync_pageout(&oy,&temp);
      /* skip */
      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);

      /* do we get the expected results/packets? */
      
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,0,0,0);
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,100,1,-1);
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,4079,2,3000);
      if(ogg_stream_packetout(&os_de,&test)!=-1){
	fprintf(stderr,"Error: loss of page did not return error\n");
	exit(1);
      }
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,76,5,-1);
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,34,6,-1);
      fprintf(stderr,"ok.\n");
    }

    /* Test lost pages on pagein/packetout: rollback with continuation */
    {
      ogg_page temp;
      ogg_packet test;

      fprintf(stderr,"Testing loss of pages (rollback required)... ");

      ogg_sync_reset(&oy);
      ogg_stream_reset(&os_de);
      for(i=0;i<5;i++){
	memcpy(ogg_sync_buffer(&oy,og[i].header_len),og[i].header,
	       og[i].header_len);
	ogg_sync_wrote(&oy,og[i].header_len);
	memcpy(ogg_sync_buffer(&oy,og[i].body_len),og[i].body,og[i].body_len);
	ogg_sync_wrote(&oy,og[i].body_len);
      }

      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);
      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);
      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);
      ogg_sync_pageout(&oy,&temp);
      /* skip */
      ogg_sync_pageout(&oy,&temp);
      ogg_stream_pagein(&os_de,&temp);

      /* do we get the expected results/packets? */
      
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,0,0,0);
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,100,1,-1);
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,4079,2,3000);
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,2956,3,4000);
      if(ogg_stream_packetout(&os_de,&test)!=-1){
	fprintf(stderr,"Error: loss of page did not return error\n");
	exit(1);
      }
      if(ogg_stream_packetout(&os_de,&test)!=1)error();
      checkpacket(&test,300,13,14000);
      fprintf(stderr,"ok.\n");
    }
    
    /* the rest only test sync */
    {
      ogg_page og_de;
      /* Test fractional page inputs: incomplete capture */
      fprintf(stderr,"Testing sync on partial inputs... ");
      ogg_sync_reset(&oy);
      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header,
	     3);
      ogg_sync_wrote(&oy,3);
      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      
      /* Test fractional page inputs: incomplete fixed header */
      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header+3,
	     20);
      ogg_sync_wrote(&oy,20);
      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      
      /* Test fractional page inputs: incomplete header */
      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header+23,
	     5);
      ogg_sync_wrote(&oy,5);
      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      
      /* Test fractional page inputs: incomplete body */
      
      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header+28,
	     og[1].header_len-28);
      ogg_sync_wrote(&oy,og[1].header_len-28);
      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      
      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body,1000);
      ogg_sync_wrote(&oy,1000);
      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      
      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body+1000,
	     og[1].body_len-1000);
      ogg_sync_wrote(&oy,og[1].body_len-1000);
      if(ogg_sync_pageout(&oy,&og_de)<=0)error();
      
      fprintf(stderr,"ok.\n");
    }

    /* Test fractional page inputs: page + incomplete capture */
    {
      ogg_page og_de;
      fprintf(stderr,"Testing sync on 1+partial inputs... ");
      ogg_sync_reset(&oy); 

      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header,
	     og[1].header_len);
      ogg_sync_wrote(&oy,og[1].header_len);

      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body,
	     og[1].body_len);
      ogg_sync_wrote(&oy,og[1].body_len);

      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header,
	     20);
      ogg_sync_wrote(&oy,20);
      if(ogg_sync_pageout(&oy,&og_de)<=0)error();
      if(ogg_sync_pageout(&oy,&og_de)>0)error();

      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header+20,
	     og[1].header_len-20);
      ogg_sync_wrote(&oy,og[1].header_len-20);
      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body,
	     og[1].body_len);
      ogg_sync_wrote(&oy,og[1].body_len);
      if(ogg_sync_pageout(&oy,&og_de)<=0)error();

      fprintf(stderr,"ok.\n");
    }
    
    /* Test recapture: garbage + page */
    {
      ogg_page og_de;
      fprintf(stderr,"Testing search for capture... ");
      ogg_sync_reset(&oy); 
      
      /* 'garbage' */
      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body,
	     og[1].body_len);
      ogg_sync_wrote(&oy,og[1].body_len);

      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header,
	     og[1].header_len);
      ogg_sync_wrote(&oy,og[1].header_len);

      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body,
	     og[1].body_len);
      ogg_sync_wrote(&oy,og[1].body_len);

      memcpy(ogg_sync_buffer(&oy,og[2].header_len),og[2].header,
	     20);
      ogg_sync_wrote(&oy,20);
      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      if(ogg_sync_pageout(&oy,&og_de)<=0)error();
      if(ogg_sync_pageout(&oy,&og_de)>0)error();

      memcpy(ogg_sync_buffer(&oy,og[2].header_len),og[2].header+20,
	     og[2].header_len-20);
      ogg_sync_wrote(&oy,og[2].header_len-20);
      memcpy(ogg_sync_buffer(&oy,og[2].body_len),og[2].body,
	     og[2].body_len);
      ogg_sync_wrote(&oy,og[2].body_len);
      if(ogg_sync_pageout(&oy,&og_de)<=0)error();

      fprintf(stderr,"ok.\n");
    }

    /* Test recapture: page + garbage + page */
    {
      ogg_page og_de;
      fprintf(stderr,"Testing recapture... ");
      ogg_sync_reset(&oy); 

      memcpy(ogg_sync_buffer(&oy,og[1].header_len),og[1].header,
	     og[1].header_len);
      ogg_sync_wrote(&oy,og[1].header_len);

      memcpy(ogg_sync_buffer(&oy,og[1].body_len),og[1].body,
	     og[1].body_len);
      ogg_sync_wrote(&oy,og[1].body_len);

      memcpy(ogg_sync_buffer(&oy,og[2].header_len),og[2].header,
	     og[2].header_len);
      ogg_sync_wrote(&oy,og[2].header_len);

      memcpy(ogg_sync_buffer(&oy,og[2].header_len),og[2].header,
	     og[2].header_len);
      ogg_sync_wrote(&oy,og[2].header_len);

      if(ogg_sync_pageout(&oy,&og_de)<=0)error();

      memcpy(ogg_sync_buffer(&oy,og[2].body_len),og[2].body,
	     og[2].body_len-5);
      ogg_sync_wrote(&oy,og[2].body_len-5);

      memcpy(ogg_sync_buffer(&oy,og[3].header_len),og[3].header,
	     og[3].header_len);
      ogg_sync_wrote(&oy,og[3].header_len);

      memcpy(ogg_sync_buffer(&oy,og[3].body_len),og[3].body,
	     og[3].body_len);
      ogg_sync_wrote(&oy,og[3].body_len);

      if(ogg_sync_pageout(&oy,&og_de)>0)error();
      if(ogg_sync_pageout(&oy,&og_de)<=0)error();

      fprintf(stderr,"ok.\n");
    }
  }    

  return(0);
}

#endif




