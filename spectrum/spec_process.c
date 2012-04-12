/*
 *
 *  gtk2 spectrum analyzer
 *    
 *      Copyright (C) 2004 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  The analyzer is distributed in the hope that it will be useful,
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

#include "analyzer.h"
#include "io.h"

static float *window=NULL;
static float *freqbuffer=0;
static fftwf_plan plan;

pthread_mutex_t feedback_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
int feedback_increment=0;

float *feedback_count;
float **plot_data;
float *plot_floor=NULL;
float **work_floor=NULL;
float *process_work;

float **feedback_acc;
float **feedback_max;
float **feedback_instant;

/* Gentlemen, power up the Variance hammer */
float **floor_y;
float **floor_yy;
int floor_count;

float **ph_acc;
float **ph_max;
float **ph_instant;

float **xmappingL;
float **xmappingH;
int metascale = -1;
int metawidth = -1;
int metares = -1;
int metanoise = 0;

sig_atomic_t acc_clear=0;
sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

static void init_process(void){
  int i;
  if(window==NULL){
    process_work=calloc(blocksize+2,sizeof(*process_work));
    feedback_count=calloc(total_ch,sizeof(*feedback_count));
    plot_data=calloc(total_ch,sizeof(*plot_data));

    feedback_acc=malloc(total_ch*sizeof(*feedback_acc));
    feedback_max=malloc(total_ch*sizeof(*feedback_max));
    feedback_instant=malloc(total_ch*sizeof(*feedback_instant));
    floor_y=malloc(total_ch*sizeof(*floor_y));
    floor_yy=malloc(total_ch*sizeof(*floor_yy));

    ph_acc=malloc(total_ch*sizeof(*ph_acc));
    ph_max=malloc(total_ch*sizeof(*ph_max));
    ph_instant=malloc(total_ch*sizeof(*ph_instant));

    freqbuffer=fftwf_malloc((blocksize+2)*sizeof(*freqbuffer));
    for(i=0;i<total_ch;i++){

      floor_y[i]=calloc(blocksize/2+1,sizeof(**floor_y));
      floor_yy[i]=calloc(blocksize/2+1,sizeof(**floor_yy));
      feedback_acc[i]=calloc(blocksize/2+1,sizeof(**feedback_acc));
      feedback_max[i]=calloc(blocksize/2+1,sizeof(**feedback_max));
      feedback_instant[i]=calloc(blocksize/2+1,sizeof(**feedback_instant));

      ph_acc[i]=calloc(blocksize+2,sizeof(**ph_acc));
      ph_max[i]=calloc(blocksize+2,sizeof(**ph_max));
      ph_instant[i]=calloc(blocksize+2,sizeof(**ph_instant));
    }

    plan=fftwf_plan_dft_r2c_1d(blocksize,freqbuffer,
                               (fftwf_complex *)freqbuffer,
                               FFTW_ESTIMATE);

    /* construct proper window (sin^4 I'd think) */
    window = calloc(blocksize,sizeof(*window));
    for(i=0;i<blocksize;i++)window[i]=sin(M_PIl*i/blocksize);
    for(i=0;i<blocksize;i++)window[i]*=window[i];
    for(i=0;i<blocksize;i++)window[i]=sin(window[i]*M_PIl*.5);
    for(i=0;i<blocksize;i++)window[i]*=window[i]/(blocksize/4)*.778;
  }
}

void rundata_clear(){
  int i,j;
  for(i=0;i<total_ch;i++){
    feedback_count[i]=0;
    memset(feedback_acc[i],0,(blocksize/2+1)*sizeof(**feedback_acc));
    memset(feedback_max[i],0,(blocksize/2+1)*sizeof(**feedback_max));
    memset(feedback_instant[i],0,(blocksize/2+1)*sizeof(**feedback_instant));

    for(j=0;j<blocksize+2;j++){
      ph_acc[i][j]=0;
      ph_max[i][j]=0;
      ph_instant[i][j]=0;
    }
  }
  acc_clear=0;
}

extern int plot_noise;

/* return 0 on EOF, 1 otherwise */
static int process(){
  int fi,i,j,ch;
  int eof_all;
  int noise=plot_noise;  

  if(acc_rewind)
    rewind_files();
  acc_rewind=0;

  if(input_read(acc_loop,0))
    return 0;

  if(acc_clear)
    rundata_clear();

  /* by channel */
  ch=0;
  for(fi=0;fi<inputs;fi++){
    if(blockbufferfill[fi]){
      for(i=ch;i<ch+channels[fi];i++){
	
	float *data=blockbuffer[i];

	/* window the blockbuffer into the FFT buffer */
	for(j=0;j<blocksize;j++){
	  freqbuffer[j]=data[j]*window[j];
	}
	
	/* transform */
	fftwf_execute(plan);
	
	pthread_mutex_lock(&feedback_mutex);

	/* perform desired accumulations */
	for(j=0;j<blocksize+2;j+=2){
	  float R = freqbuffer[j];
	  float I = freqbuffer[j+1];
	  float sqR = R*R;
	  float sqI = I*I;
	  float sqM = sqR+sqI;

	  if(noise==1){
	    floor_yy[i][j>>1]+=sqM*sqM;
	    floor_y[i][j>>1]+=sqM;
	  }
	  
	  /* deal with phase accumulate/rotate */
	  if(i==ch){
	    /* normalize/store ref for later rotation */
	    process_work[j] = R;
	    process_work[j+1] = -I;

	  }else{
	    /* rotate signed square phase according to ref for phase calculation */
	    float pR;
	    float pI;
	    float rR = process_work[j];
	    float rI = process_work[j+1];
	    pR = (rR*R - rI*I);
	    pI = (rR*I + rI*R);

	    ph_instant[i][j]=pR;
	    ph_instant[i][j+1]=pI;

	    ph_acc[i][j]+=pR;
	    ph_acc[i][j+1]+=pI;
	    
	    if(feedback_max[i][j>>1]<sqM){
	      ph_max[i][j]=pR;
	      ph_max[i][j+1]=pI;
	    }
	  }
	  
	  feedback_instant[i][j>>1]=sqM;
	  feedback_acc[i][j>>1]+=sqM;
	  
	  if(feedback_max[i][j>>1]<sqM)
	    feedback_max[i][j>>1]=sqM;
	  
	}
	feedback_count[i]++;
	
	pthread_mutex_unlock(&feedback_mutex);
      }
    }
    ch+=channels[fi];
  }
  if(noise==1)
    floor_count++;
  feedback_increment++;
  write(eventpipe[1],"",1);
  return 1;
}

void *process_thread(void *dummy){
  while(!process_exit && process());
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

void process_dump(int mode){
  int fi,i,j,ch;
  FILE *out;

  {   
    out=fopen("accumulate.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){
      for(i=0;i<blocksize/2+1;i++){
	fprintf(out,"%f ",(double)i*rate[fi]/blocksize);
	
	for(j=ch;j<ch+channels[fi];j++)
	  fprintf(out,"%f ",todB(feedback_acc[j][i])*.5);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

  {   
    out=fopen("max.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){
      for(i=0;i<blocksize/2+1;i++){
	fprintf(out,"%f ",(double)i*rate[fi]/blocksize);
	
	for(j=ch;j<ch+channels[fi];j++)
	  fprintf(out,"%f ",todB(feedback_max[j][i])*.5);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

  {   
    out=fopen("instant.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){
      for(i=0;i<blocksize/2+1;i++){
	fprintf(out,"%f ",(double)i*rate[fi]/blocksize);
	
	for(j=ch;j<ch+channels[fi];j++)
	  fprintf(out,"%f ",todB(feedback_instant[j][i])*.5);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

  {   
    out=fopen("accphase.m","w");
    ch = 0;
    for(fi=0;fi<inputs;fi++){

      /* phase */ 
      for(i=0;i<blocksize+2;i+=2){
	fprintf(out,"%f ",(double)i*.5*rate[fi]/blocksize);
	fprintf(out,"%f ",atan2(ph_acc[ch+1][i+1],ph_acc[ch+1][i])*57.29);
	fprintf(out,"\n");
      }
      fprintf(out,"\n");
      ch+=channels[fi];
    }
    fclose(out);
  }

}

void clear_noise_floor(){
  int i;
  for(i=0;i<total_ch;i++){
    memset(floor_y[i],0,(blocksize/2+1)*sizeof(**floor_y));
    memset(floor_yy[i],0,(blocksize/2+1)*sizeof(**floor_yy));
  }
  floor_count=0;
}

/* how many bins to 'trim' off the edge of calculated data when we
   know we've hit a boundary of marginal measurement */
#define binspan 5

float **process_fetch(int res, int scale, int mode, int link, 
		      int *active, int width, 
		      float *ymax, float *pmax, float *pmin,
		      float **yfloor,int noise){
  int ch,ci,i,j,fi;
  float **data;
  float **ph;
  float maxrate=-1.;
  float nyq;

  init_process();

  for(fi=0;fi<inputs;fi++)
    if(rate[fi]>maxrate)maxrate=rate[fi];
  nyq=maxrate/2.;

  *yfloor=NULL;

  /* are our scale mappings up to date? */
  if(res != metares || scale != metascale || width != metawidth){
    if(!xmappingL) xmappingL = calloc(inputs, sizeof(*xmappingL));
    if(!xmappingH) xmappingH = calloc(inputs, sizeof(*xmappingH));
    metanoise=-1;

    if(!work_floor)
      work_floor = calloc(total_ch,sizeof(*work_floor));
    for(i=0;i<total_ch;i++){
      if(work_floor[i])
	work_floor[i] = realloc(work_floor[i],(width+1)*sizeof(**work_floor));
      else
	work_floor[i] = calloc((width+1),sizeof(**work_floor));
    }

    for(fi=0;fi<inputs;fi++){

      /* if mapping preexists, resize it */
      if(xmappingL[fi]){
	xmappingL[fi] = realloc(xmappingL[fi],(width+1)*sizeof(**xmappingL));
      }else{
	xmappingL[fi] = malloc((width+1)*sizeof(**xmappingL));
      }
      if(xmappingH[fi]){
	xmappingH[fi] = realloc(xmappingH[fi],(width+1)*sizeof(**xmappingH));
      }else{
	xmappingH[fi] = malloc((width+1)*sizeof(**xmappingH));
      }

      metascale = scale;
      metawidth = width;
      metares = res;

      
      /* generate new numbers */
      for(i=0;i<width;i++){
	float off=0;
	float loff=1.;
	float hoff=1.;
	float lfreq,hfreq;

	switch(res){
	case 0: /* screen-resolution */
	  off=1.;
	  break;
	case 1: /* 1/24th octave */
	  loff = .95918945710913818816;
	  hoff = 1.04254690518999138632;
	  break;
	case 2: /* 1/12th octave */
	  loff = .94387431268169349664;
	  hoff = 1.05946309435929526455;
	  break;
	case 3: /* 1/3th octave */
	  loff = .79370052598409973738;
	  hoff = 1.25992104989487316475;
	  break;
	}

	switch(scale){
	case 0: /* log */
	  lfreq= pow(10.,(i-off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * loff;
	  hfreq= pow(10.,(i+off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * hoff;
	  break;
	case 1: /* ISO */
	  lfreq= pow(2.,(i-off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) * loff;
	  hfreq= pow(2.,(i+off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) *hoff;
	  break;
	case 2: /* screen-resolution linear */
	  lfreq=(i-off)*nyq/(width-1)*loff;
	  hfreq=(i+off)*nyq/(width-1)*hoff;
	  break;
	}

	xmappingL[fi][i]=lfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingH[fi][i]=hfreq/(rate[fi]*.5)*(blocksize/2);

      }
      
      for(i=0;i<width;i++){
	if(xmappingL[fi][i]<0.)xmappingL[fi][i]=0.;
	if(xmappingL[fi][i]>blocksize/2.)xmappingL[fi][i]=blocksize/2.;
	if(xmappingH[fi][i]<0.)xmappingH[fi][i]=0.;
	if(xmappingH[fi][i]>blocksize/2.)xmappingH[fi][i]=blocksize/2.;
      }
    }

    for(i=0;i<total_ch;i++)
      if(plot_data[i]){
	plot_data[i] = realloc(plot_data[i],(width+1)*sizeof(**plot_data));
      }else{
	plot_data[i] = malloc((width+1)*sizeof(**plot_data));
      }
  }

  /* 'illustrate' the noise floor */
  if(noise){
    if(plot_floor)
      plot_floor=realloc(plot_floor,(width+1)*sizeof(*plot_floor));
    else
      plot_floor=calloc((width+1),sizeof(*plot_floor));
    
    if(metanoise!=link){
      float *y = plot_floor;
      int ch=0;
      metanoise=link;
      for(i=0;i<width;i++)
	y[i]=-300;
      
      for(fi=0;fi<inputs;fi++){
	float *L = xmappingL[fi];
	float *H = xmappingH[fi];
	float d = 1./floor_count;
	
	for(ci=0;ci<channels[fi];ci++){
	  float *fy = floor_y[ci+ch];
	  float *fyy = floor_yy[ci+ch];
	  float *w = work_floor[ci+ch];
	  
	  for(i=0;i<width;i++){
	    int first=floor(L[i]);
	    int last=floor(H[i]);
	    float esum;
	    float vsum;
	    float v = fyy[first]*floor_count - fy[first]*fy[first];
	    
	    if(first==last){
	      float del=H[i]-L[i];
	      esum=fy[first]*del;
	      vsum=v*del;
	    }else{
	      float del=1.-(L[i]-first);
	      esum=fy[first]*del;
	      vsum=v*del;
	      
	      for(j=first+1;j<last;j++){
		v = fyy[j]*floor_count - fy[j]*fy[j];
		esum+=fy[j];
		vsum+=v;
	      }
	      
	      v = fyy[last]*floor_count - fy[last]*fy[last];
	      del=(H[i]-last);
	      esum+=fy[last]*del;
	      vsum+=v*del;
	    }
	    vsum = 10*sqrt(vsum)*d;
	    esum*=d;
	    w[i] = esum+vsum*10;
	    esum = todB_a(w+i)*.5;
	    
	    if(esum>y[i])y[i]=esum;
	  }
	}
	ch+=channels[fi];
      }
    }
    if(link == LINK_INDEPENDENT && mode==0)
      *yfloor=plot_floor;
  }else{
    for(i=0;i<total_ch;i++)
      memset(work_floor[i],0,width*sizeof(**work_floor));
    metanoise=-1;
  }
  
  /* mode selects the base data set */
  switch(mode){    
  case 0: /* independent / instant */
    data=feedback_instant;
    ph=ph_instant;
    break;
  case 1: /* independent / max */
    data=feedback_max;
    ph=ph_max;
    break;
  case 2:
    data=feedback_acc;
    ph=ph_acc;
    break;
  }
  
  ch=0;
  *ymax = -150.;
  *pmax = -180.;
  *pmin = 180.;
  for(fi=0;fi<inputs;fi++){
    float *L = xmappingL[fi];
    float *H = xmappingH[fi];

    switch(link){
    case LINK_INDEPENDENT:
      
      for(ci=0;ci<channels[fi];ci++){
	float *y = plot_data[ci+ch];
	float *m = data[ci+ch];
	if(active[ch+ci]){
	  for(i=0;i<width;i++){
	    int first=floor(L[i]);
	    int last=floor(H[i]);
	    float sum;
	    
	    if(first==last){
	      float del=H[i]-L[i];
	      sum=m[first]*del;
	    }else{
	      float del=1.-(L[i]-first);
	      sum=m[first]*del;
	      
	      for(j=first+1;j<last;j++)
		sum+=m[j];
	      
	      del=(H[i]-last);
	      sum+=m[last]*del;
	    }

	    sum=todB_a(&sum)*.5;
	    if(sum>*ymax)*ymax=sum;
	    y[i]=sum;	  
	  }
	}
      }
      break;

    case LINK_SUMMED:
      {
	float *y = plot_data[ch];
	memset(y,0,(width+1)*sizeof(*y));
      
	for(ci=0;ci<channels[fi];ci++){
	  float *m = data[ci+ch];
	  if(active[ch+ci]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      
	      if(first==last){
		float del=H[i]-L[i];
		y[i]+=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		y[i]+=m[first]*del;
		
		for(j=first+1;j<last;j++)
		  y[i]+=m[j];
		
		del=(H[i]-last);
		y[i]+=m[last]*del;
	      }
	    }
	  }
	}
      
	for(i=0;i<width;i++){
	  float sum=todB_a(y+i)*.5;
	  if(sum>*ymax)*ymax=sum;
	  y[i]=sum;	  
	}
      }
      break;
      
    case LINK_SUB_FROM:
      {
	float *y = plot_data[ch];
	if(active[ch]==0){
	  for(i=0;i<width;i++)
	    y[i]=-300;
	}else{
	  for(ci=0;ci<channels[fi];ci++){
	    float *m = data[ci+ch];
	    if(ci==0 || active[ch+ci]){
	      for(i=0;i<width;i++){
		int first=floor(L[i]);
		int last=floor(H[i]);
		float sum;
		
		if(first==last){
		  float del=H[i]-L[i];
		  sum=m[first]*del;
		}else{
		  float del=1.-(L[i]-first);
		  sum=m[first]*del;
		  
		  for(j=first+1;j<last;j++)
		    sum+=m[j];
		  
		  del=(H[i]-last);
		  sum+=m[last]*del;
		}
		
		if(ci==0){
		  y[i]=sum;
		}else{
		  y[i]-=sum;
		}
	      }
	    }
	  }
	  
	  for(i=0;i<width;i++){
	    float v = (y[i]>0?y[i]:0);
	    float sum=todB_a(&v)*.5;
	    if(sum>*ymax)*ymax=sum;
	    y[i]=sum;	  
	  }
	}
      }
      break;
    case LINK_SUB_REF:
      {
	float *r = plot_data[ch];
	for(ci=0;ci<channels[fi];ci++){
	  float *y = plot_data[ch+ci];
	  float *m = data[ci+ch];
	  if(ci==0 || active[ch+ci]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sum;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sum=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		sum=m[first]*del;
		
		for(j=first+1;j<last;j++)
		  sum+=m[j];
		
		del=(H[i]-last);
		sum+=m[last]*del;
	      }
	      
	      if(ci==0){
		r[i]=sum;
	      }else{
		sum=(r[i]>sum?0.f:sum-r[i]);
		y[i]=todB_a(&sum)*.5;
		if(y[i]>*ymax)*ymax=y[i];
	      }
	    }
	  }
	}
      }
      break;
      
    case LINK_IMPEDENCE_p1:
    case LINK_IMPEDENCE_1:
    case LINK_IMPEDENCE_10:
      {
	float shunt = (link == LINK_IMPEDENCE_p1?.1:(link == LINK_IMPEDENCE_1?1:10));
	float *r = plot_data[ch];

	for(ci=0;ci<channels[fi];ci++){
	  float *y = plot_data[ci+ch];
	  float *m = data[ch+ci];
	  
	  if(ci==0 || active[ch+ci]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sum;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sum=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		sum=m[first]*del;
		
		for(j=first+1;j<last;j++)
		  sum+=m[j];
		
		del=(H[i]-last);
		sum+=m[last]*del;
	      }

	      if(ci==0){
		/* stash the reference in the work vector */
		r[i]=sum;
	      }else{
		/* the shunt */
		/* 'r' collected at source, 'sum' across the shunt */
		float V=sqrt(r[i]);
		float S=sqrt(sum);
		
		if(S>(1e-5) && V>S){
		  y[i] = shunt*(V-S)/S;
		}else{
		  y[i] = NAN;
		}
	      }
	    }
	  }
	}
	    
	/* scan the resulting buffers for marginal data that would
	   produce spurious output. Specifically we look for sharp
	   falloffs of > 40dB or an original test magnitude under
	   -70dB. */
	{
	  float max = -140;
	  for(i=0;i<width;i++){
	    float v = r[i] = todB_a(r+i)*.5;
	    if(v>max)max=v;
	  }

	  for(ci=1;ci<channels[fi];ci++){
	    if(active[ch+ci]){
	      float *y = plot_data[ci+ch];	      
	      for(i=0;i<width;i++){
		if(r[i]<max-40 || r[i]<-70){
		  int j=i-binspan;
		  if(j<0)j=0;
		  for(;j<i;j++)
		    y[j]=NAN;
		  for(;j<width;j++){
		    if(r[j]>max-40 && r[j]>-70)break;
		    y[j]=NAN;
		  }
		  i=j+3;
		  for(;j<i && j<width;j++){
		    y[j]=NAN;
		  }
		}
		if(!isnan(y[i]) && y[i]>*ymax)*ymax = y[i];
	      }
	    }
 	  }
	}
      }
      break;

    case LINK_PHASE: /* response/phase */

      if(channels[fi]>=2){
	float *om = plot_data[ch];
	float *op = plot_data[ch+1];

	float *r = data[ch];
	float *rn = work_floor[ch];
	float *m = data[ch+1];
	float *mn = work_floor[ch+1];
	float *p = ph[ch+1];
	float mag[width];

	if(feedback_count[ch]==0){
	  memset(om,0,width*sizeof(*om));
	  memset(op,0,width*sizeof(*op));
	}else{
	  /* two vectors only; response and phase */
	  /* response */
	  if(active[ch] || active[ch+1]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sumR,sumM;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sumR=r[first]*del;
		sumM=m[first]*del;
	      }else{
		float del=1.-(L[i]-first);
		sumR=r[first]*del;
		sumM=m[first]*del;
		
		for(j=first+1;j<last;j++){
		  sumR+=r[j];
		  sumM+=m[j];
		}

		del=(H[i]-last);
		sumR+=r[last]*del;
		sumM+=m[last]*del;
	      }
	      
	      if(sumR>rn[i] && sumM>mn[i]){
		mag[i] = todB_a(&sumR)*.5;
		sumM /= sumR;
		om[i] = todB_a(&sumM)*.5;
	      }else{
		om[i] = NAN;
	      }
	    }
	  }
	  
	  /* phase */
	  if(active[ch+1]){
	    for(i=0;i<width;i++){
	      int first=floor(L[i]);
	      int last=floor(H[i]);
	      float sumR,sumI;
	      
	      if(first==last){
		float del=H[i]-L[i];
		sumR=p[(first<<1)]*del;
		sumI=p[(first<<1)+1]*del;
	      }else{
		float del=1.-(L[i]-first);
		sumR=p[(first<<1)]*del;
		sumI=p[(first<<1)+1]*del;
		
		for(j=first+1;j<last;j++){
		  sumR+=p[(j<<1)];
		  sumI+=p[(j<<1)+1];
		}

		del=(H[i]-last);
		sumR+=p[(last<<1)]*del;
		sumI+=p[(last<<1)+1]*del;
	      }

	      if(!isnan(om[i])){
		op[i] = atan2(sumI,sumR)*57.29;
	      }else{
		op[i]=NAN;
	      }
	    }
	  }
	  
	  /* scan the resulting buffers for marginal data that would
	     produce spurious output. Specifically we look for sharp
	     falloffs of > 40dB or an original test magnitude under
	     -70dB. */
	  if(active[ch] || active[ch+1]){
	    for(i=0;i<width;i++){
	      if(isnan(om[i])){
		int j=i-binspan;
		if(j<0)j=0;
		for(;j<i;j++){
		  om[j]=NAN;
		  op[j]=NAN;
		}
		for(;j<width;j++){
		  if(!isnan(om[j]))break;
		  om[j]=NAN;
		  op[j]=NAN;
		}
		i=j+3;
		for(;j<i && j<width;j++){
		  om[j]=NAN;
		  op[j]=NAN;
		}
	      }
	      if(om[i]>*ymax)*ymax = om[i];
	      if(op[i]>*pmax)*pmax = op[i];
	      if(op[i]<*pmin)*pmin = op[i];
	      
	    }
	  }
	}
      }
      break;
      
    case LINK_THD: /* THD */
    case LINK_THD2: /* THD-2 */
    case LINK_THDN: /* THD+N */
    case LINK_THDN2: /* THD+N-2 */
      
      
      break;
      
    }
    ch+=channels[fi];
  }
  
  return plot_data;
}

