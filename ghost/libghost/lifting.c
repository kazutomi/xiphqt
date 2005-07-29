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
   int stride2 = 2*stride;
   /* Prediction */
   if (basis->predict_delay > 1)
      rstart = x-stride2*(basis->predict_delay-1);
   else
      rstart = x;
   r = rstart;
   y = x + 1 - stride2*(basis->predict_length - basis->predict_delay);
   
   for (i=0;i<len;i+=stride2)
   {
      float sum = 0;
      const float *p = basis->predict;
      float *y2 = y;
      for (j=0;j<basis->predict_length;j++)
      {
         sum += *p++ * *y2;
         y2 += stride2;
      }
      *r -= sum;
      r += stride2;
      y += stride2;
   }
   //return;
   /* Update */
   r = rstart + 1 - stride2*basis->update_delay;
   y = rstart - stride2*(basis->update_length - basis->update_delay);

   for (i=0;i<len;i+=stride2)
   {
      float sum = 0;
      const float *p = basis->update;
      float *y2 = y;
      for (j=0;j<basis->update_length;j++)
      {
         sum += *p++ * *y2;
         y2 += stride2;
      }
      *r += sum;
      r += stride2;
      y += stride2;
   }
}

void lifting_backward(float *x, struct LiftingBasis *basis, int len, int stride)
{
   int i,j;
   float *r, *rstart, *ustart; /* residue/modified value */
   float *y; /* prediction start */
   int stride2 = 2*stride;

   if (basis->predict_delay > 1)
      rstart = x-2*stride*(basis->predict_delay-1);
   else
      rstart = x;

   /* De-update */
   ustart = rstart + 1 - stride2*basis->update_delay;
   y = rstart - stride2*(basis->update_length - basis->update_delay);
   r = ustart;
   
   for (i=0;i<len;i+=stride2)
   {
      float sum = 0;
      const float *p = basis->update;
      float *y2 = y;
      for (j=0;j<basis->update_length;j++)
      {
         sum += *p++ * *y2;
         y2 += stride2;
      }
      *r -= sum;
      r += stride2;
      y += stride2;
   }

   /* De-predict */
   if (basis->predict_delay > 1)
      rstart = ustart-stride2*(basis->predict_delay-1)-1;
   else
      rstart = ustart-1;
   r = rstart;
   y = ustart - stride2*(basis->predict_length - basis->predict_delay);
   
   for (i=0;i<len;i+=stride2)
   {
      float sum = 0;
      const float *p = basis->predict;
      float *y2 = y;
      for (j=0;j<basis->predict_length;j++)
      {
         sum += *p++ * *y2;
         y2 += stride2;
      }
      *r += sum;
      r += stride2;
      y += stride2;
   }

}

