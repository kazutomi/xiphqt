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

 function: function call to do simple data cascading
 last mod: $Id: cascade.c,v 1.3 2000/01/06 13:57:12 xiphmont Exp $

 ********************************************************************/

/* this one outputs residue to stdout. */

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "bookutil.h"

/* set up metrics */

double count=0.;

void process_preprocess(codebook *b,char *basename){
}
void process_postprocess(codebook *b,char *basename){
  fprintf(stderr,"Done.                      \n");
}

void process_vector(codebook *b,double *a){
  int entry=codebook_entry(b,a);
  double *e=b->valuelist+b->dim*entry;
  int i;

  for(i=0;i<b->dim;i++)
    fprintf(stdout,"%f, ",a[i]-e[i]);
  fprintf(stdout,"\n");

  if((long)(count++)%100)spinnit("working.... lines: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqcascade <codebook>.vqh datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  residual error data sent to\n"
	  "       stdout.\n\n");

}
