#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ogg/ogg2.h>
#include "codec.h"

extern int codebook_p;
extern int headerinfo_p;
extern int packetinfo_p;
extern int truncpacket_p;
extern int warn_p;

static int ilog(unsigned long v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

int mapping_info_unpack(vorbis_info_mapping *info,vorbis_info *vi,
			ogg2pack_buffer *opb){
  int i;
  unsigned long ret;
  memset(info,0,sizeof(*info));

  ogg2pack_read(opb,16,&ret);
  if(ogg2pack_eop(opb))goto eop;
  switch(ret){
  case 0:
    if(headerinfo_p)
      printf("(type 0; mono/polyphonic/standard surround)\n");
    break;
  default:
    if(headerinfo_p || warn_p)
      printf("\n\nWARN header: Illegal mapping type %lu. Invalid header.\n",
	     ret);
    return 1;
  }
  
  if(ogg2pack_read1(opb)){
    ogg2pack_read(opb,4,&ret);
    info->submaps=ret+1;
    if(ogg2pack_eop(opb))goto eop;
    if(headerinfo_p)
      printf("             multi-submap mapping\n"
	     "             submaps               : %d\n",info->submaps);
  }else{
    info->submaps=1;

    if(headerinfo_p)
      printf("             single-submap mapping\n"
	     "             submaps               : %d\n",info->submaps);
  }

  if(ogg2pack_read1(opb)){
    ogg2pack_read(opb,8,&ret);
    info->coupling_steps=ret+1;
    if(ogg2pack_eop(opb))goto eop;
    if(headerinfo_p)
      printf("             channel coupling flag : set\n"
	     "             coupling steps        : %d\n",
	     info->coupling_steps);

    for(i=0;i<info->coupling_steps;i++){
      unsigned long testM;
      unsigned long testA;

      ogg2pack_read(opb,ilog(vi->channels),&testM);
      ogg2pack_read(opb,ilog(vi->channels),&testA);
      info->coupling[i].mag=testM;
      info->coupling[i].ang=testA;
      if(ogg2pack_eop(opb))goto eop;
      
      if(headerinfo_p)
	printf("             coupling step %3d     : %lu <-> %lu\n",
	       i,testM,testA);
      
      if(testM==testA || 
	 testM>=(unsigned)vi->channels ||
	 testA>=(unsigned)vi->channels) {
	if(headerinfo_p || warn_p){
	  printf("WARN header: Illegal channel coupling declaration.\n\n");
	  
	}
	goto err_out;
      }
    }
    
  }else{
    if(headerinfo_p)
      printf("             channel coupling flag: not set\n");
  }
  
  if(ogg2pack_read1(opb)){
    if(ogg2pack_eop(opb))goto eop;
    if(headerinfo_p || warn_p)
      printf("WARN header: Reserved fields not zeroed\n\n");
    goto err_out; /* 2:reserved */
  }
  if(ogg2pack_read1(opb)){
    if(ogg2pack_eop(opb))goto eop;
    if(headerinfo_p || warn_p)
      printf("WARN header: Reserved fields not zeroed\n\n");
    goto err_out; /* 3:reserved */
  }
  
  if(info->submaps>1){
    if(headerinfo_p)
      printf("             channel->submap list  : ");

    for(i=0;i<vi->channels;i++){
      ogg2pack_read(opb,4,&ret);
      info->chmuxlist[i]=ret;
      if(ogg2pack_eop(opb))goto eop;

      if(headerinfo_p)
	printf("%d/%lu ",i,ret);
      
      if(info->chmuxlist[i]>=info->submaps){
	if(headerinfo_p || warn_p)
	  printf("\n\nWARN header: Requested submap (%lu) out of range.\n\n",
		 ret);
	goto err_out;
      }
    }
    if(headerinfo_p)
      printf("\n");
  }

  for(i=0;i<info->submaps;i++){
    if(headerinfo_p)
      printf("             submap %2d config      : ",i);
    ogg2pack_read(opb,8,&ret);
    ogg2pack_read(opb,8,&ret);
    info->submaplist[i].floor=ret;
    if(ogg2pack_eop(opb))goto eop;
    if(headerinfo_p)
      printf("floor %lu, ",ret);
    if(info->submaplist[i].floor>=vi->floors){
      if(headerinfo_p || warn_p)
	printf("WARN header: Requested floor (%lu) out of range.\n\n",ret);
      goto err_out;
    }
    ogg2pack_read(opb,8,&ret);
    if(ogg2pack_eop(opb))goto eop;
    info->submaplist[i].residue=ret;
    if(headerinfo_p)
      printf("res %lu ",ret);
    if(info->submaplist[i].residue>=vi->residues){
      if(headerinfo_p || warn_p)
	printf("WARN header: Requested residue (%lu) out of range.\n\n",ret);
      goto err_out;
    }
    if(headerinfo_p)
      printf("\n");

  }
  if(headerinfo_p)
    printf("             ----------------------\n");

  return 0;
 eop:
  if(headerinfo_p || warn_p)
    printf("WARN header: Premature EOP while parsing mapping config.\n\n");
 err_out:
  if(headerinfo_p || warn_p)
    printf("WARN header: Invalid mapping.\n\n");
  return -1;
}

int mapping_inverse(vorbis_info *vi,vorbis_info_mapping *info,
			    ogg2pack_buffer *opb){
  int   i,j;
  int  *zerobundle=alloca(sizeof(*zerobundle)*vi->channels);
  int  *nonzero=alloca(sizeof(*nonzero)*vi->channels);
  
  for(i=0;i<vi->channels;i++){
    int submap=0;
    int floorno;
    
    if(info->submaps>1)submap=info->chmuxlist[i];
    floorno=info->submaplist[submap].floor;
    
    if(packetinfo_p)
      printf("             channel %2d floor: ",i);

    if(vi->floor_param[floorno].type){
      /* floor 1 */
      nonzero[i]=floor1_inverse(vi,&vi->floor_param[floorno].floor.floor1,opb);
    }else{
      /* floor 0 */
      nonzero[i]=floor0_inverse(vi,&vi->floor_param[floorno].floor.floor0,opb);
    }
  }

  /* channel coupling can 'dirty' the nonzero listing */
  for(i=0;i<info->coupling_steps;i++){
    if(nonzero[info->coupling[i].mag] ||
       nonzero[info->coupling[i].ang]){
      nonzero[info->coupling[i].mag]=1; 
      nonzero[info->coupling[i].ang]=1; 
    }
  }

  /* recover the residue into our working vectors */
  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(!info->chmuxlist || info->chmuxlist[j]==i){
	if(nonzero[j])
	  zerobundle[ch_in_bundle++]=1;
	else
	  zerobundle[ch_in_bundle]=0;
      }
    }
    
    res_inverse(vi,vi->residue_param+info->submaplist[i].residue,
		zerobundle,ch_in_bundle,opb);
  }


  return(0);
}


int vorbis_decode(vorbis_info *vi,ogg2_packet *op){
  int                   mode;
  unsigned long         ret;
  ogg2pack_buffer       *opb=alloca(ogg2pack_buffersize());
  ogg2pack_readinit(opb,op->packet);

  if(packetinfo_p)
    printf("info packet: decoding Vorbis stream packet %ld\n",
	   (long)op->packetno);
  
  /* Check the packet type */
  if(ogg2pack_read1(opb)!=0){
    if(warn_p || packetinfo_p)
      printf("WARN packet: packet is not an audio packet! Skipping...\n\n");
    return 1 ;
  }
  
  /* read our mode and pre/post windowsize */
  ogg2pack_read(opb,ilog(vi->modes),&ret);
  mode=ret;
  if(ogg2pack_eop(opb))goto eop;
  if(packetinfo_p)
    printf("             packet mode     : %d\n",mode);
  if(mode>=vi->modes){
    if(warn_p || packetinfo_p)
      printf("WARN packet: packet mode out of range! Skipping...\n\n");
    return 1;
  }

  if(packetinfo_p)
    printf("             block size      : %d\n",
	   vi->blocksizes[vi->mode_param[mode].blockflag]);
  
  if(vi->mode_param[mode].blockflag){
    ogg2pack_read(opb,1,&ret);
    if(ogg2pack_eop(opb))goto eop;
    if(packetinfo_p)
      printf("             previous        : %d\n",
	     vi->blocksizes[ret!=0]);
    ogg2pack_read(opb,1,&ret);
    if(ogg2pack_eop(opb))goto eop;
    if(packetinfo_p)
      printf("             next            : %d\n",
	     vi->blocksizes[ret!=0]);
  }
  
  /* packet decode and portions of synthesis that rely on only this block */
  mapping_inverse(vi,vi->map_param+vi->mode_param[mode].mapping,opb);
  if(packetinfo_p)
    printf("\n");

  return 0;
 eop:
  if(warn_p || packetinfo_p)
    printf("WARN packet: Premature EOP while parsing packet mode.\n\n");
  return 0;

}
