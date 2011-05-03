/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007-2011              *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: window functions for research code
 last mod: $Id$

 ********************************************************************/

typedef struct {
  void (*rectangle)(float *,int);
  void (*sine)(float *,int);
  void (*hanning)(float *,int);
  void (*vorbis)(float *,int);
  void (*blackman_harris)(float *,int);
  void (*tgauss_deep)(float *,int);
  void (*dolphcheb)(float *,int);
  void (*maxwell1)(float *,int);
} window_bundle;

extern window_bundle window_functions;
