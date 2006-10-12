/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: basic codebook pack/unpack/code/decode operations

 ********************************************************************/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ogg/ogg2.h>
#include "codec.h"

extern int codebook_p;
extern int headerinfo_p;
extern int warn_p;

/**** pack/unpack helpers ******************************************/
int _ilog(long v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

#define VQ_FEXP 10
#define VQ_FMAN 21
#define VQ_FEXP_BIAS 768 /* bias toward values smaller than 1. */

float _float32_unpack(long unsigned val){
  double mant=val&0x1fffff;
  int    sign=val&0x80000000;
  long   exp =(val&0x7fe00000L)>>VQ_FMAN;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-(VQ_FMAN-1)-VQ_FEXP_BIAS));
}

/* given a list of word lengths, number of used entries, and byte
   width of a leaf, generate the decode table */
static int _make_words(char *l,
		       codebook *b){
  long i,j,count=0;
  long top=0;
  unsigned long marker[33];

  b->dec_table=malloc((b->used_entries*2+1)*sizeof(*b->dec_table));
  
  if(b->entries<2){
    b->dec_table[0]=0x80000000;
  }else{
    memset(marker,0,sizeof(marker));
    
    for(i=0;i<b->entries;i++){
      long length=l[i];
      if(length){
	unsigned long entry=marker[length];
	long chase=0;
	if(count && !entry){
	  if(codebook_p || headerinfo_p || warn_p)
	    printf("WARN codebk: Malformed [overpopulated] Huffman tree.\n"
		   "             Codebook is invalid.\n\n");
	  return -1; /* overpopulated tree! */
	}
	
	/* chase the tree as far as it's already populated, fill in past */
	for(j=0;j<length-1;j++){
	  ogg_int16_t bit=(entry>>(length-j-1))&1;
	  if(chase>=top){ 
	    top++;
	    b->dec_table[chase*2]=top;
	    b->dec_table[chase*2+1]=0;
	  }else
	    if(!b->dec_table[chase*2+bit])
	      b->dec_table[chase*2+bit]=top;
	  chase=b->dec_table[chase*2+bit];
	}
	{	
	  ogg_int16_t bit=(entry>>(length-j-1))&1;
	  if(chase>=top){ 
	    top++;
	    b->dec_table[chase*2+1]=0;
	  }
	  b->dec_table[chase*2+bit]= i | 0x80000000;
	}

	/* Look to see if the next shorter marker points to the node
	   above. if so, update it and repeat.  */
	for(j=length;j>0;j--){          
	  if(marker[j]&1){
	    marker[j]=marker[j-1]<<1;
	    break;
	  }
	  marker[j]++;
	}
	
	/* prune the tree; the implicit invariant says all the longer
	   markers were dangling from our just-taken node.  Dangle them
	   from our *new* node. */
	for(j=length+1;j<33;j++)
	  if((marker[j]>>1) == entry){
	    entry=marker[j];
	    marker[j]=marker[j-1]<<1;
	  }else
	    break;
      }
    }
  }
  
  return 0;
}

long _book_maptype1_quantvals(codebook *b){
  /* get us a starting hint, we'll polish it below */
  int bits=_ilog(b->entries);
  long vals=b->entries>>((bits-1)*(b->dim-1)/b->dim);

  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<b->dim;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=b->entries && acc1>b->entries){
      return(vals);
    }else{
      if(acc>b->entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}

int vorbis_book_unpack(ogg2pack_buffer *opb,codebook *s){
  char *lengthlist=0;
  long quantvals=0;
  long i,j;
  unsigned long maptype;
  unsigned long ret;

  memset(s,0,sizeof(*s));
  ogg2pack_read(opb,24,&ret);
  if(ogg2pack_eop(opb))goto eop;
  if(ret!=0x564342 && (warn_p || codebook_p || headerinfo_p)){
    printf("WARN codebk: Sync sequence (0x%lx) incorrect (!=0x564342)\n"
	   "             Corrupt codebook.\n",ret);
    goto err;
  }

  /* first the basic parameters */
  ogg2pack_read(opb,16,&ret);
  s->dim=ret;
  ogg2pack_read(opb,24,&ret);
  s->entries=ret;

  /* codeword ordering.... length ordered or unordered? */
  ogg2pack_read(opb,1,&ret);
  if(ogg2pack_eop(opb))goto eop;

  switch(ret){
  case 0:
    /* unordered */
    lengthlist=alloca(sizeof(*lengthlist)*s->entries);

    /* allocated but unused entries? */
    if(ogg2pack_read1(opb)){
      /* yes, unused entries */

      for(i=0;i<s->entries;i++){
	if(ogg2pack_read1(opb)){
	  unsigned long num;
	  ogg2pack_read(opb,5,&num);
	  if(ogg2pack_eop(opb))goto eop;
	  lengthlist[i]=num+1;
	  s->used_entries++;
	}else
	  lengthlist[i]=0;
      }

      if(codebook_p){
	printf("             Entry encoding : 0,1 (Unordered, sparse)\n");
	printf("             Entries        : %d total (%d used)\n",
	       s->entries,s->used_entries);
      }
      
    }else{
      /* all entries used; no tagging */
      s->used_entries=s->entries;
      for(i=0;i<s->entries;i++){
	unsigned long num;
	ogg2pack_read(opb,5,&num);
	if(ogg2pack_eop(opb))goto eop;
	lengthlist[i]=num+1;
      }

      if(codebook_p){
	printf("             Entry encoding : 0,0 (Unordered, fully populated)\n");
	printf("             Entries        : %d\n",
	       s->entries);
      }

    }
    
    break;
  case 1:
    /* ordered */
    {
      unsigned long length;
      ogg2pack_read(opb,5,&length);
      length++;
      s->used_entries=s->entries;
      lengthlist=alloca(sizeof(*lengthlist)*s->entries);
      
      for(i=0;i<s->entries;){
	unsigned long num;
	ogg2pack_read(opb,_ilog(s->entries-i),&num);
	if(ogg2pack_eop(opb))goto eop;
	for(j=0;j<(signed)num && i<s->entries;j++,i++)
	  lengthlist[i]=length;
	length++;
      }
      
      if(codebook_p){
	printf("             Entry encoding : 1 (Ordered, fully populated)\n");
	printf("             Entries        : %d\n",
	       s->entries);
      }
    }
    break;
  default:
    if(codebook_p)
      printf("WARN codebk: Illegal entry encoding %lu.  Codebook is corrupt.\n",
	     ret);
    /* EOF */
    goto err;
  }


  /* Do we have a mapping to unpack? */
  ogg2pack_read(opb,4,&maptype);
  if(maptype>0){
    unsigned long q_min,q_del,q_bits,q_seq;

    if(maptype==1){
      quantvals=_book_maptype1_quantvals(s);
    }else{
      quantvals=s->entries*s->dim;
    }

    ogg2pack_read(opb,32,&q_min);
    ogg2pack_read(opb,32,&q_del);
    ogg2pack_read(opb,4,&q_bits);
    q_bits++;
    ogg2pack_read(opb,1,&q_seq);
    if(ogg2pack_eop(opb))goto eop;

    if(codebook_p)
      printf("             Value mapping  : %lu (%s)\n"
	     "             Values         : %ld\n"
	     "             Min value      : %f (%lx)\n"
	     "             Delta value    : %f (%lx)\n"
	     "             Multiplier bits: %lu\n"
	     "             Sequential flag: %s\n"
	     "             ---------------\n",
	     maptype,(maptype==1?"lattice":"tessellated"),
	     quantvals,
	     _float32_unpack(q_min),q_min,
	     _float32_unpack(q_del),q_del,
	     q_bits,q_seq?"set":"unset");

    for(i=0;i<quantvals;i++)
      ogg2pack_read(opb,q_bits,&ret);

    if(ogg2pack_eop(opb))goto eop;

  }else{
    if(codebook_p)
      printf("             Value mapping  : 0 (none)\n"
	     "             ---------------\n");
	    
  }

  if(_make_words(lengthlist,s)) goto err;

  return 0;
 eop:
  if(codebook_p || headerinfo_p || warn_p)
    printf("WARN codebk: Premature EOP while parsing codebook.\n");
 err:
  if(codebook_p || headerinfo_p || warn_p)
    printf("WARN codebk: Invalid codebook; stream is undecodable.\n");
  return -1;
}

long vorbis_book_decode(codebook *book, ogg2pack_buffer *b){
  unsigned long chase=0;
  int           read=32;
  unsigned long lok;
  long          i;
  int eop=ogg2pack_look(b,read,&lok);
  
  while(eop && read>1)
    eop = ogg2pack_look(b, --read, &lok);
  if(eop<0){
    ogg2pack_adv(b,32); /* make sure to trigger EOP! */
    return -1;
  }

  /* chase the tree with the bits we got */
  for(i=0;i<read;i++){
    chase=book->dec_table[chase*2+((lok>>i)&1)];
    if(chase&0x80000000UL)break;
  }
  chase&=0x7fffffffUL;
  
  if(i<read){
    ogg2pack_adv(b,i+1);
    return chase;
  }
  ogg2pack_adv(b,32); /* make sure to trigger EOP! */
  return -1;
}

int vorbis_book_clear(codebook *b){
  if(b->dec_table)free(b->dec_table);
  memset(b,0,sizeof(*b));
  return 0;
}
