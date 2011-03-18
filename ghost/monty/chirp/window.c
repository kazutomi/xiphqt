#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <squishyio/squishyio.h>
#include "sinusoids.h"
#include "smallft.h"
#include "scales.h"

/* A few windows */

#define A0 .35875f
#define A1 .48829f
#define A2 .14128f
#define A3 .01168f

void blackmann_harris(float *out, float *in, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    float w = A0 - A1*cos(scale*i5) + A2*cos(scale*i5*2) - A3*cos(scale*i5*3);
    out[i] = in[i]*w;
  }
}

static void hanning(float *out, float *in, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    out[i] = in[i]*(.5-.5*cos(scale*i5));
  }
}
