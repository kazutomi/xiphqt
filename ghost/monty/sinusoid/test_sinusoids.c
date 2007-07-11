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
static short short_in[BLOCK_SIZE];

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

void mag_dB(float *d, int n){
  int i;
  d[0] = todB(d[0]*d[0])*.5;
  for(i=2;i<n;i+=2)
    d[i>>1] = todB(d[i-1]*d[i-1] + d[i]*d[i])*.5;
  d[n/2] = todB(d[n-1]*d[n-1])*.5;
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
    int i;
    memmove(float_in,float_in+BLOCK_SIZE,sizeof(*float_in)*BLOCK_SIZE);
    fread(short_in, sizeof(short), BLOCK_SIZE, fin);
      
    if (feof(fin))
      break;
    for (i=0;i<BLOCK_SIZE;i++)
      float_in[i+BLOCK_SIZE] = short_in[i] * .000030517578125;

    {
      /* generate a log spectrum */
      float fft_buf[BLOCK_SIZE*2];
      float weight[BLOCK_SIZE+1];

      blackmann_harris(fft_buf, float_in, BLOCK_SIZE*2);
      //dump_vec(float_in,BLOCK_SIZE*2,"data",frame);
      dump_vec(fft_buf,BLOCK_SIZE*2,"win_data",frame);
      drft_forward(&fft, fft_buf);
      for(i=0;i<BLOCK_SIZE*2;i++)fft_buf[i] *= 2./BLOCK_SIZE;
      //dump_vec(fft_buf,BLOCK_SIZE*2,"fft",frame);

      mag_dB(fft_buf,BLOCK_SIZE*2);
      dump_vec(fft_buf,BLOCK_SIZE+1,"logmag",frame);

      level_mean(fft_buf,weight,BLOCK_SIZE+1, 512,1024, 15, 44100);
      dump_vec(weight,BLOCK_SIZE+1,"weight",frame);

    }


    //fwrite(short_in, sizeof(short), BLOCK_SIZE, fout);
    frame++;
   }

   return 0;
}

