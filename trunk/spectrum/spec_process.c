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
float *process_work;

float **feedback_acc;
float **feedback_max;
float **feedback_instant;

float **ph_acc;
float **ph_max;
float **ph_instant;

float **xmappingL;
float **xmappingM;
float **xmappingH;
int metascale = -1;
int metawidth = -1;
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

    ph_acc=malloc(total_ch*sizeof(*ph_acc));
    ph_max=malloc(total_ch*sizeof(*ph_max));
    ph_instant=malloc(total_ch*sizeof(*ph_instant));

    freqbuffer=fftwf_malloc((blocksize+2)*sizeof(*freqbuffer));
    for(i=0;i<total_ch;i++){

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

  feedback_increment++;
  write(eventpipe[1],"",1);
  return 1;
}

void *process_thread(void *dummy){
  init_process();
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

/* how many bins to 'trim' off the edge of calculated data when we
   know we've hit a boundary of marginal measurement */
#define binspan 5

/* the data returned is now 2 vals per bin; a min and a max.  The spec
   plot merely draws a vertical line between. */
float **process_fetch(int scale, int mode, int link,
		      int *active, int width,
		      float *ymax, float *pmax, float *pmin){
  int ch,ci,i,j,fi;
  float **data;
  float **ph;
  float *normptr;
  float maxrate=-1.;
  float nyq;

  init_process();

  for(fi=0;fi<inputs;fi++)
    if(rate[fi]>maxrate)maxrate=rate[fi];
  nyq=maxrate/2.;

  /* are our scale mappings up to date? */
  if(scale != metascale || width != metawidth){
    if(!xmappingL) xmappingL = calloc(inputs, sizeof(*xmappingL));
    if(!xmappingM) xmappingM = calloc(inputs, sizeof(*xmappingM));
    if(!xmappingH) xmappingH = calloc(inputs, sizeof(*xmappingH));
    metanoise=-1;

    for(fi=0;fi<inputs;fi++){

      /* if mapping preexists, resize it */
      if(xmappingL[fi]){
	xmappingL[fi] = realloc(xmappingL[fi],(width+1)*sizeof(**xmappingL));
      }else{
	xmappingL[fi] = malloc((width+1)*sizeof(**xmappingL));
      }
      if(xmappingM[fi]){
	xmappingM[fi] = realloc(xmappingM[fi],(width+1)*sizeof(**xmappingM));
      }else{
	xmappingM[fi] = malloc((width+1)*sizeof(**xmappingM));
      }
      if(xmappingH[fi]){
	xmappingH[fi] = realloc(xmappingH[fi],(width+1)*sizeof(**xmappingH));
      }else{
	xmappingH[fi] = malloc((width+1)*sizeof(**xmappingH));
      }

      metascale = scale;
      metawidth = width;

      /* generate new numbers */
      for(i=0;i<width;i++){
	float off=.5;
	float loff=1.;
	float hoff=1.;
	float lfreq,mfreq,hfreq;

        /* awaiting new RBW/ VBW code */
        off=.5;

	switch(scale){
	case 0: /* log */
	  lfreq= pow(10.,(i-off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * loff;
	  mfreq= pow(10.,((float)i)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.));
	  hfreq= pow(10.,(i+off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * hoff;
	  break;
	case 1: /* ISO */
	  lfreq= pow(2.,(i-off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) * loff;
	  mfreq= pow(2.,((float)i)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.));
	  hfreq= pow(2.,(i+off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) *hoff;
	  break;
	case 2: /* screen-resolution linear */
	  lfreq=(i-off)*nyq/(width-1)*loff;
	  mfreq=((float)i)*nyq/(width-1);
	  hfreq=(i+off)*nyq/(width-1)*hoff;
	  break;
	}

	xmappingL[fi][i]=lfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingM[fi][i]=mfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingH[fi][i]=hfreq/(rate[fi]*.5)*(blocksize/2);

      }

      for(i=0;i<width;i++){
	if(xmappingL[fi][i]<0.)xmappingL[fi][i]=0.;
	if(xmappingL[fi][i]>blocksize/2.)xmappingL[fi][i]=blocksize/2.;
	if(xmappingM[fi][i]<0.)xmappingM[fi][i]=0.;
	if(xmappingM[fi][i]>blocksize/2.)xmappingM[fi][i]=blocksize/2.;
	if(xmappingH[fi][i]<0.)xmappingH[fi][i]=0.;
	if(xmappingH[fi][i]>blocksize/2.)xmappingH[fi][i]=blocksize/2.;
      }
    }

    for(i=0;i<total_ch;i++)
      if(plot_data[i]){
	plot_data[i] = realloc(plot_data[i],(width+1)*2*sizeof(**plot_data));
      }else{
	plot_data[i] = malloc((width+1)*2*sizeof(**plot_data));
      }
  }

  /* mode selects the base data set */
  normptr=NULL;
  switch(mode){
  case 0: /* independent / instant */
    data=feedback_instant;
    ph=ph_instant;
    break;
  case 1: /* independent / max */
    data=feedback_max;
    ph=ph_max;
    break;
  case 2: /* independent / accumulate */
    data=feedback_acc;
    ph=ph_acc;
    break;
  case 3: /* independent / average */
    data=feedback_acc;
    ph=ph_acc;
    normptr=feedback_count;
    break;
  }
  
  ch=0;
  *ymax = -210.;
  *pmax = -180.;
  *pmin = 180.;
  for(fi=0;fi<inputs;fi++){
    float *L = xmappingL[fi];
    float *M = xmappingM[fi];
    float *H = xmappingH[fi];
    float normalize = normptr ? 1./normptr[fi] : 1.;

    switch(link){
    case LINK_INDEPENDENT:

      for(ci=0;ci<channels[fi];ci++){
	if(active[ch+ci]){
          float *y = plot_data[ci+ch];
          float *m = data[ci+ch];
          int prevbin;
          float prevy;
	  for(i=0;i<width;i++){
	    int first=ceil(L[i]);
	    int last=ceil(H[i]);
	    float firsty,lasty,min,max;

            /* don't allow roundoff error to skip a bin entirely */
            if(i>0 && prevbin<first)first=prevbin;
            prevbin=last;

	    if(first==last){
	      float del=M[i]-floor(M[i]);
              int mid = floor(M[i]);
              float a = todB(m[mid]*normalize);
              float b = todB(m[mid+1]*normalize);
	      firsty=lasty=min=max=(a+(b-a)*del);

	    }else{
	      firsty=min=max=m[first];
	      for(j=first+1;j<last;j++){
                if(m[j]<min)min=m[j];
                if(m[j]>max)max=m[j];
              }
              lasty=todB(m[j-1]*normalize);
              firsty=todB(firsty*normalize);
              min=todB(min*normalize);
              max=todB(max*normalize);
	    }

            max*=.5;
            min*=.5;
	    if(max>*ymax)*ymax=max;

            /* link non-overlapping bins into contiguous lines */
            if(i>0){
              float midpoint = (prevy+firsty)*.25;

              if(midpoint<min)min=midpoint;
              if(midpoint>max)max=midpoint;

              if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
              if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
            }

	    y[i*2]=min;
	    y[i*2+1]=max;

            prevy=lasty;
	  }
	}
      }
      break;

    case LINK_SUMMED:
      {
        float *y = plot_data[ch];
        float **m = data+ch;
        int prevbin;
        float prevy;
        for(i=0;i<width;i++){
          int first=ceil(L[i]);
          int last=ceil(H[i]);
          float firsty,lasty,min,max;

          /* don't allow roundoff error to skip a bin entirely */
          if(i>0 && prevbin<first)first=prevbin;
          prevbin=last;

          if(first==last){
            float a=0.;
            float b=0.;
            int mid = floor(M[i]);
            float del=M[i]-floor(M[i]);
            for(ci=0;ci<channels[fi];ci++){
              if(active[ch+ci]){
                a+=m[ci][mid];
                b+=m[ci][mid+1];
              }
            }
            a=todB(a*normalize);
            b=todB(b*normalize);
            firsty=lasty=min=max=(a+(b-a)*del);
          }else{
            float a=0.;
            for(ci=0;ci<channels[fi];ci++){
              if(active[ch+ci]) a+=m[ci][first];
            }
            firsty=min=max=a;

            for(j=first+1;j<last;j++){
              a=0.;
              for(ci=0;ci<channels[fi];ci++){
                if(active[ch+ci]) a+=m[ci][j];
              }
              if(a<min)min=a;
              if(a>max)max=a;
            }

            lasty=todB(a*normalize);
            firsty=todB(firsty*normalize);
            min=todB(min*normalize);
            max=todB(max*normalize);
          }

          min*=.5;
          max*=.5;

          if(max>*ymax)*ymax=max;

          /* link non-overlapping bins into contiguous lines */
          if(i>0){
            float midpoint = (prevy+firsty)*.25;

            if(midpoint<min)min=midpoint;
            if(midpoint>max)max=midpoint;

            if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
            if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
          }

          y[i*2]=min;
          y[i*2+1]=max;

          prevy=lasty;
        }
      }
      break;

    case LINK_SUB_FROM:
      {
	float *y = plot_data[ch];
	if(active[ch]==0){
	  for(i=0;i<width*2+2;i++)
	    y[i]=-300;
	}else{
          float *y = plot_data[ch];
          float **m = data+ch;
          int prevbin;
          float prevy;
          for(i=0;i<width;i++){
            int first=ceil(L[i]);
            int last=ceil(H[i]);
            float firsty,lasty,min,max;

            /* don't allow roundoff error to skip a bin entirely */
            if(i>0 && prevbin<first)first=prevbin;
            prevbin=last;

            if(first==last){
              int mid = floor(M[i]);
              float del=M[i]-floor(M[i]);
              float a=m[0][mid];
              float b=m[0][mid+1];
              for(ci=1;ci<channels[fi];ci++){
                if(active[ch+ci]){
                  a-=m[ci][mid];
                  b-=m[ci][mid+1];
                }
              }
              a=todB(a*normalize);
              b=todB(b*normalize);
              firsty=lasty=min=max=(a+(b-a)*del);
            }else{
              float a=m[0][first];
              for(ci=1;ci<channels[fi];ci++){
                if(active[ch+ci]) a-=m[ci][first];
              }
              firsty=min=max=a;

              for(j=first+1;j<last;j++){
                a=m[0][j];
                for(ci=1;ci<channels[fi];ci++){
                  if(active[ch+ci]) a-=m[ci][j];
                }
                if(a<min)min=a;
                if(a>max)max=a;
              }

              lasty=todB(a*normalize);
              firsty=todB(firsty*normalize);
              min=todB(min*normalize);
              max=todB(max*normalize);
            }

            min*=.5;
            max*=.5;

            if(max>*ymax)*ymax=max;

            /* link non-overlapping bins into contiguous lines */
            if(i>0){
              float midpoint = (prevy+firsty)*.25;

              if(midpoint<min)min=midpoint;
              if(midpoint>max)max=midpoint;

              if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
              if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
            }

            y[i*2]=min;
            y[i*2+1]=max;

            prevy=lasty;
          }
        }
      }
      break;
    case LINK_SUB_REF:
      {
        float *y = plot_data[ch];
        for(i=0;i<width*2+2;i++)
          y[i]=-300;
      }
      {
        float *r = data[ch];
        for(ci=1;ci<channels[fi];ci++){
          if(active[ch+ci]){
            float *y = plot_data[ci+ch];
            float *m = data[ci+ch];
            int prevbin;
            float prevy;
            for(i=0;i<width;i++){
              int first=ceil(L[i]);
              int last=ceil(H[i]);
              float firsty,lasty,min,max;

              /* don't allow roundoff error to skip a bin entirely */
              if(i>0 && prevbin<first)first=prevbin;
              prevbin=last;

              if(first==last){
                float del=M[i]-floor(M[i]);
                int mid = floor(M[i]);
                float a = todB((m[mid]-r[mid])*normalize);
                float b = todB((m[mid+1]-r[mid])*normalize);
                firsty=lasty=min=max=(a+(b-a)*del);

              }else{
                firsty=min=max=m[first]-r[first];
                for(j=first+1;j<last;j++){
                  if(m[j]<min)min=m[j]-r[j];
                  if(m[j]>max)max=m[j]-r[j];
                }
                lasty=todB((m[j-1]-r[j-1])*normalize);
                firsty=todB(firsty*normalize);
                min=todB(min*normalize);
                max=todB(max*normalize);
              }

              max*=.5;
              min*=.5;
              if(max>*ymax)*ymax=max;

              /* link non-overlapping bins into contiguous lines */
              if(i>0){
                float midpoint = (prevy+firsty)*.25;

                if(midpoint<min)min=midpoint;
                if(midpoint>max)max=midpoint;

                if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
                if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
              }

              y[i*2]=min;
              y[i*2+1]=max;

              prevy=lasty;
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
	float *m = data[ch+1];
	float *p = ph[ch+1];

	if(feedback_count[ch]==0){
	  memset(om,0,width*2*sizeof(*om));
	  memset(op,0,width*2*sizeof(*op));
	}else{
	  /* two vectors only; response and phase */
	  /* response is a standard minmax vector */
          /* phase is averaged to screen resolution */
	  if(active[ch] || active[ch+1]){

            int prevbin;
            float prevy;
            float prevP=0;
            for(i=0;i<width;i++){
              int first=ceil(L[i]);
              int last=ceil(H[i]);
              float firsty,lasty,min,max;
              float P,R,I;

              /* don't allow roundoff error to skip a bin entirely */
              if(i>0 && prevbin<first)first=prevbin;
              prevbin=last;

              if(first==last){
                float del=M[i]-floor(M[i]);
                int mid = floor(M[i]);
                float a = todB(m[mid]/r[mid]);
                float b = todB(m[mid+1]/r[mid+1]);
                firsty=lasty=min=max=(a+(b-a)*del);

                if(active[ch+1]){
                  float aP = (isnan(a) ? NAN : atan2f(p[mid*2+1],p[mid*2]));
                  float bP = (isnan(b) ? NAN : atan2f(p[mid*2+3],p[mid*2+2]));
                  P=(aP+(bP-aP)*del)*57.29;
                }

              }else{
                firsty=min=max=m[first]/r[first];
                R = p[first*2];
                I = p[first*2+1];

                for(j=first+1;j<last;j++){
                  float a = m[j]/r[j];
                  if(a<min)min=a;
                  if(a>max)max=a;
                  R += p[j*2];
                  I += p[j*2+1];
                }

                lasty=todB(m[j-1]/r[j-1]);
                firsty=todB(firsty);
                min=todB(min);
                max=todB(max);

                if(active[ch+1])
                  P = atan2f(I,R)*57.29;
              }

              max*=.5;
              min*=.5;
              if(max>*ymax)*ymax=max;
              if(P>*pmax)*pmax = P;
	      if(P<*pmin)*pmin = P;

              if(active[ch+1] && min>-70){
                float midpoint = (prevP+P)*.5;
                op[i*2]=P;
                op[i*2+1]=P;

                /* link phase into contiguous line */
                if(i){
                  if(midpoint<P) op[i*2]=midpoint;
                  if(midpoint>P) op[i*2+1]=midpoint;
                  if(midpoint<op[i*2-2]) op[i*2-2]=midpoint;
                  if(midpoint>op[i*2-1]) op[i*2-1]=midpoint;
                }
              }else{
                op[i*2]=op[i*2+1]=NAN;
              }

              /* link non-overlapping bins into contiguous lines */
              if(i>0){
                float midpoint = (prevy+firsty)*.25;

                if(midpoint<min)min=midpoint;
                if(midpoint>max)max=midpoint;

                if(midpoint<om[i*2-2])om[i*2-2]=midpoint;
                if(midpoint>om[i*2-1])om[i*2-1]=midpoint;
              }

              om[i*2]=min;
              om[i*2+1]=max;

              prevy=lasty;
              prevP=P;
            }
          }
	}
      }
      break;
    }
    ch+=channels[fi];
  }
  
  return plot_data;
}

