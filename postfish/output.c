/*
 *
 *  postfish
 *    
 *      Copyright (C) 2002-2003 Monty
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

/* sound playback code is OSS-specific for now */
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include "postfish.h"
#include "input.h"

sig_atomic_t playback_active=0;
sig_atomic_t playback_exit=0;

typedef struct output_feedback{
  double *rms;
  double *peak;

  struct output_feedback *next;
} output_feedback;

static output_feedback *feedback_list_head;
static output_feedback *feedback_list_tail;
static output_feedback *feedback_pool;

static output_feedback *new_output_feedback(void){
  output_feedback *ret;
  
  pthread_mutex_lock(&master_mutex);
  if(feedback_pool){
    ret=feedback_pool;
    feedback_pool=feedback_pool->next;
    pthread_mutex_unlock(&master_mutex);
    return ret;
  }
  pthread_mutex_unlock(&master_mutex);
  ret=malloc(sizeof(*ret));
  ret->rms=malloc((input_ch+2)*sizeof(*ret->rms));
  ret->peak=malloc((input_ch+2)*sizeof(*ret->peak));
  
  return ret;
}

static void push_output_feedback(double *peak,double *rms){
  int i,n=input_ch+2;
  output_feedback *f=new_output_feedback();

  memcpy(f->rms,rms,n*sizeof(*rms));
  memcpy(f->peak,peak,n*sizeof(*peak));
  f->next=NULL;

  pthread_mutex_lock(&master_mutex);
  if(!feedback_list_tail){
    feedback_list_tail=f;
    feedback_list_head=f;
  }else{
    feedback_list_head->next=f;
    feedback_list_head=f;
  }
  pthread_mutex_unlock(&master_mutex);
}

int pull_output_feedback(double *peak,double *rms,int *n){
  output_feedback *f;
  int i,j;
  *n=input_ch+2;

  pthread_mutex_lock(&master_mutex);
  if(feedback_list_tail){
    
    f=feedback_list_tail;
    feedback_list_tail=feedback_list_tail->next;
    if(!feedback_list_tail)feedback_list_head=0;

  }else{
    pthread_mutex_unlock(&master_mutex);
    return 0;
  }
  pthread_mutex_unlock(&master_mutex);

  memcpy(rms,f->rms,sizeof(*rms)* *n);
  memcpy(peak,f->peak,sizeof(*peak)* *n);

  pthread_mutex_lock(&master_mutex);
  f->next=feedback_pool;
  feedback_pool=f;
  pthread_mutex_unlock(&master_mutex);
  return 1;
}


static void PutNumLE(long num,FILE *f,int bytes){
  int i=0;
  while(bytes--){
    fputc((num>>(i<<3))&0xff,f);
    i++;
  }
}

static void WriteWav(FILE *f,long channels,long rate,long bits,long duration){
  if(ftell(f)>0)
    if(fseek(f,0,SEEK_SET))
      return;
  fprintf(f,"RIFF");
  PutNumLE(duration+44-8,f,4);
  fprintf(f,"WAVEfmt ");
  PutNumLE(16,f,4);
  PutNumLE(1,f,2);
  PutNumLE(channels,f,2);
  PutNumLE(rate,f,4);
  PutNumLE(rate*channels*((bits-1)/8+1),f,4);
  PutNumLE(((bits-1)/8+1)*channels,f,2);
  PutNumLE(bits,f,2);
  fprintf(f,"data");
  PutNumLE(duration,f,4);
}

static int isachr(FILE *f){
  struct stat s;

  if(!fstat(fileno(f),&s))
    if(S_ISCHR(s.st_mode)) return 1;
  return 0;
}

static FILE *playback_startup(int outfileno, int ch, int r){
  FILE *playback_fd=NULL;
  int format=AFMT_S16_NE;
  int rate=r,channels=ch,ret;

  if(outfileno==-1){
    playback_fd=fopen("/dev/dsp","wb");
  }else{
    playback_fd=fdopen(dup(outfileno),"wb");
  }

  if(!playback_fd){
    fprintf(stderr,"Unable to open output for playback\n");
    return NULL;
  }

  /* is this file a block device? */
  if(isachr(playback_fd)){
    int fragment=0x0004000d;
    int fd=fileno(playback_fd);

    /* try to lower the DSP delay; this ioctl may fail gracefully */
    ret=ioctl(fd,SNDCTL_DSP_SETFRAGMENT,&fragment);
    if(ret){
      fprintf(stderr,"Could not set DSP fragment size; continuing.\n");
    }

    ret=ioctl(fd,SNDCTL_DSP_SETFMT,&format);
    if(ret || format!=AFMT_S16_NE){
      fprintf(stderr,"Could not set AFMT_S16_NE playback\n");
      exit(1);
    }
    ret=ioctl(fd,SNDCTL_DSP_CHANNELS,&channels);
    if(ret || channels!=ch){
      fprintf(stderr,"Could not set %d channel playback\n",ch);
      exit(1);
    }
    ret=ioctl(fd,SNDCTL_DSP_SPEED,&rate);
    if(ret || r!=rate){
      fprintf(stderr,"Could not set %dHz playback\n",r);
      exit(1);
    }
  }else{
    WriteWav(playback_fd,ch,r,16,-1);
  }

  return playback_fd;
}

/* playback must be halted to change blocksize. */
void *playback_thread(void *dummy){
  int audiobufsize=8192,i,j,k;
  unsigned char *audiobuf=malloc(audiobufsize);
  int bigendianp=(AFMT_S16_NE==AFMT_S16_BE?1:0);
  FILE *playback_fd=NULL;
  int setupp=0;
  time_linkage *ret;
  off_t count=0;
  long last=-1;

  int ch=-1;
  long rate=-1;

  /* for output feedback */
  double *rms=alloca(sizeof(*rms)*(input_ch+2));
  double *peak=alloca(sizeof(*peak)*(input_ch+2));

  while(1){
    if(playback_exit)break;

    /* get data */
    if(!(ret=input_read()))break;

    /************/



    /* temporary; this would be frequency domain in the finished postfish */
    if(ret && ret->samples>0){
      double scale=fromdB(master_att/10.);
      for(i=0;i<ret->samples;i++)
	for(j=0;j<ret->channels;j++)
	  ret->data[j][i]*=scale;
    }    


    /************/

    if(ret && ret->samples>0){
      memset(rms,0,sizeof(*rms)*(input_ch+2));
      memset(peak,0,sizeof(*peak)*(input_ch+2));
      ch=ret->channels;
      rate=ret->rate;

      /* lazy playbak setup; we couldn't do it until we had rate and
	 channel information from the pipeline */
      if(!setupp){
	playback_fd=playback_startup(outfileno,ch,rate);
	if(!playback_fd){
	  playback_active=0;
	  playback_exit=0;
	  return NULL;
	}
	setupp=1;
      }

      if(audiobufsize<ret->channels*ret->samples*2){
	audiobufsize=ret->channels*ret->samples*2;
	audiobuf=realloc(audiobuf,sizeof(*audiobuf)*audiobufsize);
      }
      
      /* final limiting and conversion */
      
      for(k=0,i=0;i<ret->samples;i++){
	double mean=0.;
	double div=0.;
	double divrms=0.;

	for(j=0;j<ret->channels;j++){
	  double dval=ret->data[j][i];
	  int val=rint(dval*32767.);
	  if(val>32767)val=32767;
	  if(val<-32768)val=-32768;
	  if(bigendianp){
	    audiobuf[k++]=val>>8;
	    audiobuf[k++]=val;
	  }else{
	    audiobuf[k++]=val;
	    audiobuf[k++]=val>>8;
	  }

	  if(fabs(dval)>peak[j])peak[j]=fabs(dval);
	  rms[j]+= dval*dval;
	  mean+=dval;

	}

	/* mean */
	mean/=j;
	if(fabs(mean)>peak[input_ch])peak[input_ch]=fabs(mean);
	rms[input_ch]+= mean*mean;

	/* div */
	for(j=0;j<ret->channels;j++){
	  double dval=mean-ret->data[j][i];
	  if(fabs(dval)>peak[input_ch+1])peak[input_ch+1]=fabs(dval);
	  divrms+=dval*dval;
	}
	rms[input_ch+1]+=divrms/ret->channels;

      }

      for(j=0;j<input_ch+2;j++){
	rms[j]/=ret->samples;
	rms[j]=sqrt(rms[j]);
      }
      
      count+=fwrite(audiobuf,1,ret->channels*ret->samples*2,playback_fd);

      /* inform Lord Vader his shuttle is ready */
      push_output_feedback(peak,rms);
      write(eventpipe[1],"",1);

    }else
      break; /* eof */
  }

  if(playback_fd){
    if(isachr(playback_fd)){
      int fd=fileno(playback_fd);
      ioctl(fd,SNDCTL_DSP_RESET);
    }else{
      if(ch>-1)
	WriteWav(playback_fd,ch,rate,16,count);
    } 
    fclose(playback_fd);
  }
  playback_active=0;
  playback_exit=0;
  if(audiobuf)free(audiobuf);
  write(eventpipe[1],"",1);
  return(NULL);
}

