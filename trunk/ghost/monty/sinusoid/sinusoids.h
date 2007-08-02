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

 function: research-grade sinusoidal extraction code
 last mod: $Id$

 ********************************************************************/

extern void window_weight(float *logf, float *out, int n, float flatbias,
			  int lowindow, int hiwindow, int min, int rate);

extern void extract_modulated_sinusoids(float *x, float *w, float *Aout, float *Wout, float *Pout, 
					float *delAout, float *delWout, float *ddAout, float *y, int N, int len);

extern void extract_modulated_sinusoidsB(float *x, float *w, 
					 float *Aout, float *Wout, float *Pout, 
					 float *dAout, float *dWout, float *ddAout, 
					 float *y, int N, int len);

extern void extract_modulated_sinusoids_nonlinear(float *x, float *window, 
						  float *A, float *W, float *P, 
						  float *dA, float *dW, 
						  float *y, int N, int len);
