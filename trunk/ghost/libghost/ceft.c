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

/* Unit-energy pulse codebook */
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

/* Unit-amplitude pulse codebook */
void alg_quant3(float *x, int N, int K)
{
   float y[N];
   int i,j;
   float xy = 0;
   float yy = 0;
   float E;
   for (i=0;i<N;i++)
      y[i] = 0;
   
   for (i=0;i<K;i++)
   {
      int best_id=0;
      float max_val=-1e10;
      float best_xy=0, best_yy=0;
      for (j=0;j<N;j++)
      {
         float tmp_xy, tmp_yy;
         float score;
         tmp_xy = xy + fabs(x[j]);
         tmp_yy = yy + 2*fabs(y[j]) + 1;
         score = tmp_xy*tmp_xy/tmp_yy;
         if (score>max_val)
         {
            max_val = score;
            best_id = j;
            best_xy = tmp_xy;
            best_yy = tmp_yy;
         }
      }
      
      xy = best_xy;
      yy = best_yy;
      if (x[best_id]>0)
         y[best_id] += 1;
      else
         y[best_id] -= 1;
   }
   
   E = 0;
   for (i=0;i<N;i++)
      E += y[i]*y[i];
   E = sqrt(E/N);
   for (i=0;i<N;i++)
      x[i] = E*y[i];
   
}


#define NBANDS 23 /*or 22 if we discard the small last band*/
int qbank[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};


#if 1
#define PBANDS 6
int pbank[] = {1, 5, 9, 20, 44, 84, 128};
//#define PBANDS 22 /*or 22 if we discard the small last band*/
//int pbank[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};

#else

#define PBANDS 1
int pbank[] = {1, 128};

#endif

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
   for (i=1;i<NBANDS;i++)
   {
      int q=0;
      if (i < 5)
         q = 8;
      else if (i<10)
         q = 4;
      else if (i<15)
         q = 4;
      else
         q = 4;
      //q = 1;
      q/=2;
      alg_quant3(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q);
   }
   //FIXME: This is a kludge, even though I don't think it really matters much
   X[255] = 0;
}


void pitch_quant_bank(float *X, float *P)
{
   int i;
   for (i=0;i<PBANDS;i++)
   {
      float Sxy=0;
      int j;
      float gain;
      for (j=pbank[i];j<pbank[i+1];j++)
      {
         Sxy += X[j*2-1]*P[j*2-1];
         Sxy += X[j*2]*P[j*2];
      }
      gain = Sxy/(2*(pbank[i+1]-pbank[i]));
      //if (i<3)
      //gain *= 1+.02*gain;
      if (gain > .95)
         gain = .95;
      for (j=pbank[i];j<pbank[i+1];j++)
      {
         P[j*2-1] *= gain;
         P[j*2] *= gain;
      }
      //printf ("%f ", gain);
   }
   P[255] = 0;
   //printf ("\n");
}

void pitch_renormalise_bank(float *X, float *P)
{
   int i;
   for (i=1;i<NBANDS;i++)
   {
      int j;
      float Rpp=0;
      float Rxp=0;
      float Rxx=0;
      float gain1;
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         Rxp += X[j*2-1]*P[j*2-1];
         Rxp += X[j*2  ]*P[j*2  ];
         Rpp += P[j*2-1]*P[j*2-1];
         Rpp += P[j*2  ]*P[j*2  ];
         Rxx += X[j*2-1]*X[j*2-1];
         Rxx += X[j*2  ]*X[j*2  ];
      }
      Rxx *= .5/(qbank[i+1]-qbank[i]);
      Rxp *= .5/(qbank[i+1]-qbank[i]);
      Rpp *= .5/(qbank[i+1]-qbank[i]);
      gain1 = sqrt(Rxp*Rxp + 1 - Rpp)-Rxp;
      if (Rpp>.9999)
         Rpp = .9999;
      gain1 = sqrt(1.-Rpp);
      //gain2 = -sqrt(Rxp*Rxp + 1 - Rpp)-Rxp;
      //if (fabs(gain2)<fabs(gain1))
      //   gain1 = gain2;
      //printf ("%f ", Rxx, Rxp, Rpp, gain);
      //printf ("%f ", gain1);
      Rxx = 0;
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] = P[j*2-1]+gain1*X[j*2-1];
         X[j*2  ] = P[j*2  ]+gain1*X[j*2  ];
         Rxx += X[j*2-1]*X[j*2-1];
         Rxx += X[j*2  ]*X[j*2  ];
      }
      //printf ("%f %f ", gain1, Rxx);
   }
   //printf ("\n");
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

void ceft_encode(CEFTState *st, float *in, float *out, float *pitch, float *window)
{
   //float bark[BARK_BANDS];
   float X[st->length];
   float Xbak[st->length];
   float Xp[st->length];
   int i;
   float bank[NBANDS];
   float pitch_bank[NBANDS];
   float p[st->length];
   
   for (i=0;i<st->length;i++)
      p[i] = pitch[i]*window[i];
   
#if 0
   for (i=0;i<st->length;i++)
      printf ("%f ", p[i]);
   for (i=0;i<st->length;i++)
      printf ("%f ", in[i]);
   printf ("\n");
#endif
                    
   spx_fft_float(st->frame_fft, in, X);
   spx_fft_float(st->frame_fft, p, Xp);
   
   /* Bands for the input signal */
   compute_bank(X, bank);
   normalise_bank(X, bank);
   
   /* Bands for the pitch signal */
   compute_bank(Xp, pitch_bank);
   normalise_bank(Xp, pitch_bank);
   
   
   for(i=0;i<st->length;i++)
      Xbak[i] = X[i];
   /*for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */
   pitch_quant_bank(X, Xp);
   
   if (1) {
#if 0
      for (i=0;i<st->length;i++)
         printf ("%f ", X[i]);
      for (i=0;i<st->length;i++)
         printf ("%f ", Xp[i]);
      printf ("\n");
#endif
#if 0
      float err1=0, err2=0, err0=0;
      for (i=0;i<19;i++)
      {
         err0 += (X[i])*(X[i]);
         err1 += (X[i]-Xp[i])*(X[i]-Xp[i]);
         err2 += (X[i]-.7*Xp[i])*(X[i]-.7*Xp[i]);
      }
      printf ("%f %f %f ", err0, err1, err2);
#endif
      for (i=1;i<st->length;i++)
         X[i] -= Xp[i];
      float tmp[NBANDS];
      compute_bank(X, tmp);
      normalise_bank(X, tmp);
      
   }
   //Quantise input
   quant_bank2(X);

   //Renormalise the quantised signal back to unity
   float bank2[NBANDS];
   compute_bank(X, bank2);
   normalise_bank(X, bank2);

   if (1) {
      pitch_renormalise_bank(X, Xp);
   }
   compute_bank(X, bank2);
   normalise_bank(X, bank2);

#if 0
   float err = 0;
   for(i=1;i<19;i++)
      err += (Xbak[i] - X[i])*(Xbak[i] - X[i]);
   printf ("%f\n", err);
#endif
   
   /* Denormalise back to real power */
   denormalise_bank(X, bank);
   
   //Synthesis
   spx_ifft_float(st->frame_fft, X, out);

}

