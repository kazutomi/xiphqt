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

 function: libvorbis codec internal types.  These structures are 
           'visible', but generally uninteresting to the developer
 last mod: $Id: internal.h,v 1.7 2000/07/12 09:36:17 xiphmont Exp $

 ********************************************************************/

#ifndef _vorbis_internal_h_
#define _vorbis_internal_h_

/* structures for various internal data abstractions ********************/

typedef struct {
  long endbyte;     
  int  endbit;      

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
  
} oggpack_buffer;

#endif





