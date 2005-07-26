/**
   @file pitch.c
   @brief Pitch analysis
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



