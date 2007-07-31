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
      out[i] = arith - flatbias*(arith-geom);
    }

  }

}

/* Models the signal as modulated sinusoids 
 * x is the signal
 * w are the frequencies
 * ai are the cos(x) coefficients
 * bi are the sin(x) coefficients
 * ci are the x*cos(x) coefficients
 * di are the x*sin(x) coefficients
 * y is the approximated signal by summing all the params
 * N is the number of sinusoids
 * len is the frame size
*/
void extract_modulated_sinusoids(float *x, float *w, 
				 float *Aout, float *Wout, float *Pout, float *delAout, float *delWout, float *ddAout,
				 float *y, int N, int len){
  float *cos_table[N];
  float *sin_table[N];
  float *tcos_table[N];
  float *tsin_table[N];
  float cosE[N], sinE[N];
  float costE[N], sintE[N];
  float ai[N];
  float bi[N];
  float ci[N];
  float di[N];
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
         cos_table[i][j] = cos((2.*M_PI*w[i]/len)*jj);
         sin_table[i][j] = sin((2.*M_PI*w[i]/len)*jj);
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
  
  for(i=0;i<N;i++){
    float A = hypot(ai[i],bi[i]);
    float P;
    if(A!=0.){
      if(bi[i]>0){
        P = -acos(ai[i]/A);
      }else{
        P = acos(ai[i]/A);
      }
    }else
      P=0.;
    
    Aout[i] = A;
    Wout[i] = w[i] + ((ci[i]*bi[i] - di[i]*ai[i])/(A*A)/(2.*M_PI))*len;
    Pout[i] = P;
    delAout[i] = (ci[i]*ai[i] + bi[i]*di[i])/A;
    delWout[i] = 0.;
    
  }
  
}

#define A0 .35875f
#define A1 .48829f
#define A2 .14128f
#define A3 .01168f

static void blackmann_harris(float *d, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    d[i] = A0 - A1*cos(scale*i5) + A2*cos(scale*i5*2) - A3*cos(scale*i5*3);
  }
}

static void hanning(float *d, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    d[i] = .5-.5*cos(scale*i5);
  }
}

/* Models the signal as modulated sinusoids 
 * x is the signal
 * w are the frequencies
 * ai are the cos(x) coefficients
 * bi are the sin(x) coefficients
 * ci are the x*cos(x) coefficients
 * di are the x*sin(x) coefficients
 * y is the approximated signal by summing all the params
 * N is the number of sinusoids
 * len is the frame size
*/
void extract_modulated_sinusoidsB(float *x, float *w, 
				  float *Aout, float *Wout, float *Pout, 
				  float *dAout, float *dWout, float *ddAout, 
				  float *y, int N, int len){
  float window[len];
  float *cos_table[N];
  float *sin_table[N];
  float *tcos_table[N];
  float *tsin_table[N];
  float *ttcos_table[N];
  float *ttsin_table[N];
  float cosE[N], sinE[N];
  float tcosE[N], tsinE[N];
  float ttcosE[N], ttsinE[N];
  float ai[N];
  float bi[N];
  float ci[N];
  float di[N];
  float ei[N];
  float fi[N];
  int i,j, iter;

  hanning(window,len);
  //  for (j=0;j<len;j++){
  //window[j] *= window[j];
  //window[j] *= window[j];
  //}  
  /* Build a table for the four basis functions at each frequency */
  for (i=0;i<N;i++){
    float tmpa=0;
    float tmpb=0;
    float tmpc=0;
    float tmpd=0;
    float tmpe=0;
    float tmpf=0;

    cos_table[i]=calloc(len,sizeof(**cos_table));
    sin_table[i]=calloc(len,sizeof(**sin_table));
    tcos_table[i]=calloc(len,sizeof(**tcos_table));
    tsin_table[i]=calloc(len,sizeof(**tsin_table));
    ttcos_table[i]=calloc(len,sizeof(**ttcos_table));
    ttsin_table[i]=calloc(len,sizeof(**ttsin_table));

    for (j=0;j<len;j++){
      float jj = j-len/2.+.5;

      float c = cos((2.*M_PI*w[i]/len)*jj);
      float s = sin((2.*M_PI*w[i]/len)*jj);

      cos_table[i][j] = c;
      sin_table[i][j] = s;
      tcos_table[i][j] = jj*c;
      tsin_table[i][j] = jj*s;
      ttcos_table[i][j] = jj*jj*c;
      ttsin_table[i][j] = jj*jj*s;

      /* The sinusoidal terms */
      tmpa += cos_table[i][j]*cos_table[i][j];
      tmpb += sin_table[i][j]*sin_table[i][j];

      /* The modulation terms */
      tmpc += tcos_table[i][j]*tcos_table[i][j];
      tmpd += tsin_table[i][j]*tsin_table[i][j];

      /* The second order modulations */
      tmpe += ttcos_table[i][j]*ttcos_table[i][j];
      tmpf += ttsin_table[i][j]*ttsin_table[i][j];

    }

    tmpa = sqrt(tmpa);
    tmpb = sqrt(tmpb);
    tmpc = sqrt(tmpc);
    tmpd = sqrt(tmpd);
    tmpe = sqrt(tmpe);
    tmpf = sqrt(tmpf);

    cosE[i] = (tmpa>0.f ? 1.f/tmpa : 0.f);
    sinE[i] = (tmpb>0.f ? 1.f/tmpb : 0.f);
    tcosE[i] = (tmpc>0.f ? 1.f/tmpc : 0.f);
    tsinE[i] = (tmpd>0.f ? 1.f/tmpd : 0.f);
    ttcosE[i] = (tmpe>0.f ? 1.f/tmpe : 0.f);
    ttsinE[i] = (tmpf>0.f ? 1.f/tmpf : 0.f);

    for(j=0;j<len;j++){
      cos_table[i][j] *= cosE[i];
      sin_table[i][j] *= sinE[i];
      tcos_table[i][j] *= tcosE[i];
      tsin_table[i][j] *= tsinE[i];
      ttcos_table[i][j] *= ttcosE[i];
      ttsin_table[i][j] *= ttsinE[i];
    }
  }
  
  /* y is the initial approximation of the signal */
  for (j=0;j<len;j++)
    y[j] = 0;
  for (i=0;i<N;i++)
    ai[i] = bi[i] = ci[i] = di[i] = ei[i] = fi[i] = 0;
  
  for (iter=0;iter<5;iter++){
    for (i=0;i<N;i++){
      
      float tmpa=0, tmpb=0, tmpc=0;
      float tmpd=0, tmpe=0, tmpf=0;
      
      /* (Sort of) project the residual on the four basis functions */
      for (j=0;j<len;j++){
	float jj = j-len/2.+.5;
	
	tmpa += (x[j]-y[j]) * cos_table[i][j] * window[j];
	tmpb += (x[j]-y[j]) * sin_table[i][j] * window[j];
	tmpc += (x[j]-y[j]) * tcos_table[i][j] * window[j];
	tmpd += (x[j]-y[j]) * tsin_table[i][j] * window[j];
	//tmpe += (x[j]-y[j]) * ttcos_table[i][j];
	//tmpf += (x[j]-y[j]) * ttsin_table[i][j];
	
      }
      
      /* Update the signal approximation for the next iteration */
      for (j=0;j<len;j++){
	float jj = j-len/2.+.5;
	
	y[j] += tmpa*cos_table[i][j];
	y[j] += tmpb*sin_table[i][j];
	y[j] += tmpc*tcos_table[i][j];
	y[j] += tmpd*tsin_table[i][j];
	y[j] += tmpe*ttcos_table[i][j];
	y[j] += tmpf*ttsin_table[i][j];
	
      }
      
      ai[i] += tmpa;
      bi[i] += tmpb;
      ci[i] += tmpc;
      di[i] += tmpd;
      ei[i] += tmpe;
      fi[i] += tmpf;
      
    }
  }
  
  for (i=0;i<N;i++){
    ai[i] *= cosE[i];
    bi[i] *= sinE[i];
    ci[i] *= tcosE[i];
    di[i] *= tsinE[i];
    ei[i] *= ttcosE[i];
    fi[i] *= ttsinE[i];
  }
  
  for(i=0;i<N;i++){
    float A = hypot(ai[i],bi[i]);
    float P;
    
    free(cos_table[i]);
    free(sin_table[i]);
    free(tcos_table[i]);
    free(tsin_table[i]);
    free(ttcos_table[i]);
    free(ttsin_table[i]);
     
    if(A!=0.){
      if(bi[i]>0){
	P = -acos(ai[i]/A);
      }else{
        P = acos(ai[i]/A);
      }
    }else
      P=0.;
    
    Aout[i] = A;
    Pout[i] = P;
    dAout[i] = (ci[i]*ai[i] + bi[i]*di[i])/A;
    Wout[i] = w[i] + ((ci[i]*bi[i] - di[i]*ai[i])/(A*A)/(2.*M_PI))*len;
    ddAout[i] = (ei[i]*ai[i] + bi[i]*fi[i])/A;
    dWout[i] = ((ei[i]*bi[i] - fi[i]*ai[i])/(A*A)/(2.*M_PI))*len;
  }
  
}

static void compute_sinusoids(float *A, float *W, float *P, 
			      float *delA, float *delW, float *y, int N, int len){
  int i,j;
  float ilen = 1./len;

  for(j=0;j<len;j++){
    float jj = (j-len*.5+.5)*ilen;
    y[j]=0.;
    for(i=0;i<N;i++){
      float a = A[i] + delA[i]*jj;
      float w = W[i] + delW[i]*jj;
      y[j] += a*cos(2.*M_PI*jj*w+P[i]);
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

void extract_modulated_sinusoidsLSQR(float *x, float *w, 
				 float *Aout, float *Wout, float *Pout, float *dAout, float *dWout, float *ddAout,
				 float *y, int N, int len){
  float *cos_table[N];
  float *sin_table[N];
  float *tcos_table[N];
  float *tsin_table[N];
  float cosE[N], sinE[N];
  float costE[N], sintE[N];
  float ai[N];
  float bi[N];
  float ci[N];
  float di[N];
  int i,j, iter;
  
  float e[len];
  /* Symmetric and anti-symmetric components of the error */
  int L2 = len/2;
  float sym[L2], anti[L2];

  for(i=0;i<N;i++){
    cos_table[i] = malloc(sizeof(**cos_table)*len);
    sin_table[i] = malloc(sizeof(**sin_table)*len);
    tcos_table[i] = malloc(sizeof(**tcos_table)*len);
    tsin_table[i] = malloc(sizeof(**tsin_table)*len);
  }

  /* Build a table for the four basis functions at each frequency: cos(x), sin(x), x*cos(x), x*sin(x)*/
  for (i=0;i<N;i++){
    float tmp1=0, tmp2=0;
    float tmp3=0, tmp4=0;
    float rotR, rotI;
    rotR = cos(2*M_PI*w[i]/len);
    rotI = sin(2*M_PI*w[i]/len);
    /* Computing sin/cos table using complex rotations */
    cos_table[i][0] = cos(M_PI*w[i]/len);
    sin_table[i][0] = sin(M_PI*w[i]/len);
    for (j=1;j<L2;j++){
      float re, im;
      re = cos_table[i][j-1]*rotR - sin_table[i][j-1]*rotI;
      im = sin_table[i][j-1]*rotR + cos_table[i][j-1]*rotI;
      cos_table[i][j] = re;
      sin_table[i][j] = im;
    }
    /* Only need to compute the tables for half the length because of the symmetry.
       Eventually, we'll have to replace the cos/sin with rotations */
    for (j=0;j<L2;j++){
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
    /* Float the energy because we only computed one half.
       Eventually, we should be computing/tabulating these values directly
       as a function of w[i]. */
    cosE[i] = sqrt(2*tmp1);
    sinE[i] = sqrt(2*tmp2);
    costE[i] = sqrt(2*tmp3);
    sintE[i] = sqrt(2*tmp4);
    /* Normalise the basis (should multiply by the inverse instead) */
    for (j=0;j<L2;j++){
      cos_table[i][j] *= (1.f/cosE[i]);
      sin_table[i][j] *= (1.f/sinE[i]);
      tcos_table[i][j] *= (1.f/costE[i]);
      tsin_table[i][j] *= (1.f/sintE[i]);
    }
  }
  /* y is the initial approximation of the signal */
  for (j=0;j<len;j++)
    y[j] = 0;
  for (j=0;j<len;j++)
    e[j] = x[j];
  /* Split the error into a symmetric component and an anti-symmetric component. 
     This speeds everything up by a factor of 2 */
  for (j=0;j<L2;j++){
    sym[j] = e[j+L2]+e[L2-j-1];
    anti[j] = e[j+L2]-e[L2-j-1];
  }
  
  for (i=0;i<N;i++)
    ai[i] = bi[i] = ci[i] = di[i] = 0;
  
  /* y is the initial approximation of the signal */
  for (j=0;j<len;j++)
    y[j] = 0;
  for (j=0;j<len;j++)
    e[j] = x[j];
  /* Split the error into a symmetric component and an anti-symmetric component. 
     This speeds everything up by a factor of 2 */
  for (j=0;j<L2;j++){
    sym[j] = e[j+L2]+e[L2-j-1];
    anti[j] = e[j+L2]-e[L2-j-1];
  }
  
  for (i=0;i<N;i++)
    ai[i] = bi[i] = ci[i] = di[i] = 0;{
    float va[N];
    float vb[N];
    float vc[N];
    float vd[N];
    float wa[N];
    float wb[N];
    float wc[N];
    float wd[N];
    float alpha;
    float beta;
    float theta;
    float phi;
    float phibar;
    float rho;
    float rhobar;
    float c;
    float s;
    /*beta*u=e*/
    beta = phibar = 0;
    for (j=0;j<L2;j++)
      beta += sym[j]*sym[j] + anti[j]*anti[j];
    phibar = beta = sqrtf(0.5f*beta);
    if (beta > 0){
      for (j=0;j<L2;j++){
	sym[j] *= (1.f/beta);
	anti[j] *= (1.f/beta);
      }
      /*alpha*v=A^T.u*/
      alpha = 0;
      for (i=0;i<N;i++)	{
	va[i] = 0;
	for (j=0;j<L2;j++)
	  va[i] += sym[j]*cos_table[i][j];
	vb[i] = 0;
	for (j=0;j<L2;j++)
	  vb[i] += anti[j]*sin_table[i][j];
	vc[i] = 0;
	for (j=0;j<L2;j++)
	  vc[i] += anti[j]*tcos_table[i][j];
	vd[i] = 0;
	for (j=0;j<L2;j++)
	  vd[i] += sym[j]*tsin_table[i][j];
	alpha += va[i]*va[i]+vb[i]*vb[i]+vc[i]*vc[i]+vd[i]*vd[i];
      }
      if (alpha > 0){
	alpha = sqrtf(alpha);
	for (i=0;i<N;i++){
	  wa[i] = va[i] *= (1.f/alpha);
	  wb[i] = vb[i] *= (1.f/alpha);
	  wc[i] = vc[i] *= (1.f/alpha);
	  wd[i] = vd[i] *= (1.f/alpha);
	}
	phibar = beta;
	rhobar = alpha;
	for (iter=0;iter<5;iter++){
	  float p;
	  float q;
	  /*beta*u=A.v-alpha*u*/
	  beta = 0;
	  for (j=0;j<L2;j++){
	    float da;
	    float ds;
	    ds = da = 0;
	    for (i=0;i<N;i++){
	      ds += va[i]*cos_table[i][j] + vd[i]*tsin_table[i][j];
	      da += vb[i]*sin_table[i][j] + vc[i]*tcos_table[i][j];
	    }
	    sym[j] = 2*ds-alpha*sym[j];
	    anti[j] = 2*da-alpha*anti[j];
	    beta += sym[j]*sym[j] + anti[j]*anti[j];
	  }
	  if (beta <= 0) break;
	  beta = sqrtf(0.5f*beta);
	  for (j=0;j<L2;j++){
	    sym[j] *= (1.f/beta);
	    anti[j] *= (1.f/beta);
	  }
	  /*alpha*v=A^T.u-beta*v*/
	  alpha = 0;
	  for (i=0;i<N;i++){
	    float v0;
	    float v1;
	    float v2;
	    float v3;
	    v0 = 0;
	    for (j=0;j<L2;j++)
	      v0 += sym[j]*cos_table[i][j];
	    v1 = 0;
	    for (j=0;j<L2;j++)
	      v1 += anti[j]*sin_table[i][j];
	    v2 = 0;
	    for (j=0;j<L2;j++)
	      v2 += anti[j]*tcos_table[i][j];
	    v3 = 0;
	    for (j=0;j<L2;j++)
	      v3 += sym[j]*tsin_table[i][j];
	    va[i] = v0 - beta*va[i];
	    vb[i] = v1 - beta*vb[i];
	    vc[i] = v2 - beta*vc[i];
	    vd[i] = v3 - beta*vd[i];
	    alpha += va[i]*va[i]+vb[i]*vb[i]+vc[i]*vc[i]+vd[i]*vd[i];
	  }
	  if (alpha <= 0) break;
	  alpha = sqrtf(alpha);
	  for (i=0;i<N;i++){
	    va[i] *= (1.f/alpha);
	    vb[i] *= (1.f/alpha);
	    vc[i] *= (1.f/alpha);
	    vd[i] *= (1.f/alpha);
	  }
	  rho = hypotf(rhobar,beta);
	  c = rhobar/rho;
	  s = beta/rho;
	  theta = s*alpha;
	  rhobar = -c*alpha;
	  phi = c*phibar;
	  phibar = s*phibar;
	  p = phi/rho;
	  q = theta/rho;
	  for (i=0;i<N;i++){
	    ai[i] += wa[i]*p;
	    bi[i] += wb[i]*p;
	    ci[i] += wc[i]*p;
	    di[i] += wd[i]*p;
	    wa[i] = va[i] - wa[i]*q;
	    wb[i] = vb[i] - wb[i]*q;
	    wc[i] = vc[i] - wc[i]*q;
	    wd[i] = vd[i] - wd[i]*q;
	  }
	}
      }
    }
    fprintf(stderr,"LSQR sq. err. = %lg\n",phibar*phibar);
    phibar *= 0.5f;
    for (j=0;j<L2;j++){
      e[j+L2] = phibar*(sym[j]+anti[j]);
      e[L2-j-1] = phibar*(sym[j]-anti[j]);
    }
  }
  for (j=0;j<L2;j++){
    float da;
    float ds;
    ds = da = 0;
    for (i=0;i<N;i++){
      ds += ai[i]*cos_table[i][j] + di[i]*tsin_table[i][j];
      da += bi[i]*sin_table[i][j] + ci[i]*tcos_table[i][j];
    }
    y[j+L2] = ds+da;
    y[L2-j-1] = ds-da;
  }
  for (i=0;i<N;i++){
    ai[i] /= cosE[i];
    bi[i] /= sinE[i];
    ci[i] /= costE[i];
    di[i] /= sintE[i];
  }

  for(i=0;i<N;i++){
    float A = hypot(ai[i],bi[i]);
    float P;

    free(cos_table[i]);
    free(sin_table[i]);

    if(A!=0.){
      if(bi[i]>0){
        P = -acos(ai[i]/A);
      }else{
        P = acos(ai[i]/A);
      }
    }else
      P=0.;
    
    Aout[i] = A;
    Pout[i] = P;
    dAout[i] = (ci[i]*ai[i] + bi[i]*di[i])/A;
    Wout[i] = w[i] + ((ci[i]*bi[i] - di[i]*ai[i])/(A*A)/(2.*M_PI))*len;
   
  }

}
