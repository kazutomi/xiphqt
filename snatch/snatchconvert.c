/*
 *
 *  snatch.c
 *    
 *	Copyright (C) 2001 Monty
 *
 *  This file is part of snatch2{wav,yuv}, for use with the MJPEG 
 *  tool suite.
 *	
 *  snatch2wav is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  snatch2wav is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *
 */

/* Snatch files can consist of multiple depths, rates and channels.
   The ideal choice is to resample the whole thing to one common
   format */

/* this isn't added into the lav tools because the audio/video sync
   isn't premade.  It has to be calculated from timestamps and
   maintain an arbitrary sized buffer. */

int audio_p;
int video_p;

double begin_time=0.;
double end_time=1e90;
double global_zerotime=0.;

unsigned char *buftemp;
long           buftempsize;

/* audio resampling ripped from sox and simplified */
/*
 * July 5, 1991
 * Copyright 1991 Lance Norskog And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained. 
 * Lance Norskog And Sundry Contributors are not responsible for 
 * the consequences of using this software.
 */
/*
 * October 29, 1999
 * Various changes, bugfixes(?), increased precision, by Stan Brooks.
 */
/*
 * November 13, 2001
 * Turned a horridly complex sixteen page module prone to roundoff creep 
 * into a simpler, easier to use four page program with no accumulated 
 * error.  --Monty
 */

#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define La        16
#define Lc        7
#define Lp       (Lc+La)
#define Nc       (1<<Lc)
#define Np       (1<<Lp)

#define Amask    ((1<<La)-1)

static double rolloff=0.80; /* roll-off frequency */
static double beta=16;      /* passband/stopband tuning magic */
static long Nmult=45;

typedef struct {
  long input_rate;
  long output_rate;

  long input_head;
  long input_base;
  long output_position;
  long del;
  long m;
  double ratio;
  double factor;

  double *filter;
  double *data;
  long buffersize;
  long bufferhead;

  int nofakep;
} resample_t;

/********************* on the fly audio resampling ******************/

#define IzeroEPSILON 1E-21           
static double Izero(double x){
   double sum, u, halfx, temp;
   long n;

   sum = u = n = 1;
   halfx = x*.5;
   do {
      temp = halfx/(double)n;
      n += 1;
      temp *= temp;
      u *= temp;
      sum += u;
   } while (u >= IzeroEPSILON*sum);
   return(sum);
}

static void LpFilter(double *filter,int n,double roll,double beta,long m){
  long i;

  /* Calculate filter coeffs: */
  filter[0] = roll;
  for(i=1;i<n;i++) {
    double x=M_PI*(double)i/(double)(m);
    filter[i]=sin(x*roll)/x;
  }
  
  if(beta>2) { /* Apply Kaiser window to filter coeffs: */
    double ibeta = 1.0/Izero(beta);
    for(i=1;i<n;i++) {
      double x=(double)i/(double)n;
      filter[i]*=Izero(beta*sqrt(1.-x*x))*ibeta;
    }
  }else
    exit(1);
}

int makeFilter(double *filter,long nwing,double roll,double beta,long m){
  double gain=0.;
  long mwing, i;

   /* it does help accuracy a bit to have the window stop at
    * a zero-crossing of the sinc function */
   mwing = floor((double)nwing/(m/roll))*(m/roll)+0.5;

   /* Design a Nuttall or Kaiser windowed Sinc low-pass filter */
   LpFilter(filter, mwing, roll, beta, m);

   /* 'correct' the DC gain of the lowpass filter */
   for (i=m; i<mwing; i+=m)
     gain+=filter[i];
   gain = 2*gain + filter[0];    /* DC gain of real coefficients */
   
   gain = 1.0/gain;
   for (i=0; i<mwing; i++)
     filter[i]*=gain;
   for (;i<=nwing;i++)filter[i] = 0;

   return(mwing);
}


static void resample_init(resample_t *r, long inputrate, long outputrate,
			  int nofakep){
  long nwing = Nc * (Nmult/2+1) + 1;
  memset(r,0,sizeof(*r));

  if(nofakep){
    r->filter = malloc(sizeof(*r->filter)*(nwing+1));
    makeFilter(r->filter, nwing, rolloff, beta, Nc);
  }

  r->nofakep=nofakep;
  r->input_rate=inputrate;
  r->output_rate=outputrate;
  r->ratio=r->input_rate/(double)r->output_rate;
  r->factor=r->output_rate/(double)r->input_rate;

  r->del=Np; /* Fixed-point Filter sampling-time-increment */
  if(r->factor<1.0)r->del=r->factor*Np+0.5;
  r->m=(nwing<<La)/r->del;

  r->buffersize=(r->m+r->input_rate)*16;
  if(nofakep){
    r->data=calloc(r->buffersize,sizeof(*r->data));
  }

  r->input_head=r->m*2+1; /* this eliminates output delay at the cost of
			    introducing a small absolute time shift */
  r->input_base=r->m;
}

static void resample_clear(resample_t *r){
  if(r){
    if(r->filter)free(r->filter);
    if(r->data)free(r->data);
  }
}

static double linear_product(const double *filter, const double *data,
	      long dir, double t, long del, long m){
  const double f = 1./(1<<La);
  double v=0;
  long h=t*del + (m-1)*del; /* so double sum starts with smallest coef's */
  data+=(m-1)*dir;
  
  do{
    long hh=h>>La;

    /* filter coef, lower La bits by linear interpolation */
    double coef = filter[hh] + (filter[hh+1]-filter[hh]) * (h&Amask) * f;

    v    += coef * *data;
    data -= dir;
    h    -= del;
  }while(--m);

  return v;
}

static void resample_in(resample_t *r,double in){
  if(r->input_head==r->buffersize){
    if(r->nofakep)
      memmove(r->data,r->data+r->input_base-r->m,
	      r->buffersize-r->input_base+r->m);
    r->input_head+=r->m-r->input_base;
    r->input_base=r->m;
  }
  if(r->nofakep)
    r->data[r->input_head]=in;
  r->input_head++;
}

static int resample_out(resample_t *r,double *out){
  double input_integer;
  double input_fraction=modf(r->output_position*r->ratio,&input_integer);
  long offset=r->input_base+(int)input_integer;

  if(offset+r->m+1>=r->input_head)
    return 0;
  else{
    double v;

    if(r->nofakep){
      double *data=r->data+offset;
      
      /* Past inner product: needs Np*Nmult in 31 bits */
      v=linear_product(r->filter,data,-1,input_fraction,r->del,r->m); 
      /* Future inner product: prefer even total */
      v+=linear_product(r->filter,data+1,1,1.-input_fraction,r->del,r->m);
    
      if (r->factor<1.)v*=r->factor;
      *out=v;
    }
    r->output_position++;
    if(r->output_position>=r->output_rate){
      r->output_position=0;
      r->input_base+=r->input_rate;
    }
    return(1);
  }
}


/****************** simple audio sample handling ****************/

short     *audbuf; 
long long  audbuf_samples;  /* singluar samples, not pairs */
double     audbuf_zerotime;
int        audbuf_head;  /* singluar samples, not pairs */
int        audbuf_tail;  /* singluar samples, not pairs */
int        audbuf_size;  /* singluar samples, not pairs */
long       audbuf_rate;
int        audbuf_channels;

long long samplesin=0;
long long samplesout=0;
long long samplesmissing=0;
long long samplesdiscarded=0;

static void buffer_sample(short v,int nofakep){
  if(audbuf_head>=audbuf_size){
    if(audbuf_tail>audbuf_size*3/4){
      audbuf_head-=audbuf_tail;
      if(nofakep)
	memmove(audbuf,audbuf+audbuf_tail,audbuf_head*sizeof(*audbuf));
      audbuf_tail=0;
    }else{
      if(audbuf){
	audbuf_size*=2;
	if(nofakep)
	  audbuf=realloc(audbuf,audbuf_size*sizeof(*audbuf));
      }else{
	if(nofakep){
	  audbuf_size=256*1024;
	  audbuf=malloc(audbuf_size*sizeof(*audbuf));
	}else{
	  audbuf_size=1;
	  audbuf=malloc(audbuf_size*sizeof(*audbuf));
	}
      }
    }
  }
  if(nofakep)
    audbuf[audbuf_head]=v;

  audbuf_head++;
  audbuf_samples++;
}

static void lebuffer_sample(short v,int nofakep){
#ifdef __BIG_ENDIAN__
  buffer_sample(((v<<8)&0xff00) | ((v>>8)&0xff),nofakep);
#else
  buffer_sample(v,nofakep);
#endif
}

static int convert_input(unsigned char *buf,int fmt,int *v){
  switch(fmt){
  case 4:
    *v=((int)buf[0]-128)<<8;
    return(1);
  case 5:
    *v=(buf[0]&0xff) | (((signed char *)buf)[1]<<8);
    return(2);
  case 6:
    *v=(buf[1]&0xff) | (((signed char *)buf)[0]<<8);
    return(2);
  case 7:
    *v=((signed char *)buf)[0]<<8;
    return(1);
  case 8:
    *v=(int)((buf[0]&0xff)|((buf[1]<<8)&0xff00))-32768;
    return(2);
  case 9:
    *v=(int)((buf[1]&0xff)|((buf[0]<<8)&0xff00))-32768;
    return(2);
  }
  *v=0;
  return(1);
}

static void pre_buffer_audio(long samples,int nofakep){
  long i,c=audbuf_head-audbuf_tail;

  /* just to expand lazily */
  for(i=0;i<samples;i++)
    buffer_sample(0,nofakep);
  
  /* shift memory */
  if(nofakep){
    memmove(audbuf+audbuf_tail+samples,audbuf+audbuf_tail,c*sizeof(*audbuf));
    memset(audbuf+audbuf_tail,0,samples*sizeof(*audbuf));
  }
}



/************************ snatch parsing *********************/

static int read_snatch_header(FILE *f){
  char buffer[12];
  int ret=fread(buffer,1,12,f);
  if(ret<0){
    fprintf(stderr,"Error reading header: %s\n",strerror(ferror(f)));
    exit(1);
  }
  if(ret<6){
    fprintf(stderr,"EOF when header expected!\n");
    exit(1);
  }  
  if(strncmp(buffer,"SNATCH",6)){
    fprintf(stderr,"Input does not begin with a Snatch header!\n");
    exit(1);
  }

  audio_p=0;
  video_p=0;
  if(buffer[6]=='A')audio_p=1;
  if(buffer[7]=='V')video_p=1;
  return(1);
}

resample_t resampler[2];

static int process_audio_frame(char *head,FILE *f,int track_or_process){
  int ret;
  long long nextsamplepos=audbuf_samples;
  double t;
  char *s=head+6;
  long a=atoi(s),b,ch,ra,fmt,length;
  s=strchr(s,' ');
  if(!s)return(0);
  b=atoi(s);
  t=a+b*.000001;
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  ch=atoi(s);
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  ra=atoi(s);
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  fmt=atoi(s);
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  length=atoi(s);

  if(length>buftempsize){
    if(buftemp)
      buftemp=realloc(buftemp,length*sizeof(*buftemp));
    else
      buftemp=malloc(length*sizeof(*buftemp));
    buftempsize=length;
  }

  if(track_or_process){
    ret=fread(buftemp,1,length,f);
    if(ret<length)return(0);
  }else{
    ret=fseek(f,length,SEEK_CUR);
    if(ret)return(0);
  }

  if(global_zerotime==0){
    global_zerotime=t;
    begin_time+=t;
    end_time+=t;
  }

  if(t<begin_time)return(1);
  if(t>end_time)return(1);


  if(audbuf_zerotime==0){
    audbuf_zerotime=t;
    audbuf_samples=0;
  }else{
    long long actualpos=(t-audbuf_zerotime)*audbuf_rate*audbuf_channels+.5;
    long i;
    
    //fprintf(stderr,"audio sample jitter: %ld [%ld:%ld]\n",
    //(long)(nextsamplepos-actualpos),(long)nextsamplepos,(long)actualpos);

    /* hold last sample through any gap, assuming a bit of
       hysteresis.  That also holds us through roundoff error (the
       roundoff error does *not* creep frame to frame) */
    if(audbuf_channels>1){
      for(i=actualpos-nextsamplepos-12;i>0;i-=2){
	buffer_sample(audbuf[audbuf_head-2],track_or_process);
	buffer_sample(audbuf[audbuf_head-2],track_or_process);
	samplesmissing++;
	//fprintf(stderr,".");
      }
    }else{
      for(i=actualpos-nextsamplepos-12;i>0;i--){
	buffer_sample(audbuf[audbuf_head-1],track_or_process);	
	samplesmissing++;
	//fprintf(stderr,".");
      }
    }

    /* discard samples if we're way too far ahead; only likely to
       happen due to a fault or misuse of splicing */
    if(nextsamplepos-actualpos>12){
      /* if we're so far ahead more than 10% of the frame must
         disappear, just discard, else compact things a bit by
         dropping samples */

      fprintf(stderr,"audio sync got way ahead; this case not currently handled\n");
      exit(1);

      
    }
  }
  
  if(audbuf_rate==0)audbuf_rate=ra;
  if(audbuf_channels==0)audbuf_channels=ch;
  
  if(audbuf_rate!=ra && resampler[0].input_rate!=ra){
    /* set up resampling */
    resample_clear(&resampler[0]);
    resample_clear(&resampler[1]);
    resample_init(&resampler[0],ra,audbuf_rate,track_or_process);
    resample_init(&resampler[1],ra,audbuf_rate,track_or_process);
  }
  
  if(audbuf_rate!=ra){
    long n=length,i;
    int ileft,iright;
    double left,right;
    
    for(i=0;i<n;){
      i+=convert_input(buftemp+i,fmt,&ileft);
      samplesin++;
      left=ileft*3.0517578e-5;
      
      if(ch>1){
	i+=convert_input(buftemp+i,fmt,&iright);
	right=iright*3.0517578e-5;
      }
      
      if(audbuf_channels>1){
	if(ch>1){
	  resample_in(&resampler[0],left);
	  resample_in(&resampler[1],right);
	}else{
	  resample_in(&resampler[0],left);
	}
      }else{
	if(ch>1){
	  left+=right;
	  resample_in(&resampler[0],left*.5);
	}else{
	  resample_in(&resampler[0],left);
	}
      }
      
      /* output is always S16LE */
      while(1){
	int flag=resample_out(&resampler[0],&left);
	int sleft,sright;
	if(!flag)break;
	
	sleft=left*32767.+.5,sright;
	if(sleft>32767)sleft=32767;
	if(sleft<-32768)sleft=-32768;
	lebuffer_sample(sleft,track_or_process);
	
	if(audbuf_channels>1){
	  if(ch>1){
	    flag=resample_out(&resampler[1],&right);
	    sright=right*32767.+.5;
	    if(sright>32767)sright=32767;
	    if(sright<-32768)sright=-32768;
	    lebuffer_sample(sright,track_or_process);
	  }else{
	    lebuffer_sample(sleft,track_or_process);
	  }
	}
      }
    }
    
  }else{
    /* output is always S16LE */
    long n=length,i;
    int left,right;
    
    for(i=0;i<n;){
      samplesin++;
      i+=convert_input(buftemp+i,fmt,&left);
      if(ch>1)
	i+=convert_input(buftemp+i,fmt,&right);
      
      lebuffer_sample(left,track_or_process);
      if(audbuf_channels>1){
	if(ch>1){
	  lebuffer_sample(right,track_or_process);
	}else{
	  lebuffer_sample(left,track_or_process);
	}
      }
    }
  }
  return(1);
}

/*********************** video manipulation ***********************/

/* planar YUV12 (4:2:0) */
void yuvscale(unsigned char *src,int sw,int sh,
	      unsigned char *dst,int dw,int dh,
	      int w, int h){
  int x,y;
  int dxo=(dw-sw)/4,sxo=0;
  int dyo=(dh-sh)/4,syo=0;
  
  /* dirt simple for now. No scaling, just centering */
  memset(dst,0,dw*dh*3/2);
  
  if(dyo<0){
    syo= -dyo;
    dyo=0;
  }
  if(dxo<0){
    sxo= -dxo;
    dxo=0;
  }

  for(y=0;y<sh && y<dh;y++){
    unsigned char *sptr=src+(y+syo*2)*sw+sxo*2;
    unsigned char *dptr=dst+(y+dyo*2)*dw+dxo*2;
    for(x=0;x<sw && x<dw;x++)
      *dptr++=*sptr++;
  }

  src+=sw*sh;
  dst+=dw*dh;
  sw/=2;
  dw/=2;
  sh/=2;
  dh/=2;

  for(y=0;y<sh && y<dh;y++){
    unsigned char *sptr=src+(y+syo)*sw+sxo;
    unsigned char *dptr=dst+(y+dyo)*dw+dxo;
    for(x=0;x<sw && x<dw;x++)
      *dptr++=*sptr++;
  }

  src+=sw*sh;
  dst+=dw*dh;

  for(y=0;y<sh && y<dh;y++){
    unsigned char *sptr=src+(y+syo)*sw+sxo;
    unsigned char *dptr=dst+(y+dyo)*dw+dxo;
    for(x=0;x<sw && x<dw;x++)
      *dptr++=*sptr++;
  }

}

void rgbscale(unsigned char *rgb,int sw,int sh,
	      unsigned char *dst,int dw,int dh,
	      unsigned int w, int h){
  int ih=sh/2*2;
  int iw=sw/2*2;

  unsigned char *y=alloca(ih*iw*3/2);
  unsigned char *u=y+ih*iw;
  unsigned char *v=u+ih*iw/4;

  int every=0,other=sw*3,c4=0,i,j;
  unsigned char *ye=y,*yo=y+iw;

  for(i=0;i<ih;i+=2){
    for(j=0;j<iw;j+=2){
      long yval,uval,vval;
      
      yval =   rgb[every]*19595 + rgb[every+1]*38470 + rgb[every+2]*7471;
      uval = rgb[every+2]*65536 -   rgb[every]*22117 - rgb[every+1]*43419;
      vval =   rgb[every]*65536 - rgb[every+1]*54878 - rgb[every+2]*10658;
      *ye++ =yval>>16;
      every+=3;
      yval =   rgb[every]*19595 + rgb[every+1]*38470 + rgb[every+2]*7471;
      uval+= rgb[every+2]*65536 -   rgb[every]*22117 - rgb[every+1]*43419;
      vval+=   rgb[every]*65536 - rgb[every+1]*54878 - rgb[every+2]*10658;
      *ye++ =yval>>16;
      every+=3;
      
      yval =   rgb[other]*19595 + rgb[other+1]*38470 + rgb[other+2]*7471;
      uval = rgb[other+2]*65536 -   rgb[other]*22117 - rgb[other+1]*43419;
      vval =   rgb[other]*65536 - rgb[other+1]*54878 - rgb[other+2]*10658;
      *yo++ =yval>>16;
      other+=3;
      yval =   rgb[other]*19595 + rgb[other+1]*38470 + rgb[other+2]*7471;
      uval+= rgb[other+2]*65536 -   rgb[other]*22117 - rgb[other+1]*43419;
      vval+=   rgb[other]*65536 - rgb[other+1]*54878 - rgb[other+2]*10658;
      *yo++ =yval>>16;
      other+=3;
      
      u[c4]  =(uval>>19)+128;
      v[c4++]=(vval>>19)+128;
      
    }
    ye+=iw;
    yo+=iw;
    every+=sw*3 + sw%2*3;
    other+=sw*3 + sw%2*3;
    
  }
  
  yuvscale(y,iw,ih,dst,dw,dh,w,h);

}


unsigned char     **vidbuf;
long long *vidbuf_frameno;
double     vidbuf_zerotime;
long long  vidbuf_frames;
int        vidbuf_head;
int        vidbuf_tail;
int        vidbuf_size;
int        vidbuf_height;
int        vidbuf_width;
double     vidbuf_fps;
			 
int scale_width;
int scale_height;

long long framesin=0;
long long framesout=0;
long long framesmissing=0;
long long framesdiscarded=0;
      
static int process_video_frame(char *buffer,FILE *f,int notfakep,int yuvp){
  char *s=buffer+6;
  long a=atoi(s),b,w,h,length;
  double t;
  int ret;

  s=strchr(s,' ');
  if(!s)return(0);
  b=atoi(s);
  t=a+b*.000001;
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  w=atoi(s);
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  h=atoi(s);
  
  s=strchr(s+1,' ');
  if(!s)return(0);
  length=atoi(s);

  if(length>buftempsize){
    if(buftemp)
      buftemp=realloc(buftemp,length*sizeof(*buftemp));
    else
      buftemp=malloc(length*sizeof(*buftemp));
    buftempsize=length;
  }
  if(notfakep){
    ret=fread(buftemp,1,length,f);
    if(ret<length)return(0);
  }else{
    ret=fseek(f,length,SEEK_CUR);
    if(ret)return(0);
  }

  if(global_zerotime==0){
    global_zerotime=t;
    begin_time+=t;
    end_time+=t;
  }

  if(t<begin_time)return(1);
  if(t>end_time)return(1);

  /* video sync is fundamentally different from audio. We assume that
     frames never appear early; an frame that seems early in context
     of the previous frame is due backlogged framebuffer catching up
     and this frame is actually late, as are however many 'on time'
     frames behind it. */
  if(vidbuf_zerotime==0.){
    vidbuf_zerotime=t;
  }else{
    double ideal=(double)vidbuf_frames;
    double actual=(t-vidbuf_zerotime)*vidbuf_fps;
    double drift=actual-ideal;
    int i;

    /* intentional range for hysteresis */
    if(drift<.5){
      /* 'early' frame; bump the whole train back if possible, 
	 else discard */
      if(vidbuf_head-vidbuf_tail < 
	 vidbuf_frameno[vidbuf_head-1]+1-
	 vidbuf_frameno[vidbuf_tail]){
	
	/* yes, there's a hole. look for it */
	
	vidbuf_frameno[vidbuf_head-1]--;
	for(i=vidbuf_head-1;i>vidbuf_tail;i--){
	  if(vidbuf_frameno[i]==vidbuf_frameno[i-1])
	    vidbuf_frameno[i-1]--;
	  else
	    break;
	}
	vidbuf_frames=ideal-1;
	
      }else{

	/* no room to bump back.  Discard the 'early' frame 
	   in order to reclaim sync, even if destructively. */
	framesdiscarded++;
	return(1);
      }
    }

    if(drift>1.){
      /* 'late' frame.  Skip the counter ahead.  Don't
	 carry forward through the gap yet. */
      vidbuf_frames=rint(actual);
    }
  }

  if(vidbuf_width==0)vidbuf_width=(w>>1)<<1;
  if(vidbuf_height==0)vidbuf_height=(h>>1)<<1;

  /* get a buffer */
  if(vidbuf_head>=vidbuf_size){
    if(vidbuf_tail){

      while(vidbuf_tail){
	unsigned char *temp;
	
	vidbuf_head--;
	vidbuf_tail--;
	if(notfakep){
	  temp=vidbuf[0];
	  memmove(vidbuf,vidbuf+1,(vidbuf_size-1)*sizeof(*vidbuf));
	  vidbuf[vidbuf_size-1]=temp;
	}
	memmove(vidbuf_frameno,vidbuf_frameno+1,
		(vidbuf_size-1)*sizeof(*vidbuf_frameno));
      }
    }else{
      if(vidbuf){
	vidbuf_size++;
	if(notfakep)vidbuf=realloc(vidbuf,vidbuf_size*sizeof(*vidbuf));
	vidbuf_frameno=realloc(vidbuf_frameno,vidbuf_size*sizeof(*vidbuf_frameno));
      }else{
	vidbuf_size=1;
	vidbuf=malloc(vidbuf_size*sizeof(*vidbuf));
	vidbuf_frameno=malloc(vidbuf_size*sizeof(*vidbuf_frameno));
      }
      if(notfakep)
	vidbuf[vidbuf_size-1]=malloc(vidbuf_width*vidbuf_height*3/2);
    }
  }
  vidbuf_frameno[vidbuf_head]=vidbuf_frames;
  vidbuf_frames++;
  
  /* scale image into buffer */
  if(notfakep){
    if(yuvp)
      yuvscale(buftemp,w,h,vidbuf[vidbuf_head],vidbuf_width,vidbuf_height,
	       scale_width,scale_height);
    else
      rgbscale(buftemp,w,h,vidbuf[vidbuf_head],vidbuf_width,vidbuf_height,
	       scale_width,scale_height);
  }
  /* finally any needed invasive blanking */

  vidbuf_head++;
  return(1);
}

static int read_snatch_frame(FILE *f,int wa,int wv){
  char buffer[130];
  char *ptr=buffer;
  while(1){
    int c=getc(f);
    if(c==EOF)return(0);
    *ptr++=c;
    if(c==':')break;
    if(ptr>=buffer+130)return(0);
  }
  *ptr='\0';

  if(!strncmp(buffer,"AUDIO",5)){
    /* buffer or just track the audio, doing automatic resampling to
       keep the same parameters start to end. */
    return process_audio_frame(buffer, f, wa);

  }else {
    if(!strncmp(buffer,"VIDEO",5)){
      framesin++;
      return process_video_frame(buffer, f, wv,0);

    }else{
      if(!strncmp(buffer,"YUV12",5)){
	framesin++;
	return process_video_frame(buffer, f, wv,1);
      }else{
	
	fprintf(stderr,"Garbage/unknown frame type\n");
	return(0);
      }
    }
  }
}

/* writes a wav header without the length set.  This is also the 32
   bit WAV header variety... please please please let the mjpeg tools
   ignore the length... */
void PutNumLE(long num,FILE *f,int bytes){
  int i=0;
  while(bytes--){
    if(fputc((num>>(i<<3))&0xff,f)==EOF){
      fprintf(stderr,"Unable to write output: %s\n",strerror(ferror(f)));
      exit(1);
    }
    i++;
  }
}
void WriteWav(FILE *f,long channels,long rate,long bits){
  fprintf(f,"RIFF");
  PutNumLE(0x7fffffffUL,f,4);
  fprintf(f,"WAVEfmt ");
  PutNumLE(16,f,4);
  PutNumLE(1,f,2);
  PutNumLE(channels,f,2);
  PutNumLE(rate,f,4);
  PutNumLE(rate*channels*((bits-1)/8+1),f,4);
  PutNumLE(((bits-1)/8+1)*channels,f,2);
  PutNumLE(bits,f,2);
  fprintf(f,"data");
  PutNumLE(0x7fffffffUL,f,4);
}

void WriteYuv(FILE *f,int w,int h,int fpscode){
  fprintf(f,"YUV4MPEG %d %d %d\n",w,h,fpscode);
}

/* YV12 aka 4:2:0 planar */
void YUVout(unsigned char *buf,FILE *f){
  fprintf(f,"FRAME\n");
  fwrite(buf,1,vidbuf_width*vidbuf_height*3/2,f);
}

static int synced;
static int drain;
static int header;
int ratecode;
int video_timeahead;

int snatch_iterator(FILE *in,FILE *out,int process_audio,int process_video){
  if(!header){
    if(read_snatch_header(in)){

      if(process_audio && !audio_p){
	fprintf(stderr,"No audio in this stream\n");
	return(1);
      }
      if(process_video && !video_p){
	fprintf(stderr,"No video in this stream\n");
	return(1);
      }

      header=1;
      return(0);
    }
    return(1);
  }

  if(!drain){
    int ret=read_snatch_frame(in,process_audio,process_video);
    
    if(ret==0 && feof(in))drain=1;
  }
  
  if(audio_p && video_p){
    if(!synced){
      /* pad beginning of stream for sync */
      if(vidbuf_head-vidbuf_tail && audbuf_head-audbuf_tail){
	double time_dif=vidbuf_zerotime-audbuf_zerotime;
	long frames=0;
	long samples=0;
	int i;

	if(process_audio)
	  WriteWav(out,audbuf_channels,audbuf_rate,16);
	if(process_video)
	  WriteYuv(out,vidbuf_width,vidbuf_height,ratecode);
	
	/* we don't write frames/samples here; we queue new ones out
           ahead until everything's even */

	if(time_dif>0){
	  /* audio started first; prestretch video, then repad with
	     audio */
	  frames=ceil(time_dif*vidbuf_fps);
	  time_dif-=(frames/vidbuf_fps);
	  vidbuf_zerotime-=(frames/vidbuf_fps);
	  for(i=vidbuf_tail+1;i<vidbuf_head;i++)
	    vidbuf_frameno[i]+=frames;
	  framesmissing+=frames;
	}

	samples= -time_dif*audbuf_rate;
	samplesmissing+=samples;
	
	pre_buffer_audio(samples*audbuf_channels,process_audio);
	audbuf_zerotime=vidbuf_zerotime;

	synced=1;
      }else{
	if(drain){
	  /* that was short; either no vid or no audio */
	  if(vidbuf_head-vidbuf_tail==0)
	    fprintf(stderr,"Audio/Video stream contained no video.\n");
	  if(audbuf_head-audbuf_tail==0)
	    fprintf(stderr,"Audio/Video stream contained no audio.\n");
	  return 1;
	}
      }
      return 0;
    }
  }    

  if(drain){
    int i;
    
    /* write out all pending audio/video */
    
    if(video_p){
      for(i=vidbuf_tail;i<vidbuf_head;i++){
	int frames=(i+1<vidbuf_head)?
	  vidbuf_frameno[i+1]-vidbuf_frameno[i]:
	  1;
	
	framesout+=frames;
	framesmissing+=(frames-1);
	
	if(process_video)
	  while(frames--)
	    YUVout(vidbuf[i],out);
      }
    }
    
    if(audio_p){
      long samples=audbuf_head-audbuf_tail;
      samplesout+=samples/audbuf_channels;
      if(process_audio)
	fwrite(audbuf+audbuf_tail,2,samples,out);
    }
    return 1;
  }
  
  if(video_p && process_video){
    while(vidbuf_head-vidbuf_tail>video_timeahead){
      int frames= vidbuf_frameno[vidbuf_tail+1]-
	vidbuf_frameno[vidbuf_tail];
      framesout+=frames;
      framesmissing+=(frames-1);
      
      if(process_video)
	while(frames--)
	  YUVout(vidbuf[vidbuf_tail],out);
      
      vidbuf_tail++;
    }
  }
  
  if(audio_p && process_audio){
    if(audbuf_head-audbuf_tail){
      long samples=audbuf_head-audbuf_tail;
      if(process_audio)
	fwrite(audbuf+audbuf_tail,2,samples,out);
      samplesout+=samples/audbuf_channels;
      audbuf_tail+=samples;
    }
  }

  return(0);
}
  



