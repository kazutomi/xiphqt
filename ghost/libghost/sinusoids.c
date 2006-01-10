/**
   @file sinusoids.c
   @brief Sinusoid extraction/synthesis
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


#include <math.h>
#include "sinusoids.h"
#include <stdio.h>

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

void find_sinusoids(float *psd, float *w, int N, int length)
{
   int i,j;
   float sinusoidism[length];
   float tmp_max;
   int tmp_id;
   sinusoidism[0] = sinusoidism[length-1] = -1;
   sinusoidism[1] = sinusoidism[length-2] = -1;
   for (i=2;i<length-2;i++)
   {
      if (psd[i] > psd[i-1] && psd[i] > psd[i+1])
      {
         float highlobe, lowlobe;
         highlobe = psd[i]-MIN(psd[i+1], psd[i+2]);
         lowlobe = psd[i]-MIN(psd[i-1], psd[i-2]);
         sinusoidism[i] = psd[i] + .5*(highlobe + lowlobe - .5*MAX(lowlobe, highlobe));
      } else {
         sinusoidism[i] = -1;
      }
   }
   /*for (i=0;i<=length;i++)
   {
      fprintf (stderr, "%f ", sinusoidism[i]);
   }
   fprintf (stderr, "\n");*/
   for (i=0;i<N;i++)
   {
      tmp_max = -2;
      for (j=0;j<length;j++)
      {
         if (sinusoidism[j]>tmp_max)
         {
            tmp_max = sinusoidism[j];
            tmp_id = j;
         }
      }
      sinusoidism[tmp_id] = -3;
      w[i] = M_PI*tmp_id/(length-1);
   }
}

void extract_sinusoids(float *x, float *w, float *window, float *ai, float *bi, float *y, int N, int len)
{
   float cos_table[N][len];
   float sin_table[N][len];
   float cosE[N], sinE[N];
   int i,j, iter;
   for (i=0;i<N;i++)
   {
      float tmp1=0, tmp2=0;
      for (j=0;j<len;j++)
      {
         cos_table[i][j] = cos(w[i]*j)*window[j];
         sin_table[i][j] = sin(w[i]*j)*window[j];
         tmp1 += cos_table[i][j]*cos_table[i][j];
         tmp2 += sin_table[i][j]*sin_table[i][j];
      }
      cosE[i] = tmp1;
      sinE[i] = tmp2;
   }
   for (j=0;j<len;j++)
      y[j] = 0;
   for (i=0;i<N;i++)
      ai[i] = bi[i] = 0;

   for (iter=0;iter<5;iter++)
   {
      for (i=0;i<N;i++)
      {
         float tmp1=0, tmp2=0;
         for (j=0;j<len;j++)
         {
            tmp1 += (x[j]-y[j])*cos_table[i][j];
            tmp2 += (x[j]-y[j])*sin_table[i][j];
         }
         tmp1 /= cosE[i];
         //Just in case it's a DC! Must fix that anyway
         tmp2 /= (.0001+sinE[i]);
         for (j=0;j<len;j++)
         {
            y[j] += tmp1*cos_table[i][j] + tmp2*sin_table[i][j];
         }
         ai[i] += tmp1;
         bi[i] += tmp2;
      }
   }
}

