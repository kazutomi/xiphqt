/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: miscellaneous prototypes
 last mod: $Id: misc.h,v 1.5.2.2 2000/11/04 06:43:50 xiphmont Exp $

 ********************************************************************/

#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_
#include "vorbis/codec.h"

extern void *_vorbis_block_alloc(vorbis_block *vb,long bytes);
extern void _vorbis_block_ripcord(vorbis_block *vb);
extern void _analysis_output(char *base,int i,float *v,int n,int bark,int dB);

#ifdef DEBUG_LEAKS
extern void *_VDBG__ogg_malloc(void *ptr,long bytes,char *file,long line); 
extern void _VDBG_free(void *ptr,char *file,long line); 

#ifndef MISC_C 
#undef _ogg_malloc
#undef _ogg_calloc
#undef _ogg_realloc
#undef free

#define _ogg_malloc(x) _VDBG__ogg_malloc(NULL,(x),__FILE__,__LINE__)
#define _ogg_calloc(x,y) _VDBG__ogg_malloc(NULL,(x)*(y),__FILE__,__LINE__)
#define _ogg_realloc(x,y) _VDBG__ogg_malloc((x),(y),__FILE__,__LINE__)
#define free(x) _VDBG_free((x),__FILE__,__LINE__)
#endif
#endif

#endif
