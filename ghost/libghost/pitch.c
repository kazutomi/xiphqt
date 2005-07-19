/* Copyright (C) 2005 */
/**
   @file pitch.c
   @brief Pitch analysis
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

#include <stdio.h>
#include <math.h>

float inner_prod(float *x, float *y, int len)
{
   float sum=0;
   int i;
   for (i=0;i<len;i++)
   {
      sum += x[i]*y[i];
   }
   return sum;
}

void find_pitch(float *x, float *gain, float *pitch, int start, int end, int len)
{
   int i;
   float max_score = -1;
   float sc[end+1];
   int p=0;
   for (i=start;i<=end;i++)
   {
      float corr, score, E;
      corr = inner_prod(x,x-i,len);
      E = inner_prod(x-i,x-i,len);
      //E = inner_prod(x,x,len);
      //printf ("%f ", E);
      score = corr*fabs(corr)/(1e4+E);
      sc[i] = score;
      if (score > max_score)
      {
         p = i;
         max_score = score;
         *gain = corr/(1+E);
      }
   }
   if (p == start || p == end)
   {
      *pitch = p;
   } else {
      *pitch = p+.5*(sc[p+1]-sc[p-1])/(2*sc[p]-sc[p-1]-sc[p+1]);
   }
}



