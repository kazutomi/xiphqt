/* top layer of subversion library to intercept RealPlayer socket and
   device I/O. --Monty 20011101 */

#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/uio.h>
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

/* of the members of the poll family, RP only uses select */
static int  (*libc_select)(int,fd_set *,fd_set *,fd_set *,
			   struct timeval *timeout);

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
	    "**ERROR: libsnatch.so could not find the function '%s()'\n"
	    "         in libc.  This shouldn't happen and I'm not going to\n"
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

void *backchannel_and_timer(void *dummy){
  if(debug)
    fprintf(stderr,"    ...: Backchannel thread %lx reporting for duty!\n",
	    (unsigned long)pthread_self());

  while(1){
    char length;
    size_t bytes=fread(&length,1,1,backchannel_fd);
    
    if(bytes<=0){
      fprintf(stderr,"**ERROR: Backchannel lost!  exit(1)ing...\n");
      exit(1);
    }

    if(debug)
      fprintf(stderr,"    ...: Backchannel request\n");
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
    libc_select=get_me_symbol("select");

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
		  "    ...: starting backchannel/fake event thread...\n");
	
	if((ret=pthread_create(&snatch_backchannel_thread,NULL,
		       backchannel_and_timer,NULL))){
	  fprintf(stderr,
		  "**ERROR: could not create backchannel worker thread.\n"
		  "         Error code returned: %d\n"
		  "         exit(1)ing...\n\n",ret);
	  exit(1);
	}else
	  if(debug)
	    fprintf(stderr,
		    "         ...done.\n");

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
int open(const char *pathname,int flags,mode_t mode){
  int ret;

  initialize();
  ret=(*libc_open)(pathname,flags,mode);

  if(ret>-1){
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
	audio_fd=ret;
	audio_channels=-1;
	audio_rate=-1;
	audio_format=-1;
	if(debug)
	  fprintf(stderr,
		  "    ...: Caught RealPlayer opening audio device "
		  "%s (fd %d).\n",
		  pathname,ret);
      }
    }
  }
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
  int ret;
  initialize();
  
  va_start(optional,rq);
  arg=va_arg(optional,void *);
  va_end(optional);
  
  if(fd==audio_fd){
    if(rq==SNDCTL_DSP_SPEED ||
       rq==SNDCTL_DSP_CHANNELS ||
       rq==SNDCTL_DSP_SETFMT){

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

int select(int  n,  fd_set  *readfds,  fd_set  *writefds,
	   fd_set *exceptfds, struct timeval *timeout){
  int ret;

  ret=(*libc_select)(n,readfds,writefds,exceptfds,timeout);

  /* it turns out that RealPlayer busywaits using select [jeez], so we
     don't need to do any extra work to wake it up to send our own
     events. However, just in case, if we're called with a large
     timeout, shave it down a bit. */

  /* do we have a pending synthetic event? */
  
  /* is one of the read fds our X socket? */

  return (*libc_select)(n,readfds,writefds,exceptfds,timeout);

}

