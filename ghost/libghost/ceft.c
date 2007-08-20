/* Copyright (C) 2007

   Code-Excited Fourier Transform -- This is highly experimental and
   it's not clear at all it even has a remote chance of working


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

#include "ceft.h"
#include "filterbank.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fftwrap.h"

/*
0
1
2
3
4
5
6
7
8
9
10 11
12 13
14 15
16 17 18 19
20 21 22 23
24 25 26 27
28 29 30 31 32 33
34 35 36 37 38 39 49 41
42 43 44 45 46 47 48 49
50 .. 63   (14)
64 .. 83   (20)
84 .. 127  (42)
*/
#define BARK_BANDS 20

struct CEFTState_ {
   FilterBank *bank;
   void *frame_fft;
   int length;
};

CEFTState *ceft_init(int len)
{
   CEFTState *st = malloc(sizeof(CEFTState));
   st->length = len;
   st->frame_fft = spx_fft_init(st->length);
   st->bank = filterbank_new(BARK_BANDS, 48000, st->length>>1, 0);
   return st;
}

void ceft_encode(CEFTState *st, float *in, float *out)
{
   float bark[BARK_BANDS];
   float Xps[st->length>>1];
   float X[st->length];
   int i;

   spx_fft_float(st->frame_fft, in, X);
   
   Xps[0] = .1+X[0]*X[0];
   for (i=1;i<st->length>>1;i++)
      Xps[i] = .1+X[2*i-1]*X[2*i-1] + X[2*i]*X[2*i];

   filterbank_compute_bank(st->bank, Xps, bark);
   filterbank_compute_psd(st->bank, bark, Xps);
   for(i=0;i<st->length>>1;i++)
      Xps[i] = sqrt(Xps[i]);
   X[0] /= Xps[0];
   for (i=1;i<st->length>>1;i++)
   {
      X[2*i-1] /= Xps[i];
      X[2*i] /= Xps[i];
   }
   X[st->length-1] /= Xps[(st->length>>1)-1];
   
   /*for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
*/
   
   for(i=0;i<st->length;i++)
   {
      float q = 4;
      if (i<16)
         q = 8;
      else if (i<32)
         q = 4;
      else if (i<48)
         q = 2;
      else
         q = 1;
      q=1;
      int sq = floor(.5+q*X[i]);
      //printf ("%d ", sq);
      X[i] = (1.f/q)*sq;
   }
   //printf ("\n");
   X[0]  *= Xps[0];
   for (i=1;i<st->length>>1;i++)
   {
      X[2*i-1] *= Xps[i];
      X[2*i] *= Xps[i];
   }
   X[st->length-1] *= Xps[(st->length>>1)-1];
#if 0
   for(i=0;i<BARK_BANDS;i++)
   {
      printf("%f ", bark[i]);
   }
   printf ("\n");
#endif 
   /*
   for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */

   spx_ifft_float(st->frame_fft, X, out);

}

