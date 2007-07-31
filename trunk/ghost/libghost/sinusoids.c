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

/* Find the sinusoids present in a frame -- THIS CAN BE IMPROVED
 * psd is the power spectral density of the signal
 * w are the returned sinudois frequencies (0 to pi)
 * N is the max number of sinusoids (returns the actual number)
 * length is the size of the psd
 */
void find_sinusoids(float *psd, float *w, int *N, int length)
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
         sinusoidism[i] = 1*psd[i] + 1*(highlobe + lowlobe - .5*MAX(lowlobe, highlobe));
      } else {
         sinusoidism[i] = -1;
      }
   }
   /*for (i=0;i<=length;i++)
   {
      fprintf (stderr, "%f ", sinusoidism[i]);
   }
   fprintf (stderr, "\n");*/
   for (i=0;i<*N;i++)
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
      //printf ("%f ", sinusoidism[tmp_id]);
      if (sinusoidism[tmp_id]<50)
      {
         *N=i;
         break;
      }
      sinusoidism[tmp_id] = -3;
      //w[i] = M_PI*tmp_id/(length-1);
      float corr;
      float side;
      if (psd[tmp_id+1]> psd[tmp_id-1])
         side=psd[tmp_id]-psd[tmp_id+1];
      else
         side=psd[tmp_id]-psd[tmp_id-1];
      if (side>6)
         corr = 0;
      else if (side<0)
         corr = 0;
      else
         corr = .5-.5*(side/6);
      if (psd[tmp_id+1] < psd[tmp_id-1])
         corr =- corr;
      //printf ("%f\n",corr);
      w[i] = M_PI*(tmp_id+corr)/(length-1);
   }
   //printf ("\n");
}

#define MP_RES 4096
#define MP_LEN 256
/* This is a trivial matching pursuit implementation. It's awfully slow, so beware */
void extract_sinusoids_mp(float *x, float *w, float *y, int N, int len)
{
   static float cos_table[MP_RES][MP_LEN];
   static float sin_table[MP_RES][MP_LEN];
   static float sinE[MP_RES];
   static float cosE[MP_RES];
   static init = 0;
   float res[len];
   int i,j;
   if (!init)
   {
      for (i=0;i<MP_RES;i++)
      {
         sinE[i] = 0;
         cosE[i] = 0;
         for (j=0;j<MP_LEN;j++)
         {
            float jj=j-(MP_LEN/2)+.5;
            cos_table[i][j] = cos((M_PI*(float)i/(float)MP_RES)*jj);
            sin_table[i][j] = sin((M_PI*(float)i/(float)MP_RES)*jj);
            cosE[i] += cos_table[i][j]*cos_table[i][j];
            sinE[i] += sin_table[i][j]*sin_table[i][j];
         }
      }
      init = 1;
   }
   
   for (i=0;i<len;i++)
      res[i] = x[i];
   //Loop on all sinusoids we want to extract
   for (i=0;i<N;i++)
   {
      int k;
      float best_fit=-1;
      int best_id=1;
      float best_gainC=0;
      float best_gainS=0;
      float fit = 0;
      
      //Find best sinusoid
      for (j=1;j<MP_RES/4-1;j++)
      {
         float sumC = 0, sumS = 0;
         for (k=0;k<len;k++)
         {
            sumC += res[k]*cos_table[j][k];
            sumS += res[k]*sin_table[j][k];
         }
         fit = sumC/cosE[j]*sumC + sumS/sinE[j]*sumS;
         if (fit > best_fit)
         {
            best_id = j;
            best_fit = fit;
            best_gainC = sumC/cosE[j];
            best_gainS = sumS/sinE[j];
         }
      }
      
      w[i] = M_PI*best_id/MP_RES;
            
      //Remove the sinusoid we found
      for (k=0;k<len;k++)
         res[k] = res[k] - best_gainC*cos_table[best_id][k] - best_gainS*sin_table[best_id][k];
      //printf ("%d %f %f\n", best_id, best_gainC, best_gainS);      
   }
   for (i=0;i<len;i++)
      y[i] = x[i] - res[i];

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
         if (iter==4)
         {
            printf ("%f %f ", w[i], (float)sqrt(ai[i]*ai[i] + bi[i]*bi[i]));
         }
      }
   }
   printf ("\n");
}

/* Models the signal as modulated sinusoids 
 * x      is the signal
 * w      are the initial frequencies
 * window is the analysis window (you can use a rectangular window)
 * A      has the amplitude of the sinusoids
 * phi    has the phase of the sinusoids (centered in the middle of the frame)
 * dA     has the amplitude derivative (centered in the middle of the frame)
 * dw     has the frequency offset of the last iteration
 * y      is the approximated signal by summing all the params
 * N      is the number of sinusoids
 * len    is the frame size
*/
void extract_modulated_sinusoids(float *x, float *w, float *window, float *A, float *phi, float *dA, float *dw, float *y, int N, int len)
{
   int L2 = len/2;
   float cos_table[N][L2];
   float sin_table[N][L2];
   float tcos_table[N][L2];
   float tsin_table[N][L2];
   float cosE[N], sinE[N], costE[N], sintE[N];
   float cosE_1[N], sinE_1[N], costE_1[N], sintE_1[N];
   float e[len];
   /* Symmetric and anti-symmetric components of the error */
   float sym[L2], anti[L2];
   float ai[N], bi[N], ci[N], di[N];
   float w0[N];
   int i,j, iter, nonlin;
   static count = 0;
   count++;
   for (i=0;i<N;i++)
      w0[i] = w[i];
   
   for (i=0;i<N;i++)
      A[i] = dA[i] = phi[i] = dw[i] = 0;
   for (i=0;i<N;i++)
      ai[i] = bi[i] = ci[i] = di[i] = 0;
   
   nonlin = 10;
   /* Iterate on the non-linear part (frequency) a couple times */
   while (1)
   {
      /* Build a table for the four basis functions at each frequency: cos(x), sin(x), x*cos(x), x*sin(x)*/
      for (i=0;i<N;i++)
      {
         float tmp1=0, tmp2=0;
         float tmp3=0, tmp4=0;
         float rotR, rotI;
         rotR = cos(w[i]);
         rotI = sin(w[i]);
         /* Computing sin/cos table using complex rotations.
            Only need to compute the tables for half the length because of the symmetry.*/
         cos_table[i][0] = cos(.5*w[i]);
         sin_table[i][0] = sin(.5*w[i]);
         for (j=1;j<L2;j++)
         {
            float re, im;
            re = cos_table[i][j-1]*rotR - sin_table[i][j-1]*rotI;
            im = sin_table[i][j-1]*rotR + cos_table[i][j-1]*rotI;
            cos_table[i][j] = re;
            sin_table[i][j] = im;
         }
         for (j=0;j<L2;j++)
            cos_table[i][j] *= window[j+L2];
         for (j=0;j<L2;j++)
            sin_table[i][j] *= window[j+L2];
         /* Compute energy of basis functions. There may be a closed-form solution for that
            or we could tabulate it. */
         for (j=0;j<L2;j++)
         {
            float jj = j+.5;
            /*cos_table[i][j] = cos(w[i]*jj)*window[j];
               sin_table[i][j] = sin(w[i]*jj)*window[j];*/
            tcos_table[i][j] = jj*cos_table[i][j];
            tsin_table[i][j] = jj*sin_table[i][j];
            /* The sinusoidal terms */
            tmp1 += cos_table[i][j]*cos_table[i][j];
            tmp2 += sin_table[i][j]*sin_table[i][j];
            /* The modulation terms */
            tmp3 += tcos_table[i][j]*tcos_table[i][j];
            tmp4 += tsin_table[i][j]*tsin_table[i][j];
         }
         /* Double the energy because we only computed one half.
            Eventually, we should be computing/tabulating these values directly
            as a function of w[i]. */
         cosE[i] = sqrt(2*tmp1);
         sinE[i] = sqrt(2*tmp2);
         costE[i] = sqrt(2*tmp3);
         sintE[i] = sqrt(2*tmp4);
         cosE_1[i] = 1.f/cosE[i];
         sinE_1[i] = 1.f/sinE[i];
         costE_1[i] = 1.f/costE[i];
         sintE_1[i] = 1.f/sintE[i];
         /* Normalise the basis (should multiply by the inverse instead) */
         for (j=0;j<L2;j++)
         {
            cos_table[i][j] *= cosE_1[i];
            sin_table[i][j] *= sinE_1[i];
            tcos_table[i][j] *= costE_1[i];
            tsin_table[i][j] *= sintE_1[i];
         }
      }

      /* Split the error into a symmetric component and an anti-symmetric component. 
         This speeds everything up by a factor of 2 */
      for (j=0;j<L2;j++)
      {
         sym[j] = x[j+L2]+x[L2-j-1];
         anti[j] = x[j+L2]-x[L2-j-1];
      }

      /* Start the linear optimisation where we left last time */
      for (i=0;i<N;i++)
      {
         ai[i] = A[i]*cos(phi[i])*cosE[i];
         bi[i] = -A[i]*sin(phi[i])*sinE[i];
         ci[i] = dA[i]*cos(phi[i])*costE[i];
         di[i] = -dA[i]*sin(phi[i])*sintE[i];
         for (j=0;j<L2;j++)
            sym[j] -= (2*ai[i])*cos_table[i][j] + (2*di[i])*tsin_table[i][j];
         for (j=0;j<L2;j++)
            anti[j] -= (2*bi[i])*sin_table[i][j] + (2*ci[i])*tcos_table[i][j];
      }
   
      /* Stop condition. We stop here, just after updating the error using the new frequency */
      if (nonlin-- <= 0)
         break;
   
      /* This is an iterative linear solution -- much quicker than inverting a matrix.
         It's pretty much the Gauss-Seidel algorithm */
      for (iter=0;iter<1;iter++)
      {
         for (i=0;i<N;i++)
         {
            float tmp1=0, tmp2=0;
            float tmp3=0, tmp4=0;
            /* For each of the four basis functions, project the residual (symmetric or 
               anti-symmetric) onto the basis function, then update the residual. */
            for (j=0;j<L2;j++)
               tmp1 += sym[j]*cos_table[i][j];
            for (j=0;j<L2;j++)
               sym[j] -= (2*tmp1)*cos_table[i][j];
         
            for (j=0;j<L2;j++)
               tmp2 += anti[j]*sin_table[i][j];
            for (j=0;j<L2;j++)
               anti[j] -= (2*tmp2)*sin_table[i][j];

            for (j=0;j<L2;j++)
               tmp3 += anti[j]*tcos_table[i][j];
            for (j=0;j<L2;j++)
               anti[j] -= (2*tmp3)*tcos_table[i][j];

            for (j=0;j<L2;j++)
               tmp4 += sym[j]*tsin_table[i][j];
            for (j=0;j<L2;j++)
               sym[j] -= (2*tmp4)*tsin_table[i][j];
         
            ai[i] += tmp1;
            bi[i] += tmp2;
            ci[i] += tmp3;
            di[i] += tmp4;
         }
      }
      for (i=0;i<N;i++)
      {
         ai[i] *= cosE_1[i];
         bi[i] *= sinE_1[i];
         ci[i] *= costE_1[i];
         di[i] *= sintE_1[i];
      }

      /* Update real sinusoid parameters */
      for (i=0;i<N;i++)
      {
         //float A, phi, dA, dw;
         A[i] = sqrt(ai[i]*ai[i] + bi[i]*bi[i]);
         if (A[i]<.01)
            phi[i] = 0;
         else
            phi[i] = atan2(-bi[i], ai[i]);
         dA[i] = (ci[i]*ai[i] + bi[i]*di[i])/(.01f+A[i]);
         dw[i] = (ci[i]*bi[i] - di[i]*ai[i])/(.01f+A[i]*A[i]);
         //printf ("%f %f %f %f %f %f %f %f %f\n", w[i], ai[i], bi[i], ci[i], di[i], A, phi, dA, dw);
         /* Update the frequency with only a fraction of the dw term to prevent instabilities */
         w[i] = w[i] + dw[i];
         /* Make sure we didn't deviate too much from the original frequency */
         if (w[i] > w0[i]+.025)
            w[i] = w0[i]+.025;
         if (w[i] < w0[i]-.025)
            w[i] = w0[i]-.025;
         //Prevent stupid frequencies (i.e. outsite of ]0,pi[
         if (w[i] < .002)
            w[i] = .002;
         if (w[i] > M_PI-.01)
            w[i] = M_PI-.01;
         //if (nonlin == 4)
         //   printf("%d %f %f %f %f %f %f %f %f %f %f\n", count, w[i], ai[i], bi[i], ci[i], di[i], A[i], phi[i], dA[i], dw[i], w0[i]);
      }
   }
   
   for (j=0;j<L2;j++)
   {
      e[j+L2] = .5*(sym[j]+anti[j]);
      e[L2-j-1] = .5*(sym[j]-anti[j]);
   }
   for (j=0;j<len;j++)
      y[j] = x[j]-e[j];

}
