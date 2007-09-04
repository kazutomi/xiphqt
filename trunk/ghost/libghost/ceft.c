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

#define NBANDS 15
int qbank[] =   {1, 2, 4, 6, 8, 12, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};
int qpulses[] = {3, 3, 2, 2, 3,  3,  2,  2,  1,  2,  2,  0,  0,  0,  0};
//int qpulses[] = {5, 5, 3, 3, 3,  3,  2,  2,  2,  3,  3,  0,  0,  0,  0};

//#define NBANDS 19
//int qbank[] =   {1, 2, 3, 4, 5, 6, 8, 10, 12, 14, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};
//int qpulses[] = {3, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0};
//int qpulses[] = {3, 2, 2, 2, 2, 2, 2,  2,  2,  1,  1,  1,  1,  2,  2,  0,  0,  0,  0};

//int qpulses[] = {3, 3, 2, 2, 2, 2, 2,  2,  2,  2,  3,  2,  2,  2,  3,  0,  0,  0,  0};

//int qpulses[] = {5, 5, 5, 5, 5, 5, 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5};
//int qpulses[] = {1, 1, 1, 1, 1, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1};

//#define PBANDS 5
//int pbank[] = {1, 4, 10, 16, 28, 44};

#define PBANDS 5
int pbank[] = {1, 4, 8, 12, 18, 44};

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
   
   float boost[N];
   for (i=0;i<N;i++)
      boost[i] = 0;
   if (K==1)
   {
      float centre;
      float Sxw = 0, Sw = 1e-10;
      for (i=0;i<N/2;i++)
      {
         float weight = x[2*i]*x[2*i] + x[2*i+1]*x[2*i+1];
         Sxw += i*weight;
         Sw  += weight;
      }
      centre = Sxw/Sw;
      centre = floor(centre)+(rand()&1);
      //printf ("%d ", (int)centre);
      for (i=0;i<N/2;i++)
         boost[2*i] = boost[2*i+1] = (1.f/N)*(N-fabs(i-centre));
   }
   
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
         score = 2*g*tmp_xy - g*g*tmp_yy + boost[j];
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
      
      //if (K==1)
      //   printf ("%d\n", best_id/2);
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
      x[i] = (rand()%1000)/500.-1;
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
   for (i=2*qbank[NBANDS]-1;i<256;i++)
      X[i] = 0;
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
void crappy_fft(float *X, int len, int R, int dir)
{
   int i, j;
   float fact = 2*M_PI/(len>>1);
   float out[len];
   for (i=0;i<len>>1;i++)
   {
      out[2*i] = 0;
      out[2*i+1] = 0;
      for (j=0;j<len>>1;j++)
      {
         float c, d;
         c = cos(fact*j*i);
         if (dir == 1)
            d = -sin(fact*j*i);
         else
            d = -sin(fact*j*i);
            
         out[2*i  ] += c*X[2*j] + d*X[2*j+1];
         out[2*i+1] += d*X[2*j] + c*X[2*j+1];
      }
   }
   for (i=0;i<len;i++)
      X[i] = out[i]/sqrt(len/2);
}

void crappy_rfft(float *X, int len, int R, int dir)
{
   int N=len*2;
   float x[N];
   int i;
   if (dir>0)
   {
      for (i=0;i<len;i++)
      {
         x[2*i] = X[i];
         x[2*i+1] = 0;
      }
      crappy_fft(x, N, R, 1);
      for (i=0;i<len;i++)
         X[i] = x[i];
      X[1] = x[len];
   } else {
      for (i=1;i<len>>1;i++)
      {
         x[2*i] = X[2*i];
         x[2*i+1] = X[2*i+1];
         x[2*(len-i)] = X[2*i];
         x[2*(len-i)+1] = X[2*i+1];
      }
      x[len] = X[1];
      x[len+1] = 0;
      x[1] = 0;
      x[0] = X[0];
      crappy_fft(x, N, R, -1);
      for (i=0;i<len;i++)
         X[i] = x[2*i];
   }
}

void random_rotation(float *X, int R, int dir)
{
   int i;
   for (i=0;i<NBANDS;i++)
   {
      crappy_rfft(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), R+i, dir);
      //rotate_vect(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), R+i, dir);
   }
}


void quant_bank(float *X, float *P, float centre)
{
   int i;
   for (i=0;i<NBANDS;i++)
   {
      int q;
      /*if (centre < 5)
         q =qpulses3[i];
      else if (centre < 8)
         q =qpulses2[i];
      else
         q =qpulses[i];*/
      q =qpulses[i];
      if (q)
         alg_quant(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q, P+qbank[i]*2-1);
      else
         noise_quant(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q, P+qbank[i]*2-1);
   }
   //FIXME: This is a kludge, even though I don't think it really matters much
   X[255] = 0;
}

void compute_pitch_gain(float *X, float *P, float *gains)
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
   }
   for (i=pbank[PBANDS];i<256;i++)
      P[i] = 0;
   P[255] = 0;
   /*if (rand()%10 == 0)
   {
      for (i=0;i<PBANDS;i++)
         printf ("%f ", gains[i]*gains[i]);
      printf ("\n");
   }*/
}

void pitch_quant_bank(float *X, float *P, float *gains)
{
   int i;
   for (i=0;i<PBANDS;i++)
   {
      int j;
      for (j=pbank[i];j<pbank[i+1];j++)
      {
         P[j*2-1] *= gains[i];
         P[j*2] *= gains[i];
      }
      //printf ("%f ", gain);
   }
   for (i=pbank[PBANDS];i<256;i++)
      P[i] = 0;
   P[255] = 0;
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

float norm2(float *x, int len)
{
   float E=0;
   int i;
   for (i=0;i<len;i++)
      E += x[i]*x[i];
   return E;
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
   float mask[NBANDS];
   static float obank[NBANDS+1];
   spx_fft_float(st->frame_fft, in, X);

   /* Bands for the input signal */
   compute_bank(X, bank);

   for (i=0;i<st->length;i++)
      p[i] = pitch[i]*window[i];
   
   spx_fft_float(st->frame_fft, p, Xp);
   
   /* Bands for the pitch signal */
   compute_bank(Xp, pitch_bank);
#if 0
   if (rand()%10 ==0 && fabs(X[0]) > 2 && (fabs(X[0]) > 10 || rand() % 5 == 0))
   {
      /*printf ("%f ", 20*log10(1+fabs(X[0])));
      for (i=0;i<NBANDS;i++)
         printf ("%f ", 20*log10(1+bank[i]));
      for (i=0;i<NBANDS+1;i++)
         printf ("%f ", obank[i]);
      printf ("%f ", 20*log10(1+fabs(Xp[0])));
      for (i=0;i<NBANDS;i++)
         printf ("%f ", 20*log10(1+pitch_bank[i]));
      printf (" \n");*/
      for (i=0;i<NBANDS;i++)
         printf ("%f ", 20*log10(1+bank[i])-.9*obank[i+1]);
      printf ("\n");

   }
   obank[0] = 20*log10(1+fabs(X[0]));
   for (i=0;i<NBANDS;i++)
      obank[i+1] = 20*log10(1+bank[i]);
   return;
#endif
                       
#if 0
   float tmp = 1.+X[0]*X[0];
   for (i=0;i<NBANDS;i++)
   {
      tmp = 1. + .1*tmp + bank[i]*bank[i];
      mask[i] = bank[i]*bank[i]/tmp;
      printf ("%f ", 10*log10(mask[i]));
   }
   printf ("\n");
#endif
   
   float centre=0;
#if 0
   float Sxw = 0, Sw = 0;
   for (i=0;i<NBANDS;i++)
   {
      printf ("%f ", 20*log10(bank[i]));
      Sxw += i*sqrt(bank[i]);
      Sw  += sqrt(bank[i]);
   }
   centre = Sxw/Sw;
            //printf ("%f\n", Sxw/Sw);
#endif         
   
   
   normalise_bank(X, bank);
   
   float in_bank[NBANDS];
   float qbank[NBANDS];
   static float last_err[NBANDS];
   static float last_bank[NBANDS];

   for (i=0;i<NBANDS;i++)
   {
      in_bank[i] = 20*log10(bank[i]+1) + .0*last_err[i+1] - .9*last_bank[i];
   }
   for (i=0;i<NBANDS;i++)
      qbank[i] = in_bank[i];
   
   //quantise_bands(in_bank, qbank, NBANDS);

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
   
   /*for (i=0;i<NBANDS+1;i++)
      printf ("%f ", in_bank[i]-qbank[i]);
   printf ("\n");*/
   
   
   for (i=0;i<NBANDS;i++)
      last_err[i] = qbank[i]-in_bank[i];
   
   for (i=0;i<NBANDS;i++)
   {
      qbank[i] += .9*last_bank[i];
      if (qbank[i]<0)
         qbank[i] = 0;
      bank[i] = pow(10,(qbank[i])/20)-1;
      if (bank[i] < .1)
         bank[i] = .1;
   }
   for (i=0;i<NBANDS;i++)
      last_bank[i] = qbank[i];

   {
      float sign;
      int id;
      float q = .25;
      if (X[0]<0)
         sign = -1;
      else
         sign = 1;
      id = floor(.5+20/q*log10(1+fabs(X[0])));
      if (id < 0)
         id = 0;
      if (id > 255)
      {
         printf("%d %f\n", id, X[0]);
         id = 255;
      }
      //printf ("%d %f ", id, X[0]);
      X[0] = sign*pow(10,(q*id)/20)-1;
      //printf ("%f\n", X[0]);
   }
   
   normalise_bank(Xp, pitch_bank);
   
   /*
   for(i=0;i<st->length;i++)
      Xbak[i] = X[i];
   for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */
   
   //random_rotation(X, 10, -1);
   //random_rotation(Xp, 10, -1);
   
   compute_pitch_gain(X, Xp, gains);
   quantise_pitch(gains, PBANDS);
   pitch_quant_bank(X, Xp, gains);
      
   for (i=1;i<st->length;i++)
      X[i] -= Xp[i];

   //Quantise input
   quant_bank(X, Xp, centre);
   
   //random_rotation(X, 10, 1);
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

