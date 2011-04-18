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
  float ddA; /* amplitude modulation (linear change per sample^2) */
  int label;/* used for tracking by outside code */
} chirp;

extern int
estimate_chirps(const float *x, /* unwindowed input to fit */
                float *y,       /* unwindowed outptu reconstruction */
                const float *window, /* window to apply to input/bases */
                int len,        /* block length */
                chirp *c,       /* list of chirp estimates/outputs */
                int n,          /* number of chirps */
                float fit_limit,/* minimum basis movement to continue iteration */
                int iter_limit, /* maximum number of iterations */
                int fit_gs,     /* Use Gauss-Seidel partial updates */

                int fitW,       /* fit the W parameter */
                int fitdA,      /* fit the dA parameter */
                int fitdW,      /* fit the dW parameter */
                int fitddA,     /* fit the ddA parameter */
                int nonlinear,  /* perform a linear fit (0),
                                   nonlinear fit recentering W only (1)
                                   nonlinear fit recentering W and dW (2) */
                float fit_W_alpha, /* W alpha multiplier for nonlinear fit */
                float fit_dW_alpha,/* dW alpha multiplier for nonlinear fit */
                int symm_norm,     /* Use symmetric normalization optimization */
                int bound_zero);   /* prevent W or dW from fitting to negative or
                                      greater-then-Nyquist frequencies */

extern void advance_chirps(chirp *c, int n, int len);

