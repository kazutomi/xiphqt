/* top layer of subversion library to intercept RealPlayer socket and
   device I/O. --Monty 20011101 */

/* We grab audio by watching for open() on the audio device, and then
   capturing ioctl()s and read()s.  X is dealt with at two levels;
   when we need to add our own X events, we do that through
   RealPlayer's own Xlib state (to avoid opening another, or confusing
   Xlib by adding/removing events from its stream.  This is another
   way to get the infamous 'Xlib: Unexpected async reply' error, even
   if things are properly locked).  Mostly we deal with X by
   watching/effecting the wire level protocol over the X fd.  Watching
   the raw X gives some extra flexibility (like capturing expose),
   especially for potential future features (like not mapping the RP
   windows at all, but RP being unaware of it). */

#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _REENTRANT

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <linux/soundcard.h>
#include <pthread.h>
#include <X11/Xlib.h>

static int    (*libc_open)(const char *,int,mode_t);
static int    (*libc_connect)(int sockfd, const struct sockaddr *serv_addr,
			   socklen_t addrlen);
static int    (*libc_close)(int);
static size_t (*libc_read)(int,void *,size_t);
static size_t (*libc_write)(int,const void *,size_t);
static int    (*libc_readv)(int,const struct iovec *,int);
static int    (*libc_writev)(int,const struct iovec *,int);
static int    (*libc_ioctl)(int,int,void *);
static pid_t  (*libc_fork)(void);

static Display *(*xlib_xopen)(const char *);
static Display *Xdisplay;

static int debug;
static char *outpath;
static char *audioname;
static char *Xname;
static unsigned long Xinet_port;
static char *Xunix_socket;

static FILE *backchannel_fd=NULL;

static int audio_fd=-1;
static int audio_channels=-1;
static int audio_rate=-1;
static int audio_format=-1;

static int X_fd=-1;

static pthread_t snatch_backchannel_thread;
static pthread_t snatch_event_thread;

static char *username=NULL;
static char *password=NULL;
static char *openfile=NULL;
static char *location=NULL;

static int snatch_active=1;
static int fake_audiop=0;
static int fake_videop=0;

static void (*QueuedTask)(void);

#include "x11.c" /* yeah, ugly, but I don't want to leak symbols. 
		    Oh and I'm lazy. */

static char *audio_fmts[]={"unknown format",
			  "8 bit mu-law",
			  "8 bit A-law",
			  "ADPCM",
			  "unsigned, 8 bit",
			  "signed, 16 bit, little endian",
			  "signed, 16 bit, big endian",
			  "signed, 8 bit",
			  "unsigned, 16 bit, little endian",
			  "unsigned, 16 bit, big endian",
			  "MPEG 2",
			  "Dolby Digial AC3"};

static char *formatname(int format){
  int i;
  for(i=0;i<12;i++)
    if(format==((1<<i)>>1))return(audio_fmts[i]);
  return(audio_fmts[0]);
}

/* although RealPlayer is both multiprocess and multithreaded, we
don't lock because we assume only one thread/process will be mucking
with a specific X drawable or audio device at a time */

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

static pthread_cond_t event_cond=PTHREAD_COND_INITIALIZER;
static pthread_mutex_t event_mutex=PTHREAD_MUTEX_INITIALIZER;

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
      break;
    case 'U':
    case 'P':
    case 'L':
    case 'O':
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
    case 'I':
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
    libc_close=get_me_symbol("close");
    libc_read=get_me_symbol("read");
    libc_readv=get_me_symbol("readv");
    libc_write=get_me_symbol("write");
    libc_writev=get_me_symbol("writev");
    libc_connect=get_me_symbol("connect");
    libc_ioctl=get_me_symbol("ioctl");
    libc_fork=get_me_symbol("fork");
    xlib_xopen=get_me_symbol("XOpenDisplay");

    /* output path? */
    outpath=getenv("SNATCH_OUTPUT_PATH");
    if(!outpath){
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_OUTPUT_PATH\n"
		"           not set. Using current working directory.\n");
      outpath=".";
    }else{
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_OUTPUT_PATH\n"
		"           set (%s)\n",outpath);
    }

    /* audio device? */
    audioname=getenv("SNATCH_AUDIO_DEVICE");
    if(!audioname){
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_AUDIO_DEVICE\n"
		"           not set. Using default (/dev/dsp*).\n");
      audioname="/dev/dsp*";
    }else{
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_AUDIO_DEVICE\n"
		"           set (%s)\n",audioname);
    }

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
      fprintf(stderr,"    ...: Now watching for RealPlayer audio output.\n");

    /* X socket? */
    Xname=getenv("SNATCH_X_PORT");
    if(!Xname){
      char *display=getenv("DISPLAY");
      
      if(!display){
	fprintf(stderr,
		"**ERROR: DISPLAY environment variable not set, and no explicit\n"
		"         socket/host/port set via SNATCH_X_PORT. exit(1)ing...\n\n");
	exit(1);
      }

      Xname=display;	

      if(debug)
	fprintf(stderr,
		"----env: SNATCH_X_PORT\n"
		"           not set. Using DISPLAY value (%s).\n",display);
    }else{
      if(debug)
	fprintf(stderr,
		"----env: SNATCH_X_PORT\n"
		"           set (%s)\n",Xname);
    }
    
    if(Xname[0]==':'){
      /* local display */
      Xunix_socket=strdup("/tmp/.X11-unix/X                          ");
      sprintf(Xunix_socket+16,"%d",atoi(Xname+1));

      if(debug)
	fprintf(stderr,
		"    ...: Now watching for RealPlayer X connection on\n"
		"         local AF_UNIX socket %s\n",Xunix_socket);

    }else if(Xname[0]=='/'){
      Xunix_socket=strdup(Xname);

      if(debug)
	fprintf(stderr,
		"    ...: Now watching for RealPlayer X connection on\n"
		"         local AF_UNIX socket %s\n",Xunix_socket);
    }else{
      /* named host/port.  We only pay attention to port. */
      char *colonpos=strchr(Xname,':');

      if(!colonpos)
	Xinet_port=6000;
      else
	Xinet_port=atoi(colonpos)+6000;

      if(debug)
	fprintf(stderr,
		"    ...: Now watching for RealPlayer X connection on\n"
		"         AF_INET socket *.*.*.*:%ld\n",Xinet_port);
      
    }

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

    StartClientConnection();
    StartServerConnection();
  }
}


pid_t fork(void){
  initialize();
  return((*libc_fork)());
}

/* The audio device is subverted through open() */
int open(const char *pathname,int flags,...){
  va_list ap;
  int ret;
  mode_t mode;

  initialize();

  /* open needs only watch for the audio device. */
  if( (audioname[strlen(audioname)-1]=='*' &&
       !strncmp(pathname,audioname,strlen(audioname)-1)) ||
      (audioname[strlen(audioname)-1]!='*' &&
       !strcmp(pathname,audioname))){

      /* a match! */
      if(audio_fd>-1){
	/* umm... an audio fd is already open.  report the problem and
	   continue */
	fprintf(stderr,
		"\n"
		"WARNING: RealPlayer is attempting to open more than one\n"
		"         audio device (in this case, %s).\n"
		"         This behavior is unexpected; ignoring this open()\n"
		"         request.\n",pathname);
      }else{

	/* are we faking the audio? */
	if(fake_audiop)
	  ret=(*libc_open)("/dev/null",O_RDWR,mode);
	else
	  ret=(*libc_open)(pathname,flags,mode);
	
	audio_fd=ret;
	audio_channels=-1;
	audio_rate=-1;
	audio_format=-1;
	if(debug)
	  fprintf(stderr,
		  "    ...: Caught RealPlayer opening audio device "
		  "%s (fd %d).\n",
		  pathname,ret);
	if(debug && fake_audiop)
	  fprintf(stderr,
		  "    ...: Faking the audio open and writes as requested.\n");
      }
      return(ret);
  }

  if(flags|O_CREAT){
    va_start(ap,flags);
    mode=va_arg(ap,mode_t);
    va_end(ap);
  }

  ret=(*libc_open)(pathname,flags,mode);
  return(ret);
}

/* The X channel is subverted through connect */
int connect(int sockfd,const struct sockaddr *serv_addr,socklen_t addrlen){
  int ret;
  initialize();

  ret=(*libc_connect)(sockfd,serv_addr,addrlen);

  if(ret>-1){
    if(serv_addr->sa_family==AF_UNIX){
      struct sockaddr_un *addr=(struct sockaddr_un *)serv_addr;
      if(!strcmp(Xunix_socket,addr->sun_path)){
      	X_fd=sockfd;
	if(debug)
	  fprintf(stderr,
		  "    ...: Caught RealPlayer connecting to X server\n"
		  "         local socket %s (fd %d).\n",
		  Xunix_socket,ret);
      } 
    }
    
    if(Xinet_port && serv_addr->sa_family==AF_INET){
      struct sockaddr_in *addr=(struct sockaddr_in *)serv_addr;
      unsigned int port=ntohs(addr->sin_port);
      unsigned long host=ntohl(addr->sin_addr.s_addr);
      if(Xinet_port==port){
	X_fd=sockfd;
	if(debug)
	  fprintf(stderr,
		  "    ...: Caught RealPlayer connecting to X server\n"
		  "         on host %ld.%ld.%ld.%ld port %d (fd %d).\n",
		  (host>>24)&0xff,(host>>16)&0xff,(host>>8)&0xff,	host&0xff,
		  port,ret);
      }
    }
  }

  return(ret);
}

int close(int fd){
  int ret;

  initialize();

  ret=(*libc_close)(fd);
  
  if(fd==audio_fd){
    audio_fd=-1;
    if(debug)
      fprintf(stderr,
	      "    ...: RealPlayer closed audio playback fd %d\n",fd);
  }
  if(fd==X_fd){
    X_fd=-1;
    if(debug)
      fprintf(stderr,
	      "    ...: RealPlayer shut down X socket fd %d\n",fd);

    StopClientConnection();
    StopServerConnection();
  }
  
  return(ret);
}

ssize_t write(int fd, const void *buf,size_t count){
  initialize();

  if(fd==X_fd){
    ProcessBuffer(&clientCS,(void *)buf,count,DataToServer);
    return(count);
  }

  return((*libc_write)(fd,buf,count));
}

ssize_t read(int fd, void *buf,size_t count){
  int ret;
  initialize();

  ret=(*libc_read)(fd,buf,count);

  if(ret>0)
    if(fd==X_fd)
      ProcessBuffer(&serverCS,buf,ret,NULL);
  
  return(ret);
}

int readv(int fd,const struct iovec *v,int n){
  if(fd==audio_fd || fd==X_fd){
    int i,ret,count=0;
    for(i=0;i<n;i++){
      ret=read(fd,v[i].iov_base,v[i].iov_len);
      if(ret<0 && count==0)return(ret);
      if(ret<0)return(count);
      count+=ret;
      if(ret<v[i].iov_len)return(count);
    }
    return(count);
  }else
    return((*libc_readv)(fd,v,n));
}

int writev(int fd,const struct iovec *v,int n){
  if(fd==audio_fd || fd==X_fd){
    int i,ret,count=0;
    for(i=0;i<n;i++){
      ret=write(fd,v[i].iov_base,v[i].iov_len);
      if(ret<0 && count==0)return(ret);
      if(ret<0)return(count);
      count+=ret;
      if(ret<v[i].iov_len)return(count);
    }
    return(count);
  }else
    return((*libc_writev)(fd,v,n));
}

/* watch audio ioctl()s to track playback rate/channels/depth */
/* stdargs are here only to force the clean compile */
int ioctl(int fd,unsigned long int rq, ...){
  va_list optional;
  void *arg;
  int ret=0;
  initialize();
  
  va_start(optional,rq);
  arg=va_arg(optional,void *);
  va_end(optional);
  
  if(fd==audio_fd){
    if(rq==SNDCTL_DSP_SPEED ||
       rq==SNDCTL_DSP_CHANNELS ||
       rq==SNDCTL_DSP_SETFMT){

      if(!fake_audiop)
        ret=(*libc_ioctl)(fd,rq,arg);
      
      if(ret==0){
	switch(rq){
	case SNDCTL_DSP_SPEED:
	  audio_rate=*(int *)arg;
	  if(debug)
	    fprintf(stderr,
		    "    ...: Audio output sampling rate set to %dHz.\n",
		    audio_rate);
	  break;
	case SNDCTL_DSP_CHANNELS:
	  audio_channels=*(int *)arg;
	  if(debug)
	    fprintf(stderr,
		    "    ...: Audio output set to %d channels.\n",
		    audio_channels);
	  break;
	case SNDCTL_DSP_SETFMT:
	  audio_format=*(int *)arg;
	  if(debug)
	    fprintf(stderr,
		    "    ...: Audio output format set to %s.\n",
		    formatname(audio_format));
	  break;
	}
      }
      return(ret);
    }
  }
  return((*libc_ioctl)(fd,rq,arg));
}

Display *XOpenDisplay(const char *d){
  if(!XInitThreads()){
    fprintf(stderr,"**ERROR: Unable to set multithreading support in Xlib.\n"
	    "         exit(1)ing...\n\n");
    exit(1);
  }
  return(Xdisplay=(*xlib_xopen)(d));
}

static void queue_task(void (*f)(void)){
  pthread_mutex_lock(&event_mutex);
  QueuedTask=f;
  pthread_cond_signal(&event_cond);
  pthread_mutex_unlock(&event_mutex);
}
