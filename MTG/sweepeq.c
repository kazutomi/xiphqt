#define _REENTRANT 1
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

static int original_priority;
static int original_policy;

#define REC_SAMPLE_BYTES 2
#define REC_SAMPLE_FMT AFMT_S16_LE
#define REC_SAMPLE_CH 2

int main (){
  /* sound device startup */
  FILE *playfd=fopen("/dev/dsp1","rb");
  FILE *recfd=fopen("/dev/dsp1","wb");
  audio_buf_info info;
  int playfdi=fileno(playfd),i;
  int recfdi=fileno(recfd);
  int format=AFMT_S16_NE;
  int channels=MAX_OUTPUT_CHANNELS;
  int rate=44100;
  long last=0;
  long delay=10;
  long totalsize;
  int fragment=0x7fff000d;
  int16 audiobuf[256*MAX_OUTPUT_CHANNELS];
  int ret;

  if(!playfd || !recfd){
    fprintf(stderr,"Could not open sound device.\n");
    exit(1);
  }

  /* realtime schedule setup */
  {
    struct sched_param param;
    param.sched_priority=89;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)){
      fprintf(stderr,"Could not set realtime priority for playback; am I suid root?\n");
      exit(1);
    }
  }

  ioctl(playfdi,SNDCTL_DSP_SETFRAGMENT,&fragment);
  ret=ioctl(playfdi,SNDCTL_DSP_SETFMT,&format);
  if(ret || format!=AFMT_S16_NE){
    fprintf(stderr,"Could not set AFMT_S16_NE playback\n");
    exit(1);
  }
  ret=ioctl(playfdi,SNDCTL_DSP_CHANNELS,&channels);
  if(ret || channels!=MAX_OUTPUT_CHANNELS){
    fprintf(stderr,"Could not set %d channel playback\n",MAX_OUTPUT_CHANNELS);
    exit(1);
  }

  ret=ioctl(playfdi,SNDCTL_DSP_SPEED,&rate);
  if(ret || rate!=44100){
    fprintf(stderr,"Could not set %dHz playback\n",44100);
    exit(1);
  }



  ret=ioctl(recfdi,SNDCTL_DSP_SETFMT,&format);
  if(ret || format!=REC_SAMPLE_FMT){
    fprintf(stderr,"Could not set recording format\n");
    exit(1);
  }
  ret=ioctl(recfdi,SNDCTL_DSP_CHANNELS,&channels);
  if(ret || channels!=2){
    fprintf(stderr,"Could not set %d channel recording\n",2);
    exit(1);
  }
  ret=ioctl(recfdi,SNDCTL_DSP_SPEED,&rate);
  if(ret || rate!=44100){
    fprintf(stderr,"Could not set %dHz recording\n",44100);
    exit(1);
  }


  



    ret=fread(recordbuffer+record_head,1,REC_BLOCK,recfd);

    for(i=record_head;i<record_head+REC_BLOCK;)
      for(j=0;j<REC_SAMPLE_CH;j++){
	int val=((recordbuffer[i]<<8)|(recordbuffer[i+1]<<16)|(recordbuffer[i+2]<<24))>>8;
	//int val=((recordbuffer[i]<<16)|(recordbuffer[i+1]<<24))>>8;
	if(labs(val)>rchannel_list[j].peak)
	  rchannel_list[j].peak=labs(val);
	i+=REC_SAMPLE_BYTES;
      }
    if(rec_exit)break;

    if(rec_flush_req){

      pthread_mutex_lock(&rec_buffer_mutex);
      record_head+=REC_BLOCK;
      if((unsigned)record_head>=sizeof(recordbuffer))record_head=0;
      record_count+=REC_BLOCK;
      pthread_cond_signal(&rec_buffer_cond);
      pthread_mutex_unlock(&rec_buffer_mutex);
    }
  }

  rec_active1=0;
  fprintf(stderr,"Record thread exit...\n");
  pthread_mutex_unlock(&rec_mutex);
  
  return(NULL);
}
