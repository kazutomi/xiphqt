/**
   @file lifting.c
   @brief Lifting wavelet transform
 */

/* Copyright (C) 2005

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


#include "lifting.h"

void lifting_forward(float *x, struct LiftingBasis *basis, int len, int stride)
{
   int i,j;
   float *r, *rstart; /* residue/modified value */
   float *y; /* prediction start */
   
   /* Prediction */
   if (basis->predict_delay > 1)
      rstart = x-2*stride*(basis->predict_delay-1);
   else
      rstart = x;
   r = rstart;
   y = x + 1 - 2*stride*(basis->predict_length - basis->predict_delay);
   
   for (i=0;i<len;i++)
   {
      float sum = 0;
      float *p = basis->predict;
      float *y2 = y;
      for (j=0;j<basis->predict_length;j++)
      {
         sum += *p++ * *y2;
         y2 += stride;
      }
      *r -= sum;
      r += stride;
      y += stride;
   }
   
   /* Update */
   r = rstart + 1 - 2*stride*basis->update_delay;
   y = rstart - 2*stride*(basis->update_length - basis->update_delay - 1);

   for (i=0;i<len;i++)
   {
      float sum = 0;
      float *p = basis->update;
      float *y2 = y;
      for (j=0;j<basis->update_length;j++)
      {
         sum += *p++ * *y2;
         y2 += stride;
      }
      *r += sum;
      r += stride;
      y += stride;
   }
}

void lifting_backward(float *x, struct LiftingBasis *basis, int len, int stride)
{
   int i,j;
   float *r, *rstart; /* residue/modified value */
   float *y; /* prediction start */

   if (basis->predict_delay > 1)
      rstart = x-2*stride*(basis->predict_delay-1);
   else
      rstart = x;

}

