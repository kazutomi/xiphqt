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
#include "fftwrap.h"

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
   X[0] /= Xps[0];
   for (i=1;i<st->length>>1;i++)
   {
      X[2*i-1] /= Xps[i];
      X[2*i] /= Xps[i];
   }
   X[st->length-1] /= Xps[(st->length>>1)-1];
   /*
   for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */
   X[0] *= Xps[0];
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

