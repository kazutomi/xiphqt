/* The code for dealing with the EsounD daemon */
#include <endian.h>
#include <pthread.h>

typedef struct {
  int fd;
  int rq;
  int count;

  char fakebuf[1024];
  int fakecount;

  int fake;
} esd_connection;

static void esd_add_int_to_fake(esd_connection *e,int x){
  int *ptr=(int *)(e->fakebuf+e->fakecount);
  if(e->fakecount+4>1024){
    fprintf(stderr,
	    "**ERROR: The EsounD server faking code has overflowed\n"
	    "         its storage buffer.  This *should* be impossible\n"
	    "         and seeing this message imlies an internal fault.\n"
	    "         if things haven't gone sour yet, they're about to...\n");
    return;
  }
  *ptr=x;
  e->fakecount+=4;
}

static void esd_add_char_to_fake(esd_connection *e,char c){
  if(e->fakecount>=1024){
    fprintf(stderr,
	    "**ERROR: The EsounD server faking code has overflowed\n"
	    "         its storage buffer.  This *should* be impossible\n"
	    "         and seeing this message imlies an internal fault.\n"
	    "         if things haven't gone sour yet, they're about to...\n");
    return;
  }
  e->fakebuf[e->fakecount]=c;
  e->fakecount++;
}

static void esd_add_string_to_fake(esd_connection *e,char *c,int n){
  while(n--)
    esd_add_char_to_fake(e,*c++);
}

static void esd_add_nil_to_fake(esd_connection *e,int n){
  while(n--)
    esd_add_char_to_fake(e,0);
}

static unsigned int esd_format;
static char *esdsocket;
static char playername[128];
static pthread_mutex_t esd_mutex=PTHREAD_MUTEX_INITIALIZER;

/* RealPlayer opens two EsounD connections.  One for daemon control
   requests, the other for streaming audio */

static esd_connection esdconn[2]={{-1},{-1}};
static int esdconmax=2;

static esd_connection *getcon(int fd){
  int i;
  pthread_mutex_lock(&esd_mutex);
  for(i=0;i<esdconmax;i++)
    if(esdconn[i].fd==-1){
      memset(&esdconn[i],0,sizeof(esdconn[1]));
      esdconn[i].fd=fd;
      if(fake_audiop)esdconn[i].fake=1;
      esdconn[i].count=20; /* 5 more bytes for auth header */
      pthread_mutex_unlock(&esd_mutex);
      return &esdconn[i];
    }
  fprintf(stderr,
	  "**ERROR: RealPlayer tried to open more than two connections\n"
	  "         to the EsounD server, something not expected.\n"
	  "         Not capturing this one; things may go south quickly.\n");

  pthread_mutex_unlock(&esd_mutex);
  return(NULL);
}

static int esd_translate_format(int e){
  switch(e&0xf){
  case 0:
    /* unsigned 8 bit */
    return(4);
  case 1:
    /* signed 16 bit, host endian */
#ifdef __BIG_ENDIAN__
    return(6);
#else
    return(5);
#endif

  default:
    fprintf(stderr,
	    "**ERROR: RealPlayer requested an unknown audio format for\n"
	    "         playback.  Winging it.\n");
    break;
  }
  return(0);
}

static int esd_translate_channels(int e){
  return((e>>4)&0xf);
}

static void esd_hook_init(void ){
  /* esd socket? */
  esdsocket=nstrdup(getenv("SNATCH_ESD_SOCKET"));
  if(!esdsocket){
    esdsocket=nstrdup(getenv("ESPEAKER"));
    if(debug){
      if(esdsocket){
	fprintf(stderr,
		"----env: SNATCH_ESD_PORT\n"
		"           not set. Using ESPEAKER variable.\n");
      }else{
	fprintf(stderr,
		"----env: SNATCH_ESD_PORT\n"
		"           not set. Using default (/var/run/esound/socket).\n");
	
	esdsocket=nstrdup("/var/run/esound/socket");
      }
    }
  }else{
    if(debug)
      fprintf(stderr,
	      "----env: SNATCH_ESD_SOCKET\n"
	      "           set (%s)\n",esdsocket);
  }
}

/* EsounD is socket based.  Watch for a connection */
static int esd_identify(const struct sockaddr *serv_addr,socklen_t addrlen){
  if(serv_addr->sa_family==AF_UNIX){
    struct sockaddr_un *addr=(struct sockaddr_un *)serv_addr;
    if(!strcmp(esdsocket,addr->sun_path))return(1);
    return(0);
  }
  if(serv_addr->sa_family==AF_INET){
    struct sockaddr_in *addr=(struct sockaddr_in *)serv_addr;
    unsigned int port=ntohs(addr->sin_port);
    char *colonpos=strchr(esdsocket,':');
    if(colonpos && port==atoi(colonpos+1))return(1);
  }
  return(0);
}

static int esd_connect_hook(int sockfd,const struct sockaddr *serv_addr,
			    socklen_t addrlen){

  esd_connection *esd=getcon(sockfd);
  if(!esd)return(-1);

  if(esd->fake){
    return(0);
  }else{
    int ret=(*libc_connect)(sockfd,serv_addr,addrlen);
    if(ret>-1){
      if(serv_addr->sa_family==AF_UNIX){
	if(debug)
	  fprintf(stderr,
		  "    ...: Caught RealPlayer connecting to EsounD server\n"
		  "         local socket %s (fd %d).\n",
		  esdsocket,sockfd);
	
      }else if(serv_addr->sa_family==AF_INET){
	struct sockaddr_in *addr=(struct sockaddr_in *)serv_addr;
	unsigned int port=ntohs(addr->sin_port);
	unsigned long host=ntohl(addr->sin_addr.s_addr);
	if(debug)
	  fprintf(stderr,
		  "    ...: Caught RealPlayer connecting to EsounD server\n"
		  "         on host %ld.%ld.%ld.%ld port %d (fd %d).\n",
		  (host>>24)&0xff,(host>>16)&0xff,(host>>8)&0xff,       host&0xff,
		  port,ret);
	
	/* unfortunately there's far too little documentation on how
           the Hell ESD handles mixed-endianness connections (if at
           all; some of the endianness code I've found in it varied
           from frightening to outright broken).  Take no chance of
           blowing out someone's ears. */
	
	fprintf(stderr,
		"**ERROR: Sorry, but Snatch doesn't currently do remote\n"
		"         EsounD connections.  This connection will not be\n"
		"         captured.\n");

	pthread_mutex_lock(&esd_mutex);
	esd->fd=-1;
	pthread_mutex_unlock(&esd_mutex);
	
      }
    }else{
      pthread_mutex_lock(&esd_mutex);
      esd->fd=-1;
      pthread_mutex_unlock(&esd_mutex);
    }
    return(ret);
  }
}

static void esd_close_hook(int fd){
  int i;
  for(i=0;i<esdconmax;i++)
    if(fd==esdconn[i].fd)
      esdconn[i].fd=-1;
}

static int esd_rw_hook_p(int fd){
  int i;
  if(fd==audio_fd)return(0);
  for(i=0;i<esdconmax;i++)
    if(fd==esdconn[i].fd)return(1);
  return(0);
}

static int esd_read_hook(int fd,void *buf,int count){
  int i;
  for(i=0;i<esdconmax;i++)
    if(fd==esdconn[i].fd){
      if(esdconn[i].fake){

	/* read from the fake buffer */
	if(esdconn[i].fakecount<count)
	  count=esdconn[i].fakecount;
	
	if(count)memcpy(buf,esdconn[i].fakebuf,count);
	esdconn[i].fakecount-=count;
	if(esdconn[i].fakecount)
	  memmove(esdconn[i].fakebuf,
		  esdconn[i].fakebuf+count,
		  esdconn[i].fakecount);

	return(count);
      }else{
	int ret=((*libc_read)(fd,buf,count));
	return(ret);
      }
    }

  return((*libc_read)(fd,buf,count));
}

static int esd_write_hook(int fd, const void *buf,int count){
  int *ptr=(int *)buf;
  char *cptr=(char *)buf;
  int i,n,ret;

  for(i=0;i<esdconmax;i++)
    if(fd==esdconn[i].fd){
      
      if(!esdconn[i].fake){
	ret=(*libc_write)(fd,buf,count);
	if(ret>0)
	  n=ret/4;
	else
	  return(ret);
      }else{
	ret=count;
	n=count/4;
      }
      
      while(n-->0){
	if(esdconn[i].count<=0){
	  /* handle new request */
	  esdconn[i].rq=*ptr++;
	  switch(esdconn[i].rq){
	  case 3:
	    /* we're the audio stream! */
	    esdconn[i].count=136;
	    break;
	  case 17:
	    /* get complete server info */
	    esdconn[i].count=4;
	    break;
	  case 20:
	    /* set volume/balance */
	    esdconn[i].count=12;
	    break;
	  }
	}else{
	  /* read or ignore request fields */
	  switch(esdconn[i].rq){
	  case 3:
	    switch(esdconn[i].count){
	    case 136:
	      esd_format=*ptr;
	      audio_format=esd_translate_format(*ptr);
	      audio_channels=esd_translate_channels(*ptr);
	      
	      if(debug)
		fprintf(stderr,"    ...: Audio output set to %d channels.\n",
			audio_channels);
	      if(debug)
		fprintf(stderr,"    ...: Audio output format set to %s.\n",
			audio_fmts[audio_format]);
	      break;
	    case 132:
	      audio_rate=*ptr;
	      if(debug)
		fprintf(stderr,
			"    ...: Audio output sampling rate set to %dHz.\n",
			audio_rate);
	      break;
	    default:
	      /* need this for later */
	      {
		char *p=playername+128-esdconn[i].count;
		*p++=cptr[0];
		*p++=cptr[1];
		*p++=cptr[2];
		*p++=cptr[3];
	      }
	      break;
	    }
	    break;
	  case 17:
	    /* ignore */
	    break;
	  case 20:
	    /* ignore */
	    break;
	  }
	  ++ptr;
	  cptr+=4;
	  esdconn[i].count-=4;

	  if(esdconn[i].count<=0){
	    /* cleanup from request if it's done */
	    switch(esdconn[i].rq){
	    case 3:
	      /* ok, audio stream init all handled; cut it loose */
	      audio_fd=fd;
	      /* no response; just go */
	      break;
	    case 17:
	      /* place the server info in the fake buffer if we're faking */
	      if(esdconn[i].fake){
		esd_connection *e=&esdconn[i];

		/* server info */
		esd_add_int_to_fake(e,0);
		esd_add_int_to_fake(e,44100);
		esd_add_int_to_fake(e,0x21); /* stereo 16 bit */

		/* realplayer entry */
		esd_add_int_to_fake(e,0x7);
		esd_add_string_to_fake(e,playername,128);
		esd_add_int_to_fake(e,audio_rate);
		esd_add_int_to_fake(e,0x100);
		esd_add_int_to_fake(e,0x100);
		esd_add_int_to_fake(e,esd_format);

		/* nil player entry */
		esd_add_int_to_fake(e,0);
		esd_add_nil_to_fake(e,128);
		esd_add_int_to_fake(e,0);
		esd_add_int_to_fake(e,0x100);
		esd_add_int_to_fake(e,0x100);
		esd_add_int_to_fake(e,0);

		/* nil sample entry */
		esd_add_int_to_fake(e,0);
		esd_add_nil_to_fake(e,128);
		esd_add_int_to_fake(e,0);
		esd_add_int_to_fake(e,0x100);
		esd_add_int_to_fake(e,0x100);
		esd_add_int_to_fake(e,0);
		esd_add_int_to_fake(e,0);

		/* 460 bytes */
	      }
	      break;
	    case 20:case 0:
	      /* place the ok response in the fake buffer if we're faking */
	      if(esdconn[i].fake){
		esd_add_int_to_fake(&esdconn[i],0x1);
	      }
	      break;
	    }
	  }
	}
      }
      return(ret);
    }
  return(-1);
}

