/* All the code for dealing with OSS */

#include <linux/soundcard.h>

static char *ossname;
static int oss_fd=-1;
static int oss_fd_fakeopen=0;

static int oss_fmt_translate(int format){
  int i;
  for(i=0;i<10;i++)
    if(format==((1<<i)>>1))return(i);
  return(0);
}

static void oss_hook_init(void){

  /* oss device? */
  ossname=nstrdup(getenv("SNATCH_OSS_DEVICE"));
  if(!ossname){
    if(debug)
      fprintf(stderr,
	      "----env: SNATCH_OSS_DEVICE\n"
	      "           not set. Using default (/dev/dsp*).\n");
    ossname=nstrdup("/dev/dsp*");
  }else{
    if(debug)
      fprintf(stderr,
	      "----env: SNATCH_OSS_DEVICE\n"
	      "           set (%s)\n",ossname);
  }
}

/* The OSS audio device is subverted through open().  If we didn't
   care about allowing a fake audio open() to 'succeed' even when the
   real device is busy, then we could just watch for the ioctl(), grab
   the fd() then, and not need to bother with any silly string
   matching.  However, we *do* care, so we do this the more complex,
   slightly more error prone way. */

static int oss_identify(const char *pathname){
  if( (ossname[strlen(ossname)-1]=='*' &&
       !strncmp(pathname,ossname,strlen(ossname)-1)) ||
      (ossname[strlen(ossname)-1]!='*' &&
       !strcmp(pathname,ossname)))return(1);
  return(0);
}

static int oss_open_hook(const char *pathname){
  int ret;
  if(oss_fd>-1){
    /* umm... an oss fd is already open.  report the problem and
       continue */
    fprintf(stderr,
	    "\n"
	    "WARNING: RealPlayer is attempting to open more than one\n"
	    "         OSS audio device (in this case, %s).\n"
	    "         This behavior is unexpected; ignoring this open()\n"
	    "         request.\n",pathname);
    return(-1);
  }else{

    /* are we faking the audio? */
    if(fake_audiop){
      ret=(*libc_open)("/dev/null",O_RDWR,0);
      oss_fd_fakeopen=1;
    }else{
      ret=(*libc_open)(pathname,O_RDWR,0);
      oss_fd_fakeopen=0;
    }

    oss_fd=ret;
    audio_fd=ret;
    audio_channels=-1;
    audio_rate=-1;
    audio_format=-1;
    audio_samplepos=0;
    audio_timezero=bigtime(NULL,NULL);
    
    if(debug)
      fprintf(stderr,
	      "    ...: Caught RealPlayer opening OSS audio device "
	      "%s (fd %d).\n",
	      pathname,ret);
    if(debug && fake_audiop)
      fprintf(stderr,
	      "    ...: Faking the audio open and writes as requested.\n");
  }
  return(ret);
}

static void oss_close_hook(int fd){
  if(fd==oss_fd)oss_fd=-1;
}

/* watch OSS audio ioctl()s to track playback rate/channels/depth */

static int oss_ioctl_hook_p(int fd){
  if(fd==oss_fd)return(1);
  return(0);
}

static int oss_ioctl_hook(int fd,int rq,void *arg){
  int ret=0;
  if(!fake_audiop && !oss_fd_fakeopen)
    ret=(*libc_ioctl)(fd,rq,arg);
    
  switch(rq){
  case SNDCTL_DSP_RESET:
    audio_timezero=0;
    audio_samplepos=0;
    break;
  case SNDCTL_DSP_SPEED:
    audio_rate=*(int *)arg;
    if(debug)
      fprintf(stderr,
	      "    ...: Audio output sampling rate set to %dHz.\n",
	      audio_rate);
    CloseOutputFile();
    break;
  case SNDCTL_DSP_CHANNELS:
    audio_channels=*(int *)arg;
    if(debug)
      fprintf(stderr,
	      "    ...: Audio output set to %d channels.\n",
	      audio_channels);
    CloseOutputFile();
    break;
  case SNDCTL_DSP_SETFMT:
    audio_format=oss_fmt_translate(*(int *)arg);
    if(debug)
      fprintf(stderr,
	      "    ...: Audio output format set to %s.\n",
	      audio_fmts[audio_format]);
    CloseOutputFile();
    break;
  case SNDCTL_DSP_GETOSPACE:
    if(fake_audiop){
      audio_buf_info *temp=arg;
      temp->fragments=32;
      temp->fragstotal=32;
      temp->fragsize=2048;
      temp->bytes=64*1024;
      
      if(debug)
	fprintf(stderr,"    ...: OSS output buffer size requested; faking 64k\n");
      ret=0;
    }
    break;
  case SNDCTL_DSP_GETODELAY: /* Must reject the ODELAY if we're not going to 
				properly simulate it! */
    if(fake_audiop){
      if(debug)
	fprintf(stderr,
		"    ...: Rejecting SNDCTL_DSP_GETODELAY ioctl()\n");
      *(int *)arg=0;
      ret=-1;
    }
    break;
  }
  
  return(ret);
}
