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

  function: pack variable sized words into an octet stream
  last mod: $Id: bitwise.c,v 1.14.2.5 2003/02/03 23:01:47 xiphmont Exp $

 ********************************************************************/

/* the 'oggpack_xxx functions are 'LSb' endian; if we write a word but
   read individual bits, then we'll read the lsb first */
/* the 'oggpackB_xxx functions are 'MSb' endian; if we write a word but
   read individual bits, then we'll read the msb first */

#include <string.h>
#include <stdlib.h>
#include "ogginternal.h"

#ifdef _V_SELFTEST
#define OGGPACK_CHUNKSIZE 4
#else
#define OGGPACK_CHUNKSIZE 128
#endif

static unsigned long mask[]=
{0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
 0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
 0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
 0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
 0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
 0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
 0x3fffffff,0x7fffffff,0xffffffff };

void oggpack_writeinit(oggpack_buffer *b,ogg_buffer_state *oy){
  memset(b,0,sizeof(*b));
  b->owner=oy;
}

void oggpackB_writeinit(oggpack_buffer *b,ogg_buffer_state *oy){
  oggpack_writeinit(b,oy);
}

static void _oggpack_extend(oggpack_buffer *b){
  if(b->head){
    b->head->used=b->head->size;
    b->count+=b->head->size;
    b->head->next=ogg_buffer_alloc(b->owner,OGGPACK_CHUNKSIZE);
    b->head=b->head->next;
  }else{
    b->head=b->tail=ogg_buffer_alloc(b->owner,OGGPACK_CHUNKSIZE);
  }

  b->headptr=b->head->data;
  b->headend=b->head->size;
}

/* Takes only up to 32 bits. */
void oggpack_write(oggpack_buffer *b,unsigned long value,int bits){

  value&=mask[bits]; 
  bits+=b->headbit;

  if(!b->headend){
    _oggpack_extend(b);
    *b->headptr=value;
  }else
    *b->headptr|=value<<b->headbit;  

  if(bits>=8){
    ++b->headptr;
    if(!--b->headend)_oggpack_extend(b);
    *b->headptr=value>>(8-b->headbit);  
    
    if(bits>=16){
      ++b->headptr;
      if(!--b->headend)_oggpack_extend(b);
      *b->headptr=value>>(16-b->headbit);  
      
      if(bits>=24){
	++b->headptr;
	if(!--b->headend)_oggpack_extend(b);
	*b->headptr=value>>(24-b->headbit);  
	
	if(bits>=32){
	  ++b->headptr;
	  if(!--b->headend)_oggpack_extend(b);
	  if(b->headbit)
	    *b->headptr=value>>(32-b->headbit);  
	  else
	    *b->headptr=0;
	}
      }
    }
  }
    
  b->headbit=bits&7;

}

/* Takes only up to 32 bits. */
void oggpackB_write(oggpack_buffer *b,unsigned long value,int bits){

  value=(value&mask[bits])<<(32-bits); 
  bits+=b->headbit;

  if(!b->headend){
    _oggpack_extend(b);
    *b->headptr=value>>24;
  }else
    *b->headptr|=value>>(24+b->headbit);    

  if(bits>=8){
    ++b->headptr;
    if(!--b->headend)_oggpack_extend(b);
    *b->headptr=value>>(16+b->headbit);  
    
    if(bits>=16){
      ++b->headptr;
      if(!--b->headend)_oggpack_extend(b);
      *b->headptr=value>>(8+b->headbit);  
      
      if(bits>=24){
	++b->headptr;
	if(!--b->headend)_oggpack_extend(b);
	*b->headptr=value>>(b->headbit);  
	
	if(bits>=32){
	  ++b->headptr;
	  if(!--b->headend)_oggpack_extend(b);
	  if(b->headbit)
	    *b->headptr=value<<(8-b->headbit);
	  else
	    *b->headptr=0;
	}
      }
    }
  }

  b->headbit=bits&7;
}

void oggpack_writealign(oggpack_buffer *b){
  int bits=8-b->headbit;
  if(bits<8)
    oggpack_write(b,0,bits);
}

void oggpackB_writealign(oggpack_buffer *b){
  int bits=8-b->headbit;
  if(bits<8)
    oggpackB_write(b,0,bits);
}

void oggpack_writebuffer(oggpack_buffer *b, ogg_buffer_reference *r){
  /* unlike ogg_buffer_references, the oggpack write buffers do not
     have any potential prefixed/unused ogg_buffer data */

  b->head->used=b->headptr-b->head->data+(b->headbit+7)/8;
  r->buffer=b->tail;
  r->begin=0;
  r->length=oggpack_bytes(b);
  r->owner=b->owner;

}

void oggpackB_writebuffer(oggpack_buffer *b, ogg_buffer_reference *r){
  oggpack_writebuffer(b,r);
}

/* frees and deallocates the oggpack_buffer ogg_buffer usage */
void oggpack_writeclear(oggpack_buffer *b){
  ogg_buffer *ptr=b->tail;

  while(ptr){
    ogg_buffer *next=ptr->next;
    ogg_buffer_release(ptr,b->owner);
    ptr=next;
  }
   
  memset(b,0,sizeof(*b));
}

void oggpackB_writeclear(oggpack_buffer *b){
  oggpack_writeclear(b);
}

void oggpack_readinit(oggpack_buffer *b,ogg_buffer_reference *r){
  int begin=r->begin;
  memset(b,0,sizeof(*b));

  b->owner=r->owner;
  b->head=r->buffer;

  /* advance head ptr to beginning of reference */
  while(begin>=b->head->used){
    ogg_buffer *next=b->head->next;
    begin-=b->head->used;
    //oggbuffer_release(b->head);
    b->head=next;
  }

  b->tail=r->buffer;  
  b->count= -begin;
  b->length=r->length+begin;
  b->headptr=b->head->data+begin;

  if(b->head->used>b->length){
    b->headend=b->length;
  }else{
    b->headend=b->head->used-begin;
  }
}

void oggpackB_readinit(oggpack_buffer *b,ogg_buffer_reference *r){
  oggpack_readinit(b,r);
}

/* Read in bits without advancing the bitptr; bits <= 32 */
int oggpack_look(oggpack_buffer *b,int bits,unsigned long *ret){
  unsigned long m=mask[bits];

  bits+=b->headbit;

  if(bits > b->headend<<3){
    int            end=b->headend;
    unsigned char *ptr=b->headptr;
    
    /* headend's semantic usage is complex; it's a 'check carefully
       past this point' marker.  It can mean we're either at the end
       of a buffer fragment, or done reading. */

    /* check to see if there are enough bytes left in next fragment
       [if any] to fufill read request. Spanning more than one boundary
       isn't possible so long as the ogg buffer abstraction enforces >
       4 byte fragments, which it does. */

    if(bits > (b->length-b->head->used+b->headend)*8)
      return -1;

    /* At this point, we're certain we span and that there's sufficient 
       data in the following buffer to fufill the read */

    if(!end)ptr=b->head->next->data;
    *ret=*ptr++>>b->headbit;
    if(bits>8){
      if(!--end)ptr=b->head->next->data;
      *ret|=*ptr++<<(8-b->headbit);  
      if(bits>16){
	if(!--end)ptr=b->head->next->data;
	*ret|=*ptr++<<(16-b->headbit);  
	if(bits>24){
	  if(!--end)ptr=b->head->next->data;
	  *ret|=*ptr++<<(24-b->headbit);  
	  if(bits>32 && b->headbit){
	    if(!--end)ptr=b->head->next->data;
	    *ret|=*ptr<<(32-b->headbit);
	  }
	}
      }
    }

  }else{

    /* make this a switch jump-table */
    *ret=b->headptr[0]>>b->headbit;
    if(bits>8){
      *ret|=b->headptr[1]<<(8-b->headbit);  
      if(bits>16){
	*ret|=b->headptr[2]<<(16-b->headbit);  
	if(bits>24){
	  *ret|=b->headptr[3]<<(24-b->headbit);  
	  if(bits>32 && b->headbit)
	    *ret|=b->headptr[4]<<(32-b->headbit);
	}
      }
    }
  }

  *ret&=m;
  return 0;
}

/* Read in bits without advancing the bitptr; bits <= 32 */
int oggpackB_look(oggpack_buffer *b,int bits,unsigned long *ret){
  int m=32-bits;

  bits+=b->headbit;

  if(bits > b->headend<<3){
    int            end=b->headend;
    unsigned char *ptr=b->headptr;
    
    /* headend's semantic usage is complex; it's a 'check carefully
       past this point' marker.  It can mean we're either at the end
       of a buffer fragment, or done reading. */

    /* check to see if there are enough bytes left in next fragment
       [if any] to fufill read request. Spanning more than one boundary
       isn't possible so long as the ogg buffer abstraction enforces >
       4 byte fragments, which it does. */

    if(bits > (b->length-b->head->used+b->headend)*8)
      return -1;

    /* At this point, we're certain we span and that there's sufficient 
       data in the following buffer to fufill the read */

    if(!end)ptr=b->head->next->data;
    *ret=*ptr++<<(24+b->headbit);
    if(bits>8){
      if(!--end)ptr=b->head->next->data;
      *ret|=*ptr++<<(16+b->headbit);   
      if(bits>16){
	if(!--end)ptr=b->head->next->data;
	*ret|=*ptr++<<(8+b->headbit);  
	if(bits>24){
	  if(!--end)ptr=b->head->next->data;
	  *ret|=*ptr++<<(b->headbit);  
	  if(bits>32 && b->headbit){
	    if(!--end)ptr=b->head->next->data;
	    *ret|=*ptr>>(8-b->headbit);
	  }
	}
      }
    }
    
  }else{
  
    *ret=b->headptr[0]<<(24+b->headbit);
    if(bits>8){
      *ret|=b->headptr[1]<<(16+b->headbit);  
      if(bits>16){
	*ret|=b->headptr[2]<<(8+b->headbit);  
	if(bits>24){
	  *ret|=b->headptr[3]<<(b->headbit);  
	  if(bits>32 && b->headbit)
	    *ret|=b->headptr[4]>>(8-b->headbit);
	}
      }
    }
  }

  *ret>>=m;
  return 0;
}

long oggpack_look1(oggpack_buffer *b){
  if(!b->headend) return (-1);
  return((b->headptr[0]>>b->headbit)&1);
}

long oggpackB_look1(oggpack_buffer *b){
  if(!b->headend) return (-1);
  return((b->headptr[0]>>(7-b->headbit))&1);
}

static void _oggpack_adv_halt(oggpack_buffer *b){
  /* implicit; called only when b->length<=b->head->used */
  b->headptr=b->head->data+b->length;
  b->length=0;
  b->headend=0;
  b->headbit=0;
}

static void _oggpack_adv_spanner(oggpack_buffer *b){

  if(b->length-b->head->used>0){
    /* on to the next fragment */
      
    b->count+=b->head->used;
    b->length-=b->head->used;
    b->head=b->head->next;
    b->headptr=b->head->data-b->headend;
    
    if(b->length<b->head->used){
      b->headend+=b->length;
    }else{
      b->headend+=b->head->used;
    }
    
  }else{
    
    /* no more, bring it to a halt */
    _oggpack_adv_halt(b);
    
  }
}

/* limited to 32 at a time */
void oggpack_adv(oggpack_buffer *b,int bits){
  bits+=b->headbit;
  b->headend-=bits/8;
  b->headbit=bits&7;
  b->headptr+=bits/8;
 
  if(b->headend<1)_oggpack_adv_spanner(b);
}

void oggpackB_adv(oggpack_buffer *b,int bits){
  oggpack_adv(b,bits);
}

void oggpack_adv1(oggpack_buffer *b){
  if(++(b->headbit)>7){
    b->headbit=0;
    ++b->headptr;
    --b->headend;  
    if(b->headend<1)_oggpack_adv_spanner(b);
  }
}

void oggpackB_adv1(oggpack_buffer *b){
  oggpack_adv1(b);
}

/* bits <= 32 */
int oggpack_read(oggpack_buffer *b,int bits,unsigned long *ret){
  unsigned long m=mask[bits];

  bits+=b->headbit;

  if(bits > b->headend<<3){

    /* headend's semantic usage is complex; it's a 'check carefully
       past this point' marker.  It can mean we're either at the end
       of a buffer fragment, or done reading. */

    /* check to see if there are enough bytes left in next fragment
       [if any] to fufill read request. Spanning more than one boundary
       isn't possible so long as the ogg buffer abstraction enforces >
       4 byte fragments, which it does. */

    if(bits > (b->length-b->head->used+b->headend)*8){
      _oggpack_adv_halt(b);
      return -1;
    }

    if(!b->headend)b->headptr=b->head->next->data;
    *ret=*b->headptr>>b->headbit;
    if(bits>=8){
      b->headptr++;
      if(!--b->headend)b->headptr=b->head->next->data;
      *ret|=*b->headptr<<(8-b->headbit);   
      if(bits>=16){
	b->headptr++;
	if(!--b->headend)b->headptr=b->head->next->data;
	*ret|=*b->headptr<<(16-b->headbit);  
	if(bits>=24){
	  b->headptr++;
	  if(!--b->headend)b->headptr=b->head->next->data;
	  *ret|=*b->headptr<<(24-b->headbit);  
	  if(bits>=32){
	    b->headptr++;
	    if(!--b->headend)b->headptr=b->head->next->data;
	    if(b->headbit)*ret|=*b->headptr<<(32-b->headbit);
	  }
	}
      }
    }
    
    b->count+=b->head->used;
    b->length-=b->head->used;
    b->head=b->head->next;
    b->headend+=b->head->used;

  }else{
  
    *ret=b->headptr[0]>>b->headbit;
    if(bits>8){
      *ret|=b->headptr[1]<<(8-b->headbit);  
      if(bits>16){
	*ret|=b->headptr[2]<<(16-b->headbit);  
	if(bits>24){
	  *ret|=b->headptr[3]<<(24-b->headbit);  
	  if(bits>32 && b->headbit){
	    *ret|=b->headptr[4]<<(32-b->headbit);
	  }
	}
      }
    }
    
    b->headptr+=bits/8;
    b->headend-=bits/8;
  }

  *ret&=m;
  b->headbit=bits&7;   
  return(0);
}

/* bits <= 32 */
int oggpackB_read(oggpack_buffer *b,int bits,unsigned long *ret){
  long m=32-bits;
  
  bits+=b->headbit;

  if(bits > b->headend<<3){
    
    /* headend's semantic usage is complex; it's a 'check carefully
       past this point' marker.  It can mean we're either at the end
       of a buffer fragment, or done reading. */

    /* check to see if there are enough bytes left in next fragment
       [if any] to fufill read request. Spanning more than one boundary
       isn't possible so long as the ogg buffer abstraction enforces >
       4 byte fragments, which it does. */

    if(bits > (b->length-b->head->used+b->headend)*8){
      _oggpack_adv_halt(b);
      return -1;
    }

    if(!b->headend)b->headptr=b->head->next->data;
    *ret=*b->headptr<<(24+b->headbit);
    if(bits>=8){
      b->headptr++;
      if(!--b->headend)b->headptr=b->head->next->data;
      *ret|=*b->headptr<<(16+b->headbit);   
      if(bits>=16){
	b->headptr++;
	if(!--b->headend)b->headptr=b->head->next->data;
	*ret|=*b->headptr<<(8+b->headbit);  
	if(bits>=24){
	  b->headptr++;
	  if(!--b->headend)b->headptr=b->head->next->data;
	  *ret|=*b->headptr<<(b->headbit);  
	  if(bits>=32){
	    b->headptr++;
	    if(!--b->headend)b->headptr=b->head->next->data;
	    if(b->headbit)*ret|=*b->headptr>>(8-b->headbit);
	  }
	}
      }
    }
    
    b->count+=b->head->used;
    b->length-=b->head->used;
    b->head=b->head->next;
    b->headend+=b->head->used;
    
  }else{
  
    *ret=b->headptr[0]<<(24+b->headbit);
    if(bits>8){
      *ret|=b->headptr[1]<<(16+b->headbit);  
      if(bits>16){
	*ret|=b->headptr[2]<<(8+b->headbit);  
	if(bits>24){
	  *ret|=b->headptr[3]<<(b->headbit);  
	  if(bits>32 && b->headbit)
	    *ret|=b->headptr[4]>>(8-b->headbit);
	}
      }
    }

    b->headptr+=bits/8;
    b->headend-=bits/8;
  }

  b->headbit=bits&7;
  *ret>>=m;
  return 0;
}

long oggpack_read1(oggpack_buffer *b){
  unsigned long ret;

  if(!b->headend) return (-1UL);
  ret=(b->headptr[0]>>b->headbit)&1;

  if(++(b->headbit)>7){
    b->headbit=0;
    ++b->headptr;
    --b->headend;  
    if(b->headend<1)_oggpack_adv_spanner(b);
  }

  return(ret);
}

long oggpackB_read1(oggpack_buffer *b){
  unsigned long ret;

  if(!b->headend) return (-1UL);
  ret=(b->headptr[0]>>(7-b->headbit))&1;

  if(++(b->headbit)>7){
    b->headbit=0;
    ++b->headptr;
    --b->headend;  
    if(b->headend<1)_oggpack_adv_spanner(b);
  }

  return(ret);
}

long oggpack_bytes(oggpack_buffer *b){
  return(b->count+b->headptr-b->head->data+(b->headbit+7)/8);
}

long oggpack_bits(oggpack_buffer *b){
  return((b->count+b->headptr-b->head->data)*8+b->headbit);
}

long oggpackB_bytes(oggpack_buffer *b){
  return oggpack_bytes(b);
}

long oggpackB_bits(oggpack_buffer *b){
  return oggpack_bits(b);
}
  

/* Self test of the bitwise routines; everything else is based on
   them, so they damned well better be solid. */

#ifdef _V_SELFTEST
#include <stdio.h>

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}
      
oggpack_buffer o;
oggpack_buffer r;
ogg_buffer_state bs;
ogg_buffer_reference or;
#define TESTWORDS 4096

void report(char *in){
  fprintf(stderr,"%s",in);
  exit(1);
}

int getbyte(ogg_buffer_reference *fb,int position){
  ogg_buffer *b=fb->buffer;
  while(b && position>=b->used){
    position-=b->used;
    b=b->next;
    if(b==NULL){
      fprintf(stderr,"\n\tERROR: getbyte ran off end of buffer.\n");
      exit(1);
    }
  }

  return(b->data[position]);
}

void cliptest(unsigned long *b,int vals,int bits,int *comp,int compsize){
  long bytes,i,bitcount=0;
  ogg_buffer_reference or;

  oggpack_writeinit(&o,&bs);
  for(i=0;i<vals;i++){
    oggpack_write(&o,b[i],bits?bits:ilog(b[i]));
    bitcount+=bits?bits:ilog(b[i]);
    if(bitcount!=oggpack_bits(&o)){
      report("wrong number of bits while writing!\n");
    }
    if((bitcount+7)/8!=oggpack_bytes(&o)){
      report("wrong number of bytes while writing!\n");
    }
  }

  oggpack_writebuffer(&o,&or);
  bytes=oggpack_bytes(&o);

  if(bytes!=compsize)report("wrong number of bytes!\n");
  for(i=0;i<bytes;i++)if(getbyte(&or,i)!=comp[i]){
    for(i=0;i<bytes;i++)fprintf(stderr,"%x %x\n",getbyte(&or,i),(int)comp[i]);
    report("wrote incorrect value!\n");
  }

  bitcount=0;
  oggpack_readinit(&r,&or);
  for(i=0;i<vals;i++){
    unsigned long test;
    int tbit=bits?bits:ilog(b[i]);
    if(oggpack_look(&r,tbit,&test))
      report("out of data!\n");
    if(test!=(b[i]&mask[tbit])){
      fprintf(stderr,"%ld) %lx %lx\n",i,(b[i]&mask[tbit]),test);
      report("looked at incorrect value!\n");
    }
    if(tbit==1)
      if(oggpack_look1(&r)!=(int)(b[i]&mask[tbit]))
	report("looked at single bit incorrect value!\n");
    if(tbit==1){
      if(oggpack_read1(&r)!=(int)(b[i]&mask[tbit]))
	report("read incorrect single bit value!\n");
    }else{
      oggpack_read(&r,tbit,&test);
      if(test!=(b[i]&mask[tbit])){
	fprintf(stderr,"%ld) %lx %lx\n",i,(b[i]&mask[tbit]),test);
	report("read incorrect value!\n");
      }
    }
    bitcount+=tbit;

    if(bitcount!=oggpack_bits(&r)){
      report("wrong number of bits while reading!\n");
    }
    if((bitcount+7)/8!=oggpack_bytes(&r)){
      report("wrong number of bytes while reading!\n");
    }
  }
  if(oggpack_bytes(&r)!=bytes)report("leftover bytes after read!\n");
  oggpack_writeclear(&o);

}

void cliptestB(unsigned long *b,int vals,int bits,int *comp,int compsize){
  long bytes,i;
  ogg_buffer_reference or;

  oggpackB_writeinit(&o,&bs);
  for(i=0;i<vals;i++)
    oggpackB_write(&o,b[i],bits?bits:ilog(b[i]));
  oggpackB_writebuffer(&o,&or);
  bytes=oggpackB_bytes(&o);
  if(bytes!=compsize)report("wrong number of bytes!\n");
  for(i=0;i<bytes;i++)if(getbyte(&or,i)!=comp[i]){
    for(i=0;i<bytes;i++)fprintf(stderr,"%x %x\n",getbyte(&or,i),(int)comp[i]);
    report("wrote incorrect value!\n");
  }
  oggpackB_readinit(&r,&or);
  for(i=0;i<vals;i++){
    unsigned long test;
    int tbit=bits?bits:ilog(b[i]);
    if(oggpackB_look(&r,tbit,&test))
      report("out of data!\n");
    if(test!=(b[i]&mask[tbit]))
      report("looked at incorrect value!\n");
    if(tbit==1)
      if(oggpackB_look1(&r)!=(int)(b[i]&mask[tbit]))
	report("looked at single bit incorrect value!\n");
    if(tbit==1){
      if(oggpackB_read1(&r)!=(int)(b[i]&mask[tbit]))
	report("read incorrect single bit value!\n");
    }else{
      oggpackB_read(&r,tbit,&test);
      if(test!=(b[i]&mask[tbit]))
	report("read incorrect value!\n");
    }
  }
  if(oggpackB_bytes(&r)!=bytes)report("leftover bytes after read!\n");
  oggpackB_writeclear(&o);
}

int flatten (unsigned char *flat){
  unsigned char *ptr=flat;
  ogg_buffer *head;
  oggpack_writebuffer(&o,&or);
  head=or.buffer;
  while(head){
    memcpy(ptr,head->data,head->used);
    ptr+=head->used;
    head=head->next;
  }
  return ptr-flat;
}

void lsbverify(unsigned long *values,int *len,unsigned char *flat) {
  int j,k;
  int flatbyte=0;
  int flatbit=0;
  
  /* verify written buffer is correct bit-by-bit */
  for(j=0;j<TESTWORDS;j++){
    for(k=0;k<len[j];k++){
      int origbit=(values[j]>>k)&1;
      int bit=(flat[flatbyte]>>flatbit)&1;
      flatbit++;
      if(flatbit>7){
	flatbit=0;
	++flatbyte;
      }
      
      if(origbit!=bit){
	fprintf(stderr,"\n\tERROR: bit mismatch!  "
		"word %d, bit %d, value %lx, len %d\n",
		j,k,values[j],len[j]);
	exit(1);
      }
      
    }
  }

  /* verify that trailing packing is zeroed */
  while(flatbit && flatbit<8){
    int bit=(flat[flatbyte]>>flatbit++)&1;
  
    if(0!=bit){
      fprintf(stderr,"\n\tERROR: trailing byte padding not zero!\n");
      exit(1);
    }
  }  
  

}

int main(void){
  long bytes,i;
  static unsigned long testbuffer1[]=
    {18,12,103948,4325,543,76,432,52,3,65,4,56,32,42,34,21,1,23,32,546,456,7,
       567,56,8,8,55,3,52,342,341,4,265,7,67,86,2199,21,7,1,5,1,4};
  int test1size=43;

  static unsigned long testbuffer2[]=
    {216531625L,1237861823,56732452,131,3212421,12325343,34547562,12313212,
       1233432,534,5,346435231,14436467,7869299,76326614,167548585,
       85525151,0,12321,1,349528352};
  int test2size=21;

  static unsigned long testbuffer3[]=
    {1,0,14,0,1,0,12,0,1,0,0,0,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,1,1,1,1,0,0,1,
       0,1,30,1,1,1,0,0,1,0,0,0,12,0,11,0,1,0,0,1};
  int test3size=56;

  static unsigned long large[]=
    {2136531625L,2137861823,56732452,131,3212421,12325343,34547562,12313212,
       1233432,534,5,2146435231,14436467,7869299,76326614,167548585,
       85525151,0,12321,1,2146528352};

  int onesize=33;
  static int one[33]={146,25,44,151,195,15,153,176,233,131,196,65,85,172,47,40,
                    34,242,223,136,35,222,211,86,171,50,225,135,214,75,172,
                    223,4};
  static int oneB[33]={150,101,131,33,203,15,204,216,105,193,156,65,84,85,222,
		       8,139,145,227,126,34,55,244,171,85,100,39,195,173,18,
		       245,251,128};

  int twosize=6;
  static int two[6]={61,255,255,251,231,29};
  static int twoB[6]={247,63,255,253,249,120};

  int threesize=54;
  static int three[54]={169,2,232,252,91,132,156,36,89,13,123,176,144,32,254,
                      142,224,85,59,121,144,79,124,23,67,90,90,216,79,23,83,
                      58,135,196,61,55,129,183,54,101,100,170,37,127,126,10,
                      100,52,4,14,18,86,77,1};
  static int threeB[54]={206,128,42,153,57,8,183,251,13,89,36,30,32,144,183,
			 130,59,240,121,59,85,223,19,228,180,134,33,107,74,98,
			 233,253,196,135,63,2,110,114,50,155,90,127,37,170,104,
			 200,20,254,4,58,106,176,144,0};

  int foursize=38;
  static int four[38]={18,6,163,252,97,194,104,131,32,1,7,82,137,42,129,11,72,
                     132,60,220,112,8,196,109,64,179,86,9,137,195,208,122,169,
                     28,2,133,0,1};
  static int fourB[38]={36,48,102,83,243,24,52,7,4,35,132,10,145,21,2,93,2,41,
			1,219,184,16,33,184,54,149,170,132,18,30,29,98,229,67,
			129,10,4,32};

  int fivesize=45;
  static int five[45]={169,2,126,139,144,172,30,4,80,72,240,59,130,218,73,62,
                     241,24,210,44,4,20,0,248,116,49,135,100,110,130,181,169,
                     84,75,159,2,1,0,132,192,8,0,0,18,22};
  static int fiveB[45]={1,84,145,111,245,100,128,8,56,36,40,71,126,78,213,226,
			124,105,12,0,133,128,0,162,233,242,67,152,77,205,77,
			172,150,169,129,79,128,0,6,4,32,0,27,9,0};

  int sixsize=7;
  static int six[7]={17,177,170,242,169,19,148};
  static int sixB[7]={136,141,85,79,149,200,41};

  /* Test read/write together */
  /* Later we test against pregenerated bitstreams */
  ogg_buffer_init(&bs);

  fprintf(stderr,"\nSmall preclipped packing (LSb): ");
  cliptest(testbuffer1,test1size,0,one,onesize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nNull bit call (LSb): ");
  cliptest(testbuffer3,test3size,0,two,twosize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nLarge preclipped packing (LSb): ");
  cliptest(testbuffer2,test2size,0,three,threesize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\n32 bit preclipped packing (LSb): ");

  oggpack_writeclear(&o);
  oggpack_writeinit(&o,&bs);
  for(i=0;i<test2size;i++)
    oggpack_write(&o,large[i],32);
  oggpack_writebuffer(&o,&or);
  bytes=oggpack_bytes(&o);
  oggpack_readinit(&r,&or);
  for(i=0;i<test2size;i++){
    unsigned long test;
    if(oggpack_look(&r,32,&test)==-1)report("out of data. failed!");
    if(test!=large[i]){
      fprintf(stderr,"%ld != %ld (%lx!=%lx):",test,large[i],
	      test,large[i]);
      report("read incorrect value!\n");
    }
    oggpack_adv(&r,32);
  }
  if(oggpack_bytes(&r)!=bytes)report("leftover bytes after read!\n");
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nSmall unclipped packing (LSb): ");
  cliptest(testbuffer1,test1size,7,four,foursize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nLarge unclipped packing (LSb): ");
  cliptest(testbuffer2,test2size,17,five,fivesize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nSingle bit unclipped packing (LSb): ");
  cliptest(testbuffer3,test3size,1,six,sixsize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nTesting read past end (LSb): ");
  {
    ogg_buffer lob={"\0\0\0\0\0\0\0\0",8,0,8,1};
    ogg_buffer_reference lor={&lob,0,8,&bs};

    oggpack_readinit(&r,&lor);
    for(i=0;i<64;i++){
      if(oggpack_read1(&r)!=0){
	fprintf(stderr,"failed; got -1 prematurely.\n");
	exit(1);
      }
    }
    if(oggpack_look1(&r)!=-1 ||
       oggpack_read1(&r)!=-1){
      fprintf(stderr,"failed; read past end without -1.\n");
      exit(1);
    }
  }
  {
    ogg_buffer lob={"\0\0\0\0\0\0\0\0",8,0,8,1};
    ogg_buffer_reference lor={&lob,0,8,&bs};
    unsigned long test;

    oggpack_readinit(&r,&lor);
    if(oggpack_read(&r,30,&test)!=0 || oggpack_read(&r,16,&test)!=0){
      fprintf(stderr,"failed 2; got -1 prematurely.\n");
      exit(1);
    }
    
    if(oggpack_look(&r,18,&test)!=0){
      fprintf(stderr,"failed 3; got -1 prematurely.\n");
      exit(1);
    }
    if(oggpack_look(&r,19,&test)!=-1){
      fprintf(stderr,"failed; read past end without -1.\n");
      exit(1);
    }
    if(oggpack_look(&r,32,&test)!=-1){
      fprintf(stderr,"failed; read past end without -1.\n");
      exit(1);
    }
  }
  fprintf(stderr,"ok.\n");
  oggpack_writeclear(&o);

  /********** lazy, cut-n-paste retest with MSb packing ***********/

  /* Test read/write together */
  /* Later we test against pregenerated bitstreams */

  fprintf(stderr,"\nSmall preclipped packing (MSb): ");
  cliptestB(testbuffer1,test1size,0,oneB,onesize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nNull bit call (MSb): ");
  cliptestB(testbuffer3,test3size,0,twoB,twosize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nLarge preclipped packing (MSb): ");
  cliptestB(testbuffer2,test2size,0,threeB,threesize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\n32 bit preclipped packing (MSb): ");

  oggpackB_writeinit(&o,&bs);
  for(i=0;i<test2size;i++)
    oggpackB_write(&o,large[i],32);
  oggpackB_writebuffer(&o,&or);
  bytes=oggpackB_bytes(&o);
  oggpackB_readinit(&r,&or);
  for(i=0;i<test2size;i++){
    unsigned long test;
    if(oggpackB_look(&r,32,&test)==-1)report("out of data. failed!");
    if(test!=large[i]){
      fprintf(stderr,"%ld != %ld (%lx!=%lx):",test,large[i],
	      test,large[i]);
      report("read incorrect value!\n");
    }
    oggpackB_adv(&r,32);
  }
  if(oggpackB_bytes(&r)!=bytes)report("leftover bytes after read!\n");
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nSmall unclipped packing (MSb): ");
  cliptestB(testbuffer1,test1size,7,fourB,foursize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nLarge unclipped packing (MSb): ");
  cliptestB(testbuffer2,test2size,17,fiveB,fivesize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nSingle bit unclipped packing (MSb): ");
  cliptestB(testbuffer3,test3size,1,sixB,sixsize);
  fprintf(stderr,"ok.");

  fprintf(stderr,"\nTesting read past end (MSb): ");
  {
    ogg_buffer lob={"\0\0\0\0\0\0\0\0",8,0,8,1};
    ogg_buffer_reference lor={&lob,0,8,&bs};
    unsigned long test;

    oggpackB_readinit(&r,&lor);
    for(i=0;i<64;i++){
      if(oggpackB_read(&r,1,&test)){
	fprintf(stderr,"failed; got -1 prematurely.\n");
	exit(1);
      }
    }
    if(oggpackB_look(&r,1,&test)!=-1 ||
       oggpackB_read(&r,1,&test)!=-1){
      fprintf(stderr,"failed; read past end without -1.\n");
      exit(1);
    }
  }
  {
    ogg_buffer lob={"\0\0\0\0\0\0\0\0",8,0,8,1};
    ogg_buffer_reference lor={&lob,0,8,&bs};
    unsigned long test;
    oggpackB_readinit(&r,&lor);

    if(oggpackB_read(&r,30,&test)!=0 || oggpackB_read(&r,16,&test)!=0){
      fprintf(stderr,"failed 2; got -1 prematurely.\n");
      exit(1);
    }
    
    if(oggpackB_look(&r,18,&test)!=0){
      fprintf(stderr,"failed 3; got -1 prematurely.\n");
      exit(1);
    }
    if(oggpackB_look(&r,19,&test)!=-1){
      fprintf(stderr,"failed 4; read past end without -1.\n");
      exit(1);
    }
    if(oggpackB_look(&r,32,&test)!=-1){
      fprintf(stderr,"failed 5; read past end without -1.\n");
      exit(1);
    }
    fprintf(stderr,"ok.\n\n");
  }
  oggpackB_writeclear(&o);

  /* now the scary shit: randomized testing */

  for(i=0;i<1000;i++){
    int j,count=0,count2=0;
    unsigned long values[TESTWORDS];
    int len[TESTWORDS];
    unsigned char flat[4*TESTWORDS]; /* max possible needed size */

    fprintf(stderr,"\rRandomized testing (LSb)... (%ld)   ",1000-i);
    oggpack_writeinit(&o,&bs);

    /* generate a list of words and lengths */
    /* write the required number of bits out to packbuffer */
    for(j=0;j<TESTWORDS;j++){
      values[j]=rand();
      len[j]=(rand()%32)+1;
      count+=len[j];
      oggpack_write(&o,values[j],len[j]);
    }

    /* flatten the packbuffer out to a vector */
    count2=flatten(flat);

    /* verify against original list */
    lsbverify(values,len,flat);

    /* construct random-length buffer chain from flat vector; random
       byte starting offset within the length of the vector */
    {
      ogg_buffer *obl=NULL,*ob=NULL;
      unsigned char *ptr=flat;
      
      /* build buffer chain */
      while(count2){
	int ilen;
	ogg_buffer *temp=_ogg_malloc(sizeof(*temp)); /* we don't bother
                                                      freeing later;
                                                      this is just a
                                                      unit test */
	if(obl)
	  obl->next=temp;
	else
	  ob=temp;

	ilen=(rand()%8)*4+4;
	if(ilen>count2)ilen=count2;
	obl=temp;
	obl->data=ptr;
	obl->size=count2;
	obl->used=ilen;
	
	count2-=ilen;
	ptr+=ilen;
      }
      /* build reference; choose a starting offset. */
      {
	int begin=(rand()%TESTWORDS);
	int ilen=(rand()%(TESTWORDS-begin));
	int bitoffset,bitcount=0;
	unsigned long temp;
	or.buffer=ob;

	for(j=0;j<begin;j++)
	  bitcount+=len[j];
	or.begin=bitcount/8;
	bitoffset=bitcount%8;
	for(;j<begin+ilen;j++)
	  bitcount+=len[j];
	or.length=((bitcount+7)/8)-or.begin;

	oggpack_readinit(&o,&or);

	for(j=0;j<bitoffset;j++)
	  oggpack_read1(&o);
	
	/* read and compare to original list */
	for(j=begin;j<begin+ilen;j++){
	  if(oggpack_read(&o,len[j],&temp)){
	    fprintf(stderr,"\nERROR: End of stream too soon! word: %d\n",
		    j-begin);
	    exit(1);
	  }
	  if(temp!=(values[j]&mask[len[j]])){
	    fprintf(stderr,"\nERROR: Incorrect read %lx != %lx, word %d, len %d\n",
		    values[j]&mask[len[j]],temp,j-begin,len[j]);
	    exit(1);
	  }
	}
      }
    }
    oggpack_writeinit(&o,&bs);
  }
  fprintf(stderr,"\rRandomized n-bit testing (LSb)... ok.   \n");

  oggpackB_writeclear(&o);

  ogg_buffer_clear(&bs);
  return(0);
}  
#endif  /* _V_SELFTEST */
