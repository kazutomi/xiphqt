#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sinusoids.h"
#include "smallft.h"
#include "scales.h"

#define BLOCK_SIZE 1024

static float float_in[BLOCK_SIZE*2];
static float float_out[262144];
static short short_in[BLOCK_SIZE];
static int outcount=0;

static void dump_vec(float *data, int n, char *base, int i){
  char *filename;
  FILE *f;

  asprintf(&filename,"%s_%d.m",base,i);
  f=fopen(filename,"w");
  
  for(i=0;i<n;i++)
    fprintf(f,"%f %f\n",
	    (float)i/n,data[i]);

  fclose(f);

}

static void dump_vec2(float *x, float *y, int n, char *base, int i){
  char *filename;
  FILE *f;

  asprintf(&filename,"%s_%d.m",base,i);
  f=fopen(filename,"w");
  
  for(i=0;i<n;i++){
    fprintf(f,"%f -120\n",
	    x[i]);
    fprintf(f,"%f %f\n\n",
	    x[i],y[i]);
  }

  fclose(f);

}

#define A0 .35875f
#define A1 .48829f
#define A2 .14128f
#define A3 .01168f

void blackmann_harris(float *out, float *in, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    float w = A0 - A1*cos(scale*i5) + A2*cos(scale*i5*2) - A3*cos(scale*i5*3);
    out[i] = in[i]*w;
  }
}

static void hanning(float *out, float *in, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    out[i] = in[i]*(.5-.5*cos(scale*i5));
  }
}

static void hanningW(float *out, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    out[i] = (.5-.5*cos(scale*i5));
  }
}


void mag_dB(float *log,float *d, int n){
  int i;
  log[0] = todB(d[0]*d[0])*.5;
  for(i=2;i<n;i+=2)
    log[i>>1] = todB(d[i-1]*d[i-1] + d[i]*d[i])*.5;
  log[n/2] = todB(d[n-1]*d[n-1])*.5;
}

int main(int argc, char **argv){
  FILE *fin, *fout;
  int i,frame=0;
  drft_lookup fft;

  drft_init(&fft, BLOCK_SIZE*2);

  if (argc != 3){
    fprintf (stderr, "usage: testghost input_file output_file\nWhere the input and output are raw mono files sampled at 44.1 kHz or 48 kHz\n");
    exit(1);
  }
  
  fin = fopen(argv[1], "r");
  fout = fopen(argv[2], "w");

  /*
    for(i=0;i<BLOCK_SIZE*2;i++)
    float_in[i]=1.;
    blackmann_harris(float_in, float_in, BLOCK_SIZE*2);
    dump_vec(float_in,BLOCK_SIZE*2,"window",0);
    memset(float_in,0,sizeof(float_in));
  */

  while (1){
    int i,j;
    memmove(float_in,float_in+BLOCK_SIZE,sizeof(*float_in)*BLOCK_SIZE);
    fread(short_in, sizeof(short), BLOCK_SIZE, fin);
      
    if (feof(fin))
      break;
    for (i=0;i<BLOCK_SIZE;i++)
      float_in[i+BLOCK_SIZE] = short_in[i] * .000030517578125;

    {

      /* generate a log spectrum */
      float fft_buf[BLOCK_SIZE*2];
      float log_fft[BLOCK_SIZE+1];
      float weight[BLOCK_SIZE+1];

      // polish the strongest peaks from weighting
      float w[BLOCK_SIZE];
      float Wout[BLOCK_SIZE];
      float Aout[BLOCK_SIZE]={0};
      float Pout[BLOCK_SIZE]={0};
      float dAout[BLOCK_SIZE]={0};
      float ddAout[BLOCK_SIZE]={0};
      float dWout[BLOCK_SIZE]={0};
      float y[BLOCK_SIZE*2];
      float window[BLOCK_SIZE*2];
      
      hanning(fft_buf, float_in, BLOCK_SIZE*2);
      //dump_vec(float_in,BLOCK_SIZE*2,"data",frame);
      drft_forward(&fft, fft_buf);
      for(i=0;i<BLOCK_SIZE*2;i++)fft_buf[i] *= 1./BLOCK_SIZE;
      
      mag_dB(log_fft,fft_buf,BLOCK_SIZE*2);
      //dump_vec(log_fft,BLOCK_SIZE+1,"logmag",frame);
      
      window_weight(log_fft,weight,BLOCK_SIZE+1, 0.f, 512,256, 30, 44100);
      //dump_vec(weight,BLOCK_SIZE+1,"weight",frame);

      //j=2;
      //	w[0]=.0044*BLOCK_SIZE;
      //	w[1]=.136*BLOCK_SIZE;

      int j,k;
      for(j=0;j<20;j++){
	/* largest weighted */
	int best=-120;
	int besti=-1;
	for(i=0;i<BLOCK_SIZE+1;i++){
	  float v = log_fft[i] - weight[i];
	  if(v>best){
	    for(k=0;k<j;k++)
	      if(i == w[j])break;
	    if(k==j){
	      besti = i;
	      best = v;
	    }
	  }
	}
	w[j] = besti;
      }

      //blackmann_harris(float_in, float_in, BLOCK_SIZE*2);
      /*hanningW(window,BLOCK_SIZE*2);
	extract_modulated_sinusoids_nonlinear(float_in, window, Aout, w, Pout, dAout, dWout, y, j, BLOCK_SIZE*2);
	extract_modulated_sinusoids_nonlinear(float_in, window, Aout, w, Pout, dAout, dWout, y, j, BLOCK_SIZE*2);
	extract_modulated_sinusoids_nonlinear(float_in, window, Aout, w, Pout, dAout, dWout, y, j, BLOCK_SIZE*2);
	extract_modulated_sinusoids_nonlinear(float_in, window, Aout, w, Pout, dAout, dWout, y, j, BLOCK_SIZE*2);
	extract_modulated_sinusoids_nonlinear(float_in, window, Aout, w, Pout, dAout, dWout, y, j, BLOCK_SIZE*2);*/
      
	extract_modulated_sinusoidsB(float_in, w, Aout, Wout, Pout, dAout, dWout, ddAout, y, j, BLOCK_SIZE*2);
	for(i=0;i<j;i++)
	  w[i]=Wout[i];
	extract_modulated_sinusoidsB(float_in, w, Aout, Wout, Pout, dAout, dWout, ddAout, y, j, BLOCK_SIZE*2);
	for(i=0;i<j;i++)
	  w[i]=Wout[i];
	extract_modulated_sinusoidsB(float_in, w, Aout, Wout, Pout, dAout, dWout, ddAout, y, j, BLOCK_SIZE*2);
	for(i=0;i<j;i++)
	  w[i]=Wout[i];
	extract_modulated_sinusoidsB(float_in, w, Aout, Wout, Pout, dAout, dWout, ddAout, y, j, BLOCK_SIZE*2);


	for(i=0;i<j;i++)
	  fprintf(stdout, "%d %f\n\n",frame,Wout[i]/BLOCK_SIZE*22050);

	
	/*	for(i=0;i<BLOCK_SIZE*2;i++)
	  fft_buf[i] = float_in[i]-y[i];
	
	dump_vec(fft_buf,BLOCK_SIZE*2,"res",0);

	hanning(fft_buf, fft_buf, BLOCK_SIZE*2);
	drft_forward(&fft, fft_buf);
	for(i=0;i<BLOCK_SIZE*2;i++)fft_buf[i] *= 1./BLOCK_SIZE;
	mag_dB(log_fft,fft_buf,BLOCK_SIZE*2);
	
	dump_vec(log_fft,BLOCK_SIZE+1,"exlogmag",0);
	
	for(i=0;i<j;i++){
	  Aout[i]=todB(Aout[i]);
	  dAout[i]=todB(dAout[i]);
	  w[i] /= BLOCK_SIZE;
	}
	  
	dump_vec2(w,Aout,j,"ex",0);
	dump_vec2(w,dAout,j,"dA",0);
	dump_vec2(w,dWout,j,"dW",0);
	dump_vec(y,BLOCK_SIZE*2,"extract",0);
	*/
	//for (i=0;i<BLOCK_SIZE*2;i++)
	//  float_out[i+outcount] += fft_buf[i];
	//outcount+=BLOCK_SIZE;

	//if(outcount+BLOCK_SIZE*2>262144){

	//	  hanning(float_out, float_out, 262144);
	//drft_init(&fft, 262144);
	//drft_forward(&fft, float_out);
	//for(i=0;i<262144;i++)float_out[i] *= 1./262144;

	//mag_dB(float_out,float_out,262144);
	//dump_vec(float_out,262144/2,"res",0);
	//exit(0);

	//}
	//}
      
    }
  
    
    
    //fwrite(short_in, sizeof(short), BLOCK_SIZE, fout);
    frame++;
  }
  
  return 0;
}

