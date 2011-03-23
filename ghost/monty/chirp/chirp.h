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

 function: research-grade chirp extraction code
 last mod: $Id$

 ********************************************************************/

typedef struct {
  float A;  /* center amplitude (linear) */
  float W;  /* frequency (radians per sample, not cycles per block) */
  float P;  /* phase (radians) */
  float dA; /* amplitude modulation (linear change per sample) */
  float dW; /* frequency modulation (radians per sample^2) */
  int label;/* used for tracking by outside code */
} chirp;

extern int estimate_chirps(float *x, float *y, float *r,
                           float *window, int len,
                           chirp *c, int n, int iter_limit, float fit_limit);
extern void advance_chirps(chirp *c, int n, int len);

