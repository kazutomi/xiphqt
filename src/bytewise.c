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

  function: byte-aligned access; array-like abstraction over buffers
  last mod: $Id$

 ********************************************************************/

#include <string.h>
#include <stdlib.h>
#include "ogginternal.h"

/* this is an internal abstraction, and it is not guarded from misuse
   or botching a fencepost. */

static void _positionB(ogg2byte_buffer *b,int pos){
  if(pos<b->pos){
    /* start at beginning, scan forward */
    b->ref=b->baseref;
    b->pos=0;
    b->end=b->pos+b->ref->length;
    b->ptr=b->ref->buffer->data+b->ref->begin;
  }
}

static void _positionF(ogg2byte_buffer *b,int pos){
  /* scan forward for position */
  while(pos>=b->end){
    /* just seek forward */
    b->pos+=b->ref->length;
    b->ref=b->ref->next;
    b->end=b->ref->length+b->pos;
    b->ptr=b->ref->buffer->data+b->ref->begin;
  }
}

static void _positionFE(ogg2byte_buffer *b,int pos){
  /* scan forward for position */
  while(pos>=b->end){
    if(!b->ref->next){
      /* perhaps just need to extend length field... */
      if(pos-b->pos < b->ref->buffer->size-b->ref->begin){

	/* yes, there's space here */
	b->ref->length=pos-b->pos+1;
	b->end=b->pos+b->ref->length;

      }else{
	b->ref->length=b->ref->buffer->size-b->ref->begin;
	/* extend the array and span */
	b->pos+=b->ref->length;	
	b->ref=ogg2_buffer_extend(b->ref,OGG2PACK_CHUNKSIZE);
	b->end=b->pos;
	b->ptr=b->ref->buffer->data+b->ref->begin;
      }

    }else{

      /* just seek forward */
      b->pos+=b->ref->length;
      b->ref=b->ref->next;
      b->end=b->ref->length+b->pos;
      b->ptr=b->ref->buffer->data+b->ref->begin;
    }
  }
}

int ogg2byte_init(ogg2byte_buffer *b,ogg2_reference *or,ogg2_buffer_state *bs){
  memset(b,0,sizeof(*b));
    
  if(!or){
    if(!bs)return -1;
    or=ogg2_buffer_alloc(bs,OGG2PACK_CHUNKSIZE);
  }else{
    b->external=1;
  }

  b->owner=bs;
  b->ref=b->baseref=or;
  b->pos=0;
  b->end=b->ref->length;
  b->ptr=b->ref->buffer->data+b->ref->begin;
  
  return(0);
}

ogg2_reference *ogg2byte_return_and_reset(ogg2byte_buffer *b){
  if(!b->external){
    ogg2_reference *ret=b->baseref;
    memset(b,0,sizeof(*b));
    return(ret);
  }
  return(NULL);
}

void ogg2byte_clear(ogg2byte_buffer *b){
  if(!b->external)ogg2_buffer_release(b->baseref);
  memset(b,0,sizeof(*b));
}

void ogg2byte_set1(ogg2byte_buffer *b,unsigned char val,int pos){
  _positionB(b,pos);
  _positionFE(b,pos);
  b->ptr[pos-b->pos]=val;
}

void ogg2byte_set2(ogg2byte_buffer *b,int val,int pos){
  _positionB(b,pos);
  _positionFE(b,pos);
  b->ptr[pos-b->pos]=val;
  _positionFE(b,++pos);
  b->ptr[pos-b->pos]=val>>8;
}

void ogg2byte_set4(ogg2byte_buffer *b,ogg_uint32_t val,int pos){
  int i;
  _positionB(b,pos);
  for(i=0;i<4;i++){
    _positionFE(b,pos);
    b->ptr[pos-b->pos]=val;
    val>>=8;
    ++pos;
  }
}

void ogg2byte_set8(ogg2byte_buffer *b,ogg_int64_t val,int pos){
  int i;
  _positionB(b,pos);
  for(i=0;i<8;i++){
    _positionFE(b,pos);
    b->ptr[pos-b->pos]=val;
    val>>=8;
    ++pos;
  }
}
 
unsigned char ogg2byte_read1(ogg2byte_buffer *b,int pos){
  _positionB(b,pos);
  _positionF(b,pos);
  return b->ptr[pos-b->pos];
}

int ogg2byte_read2(ogg2byte_buffer *b,int pos){
  int ret;
  _positionB(b,pos);
  _positionF(b,pos);
  ret=b->ptr[pos-b->pos];
  _positionF(b,++pos);
  return ret|b->ptr[pos-b->pos]<<8;  
}

ogg_uint32_t ogg2byte_read4(ogg2byte_buffer *b,int pos){
  ogg_uint32_t ret;
  _positionB(b,pos);
  _positionF(b,pos);
  ret=b->ptr[pos-b->pos];
  _positionF(b,++pos);
  ret|=b->ptr[pos-b->pos]<<8;
  _positionF(b,++pos);
  ret|=b->ptr[pos-b->pos]<<16;
  _positionF(b,++pos);
  ret|=b->ptr[pos-b->pos]<<24;
  return ret;
}

ogg_int64_t ogg2byte_read8(ogg2byte_buffer *b,int pos){
  ogg_int64_t ret;
  unsigned char t[7];
  int i;
  _positionB(b,pos);
  for(i=0;i<7;i++){
    _positionF(b,pos);
    t[i]=b->ptr[pos++ -b->pos];
  }

  _positionF(b,pos);
  ret=b->ptr[pos-b->pos];

  for(i=6;i>=0;--i)
    ret= ret<<8 | t[i];

  return ret;
}


#ifdef _V_SELFTEST

#include <stdio.h>
#define TESTBYTES 4096

unsigned char ref[TESTBYTES];
unsigned char work[TESTBYTES];
ogg2_buffer_state *bs;

void _read_linear_test1(ogg2_reference *or){
  ogg2byte_buffer obb;
  int j;

  ogg2byte_init(&obb,or,0);
  for(j=0;j<TESTBYTES;j++){
    unsigned char ret=ogg2byte_read1(&obb,j);
    if(ref[j]!=ret){
      fprintf(stderr,"\nERROR:  %02x != %02x, position %d\n\n",
	      ref[j],ret,j);
      exit(1);
    }
  }
}

void _read_linear_test1b(ogg2_reference *or){
  ogg2byte_buffer obb;
  int j;

  ogg2byte_init(&obb,or,0);
  for(j=0;j<TESTBYTES;j++){
    if(work[j]){
      unsigned char ret=ogg2byte_read1(&obb,j);
      if(ref[j]!=ret){
	fprintf(stderr,"\nERROR:  %02x != %02x, position %d\n\n",
		ref[j],ret,j);
	exit(1);
      }
    }
  }
}

void _read_linear_test2(ogg2_reference *or){
  ogg2byte_buffer obb;
  int j;

  ogg2byte_init(&obb,or,0);
  for(j=0;j+1<TESTBYTES;j++){
    int ret=ogg2byte_read2(&obb,j);
    if(ref[j]!=(ret&0xff) || ref[j+1]!=((ret>>8)&0xff)){
      fprintf(stderr,"\nERROR:  %02x%02x != %04x, position %d\n\n",
	      ref[j+1],ref[j],ret,j);
      exit(1);
    }
  }
}

void _read_linear_test4(ogg2_reference *or){
  ogg2byte_buffer obb;
  int j;

  ogg2byte_init(&obb,or,0);

  for(j=0;j+3<TESTBYTES;j++){
    ogg_uint32_t ret=ogg2byte_read4(&obb,j);
    if(ref[j]!=(ret&0xff) ||
       ref[j+1]!=((ret>>8)&0xff) ||
       ref[j+2]!=((ret>>16)&0xff) ||
       ref[j+3]!=((ret>>24)&0xff)    ){
      fprintf(stderr,"\nERROR:  %02x%02x%02x%02x != %08lx, position %d\n\n",
	      ref[j+3],ref[j+2],ref[j+1],ref[j],
	      (unsigned long)ret,j);
      exit(1);
    }
  }
}

void _read_linear_test8(ogg2_reference *or){
  ogg2byte_buffer obb;
  int j;

  ogg2byte_init(&obb,or,0);

  for(j=0;j+7<TESTBYTES;j++){
    ogg_int64_t ret=ref[j+7];
    ret=(ret<<8)|ref[j+6];
    ret=(ret<<8)|ref[j+5];
    ret=(ret<<8)|ref[j+4];
    ret=(ret<<8)|ref[j+3];
    ret=(ret<<8)|ref[j+2];
    ret=(ret<<8)|ref[j+1];
    ret=(ret<<8)|ref[j];
    
    if(ret!=ogg2byte_read8(&obb,j)){
      int i;
      ret=ogg2byte_read8(&obb,j);
      fprintf(stderr,"\nERROR:  %02x%02x%02x%02x%02x%02x%02x%02x != ",
	      ref[j+7],ref[j+6],ref[j+5],ref[j+4],
	      ref[j+3],ref[j+2],ref[j+1],ref[j]);
      for(i=0;i<8;i++){
	ref[j+i]=ret&0xff;
	ret>>=8;
      }
      fprintf(stderr,"%02x%02x%02x%02x%02x%02x%02x%02x, position %d\n\n",
	      ref[j+7],ref[j+6],ref[j+5],ref[j+4],
	      ref[j+3],ref[j+2],ref[j+1],ref[j],
	      j);
      exit(1);
    }
  }
}

void _read_seek_test(ogg2_reference *or){
  ogg2byte_buffer obb;
  int i,j;
  int length=TESTBYTES;
  unsigned char *lref=ref;

  ogg2byte_init(&obb,or,0);
  
  for(i=0;i<TESTBYTES;i++){
    unsigned char ret;
    j=rand()%length;
    ret=ogg2byte_read1(&obb,j);
    if(lref[j]!=ret){
      fprintf(stderr,"\nERROR:  %02x != %02x, position %d\n\n",
	      lref[j],ret,j);
      exit(1);
    }
  }

  for(i=0;i<TESTBYTES;i++){
    int ret;
    j=rand()%(length-1);
    ret=ogg2byte_read2(&obb,j);
    if(lref[j]!=(ret&0xff) || lref[j+1]!=((ret>>8)&0xff)){
      fprintf(stderr,"\nERROR:  %02x%02x != %04x, position %d\n\n",
	      lref[j+1],lref[j],ret,j);
      exit(1);
    }
  }

  for(i=0;i<TESTBYTES;i++){
    ogg_uint32_t ret;
    j=rand()%(length-3);
    ret=ogg2byte_read4(&obb,j);
    if(lref[j]!=(ret&0xff) ||
       lref[j+1]!=((ret>>8)&0xff) ||
       lref[j+2]!=((ret>>16)&0xff) ||
       lref[j+3]!=((ret>>24)&0xff)    ){
      fprintf(stderr,"\nERROR:  %02x%02x%02x%02x != %08lx, position %d\n\n",
	      lref[j+3],lref[j+2],lref[j+1],lref[j],
	      (unsigned long)ret,j);
      exit(1);
    }
  }

  for(i=0;i<TESTBYTES;i++){
    ogg_int64_t ret;
    j=rand()%(length-7);
    ret=lref[j+7];
    ret=(ret<<8)|lref[j+6];
    ret=(ret<<8)|lref[j+5];
    ret=(ret<<8)|lref[j+4];
    ret=(ret<<8)|lref[j+3];
    ret=(ret<<8)|lref[j+2];
    ret=(ret<<8)|lref[j+1];
    ret=(ret<<8)|lref[j];
    
    if(ret!=ogg2byte_read8(&obb,j)){
      int i;
      ret=ogg2byte_read8(&obb,j);
      fprintf(stderr,"\nERROR:  %02x%02x%02x%02x%02x%02x%02x%02x != ",
	      lref[j+7],lref[j+6],lref[j+5],lref[j+4],
	      lref[j+3],lref[j+2],lref[j+1],lref[j]);
      for(i=0;i<8;i++){
	lref[j+i]=ret&0xff;
	ret>>=8;
      }
      fprintf(stderr,"%02x%02x%02x%02x%02x%02x%02x%02x, position %d\n\n",
	      lref[j+7],lref[j+6],lref[j+5],lref[j+4],
	      lref[j+3],lref[j+2],lref[j+1],lref[j],
	      j);
      exit(1);
    }
  }
}

void _head_prep(ogg2_reference *head){
  int count=0;
  while(head){
    memset(head->buffer->data,0,head->buffer->size);
    count+=head->length;

    if(count>TESTBYTES/2+rand()%(TESTBYTES/4)){
      ogg2_buffer_release(head->next);
      head->next=0;
      break;
    }else{
      head=head->next;
    }
  }
}

void _write_linear_test(ogg2_reference *tail){
  ogg2byte_buffer ob;
  int i;

  _head_prep(tail);
  ogg2byte_init(&ob,tail,0);
  for(i=0;i<TESTBYTES;i++)
    ogg2byte_set1(&ob,ref[i],i);
  _read_linear_test1(tail);
  if(ogg2_buffer_length(tail)!=TESTBYTES){
    fprintf(stderr,"\nERROR: ogg2byte_set1 extended incorrectly.\n\n");
    exit(1);
  }

  _head_prep(tail);

  ogg2byte_init(&ob,tail,0);
  for(i=0;i<TESTBYTES;i+=2){
    unsigned int val=ref[i]|(ref[i+1]<<8);
    ogg2byte_set2(&ob,val,i);
  }
  _read_linear_test1(tail);
  if(ogg2_buffer_length(tail)>TESTBYTES){
    fprintf(stderr,"\nERROR: ogg2byte_set2 extended incorrectly.\n\n");
    exit(1);
  }

  _head_prep(tail);

  ogg2byte_init(&ob,tail,0);
  for(i=0;i<TESTBYTES;i+=4){
    unsigned long val=ref[i+2]|(ref[i+3]<<8);
    val=(val<<16)|ref[i]|(ref[i+1]<<8);
    ogg2byte_set4(&ob,val,i);
  }
  _read_linear_test1(tail);
  if(ogg2_buffer_length(tail)>TESTBYTES){
    fprintf(stderr,"\nERROR: ogg2byte_set4 extended incorrectly.\n\n");
    exit(1);
  }

  _head_prep(tail);

  ogg2byte_init(&ob,tail,0);
  for(i=0;i<TESTBYTES;i+=8){
    ogg_int64_t val=ref[i+6]|(ref[i+7]<<8);
    val=(val<<16)|ref[i+4]|(ref[i+5]<<8);
    val=(val<<16)|ref[i+2]|(ref[i+3]<<8);
    val=(val<<16)|ref[i]|(ref[i+1]<<8);
    ogg2byte_set8(&ob,val,i);
  }
  _read_linear_test1(tail);
  if(ogg2_buffer_length(tail)>TESTBYTES){
    fprintf(stderr,"\nERROR: ogg2byte_set8 extended incorrectly.\n\n");
    exit(1);
  }

}

void _write_zero_test(void){
  ogg2byte_buffer ob;
  int i;

  ogg2byte_init(&ob,0,bs);
  for(i=0;i<TESTBYTES;i++)
    ogg2byte_set1(&ob,ref[i],i);
  _read_linear_test1(ob.baseref);
  if(ogg2_buffer_length(ob.baseref)!=TESTBYTES){
    fprintf(stderr,"\nERROR: ogg2byte_set1 extended incorrectly.\n\n");
    exit(1);
  }
}

void _write_seek_test(void){
  ogg2byte_buffer ob;
  int i;

  memset(work,0,TESTBYTES);

  ogg2byte_init(&ob,0,bs);

  for(i=0;i<TESTBYTES;i++){
    int j=rand()%TESTBYTES;
    ogg2byte_set1(&ob,ref[j],j);
    work[j]=1;
  }

  _read_linear_test1b(ob.baseref);
}

int main(void){
  int i,j;
  bs=ogg2_buffer_create();

  /* test all codepaths through randomly generated data set */
  fprintf(stderr,"\nRandomized testing of byte aligned access abstraction: \n");

  for(i=0;i<1000;i++){

    /* fill reference array */
    for(j=0;j<TESTBYTES;j++)
      ref[j]=rand()%TESTBYTES;
  
    /* test basic reading using single synthetic reference 1,2,4,8 */
    fprintf(stderr,"\r\t loops left (%d),  basic read test... ",1000-i);
    {
      ogg2_buffer ob;
      ogg2_reference or;

      ob.data=ref;
      ob.size=TESTBYTES;
      or.buffer=&ob;
      or.begin=0;
      or.length=TESTBYTES;

      _read_linear_test1(&or);
      _read_linear_test2(&or);
      _read_linear_test4(&or);
      _read_linear_test8(&or);
    }

    /* test basic reading using multiple synthetic refs 1,2,4,8 */
    fprintf(stderr,"\r\t loops left (%d),  fragmented read test... ",1000-i);
    {
      ogg2_reference *tail=0;
      ogg2_reference *head=0;
      long count=0;
      
      
      while(count<TESTBYTES){
	int begin=rand()%32;
	int length=rand()%32;
	int pad=rand()%32;

	if(length>TESTBYTES-count)length=TESTBYTES-count;

	if(tail)
	  head=ogg2_buffer_extend(head,begin+length+pad);
	else
	  tail=head=ogg2_buffer_alloc(bs,begin+length+pad);

	memcpy(head->buffer->data+begin,ref+count,length);
	head->begin=begin;
	head->length=length;

	count+=length;
      }

      _read_linear_test1(tail);
      _read_linear_test2(tail);
      _read_linear_test4(tail);
      _read_linear_test8(tail);
      /* test reading with random seek using multiple synthetic refs */
      fprintf(stderr,"\r\t loops left (%d),  nonsequential read test... ",1000-i);
      _read_seek_test(tail);

      /* test writing, starting at offset zero in already built reference */
      fprintf(stderr,"\r\t loops left (%d),  linear write test...       ",1000-i);      
      _write_linear_test(tail);

      ogg2_buffer_release(tail);
    }    
    
    /* test writing, init blank reference */
    fprintf(stderr,"\r\t loops left (%d),  zero-start write test...   ",1000-i);      
    _write_zero_test();
    
    /* random writing, init blank ref */
    fprintf(stderr,"\r\t loops left (%d),  random-offset write test...   ",1000-i);      
    
    _write_seek_test();


  }
  fprintf(stderr,"\r\t all tests ok.                                              \n\n");
  ogg2_buffer_destroy(bs);
  return 0;
}

#endif
