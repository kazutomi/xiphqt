#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ogg2/ogg.h>
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
			oggpack_buffer *opb){
  int i;
  unsigned long ret;
  memset(info,0,sizeof(*info));

  oggpack_read(opb,16,&ret);
  if(oggpack_eop(opb))goto eop;
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
  
  if(oggpack_read1(opb)){
    oggpack_read(opb,4,&ret);
    info->submaps=ret+1;
    if(oggpack_eop(opb))goto eop;
    if(headerinfo_p)
      printf("             multi-submap mapping\n"
	     "             submaps               : %d\n",info->submaps);
  }else{
    info->submaps=1;

    if(headerinfo_p)
      printf("             single-submap mapping\n"
	     "             submaps               : %d\n",info->submaps);
  }

  if(oggpack_read1(opb)){
    oggpack_read(opb,8,&ret);
    info->coupling_steps=ret+1;
    if(oggpack_eop(opb))goto eop;
    if(headerinfo_p)
      printf("             channel coupling flag : set\n"
	     "             coupling steps        : %d\n",
	     info->coupling_steps);

    for(i=0;i<info->coupling_steps;i++){
      unsigned long testM;
      unsigned long testA;

      oggpack_read(opb,ilog(vi->channels),&testM);
      oggpack_read(opb,ilog(vi->channels),&testA);
      info->coupling[i].mag=testM;
      info->coupling[i].ang=testA;
      if(oggpack_eop(opb))goto eop;
      
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
  
  if(oggpack_read1(opb)){
    if(oggpack_eop(opb))goto eop;
    if(headerinfo_p || warn_p)
      printf("WARN header: Reserved fields not zeroed\n\n");
    goto err_out; /* 2:reserved */
  }
  if(oggpack_read1(opb)){
    if(oggpack_eop(opb))goto eop;
    if(headerinfo_p || warn_p)
      printf("WARN header: Reserved fields not zeroed\n\n");
    goto err_out; /* 3:reserved */
  }
  
  if(info->submaps>1){
    if(headerinfo_p)
      printf("             channel->submap list  : ");

    for(i=0;i<vi->channels;i++){
      oggpack_read(opb,4,&ret);
      info->chmuxlist[i]=ret;
      if(oggpack_eop(opb))goto eop;

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
    oggpack_read(opb,8,&ret);
    oggpack_read(opb,8,&ret);
    info->submaplist[i].floor=ret;
    if(oggpack_eop(opb))goto eop;
    if(headerinfo_p)
      printf("floor %lu, ",ret);
    if(info->submaplist[i].floor>=vi->floors){
      if(headerinfo_p || warn_p)
	printf("WARN header: Requested floor (%lu) out of range.\n\n",ret);
      goto err_out;
    }
    oggpack_read(opb,8,&ret);
    if(oggpack_eop(opb))goto eop;
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

#if 0
ogg_int16_t mapping_inverse(vorbis_dsp_state *vd,vorbis_info_mapping *info){
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;

  ogg_int16_t   i,j;
  ogg_int32_t   n=ci->blocksizes[vd->W];

  ogg_int32_t **pcmbundle=_ogg_alloc(0,sizeof(*pcmbundle)*vi->channels);
  ogg_int16_t  *zerobundle=_ogg_alloc(0,sizeof(*zerobundle)*vi->channels);
  ogg_int16_t  *nonzero=_ogg_alloc(0,sizeof(*nonzero)*vi->channels);
  ogg_int32_t **floormemo=_ogg_alloc(0,sizeof(*floormemo)*vi->channels);
  
  /* recover the spectral envelope; store it in the PCM vector for now */
  for(i=0;i<vi->channels;i++){
    ogg_int16_t submap=0;
    ogg_int16_t floorno;
    
    if(info->submaps>1)
      submap=info->chmuxlist[i];
    floorno=info->submaplist[submap].floor;
    
    if(ci->floor_type[floorno]){
      /* floor 1 */
      floormemo[i]=_ogg_alloc(0,sizeof(*floormemo[i])*
			  floor1_memosize(ci->floor_param[floorno]));
      floormemo[i]=floor1_inverse1(vd,ci->floor_param[floorno],floormemo[i]);
    }else{
      /* floor 0 */
      floormemo[i]=_ogg_alloc(0,sizeof(*floormemo[i])*
			  floor0_memosize(ci->floor_param[floorno]));
      floormemo[i]=floor0_inverse1(vd,ci->floor_param[floorno],floormemo[i]);
    }
    
    if(floormemo[i])
      nonzero[i]=1;
    else
      nonzero[i]=0;      
    _ogg_bzero(vd->work[i],sizeof(*vd->work[i])*n/2);
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
    ogg_int16_t ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(!info->chmuxlist || info->chmuxlist[j]==i){
	if(nonzero[j])
	  zerobundle[ch_in_bundle]=1;
	else
	  zerobundle[ch_in_bundle]=0;
	pcmbundle[ch_in_bundle++]=vd->work[j];
      }
    }
    
    res_inverse(vd,ci->residue_param+info->submaplist[i].residue,
		pcmbundle,zerobundle,ch_in_bundle);
  }


  _analysis("mid",seq,vd->work[0],n/2,0,0);
  _analysis("side",seq,vd->work[1],n/2,0,0);

  /* channel coupling */
  for(i=info->coupling_steps-1;i>=0;i--){
    ogg_int32_t *pcmM=vd->work[info->coupling[i].mag];
    ogg_int32_t *pcmA=vd->work[info->coupling[i].ang];
    
    for(j=0;j<n/2;j++){
      ogg_int32_t mag=pcmM[j];
      ogg_int32_t ang=pcmA[j];
      
      if(mag>0)
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag-ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag+ang;
	}
      else
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag+ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag-ang;
	}
    }
  }

  _analysis("resL",seq,vd->work[0],n/2,0,0);
  _analysis("resR",seq,vd->work[1],n/2,0,0);

  /* compute and apply spectral envelope */
  for(i=0;i<vi->channels;i++){
    ogg_int32_t *pcm=vd->work[i];
    ogg_int16_t submap=0;
    ogg_int16_t floorno;

    if(info->submaps>1)
      submap=info->chmuxlist[i];
    floorno=info->submaplist[submap].floor;

    if(ci->floor_type[floorno]){
      /* floor 1 */
      floor1_inverse2(vd,ci->floor_param[floorno],floormemo[i],pcm);
    }else{
      /* floor 0 */
      floor0_inverse2(vd,ci->floor_param[floorno],floormemo[i],pcm);
    }
  }

  _analysis("specL",seq,vd->work[0],n/2,1,1);
  _analysis("specR",seq++,vd->work[1],n/2,1,1);
  

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++)
    mdct_backward(n,vd->work[i]);

  _ogg_free(pcmbundle); /* need only free the lowest */

  /* all done! */
  return(0);
}

#endif
