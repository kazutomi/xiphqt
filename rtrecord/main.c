/*
 *
 *  rtrecord
 *    
 *      Copyright (C) 2006 Red Hat Inc.
 *
 *  rtrecord is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  rtrecord is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with rtrecord; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */


#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>

#include "rtrecord.h"

int channels=2;
int rate=44100;
int width=16;
char *device="hw:0,0";
snd_pcm_t *devhandle=NULL;
int weight=0;
int format = SND_PCM_FORMAT_S16_LE;
int samplesize = 2;
int framesize = 0;
int quiet = 0;
int ttyfd;
int ttypipe[2];
int exiting = 0;
int paused = 0;
int realtime=1;
int lockbuffer=1;
float smooth = .5;

pthread_t main_thread_id;
pthread_t record_thread_id;
pthread_t disk_thread_id;
pthread_t tty_thread_id;
pthread_t ui_thread_id;
sem_t ui_sem;
sem_t buffer_sem;
sem_t setup_sem;

unsigned char *diskbuffer=0;
sig_atomic_t diskbuffer_head=0;
sig_atomic_t diskbuffer_tail=0;

int diskbuffer_size;
int diskbuffer_frames;

char *out_name=0;
FILE *out_FILE=0;

atomic_mark dma_mark = {0,0};
atomic_mark disk_mark = {0,0};

long dmabuffer_frames = 0;
buffer_meta dma_data[2] = {
  {0,0,0,0},
  {0,0,0,0},
};
channel_meta *pcm_data[2];

/* used to redirect the real tty input into the pipe that ncurses
   thinks is the tty */
void *tty_thread(void *dummy){
  char buf;

  while(!exiting){
    int ret=read(ttyfd,&buf,1);
    if(ret==1){
      write(ttypipe[1],&buf,1);
    }
  }
  return NULL;
}

/* used to request a ui update (vi the ui sempahore) such that there
   is absolutely no possibility of blocking if the other UI code is
   otherwise unresponsive; just writing to the ttypipe could backfire
   if the pipe fills  */
void *ui_thread(void *dummy){
  while(!exiting){
    write(ttypipe[1],"",1);
    sem_wait(&ui_sem);
  }
  return NULL;
}

static void record_setup(void){
  snd_pcm_hw_params_t *hw;
  snd_pcm_uframes_t frames;
  int ret;

  if ((ret = snd_pcm_hw_params_malloc (&hw)) < 0) {
    fprintf (stderr, "capture cannot allocate hardware parameter structure (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
  if ((ret = snd_pcm_hw_params_any (devhandle, hw)) < 0) {
    fprintf (stderr, "capture cannot initialize hardware parameter structure (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
    
  if ((ret = snd_pcm_hw_params_set_access (devhandle, hw, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "capture cannot set access type (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
  if ((ret = snd_pcm_hw_params_set_format (devhandle, hw, format)) < 0) {
    fprintf (stderr, "capture cannot set sample format (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
  if ((ret = snd_pcm_hw_params_set_rate (devhandle, hw, rate, 0)) < 0) {
    fprintf (stderr, "capture cannot set sample rate (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
  if ((ret = snd_pcm_hw_params_set_channels (devhandle, hw, channels)) < 0) {
    fprintf (stderr, "capture cannot set channel count (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
  if ((ret = snd_pcm_hw_params (devhandle, hw)) < 0) {
    fprintf (stderr, "capture cannot set parameters (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
  {
    if((ret = snd_pcm_hw_params_get_buffer_size(hw,&frames)) < 0){
      fprintf (stderr, "capture cannot query buffer size (%s)\n",
	       snd_strerror (ret));
      exit (1);
    }
    
    dmabuffer_frames = frames;
  }
  
  snd_pcm_hw_params_free (hw);
  
  if ((ret = snd_pcm_prepare (devhandle)) < 0) {
    fprintf (stderr, "capture cannot prepare audio interface for use (%s)\n",
	     snd_strerror (ret));
    exit (1);
  }
  
}

/* realtime 'pounce' thread that grabs data from the audio device (and
   the little bitty hardware buffer) and stuffs the data, for the time
   being, into a relatively huge ringbuffer that the disk thread can
   service at its leisure */
void *record_thread(void *dummy){
  /* sound device startup */
  int ret;
  snd_pcm_sframes_t sframes;
  int fps16 = (rate>>4);

  if(realtime){
    struct sched_param param;
    param.sched_priority=89;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)){
      fprintf(stderr,"\nERROR: Could not set realtime priority for capture thread.\n\n");
      fprintf(stderr,
	      "To run without realtime scheduling, use the --no-realtime or -n option, or\n"
	      "consider running 'record', equivalent to 'rtrecord --no-lock --no-realtime'\n"
	      "Be warned that recording without realtime scheduling may be prone to skips\n"
	      "and pops depending on machine load and specific hardware; using realtime\n"
	      "is strongly suggested for any mission critical recording.\n\n"
	      "To run with realtime scheduling as a non-root user, make the rtrecord\n"
	      "executable setuid root.\n\n");
      exit(1);
    }
  }

  sem_post(&setup_sem);

  while(!exiting){
    int frames = 0;
    int head = diskbuffer_head; 

    while(frames<fps16){  
      int to_read = diskbuffer_frames - head;
      if(to_read > fps16-frames) to_read = fps16 - frames;

      /* update high water mark of dma buffer */
      if(snd_pcm_delay (devhandle, &sframes)==0){
	int watermark = dmabuffer_frames-sframes;
	if(dma_data[dma_mark.bank].dmabuffer_min>watermark)
	  dma_data[dma_mark.bank].dmabuffer_min=watermark;
      }
      
      /* read -- we always have space due to invariant */
      ret = snd_pcm_readi (devhandle, diskbuffer + head*framesize, 
			   to_read);
      
      if(ret<0){
	// most likely an underrun
	switch(ret){
     	case -EAGAIN:
	  break;
	case -EPIPE: case -ESTRPIPE:
	  // underrun; set starve and reset soft device
	  dma_data[dma_mark.bank].dmabuffer_overrun=1;
	  snd_pcm_drop(devhandle);
	  snd_pcm_prepare(devhandle);
	  frames=0; 
	  
	  break;
	case -EBADFD:
	default:
	  // Something went very wrong. Don't hose the machine here in a realtime thread
	  frames = fps16;
	  exiting=1;
	  fprintf (stderr, "unrecoverable capture read error: (%s)\n",
		   snd_strerror (ret));
	  break;
	}
      }else{
	frames+=ret;
	head+=ret;
	if(head>=diskbuffer_frames) head = 0;
      }
    }
    
    if(!exiting){
      /* advance the head index, but only if there will be enough
	 space left to write the next frame.  if there isn't enough
	 space left, leave the head as-is and set 'underrun'.  This
	 wastes a small amount of buffer space but eliminates the
	 need for the sample thread to conditionally block on the disk
	 thread. */
      int tail = diskbuffer_tail;
      int remaining = diskbuffer_frames - (tail <= head ? 
					   (head - tail):
					   (head + diskbuffer_frames - tail));
      
      if(remaining >= fps16){
	
	/* explicit invariant: this is always full_frame aligned
	   (equal numbers of frames in each channel) */

	if(dma_data[dma_mark.bank].diskbuffer_min>remaining)
	  dma_data[dma_mark.bank].diskbuffer_min=remaining;
	if(dma_mark.read == dma_mark.bank){
	  dma_mark.bank = !dma_mark.bank;

	  /* initialize fresh bank */
	  dma_data[dma_mark.bank].diskbuffer_min = diskbuffer_frames;
	  dma_data[dma_mark.bank].dmabuffer_min = dmabuffer_frames;
	  dma_data[dma_mark.bank].diskbuffer_overrun=0;
	  dma_data[dma_mark.bank].dmabuffer_overrun=0;

	}

	diskbuffer_head = head;
	sem_post(&buffer_sem);
	
      }else{
	/* not enough space for next frame; discard what we just read
	   and mark underrun */
	dma_data[dma_mark.bank].diskbuffer_overrun=1;
	dma_data[dma_mark.bank].diskbuffer_min=0;
	if(dma_mark.read == dma_mark.bank){
	  dma_mark.bank = !dma_mark.bank;

	  /* initialize fresh bank */
	  dma_data[dma_mark.bank].diskbuffer_min = diskbuffer_frames;
	  dma_data[dma_mark.bank].dmabuffer_min = dmabuffer_frames;
	  dma_data[dma_mark.bank].diskbuffer_overrun=0;
	  dma_data[dma_mark.bank].dmabuffer_overrun=0;

	}
      }
    }
  }
  
  snd_pcm_close(devhandle);
  return(NULL);
}

void PutNumLE(long num,FILE *f,int bytes){
  int i=0;
  while(bytes--){
    fputc((num>>(i<<3))&0xff,f);
    i++;
  }
}

/* mutant WAV header with undeclared chunklength */
void wav_header(FILE *f,long channels,long rate,long bits){
  fseek(f,0,SEEK_SET);
  fprintf(f,"RIFF");
  PutNumLE(0xffffffff,f,4);
  fprintf(f,"WAVEfmt ");
  PutNumLE(16,f,4);
  PutNumLE(1,f,2);
  PutNumLE(channels,f,2);
  PutNumLE(rate,f,4);
  PutNumLE(rate*channels*((bits-1)/8+1),f,4);
  PutNumLE(((bits-1)/8+1)*channels,f,2);
  PutNumLE(bits,f,2);
  fprintf(f,"data");
  PutNumLE(0xffffffff,f,4);
}

float aweight_w1=0.f;
float aweight_w2=0.f;
float aweight_w3=0.f;
float aweight_w4=0.f;
float aweight_g=0.f;

typedef struct {

  float _z1a; 
  float _z1b; 
  float _z2; 
  float _z3; 
  float _z4a;
  float _z4b;

} afilter;

/* A-weighting code borrowed from LADSPA A-weighting plugin */
afilter *aweight_filter=0;

#define AW_F1 20.5990
#define AW_F2 107.652
#define AW_F3 737.862
#define AW_F4 12194.2

void aweight_init (int rate, int channels){
  double f;
  
  aweight_filter = calloc(channels, sizeof(*aweight_filter));
 
  switch (rate) {
  case 44100: 
    aweight_w4 = 0.846; 
    break;
  case 48000: 
    aweight_w4 = 0.817; 
    break;
  case 88200: 
    aweight_w4 = 0.587; 
    break;
  case 96000: 
    aweight_w4 = 0.555; 
    break;
  default: 
    fprintf(stderr,"A-weighting only available with following sample-rates:\n"
	    "  44100\n"
	    "  48000\n"
	    "  88200\n"
	    "  96000\n\n");
    exit(1);
  }

  aweight_g = 1.2502f;

  f = AW_F1 / rate;
  aweight_w1 = 2 * M_PI * f;
  aweight_g *= 2 / (2 - aweight_w1);
  aweight_g *= 2 / (2 - aweight_w1); // twice !!
  aweight_w1 *= 1 - 3 * f;

  f = AW_F2 / rate;
  aweight_w2 = 2 * M_PI * f;
  aweight_g *= 2 / (2 - aweight_w2);
  aweight_w2 *= 1 - 3 * f;

  f = AW_F3 / rate;
  aweight_w3 = 2 * M_PI * f;
  aweight_g *= 2 / (2 - aweight_w3);
  aweight_w3 *= 1 - 3 * f;

}

float aweight_process (float x, afilter *f){

  // highpass sections
  f->_z1a += aweight_w1 * (x - f->_z1a + 1e-40f); 
  x -= f->_z1a;
  f->_z1b += aweight_w1 * (x - f->_z1b + 1e-40f); 
  x -= f->_z1b;
  f->_z2 += aweight_w2 * (x - f->_z2 + 1e-40f); 
  x -= f->_z2;
  f->_z3 += aweight_w3 * (x - f->_z3 + 1e-40f); 
  x -= f->_z3;
  
  // lowpass sections
  f->_z4a += aweight_w4 * (x - f->_z4a);
  x  = 0.25 * f->_z4b;
  f->_z4b += aweight_w4 * (f->_z4a - f->_z4b);
  x += 0.75 * f->_z4b;
  
  return aweight_g * x;
}

void *disk_thread(void *dummy){
  
  /* write out a 'streaming' WAV header */
  if(out_FILE)
    wav_header(out_FILE, channels, rate, width);

  /* become consumer to the record thread's producer */
  while(!exiting){
    int head = diskbuffer_head;
    int tail = diskbuffer_tail;
    int full_frames,i,j;
    unsigned char *ptr;
    if(tail > head) head = diskbuffer_frames;
    full_frames = (head - tail);
    
    /* scan audio, compiling metadata */
    ptr = diskbuffer + tail * framesize;
    for(i=0;i<full_frames;i++){
      for(j=0;j<channels;j++){
	int32_t val=0;
	float wval;

	switch(samplesize){
	case 4:
	  val |= (int)((unsigned int)*ptr);
	  val |= (int)(((unsigned int)*(ptr+1))<<8);
	  val |= (int)(((unsigned int)*(ptr+2))<<16);
	  val |= (int)(((unsigned int)*(ptr+3))<<24);
	  if(val == 0x7fffffff)pcm_data[disk_mark.bank][j].pcm_clip = 1;
	  break;
	case 3:
	  val |= (int)(((unsigned int)*ptr)<<8);
	  val |= (int)(((unsigned int)*(ptr+1))<<16);
	  val |= (int)(((unsigned int)*(ptr+2))<<24);
	  if(val == 0x7fffff00)pcm_data[disk_mark.bank][j].pcm_clip = 1;
	  break;
	case 2:
	  val |= (int)(((unsigned int)*ptr)<<16);
	  val |= (int)(((unsigned int)*(ptr+1))<<24);
	  if(val == 0x7fff0000)pcm_data[disk_mark.bank][j].pcm_clip = 1;
	  break;
	case 1:
	  val |= (int)(((unsigned int)*ptr)<<24);
	  if(val == 0x7f000000)pcm_data[disk_mark.bank][j].pcm_clip = 1;
	  break;
	}

	if(val == (int32_t)0x80000000)pcm_data[disk_mark.bank][j].pcm_clip = 1;

	/* find peak val per channel */
	if(pcm_data[disk_mark.bank][j].peak < abs(val))
	  pcm_data[disk_mark.bank][j].peak = abs(val);
	
	/* find rms/A-weight per-channel */
	wval = val*.00000000046566128730;
	if(weight){
	  wval = aweight_process(wval,&aweight_filter[j]);
	  val = wval / .00000000046566128730;
	}

	pcm_data[disk_mark.bank][j].weight_num += wval*wval;
	pcm_data[disk_mark.bank][j].weight_denom += 1.;
	
	/* write audio out */
	if(out_FILE && !paused)
	  fwrite(ptr,1,samplesize,out_FILE);

	ptr+=samplesize;
      }
    }
    
    if(full_frames){
      if(disk_mark.read == disk_mark.bank){
	disk_mark.bank = !disk_mark.bank;
	
	/* initialize new bank */
	for(j=0;j<channels;j++){
	  pcm_data[disk_mark.bank][j].peak = 0;
	  pcm_data[disk_mark.bank][j].weight_num = 0.;
	  pcm_data[disk_mark.bank][j].weight_denom = 0.;
	  pcm_data[disk_mark.bank][j].pcm_clip = 0;
	}
      }
    }
    
    /* atomic tail update */
    tail = head;
    if(tail == diskbuffer_frames) tail = 0;
    diskbuffer_tail = tail;
    
    sem_post(&ui_sem);
    if(diskbuffer_head == diskbuffer_tail)
      sem_wait(&buffer_sem);
  }
  return(NULL);
}

const char *optstring = "ac:d:r:w:qb:ns:hu";
struct option options [] = {
  {"help",no_argument,NULL,'h'},
  {"no-realtime",no_argument,NULL,'n'},
  {"no-lock",no_argument,NULL,'u'},
  {"a-weight",no_argument,NULL,'a'},
  {"quiet",no_argument,NULL,'q'},
  {"buffer",optional_argument,NULL,'b'},
  {"channels",optional_argument,NULL,'c'},
  {"width",optional_argument,NULL,'w'},
  {"rate",optional_argument,NULL,'r'},
  {"device",optional_argument,NULL,'d'},
  {"smooth",optional_argument,NULL,'s'},
  {NULL,0,NULL,0}
};

static void usage(void){
  fprintf(stderr,
          "Usage: rtrecord [options] [output_file] \n\n"
          "Options: \n\n"
	  "  -a --a-weight         A-weight the RMS level display\n"
	  "                        default is unweighted\n\n"
	  "  -b --buffer <seconds> Request <seconds> of disk buffer\n"
	  "                        default is 10 seconds\n\n"
          "  -c --channels <n>     Request recording of <n> channel audio\n"
	  "                        default is 2 channels (stereo)\n\n"
          "  -d --device <devname> Record from specified audio device\n"
	  "                        default is hw:0,0\n\n"
	  "  -h --help             This help message\n\n"
	  "  -n --no-realtime      Do not use realtime scheduling\n\n"
	  "  -r --rate <n>         Request sampling rate of <n> samples per\n"
	  "                        second.  Default is 44100\n\n"
	  "  -s --smooth <n>       Smooth the VU readouts by the specified\n"
	  "                        factor; 0 is unsmoothed, 10 is\n"
	  "                        very smooth; default is 1\n\n"
	  "  -q --quiet            Produce no terminal output; run silently\n\n"
	  "  -u --no-lock          Do not try to lock the recording buffer\n"
	  "                        into physical memory\n\n"
	  "  -w --width <n>        Request a sample width of <n> bits\n"
	  "                        default is 16\n\n"
	  "Keys:\n\n"
	  "   p     Pause recoding. Pause does not interrupt sampling; it \n"
	  "         merely suspends saving/piping recoded data out.\n\n"
	  "   q     Quit\n\n"
	  " space   clear clipping and overrun flags\n\n"
          "rtrecord outputs only uncompressed RIFF WAV format audio.  When no\n"
	  "output file is specified and stdout is not redirected away from\n"
	  "the tty, no output is produced; rtrecord will only sample and update\n"
	  "the terminal VU meters.\n\n");
  exit(1);
}

int main(int argc,char *argv[]){
  int c,long_option_index,ret,seconds=10;
  pthread_t dummy;

  /* before anything else, drop filesystem privs! */
  setfsuid(getuid());
  
  if(!strcmp(argv[0],"record") ||
     (strlen(argv[0])>=7 && !strcmp(argv[0]+strlen(argv[0])-7,"/record"))){
    /* an alias for a version that requires no resources normally
       limited to root, eg, don't lock the buffer, don't ask for
       realtime in the record thread */
    lockbuffer=0;
    realtime=0;
  }

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'c':
      channels = atoi(optarg);

      if(channels<=0){
        fprintf(stderr,"Number of channels must be greater than zero\n");
        exit(1);
      }
      break;
    case 'b':
      seconds = atoi(optarg);

      if(seconds<=0){
        fprintf(stderr,"Disk buffer must be at least 1 second.\n");
        exit(1);
      }
      break;

    case 'd':
      device = strdup(optarg);
      break;
      
    case 'a':
      weight = 1;
      break;

    case 'n':
      realtime=0;
      break;
      
    case 'q':
      quiet = 1;
      break;
      
    case 'u':
      lockbuffer = 0;
      break;
      
    case 'r':
      rate = atoi(optarg);
      if(rate<=0){
        fprintf(stderr,"Sampling rate must be greater than zero\n");
        exit(1);
      }
      break;
    case 's':
      {
	int s = atoi(optarg);
	if(s<0){
	  fprintf(stderr,"Minimum 'smooth' value is 0\n");
	  exit(1);
	}
	if(s>10){
	  fprintf(stderr,"Maximum 'smooth' value is 10\n");
	  exit(1);
	}
	smooth=1./(s+1);
      }
      break;

    case 'w':
      width = atoi(optarg);
      switch(width){
      case 8: 
	format = SND_PCM_FORMAT_S8;
	samplesize = 1;
	break;
      case 16:
	format = SND_PCM_FORMAT_S16_LE;
	samplesize = 2;
	break;
      case 24:
	format = SND_PCM_FORMAT_S24_3LE;
	samplesize = 3; 
	break;
      case 32:
	format = SND_PCM_FORMAT_S32_LE;
	samplesize = 4;
	break;
      default:
	fprintf(stderr,"Supported linear PCM sampling widths: 8, 16, 24 and 32 bits\n");
        exit(1);
      }
      break;
    default:
      usage();
    }
  }
  
  if(optind<argc){
    /* assume that anything following the options must be a filename */
    out_name = strdup(argv[optind]);
    out_FILE = fopen(out_name,"wb");
    if(!out_FILE){
      fprintf(stderr,"Unable to open file %s for writing.\n",out_name);
      exit(1);
    }
  }else{
    if(!isatty(STDOUT_FILENO)){
      /* use stdout for writing, except that curses is fairly hardwired
	 to want stdout, so dup this first... */
      int newfd = dup(STDOUT_FILENO); 
      out_name="stdout";
      out_FILE = fdopen(newfd,"wb");
    }else{
      out_name="(none)";
      out_FILE = 0;
    }      
  }
  
  /* if stdout isn't a terminal, and we're not running silent... */
  if(!quiet && (!isatty(STDOUT_FILENO))){
    /* if stderr is the terminal, let's use that. */
    if(isatty(STDERR_FILENO)){
      /* curses is fairly hardwired to want stdout, so remap stderr to stdout */
      dup2(STDERR_FILENO,STDOUT_FILENO);      
    }else{
      fprintf(stderr,"No terminal available for panel output; assuming -q\n");
      quiet=1;
    }
  }

  if(weight){
    aweight_init(rate,channels);
  }


  /* Open hardware sample device */
  if ((ret = snd_pcm_open (&devhandle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf(stderr,"unable to open audio device %s for record: %s.\n",device,snd_strerror(ret));
    exit(1);
  }
  record_setup();
  
  /* set up tty input subversion so that ncurses can be fully
     functional entirely withing one thread */
  ttyfd=open("/dev/tty",O_RDONLY);
  if(ttyfd<0){
    fprintf(stderr,"Unable to open /dev/tty:\n"
	    "  %s\n",strerror(errno));
    exit(1);
  }
  if(pipe(ttypipe)){
    fprintf(stderr,"Unable to open tty pipe:\n"
	    "  %s\n",strerror(errno));
    exit(1);
  }
  dup2(ttypipe[0],0);

  framesize = samplesize * channels;
  diskbuffer_frames = (rate*seconds/getpagesize()+1)*getpagesize();
  diskbuffer_size = diskbuffer_frames*framesize;
    
 if(lockbuffer){
    /* set up record buffer, lock it in-core */
     diskbuffer = mmap(0, diskbuffer_size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED, 0, 0);
    if(diskbuffer == MAP_FAILED){
      if(errno == EAGAIN){
	fprintf(stderr,"\nERROR: Unable to mmap locked buffer for capture;\n",strerror(errno));
	fprintf(stderr,"The configured per-user resource limits currently disallow\n"
		"locking enough memory in-core for a recording ringbuffer.\n\n"
		"To run with an unlocked ringbuffer, use the -u option; an unlocked\n"
		"ringbuffer may be slightly less reliable on a loaded machine.\n\n"
		"To allow running with a locked ringbuffer, consider unlimiting\n"
		"\"max locked memory\" for selected users or installing rtrecord\n"
		"setuid root.\n\n");
      }else{
	fprintf(stderr,"Unable to mmap locked buffer for capture:\n  %s\n",strerror(errno));
      }

      exit(1);
    }
  }else{
   diskbuffer = malloc(diskbuffer_size);
 }

  /* set up shared metadata feedback structs */
  pcm_data[0] = calloc(channels,sizeof(**pcm_data));
  pcm_data[1] = calloc(channels,sizeof(**pcm_data));
  
  sem_init(&ui_sem,0,0);
  sem_init(&buffer_sem,0,0);
  sem_init(&setup_sem,0,0);

  /* spawn a small army of threads */
  pthread_create(&disk_thread_id,NULL,disk_thread,NULL);
  pthread_create(&record_thread_id,NULL,record_thread,NULL);

  /* wait for realtime thread to declare it has started up */
  sem_wait(&setup_sem);

  /* drop privs, then continue starting threads */
  setuid(getuid());

  pthread_create(&tty_thread_id,NULL,tty_thread,NULL);
  pthread_create(&ui_thread_id,NULL,ui_thread,NULL);
  main_thread_id=pthread_self();

  //signal(SIGINT,SIG_IGN);


  if(!quiet)
    terminal_init_panel();
  
  /* setup complete; perform work */
  terminal_main_loop(quiet);
  
  /* remove the panel if possible */
  if(!quiet)
    terminal_remove_panel();
  
  exiting=1;
  
  /* wake the disk thread if its asleep */
  sem_post(&buffer_sem);

  /* wake the ui thread if its asleep */
  sem_post(&ui_sem);

  /* waking the tty thread cleanly would require another fd and a
     select(); much easier to just uncleanly cancel it */
  pthread_cancel(tty_thread_id);

  pthread_join(ui_thread_id, NULL);
  pthread_join(tty_thread_id, NULL);
  pthread_join(record_thread_id, NULL);
  pthread_join(disk_thread_id, NULL);
  sem_destroy(&ui_sem);
  sem_destroy(&buffer_sem);

  close(ttypipe[0]);
  close(ttypipe[1]);
  if(out_FILE)
    fclose(out_FILE);
  close(ttyfd);

  free(pcm_data[0]);
  free(pcm_data[1]);
  return(0);
  
}
