#include <stdlib.h>
#include <stdio.h>
#include <ogg/ogg2.h>
#include "codec.h"

extern int headerinfo_p;
extern int packetinfo_p;
extern int truncpacket_p;
extern int warn_p;

static int ilog(unsigned long v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static int floor0_info_unpack(vorbis_info *vi,
			      ogg2pack_buffer *opb,
			      vorbis_info_floor0 *info){

  int j;
  unsigned long ret;

  ogg2pack_read(opb,8,&ret);
  info->order=ret;
  ogg2pack_read(opb,16,&ret);
  info->rate=ret;
  ogg2pack_read(opb,16,&ret);
  info->barkmap=ret;
  ogg2pack_read(opb,6,&ret);
  info->ampbits=ret;
  ogg2pack_read(opb,8,&ret);
  info->ampdB=ret;
  ogg2pack_read(opb,4,&ret);
  info->numbooks=ret+1;
  
  if(ogg2pack_eop(opb))goto eop;
  if(info->order<1)goto err;
  if(info->rate<1)goto err;
  if(info->barkmap<1)goto err;
    
  if(headerinfo_p)
    printf("(type 0; LSP)\n"
	   "             filter order            : %d (%s)\n"
	   "             filter sample rate      : %ld\n"
	   "             filter bark mapping size: %ld\n"
	   "             amplitude resolution    : %d bits\n"
	   "             amplitude range         : %d dB\n"
	   "             floor decode books      : %d books (",
	   info->order,(info->order%1?"odd":"even"),
	   info->rate,info->barkmap,info->ampbits,info->ampdB,
	   info->numbooks);
	   
  for(j=0;j<info->numbooks;j++){
    ogg2pack_read(opb,8,&ret);
    if(ogg2pack_eop(opb))goto eop;
    info->books[j]=ret;
    if(info->books[j]>=vi->books){
      if(headerinfo_p || warn_p)
	printf("\nWARN  floor: requested book (%lu) greater than highest number\n"
	       "               available book (%d).\n\n",ret,vi->books-1);
      goto err;
    }

    if(headerinfo_p){
      if(j%4==0 && j!=0)
	printf(")\n                                              ");
      printf("%3lu",ret);
      if(j+1>=info->numbooks)
	printf(")\n             ------------------------\n");
    }
  }
  
  if(ogg2pack_eop(opb))goto eop;
  return(0);
 eop:
  if(headerinfo_p || warn_p)
    printf("WARN  floor: Premature EOP while parsing floor.\n\n");
 err:
  if(headerinfo_p || warn_p)
    printf("WARN  floor: Invalid floor.\n");
  return(1);
}

int floor0_inverse(vorbis_info *vi,vorbis_info_floor0 *info,
		   ogg2pack_buffer *opb){
  int j;
  
  unsigned long ampraw;
  ogg2pack_read(opb,info->ampbits,&ampraw);
  if(ogg2pack_eop(opb))goto eop;

  if(ampraw>0){
    unsigned long booknum;

    if(packetinfo_p){
      long maxval=(1<<info->ampbits)-1;
      printf("amplitude %.1fdB (%lu)\n",
	     (float)ampraw/maxval*info->ampdB-info->ampdB,ampraw);
    }

    ogg2pack_read(opb,ilog(info->numbooks),&booknum);
    if(ogg2pack_eop(opb))goto eop;

    if((signed)booknum<info->numbooks){ /* be paranoid */
      codebook *b=vi->book_param+info->books[booknum];
      for(j=0;j<info->order;j+=b->dim)
	vorbis_book_decode(b,opb);
      if(ogg2pack_eop(opb))goto eop;
      return 1;
    }else{
      if(packetinfo_p || warn_p)
	printf("WARN packet: out of range codebook requested (%lu).\n"
	       "             zeroing packet energy.\n\n",
	       booknum);
      return 0;
    }
  } else {
    if(packetinfo_p)
      printf("amplitude zero\n");
    return 0;
  }
 eop:
  if(packetinfo_p || truncpacket_p)
    printf("info packet: packet truncated in floor encoding; zeroing\n"
	   "             floor energy.\n\n");
  return 0;
}

static const int quant_look[4]={256,128,86,64};

static int floor1_info_unpack(vorbis_info *vi,
			      ogg2pack_buffer *opb,
			      vorbis_info_floor1 *info){
  
  int j,k,count=0,maxclass=-1,rangebits;
  unsigned long ret;
  
  /* read partitions */
  ogg2pack_read(opb,5,&ret);
  info->partitions=ret;
  if(headerinfo_p)
    printf("(type 1; log piecewise)\n"
	   "             partitions       : %lu\n"
	   "             partition classes: ",ret);
  if(ogg2pack_eop(opb))goto eop;
  for(j=0;j<info->partitions;j++){
    ogg2pack_read(opb,4,&ret);
    if(ogg2pack_eop(opb))goto eop;
    info->partitionclass[j]=ret;
    if(headerinfo_p){
      printf("%lu",ret);
      if(j+1<info->partitions)printf(", ");
    }
    if(maxclass<info->partitionclass[j])
      maxclass=info->partitionclass[j];
  }
  if(headerinfo_p)
    printf("\n");
    
  /* read partition classes */
  for(j=0;j<maxclass+1;j++){
    if(headerinfo_p)
      printf("             class %d config   : ",j);
    ogg2pack_read(opb,3,&ret);
    info->class[j].class_dim=ret+1;
    ogg2pack_read(opb,2,&ret);
    info->class[j].class_subs=ret;
    if(ogg2pack_eop(opb)) goto eop;
    
    if(info->class[j].class_subs){
      ogg2pack_read(opb,8,&ret);
      info->class[j].class_book=ret;
      if(ogg2pack_eop(opb)) goto eop;
      
      if(headerinfo_p)
	printf("dim=%d, subs=%d (1<<%d), book=%lu\n",
	       info->class[j].class_dim,1<<info->class[j].class_subs,
	       info->class[j].class_subs,
	       ret);
      
      if(info->class[j].class_book>=vi->books){
	if(warn_p || headerinfo_p)
	  printf("WARN header: Class book number (%lu) is greater than highest\n"
		 "             numbered available codebook (%d).\n\n",
		 ret,vi->books-1);
	goto err;
      }
    }else{
      info->class[j].class_book=0;
      if(headerinfo_p)
	printf("dim=%d no subclasses\n",
	       info->class[j].class_dim);
    }

    if(headerinfo_p)
      printf("                               (subbooks: ");
    for(k=0;k<(1<<info->class[j].class_subs);k++){
      ogg2pack_read(opb,8,&ret);
      info->class[j].class_subbook[k]=ret-1;
      if(ogg2pack_eop(opb))goto eop;
      if(headerinfo_p){
	if(info->class[j].class_subbook[k]==-1)
	  printf(" x");
	else
	  printf("%d",info->class[j].class_subbook[k]);
	if(k+1<(1<<info->class[j].class_subs))
	  printf(", ");
      }
      if(info->class[j].class_subbook[k]>=vi->books){
	if(warn_p || headerinfo_p)
	  printf("\nWARN header: Class subbook number (%lu) is greater than highest\n"
		 "             numbered available codebook (%d).\n\n",
		 ret-1,vi->books-1);
	
	goto err;
      }
    }
    if(headerinfo_p)
      printf(")\n");
  }
  
  /* read the post list */
  ogg2pack_read(opb,2,&ret);
  info->mult=ret+1;     /* only 1,2,3,4 legal now */ 
  if(headerinfo_p)
    printf("             multiplier       : %d (number %d)\n",quant_look[ret],
	   info->mult);
  
  ogg2pack_read(opb,4,&ret);
  rangebits=ret;
  if(headerinfo_p)
    printf("             post range       : %d bits\n"
	   "             post values      : ",info->mult);
  
  count=0;
  for(j=0,k=0;j<info->partitions;j++){
    count+=info->class[info->partitionclass[j]].class_dim; 
    for(;k<count;k++){
      int t;
      ogg2pack_read(opb,rangebits,&ret);
      t=ret;
      
      if(headerinfo_p){
	if(k!=0 && k%4==0)
	  printf("\n                                ");
	printf("%5d",t);
	if(k+1<count || j+1<info->partitions)
	  printf(",");
      }
      
      if(t>=(1<<rangebits)){
	if(headerinfo_p || warn_p)
	  printf("\nWARN header: Post value (%d) out of range.\n\n",t);
	goto err;
      }
    }
  }
  if(headerinfo_p)
    printf("\n             -----------------\n");

  if(ogg2pack_eop(opb))goto eop;
  info->posts=count+2;
  
  return 0;
 eop: 
  if(headerinfo_p || warn_p)
    printf("WARN header: Premature EOP parsing floor config.\n\n");
 err:
  if(headerinfo_p || warn_p)
    printf("WARN header: Floor configuration is invalid.\n");
  return 1;
}

int floor1_inverse(vorbis_info *vi,vorbis_info_floor1 *info,
		   ogg2pack_buffer *opb){
  int i,j,k;
  codebook *books=vi->book_param;   
  unsigned long ret;

  /* unpack wrapped/predicted values from stream */
  if(ogg2pack_read1(opb)==1){
    int quant_q=quant_look[info->mult-1];
    if(packetinfo_p)
      printf("amplitude nonzero\n");
    ogg2pack_read(opb,ilog(quant_q-1),&ret);
    ogg2pack_read(opb,ilog(quant_q-1),&ret);
    
    /* partition by partition */
    /* partition by partition */
    for(i=0,j=2;i<info->partitions;i++){
      ogg_int16_t classv=info->partitionclass[i];
      ogg_int16_t cdim=info->class[classv].class_dim;
      ogg_int16_t csubbits=info->class[classv].class_subs;
      ogg_int32_t csub=1<<csubbits;
      ogg_int32_t cval=0;

      /* decode the partition's first stage cascade value */
      if(csubbits){
	cval=vorbis_book_decode(books+info->class[classv].class_book,opb);
	if(cval==-1)goto eop;
      }

      for(k=0;k<cdim;k++){
	ogg_uint16_t book=info->class[classv].class_subbook[cval&(csub-1)];
	cval>>=csubbits;
	if(book!=0xffff){
	  if(vorbis_book_decode(books+book,opb)==-1)
	    goto eop;
	}
      }
      j+=cdim;
    }
    
    return 1;
  }else{
    if(packetinfo_p)
      printf("amplitude zero\n");
    return 0;
  }
 eop:
  if(packetinfo_p || truncpacket_p)
    printf("info packet: packet truncated in floor encoding; zeroing\n"
	   "             floor energy.\n");
  return 0;
}

int floor_info_unpack(vorbis_info *vi,
		       ogg2pack_buffer *opb,
		       vorbis_info_floor *fi){
  unsigned long ret,i;
  ogg2pack_read(opb,16,&ret);
  fi->type=ret;
  if(ogg2pack_eop(opb)){
    if(headerinfo_p || warn_p)
      printf("WARN header: Premature EOP parsing floor setup.\n");
    return 1;
  }
  if(ret>=2){
    if(headerinfo_p || warn_p)
      printf("WARN header: Floor %lu is an illegal type (%lu).\n\n",
	     i,ret);
    return 1;
  }
  
  if(fi->type){
    if(floor1_info_unpack(vi,opb,&fi->floor.floor1))return 1;
  }else{
    if(floor0_info_unpack(vi,opb,&fi->floor.floor0))return 1;
  }
  return 0;
}

