/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggTheora SOURCE CODE IS (C) COPYRIGHT 1994-2008             *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function: packing variable sized words into an octet stream
  last mod: $Id$

 ********************************************************************/

/*We're 'MSb' endian; if we write a word but read individual bits,
   then we'll read the MSb first.*/

#include <string.h>
#include <stdlib.h>
#include "bitpack.h"

#if !defined(NEW_BITPACK)
void theorapackB_readinit(oggpack_buffer *_b,unsigned char *_buf,int _bytes){
  memset(_b,0,sizeof(*_b));
  _b->buffer=_b->ptr=_buf;
  _b->storage=_bytes;
}

int theorapackB_look1(oggpack_buffer *_b,long *_ret){
  if(_b->endbyte>=_b->storage){
    *_ret=0L;
    return -1;
  }
  *_ret=(_b->ptr[0]>>7-_b->endbit)&1;
  return 0;
}

void theorapackB_adv1(oggpack_buffer *_b){
  if(++(_b->endbit)>7){
    _b->endbit=0;
    _b->ptr++;
    _b->endbyte++;
  }
}

/*Here we assume that 0<=_bits&&_bits<=32.*/
int theorapackB_read(oggpack_buffer *_b,int _bits,long *_ret){
  long ret;
  long m;
  long d;
  int fail;
  m=32-_bits;
  _bits+=_b->endbit;
  d=_b->storage-_b->endbyte;
  if(d<=4){
    /*Not the main path.*/
    if(d*8<_bits){
      *_ret=0L;
      fail=-1;
      goto overflow;
    }
    /*Special case to avoid reading _b->ptr[0], which might be past the end of
       the buffer; also skips some useless accounting.*/
    else if(!_bits){
      *_ret=0L;
      return 0;
    }
  }
  ret=_b->ptr[0]<<24+_b->endbit;
  if(_bits>8){
    ret|=_b->ptr[1]<<16+_b->endbit;
    if(_bits>16){
      ret|=_b->ptr[2]<<8+_b->endbit;
      if(_bits>24){
        ret|=_b->ptr[3]<<_b->endbit;
        if(_bits>32)ret|=_b->ptr[4]>>8-_b->endbit;
      }
    }
  }
  *_ret=((ret&0xFFFFFFFFUL)>>(m>>1))>>(m+1>>1);
  fail=0;
overflow:
  _b->ptr+=_bits>>3;
  _b->endbyte+=_bits>>3;
  _b->endbit=_bits&7;
  return fail;
}

int theorapackB_read1(oggpack_buffer *_b,long *_ret){
  int fail;
  if(_b->endbyte>=_b->storage){
    /*Not the main path.*/
    *_ret=0L;
    fail=-1;
  }
  else{
    *_ret=(_b->ptr[0]>>7-_b->endbit)&1;
    fail=0;
  }
  _b->endbit++;
  if(_b->endbit>7){
    _b->endbit=0;
    _b->ptr++;
    _b->endbyte++;
  }
  return fail;
}

long theorapackB_bytes(oggpack_buffer *_b){
  return _b->endbyte+(_b->endbit+7>>3);
}

long theorapackB_bits(oggpack_buffer *_b){
  return _b->endbyte*8+_b->endbit;
}

unsigned char *theorapackB_get_buffer(oggpack_buffer *_b){
  return _b->buffer;
}
#endif


#define BIT_WINDOW_SIZE 32
#define BIT_WINDOW_MASK 0xFFFFFFFFUL

void theorapackC_readinit(oggpack_buffer *_b,unsigned char *_buf,int _bytes,int _bits){
  memset(_b,0,sizeof(*_b));
  _b->endbit=-_bits;
  _b->ptr=_buf;
  _b->buffer=_buf+_bytes;
}

static unsigned int inline theorapackC_refill(oggpack_buffer *_b,int _bits)
{
  int available=_b->endbit;
  unsigned int window=_b->storage&BIT_WINDOW_MASK;
  unsigned char const *ptr=_b->ptr;
  unsigned char const *stop=_b->buffer;

  if (ptr>=stop)
    available=0x40000000;
  while(available<=BIT_WINDOW_SIZE-8)
  {
    available+=8;
    window|=*ptr++<<(BIT_WINDOW_SIZE-available);
    if (ptr>=stop)
      available=0x40000000;
  }
  _b->ptr=(unsigned char *)ptr;
  if (_bits>available)
    window|=*ptr>>(available&7);

  _b->endbit=available;
  return window&BIT_WINDOW_MASK;
}

int theorapackC_look1(oggpack_buffer *_b){
  int available=_b->endbit;
  unsigned int window=_b->storage&BIT_WINDOW_MASK;
  if (1>available)_b->storage=window=theorapackC_refill(_b,1);
  return window>>(BIT_WINDOW_SIZE-1);
}

void theorapackC_adv1(oggpack_buffer *_b){
  _b->storage<<=1;
  _b->endbit--;
}

/*Here we assume that 0<=_bits&&_bits<=32.*/
long theorapackC_read(oggpack_buffer *_b,int _bits){
  int available=_b->endbit;
  unsigned int window=_b->storage&BIT_WINDOW_MASK;
  long result;
  if(_bits==0)return 0;
  if(_bits>available){
    window=theorapackC_refill(_b,_bits);
    available=_b->endbit;
  }
  result=window>>(BIT_WINDOW_SIZE-_bits);
  available-=_bits;
  window<<=1;
  window<<=_bits-1;
  _b->endbit=available;
  _b->storage=window;
  return result;
}

int theorapackC_read1(oggpack_buffer *_b){
  int available=_b->endbit;
  unsigned int window=_b->storage&BIT_WINDOW_MASK;
  int result;
  if (1>available){
    window=theorapackC_refill(_b,1);
    available=_b->endbit;
  }
  result=window>>(BIT_WINDOW_SIZE-1);
  available--;
  window<<=1;
  _b->endbit=available;
  _b->storage=window;
  return result;
}
