/* top layer of subversion library to intercept RealPlayer socket and
   device I/O. --20011101 */

#define _GNU_SOURCE
#define _REENTRANT
#define _LARGEFILE_SOURCE 
#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

static pthread_mutex_t output_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_cond_t event_cond=PTHREAD_COND_INITIALIZER;
static pthread_mutex_t event_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t display_cond=PTHREAD_COND_INITIALIZER;
static pthread_mutex_t display_mutex=PTHREAD_MUTEX_INITIALIZER;

static int      (*libc_open)(const char *,int,mode_t);
static int      (*libc_connect)(int sockfd, const struct sockaddr *serv_addr,
				socklen_t addrlen);
static int      (*libc_close)(int);
static size_t   (*libc_read)(int,void *,size_t);
static size_t   (*libc_write)(int,const void *,size_t);
static int      (*libc_readv)(int,struct iovec *,int);
static int      (*libc_writev)(int,const struct iovec *,int);
static int      (*libc_ioctl)(int,int,void *);
static pid_t    (*libc_fork)(void);

static int      (*xlib_xclose)(Display *);
static Display *(*xlib_xopen)(const char *);
static int      (*xlib_xdrawsegments)(Display *,Drawable,GC,XSegment *,int);
static Window   (*xlib_xcreatewindow)(Display *,Window,int,int,unsigned int,unsigned int,
				      unsigned int,int,unsigned int,Visual*,unsigned long,
				      XSetWindowAttributes *);
static int      (*xlib_xconfigurewindow)(Display *,Window,unsigned int,XWindowChanges *);
static int      (*xlib_xchangeproperty)(Display *,Window,Atom,Atom,int,int,const unsigned char *,int);
static int      (*xlib_xputimage)(Display *,Drawable,GC,XImage *,int,int,int,int,
				  unsigned int,unsigned int);
static int      (*xlib_xshmputimage)(Display *,Drawable,GC,XImage *,int,int,int,int,
				     unsigned int,unsigned int,Bool);

static int      (*xlib_xresizewindow)(Display *,Window,unsigned int,unsigned int);

static Display *Xdisplay;

static int debug;
static char *outpath;
static FILE *backchannel_fd=NULL;

static int audio_fd=-1;
static int audio_channels=-1;
static int audio_rate=-1;
static int audio_format=-1;
static long long audio_samplepos=0;
static double audio_timezero=0;

static pthread_t snatch_backchannel_thread;
static pthread_t snatch_event_thread;

static char *username=NULL;
static char *password=NULL;
static char *openfile=NULL;
static char *location=NULL;

static int snatch_active=1;
static int fake_audiop=0;
static int fake_videop=0;

static int output_audio_p=0;
static int output_video_p=0;

static void (*QueuedTask)(void);

static int outfile_fd=-1;

static void CloseOutputFile();
static void OpenOutputFile();

static char *audio_fmts[]={"unknown format",
			   "8 bit mu-law",
			   "8 bit A-law",
			   "ADPCM",
			   "unsigned, 8 bit",
			   "signed, 16 bit, little endian",
			   "signed, 16 bit, big endian",
			   "signed, 8 bit",
			   "unsigned, 16 bit, little endian",
			   "unsigned, 16 bit, big endian"};

static int audio_fmt_bytesps[]={0,1,1,0,1,2,2,1,2,2};

static char *nstrdup(char *s){
  if(s)return strdup(s);
  return NULL;
}

static int gwrite(int fd, void *buf, int n){
  if(fd>=0){
    while(n){
      int ret=(*libc_write)(fd,buf,n);
      if(ret<0){
	if(errno==EAGAIN)
	  ret=0;
	else{
	  fprintf(stderr,"**ERROR: Write error on capture file!\n"
		  "       : %s\n",strerror(errno));
	  CloseOutputFile(1); /* if the error is the 2GB limit on Linux 2.2,
				this will result in a new file getting opened */
	  return(ret);
	}
      }
      buf+=ret;
      n-=ret;
    }
  }
  return(0);
}

static double bigtime(long *seconds,long *micros){
  static struct timeval   tp;
    
  (void)gettimeofday(&tp, (struct timezone *)NULL);
  if(seconds)*seconds=tp.tv_sec;
  if(micros)*micros=tp.tv_usec;
  return(tp.tv_sec+tp.tv_usec*.000001);
}

/* yeah, ugly, but I don't want to leak symbols.  Oh and I'm lazy. */
#include "x11.c" 
#include "oss.c"
#include "esd.c"

void *get_me_symbol(char *symbol){
  void *ret=dlsym(RTLD_NEXT,symbol);
  if(ret==NULL){
    char *dlerr=dlerror();
    fprintf(stderr,
	    "**ERROR: libsnatch.so could not find the function '%s()'.\n"
	    "         This shouldn't happen and I'm not going to\n"
	    "         make any wild guesses as to what caused it.  The\n"
	    "         error returned by dlsym() was:\n         %s",
	    symbol,(dlerr?dlerr:"no such symbol"));
    fprintf (stderr,"\n\nCannot continue.  exit(1)ing...\n\n");
    exit(1);
  }else
    if(debug)
      fprintf(stderr,"    ...: symbol '%s()' found and subverted.\n",symbol);

  return(ret);
}

void *event_thread(void *dummy){
  if(debug)
    fprintf(stderr,"    ...: Event thread %lx reporting for duty!\n",
	    (unsigned long)pthread_self());

  while(1){
    pthread_cond_wait(&event_cond,&event_mutex);
    if(QueuedTask){
      (*QueuedTask)();
      QueuedTask=NULL;
    }else{
      fprintf(stderr,
	      "**ERROR: Internal fault! event thread awoke without an event\n"
	      "         to process!\n");
    }
  }
}

void *backchannel_thread(void *dummy){
  if(debug)
    fprintf(stderr,"    ...: Backchannel thread %lx reporting for duty!\n",
	    (unsigned long)pthread_self());

  pthread_mutex_lock(&display_mutex);
  if(!Xdisplay)
    pthread_cond_wait(&display_cond,&display_mutex);
  pthread_mutex_unlock(&display_mutex);

  while(1){
    char rq;
    size_t ret=fread(&rq,1,1,backchannel_fd);
    short length;
    char *buf=NULL;

    if(ret<=0){
      fprintf(stderr,"**ERROR: Backchannel lost!  exit(1)ing...\n");
      exit(1);
    }
    rpauth_already=0;

    switch(rq){
    case 'K':
      {
	unsigned char sym;
	unsigned short mod;
	ret=fread(&sym,1,1,backchannel_fd);
	ret=fread(&mod,2,1,backchannel_fd);
	if(ret==1)
	  FakeKeySym(sym,mod,rpplay_window);
      }
      CloseOutputFile(0); /* it will only happen on Robot commands that would
			    be starting a new file */
      break;
    case 'E':
    case 'U':
    case 'P':
    case 'L':
    case 'O':
    case 'F':
    case 'D':
      ret=fread(&length,2,1,backchannel_fd);
      if(ret==1){
	if(length)buf=calloc(length+1,1);
	if(length)ret=fread(buf,1,length,backchannel_fd);
	if(length && ret==length)
	  switch(rq){
	  case 'U':
	    if(username)free(username);
	    username=buf;
	    break;
	  case 'P':
	    if(password)free(password);
	    password=buf;
	    break;
	  case 'L':
	    if(location)free(location);
	    location=buf;
	    break;
	  case 'O':
	    if(openfile)free(openfile);
	    openfile=buf;
	    break;
	  case 'E':
	    if(esdsocket)free(esdsocket);
	    esdsocket=buf;
	    break;
	  case 'F':
	    if(outpath)free(outpath);
	    outpath=buf;
	    break;
	  case 'D':
	    if(ossname)free(ossname);
	    ossname=buf;
	    break;
	  }
      }
      break;
    case 'T':
      snatch_active=2;
      FakeExposeRPPlay();
      break;
    case 'A':
      snatch_active=1;
      FakeExposeRPPlay();
      break;
    case 'C':
      CloseOutputFile(1);
      break;
    case 'I':
      CloseOutputFile(1);
      snatch_active=0;
      FakeExposeRPPlay();
      break;
    case 's':
      fake_audiop=1;
      break;
    case 'S':
      fake_audiop=0;
      break;
    case 'v':
      fake_videop=1;
      break;
    case 'V':
      fake_videop=0;
      FakeExposeRPPlay();
      break;
    }
  }
}

void initialize(void){
  if(!libc_open){

    /* be chatty? */    
    if(getenv("SNATCH_DEBUG"))debug=1;
    if(debug)fprintf(stderr,
		     "----env: SNATCH_DEBUG\n"
		     "           set\n");

    /* get handles to the libc symbols we're subverting */
    libc_open=get_me_symbol("open");
    libc_connect=get_me_symbol("connect");
    libc_close=get_me_symbol("close");
    libc_read=get_me_symbol("read");
    libc_write=get_me_symbol("write");
    libc_readv=get_me_symbol("readv");
    libc_writev=get_me_symbol("writev");
    libc_ioctl=get_me_symbol("ioctl");
    libc_fork=get_me_symbol("fork");
    xlib_xopen=get_me_symbol("XOpenDisplay");
    xlib_xclose=get_me_symbol("XCloseDisplay");
    xlib_xdrawsegments=get_me_symbol("XDrawSegments");
    xlib_xcreatewindow=get_me_symbol("XCreateWindow");
    xlib_xconfigurewindow=get_me_symbol("XConfigureWindow");
    xlib_xchangeproperty=get_me_symbol("XChangeProperty");
    xlib_xputimage=get_me_symbol("XPutImage");
    xlib_xshmputimage=get_me_symbol("XShmPutImage");
    xlib_xresizewindow=get_me_symbol("XResizeWindow");

    /* output path? */
    outpath=nstrdup(getenv("SNATCH_OUTPUT_PATH"));
    if(!outpath){
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_OUTPUT_PATH\n"
		"           not set. Using current working directory.\n");
      outpath=nstrdup(".");
    }else{
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_OUTPUT_PATH\n"
		"           set (%s)\n",outpath);
    }

    oss_hook_init();
    esd_hook_init();

    if(getenv("SNATCH_AUDIO_FAKE")){
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_AUDIO_FAKE\n"
		"           set.  Faking audio operations.\n");
      fake_audiop=1;
    }else
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_AUDIO_FAKE\n"
		"           not set.\n");

    if(getenv("SNATCH_VIDEO_FAKE")){
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_VIDEO_FAKE\n"
		"           set.  Faking video operations.\n");
      fake_videop=1;
    }else
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_VIDEO_FAKE\n"
		"           not set.\n");

    if(debug)
      fprintf(stderr,"    ...: Now watching for RealPlayer audio/video output.\n");


    {
      int ret;
      struct sockaddr_un addr;
      char backchannel_socket[sizeof(addr.sun_path)];
      int temp_fd;

      memset(backchannel_socket,0,sizeof(backchannel_socket));
      if(getenv("SNATCH_COMM_SOCKET"))
	strncpy(backchannel_socket,getenv("SNATCH_COMM_SOCKET"),
		sizeof(addr.sun_path)-1);

      if(backchannel_socket[0]){

	if(debug)
	  fprintf(stderr,
		  "----env: SNATCH_COMM_SOCKET\n"
		  "         set to %s; trying to connect...\n",
		  backchannel_socket);
	temp_fd=socket(AF_UNIX,SOCK_STREAM,0);
	if(temp_fd<0){
	  fprintf(stderr,
		  "**ERROR: socket() call for backchannel failed.\n"
		  "         returned error %d:%s\n"
		  "         exit(1)ing...\n\n",errno,strerror(errno));
	  exit(1);
	}

	addr.sun_family=AF_UNIX;
	strcpy(addr.sun_path,backchannel_socket);

	if((*libc_connect)(temp_fd,(struct sockaddr *)&addr,sizeof(addr))<0){
	  fprintf(stderr,
		  "**ERROR: connect() call for backchannel failed.\n"
		  "         returned error %d: %s\n"
		  "         exit(1)ing...\n\n",errno,strerror(errno));
	  exit(1);
	}
	if(debug)
	  fprintf(stderr,
		  "    ...: connected to backchannel\n");

	backchannel_fd=fdopen(temp_fd,"w+");
	if(backchannel_fd==NULL){
	  fprintf(stderr,
		  "**ERROR: fdopen() failed on backchannel fd.  error returned:\n"
		  "         %s\n         exit(1)ing...\n\n",strerror(errno));
	  exit(1);
	}

	if(debug)
	  fprintf(stderr,
		  "    ...: starting backchannel/fake event threads...\n");
	
	if((ret=pthread_create(&snatch_backchannel_thread,NULL,
		       backchannel_thread,NULL))){
	  fprintf(stderr,
		  "**ERROR: could not create backchannel worker thread.\n"
		  "         Error code returned: %d\n"
		  "         exit(1)ing...\n\n",ret);
	  exit(1);
	}else{
	  pthread_mutex_lock(&event_mutex);
	  if((ret=pthread_create(&snatch_event_thread,NULL,
				 event_thread,NULL))){
	    fprintf(stderr,
		    "**ERROR: could not create event worker thread.\n"
		    "         Error code returned: %d\n"
		    "         exit(1)ing...\n\n",ret);
	    exit(1);
	  }
	}
      }else
	if(debug)
	  fprintf(stderr,
		  "----env: SNATCH_COMM_SOCKET\n"
		  "         not set \n");

    }
  }
}


pid_t fork(void){
  initialize();
  return((*libc_fork)());
}

int open(const char *pathname,int flags,...){
  va_list ap;
  mode_t mode;

  initialize();

  if(oss_identify(pathname))return oss_open_hook(pathname);

  if(flags|O_CREAT){
    va_start(ap,flags);
    mode=va_arg(ap,mode_t);
    va_end(ap);
  }

  return ((*libc_open)(pathname,flags,mode));
}

int connect(int sockfd,const struct sockaddr *serv_addr,socklen_t addrlen){
  initialize();

  if(esd_identify(serv_addr,addrlen)) 
    return esd_connect_hook(sockfd,serv_addr,addrlen);
  
  return ((*libc_connect)(sockfd,serv_addr,addrlen));
}

int close(int fd){
  int ret;

  initialize();

  oss_close_hook(fd);
  esd_close_hook(fd);
  
  ret=(*libc_close)(fd);

  if(fd==audio_fd){
    audio_fd=-1;
    audio_samplepos=0;
    audio_channels=0;
    audio_timezero=bigtime(NULL,NULL);
    if(debug)
      fprintf(stderr,
	      "    ...: RealPlayer closed audio playback fd %d\n",fd);
    CloseOutputFile(0);
  }
    
  return(ret);
}

ssize_t read(int fd, void *buf,size_t count){
  if(esd_rw_hook_p(fd))return(esd_read_hook(fd,buf,count));
  return((*libc_read)(fd,buf,count));
}

ssize_t write(int fd, const void *buf,size_t count){

  if(fd==audio_fd){

    /* track audio sync regardless of record activity or fake setting;
       we could have record suddenly activated */
    
    long a,b;
    double now=bigtime(NULL,NULL),fa;
    double stime=audio_samplepos/(double)audio_rate+audio_timezero;

    /* do it first so we know how many bytes actually succeed */
    if(!fake_audiop)
      count=(*libc_write)(fd,buf,count);
    if(count<=0)return(count);
    
    /* video and audio buffer differently; video is always 'just
       in time' and thus uses absolute time positioning.  Video
       frames can also arrive 'late' because of X server
       congestion, but frames will never arrive early.  Sound is
       the opposite; samples must be queued ahead of time (and
       will queue arbitrarily deep) and always must arrive early,
       not late. For this reason, we need to pay attention to when
       the current sample buffer will begin playing, not when it
       is queued */
    
    if(stime<now){
      /* queue starved; player will need to skip or stretch.  Advance
	 the absolute position to maintain sync.  Note that this is
	 normal at beginning of capture or after pause */
      audio_samplepos=(now-audio_timezero)*audio_rate;
      stime=audio_samplepos/(double)audio_rate+audio_timezero;
    }
    
    b=modf(stime,&fa)*1000000.;
    a=fa;
    
    if(count>0 && snatch_active==1){
      
      pthread_mutex_lock(&output_mutex);
      if(outfile_fd>=0 && !output_audio_p)CloseOutputFile(1);
      if(outfile_fd<0)OpenOutputFile();
      pthread_mutex_unlock(&output_mutex);
      
      if(outfile_fd>=0){ /* always be careful */
	char cbuf[80];
	int len;
	
	len=sprintf(cbuf,"AUDIO %ld %ld %d %d %d %d:",a,b,audio_channels,
		    audio_rate,audio_format,count);
	
	pthread_mutex_lock(&output_mutex);
	gwrite(outfile_fd,cbuf,len);
	gwrite(outfile_fd,(void *)buf,count);
	pthread_mutex_unlock(&output_mutex);
	
      }
    }
    audio_samplepos+=(count/(audio_channels*audio_fmt_bytesps[audio_format]));
    
    return(count);
  }

  if(esd_rw_hook_p(fd))return(esd_write_hook(fd,buf,count));

  return((*libc_write)(fd,buf,count));
}

int writev(int fd,const struct iovec *v,int n){
  int i,ret,count=0;
  for(i=0;i<n;i++){
    ret=write(fd,v[i].iov_base,v[i].iov_len);
    if(ret<0 && count==0)return(ret);
    if(ret<0)return(count);
    count+=ret;
    if(ret<v[i].iov_len)return(count);
  }
  return(count);
}

int readv(int fd,const struct iovec *v,int n){
  int i,ret,count=0;
  for(i=0;i<n;i++){
    ret=read(fd,v[i].iov_base,v[i].iov_len);
    if(ret<0 && count==0)return(ret);
    if(ret<0)return(count);
    count+=ret;
    if(ret<v[i].iov_len)return(count);
  }
  return(count);
}

int ioctl(int fd,unsigned long int rq, ...){
  va_list optional;
  void *arg;
  initialize();
  
  va_start(optional,rq);
  arg=va_arg(optional,void *);
  va_end(optional);

  if(oss_ioctl_hook_p(fd))return(oss_ioctl_hook(fd,rq,arg));

  return((*libc_ioctl)(fd,rq,arg));
}

static void queue_task(void (*f)(void)){
  pthread_mutex_lock(&event_mutex);
  QueuedTask=f;
  pthread_cond_signal(&event_cond);
  pthread_mutex_unlock(&event_mutex);
}

static void OpenOutputFile(){
  if(outfile_fd!=-2){
    if(!strcmp(outpath,"-")){
      outfile_fd=STDOUT_FILENO;
      if(debug)fprintf(stderr,"    ...: Capturing to stdout\n");

      if(videocount){
	output_video_p=1;
	if(audio_channels){
	  output_audio_p=1;
	  gwrite(outfile_fd,"SNATCHAV---\n",12);
	}else{
	  gwrite(outfile_fd,"SNATCH-V---\n",12);
	}
      }else{
	if(audio_channels){
	  output_audio_p=1;
	  gwrite(outfile_fd,"SNATCHA----\n",12);
	}else{
	  gwrite(outfile_fd,"SNATCH-----\n",12);
	}
      }

    }else{
      struct stat buf;
      int ret=stat(outpath,&buf);
      if(!ret && S_ISDIR(buf.st_mode)){
	/* construct a new filename */
	struct tm *now;
	char buf2[4096];
	char buf1[256];
	time_t nows;
	nows=time(NULL);
	now=localtime(&nows);
	strftime(buf1,256,"%Y%m%d_%H:%M:%S",now);
	if(videocount){
	  if(audio_channels){
	    sprintf(buf2,"%s/%s_%s%dHz_%dx%d_AV.snatch",
		    outpath,
		    buf1,
		    (audio_channels==1?"mono":"stereo"),
		    audio_rate,
		    video_width,
		    video_height);
	  }else{
	    sprintf(buf2,"%s/%s_%dx%d_V.snatch",
		    outpath,
		    buf1,
		    video_width,
		    video_height);
	  }
	}else{
	  if(audio_channels){
	    sprintf(buf2,"%s/%s_%s%dHz_A.snatch",
		    outpath,
		    buf1,
		    (audio_channels==1?"mono":"stereo"),
		    audio_rate);
	  }else{
	    return;
	  }
	}
	
	outfile_fd=open64(buf2,O_RDWR|O_CREAT|O_APPEND,0770);
	if(outfile_fd<0){
	  fprintf(stderr,"**ERROR: Could not stat requested output path!\n"
		  "         %s: %s\n\n",buf2,strerror(errno));
	  outfile_fd=-2;
	}else{
	  if(debug)fprintf(stderr,"    ...: Capturing to file %s\n",buf2);
	}
	
	if(videocount){
	  output_video_p=1;
	  if(audio_channels){
	    output_audio_p=1;
	    gwrite(outfile_fd,"SNATCHAV---\n",12);
	  }else{
	    gwrite(outfile_fd,"SNATCH-V---\n",12);
	  }
	}else{
	  if(audio_channels){
	    output_audio_p=1;
	    gwrite(outfile_fd,"SNATCHA----\n",12);
	  }else{
	    gwrite(outfile_fd,"SNATCH-----\n",12);
	  }
	}
	
      }else{
	outfile_fd=open64(outpath,O_RDWR|O_CREAT|O_APPEND,0770);
	if(outfile_fd<0){
	  fprintf(stderr,"**ERROR: Could not stat requested output path!\n"
		  "         %s: %s\n\n",outpath,strerror(errno));
	  outfile_fd=-2;
	}else{
	  if(debug)fprintf(stderr,"    ...: Capturing to file %s\n",outpath);
	  
	  if(videocount){
	    output_video_p=1;
	    if(audio_channels){
	      output_audio_p=1;
	      gwrite(outfile_fd,"SNATCHAV---\n",12);
	    }else{
	      gwrite(outfile_fd,"SNATCH-V---\n",12);
	    }
	  }else{
	    if(audio_channels){
	      output_audio_p=1;
	      gwrite(outfile_fd,"SNATCHA----\n",12);
	    }else{
	      gwrite(outfile_fd,"SNATCH-----\n",12);
	    }
	  }
	}
      }
    }
  }
}

static void CloseOutputFile(int preserve){
  pthread_mutex_lock(&output_mutex);
  if(outfile_fd>=0){
    videocount=0;
    if(debug)fprintf(stderr,"    ...: Capture stopped.\n");
    if(outfile_fd!=STDOUT_FILENO)
      close(outfile_fd);
    outfile_fd=-1;
    if(!preserve){
      output_audio_p=0;
      output_video_p=0;
    }
  }
  pthread_mutex_unlock(&output_mutex);
}
