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

#define _GNU_SOURCE
#include <math.h>
#include "window.h"

/* A few windows */

/* Not-a-window */
static void rectangle(float *x, int n){
  int i;
  for(i=0;i<n;i++)
    x[i]=1.;
}

/* Minimum 4-term Blackman Harris; highest sidelobe = -92dB */
#define A0 .35875f
#define A1 .48829f
#define A2 .14128f
#define A3 .01168f

static void blackmann_harris(float *x, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    float w = A0 - A1*cos(scale*i5) + A2*cos(scale*i5*2) - A3*cos(scale*i5*3);
    x[i] = w;
  }
}

/* Good 'ol Hanning window (narrow mainlobe, fast falloff, high sidelobes) */
static void hanning(float *x, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    x[i] = (.5-.5*cos(scale*i5));
  }
}

/* Triangular * gaussian (sidelobeless window) */
#define TGA 1.e-4
#define TGB 21.6
static void tgauss_deep(float *x, int n){
  int i;
  for(i=0;i<n;i++){
    float f = (i-n/2.)/(n/2.);
    x[i] = exp(-TGB*pow(f,2)) * pow(1.-fabs(f),TGA);
  }
}

static void vorbis(float *d, int n){
  int i;
  for(i=0;i<n;i++)
    d[i] = sin(0.5 * M_PI * sin((i+.5)/n * M_PI)*sin((i+.5)/n * M_PI));
}

static float beta(int n, float alpha){
  return cosh (acosh(pow(10,alpha))/(n-1));
}

static double T(double n, double x){
  if(fabs(x)<=1){
    return cos(n*acos(x));
  }else{
    return cosh(n*acosh(x));
  }
}

/* Dolph-Chebyshev window (a=6., all sidelobes < -120dB) */
static void dolphcheb(float *d, int n){
  int i,k;
  float a = 6.;
  int M=n/2;
  int N=M*2;
  double b = beta(N,a);

  for(i=0;i<n;i++){
    double sum=0;
    for(k=0;k<M;k++)
      sum += (k&1?-1:1)*T(N,b*cos(M_PI*k/N)) * cos (2*i*k*M_PI/N);
    sum /= T(N,b);
    sum-=.5;
    d[i]=sum;
  }
}

window_bundle window_functions = {
  rectangle,
  hanning,
  vorbis,
  blackmann_harris,
  tgauss_deep,
  dolphcheb
};
