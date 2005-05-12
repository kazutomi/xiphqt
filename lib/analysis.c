/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Ogg Vorbis SOURCE CODE IS (C) COPYRIGHT 1994-2005            *
 * by the Xiph.org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: single-block PCM analysis mode dispatch
 last mod: $Id$

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg2.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "scales.h"
#include "os.h"
#include "misc.h"

int analysis_noisy=1;

/* decides between modes, dispatches to the appropriate mapping. */
int vorbis_analysis(vorbis_block *vb, ogg2_packet *op){
  int ret,i;
  vorbis_block_internal *vbi=vb->internal;

  vb->glue_bits=0;
  vb->time_bits=0;
  vb->floor_bits=0;
  vb->res_bits=0;

  /* first things first.  Make sure encode is ready */
  for(i=0;i<PACKETBLOBS;i++)
    ogg2pack_writeinit(vbi->packetblob[i], vbi->ogg2bufferstate);

  /* we only have one mapping type (0), and we let the mapping code
     itself figure out what soft mode to use.  This allows easier
     bitrate management */

  if((ret=_mapping_P[0]->forward(vb)))
    return(ret);

  if(op){
    if(vorbis_bitrate_managed(vb))
      /* The app is using a bitmanaged mode... but not using the
         bitrate management interface. */
      return(OV_EINVAL);
    
    op->packet=ogg2pack_writebuffer(&vb->opb);
    op->bytes=ogg2pack_bytes(&vb->opb);
    op->b_o_s=0;
    op->e_o_s=vb->eofflag;
    # This should be the first granule, this is a temporary cheat
    op->top_granule=0;
    op->end_granule=vb->granulepos;
    op->packetno=vb->sequence; /* for sake of completeness */
  }
  return(0);
}

/* there was no great place to put this.... */
void _analysis_output_always(char *base,int i,float *v,int n,int bark,int dB,ogg_int64_t off){
  int j;
  FILE *of;
  char buffer[80];

  /*  if(i==5870){*/
    sprintf(buffer,"%s_%d.m",base,i);
    of=fopen(buffer,"w");
    
    if(!of)perror("failed to open data dump file");
    
    for(j=0;j<n;j++){
      if(bark){
	float b=toBARK((4000.f*j/n)+.25);
	fprintf(of,"%f ",b);
      }else
	if(off!=0)
	  fprintf(of,"%f ",(double)(j+off)/8000.);
	else
	  fprintf(of,"%f ",(double)j);
      
      if(dB){
	float val;
	if(v[j]==0.)
	  val=-140.;
	else
	  val=todB(v+j);
	fprintf(of,"%f\n",val);
      }else{
	fprintf(of,"%f\n",v[j]);
      }
    }
    fclose(of);
    /*  } */
}

void _analysis_output(char *base,int i,float *v,int n,int bark,int dB,
		      ogg_int64_t off){
  if(analysis_noisy)_analysis_output_always(base,i,v,n,bark,dB,off);
}
