#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "smallft.h"

#define todB(x)   ((x)==0?-400.f:log((x)*(x))*4.34294480f)
#define todB_nn(x)   ((x)==0.f?-400.f:log((x))*8.6858896f)

static void _analysis(char *base,int seq, float *data, int n,int dB, 
                      off_t offset){

  FILE *f;
  char buf[80];
  
  sprintf(buf,"%s_%d.m",base,seq);

  f=fopen(buf,"w");
  if(f){
    int i;
    for(i=0;i<n;i++)
      if(dB)
        fprintf(f,"%d %f\n",(int)(i+offset),todB(data[i]));
      else
        fprintf(f,"%d %f\n",(int)(i+offset),(data[i]));

  }
  fclose(f);
}

void compute_bh4(float *d, int n){
  int i; 

  float a0 = .35875f;
  float a1 = .48829f;
  float a2 = .14128f;
  float a3 = .01168f;

  for(i=0;i<n;i++){
    float phi =2.*M_PI/n*(i+.5);
    d[i] = sin((i+.5)/n * M_PI)*sin((i+.5)/n * M_PI);
  }
}

void compute_vorbis(float *d, int n){
  int i; 
  for(i=0;i<n;i++)
    d[i] = sin(0.5 * M_PI * sin((i+.5)/n * M_PI)*sin((i+.5)/n * M_PI));
}

float beta(int n, float alpha){
  return cosh (acosh(pow(10,alpha))/(n-1));
}

double T(double n, double x){
  if(fabs(x)<=1){
    return cos(n*acos(x));
  }else{
    return cosh(n*acosh(x));
  }
}

void compute_dolphcheb(float *d, int n){
  int i,k; 
  float a = 6.;
  int M=n/2;
  int N=M*2;
  double b = beta(N,a);

  for(i=0;i<n;i++){
    double sum=0;
    for(k=0;k<M;k++)
      sum += (k&1?-1:1)*T(N,b*cos(M_PI*k/N)) * cos (2*i*k*M_PI/N);
    
    sum /= T(N,b);
    
    sum-=.5;
    d[i]=sum;
  }
}


void compute_hanning(float *x, int n){
  float scale = 2*M_PI/n;
  int i;
  for(i=0;i<n;i++){
    float i5 = i+.5;
    x[i] = .5-.5*cos(scale*i5);
  }
}

void compute_inverse(float *d, float *inv, int n){
  int i;

  float temp[n];
  compute_vorbis(temp,n);

  for(i=0;i<n/2;i++)
    inv[i] = d[i]*temp[i] + d[n/2-i-1]*temp[n/2-i-1];

  for(;i<n;i++)
    inv[i] = d[i]*temp[i] + d[n-(i-n/2)-1]*temp[n-(i-n/2)-1];

 
  for(i=0;i<n;i++)
    inv[i] = temp[i] /inv[i];


}

void multiply(float *acc, float *x, int n){
  int i;
  for(i=0;i<n;i++)
    acc[i]*=x[i];
}

void lap(float *d, float *dest, int n){
  int i;

  for(i=0;i<n/2;i++)
    dest[i] = d[i] + d[n/2-i-1];

  for(;i<n;i++)
    dest[i] = d[i] + d[n-(i-n/2)-1];


}

void fftmag(float *in, float *mag, int n){
  int i;
  drft_lookup fft;
  drft_init(&fft,n);
  memcpy(mag,in,n*sizeof(*mag));
  drft_forward(&fft,mag);
  for(i=1;i+1<n;i+=2)
    mag[(i+1)>>1] = hypot(mag[i],mag[i+1]);
}

#define width 256
main(){
  int i;
  float win[width];
  float inverse[width];
  float measure[width];

  compute_bh4(win,width);
  compute_inverse(win,inverse,width);

  _analysis("bh4",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("bh4_H",0,measure,width/2,1,0);

  _analysis("bh4i",0,inverse,width,0,0);
  fftmag(inverse,measure,width);
  _analysis("bh4i_H",0,measure,width/2,1,0);

  multiply(win,inverse,width);
  _analysis("bh4sq",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("bh4sq_H",0,measure,width/2,1,0);

  lap(win,inverse,width);
  _analysis("bh4lap",0,inverse,width,0,0);
  fftmag(inverse,measure,width);
  _analysis("bh4lap_H",0,measure,width/2,1,0);


  compute_vorbis(win,width);
  compute_inverse(win,inverse,width);

  _analysis("v",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("v_H",0,measure,width/2,1,0);

  _analysis("vi",0,inverse,width,0,0);
  fftmag(inverse,measure,width);
  _analysis("vi_H",0,measure,width/2,1,0);

  multiply(win,inverse,width);
  _analysis("vsq",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("vsq_H",0,measure,width/2,1,0);


  compute_dolphcheb(win,width);
  compute_inverse(win,inverse,width);

  _analysis("d",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("d_H",0,measure,width/2,1,0);

  _analysis("di",0,inverse,width,0,0);
  fftmag(inverse,measure,width);
  _analysis("di_H",0,measure,width/2,1,0);

  multiply(win,inverse,width);
  _analysis("dsq",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("dsq_H",0,measure,width/2,1,0);


  compute_hanning(win,width);
  compute_inverse(win,inverse,width);

  _analysis("h",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("h_H",0,measure,width/2,1,0);

  _analysis("hi",0,inverse,width,0,0);
  fftmag(inverse,measure,width);
  _analysis("hi_H",0,measure,width/2,1,0);

  multiply(win,inverse,width);
  _analysis("hsq",0,win,width,0,0);
  fftmag(win,measure,width);
  _analysis("hsq_H",0,measure,width/2,1,0);

}
