/* X11 layer of subversion library to intercept RealPlayer socket and
   device I/O. --Monty 20011101 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include "snatchppm.h"
#include "waitppm.h"
static int savefile=-1;

static unsigned long window_id_base=0;
static unsigned long window_id_mask=0;

static unsigned long root_window=0;
static unsigned long rpshell_window=0;
static unsigned long rpmain_window=0;
static unsigned long rpmenu_window=0;
static unsigned long rpplay_window=0;
static unsigned long rpplay_width=0;
static unsigned long rpplay_height=0;

static unsigned long logo_x=0;
static unsigned long logo_y=0;
static unsigned long logo_prev=-1;

static unsigned long play_blackleft=-1;
static unsigned long play_blackright=-1;
static unsigned long play_blackupper=-1;
static unsigned long play_blacklower=-1;

static unsigned long rpvideo_window=0;
static int video_width=-1;
static int video_length=-1;
static int bigendian_p=0;

static unsigned long rpauth_shell=0;
static unsigned long rpauth_main=0;
static unsigned long rpauth_password=0;
static unsigned long rpauth_username=0;
static unsigned long rpauth_okbutton=0;
static unsigned long rpauth_cancel=0;
static int rpauth_count=0;
static int rpauth_already=0;

static unsigned long rploc_shell=0;
static unsigned long rploc_button=0;
static unsigned long rploc_clear=0;
static unsigned long rploc_entry=0;
static unsigned long rploc_ok=0;
static unsigned long rploc_main=0;
static int rploc_count=0;

static unsigned long rpfile_shell=0;
static unsigned long rpfile_main=0;

static void queue_task(void (*f)(void));

/* Built out of a few pieces of xscope by James Peterson, 1988 
   xscope is (c) Copyright MCC, 1988 */

static int littleEndian;

static unsigned long ILong(unsigned char *buf){
  if(littleEndian)
    return((((((buf[3] << 8) | buf[2]) << 8) | buf[1]) << 8) | buf[0]);
  return((((((buf[0] << 8) | buf[1]) << 8) | buf[2]) << 8) | buf[3]);
}

static unsigned short IShort(unsigned char *buf){
  if(littleEndian)
    return (buf[1] << 8) | buf[0];
  return((buf[0] << 8) | buf[1]);
}

static unsigned short IByte (unsigned char *buf){
  return(buf[0]);
}

static void FakeKeycode(int keycode, int modmask, unsigned long window){
  XKeyEvent event;
  memset(&event,0,sizeof(event));

  event.display=Xdisplay;
  event.type=2; /* key down */
  event.keycode=keycode;
  event.root=root_window;
  event.window=window;
  event.state=modmask;

  XSendEvent(Xdisplay,(Window)window,0,0,(XEvent *)&event);

  event.type=3; /* key up */

  XSendEvent(Xdisplay,(Window)window,0,0,(XEvent *)&event);

}

static void FakeKeySym(int keysym, int modmask, unsigned long window){
  KeyCode c=XKeysymToKeycode(Xdisplay,keysym);
  
  if(XKeycodeToKeysym(Xdisplay,c,0)==keysym){
    FakeKeycode(c,modmask,window);
  }else{
    FakeKeycode(c,1|modmask,window);
  }

}

void FakeButton1(unsigned long window){
  XButtonEvent event;
  XCrossingEvent enter;

  memset(&enter,0,sizeof(enter));
  enter.type=7;
  enter.display=Xdisplay;
  enter.window=window;
  enter.root=root_window;
  enter.mode=0;
  enter.detail=3;
  enter.same_screen=1;
  XSendEvent(Xdisplay,(Window)window,0,0,(XEvent *)&enter);

  memset(&event,0,sizeof(event));
  event.display=Xdisplay;
  event.type=4; /* button down */
  event.button=1;
  event.root=root_window;
  event.window=window;
  event.same_screen=1;

  XSendEvent(Xdisplay,(Window)window,0,0,(XEvent *)&event);

  event.type=5; /* button up */
  event.state=0x100;

  XSendEvent(Xdisplay,(Window)window,0,0,(XEvent *)&event);

}

void FakeExposeRPPlay(void){
  XExposeEvent event;
  memset(&event,0,sizeof(event));

  event.display=Xdisplay;
  event.type=12;
  event.window=rpplay_window;
  event.width=rpplay_width;
  event.height=rpplay_height;

  XSendEvent(Xdisplay,(Window)rpplay_window,0,0,(XEvent *)&event);
}

void FakeTypeString(unsigned char *buf,unsigned long window){
  FakeButton1(window);
  while(*buf){
    FakeKeySym(*buf,0,window);
    buf++;
  }
}

static void SetUpReply(unsigned char *buf){
  if(IByte(&buf[0])){
    window_id_base=ILong(&buf[12]);
    window_id_mask=ILong(&buf[16]);
    bigendian_p=IByte(&buf[30]);
    
    if(debug){
      fprintf(stderr,
	      "    ...: RealPlayer X setup\n"
	      "           window id base = %lx\n"
	      "           window id mask = %lx\n"
	      "           server image endianness = %s\n",
	      window_id_base,window_id_mask,(bigendian_p?"big":"small"));
    }
  }
}

static void UsernameAndPassword(void){

  fprintf(stderr,"    ...: filling in username and password...\n");
  if(username)
    FakeTypeString(username,rpauth_username);
  if(password)
    FakeTypeString(password,rpauth_password);

  FakeTypeString(" ",rpauth_okbutton); /* space activates the button.
                                          Saves work parsing the
                                          window tree to get an
                                          absolute X,Y to make an
                                          event */
  rpauth_shell=0;
  rpauth_main=0;
  rpauth_password=0;
  rpauth_username=0;
  rpauth_okbutton=0;
  rpauth_count=0;
  
}

static void Location(void){
  
  fprintf(stderr,"    ...: filling in location field...\n");

  FakeTypeString(" ",rploc_clear); /* space activates the button.
				      Saves work parsing the
				      window tree to get an
				      absolute X,Y to make an
				      event */
  if(location)
    FakeTypeString(location,rploc_entry);
  FakeTypeString(" ",rploc_ok);

  rpauth_shell=0;
  rpauth_main=0;
  rpauth_password=0;
  rpauth_username=0;
  rpauth_okbutton=0;
  rpauth_count=0;
  
}

static void FileEntry(void){
  
  fprintf(stderr,"    ...: filling in file field...\n");
  if(openfile)
    FakeTypeString(openfile,rpfile_main);

  FakeKeySym(XStringToKeysym("Return"),0,rpfile_main);

  rpfile_shell=0;
  rpfile_main=0;
  
}

static void PolySegment(unsigned char *buf){
  /* we assume the auth window is ready when we see the last polylines put
     into the cancel window */
  unsigned long id=ILong(&buf[4]);
  if(id==rpauth_cancel){
    rpauth_cancel=0;
    queue_task(UsernameAndPassword);
  }

  if(id==rploc_entry && rploc_button){
    rploc_button=0;
    queue_task(Location);
  }

  if(id==rpfile_main && rpfile_shell){
    rpfile_shell=0;
    queue_task(FileEntry);
  }
}

static void CreateWindow(unsigned char *buf){
  unsigned long id=ILong(&buf[4]);
  unsigned long parent=ILong(&buf[8]);
   
  if((id & ~window_id_mask) == window_id_base){
    if(!root_window)
      root_window=parent;

    /* Main player windows */
    if(parent==rpshell_window){
      rpmain_window=id;
      rpplay_window=0;
      if(debug)
	fprintf(stderr,
		"    ...: RealPlayer main window id=%lx\n",rpmain_window);
      
    }else if(rpshell_window){
      if(parent==rpmain_window){
	if(!rpplay_window){
	  rpplay_window=id;

	  if(debug)
	    fprintf(stderr,
		    "    ...: RealPlayer console window id=%lx\n",rpplay_window);
	}else{
	  rpmenu_window=id;
	}
      }else if(parent==rpplay_window){
	rpvideo_window=id;
	if(debug)
	  fprintf(stderr,
		  "    ...: RealPlayer video window id=%lx\n",rpvideo_window);
      }
    }
    
    /* Auth dialog windows */
    if(parent==rpauth_shell){
	rpauth_main=id;
    }
    if(parent==rpauth_main){
      switch(rpauth_count++){
      case 5:
	rpauth_username=id;
	fprintf(stderr,"    ...: username window: %lx\n",id);
	break;
      case 3:
	rpauth_password=id;
	fprintf(stderr,"    ...: password window: %lx\n",id);
	break;
      case 1:
	rpauth_okbutton=id;
	fprintf(stderr,"    ...: OK button: %lx\n",id);
	break;
      case 0:
	rpauth_cancel=id;
	fprintf(stderr,"    ...: cancel button: %lx\n",id);
	break;
      }
    }

    /* Location dialog windows */
    if(parent==rploc_shell){
      rploc_main=id;
    }
    if(parent==rploc_main || parent==rploc_button){
      switch(rploc_count++){
      case 0:
	rploc_button=id;
	break;
      case 1:
	fprintf(stderr,"    ...: clear button: %lx\n",id);
	rploc_clear=id;
	break;
      case 3:
	fprintf(stderr,"    ...: ok button: %lx\n",id);
	rploc_ok=id;
	break;
      case 5:
	rploc_main=id;
	break;
      case 7:
	fprintf(stderr,"    ...: text entry: %lx\n",id);
	rploc_entry=id;
	break;
      }
    }

    /* File dialog windows */
    if(parent==rpfile_shell){
      rpfile_main=id;
    }
  }
}

/*
  13.49: Client -->   16 bytes
         ............REQUEST: ChangeWindowAttributes
             sequence number: 00000c7f
              request length: 0004
                      window: WIN 012000ce
                  value-mask: event-mask
                  value-list:
                          event-mask: KeyPress | ButtonPress | ButtonRelease |                                       EnterWindow | LeaveWindow | 
			              ButtonMotion | Exposure | FocusChange

*/

static void ConfigureWindow(unsigned char *buf){
  unsigned long id=ILong(&buf[4]);

  if(id==rpplay_window){
    unsigned long bitmask=IShort(&buf[8]);
    int i,count=0;
    
    for(i=0;i<16;i++){
      unsigned long testmask=1<<i;
      unsigned long val;
      if(bitmask & testmask){
	if(bigendian_p)
	  val=IShort(&buf[12+count+2]);
	else
	  val=IShort(&buf[12+count]);

	if(testmask==0x4){ /* width */
	  rpplay_width=val;
	}
	if(testmask==0x8){ /* height */
	  rpplay_height=val;
	}
	count+=4;
      }
    }
  }
}

static void ChangeProperty(unsigned char *buf){
  long    n;

  unsigned long id=ILong(&buf[4]);
  long property=ILong(&buf[8]);
  long type=ILong(&buf[12]);
  long format=ILong(&buf[16]);
  char *data;

  if(property!=67 || type!=31)return;  /* not interested if not
                                          WM_CLASS and STRING */
  n = ILong(&buf[20])*format/8;
  data=&buf[24];
  
  /* look for the RealPlayer shell window; the other player windows
     descend from it in a predicatble pattern */
  
  if(rpshell_window==0){
    if(debug)
      fprintf(stderr,
	      "    ...: looking for our shell window...\n"
	      "           candidate: id=%lx, name=%s class=%s\n",
	      id,(data?data:""),(data?strchr(data,'\0')+1:""));
    
    if(n>26 &&  !memcmp(data,"RealPlayer\0RCACoreAppShell\0",27)){
      /* it's our shell window above the WM parent */
      rpshell_window=id;
      
      /* re-setup */
      rpmain_window=0;
      rpmenu_window=0;
      rpplay_window=0;
      rpplay_width=0;
      rpplay_height=0;
      
      logo_x=0;
      logo_y=0;
      logo_prev=-1;
      
      rpvideo_window=0;
      video_width=-1;
      video_length=-1;
      
      if(debug)
	fprintf(stderr,"           GOT IT!\n");
    }else{
      if(debug)
	fprintf(stderr,"           nope...\n");
    }
  }

  /* watch for the auth password window */
  if(n>32 &&  !memcmp(data,"AuthDialogShell\0RCACoreAppShell\0",32)){
    if(rpauth_already>2){
      fprintf(stderr,
	      "**ERROR: Password not accepted.\n");
      rpauth_shell=0;
      rpauth_already=0;
    }else{      
      fprintf(stderr,
	      "    ...: RealPlayer popped auth window.  Watching for username\n"
	      "         password and ok button windows\n");
      rpauth_shell=id;
      rpauth_count=0;
      rpauth_username=0;
      rpauth_password=0;
      rpauth_okbutton=0;
      rpauth_already++;
    }
  }

  /* watch for the open location window */
  if(n>36 &&  !memcmp(data,"OpenLocationDialogShell\0RCACoreAppShell\0",36)){
    fprintf(stderr,
	    "    ...: RealPlayer popped open location dialog.  Watching for\n"
	    "         dialog window tree...\n");
    rploc_shell=id;
    rploc_count=0;
    rploc_entry=0;
    rploc_clear=0;
    rploc_ok=0;
  }

  /* watch for the open file window */
  if(n>32 &&  !memcmp(data,"OpenFileDialogShell\0RCACoreAppShell\0",32)){
    fprintf(stderr,
	    "    ...: RealPlayer popped open file dialog.\n");
    rpfile_shell=id;
    rpfile_main=0;
  }
}

static void PutImage(unsigned char *header,unsigned char *data){
  int id=ILong(&header[4]);
  if(snatch_active && id==rpvideo_window){
    int width=IShort(&header[12])+IByte(&header[20]);
    int height=IShort(&header[14]);
    int n = width*height*4,i,j;
    unsigned char *work=alloca(n+1),*ptr=data,charbuf[80]; 
    
    static long ZeroTime1 = -1;
    static long ZeroTime2 = -1;
    static struct timeval   tp;
    static long lastsec = 0;
    long    sec /* seconds */ ;
    long    hsec /* hundredths of a second */ ;
    
    //if(savefile==-1)
    //savefile=open("realplayer-out.ppm",O_RDWR|O_APPEND|O_CREAT,0770);

    (void)gettimeofday(&tp, (struct timezone *)NULL);
    if (ZeroTime1 == -1 || (tp.tv_sec - lastsec) >= 1000)
      {
	ZeroTime1 = tp.tv_sec;
	ZeroTime2 = tp.tv_usec / 10000;
      }
    
    lastsec = tp.tv_sec;
    sec = tp.tv_sec - ZeroTime1;
    hsec = tp.tv_usec / 10000 - ZeroTime2;
    if (hsec < 0)
      {
	hsec += 100;
	sec -= 1;
      }

    sprintf(charbuf,"P6\n# time %ld.%08ld\n%d %d 255\n",sec,hsec,width, height);
    //write(savefile,charbuf,strlen(charbuf));
    
    for(i=0,j=0;i<n;i+=4){
      work[j++]=ptr[i+2];
      work[j++]=ptr[i+1];
      work[j++]=ptr[i];
    }
    work[j++]='\n';
    
    //write(savefile,work,j);
  }
  
  /* Subvert the Real sign on logo; paste the Snatch logo in.
     Although this might seem like a vanity issue, the primary reason
     for doing this is to give the user a clear indication Snatch is
     working properly, so some care should be taken to get it right. */
  
  if(id==rpplay_window){
    int width=IShort(&header[12])+IByte(&header[20]);
    int height=IShort(&header[14]);
    int x=IShort(&header[16]);
    int y=IShort(&header[18]);

    unsigned char *ptr=data;
    long i,j,k;

    if(x==0 && width==rpplay_width){
      if(y==0){
	play_blackupper=42;
	play_blacklower=-1;
	play_blackleft=-1;
	play_blackright=-1;
      }

      if(y<=play_blackupper && y+width>play_blackupper){
	for(play_blackleft=20;play_blackleft>0;play_blackleft--)
	  if(ptr[(play_blackupper-y)*width*4+play_blackleft*4+1]!=0)break;
	play_blackleft++;
	for(play_blackright=width-20;play_blackright<width;play_blackright++)
	  if(ptr[(play_blackupper-y)*width*4+play_blackright*4+1]!=0)break;
      }

      if(play_blacklower==-1){
	play_blacklower=42;
	if(y>play_blacklower)play_blacklower=y;
	for(;play_blacklower<y+height;play_blacklower++)
	  if(ptr[(play_blacklower-y)*width*4+81]!=0)break;
	
	if(play_blacklower==y+height)play_blacklower=-1;
      }
    }

    /* after a resize, look where to put the logo... */
    if(x==0 && y<rpplay_height/2 && width==rpplay_width){
      if(y<=logo_prev)
	logo_y=-1;

      if(logo_y==-1){
	/* look for the real logo in the data; it's in the middle of the
	   big black block */
	int test;
	
	for(test=play_blackupper;test<height+y;test++)
	  if(test>=y)
	    if(ptr[(test-y)*width*4+(width/2*4)+1]!=0)break;
	
	if(test<height+y && 
	   (test+snatchheight<play_blacklower || play_blacklower==-1) && 
	   test!=y){
	  logo_y=test;
	  
	  /* verify enough room to display... */
	  if(test<50){
	    long blacklower;
	    
	    for(blacklower=test;blacklower<y+height;blacklower++)
	    if(ptr[(blacklower-y)*width*4+(20*4)+1]!=0)break;
	    
	    if(blacklower-test<snatchheight)logo_y=-1;
	  }
	}
      }
      logo_prev=y;
      logo_x=(width/2)-(snatchwidth/2);
    }

    /* blank background */
    if(snatch_active){
      unsigned char *bptr;
      
      if(snatch_active==1)
	bptr=snatchppm;
      else
	bptr=waitppm;

      if(bigendian_p){
	int lower=(play_blacklower==-1?y+height:play_blacklower);
	for(i=play_blackupper;i<lower;i++)
	  if(i>=y && i<y+height)
	    for(j=play_blackleft;j<play_blackright;j++)
	      if(j>=x && j<x+width){
		ptr[(i-y)*width*4+(j-x)*4]=0x00;
		ptr[(i-y)*width*4+(j-x)*4+1]=bptr[0];
		ptr[(i-y)*width*4+(j-x)*4+2]=bptr[1];
		ptr[(i-y)*width*4+(j-x)*4+3]=bptr[2];
	      }
      }else{
	int lower=(play_blacklower==-1?y+height:play_blacklower);
	for(i=play_blackupper;i<lower;i++)
	  if(i>=y && i<y+height)
	    for(j=play_blackleft;j<play_blackright;j++)
	      if(j>=x && j<x+width){
		ptr[(i-y)*width*4+(j-x)*4+3]=0x00;
		ptr[(i-y)*width*4+(j-x)*4+2]=bptr[0];
		ptr[(i-y)*width*4+(j-x)*4+1]=bptr[1];
		ptr[(i-y)*width*4+(j-x)*4]=bptr[2];
	      }
      }
    
      /* paint logo */
      if(logo_y!=-1){
	for(i=0;i<snatchheight;i++){
	  if(i+logo_y>=y && i+logo_y<height+y){
	    char *snatch;
	    char *real;
	    long end;
	    
	    k=x-logo_x;
	    if(k<0)k=0;
	    end=x+snatchwidth-logo_x;
	    j=(logo_x-x)*4;
	    if(j<0)j=0;
	    
	    real=ptr+width*4*(i+logo_y-y);
	    snatch=(snatch_active==1?snatchppm:waitppm)+snatchwidth*3*i;
	    
	    if(bigendian_p){
	      for(k*=3;k<snatchwidth*3 && j<width*4;){
		real[++j]=snatch[k++];
		real[++j]=snatch[k++];
		real[++j]=snatch[k++];
		++j;
	      }
	      
	      
	    }else{
	      for(k*=3;k<snatchwidth*3 && j<width*4;j+=4){
		real[j+2]=snatch[k++];
		real[j+1]=snatch[k++];
		real[j]=snatch[k++];
	      }
	    }
	  }
	}
      }
    }
  }
}
 
/* Client-to-Server and Server-to-Client interception processing */
/* Here are the most in-tact bits of xscope */
struct ConnState {
  unsigned char   *SavedBytes;
  long    SizeofSavedBytes;
  long    NumberofSavedBytes;
  long    NumberofBytesNeeded;
  long    (*ByteProcessing)();

  /* a hack to optimize the video PutImage; this is the only case
     where we actually care much about copies.  Most X requests are
     either very infrequent or tiny.  PutImage, on the other hand, is
     blasting several MB a second to do video.  So, this is ugly, but
     needed. */
  unsigned char PutImageHeader[24];
  int PutImageUsed;
};
 
static struct ConnState serverCS;
static struct ConnState clientCS;

static void DecodeRequest(unsigned char *pih,int pi,
			  unsigned char *buf,long n){
  if(pi){
    PutImage(pih,buf);
  }else{
    int   Request = IByte (&buf[0]);
    switch (Request){
    case 1:
      CreateWindow(buf);
      break;
    case 12:
      ConfigureWindow(buf);
      break;
    case 18:
      ChangeProperty(buf);
      break;
    case 66:
      PolySegment(buf);
      break;
    case 72:
      PutImage(buf,buf+24);
      break;
    }
  }
}

static long DataToServer(unsigned char *buf,long n){
  long togo=n;
  unsigned char *p=buf;
  
  if(n){
    while(togo>0){
      int bw=(*libc_write)(X_fd,p,togo);

      if(bw<0 && (errno==EAGAIN || errno==EINTR))bw=0;
      if(bw>=0)
	p+=bw;
      else{
	return(bw);
      }
      togo-=bw;
    }
  }

  return(0);
}

static void SaveBytes(struct ConnState *cs,unsigned char *buf,long n){

  /* a hack to avoid huge copies in PutImage */
  if(cs->NumberofSavedBytes==0 && 
     n>=24 && 
     cs->PutImageUsed==0 && 
     IByte(&buf[0])==72){

    unsigned long id=ILong(&buf[4]);
    /* other putimage requests have side effects and we don't want to
       write into Xlib maintained memory.  Besides, the only case we
       really need to optimize is the video window (which we currently
       don't alter) */
    if(id==rpvideo_window){
      memcpy(cs->PutImageHeader,buf,24);
      cs->PutImageUsed=24;
      n-=cs->PutImageUsed;
    }
  }

  /* In fact,t he way Xlib flushes events, this will practically never
     be needed, but is good to have around */
  
  /* check if there is enough space to hold the bytes we want */
  if(n>0){
    if (cs->NumberofSavedBytes + n > cs->SizeofSavedBytes){
      long    SizeofNewBytes = (cs->NumberofSavedBytes + n + 1);
      if(cs->SavedBytes)
	cs->SavedBytes = realloc(cs->SavedBytes,SizeofNewBytes);
      else
	cs->SavedBytes = malloc(SizeofNewBytes);
      
    cs->SizeofSavedBytes = SizeofNewBytes;
    }
    
    /* now copy the new bytes onto the end of the old bytes */
    memcpy(cs->SavedBytes + cs->NumberofSavedBytes,buf,n);
    cs->NumberofSavedBytes += n;
  }
}

static long RemoveHeader(struct ConnState *cs){
  long ret=cs->PutImageUsed;
  cs->PutImageUsed=0;
  return(ret);
}

static void RemoveSavedBytes(struct ConnState *cs,long n){
  /* check if all bytes are being removed -- easiest case */
  if (cs->NumberofSavedBytes <= n)
    cs->NumberofSavedBytes = 0;
  else if (n == 0)
    return;
  else{
    /* not all bytes are being removed -- shift the remaining ones down  */
    memmove(cs->SavedBytes,cs->SavedBytes+n,cs->NumberofSavedBytes - n);
    cs->NumberofSavedBytes -= n;
  }
}

static long pad(long n){
  return((n + 3) & ~0x3);
}

static long FinishRequest(struct ConnState *cs,unsigned char *buf,long n);

static long StartRequest(struct ConnState *cs,unsigned char *buf,long n){
  unsigned short requestlength;

  /* bytes 0,1 are ignored now; bytes 2,3 tell us the request length */
  requestlength = IShort(&buf[2]);
  cs->ByteProcessing = FinishRequest;
  cs->NumberofBytesNeeded = 4 * requestlength;
  return(0);
}

static long FinishSetUpMessage(struct ConnState *cs,unsigned char *buf,long n){
  littleEndian = (buf[0] == 'l');
  cs->ByteProcessing = StartRequest;
  cs->NumberofBytesNeeded = 4;
  return(n);
}

static long StartSetUpMessage(struct ConnState *cs,unsigned char *buf,long n){
  short   namelength;
  short   datalength;

  namelength = IShort(&buf[6]);
  datalength = IShort(&buf[8]);
  cs->ByteProcessing = FinishSetUpMessage;
  cs->NumberofBytesNeeded = n + pad((long)namelength) + pad((long)datalength);
  return(0);
}

static long FinishRequest(struct ConnState *cs,unsigned char *buf,long n){
  DecodeRequest(cs->PutImageHeader,cs->PutImageUsed,buf, n);
  cs->ByteProcessing = StartRequest;
  cs->NumberofBytesNeeded = 4;
  return(n);
}

/* ************************************************************ */

static long FinishSetUpReply(struct ConnState *cs,unsigned char *buf,long n){
  SetUpReply(buf);
  cs->ByteProcessing = NULL; /* no further reason to watch the stream */
  cs->NumberofBytesNeeded = 0;
  return(n);
}

static long StartSetUpReply(struct ConnState *cs,unsigned char *buf,long n){
  int replylength = IShort(&buf[6]);
  cs->ByteProcessing = FinishSetUpReply;
  cs->NumberofBytesNeeded = n + 4 * replylength;
  return(0);
}

static void StartClientConnection(void){
  memset(&clientCS,0,sizeof(clientCS));
  clientCS.ByteProcessing = StartSetUpMessage;
  clientCS.NumberofBytesNeeded = 12;
}

static void StopClientConnection(void){
  if (clientCS.SizeofSavedBytes > 0)
    free(clientCS.SavedBytes);
  memset(&clientCS,0,sizeof(clientCS));
}

static void StartServerConnection(void){
  memset(&serverCS,0,sizeof(clientCS));
  serverCS.ByteProcessing = StartSetUpReply;
  serverCS.NumberofBytesNeeded = 8;
}

static void StopServerConnection(void){
  if(serverCS.SizeofSavedBytes > 0)
    free(serverCS.SavedBytes);
  memset(&serverCS,0,sizeof(clientCS));
}

static void ProcessBuffer(struct ConnState *cs,unsigned char *buf,long n,
		     long (*w)(unsigned char *,long)){
  unsigned char   *BytesToProcess;
  long             NumberofUsedBytes;
  
  while (cs->ByteProcessing && /* we turn off watching replies from
				    the server after grabbing set up */	 
	 cs->NumberofSavedBytes + n + cs->PutImageUsed >= 
	 cs->NumberofBytesNeeded){

    if (cs->NumberofSavedBytes == 0){
      /* no saved bytes, so just process the first bytes in the
	 read buffer */
      BytesToProcess = buf /* address of request bytes */;
    }else{
      if (cs->NumberofSavedBytes < cs->NumberofBytesNeeded){
	/* first determine the number of bytes we need to
	   transfer; then transfer them and remove them from
	   the read buffer. (there may be additional requests
	   in the read buffer) */
	long    m;
	m = cs->NumberofBytesNeeded - cs->NumberofSavedBytes;
	SaveBytes(cs, buf, m);
	buf += m;
	n -= m;
      }
      BytesToProcess = cs->SavedBytes /* address of request bytes */;
    }
    
    NumberofUsedBytes = (*cs->ByteProcessing)
      (cs, BytesToProcess, cs->NumberofBytesNeeded);
    
    /* *After* we've processed the buffer (and possibly caused side
       effects), we ship the request off to the recipient */
    if(w){
      (*w)(cs->PutImageHeader,cs->PutImageUsed);
      (*w)(BytesToProcess,NumberofUsedBytes-cs->PutImageUsed);
    }

    /* the number of bytes that were actually used is normally (but not
       always) the number of bytes needed.  Discard the bytes that were
       actually used, not the bytes that were needed. The number of used
       bytes must be less than or equal to the number of needed bytes. */
    
    if (NumberofUsedBytes > 0){
      n-=RemoveHeader(cs);

      if (cs->NumberofSavedBytes > 0)
	RemoveSavedBytes(cs, NumberofUsedBytes);
      else{
	/* there are no saved bytes, so the bytes that were
	   used must have been in the read buffer */
	buf += NumberofUsedBytes;
	n -= NumberofUsedBytes;
      }
    }
  } /* end of while (NumberofSavedBytes + n >= NumberofBytesNeeded) */

  /* not enough bytes -- just save the new bytes for more later */
  if (cs->ByteProcessing && n > 0)
    SaveBytes(cs, buf, n);
  return;
}

