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

 function: research-grade sinusoidal extraction code
 last mod: $Id$

 ********************************************************************/

#include <math.h>
#include "sinusoids.h"
#include "scales.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void window_weight(float *logf, float *out, int n, float flatbias,
		   int lowindow, int hiwindow, int min, int rate){
  int bark[n],i;
  float binwidth = rate*.5f/n; 
  float ibinwidth = 1.f/binwidth;

  for(i=0;i<n;i++)
    bark[i] = rint(toBark(i*binwidth) * 1024);

  float gacc = 0.f;
  float gadd = 0.f;
  float aacc = 0.f;
  float aadd = 0.f;

  float dacc = 0.f;
  float dadd = 0.f;
  
  int hihead=0;
  int hitail=0;
  int lohead=0;
  int lotail=0;

  for(i=0;i<n;i++){

    for( ; hihead<n && (bark[hihead]<=bark[i]+hiwindow || hihead<i+min);hihead++){
      int c = hihead-i+1;
      float d = (c<min?1./min:1./c);

      gadd += logf[hihead]*d;
      aadd += fromdB(logf[hihead])*d;
      dadd += d;

      while(c<min){
	gacc += logf[hihead]*d;
	aacc += fromdB(logf[hihead])*d;
	dacc += d;
	c++;
      }
    }

    aacc += aadd;
    gacc += gadd;
    dacc += dadd;

    if(lohead<n){
      while(lohead<n && (bark[lohead]<=bark[i]+lowindow || lohead<i+min))
	lohead++;
    }else
      lohead++;

    {
      int c = lohead-i;
      float d = 1./c;
      gadd -= logf[i]*d;
      aadd -= fromdB(logf[i])*d;
      dadd -= d;
    }

    for( ;lotail<i && bark[lotail]<bark[i]-lowindow && lotail<=i-min; lotail++){
      int c = i-lotail;
      float d = 1./c;
      gadd += logf[lotail]*d;
      aadd += fromdB(logf[lotail])*d;
      dadd += d;
    }
    
    while( hitail<i && bark[hitail]<bark[i]-hiwindow && hitail<=i-min)
      hitail++;

    {
      int c = i-hitail+1;
      float d = (c<min?1./min:1./c);
      gadd -= logf[i]*d;
      aadd -= fromdB(logf[i])*d;
      dadd -= d;
    }

    { 
      float arith = todB(aacc / dacc);
      float geom = gacc / dacc;
      out[i] = logf[i] - arith + flatbias*(arith-geom);
    }

  }

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
void extract_modulated_sinusoids(float *x, float *w, float *ai, float *bi, float *ci, float *di, float *y, int N, int len)
{
  float *cos_table[N];
  float *sin_table[N];
  float *tcos_table[N];
  float *tsin_table[N];
  float cosE[N], sinE[N];
  float costE[N], sintE[N];
   int i,j, iter;

  for(i=0;i<N;i++){
    cos_table[i] = malloc(sizeof(**cos_table)*len);
    sin_table[i] = malloc(sizeof(**sin_table)*len);
    tcos_table[i] = malloc(sizeof(**tcos_table)*len);
    tsin_table[i] = malloc(sizeof(**tsin_table)*len);
  }

   /* Build a table for the four basis functions at each frequency: cos(x), sin(x), x*cos(x), x*sin(x)*/
   for (i=0;i<N;i++)
   {
      float tmp1=0, tmp2=0;
      float tmp3=0, tmp4=0;
      for (j=0;j<len;j++)
      {
         float jj = j-len/2.+.5;
         cos_table[i][j] = cos(w[i]*jj);
         sin_table[i][j] = sin(w[i]*jj);
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

  for(i=0;i<N;i++){
    free(cos_table[i]);
    free(sin_table[i]);
    free(tcos_table[i]);
    free(tsin_table[i]);
  }


}

void compute_sinusoids(float *Aout, float *Wout, float *Pout, 
		       float *delAout, float *delWout, float *y, int N, int len){
  int i,j;
  double ilen = 1./len;

  for(j=0;j<len;j++){
    double jj = (j-len*.5+.5)*ilen;
    y[j]=0.;
    for(i=0;i<N;i++){
      double A = Aout[i] + delAout[i]*jj;
      double W = Wout[i] + delWout[i]*jj;
      y[j] += A*cos(M_2_PI*jj*W+Pout[i]);
    }
  }
}

// brute-force, simultaneous nonlinear system CG, meant to be a 'high anchor' of possible numeric performance 
void extract_modulated_sinusoids2(float *x, float *w, float *Aout, float *Wout, float *Pout, 
				 float *delAout, float *delWout, float *y, int N, int len){
  int i,j;
  double ilen = 1./len;

  /* w contains our frequency seeds; initialize Wout, Pout, Aout from this data */
  /* delAout and delPout start at zero */
  for(i=0;i<N;i++){
    double re = 0.;
    double im = 0.;

    for(j=0;j<len;j++){
      double jj = M_2_PI*(j-len*.5+.5)*ilen*w[i];
      double s = sin(jj);
      double c = cos(jj);
      im += s*x[j];
      re += c*x[j];
    }

    double ph = 0;
    double m = hypot(re,im);
    if(m!=0.){
      if(im>0){
        ph = acos(re/m)/M_PI;
      }else{
        ph = -acos(re/m)/M_PI;
      }
    }

    Aout[i] = 2*m*ilen;
    Pout[i] = ph;
    Wout[i] = w[i];
    delAout[i] = 0.f;
    delWout[i] = 0.f;
  }

  compute_sinusoids(Aout,Wout,Pout,delAout,delWout,y,N,len);

}
