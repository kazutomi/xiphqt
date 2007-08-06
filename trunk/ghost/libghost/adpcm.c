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

typedef struct {
   int N;
   float *coef;
   float *mem;
} ADPCMState;

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
}

void adpcm_quant(ADPCMState *st, float *x, int *q, int len)
{
   int i,j;
   int N=st->N;
   float *a = st->coef;

   for (i=0;i<len;i++)
   {
      float p = 0;
      /* Prediction: conceptual code, this will segfault (or worse) */
      for (j=0;j<N;j++)
         p += a[j]*x[i-j-1];
      /* Difference */
      q[i] = rint(x[i]-p);
      x[i] = q[i]+p;
      /* Adaptation: conceptual code, this will segfault (or worse) */
      for (j=0;j<N;j++)
         a[j] += q[i]*q[i-j-1]/(q[i]*q[i]); 
   }
}

