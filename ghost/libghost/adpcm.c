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

void adpcm_quant(float *x, float *a, int *q, int N, int len)
{
   int i,j;
   for (i=0;i<len;i++)
   {
      float p = 0;
      /* Prediction: conceptual code, this will segfault (or worse) */
      for (j=0;j<N;j++)
         p += a[j]*q[i-j-1];
      /* Difference */
      q[i] = rint(x[i]-p);
      /* Adaptation: conceptual code, this will segfault (or worse) */
      for (j=0;j<N;j++)
         a[j] += q[i]*q[i-j-1]/(q[i]*q[i]); 
   }
}

void adpcm_unquant(int *q, float *a, float *x, int len)
{
   int i;
   for (i=0;i<len;i++)
   {
      float p = 0;
      x[i] = q[i]+p;
   }
}

