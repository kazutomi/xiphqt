/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: single-block PCM synthesis
 last mod: $Id: synthesis.c,v 1.13 2000/01/20 04:43:04 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include "vorbis/codec.h"
#include "registry.h"
#include "bitwise.h"

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  vorbis_dsp_state *vd=vb->vd;
  vorbis_info      *vi=vd->vi;
  oggpack_buffer   *opb=&vb->opb;
  int              type;
  int              mode;

  /* first things first.  Make sure decode is ready */
  _vorbis_block_ripcord(vb);
  _oggpack_readinit(opb,op->packet,op->bytes);

  /* Check the packet type */
  if(_oggpack_read(opb,1)!=0){
    /* Oops.  This is not an audio data packet */
    return(-1);
  }

  /* read our mode */
  mode=_oggpack_read(&vb->opb,vd->modebits);
  type=vi->mappingtypes[mode]; /* unpack_header enforces range checking */

  return(vorbis_map_synthesis_P[type](vb,vi->modelist[mode],op));
}


