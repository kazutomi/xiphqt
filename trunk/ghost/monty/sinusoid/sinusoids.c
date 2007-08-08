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

static void hanning(float *d, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    d[i] = .5-.5*cos(scale*i5);
  }
}

static void compute_sinusoid(float A, float W, float P, 
			     float delA, float delW, 
			     float *y, int len){
  int i,j;
  float ilen = 1./len;

  for(j=0;j<len;j++){
    float jj = (j-len*.5+.5)*ilen;
    float a = A + delA*jj;
    float w = W + delW*jj;
    y[j] += a*cos(2.*M_PI*jj*w+P);
  }
}

int extract_sinusoids_iteration(float *x, float *window, 
				float *A, float *W, float *P, 
				float *dA, float *dW, 
				float *y, int N, int len){

  float scale = 2.*M_PI/len ;
  float iscale = len/(2.*M_PI);
  int i,j,flag=0;
  float cwork[len];
  float swork[len];

  for(i=0;i<N;i++){
    
    float aP=0, bP=0, cP=0;
    float dP=0, eP=0, fP=0;
    float aE=0, bE=0, cE=0;
    float dE=0, eE=0, fE=0;
    float ldW = (dW?dW[i]:0.f);
    
    if(A[i]!=0. || dA[i]!=0.)
      compute_sinusoid(-A[i],W[i],P[i],-dA[i],dW[i],y,len);

    for(j=0;j<len;j++){
      float jj = j-len/2.+.5;
      float jj2 = jj*jj;
      float jj4 = jj2*jj2;
      float c = cos((W[i]*scale + ldW*scale*jj)*jj);
      float s = sin((W[i]*scale + ldW*scale*jj)*jj);
      float cw = c*window[j];
      float sw = s*window[j];
      float ccw = cw*c;
      float ssw = sw*s;

      aE += ccw;
      bE += ssw;
      cE += ccw*jj2;
      dE += ssw*jj2;
      eE += ccw*jj4;
      fE += ssw*jj4;
      
      aP += (x[j]-y[j]) * cw;
      bP += (x[j]-y[j]) * sw;

      cwork[j] = c;
      swork[j] = s;
    }

    aP = (aE>0.f ? aP/aE : 0.f);
    bP = (bE>0.f ? bP/bE : 0.f);

    for(j=0;j<len;j++){
      float jj = j-len/2.+.5;

      cP += (x[j]-y[j]-aP*cwork[j]) * cwork[j] * window[j] * jj;
      dP += (x[j]-y[j]-bP*cwork[j]) * swork[j] * window[j] * jj;      
    }

    cP = (cE>0.f ? cP/cE : 0.f);
    dP = (dE>0.f ? dP/dE : 0.f);

    for(j=0;j<len;j++){
      float jj = j-len/2.+.5;
      float jj2 = jj*jj;

      eP += (x[j]-y[j]-(aP+cP*jj)*cwork[j]) * cwork[j]*window[j] * jj2;
      fP += (x[j]-y[j]-(bP+dP*jj)*swork[j]) * swork[j]*window[j] * jj2;

    }
    
    eP = (eE>0.f ? eP/eE : 0.f);
    fP = (fE>0.f ? fP/fE : 0.f);
    
    {
      float lA2 = aP*aP + bP*bP;
      float lA = sqrt(aP*aP + bP*bP);
      float lP;
      float lW;
      float ldW;
      
      if(lA!=0.){
	if(bP>0){
	  lP = -acos(aP/lA);
	}else{
	  lP = acos(aP/lA);
	}
      }else
	lP=0.;

      lW  = (cP*bP - dP*aP)/lA2*iscale;
      ldW = (eP*bP - fP*aP)/lA2*iscale;

      if( fabs(lW)>.001 || fabs(ldW)>.001) flag=1;
    
      A[i] = lA;
      P[i]  = lP;
      dA[i] = (cP*aP + bP*dP)/lA;
      W[i]  += lW;
      dW[i] += ldW;
    }

    compute_sinusoid(A[i],W[i],P[i],dA[i],dW[i],y,len);

  }

  return flag;
}

void extract_modulated_sinusoids_nonlinear(float *x, float *w,
				 float *A, float *W, float *P, 
				 float *dA, float *dW, 
				 float *y, int N, int len){
  float window[len];
  hanning(window,len);
  memset(y,0,sizeof(*y)*len);
  memset(A,0,sizeof(*A)*N);
  memset(P,0,sizeof(*A)*N);
  memset(dA,0,sizeof(*A)*N);
  memset(dW,0,sizeof(*A)*N);
  int count=5;

  while(extract_sinusoids_iteration(x, window, A, W, P, dA, dW, y, N, len) && --count);
    
}

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

      float c = cos((2.*M_PI*w[i]/len + 2.*M_PI*dWout[i]/len*jj)*jj);
      float s = sin((2.*M_PI*w[i]/len + 2.*M_PI*dWout[i]/len*jj)*jj);

      cos_table[i][j] = c;
      sin_table[i][j] = s;
      tcos_table[i][j] = jj*c;
      tsin_table[i][j] = jj*s;
      ttcos_table[i][j] = jj*jj*c;
      ttsin_table[i][j] = jj*jj*s;

      /* The sinusoidal terms */
      tmpa += cos_table[i][j]*cos_table[i][j]*window[j];
      tmpb += sin_table[i][j]*sin_table[i][j]*window[j];

      /* The modulation terms */
      tmpc += tcos_table[i][j]*tcos_table[i][j]*window[j];
      tmpd += tsin_table[i][j]*tsin_table[i][j]*window[j];

      /* The second order modulations */
      tmpe += ttcos_table[i][j]*ttcos_table[i][j]*window[j];
      tmpf += ttsin_table[i][j]*ttsin_table[i][j]*window[j];

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
	if(iter>0){
	  tmpe += (x[j]-y[j]) * ttcos_table[i][j] * window[j];
	  tmpf += (x[j]-y[j]) * ttsin_table[i][j] * window[j];
	}
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
    dWout[i] += ((ei[i]*bi[i] - fi[i]*ai[i])/(A*A)/(2.*M_PI))*len;
  }
  
}

