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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fftwrap.h"


#define NBANDS 19
int qbank[] =   {1, 2, 3, 4, 5, 6, 8, 10, 12, 14, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};
//int qpulses[] = {3, 2, 2, 2, 2, 2, 2,  2,  2,  1,  1,  1,  1,  2,  2,  0,  0,  0,  0};
int qpulses[] = {3, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0};

//int qpulses[] = {3, 3, 2, 2, 2, 2, 2,  2,  2,  2,  3,  2,  2,  2,  3,  0,  0,  0,  0};

//int qpulses[] = {5, 5, 5, 5, 5, 5, 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5};
//int qpulses[] = {1, 1, 1, 1, 1, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1};

//#define PBANDS 5
//int pbank[] = {1, 4, 10, 16, 28, 44};

#define PBANDS 5
int pbank[] = {1, 3, 6, 12, 18, 44};

//#define PBANDS NBANDS
//#define pbank qbank

void alg_quant(float *x, int N, int K, float *p)
{
   float y[N];
   int i,j;
   float xy = 0;
   float yy = 0;
   float yp = 0;
   float Rpp=0;
   float gain=0;
   for (j=0;j<N;j++)
      Rpp += p[j]*p[j];
   for (i=0;i<N;i++)
      y[i] = 0;
   
   if (0)
   {
      int tmp = N;
      int b=0;
      while (tmp>1)
      {
         b++;
         tmp >>= 1;
      }
      printf ("%d\n", 1+K*b);
   }
   
   for (i=0;i<K;i++)
   {
      int best_id=0;
      float max_val=-1e10;
      float best_xy=0, best_yy=0, best_yp = 0;
      for (j=0;j<N;j++)
      {
         float tmp_xy, tmp_yy, tmp_yp;
         float score;
         float g;
         tmp_xy = xy + fabs(x[j]);
         tmp_yy = yy + 2*fabs(y[j]) + 1;
         if (x[j]>0)
            tmp_yp = yp + p[j];
         else
            tmp_yp = yp - p[j];
         g = (sqrt(tmp_yp*tmp_yp + tmp_yy - tmp_yy*Rpp) - tmp_yp)/tmp_yy;
         //g = 1/sqrt(tmp_yy);
         score = 2*g*tmp_xy - g*g*tmp_yy;
         //score = tmp_xy*tmp_xy/tmp_yy;
         if (score>max_val)
         {
            max_val = score;
            best_id = j;
            best_xy = tmp_xy;
            best_yy = tmp_yy;
            best_yp = tmp_yp;
            gain = g;
         }
         if (0) {
            float ny[N];
            int k;
            for (k=0;k<N;k++)
               ny[k] = y[k];
            if (x[j]>0)
               ny[j] += 1;
            else
               ny[j] -= 1;
            float E = 0;
            for (k=0;k<N;k++)
               E += (p[k]+g*ny[k])*(p[k]+g*ny[k]);
            printf ("(%f %f %f) ", g, tmp_yp, E);
         }
      }
      
      xy = best_xy;
      yy = best_yy;
      yp = best_yp;
      if (x[best_id]>0)
         y[best_id] += 1;
      else
         y[best_id] -= 1;
   }
   if (0) {
      int k;
      float E = 0, Ex=0;
      for (k=0;k<N;k++)
         E += (p[k]+gain*y[k])*(p[k]+gain*y[k]);
      for (k=0;k<N;k++)
         Ex += (x[k]*x[k]);
      printf ("** %f %f %f ", E, Ex, Rpp);
   }

   //printf ("\n");
   for (i=0;i<N;i++)
      x[i] = p[i]+gain*y[i];
   
}

void noise_quant(float *x, int N, int K, float *p)
{
   int i;
   float E = 1e-10;
   for (i=0;i<N;i++)
   {
      x[i] = (rand()%1000)/500.+1;
      E += x[i]*x[i];
   }
   E = 1./sqrt(E);
   for (i=0;i<N;i++)
   {
      x[i] *= E;
   }
}

void compute_bank(float *X, float *bank)
{
   int i;
   for (i=0;i<NBANDS;i++)
   {
      int j;
      bank[i] = 1e-10;
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         bank[i] += X[j*2-1]*X[j*2-1];
         bank[i] += X[j*2]*X[j*2];
      }
      //bank[i] = sqrt(.5*bank[i]/(qbank[i+1]-qbank[i]));
      bank[i] = sqrt(bank[i]);
   }
}

void normalise_bank(float *X, float *bank)
{
   int i;
   for (i=0;i<NBANDS;i++)
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
   X[255] = 1;
}

void denormalise_bank(float *X, float *bank)
{
   int i;
   for (i=0;i<NBANDS;i++)
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

void quant_bank(float *X, float *P)
{
   int i;
   for (i=0;i<NBANDS;i++)
   {
      int q=qpulses[i];
      if (q)
         alg_quant(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q, P+qbank[i]*2-1);
      else
         noise_quant(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q, P+qbank[i]*2-1);
   }
   //FIXME: This is a kludge, even though I don't think it really matters much
   X[255] = 0;
}

void pitch_quant_bank(float *X, float *P, float *gains)
{
   int i;
   for (i=0;i<PBANDS;i++)
   {
      float Sxy=0;
      float Sxx = 0;
      int j;
      float gain;
      for (j=pbank[i];j<pbank[i+1];j++)
      {
         Sxy += X[j*2-1]*P[j*2-1];
         Sxy += X[j*2]*P[j*2];
         Sxx += X[j*2-1]*X[j*2-1] + X[j*2]*X[j*2];
      }
      gain = Sxy/(1e-10+Sxx);
      //gain = Sxy/(2*(pbank[i+1]-pbank[i]));
      //if (i<3)
      //gain *= 1+.02*gain;
      if (gain > .90)
         gain = .90;
      if (gain < 0.0)
         gain = 0.0;

      gains[i] = gain;
      for (j=pbank[i];j<pbank[i+1];j++)
      {
         P[j*2-1] *= gain;
         P[j*2] *= gain;
      }
      //printf ("%f ", gain);
   }
   for (i=pbank[PBANDS];i<256;i++)
      P[i] = 0;
   P[255] = 0;
   //printf ("\n");
}

void pitch_renormalise_bank(float *X, float *P)
{
   int i;
   for (i=0;i<NBANDS;i++)
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
      //Rxx *= .5/(qbank[i+1]-qbank[i]);
      //Rxp *= .5/(qbank[i+1]-qbank[i]);
      //Rpp *= .5/(qbank[i+1]-qbank[i]);
      float arg = Rxp*Rxp + 1 - Rpp;
      if (arg < 0)
      {
         printf ("arg: %f %f %f %f\n", arg, Rxp, Rpp, Rxx);
         arg = 0;
      }
      gain1 = sqrt(arg)-Rxp;
      if (Rpp>.9999)
         Rpp = .9999;
      //gain1 = sqrt(1.-Rpp);
      
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


struct CEFTState_ {
   void *frame_fft;
   int length;
};

CEFTState *ceft_init(int len)
{
   CEFTState *st = malloc(sizeof(CEFTState));
   st->length = len;
   st->frame_fft = spx_fft_init(st->length);
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
   float gains[PBANDS];
   
   spx_fft_float(st->frame_fft, in, X);

   /* Bands for the input signal */
   compute_bank(X, bank);
/*
   if (rand()%10 ==0 && fabs(X[0]) > 2 && (fabs(X[0]) > 10 || rand() % 5 == 0))
   {
      printf ("%f ", 20*log10(1+fabs(X[0])));
      for (i=0;i<NBANDS;i++)
         printf ("%f ", 20*log10(1+bank[i]));
      printf ("\n");
   }
   return;*/
  
   for (i=0;i<st->length;i++)
      p[i] = pitch[i]*window[i];
   
   spx_fft_float(st->frame_fft, p, Xp);
   
   normalise_bank(X, bank);
   
   float in_bank[NBANDS+1];
   float qbank[NBANDS+1];
   static float last_err[NBANDS+1];

   in_bank[0] = 20*log10(fabs(X[0])+1);
   for (i=0;i<NBANDS;i++)
   {
      in_bank[i+1] = 20*log10(bank[i]+1) + .4*last_err[i+1];
   }
   for (i=0;i<NBANDS+1;i++)
      qbank[i] = in_bank[i];
   
   //quantise_bands(in_bank, qbank, NBANDS+1);

#if 0
   float q = .25f;
   for (i=0;i<NBANDS+1;i++)
   {
      if (i<1)
         q = 1.;
      else if (i<4)
         q = 2.;
      else if (i<5)
         q = 3.;
      else if (i<10)
         q = 4.;
      else if (i<14)
         q = 5.;
      else
         q = 6.;
      int sc = floor(.5 + (in_bank[i]-qbank[i])/q);
      printf ("%d ", sc);
      qbank[i] = q * sc;
   }
   printf ("\n");
#endif
   
   for (i=0;i<NBANDS+1;i++)
      printf ("%f ", in_bank[i]-qbank[i]);
   printf ("\n");
   
   
   for (i=0;i<NBANDS+1;i++)
      last_err[i] = qbank[i]-in_bank[i];
   
   for (i=0;i<NBANDS;i++)
   {
      bank[i] = pow(10,qbank[i+1]/20)-1;
      if (bank[i] < .1)
         bank[i] = .1;
   }
   /* Bands for the pitch signal */
   compute_bank(Xp, pitch_bank);
   normalise_bank(Xp, pitch_bank);
   
   /*
   for(i=0;i<st->length;i++)
      Xbak[i] = X[i];
   for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */
   pitch_quant_bank(X, Xp, gains);
      
   for (i=1;i<st->length;i++)
      X[i] -= Xp[i];

   //Quantise input
   quant_bank(X, Xp);
   
   //pitch_renormalise_bank(X, Xp);

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

