/*
 *
 *  postfish
 *    
 *      Copyright (C) 2002-2004 Monty and Xiph.Org
 *
 *  Postfish is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  Postfish is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include "postfish.h"
#include <math.h>
#include <sys/types.h>
#include "feedback.h"
#include "freq.h"

extern int input_rate;
extern int input_ch;
extern int input_size;

/* feedback! */
typedef struct freq_feedback{
  feedback_generic parent_class;
  double **peak;
  double **rms;
} freq_feedback;

/* accessed only in playback thread/setup */
static double frequencies[freqs+1];
static char *freq_labels[freqs]={
  "31", "37","44","53",
  "63", "75","88","105",
  "125","150","180","210",
  "250","300","350","420",
  "500","600", "700", "840",
  "1k", "1.2k", "1.4k", "1.7k",
  "2k", "2.4k", "2.8k", "3.4k",
  "4k", "4.8k", "5.6k", "6.7k",
  "8k", "9.5k", "11k", "13k",
  "16k","19k",  "22k"
};

void _analysis(char *base,int i,double *v,int n,int bark,int dB){
  int j;
  FILE *of;
  char buffer[80];

  sprintf(buffer,"%s_%d.m",base,i);
  of=fopen(buffer,"w");
  
  if(!of)perror("failed to open data dump file");
  
  for(j=0;j<n;j++){
    if(bark){
      float b=toBark((4000.f*j/n)+.25);
      fprintf(of,"%f ",b);
    }else
      fprintf(of,"%f ",(double)j);
    
    if(dB){
      if(j==0||j==n-1)
	fprintf(of,"%f\n",todB(v[j]));
      else{
	fprintf(of,"%f\n",todB(hypot(v[j],v[j+1])));
	j++;
      }
    }else{
      fprintf(of,"%f\n",v[j]);
    }
  }
  fclose(of);
}

static feedback_generic *new_freq_feedback(void){
  int i;
  freq_feedback *ret=malloc(sizeof(*ret));
  ret->peak=malloc(input_ch*sizeof(*ret->peak));
  ret->rms=malloc(input_ch*sizeof(*ret->rms));

  for(i=0;i<input_ch;i++){
    ret->peak[i]=malloc(freqs*sizeof(**ret->peak));
    ret->rms[i]=malloc(freqs*sizeof(**ret->rms));
  }

  return (feedback_generic *)ret;
}

/* total, peak, rms are pulled in array[freqs][input_ch] order */

int pull_freq_feedback(freq_state *ff,double **peak,double **rms){
  freq_feedback *f=(freq_feedback *)feedback_pull(&ff->feedpool);
  int i,j;
  
  if(!f)return 0;
  
  if(peak)
    for(i=0;i<freqs;i++)
      for(j=0;j<input_ch;j++)
	peak[i][j]=f->peak[j][i];

  if(rms)
    for(i=0;i<freqs;i++)
      for(j=0;j<input_ch;j++)
	rms[i][j]=f->rms[j][i];
  
  feedback_old(&ff->feedpool,(feedback_generic *)f);
  return 1;
}

/* called only by initial setup */
int freq_load(freq_state *f,int blocksize){
  int i,j;
  
  memset(f,0,sizeof(*f));

  drft_init(&f->fft,blocksize*2);
  f->blocksize=blocksize;
  
  f->fillstate=0;
  f->cache_samples=0;
  f->cache=malloc(input_ch*sizeof(*f->cache));
  for(i=0;i<input_ch;i++)
    f->cache[i]=malloc(input_size*sizeof(**f->cache));
  f->lap=malloc(input_ch*sizeof(*f->lap));
  for(i=0;i<input_ch;i++)
    f->lap[i]=malloc(input_size*sizeof(**f->lap));

  f->out.size=input_size;
  f->out.channels=input_ch;
  f->out.rate=input_rate;
  f->out.data=malloc(input_ch*sizeof(*f->out.data));
  for(i=0;i<input_ch;i++)
    f->out.data[i]=malloc(input_size*sizeof(**f->out.data));

  /* fill out the frequencies we potentially offer */
  /* yes this gets repeated.  it's pre-threading, not a big deal */
  for(i=0;i<freqs;i++)
    frequencies[i]=fromOC(i*.25-1.);
  frequencies[freqs-1]=22000.;
  frequencies[freqs]=1e20;


  /* unlike old postfish, we offer all frequencies via smoothly
     supersampling the spectrum */
  /* I'm too lazy to figure out the integral symbolically, use this
     fancy CPU-thing for something */
  
  f->ho_window=malloc(freqs*sizeof(*f->ho_window));
  {
    double lastf=0.;
    double *working=alloca((blocksize+1)*sizeof(*working));

    for(i=0;i<freqs;i++){
      double thisf=frequencies[i];
      double nextf=frequencies[i+1];

      /* conceptually simple, easy to verify, absurdly inefficient,
         but hey, we're only doing it at init */
      memset(working,0,sizeof(*working)*(blocksize+1));
      
      for(j=0;j<((blocksize+1)<<5);j++){
	double localf= .5*j*input_rate/(blocksize<<5);
	int localbin= j>>5;
	double localwin;

	if(localf>=lastf && localf<thisf){
	  if(i==0)
	    localwin=1.;
	  else
	    localwin= sin((localf-lastf)/(thisf-lastf)*M_PIl*.5);
	  localwin*=localwin;
	  working[localbin]+=localwin*(1./32);

	}else if(localf>=thisf && localf<nextf){
	  if(i+1==freqs)
	    localwin=1.;
	  else
	    localwin= sin((nextf-localf)/(nextf-thisf)*M_PIl*.5);
	  
	  localwin*=localwin;
	  working[localbin]+=localwin*(1./32);
	  
	}
      }

      /* now take what we learned and distill it a bit */
      for(j=0;j<blocksize+1;j++)if(working[j]!=0.)break;
      f->ho_bin_lo[i]=j;
      for(;j<blocksize+1;j++)if(working[j]==0.)break;
      f->ho_bin_hi[i]=j;
      f->ho_window[i]=malloc((f->ho_bin_hi[i]-f->ho_bin_lo[i])*
			     sizeof(**f->ho_window));
      for(j=f->ho_bin_lo[i];j<f->ho_bin_hi[i];j++)
	f->ho_area[i]+=f->ho_window[i][j-f->ho_bin_lo[i]]=working[j];

      f->ho_area[i]=1./f->ho_area[i];
      lastf=thisf;

    }
  }
      
  /* fill in time window */
  f->window=malloc(blocksize*sizeof(*f->window)); 
  /* we actually use 2x blocks, but the window is nonzero only in half */
  
  for(i=0;i<blocksize;i++)f->window[i]=sin(M_PIl*i/blocksize);
  for(i=0;i<blocksize;i++)f->window[i]*=f->window[i];
  for(i=0;i<blocksize;i++)f->window[i]=sin(f->window[i]*M_PIl*.5);

  return(0);
}

/* called only in playback thread */
int freq_reset(freq_state *f){
  /* reset cached pipe state */
  f->fillstate=0;
  while(pull_freq_feedback(f,NULL,NULL));
  return 0;
}

const char *freq_frequency_label(int n){
  if(n<0)return "";
  if(n>freqs)return "";
  return freq_labels[n];
}

static void transform_work(double *work,freq_state *f,double *peak,double *rms){
  double *workoff=work+f->blocksize/2;
  int i,j,k;
  
  /* window the time data */
  memset(work,0,sizeof(*work)*f->blocksize);
  memset(work+f->blocksize*3/2,0,sizeof(*work)*f->blocksize/2);
  for(i=0;i<f->blocksize;i++)workoff[i]*=f->window[i];
  
  /* transform the time data */
  drft_forward(&f->fft,work);
  for(i=0;i<f->blocksize;i++)workoff[i]*=(1./f->blocksize);

  /* fill in metrics */
  memset(peak,0,sizeof(*peak)*freqs);
  memset(rms,0,sizeof(*rms)*freqs);
  {
    double sq_mags[f->blocksize+1];
    sq_mags[0]=work[0]*work[0];
    sq_mags[f->blocksize]=work[f->blocksize*2-1]*work[f->blocksize*2-1];
    for(i=1;i<f->blocksize;i++)
      sq_mags[i]=work[i*2]*work[i*2]+work[i*2-1]*work[i*2-1];

    for(i=0;i<freqs;i++){
      double *ho_window=f->ho_window[i];
      for(k=0,j=f->ho_bin_lo[i];j<f->ho_bin_hi[i];j++,k++){
	double val=sq_mags[j]*ho_window[k];
	rms[i]+=val;
	if(val>peak[i])peak[i]=val;
      }
      rms[i]=sqrt(rms[i]*f->ho_area[i]);
      peak[i]=sqrt(peak[i]);
    }
  }
}

static void feedback_work(double *peak,double *rms,
		   double *feedback_peak,double *feedback_rms){
  int i;
  for(i=0;i<freqs;i++){
    feedback_rms[i]+=rms[i]*rms[i];
    if(feedback_peak[i]<peak[i])feedback_peak[i]=peak[i];
  }
}

static void lap_work(double *work,double *lap,double *out,freq_state *f){
  double *workoff=work+f->blocksize/2;
  int i,j;

  /* back to time we go */
  drft_backward(&f->fft,work);
  
  /* lap and out */
  if(out)
    for(i=0;i<f->blocksize/2;i++)
      out[i]=lap[i]+workoff[i]*f->window[i];
  
  for(i=f->blocksize/2,j=0;i<f->blocksize;i++)
    lap[j++]=workoff[i]*f->window[i];

}


/* called only by playback thread */
time_linkage *freq_read(time_linkage *in, freq_state *f,
			void (*func)(double *data,freq_state *f,
				     double *peak, double *rms)){
  int i;

  double feedback_peak[input_ch][freqs];
  double feedback_rms[input_ch][freqs];

  double peak[freqs];
  double rms[freqs];

  int blocks=0;

  memset(peak,0,sizeof(peak));
  memset(rms,0,sizeof(rms));

  {
    double work[f->blocksize*2];
    
    switch(f->fillstate){
    case 0: /* prime the lapping and cache */
      for(i=0;i<input_ch;i++){
	double *temp=in->data[i];

	memset(work+f->blocksize/2,0,sizeof(*work)*f->blocksize/2);
	memcpy(work+f->blocksize,temp,sizeof(*work)*f->blocksize/2);

	transform_work(work,f,peak,rms);
	func(work,f,peak,rms);
	feedback_work(peak,rms,feedback_peak[i],feedback_rms[i]);
	lap_work(work,f->lap[i],0,f);
	blocks++;	

	memset(f->cache[i],0,sizeof(**f->cache)*input_size);
	in->data[i]=f->cache[i];
	f->cache[i]=temp;
      }
      f->cache_samples=in->samples;
      f->fillstate=1;
      f->out.samples=0;
      if(in->samples==in->size)goto tidy_up;
	
      for(i=0;i<input_ch;i++)
	memset(in->data[i],0,sizeof(**in->data)*in->size);
      in->samples=0;
      /* fall through */
    case 1: /* nominal processing */
      for(i=0;i<input_ch;i++){
	double *temp=f->cache[i];
	int j;
	for(j=0;j+f->blocksize<=f->out.size;j+=f->blocksize/2){

	  memset(work,0,sizeof(*work)*f->blocksize/2);
	  memcpy(work+f->blocksize/2,temp+j,sizeof(*work)*f->blocksize);
	  memset(work+f->blocksize*3/2,0,sizeof(*work)*f->blocksize/2);

	  transform_work(work,f,&peak[i],&rms[i]);
	  func(work,f,peak,rms);
	  feedback_work(peak,rms,feedback_peak[i],feedback_rms[i]);
	  lap_work(work,f->lap[i],f->out.data[i]+j,f);
	  blocks++;	
	}

	memset(work,0,sizeof(*work)*f->blocksize/2);
	memcpy(work+f->blocksize/2,temp+j,sizeof(*work)*f->blocksize/2);
	memcpy(work+f->blocksize,in->data[i],sizeof(*work)*f->blocksize/2);
	memset(work+f->blocksize*3/2,0,sizeof(*work)*f->blocksize/2);
	
	transform_work(work,f,&peak[i],&rms[i]);
	func(work,f,peak,rms);
	feedback_work(peak,rms,feedback_peak[i],feedback_rms[i]);
	lap_work(work,f->lap[i],f->out.data[i]+j,f);
	blocks++;	
	
	f->cache[i]=in->data[i];
	in->data[i]=temp;
      }
      f->out.samples=f->cache_samples;
      f->cache_samples=in->samples;
      if(f->out.samples<f->out.size)f->fillstate=2;
      break;
    case 2: /* we've pushed out EOF already */
      f->out.samples=0;
    }
  }

  /* finish up the state feedabck */
  blocks/=input_ch;
  {
    int j;
    double scale=1./blocks;
    freq_feedback *ff=
      (freq_feedback *)feedback_new(&f->feedpool,new_freq_feedback);

    for(i=0;i<input_ch;i++)
      for(j=0;j<freqs;j++)
	feedback_rms[i][j]=sqrt(feedback_rms[i][j]*scale);

      
    for(i=0;i<input_ch;i++){
      memcpy(ff->peak[i],feedback_peak[i],freqs*sizeof(**feedback_peak));
      memcpy(ff->rms[i],feedback_rms[i],freqs*sizeof(**feedback_rms));
    } 
    feedback_push(&f->feedpool,(feedback_generic *)ff);
  }

 tidy_up:
  {
    int tozero=f->out.size-f->out.samples;
    if(tozero)
      for(i=0;i<f->out.channels;i++)
	memset(f->out.data[i]+f->out.samples,0,sizeof(**f->out.data)*tozero);
  }

  return &f->out;
}
