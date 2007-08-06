/**
   @file adpcm.c
   @brief ADPCM-like code
 */

/* Copyright (C) 2007

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "adpcm.h"

struct ADPCMState_ {
   int N;
   float *coef;
   float *mem;
   float E;
   float alpha;
};

ADPCMState *adpcm_init(int N)
{
   int i;
   ADPCMState *st = malloc(sizeof(ADPCMState));
   st->N = N;
   st->coef = malloc(N*sizeof(float));
   st->mem = malloc(N*sizeof(float));
   for (i=0;i<N;i++)
   {
      st->coef[i] = 0;
      st->mem[i] = 0;
   }
   st->E = 1;
   st->alpha = .3/N;
   return st;
}

#define MIN(a,b) ((a) < (b) ? (a) : (b)) 

void adpcm_quant(ADPCMState *st, float *x, int *q, int len)
{
   int i,j;
   int N=st->N;
   float *a = st->coef;
   float *mem = st->mem;
   float mu=.05;
   
   for (i=0;i<len;i++)
   {
      float p = 0;
      float e;
      /* Prediction: conceptual code, this will segfault (or worse) */
      for (j=i;j<N;j++)
         p += a[j]*mem[j-i];
      for (j=0;j<MIN(i,N);j++)
         p += a[j]*x[i-j-1];
      
      /* Difference */
      e = x[i]-p;
      q[i] = rint(e);
      x[i] = q[i]+p;
      
      /* Energy update */
      st->E = (1-st->alpha)*st->E + st->alpha*x[i]*x[i];
      if (st->E < 1)
         st->E = 1;
      
      /* Adaptation: conceptual code, this will segfault (or worse) */
      for (j=i;j<N;j++)
         a[j] += mu*e*mem[j-i]/st->E;
      for (j=0;j<MIN(i,N);j++)
         a[j] += mu*e*x[i-j-1]/st->E;
   }
   assert(len >= N);
   for (i=0;i<N;i++)
      mem[i] = x[len-i-1];
}

