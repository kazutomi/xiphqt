#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ogg/ogg2.h>
#include "codec.h"

extern int headerinfo_p;
extern int packetinfo_p;
extern int truncpacket_p;
extern int warn_p;

/* vorbis_info is for range checking */
int res_unpack(vorbis_info_residue *info,
	       vorbis_info *vi,ogg2pack_buffer *opb){
  int j,k;
  unsigned long ret;

  memset(info,0,sizeof(*info));

  ogg2pack_read(opb,16,&ret);
  info->type=ret;
  ogg2pack_read(opb,24,&ret);
  info->begin=ret;
  ogg2pack_read(opb,24,&ret);
  info->end=ret;
  ogg2pack_read(opb,24,&ret);
  info->grouping=ret+1;
  ogg2pack_read(opb,6,&ret);
  info->partitions=ret+1;
  ogg2pack_read(opb,8,&ret);
  info->groupbook=ret;
  if(ogg2pack_eop(opb))goto eop;

  if(info->type>2 || info->type<0){
    if(warn_p || headerinfo_p)
      printf("WARN header: illegal residue type (%d)\n\n",info->type);
    goto errout;
  }

  if(headerinfo_p){
    switch(info->type){
    case 0:
      printf("(type 0; interleaved values only)\n");
      break;
    case 1:
      printf("(type 1; uninterleaved)\n");
      break;
    case 2:
      printf("(type 2; interleaved channels only)\n");
      break;
    }
    printf("             sample range      : %ld through %ld\n"
	   "             partition size    : %d\n"
	   "             partition types   : %d\n"
	   "             partition book    : %d\n",
	   info->begin,info->end,info->grouping,info->partitions,
	   info->groupbook);
  }

  if(info->groupbook>=vi->books){
    if(warn_p || headerinfo_p)
      printf("WARN header: requested partition book (%d) greater than\n"
	     "             highest numbered available codebook (%d)\n\n",
	     info->groupbook,vi->books-1);
    goto errout;
  }
  
  for(j=0;j<info->partitions;j++){
    unsigned long cascade;
    ogg2pack_read(opb,3,&cascade);
    if(ogg2pack_read1(opb)){
      ogg2pack_read(opb,5,&ret);
      cascade|=(ret<<3);
    }
    info->stagemasks[j]=cascade;
  }
  
  for(j=0;j<info->partitions;j++){
    if(headerinfo_p)
      printf("             partition %2d books: ",j);
    for(k=0;k<8;k++){
      if((info->stagemasks[j]>>k)&1){
	ogg2pack_read(opb,8,&ret);
	if((signed)ret>=vi->books){
	  printf("\nWARN header: requested residue book (%lu) greater than\n"
		 "             highest numbered available codebook (%d)\n\n",
		 ret,vi->books-1);
	  goto errout;
	}
	info->stagebooks[j*8+k]=ret;
	if(k+1>info->stages)info->stages=k+1;
	if(headerinfo_p)
	  printf("%3d ",(int)ret);
      }else{
	info->stagebooks[j*8+k]=-1;
	if(headerinfo_p)
	  printf("... ");
      }
    }
    if(headerinfo_p)
      printf("\n");
  }
  if(headerinfo_p)
    printf("             ------------------\n");    
  
  if(ogg2pack_eop(opb))goto eop;
  return 0;
 eop:
  if(headerinfo_p || warn_p)
    printf("WARN header: Premature EOP while parsing residue.\n\n");
 errout:
  if(headerinfo_p || warn_p)
    printf("WARN header: Invalid residue.\n\n");
  return 1;
}

int res_inverse(vorbis_info *vi,
		vorbis_info_residue *info,
		int *nonzero,int ch,
		ogg2pack_buffer *opb){
  
  long i,j,k,l,s,used=0;
  codebook *phrasebook=vi->book_param+info->groupbook;
  long samples_per_partition=info->grouping;
  long partitions_per_word=phrasebook->dim;
  long n=info->end-info->begin;
  long partvals=n/samples_per_partition;
  long partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int **partword=0;

  if(info->type<2){
    for(i=0;i<ch;i++)if(nonzero[i])used++;
    ch=used;
    if(!used){
      if(packetinfo_p)
	printf("             zero spectral energy, residue decode skipped\n");
      return 0;
    }
  }else{
    for(i=0;i<ch;i++)if(nonzero[i])break;
    if(i==ch){
      if(packetinfo_p)
	printf("             zero spectral energy, residue decode skipped\n");
      return 0;
    }
    ch=1;
  }

  partword=alloca(ch*sizeof(*partword));
  for(j=0;j<ch;j++)
    partword[j]=alloca(partwords*partitions_per_word*sizeof(*partword[j]));
  
  for(s=0;s<info->stages;s++){
    
    for(i=0;i<partvals;){
      if(s==0){
	/* fetch the partition word for each channel */
	
	partword[0][i+partitions_per_word-1]=1;
	for(k=partitions_per_word-2;k>=0;k--)
	  partword[0][i+k]=partword[0][i+k+1]*info->partitions;
	
	for(j=1;j<ch;j++)
	  for(k=partitions_per_word-1;k>=0;k--)
	    partword[j][i+k]=partword[j-1][i+k];
	
	for(j=0;j<ch;j++){
	  ogg_int32_t temp=vorbis_book_decode(phrasebook,opb);
	  if(ogg2pack_eop(opb))goto eopbreak1;
	  
	  /* this can be done quickly in assembly due to the quotient
	     always being at most six bits */
	  for(k=0;k<partitions_per_word;k++){
	    ogg_uint32_t div=partword[j][i+k];
	    partword[j][i+k]=temp/div;
	    temp-=partword[j][i+k]*div;
	  }
	}
      }
      
      /* now we decode residual values for the partitions */
      for(k=0;k<partitions_per_word && i<partvals;k++,i++){
	for(j=0;j<ch;j++){
	  if(info->stagemasks[partword[j][i]]&(1<<s)){
	    codebook *stagebook=vi->book_param+
	      info->stagebooks[(partword[j][i]<<3)+s];
	    int step=samples_per_partition/stagebook->dim;
	    for (l=0;l<step;l++)
	      if(vorbis_book_decode(stagebook,opb)==-1)
		goto eopbreak1;
	  }
	}
      }
    }
  }

  return 0;
 eopbreak1:
  if(packetinfo_p || truncpacket_p)
    printf("info packet: packet truncated in residue decode\n\n");

  return 0;
}
