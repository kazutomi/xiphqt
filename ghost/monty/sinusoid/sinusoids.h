/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007                   *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: research-grade sinusoidal extraction sode
 last mod: $Id$

 ********************************************************************/

extern void window_weight(float *logf, float *out, int n, float flatbias,
			  int lowindow, int hiwindow, int min, int rate);

extern void extract_modulated_sinusoids(float *x, float *w, float *ai, float *bi, float *ci, float *di, float *y, int N, int len);
 
extern void extract_modulated_sinusoids2(float *x, float *w, float *Aout, float *Wout, float *Pout, 
					 float *delAout, float *delWout, float *y, int N, int len);
