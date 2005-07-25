/* Copyright (C) 2005 */
/**
   @file lifting.c
   @brief Lifting wavelet transform
 */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
   
}

