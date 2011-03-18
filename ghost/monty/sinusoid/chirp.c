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

#define _GNU_SOURCE
#include <math.h>
#include "sinusoids.h"
#include "scales.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_descending_A(const void *p1, const void *p2){
  chirp *A = (chirp *)p1;
  chirp *B = (chirp *)p2;

  return (A->A<B->A) - (B->A<A->A);
}

static int cmp_ascending_W(const void *p1, const void *p2){
  chirp *A = (chirp *)p1;
  chirp *B = (chirp *)p2;

  return (B->W<A->W) - (A->W<B->W);
}

/* performs a second-order nonlinear chirp fit. Passed in chirps are
   used as initial estimation inputs to the iteration. Applies the
   square of passed in window to the input/basis functions.

   x: input signal (unwindowed)
   y: output reconstruction (unwindowed)
   window: window to apply to input/basis during fitting process
   len: length of x and window vectors
   c: chirp estimation inputs, chirp fit outputs (sorted in frequency order)
   n: number of chirps
   iter_limit: maximum allowable number of iterations
   fit_limit: desired fit limit

   Chirp frequency must be within ~ 1 FFT bin of fit frequency (see
   paper).  Amplitude, amplitude' and phase are fit wouthout a need
   for initial estimate.  Frequency' fit will converge from any point,
   though it will fit to the closest lobe (wheter mainlobe or
   sidelobe).  All parameters fit within 3 iterations with the
   exception of frequency' which shows approximately linear
   convergence outside of ~ .5 frequency bins.

   Fitting terminates when no fit parameter changes by more than
   fit_limit in an iteration or when the fit loop reaches the
   iteration limit.  The fit parameters as checked are scaled over the
   length of the block.

*/

int estimate_chirps(float *x, float *y, float *window, int len,
                    chirp *c, int n, int iter_limit, float fit_limit){

  int i,j,flag=1;
  float cwork[len];
  float swork[len];
  float residue[len];

  /* Build a starting reconstruction based on the passied in initial
     chirp estimates */
  memset(y,0,sizeof(*y)*len);
  for(i=0;i<n;i++){
    for(j=0;j<len;j++){
      float jj = j-len*.5+.5;
      float a = c[i].A + c[i].dA*jj;
      float w = (c[i].W + c[i].dW*jj)*jj;
      y[j] += a*cos(w+c[i].P);
    }
  }

  /* outer fit iteration */
  while(flag && iter_limit--){
    flag=0;

    /* precompute the portion of the projection/fit estimate shared by
       the zero, first and second order fits.  Subtractsthe current
       best fits from the input signal and widows the result. */
    for(j=0;j<len;j++)
      residue[j]=(x[j]-y[j])*window[j];
    memset(y,0,sizeof(*y)*len);

    /* Sort chirps by descending amplitude */
    qsort(c, n, sizeof(*c), cmp_descending_A);

    /* reestimate the next chirp fit */
    for(i=0;i<n;i++){
      float lA2,lA,lW,ldA,lP,ldW;
      float aP=0, bP=0;
      float cP=0, dP=0;
      float eP=0, fP=0;
      float aE=0, bE=0;
      float cE=0, dE=0;
      float eE=0, fE=0;
      float aC = cos(c[i].P);
      float aS = sin(c[i].P);

      for(j=0;j<len;j++){
	float jj = j-len*.5+.5;
	float jj2 = jj*jj;
	float co = cos((c[i].W + c[i].dW*jj)*jj)*window[j];
	float si = sin((c[i].W + c[i].dW*jj)*jj)*window[j];
        float c2 = co*co;
        float s2 = si*si;

        /* add the current estimate back to the residue vector */
        float yy = residue[j] += (aC*co-aS*si) * (c[i].A + c[i].dA*jj);

        /* Cache the basis function premultiplied by jj for the two
           later fitting loops */
        cwork[j] = c2*jj;
        swork[j] = s2*jj;

        /* zero order projection */
	aP += co*yy;
	bP += si*yy;
        /* the part of the first order fit we can perform before
           having an updated amplitude/phase.  This saves needing to
           cache or recomupute yy later.  */
        cP += co*yy*jj;
        dP += si*yy*jj;
        /* the part of the second order fit we can perform before
           having an updated amplitude/phase/frequency/ampliude'.
           This saves needing to cache or recomupute yy later. */
        eP += co*yy*jj2;
        fP += si*yy*jj2;

        /* integrate the total energy under the curve of each basis
           function */
	aE += c2;
	bE += s2;
	cE += c2*jj2;
	dE += s2*jj2;
        eE += c2*jj2*jj2;
        fE += s2*jj2*jj2;
      }
      /* aP/bP in terms of total basis energy */
      aP = (aE>0.f ? aP/aE : 0.f);
      bP = (bE>0.f ? bP/bE : 0.f);

      /* the frequency/amplitude' fits converge faster now that we
         have an updated amplitude/phase estimate.  Finish the portion
         of the first order projection we couldn't compute above. */
      for(j=0;j<len;j++){
        cP -= aP*cwork[j];
        dP -= bP*swork[j];
      }
      /* cP/dP in terms of total basis energy */
      cP = (cE>0.f ? cP/cE : 0.f);
      dP = (dE>0.f ? dP/dE : 0.f);

      /* the frequency' fit converges faster now that we have an
         updated estimates for the other parameters.  Finish the
         portion of the second order projection we couldn't compute
         above. */
      for(j=0;j<len;j++){
        float jj = j-len*.5+.5;
        eP -= (aP+cP*jj)*cwork[j]*jj;
        fP -= (bP+dP*jj)*swork[j]*jj;
      }
      /* eP/fP in terms of total basis energy */
      eP = (eE>0.f ? eP/eE : 0.f);
      fP = (fE>0.f ? fP/fE : 0.f);

      /* computer new chirp fit estimate from basis projections */
      lA2 = aP*aP + bP*bP;
      lA = sqrt(lA2);
      lP = -atan2(bP, aP);
      lW = (cP*bP - dP*aP)/lA2;
      ldA = (cP*aP + dP*bP)/lA;
      ldW = 1.75*(eP*bP - fP*aP)/lA2;

      /* have we converged to within a requested limit */
      if(c[i].A==0 || fabs((lA-c[i].A)/c[i].A)>fit_limit) flag=1;
      if(fabs(lP - c[i].P)>fit_limit) flag=1;
      if(fabs(lW*len)>fit_limit) flag=1;
      if(c[i].A==0 || fabs((ldA-c[i].dA)/c[i].A)>fit_limit) flag=1;
      if(fabs(ldW*len*len)>fit_limit*2.*M_PI) flag=1;

      /* save new fit estimate */
      c[i].A = lA;
      c[i].P = lP;
      c[i].W += lW;
      c[i].dA = ldA;
      c[i].dW += ldW;

      /* update the reconstruction/residue vectors with new fit */
      for(j=0;j<len;j++){
        float jj = j-len*.5+.5;
        float a = c[i].A + c[i].dA*jj;
        float w = (c[i].W + c[i].dW*jj)*jj;
        float v = a*cos(w+c[i].P);
        residue[j] -= v*window[j];
        y[j] += v;
      }
    }
  }

  /* Sort by ascending frequency */
  qsort(c, n, sizeof(*c), cmp_ascending_W);

  return iter_limit;
}

/* advances/extrapolates the passed in chirps so that the params are
   centered forward in time by len samples. */

void advance_chirps(chirp *c, int n, int len){
  int i;
  for(i=0;i<n;i++){
    c[i].A += c[i].dA*len;
    c[i].W += c[i].dW*len;
    c[i].P += c[i].W*len;
  }
}

/* OMG!  An example! */

#if 0

void hanning(float *x, int n){
  float scale = 2*M_PI/n;
  int i;
  for(i=0;i<n;i++){
    float i5 = i+.5;
    x[i] = .5-.5*cos(scale*i5);
  }
}

int main(){
  int BLOCKSIZE=1024;
  int i;
  float w[1024];
  float x[1024];
  float y[1024];

  chirp c[]={
    {.3, 100.*2.*M_PI/BLOCKSIZE, .2, 0, 0},
    {1., 95.*2.*M_PI/BLOCKSIZE, .2, 0, 2.5*2.*M_PI/BLOCKSIZE/BLOCKSIZE}
  };

  hanning(w,BLOCKSIZE);

  for(i=0;i<BLOCKSIZE;i++){
    float jj = i-BLOCKSIZE*.5+.5;
    x[i]=(.3+.1*jj/BLOCKSIZE)*cos(jj*2.*M_PI/BLOCKSIZE * 100.2 +1.2);
    x[i]+=cos(jj*2.*M_PI/BLOCKSIZE * (96. + 3./BLOCKSIZE*jj) +1.2);
  }

  int ret=estimate_chirps(x,y,w,BLOCKSIZE,c,2,55,.01);

  for(i=0;i<sizeof(c)/sizeof(*c);i++)
    fprintf(stderr,"W:%f\nP:%f\nA:%f\ndW:%f\ndA:%f\nreturned=%d\n\n",
            c[i].W*BLOCKSIZE/2./M_PI,
            c[i].P,
            c[i].A,
            c[i].dW*BLOCKSIZE*BLOCKSIZE/2./M_PI,
            c[i].dA*BLOCKSIZE,ret);

  return 0;
}
#endif
