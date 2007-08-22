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

float dist(float *a, float *b, int N)
{
   float d = 0;
   int i;
   for (i=0;i<N;i++)
   {
      d += (a[i]-b[i])*(a[i]-b[i]);
   }
   return d;
}

/* First attempt at a near-uniform algebraic quantiser -- failed */
void alg_quant_fail1(float *x, int N, int K)
{
   int i, j, k;
   int level;
   float S[N][N];
   float NN[N][N];
   float D[N];
   for (i=0;i<N;i++)
   {
      for (j=0;j<N;j++)
         S[i][j] = 0;
      if (x[i]>0)
         S[i][i] = 1;
      else
         S[i][i] = -1;
   }
   for (i=0;i<N;i++)
   {
      x[i] = x[i]/sqrt(N);
   }
   
   for (level = 0;level < K; level++)
   {

      for (j=0;j<N;j++)
         for(k=0;k<N;k++)
            NN[j][k] = -100;

      for (j=0;j<N;j++)
         D[j] = 1e5;

      for (i=0;i<N;i++)
      {
         for (j=0;j<=i;j++)
         {
            float E=0;
            float tmp[N];
            //printf("ij = %d %d (%f %f %f %f)\n", i, j, S[0][0], S[1][0], S[0][1], S[1][1]);
            for (k=0;k<N;k++)
            {
               //printf("%f %f %d %d %d\n", S[i][k], S[j][k], i, j, k);
               tmp[k] = (S[i][k]+S[j][k]);
               E += tmp[k]*tmp[k];
            }
            //printf("%d %d ", i, j);
            //for (k=0;k<N;k++)
            //   printf("%f ", tmp[k]);
            //printf("%f\n", E);   
            E = 1.f/sqrt(E);
            for (k=0;k<N;k++)
               tmp[k] *= E;
            float d = dist(x, tmp, N);
            //printf("%d %d ", i, j);
            //for (k=0;k<N;k++)
            //   printf("%f ", tmp[k]);
            //printf("%f\n", d);   
            if (d<D[N-1])
            {
               int id = N-1;
               while (id>0 && d<D[id-1])
                  id--;
               //printf ("id = %d (%f < %f)\n", id, d, D[id]);
               int m;
               for (m=N-1;m>id;m--)
               {
                  for (k=0;k<N;k++)
                  {
                     D[m] = D[m-1];
                     NN[m][k] = NN[m-1][k];
                  }
               }
               for (k=0;k<N;k++)
               {
                  NN[id][k] = tmp[k];
                  D[id] = d;
               }
            }
            //printf ("\n");
         }
      }
      for (j=0;j<N;j++)
         for(k=0;k<N;k++)
            S[j][k] = NN[j][k];
      
   }
   
   for (i=0;i<N;i++)
   {
      x[i] = S[0][i]*sqrt(N);
   }
}

void alg_quant2(float *x, int N, int K)
{
   int pulses[N];
   float sign[N];
   float y[N];
   int i,j;
   
   float P = sqrt((1.f*N)/K);
   for (i=0;i<N;i++)
      pulses[i] = 0;
   for (i=0;i<N;i++)
      sign[i] = 0;

   for (i=0;i<N;i++)
      y[i] = 0;
   
   for (i=0;i<K;i++)
   {
      int best_id=0;
      float max_val=-1e10;
      float p;
      for (j=0;j<N;j++)
      {
         float E = 0;
         if (pulses[j])
            p = P*sign[j]*(sqrt(pulses[j]+1)-sqrt(pulses[j]));
         else if (x[j]>0)
            p=P;
         else
            p=-P;
         E = x[j]*x[j] - (x[j]-p)*(x[j]-p);
         if (E>max_val)
         {
            max_val = E;
            best_id = j;
         }
      }
      
      if (pulses[best_id])
         p = P*sign[best_id]*(sqrt(pulses[best_id]+1)-sqrt(pulses[best_id]));
      else if (x[best_id]>0)
         p=P;
      else
         p=-P;
      y[best_id] += p;
      x[best_id] -= p;
      pulses[best_id]++;
      if (p>0)
         sign[best_id]=1;
      else
         sign[best_id]=-1;
   }
   
   for (i=0;i<N;i++)
      x[i] = y[i];
   
}


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

#define NBANDS 23 /*or 22 if we discard the small last band*/
int qbank[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};

#if 0
void compute_bank(float *ps, float *bank)
{
   int i;
   for (i=0;i<NBANDS;i++)
   {
      int j;
      bank[i]=0;
      for (j=qbank[i];j<qbank[i+1];j++)
         bank[i] += ps[j];
      bank[i] = sqrt(bank[i]/(qbank[i+1]-qbank[i]));
   }
}
#else
void compute_bank(float *X, float *bank)
{
   int i;
   bank[0] = 1e-10+fabs(X[0]);
   for (i=1;i<NBANDS;i++)
   {
      int j;
      bank[i] = 1e-10;
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         bank[i] += X[j*2-1]*X[j*2-1];
         bank[i] += X[j*2]*X[j*2];
      }
      bank[i] = sqrt(.5*bank[i]/(qbank[i+1]-qbank[i]));
   }
   //FIXME: Kludge
   X[255] = 1;
}
#endif

void normalise_bank(float *X, float *bank)
{
   int i;
   X[0] /= bank[0];
   for (i=1;i<NBANDS;i++)
   {
      int j;
      float x = 1.f/bank[i];
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] *= x;
         X[j*2]   *= x;
      }
   }
   //FIXME: Kludge
   X[255] = 0;
}

void denormalise_bank(float *X, float *bank)
{
   int i;
   X[0] *= bank[0];
   for (i=1;i<NBANDS;i++)
   {
      int j;
      float x = bank[i];
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] *= x;
         X[j*2]   *= x;
      }
   }
   //FIXME: Kludge
   X[255] = 0;
}

void quant_bank(float *X)
{
   int i;
   float q=8;
   X[0] = (1.f/q)*floor(.5+q*X[0]);
   for (i=1;i<NBANDS;i++)
   {
      int j;
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] = (1.f/q)*floor(.5+q*X[j*2-1]);
         X[j*2] = (1.f/q)*floor(.5+q*X[j*2]);
      }
   }
   //FIXME: Kludge
   X[255] = 0;
}

void quant_bank2(float *X)
{
   int i;
   float q=8;
   X[0] = (1.f/q)*floor(.5+q*X[0]);
   for (i=1;i<NBANDS;i++)
   {
      int j;
      alg_quant2(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), 1);
      /*for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] = (1.f/q)*floor(.5+q*X[j*2-1]);
         X[j*2] = (1.f/q)*floor(.5+q*X[j*2]);
      }*/
   }
   //FIXME: Kludge
   X[255] = 0;
}

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
   //float bark[BARK_BANDS];
   float Xps[st->length>>1];
   float X[st->length];
   int i;

   spx_fft_float(st->frame_fft, in, X);
   
   Xps[0] = .1+X[0]*X[0];
   for (i=1;i<st->length>>1;i++)
      Xps[i] = .1+X[2*i-1]*X[2*i-1] + X[2*i]*X[2*i];

#if 1
   float bank[NBANDS];
   compute_bank(X, bank);
   normalise_bank(X, bank);
#else
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
#endif
   
   /*for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
*/
   
#if 1
   quant_bank2(X);
#else
   for(i=0;i<st->length;i++)
   {
      float q = 4;
      if (i<10)
         q = 8;
      else if (i<20)
         q = 4;
      else if (i<30)
         q = 2;
      else if (i<50)
         q = 1;
      else
         q = .5;
      //q=1;
      int sq = floor(.5+q*X[i]);
      printf ("%d ", sq);
      X[i] = (1.f/q)*(sq);
   }
   printf ("\n");
#endif

#if 0
   X[0]  *= Xps[0];
   for (i=1;i<st->length>>1;i++)
   {
      X[2*i-1] *= Xps[i];
      X[2*i] *= Xps[i];
   }
   X[st->length-1] *= Xps[(st->length>>1)-1];
#else
   float bank2[NBANDS];
   compute_bank(X, bank2);
   normalise_bank(X, bank2);

   denormalise_bank(X, bank);
#endif
   
   
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

