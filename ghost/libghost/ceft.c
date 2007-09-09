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

/* Number of bands to consider, excliding the DC */
#define NBANDS 15

/* Start frequency of each band. The two extra elements are for the end of the last band (+1) and the end of the array itself */
int qbank[] =   {1, 2, 4, 6, 8, 12, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};

/* Number of pulses in each band. The number of bits for each band with a non-zero
   number of pulses is equal to  (1 + nb_pulses * log2 (2 * width_of_band) )  */
//32 kbps
#define WAVEFORM_END 44
int qpulses[] = {3, 4, 4, 3, 3,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0}; //85 bits

//44 kbps
//int qpulses[] = {4, 7, 6, 4, 4,  3,  3,  3,  3,  3,  3,  3,  0,  0,  0}; //134 bits


/* Number of bands only for the pitch prediction */
#define PBANDS 5
/* Start frequency of each band */
int pbank[] = {1, 4, 8, 12, 20, 44};

/* Algebraic pulse-base quantiser. The signal x is replaced by the sum of the pitch 
   a combination of pulses such that its norm is still equal to 1 */
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
         score = 2*g*tmp_xy - g*g*tmp_yy;
         if (score>max_val)
         {
            max_val = score;
            best_id = j;
            best_xy = tmp_xy;
            best_yy = tmp_yy;
            best_yp = tmp_yp;
            gain = g;
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
   
   for (i=0;i<N;i++)
      x[i] = p[i]+gain*y[i];
   
}

/* Improved algebraic pulse-base quantiser. The signal x is replaced by the sum of the pitch 
   a combination of pulses such that its norm is still equal to 1. The only difference with 
   the quantiser above is that the search is more complete. */
void alg_quant2(float *x, int N, int K, float *p)
{
   int L = 5;
   float tata[200];
   float y[L][N];
   float tata2[200];
   float ny[L][N];
   int i, j, m;
   float xy[L], nxy[L];
   float yy[L], nyy[L];
   float yp[L], nyp[L];
   float best_scores[L];
   float Rpp=0;
   float gain[L];
   int maxL = 1;
   for (j=0;j<N;j++)
      Rpp += p[j]*p[j];
   for (m=0;m<L;m++)
      for (i=0;i<N;i++)
         y[m][i] = 0;
      
   for (m=0;m<L;m++)
      for (i=0;i<N;i++)
         ny[m][i] = 0;

   for (m=0;m<L;m++)
      xy[m] = yy[m] = yp[m] = gain[m] = 0;
   
   for (i=0;i<K;i++)
   {
      int L2 = L;
      if (L>maxL)
      {
         L2 = maxL;
         maxL *= N;
      }
      for (m=0;m<L;m++)
         best_scores[m] = -1e10;

      for (m=0;m<L2;m++)
      {
         for (j=0;j<N;j++)
         {
            //fprintf (stderr, "%d/%d %d/%d %d/%d\n", i, K, m, L2, j, N);
            float tmp_xy, tmp_yy, tmp_yp;
            float score;
            float g;
            tmp_xy = xy[m] + fabs(x[j]);
            tmp_yy = yy[m] + 2*fabs(y[m][j]) + 1;
            if (x[j]>0)
               tmp_yp = yp[m] + p[j];
            else
               tmp_yp = yp[m] - p[j];
            g = (sqrt(tmp_yp*tmp_yp + tmp_yy - tmp_yy*Rpp) - tmp_yp)/tmp_yy;
            score = 2*g*tmp_xy - g*g*tmp_yy;

            if (score>best_scores[L-1])
            {
               int k, n;
               int id = L-1;
               while (id > 0 && score > best_scores[id-1])
                  id--;
               
               for (k=L-1;k>id;k--)
               {
                  nxy[k] = nxy[k-1];
                  nyy[k] = nyy[k-1];
                  nyp[k] = nyp[k-1];
                  //fprintf(stderr, "%d %d \n", N, k);
                  for (n=0;n<N;n++)
                     ny[k][n] = ny[k-1][n];
                  gain[k] = gain[k-1];
                  best_scores[k] = best_scores[k-1];
               }

               nxy[id] = tmp_xy;
               nyy[id] = tmp_yy;
               nyp[id] = tmp_yp;
               gain[id] = g;
               for (n=0;n<N;n++)
                  ny[id][n] = y[m][n];
               if (x[j]>0)
                  ny[id][j] += 1;
               else
                  ny[id][j] -= 1;
               best_scores[id] = score;
            }
            
         }
         
      }
      int k,n;
      for (k=0;k<L;k++)
      {
         xy[k] = nxy[k];
         yy[k] = nyy[k];
         yp[k] = nyp[k];
         for (n=0;n<N;n++)
            y[k][n] = ny[k][n];
      }

   }
   
   for (i=0;i<N;i++)
      x[i] = p[i]+gain[0]*y[0][i];
   
}

/* Just replace the band with noise of unit energy */
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

/* Just replace the band with the replicated spectrum */
void sbr_quant(float *x, int N, int K, float *p)
{
   int i;
   float *sbr = x+WAVEFORM_END-N;

   float E = 1e-10;
   for (i=0;i<N;i++)
   {
      x[i] = sbr[i];
      E += x[i]*x[i];
   }
   E = 1./sqrt(E);
   for (i=0;i<N;i++)
   {
      x[i] *= E;
   }
}

/* Compute the energy in each of the bands */
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
      bank[i] = sqrt(bank[i]);
   }
}

/* Normalise each band such that the energy is one. */
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

/* De-normalise the energy to produce the synthesis from the unit-energy bands */
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

/* L2-norm of a vector */
float norm2(float *x, int len)
{
   float E=0;
   int i;
   for (i=0;i<len;i++)
      E += x[i]*x[i];
   return E;
}

/* Really crappy DFT for test purposes. May be harmful to young children */
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

/* Really crappy real-value DFT for test purposes. May be harmful to young children */
void crappy_rfft(float *X, int len, int R, int dir)
{
   int N=len*2;
   float x[N];
   int i;
   /*for (i=0;i<len;i++)
      printf ("%f ", X[i]);*/
   if (dir>0)
   {
      for (i=0;i<len;i++)
      {
         x[2*i] = X[i];
         x[2*i+1] = 0;
      }
      crappy_fft(x, N, R, 1);
      for (i=2;i<len;i++)
         X[i] = sqrt(2)*x[i];
      X[1] = x[len];
      X[0] = x[0];
   } else {
      for (i=1;i<len>>1;i++)
      {
         x[2*i] = sqrt(.5)*X[2*i];
         x[2*i+1] = sqrt(.5)*X[2*i+1];
         x[2*(len-i)] = sqrt(.5)*X[2*i];
         x[2*(len-i)+1] = -sqrt(.5)*X[2*i+1];
      }
      x[len] = X[1];
      x[len+1] = 0;
      x[1] = 0;
      x[0] = X[0];
      crappy_fft(x, N, R, -1);
      for (i=0;i<len;i++)
         X[i] = x[2*i];
   }
   /*printf ("  ");
   for (i=0;i<len;i++)
      printf ("%f ", X[i]);
   printf ("\n");*/
}

/* Applies a series of rotations so that pulses are spread like a two-sided exponential */
void exp_rotation(float *X, int len, float theta, int dir)
{
   int i;
   float c, s;
   c = cos(theta);
   s = sin(theta);
   if (dir > 0)
   {
      for (i=0;i<(len/2)-1;i++)
      {
         float x1, x2;
         x1 = X[2*i];
         x2 = X[2*i+2];
         X[2*i] = c*x1 - s*x2;
         X[2*i+2] = c*x2 + s*x1;
         
         x1 = X[2*i+1];
         x2 = X[2*i+3];
         X[2*i+1] = c*x1 - s*x2;
         X[2*i+3] = c*x2 + s*x1;
      }
      for (i=(len/2)-3;i>=0;i--)
      {
         float x1, x2;
         x1 = X[2*i];
         x2 = X[2*i+2];
         X[2*i] = c*x1 - s*x2;
         X[2*i+2] = c*x2 + s*x1;
         
         x1 = X[2*i+1];
         x2 = X[2*i+3];
         X[2*i+1] = c*x1 - s*x2;
         X[2*i+3] = c*x2 + s*x1;
      }

   } else {
      for (i=0;i<(len/2)-2;i++)
      {
         float x1, x2;
         x1 = X[2*i];
         x2 = X[2*i+2];
         X[2*i] = c*x1 + s*x2;
         X[2*i+2] = c*x2 - s*x1;
         
         x1 = X[2*i+1];
         x2 = X[2*i+3];
         X[2*i+1] = c*x1 + s*x2;
         X[2*i+3] = c*x2 - s*x1;
      }
      
      for (i=(len/2)-2;i>=0;i--)
      {
         float x1, x2;
         x1 = X[2*i];
         x2 = X[2*i+2];
         X[2*i] = c*x1 + s*x2;
         X[2*i+2] = c*x2 - s*x1;
         
         x1 = X[2*i+1];
         x2 = X[2*i+3];
         X[2*i+1] = c*x1 + s*x2;
         X[2*i+3] = c*x2 - s*x1;
      }
   }
}

/* Apply a rotation to all bands to spread the effect of pulses */
void random_rotation(float *X, int R, int dir)
{
   int i;
   for (i=0;i<NBANDS-4;i++)
   {
      //crappy_rfft(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), R+i, dir);
      //printf ("%f ", norm2(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i])));
      //rotate_vect(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), R+i, dir);
      
      float theta;
      if (qbank[i+1]-qbank[i] < 12)
         theta = .25;
      else
         theta = .5;
      exp_rotation(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), theta, dir);
   }
   //printf ("\n");
}

/* Quantise the normalised signal in each band */
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
         alg_quant2(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q, P+qbank[i]*2-1);
      else
         sbr_quant(X+qbank[i]*2-1, 2*(qbank[i+1]-qbank[i]), q, P+qbank[i]*2-1);
   }
   //FIXME: This is a kludge, even though I don't think it really matters much
   X[255] = 0;
}

/* Compute the best gain for each "pitch band" */
void compute_pitch_gain(float *X, float *P, float *gains, float *bank)
{
   int i;
   float w[256];
   for (i=0;i<NBANDS;i++)
   {
      int j;
      for (j=qbank[i];j<qbank[i+1];j++)
         w[j] = bank[i];
   }

   
   for (i=0;i<PBANDS;i++)
   {
      float Sxy=0;
      float Sxx = 0;
      int j;
      float gain;
      for (j=pbank[i];j<pbank[i+1];j++)
      {
         Sxy += X[j*2-1]*P[j*2-1]*w[j];
         Sxy += X[j*2]*P[j*2]*w[j];
         Sxx += X[j*2-1]*X[j*2-1]*w[j] + X[j*2]*X[j*2]*w[j];
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
   for (i=pbank[PBANDS]*2-1;i<256;i++)
      P[i] = 0;
   /*if (rand()%10 == 0)
   {
      for (i=0;i<PBANDS;i++)
         printf ("%f ", gains[i]*gains[i]);
      printf ("\n");
   }*/
}

/* Apply the (quantised) gain to each "pitch band" */
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
   for (i=pbank[PBANDS]*2-1;i<256;i++)
      P[i] = 0;
}

/* Scales the pulse-codebook entry in each band such that unit-energy is conserved when 
   adding the pitch */
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


/* Main CEFT encoder function */
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
   
   /* FFT of windowed input signal */
   spx_fft_float(st->frame_fft, in, X);

   /* Compute energy for each bands in the input signal */
   compute_bank(X, bank);

   /* Apply a window and FFT to the (already-delayed) pitch signal */
   for (i=0;i<st->length;i++)
      p[i] = pitch[i]*window[i];
   
   spx_fft_float(st->frame_fft, p, Xp);
   
   /* Compute energy for each bands in the pitch signal */
   compute_bank(Xp, pitch_bank);
   
   /* Enable to dump data for training purposes */
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
      /*printf ("%f ", 20*log10(1+fabs(X[0])));
      for (i=0;i<NBANDS;i++)
         printf ("%f ", 20*log10(1+bank[i]));*/
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
   
   /* Normalise each band to have unit energy */
   normalise_bank(X, bank);
   
   /* All the following is for quantisation of the energy in each band */
   float in_bank[NBANDS];
   float qbank[NBANDS];
   static float last_err[NBANDS];
   static float last_bank[NBANDS];

   /* Apply energy predictor */
   for (i=0;i<NBANDS;i++)
   {
      in_bank[i] = 20*log10(bank[i]+1) + .0*last_err[i+1] - .9*last_bank[i];
   }
   for (i=0;i<NBANDS;i++)
      qbank[i] = in_bank[i];
   
   /* Vecctor-quantise the prediction error */
   quantise_bands(in_bank, qbank, NBANDS);

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
   
   /* Undo the prediction */
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

   /* This is for quantisation of the DC. That is done independently from everything else */
   {
      float sign;
      int id;
      float q = 2.;
      if (X[0]<0)
         sign = -1;
      else
         sign = 1;
      id = floor(.5+20/q*log10(1+fabs(X[0])));
      if (id < 0)
         id = 0;
      if (id > 32)
      {
         printf("%d %f\n", id, X[0]);
         id = 32;
      }
      //printf ("%d %f ", id, X[0]);
      X[0] = sign*pow(10,(q*id)/20)-1;
      //printf ("%f\n", X[0]);
   }
   
   /* Normalise the pitch signal to have unit energy. */
   normalise_bank(Xp, pitch_bank);
   
   /*
   for(i=0;i<st->length;i++)
      Xbak[i] = X[i];
   for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */
   
   /* Apply spreading on input signal and pitch spectrum */
   random_rotation(X, 10, -1);
   random_rotation(Xp, 10, -1);
   
   compute_pitch_gain(X, Xp, gains, bank);
   quantise_pitch(gains, PBANDS);
   pitch_quant_bank(X, Xp, gains);
   
   /* Subtract the pitch prediction from the signal to encode */
   for (i=1;i<st->length;i++)
      X[i] -= Xp[i];

   //Quantise input
   quant_bank(X, Xp, centre);
   
   /* Undo the pulse spreading */
   random_rotation(X, 10, 1);
   //pitch_renormalise_bank(X, Xp);

#if 0
   float err = 0;
   for(i=1;i<19;i++)
      err += (Xbak[i] - X[i])*(Xbak[i] - X[i]);
   printf ("%f\n", err);
#endif
   //printf ("\n");

   /* Denormalise back to real power */
   denormalise_bank(X, bank);
   
   //Synthesis
   spx_ifft_float(st->frame_fft, X, out);

}

