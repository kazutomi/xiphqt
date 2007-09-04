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

//#include "../work/bands_quant.c"
#include "pitch_quant.h"
#include <math.h>

struct VQuantiser_ {
   int len;
   int entries;
   float *means;
};

void vq_train(float *data, int N, int len, int entries)
{
   
}

/* Taken from Speex.
   Finds the index of the entry in a codebook that best matches the input*/
int vq_index(float *in, const float *codebook, int len, int entries)
{
   int i,j;
   float min_dist=0;
   int best_index=0;
   for (i=0;i<entries;i++)
   {
      float dist=0;
      for (j=0;j<len;j++)
      {
         float tmp = in[j]-*codebook++;
         dist += tmp*tmp;
      }
      if (i==0 || dist<min_dist)
      {
         min_dist=dist;
         best_index=i;
      }
   }
   return best_index;
}

#if 0
int quantise_bands(float *in, float *out, int len)
{
   float err[len];
   int id, i;
   id = vq_index(in, cdbk_band1, len, ENTRIES1);
   for (i=0;i<len;i++)
   {
      out[i] = cdbk_band1[id*len + i];
      err[i] = in[i]-out[i];
   }

   id = vq_index(err, cdbk_band2, len, ENTRIES2);
   for (i=0;i<len;i++)
   {
      out[i] += cdbk_band2[id*len + i];
      err[i] = in[i]-out[i];
   }

   
   id = vq_index(err, cdbk_band3a, LEN3A, ENTRIES3A);
   for (i=0;i<LEN3A;i++)
   {
      out[i] += cdbk_band3a[id*LEN3A + i];
      err[i] = in[i]-out[i];
   }
   
   id = vq_index(err+LEN3A, cdbk_band3b, LEN3B, ENTRIES3B);
   for (i=0;i<LEN3B;i++)
   {
      out[i+LEN3A] += cdbk_band3b[id*LEN3B + i];
      err[i+LEN3A] = in[i+LEN3A]-out[i+LEN3A];
   }

   id = vq_index(err+LEN3A+LEN3B, cdbk_band3c, LEN3C, ENTRIES3C);
   for (i=0;i<LEN3C;i++)
   {
      out[i+LEN3A+LEN3B] += cdbk_band3c[id*LEN3C + i];
      err[i+LEN3A+LEN3B] = in[i+LEN3A+LEN3B]-out[i+LEN3A+LEN3B];
   }

}
#endif

void quantise_pitch(float *gains, int len)
{
   int i, id;
   float g2[len];
   for (i=0;i<len;i++)
      g2[i] = gains[i]*gains[i];
   id = vq_index(g2, cdbk_pitch, len, 32);
   for (i=0;i<len;i++)
      gains[i] = sqrt(cdbk_pitch[id*len+i]);
}
