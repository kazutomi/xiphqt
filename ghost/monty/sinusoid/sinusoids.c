/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007                   *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: research-grade sinusoidal extraction sode
 last mod: $Id$

 ********************************************************************/

#include <math.h>
#include "sinusoids.h"
#include "scales.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* f: input; log magnitude (decibel scale) of input spectrum */

void level_mean(float *f,float *out,int n, float lowindow, float hiwindow, int min, int rate){
  float nevent[n];
  float devent[n];

  float nadd = 0.f;
  float dadd = 0.f;
  float nacc = 0.f;
  float dacc = 0.f;

  float binwidth = rate*.5f/n; 
  float ibinwidth = 1.f/binwidth;

  int head=-1;
  int tail=-1;
  int i;
  
  memset(nevent,0,sizeof(nevent));
  memset(devent,0,sizeof(devent));

  /* init case for hi-side window */
  int lo;
  int hi = rint(fromBark(hiwindow)*ibinwidth);
  if(hi<min)hi=min;

  for(head=0;head<hi;head++){
    float de = 1./hi;
    float nu = f[head]*de;

    nevent[head] -= nu;
    devent[head] -= de;

    nadd += nu;
    dadd += de;
    
    nacc +=nadd;
    dacc +=dadd;
  }

  for(i=0;i<n;i++){
    float bark = toBark(i*binwidth);
    hi = rint(fromBark(bark+hiwindow)*ibinwidth)-i;
    if(bark>lowindow)
      lo = i-rint(fromBark(bark-lowindow)*ibinwidth);
    else
      lo = rint(fromBark(lowindow)*ibinwidth);
    if(hi<min)hi=min;
    if(lo<min)lo=min;

    /* high side window*/
    for(;head<i+hi;head++){
      if(head<n){
	float de = 1./hi;
	float nu = f[head]*de;

	nevent[i+hi] -= nu;
	devent[i+hi] -= de;
	
	nadd += nu;
	dadd += de;

      }
    }

    /* low side window */
    {
      float de = 1./lo;
      float nu = f[i]*de;

      if(i+lo<n){
	nevent[i+lo] += nu;
	devent[i+lo] += de;
      }
      if(i<n){
	nevent[i] -= nu;
	devent[i] -= de;
      }
    }

    nadd += nevent[i];
    dadd += devent[i];
    
    nacc += nadd;
    dacc += dadd;

    out[i] = nacc/dacc; 
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
         if (iter==4)
         {
            printf ("%f %f ", w[i], (float)sqrt(ai[i]*ai[i] + bi[i]*bi[i]));
         }
      }
   }
   printf ("\n");
}

/* Models the signal as modulated sinusoids 
 * x is the signal
 * w are the frequencies
 * window is the analysis window (you can use a rectangular window)
 * ai are the cos(x) coefficients
 * bi are the sin(x) coefficients
 * ci are the x*cos(x) coefficients
 * di are the x*sin(x) coefficients
 * y is the approximated signal by summing all the params
 * N is the number of sinusoids
 * len is the frame size
*/
void extract_modulated_sinusoids(float *x, float *w, float *window, float *ai, float *bi, float *ci, float *di, float *y, int N, int len)
{
   float cos_table[N][len];
   float sin_table[N][len];
   float tcos_table[N][len];
   float tsin_table[N][len];
   float cosE[N], sinE[N];
   float costE[N], sintE[N];
   int i,j, iter;
   /* Build a table for the four basis functions at each frequency: cos(x), sin(x), x*cos(x), x*sin(x)*/
   for (i=0;i<N;i++)
   {
      float tmp1=0, tmp2=0;
      float tmp3=0, tmp4=0;
      for (j=0;j<len;j++)
      {
         float jj = j-len/2+.5;
         cos_table[i][j] = cos(w[i]*jj)*window[j];
         sin_table[i][j] = sin(w[i]*jj)*window[j];
         tcos_table[i][j] = ((jj))*cos_table[i][j];
         tsin_table[i][j] = ((jj))*sin_table[i][j];
         /* The sinusoidal terms */
         tmp1 += cos_table[i][j]*cos_table[i][j];
         tmp2 += sin_table[i][j]*sin_table[i][j];
         /* The modulation terms */
         tmp3 += tcos_table[i][j]*tcos_table[i][j];
         tmp4 += tsin_table[i][j]*tsin_table[i][j];
      }
      cosE[i] = sqrt(tmp1);
      sinE[i] = sqrt(tmp2);
      costE[i] = sqrt(tmp3);
      sintE[i] = sqrt(tmp4);
      for (j=0;j<len;j++)
      {
         cos_table[i][j] /= cosE[i];
         sin_table[i][j] /= sinE[i];
         tcos_table[i][j] /= costE[i];
         tsin_table[i][j] /= sintE[i];
      }
   }
   /* y is the initial approximation of the signal */
   for (j=0;j<len;j++)
      y[j] = 0;
   for (i=0;i<N;i++)
      ai[i] = bi[i] = ci[i] = di[i] = 0;
   int tata=0;
   /* This is an iterative solution -- much quicker than inverting a matrix */
   for (iter=0;iter<5;iter++)
   {
      for (i=0;i<N;i++)
      {
         float tmp1=0, tmp2=0;
         float tmp3=0, tmp4=0;
         /* (Sort of) project the residual on the four basis functions */
         for (j=0;j<len;j++)
         {
            tmp1 += (x[j]-y[j])*cos_table[i][j];
            tmp2 += (x[j]-y[j])*sin_table[i][j];
            tmp3 += (x[j]-y[j])*tcos_table[i][j];
            tmp4 += (x[j]-y[j])*tsin_table[i][j];
         }
         
         //tmp3=tmp4 = 0;

         /* Update the signal approximation for the next iteration */
         for (j=0;j<len;j++)
         {
            y[j] += tmp1*cos_table[i][j] + tmp2*sin_table[i][j] + tmp3*tcos_table[i][j] + tmp4*tsin_table[i][j];
         }
         ai[i] += tmp1;
         bi[i] += tmp2;
         ci[i] += tmp3;
         di[i] += tmp4;
         if (iter==4)
         {
            if (w[i] > .49 && w[i] < .53 && !tata)
            {
               //printf ("%f %f %f %f %f\n", w[i], ai[i], bi[i], ci[i], di[i]);
               tata = 1;
            }
            //printf ("%f %f ", w[i], (float)sqrt(ai[i]*ai[i] + bi[i]*bi[i]));
         }
      }
   }
   for (i=0;i<N;i++)
   {
      ai[i] /= cosE[i];
      bi[i] /= sinE[i];
      ci[i] /= costE[i];
      di[i] /= sintE[i];
   }
#if 0
   if (N)
   for (i=0;i<1;i++)
   {
      float A, phi, dA, dw;
      A = sqrt(ai[i]*ai[i] + bi[i]*bi[i]);
      phi = atan2(bi[i], ai[i]);
      //phi = ai[i]*ai[i] + bi[i]*bi[i];
      dA = (ci[i]*ai[i] + bi[i]*di[i])/(.1+A);
      dw = (ci[i]*bi[i] - di[i]*ai[i])/(.1+A*A);
      printf ("%f %f %f %f %f %f %f %f %f\n", w[i], ai[i], bi[i], ci[i], di[i], A, phi, dA, dw);
   }
#endif
   //if(!tata)
      //printf ("0 0 0 0 0\n");

   //printf ("\n");
}
