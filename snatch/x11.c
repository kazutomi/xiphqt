/* X11 layer of subversion library to intercept RealPlayer socket and
   device I/O. --20011101 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include "snatchppm.h"
#include "waitppm.h"
#include "realppm.h"

static unsigned long root_window=0;
static unsigned long rpshell_window=0;
static unsigned long rpmain_window=0;
static unsigned long rpmenu_window=0;
static unsigned long rpplay_window=0;
static unsigned long rpplay_width=0;
static unsigned long rpplay_height=0;

static unsigned long logo_y=-1;
static unsigned long logo_prev=-1;

static unsigned long play_blackleft=-1;
static unsigned long play_blackright=-1;
static unsigned long play_blackupper=-1;
static unsigned long play_blacklower=-1;

static unsigned long rpvideo_window=0;
static int video_width=-1;
static int video_height=-1;

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
static unsigned long rpfile_entry=0;
static int rpfile_lowest=0;

static void queue_task(void (*f)(void));
static void initialize();
static int videocount=0;
static int videotime=0;

static int depthwarningflag;

static void XGetGeometryRoot(unsigned long id,int *root_x,int *root_y){
  int x=0;
  int y=0;
  while(id!=root_window){
    Window root_return,parent_return,*children;
    int x_return, y_return;
    unsigned int width_return, height_return, border_width_return,
      depth_return;

    XGetGeometry(Xdisplay,id,&root_return, &x_return, &y_return,
		    &width_return,&height_return,&border_width_return,
		 &depth_return);
    x+=x_return;
    y+=y_return;
    XQueryTree(Xdisplay,id,&root_return,&parent_return,&children,&depth_return);
    XFree(children);
    id=parent_return;
  }
  *root_x=x;
  *root_y=y;
}

/* Robot events *********************************************************/

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

  /* Don't send the keyups; RP doesn't care and a 'return' or 'space'
     can close a window... resulting in sending a keyup to a drawable
     that doesn't exist. */

  //event.type=3; /* key up */

  //XSendEvent(Xdisplay,(Window)window,0,0,(XEvent *)&event);

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
  int root_x,root_y;

  XGetGeometryRoot(window,&root_x,&root_y);
  memset(&event,0,sizeof(event));
  event.display=Xdisplay;
  event.type=4; /* button down */
  event.button=1;
  event.root=root_window;
  event.x=4;
  event.y=4;
  event.x_root=root_x+4;
  event.y_root=root_y+4;

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

static void UsernameAndPassword(void){

  fprintf(stderr,"    ...: filling in username and password...\n");
  if(username)
    FakeTypeString(username,rpauth_username);
  if(password)
    FakeTypeString(password,rpauth_password);
  FakeButton1(rpauth_okbutton);

  rpauth_shell=0;
  rpauth_main=0;
  rpauth_password=0;
  rpauth_username=0;
  rpauth_okbutton=0;
  rpauth_count=0;
  
}

static void Location(void){
  
  fprintf(stderr,"    ...: filling in location field...\n");

  FakeButton1(rploc_clear);

  if(location)
    FakeTypeString(location,rploc_entry);

  FakeButton1(rploc_ok);

  rploc_shell=0;
  rploc_main=0;
  rploc_ok=0;
  rploc_count=0;
  rploc_entry=0;
  
}

static void FileEntry(void){
  int i;
  fprintf(stderr,"    ...: filling in file field...\n");

  /* gah, hack */
  for(i=0;i<300;i++)
    FakeKeySym(XStringToKeysym("BackSpace"),0,rpfile_entry);

  if(openfile)
    FakeTypeString(openfile,rpfile_entry);

  FakeKeySym(XStringToKeysym("Return"),0,rpfile_entry);

  rpfile_shell=0;
  rpfile_main=0;
  
}

/* captured calls ******************************************************/

Display *XOpenDisplay(const char *d){
  initialize();

  if(!XInitThreads()){
    fprintf(stderr,"**ERROR: Unable to set multithreading support in Xlib.\n"
	    "         exit(1)ing...\n\n");
    exit(1);
  }
  Xdisplay=(*xlib_xopen)(d);
  pthread_mutex_lock(&display_mutex);
  pthread_cond_signal(&display_cond);
  pthread_mutex_unlock(&display_mutex);
  return(Xdisplay);
}

/* we assume the * window is ready when we see the last polylines put
   into the chosen button window */
int XDrawSegments(Display *display, Drawable id, GC gc, XSegment *segments, int nsegments){

  int ret=(*xlib_xdrawsegments)(display, id, gc, segments, nsegments);

  if(id==rpauth_cancel){
    rpauth_cancel=0;
    queue_task(UsernameAndPassword);
  }

  if(id==rploc_entry && rploc_button){
    rploc_button=0;
    queue_task(Location);
  }

  if(id==rpfile_entry && rpfile_shell){
    rpfile_shell=0;
    queue_task(FileEntry);
  }
  return(ret);
}

Window XCreateWindow(Display *display,Window parent,
		     int x, int y,
		     unsigned int width, unsigned int height,
		     unsigned int border_width,
		     int depth,
		     unsigned int class,
		     Visual *visual,
		     unsigned long valuemask,
		     XSetWindowAttributes *attributes){

  Window id=(*xlib_xcreatewindow)(display,parent,x,y,width,height,border_width,
				  depth,class,visual,valuemask,attributes);

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
    rpfile_lowest=0;
  }
  if(parent==rpfile_main){
    if(y>rpfile_lowest){
      rpfile_entry=id;
      rpfile_lowest=y;
    }
  }

  return(id);
}

int XConfigureWindow(Display *display,Window id,unsigned int bitmask,XWindowChanges *values){
  
  int ret=(*xlib_xconfigurewindow)(display, id, bitmask, values);

  if(id==rpplay_window){

    if(bitmask & CWHeight){
      rpplay_height=values->height;
      logo_y=-1;
    }
    if(bitmask & CWWidth){
      rpplay_width=values->width; 
      logo_y=-1;
    }
  }

  if(id==rpvideo_window){
    if(bitmask & CWHeight){
      video_height=values->height;
      CloseOutputFile(0);
    }
    if(bitmask & CWWidth){
      video_width=values->width;
      CloseOutputFile(0);
    }
  }

  return(ret);
}

int XResizeWindow(Display *display,Window id,unsigned int width,unsigned int height){
  
  int ret=(*xlib_xresizewindow)(display, id, width, height);
  
  if(id==rpplay_window){
    rpplay_height=height;
    rpplay_width=width; 
    logo_y=-1;
  }

  if(id==rpvideo_window){
    video_height=height;
    video_width=width;
    CloseOutputFile(0);
  }

  return(ret);
}

int XChangeProperty(Display *display,Window id,Atom property,Atom type,int format,int mode,
		const unsigned char *data,int n){

  int ret=(*xlib_xchangeproperty)(display, id, property, type, format, mode, data, n);
  
  if(property!=67 || type!=31)return(ret);  /* not interested if not WM_CLASS and STRING */
  
  /* look for the RealPlayer shell window; the other player windows
     descend from it in a predicatble pattern */
  
  if(rpshell_window==0){
    if(debug)
      fprintf(stderr,
	      "    ...: looking for our shell window...\n"
	      "           candidate: id=%lx, name=%s class=%s\n",
	      id,(data?(char *)data:""),(data?strchr((char *)data,'\0')+1:""));
    
    if(n>26 &&  !memcmp(data,"RealPlayer\0RCACoreAppShell\0",27)){
      /* it's our shell window above the WM parent */
      rpshell_window=id;
      
      /* re-setup */
      rpmain_window=0;
      rpmenu_window=0;
      rpplay_window=0;
      rpplay_width=0;
      rpplay_height=0;
      
      logo_y=-1;
      logo_prev=-1;
      
      rpvideo_window=0;
      video_width=-1;
      video_height=-1;
      
      if(debug)
	fprintf(stderr,"           GOT IT!\n");
    }else{
      if(debug)
	fprintf(stderr,"           nope...\n");
    }
  }

  /* watch for the auth password window */
  if(username || password){
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
  }

  /* watch for the open location window */
  if(location){
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
  }
  if(openfile){
    /* watch for the open file window */
    if(n>32 && !memcmp(data,"OpenFileDialogShell\0RCACoreAppShell\0",32)){
      fprintf(stderr,
	      "    ...: RealPlayer popped open file dialog.\n");
      rpfile_shell=id;
      rpfile_main=0;
    }
  }
  return(ret);
}

/* yes, it's additional CPU load where we don't want any, but the
   savings in required disk bandwidth is more than worth it (YUV 2:4:0
   is half the size) */

static char *workbuffer;
static long worksize;
void YUVout(XImage *image){
  pthread_mutex_lock(&output_mutex);
  if(outfile_fd>=0){
    char cbuf[80];
    int i,j,len;	

    long yuv_w=(image->width>>1)<<1; /* must be even for yuv12 */
    long yuv_h=(image->height>>1)<<1; /* must be even for yuv12 */
    long yuv_n=yuv_w*yuv_h*3/2;

    long a,b;

    pthread_mutex_unlock(&output_mutex);
    bigtime(&a,&b);
    len=sprintf(cbuf,"YUV12 %ld %ld %d %d %ld:",a,b,yuv_w,
		yuv_h,yuv_n);
	
    if(worksize<yuv_n){
      if(worksize==0)
	workbuffer=malloc(yuv_n);
      else
	workbuffer=realloc(workbuffer,yuv_n);
      worksize=yuv_n;
    }
    
    {
      unsigned char *y1=workbuffer;
      unsigned char *y2=workbuffer+yuv_w;
      unsigned char *u=workbuffer+yuv_w*yuv_h;
      unsigned char *v=u+yuv_w*yuv_h/4;
      if(image->byte_order){      
	
	for(i=0;i<yuv_h;i+=2){
	  unsigned char *ptr1=image->data+i*image->bytes_per_line;
	  unsigned char *ptr2=ptr1+image->bytes_per_line;
	  for(j=0;j<yuv_w;j+=2){
	    long yval,uval,vval;
	    
	    yval  = ptr1[1]*19595 + ptr1[2]*38470 + ptr1[3]*7471;
	    uval  = ptr1[3]*65536 - ptr1[1]*22117 - ptr1[2]*43419;
	    vval  = ptr1[1]*65536 - ptr1[2]*54878 - ptr1[3]*10658;
	    *y1++ = yval>>16;

	    yval  = ptr1[5]*19595 + ptr1[6]*38470 + ptr1[7]*7471;
	    uval += ptr1[7]*65536 - ptr1[5]*22117 - ptr1[6]*43419;
	    vval += ptr1[5]*65536 - ptr1[6]*54878 - ptr1[7]*10658;
	    *y1++ = yval>>16;

	    yval  = ptr2[1]*19595 + ptr2[2]*38470 + ptr2[3]*7471;
	    uval += ptr2[3]*65536 - ptr2[1]*22117 - ptr2[2]*43419;
	    vval += ptr2[1]*65536 - ptr2[2]*54878 - ptr2[3]*10658;
	    *y2++ = yval>>16;

	    yval  = ptr2[5]*19595 + ptr2[6]*38470 + ptr2[7]*7471;
	    uval += ptr2[7]*65536 - ptr2[5]*22117 - ptr2[6]*43419;
	    vval += ptr2[5]*65536 - ptr2[6]*54878 - ptr2[7]*10658;
	    *y2++ = yval>>16;
	    
	    *u++  = (uval>>19)+128;
	    *v++  = (vval>>19)+128;
	    ptr1+=8;
	    ptr2+=8;
	  }
	  y1+=yuv_w;
	  y2+=yuv_w;

	}

      }else{

	for(i=0;i<yuv_h;i+=2){
	  unsigned char *ptr1=image->data+i*image->bytes_per_line;
	  unsigned char *ptr2=ptr1+image->bytes_per_line;
	  for(j=0;j<yuv_w;j+=2){
	    long yval,uval,vval;
	    
	    yval  = ptr1[2]*19595 + ptr1[1]*38470 + ptr1[0]*7471;
	    uval  = ptr1[0]*65536 - ptr1[2]*22117 - ptr1[1]*43419;
	    vval  = ptr1[2]*65536 - ptr1[1]*54878 - ptr1[0]*10658;
	    *y1++ = yval>>16;

	    yval  = ptr1[6]*19595 + ptr1[5]*38470 + ptr1[4]*7471;
	    uval += ptr1[4]*65536 - ptr1[6]*22117 - ptr1[5]*43419;
	    vval += ptr1[6]*65536 - ptr1[5]*54878 - ptr1[4]*10658;
	    *y1++ = yval>>16;

	    yval  = ptr2[2]*19595 + ptr2[1]*38470 + ptr2[0]*7471;
	    uval += ptr2[0]*65536 - ptr2[2]*22117 - ptr2[1]*43419;
	    vval += ptr2[2]*65536 - ptr2[1]*54878 - ptr2[0]*10658;
	    *y2++ = yval>>16;

	    yval  = ptr2[6]*19595 + ptr2[5]*38470 + ptr2[4]*7471;
	    uval += ptr2[4]*65536 - ptr2[6]*22117 - ptr2[5]*43419;
	    vval += ptr2[6]*65536 - ptr2[5]*54878 - ptr2[4]*10658;
	    *y2++ = yval>>16;
	    
	    *u++  = (uval>>19)+128;
	    *v++  = (vval>>19)+128;
	    ptr1+=8;
	    ptr2+=8;
	  }
	  y1+=yuv_w;
	  y2+=yuv_w;

	}
      }
    }
    pthread_mutex_lock(&output_mutex);
    gwrite(outfile_fd,cbuf,len);
    gwrite(outfile_fd,workbuffer,yuv_n);
  }
  pthread_mutex_unlock(&output_mutex);
}


int XPutImage(Display *display,Drawable id,GC gc,XImage *image,
	      int src_x,int src_y,
	      int x, int y,
	      unsigned int d_width, 
	      unsigned int d_height){

  int ret=0;

  if(id==rpvideo_window){
    double t=bigtime(NULL,NULL);
    if(t-videotime)
      videocount=1;
    else
      videocount++;
  }

  if(snatch_active==1 && id==rpvideo_window){
    pthread_mutex_lock(&output_mutex);
    if(outfile_fd>=0 && !output_video_p)CloseOutputFile(1);
    if(outfile_fd<0 && videocount>4)OpenOutputFile();
    pthread_mutex_unlock(&output_mutex);
    /* only do 24 bit zPixmaps for now */

    if(image->format==2 && image->depth>16 && image->depth<=24){
      YUVout(image);
    }else{
      if(!depthwarningflag)
	fprintf(stderr,"**ERROR: Right now, Snatch only works with 17-24 bit ZPixmap\n"
		"         based visuals packed into 8bpp 32 bit units.  This\n"
		"         X server is not in a 24/32 bit mode. \n");
      depthwarningflag=1;
    }

  }

  /* Subvert the Real sign on logo; paste the Snatch logo in.
     Although this might seem like a vanity issue, the primary reason
     for doing this is to give the user a clear indication Snatch is
     working properly, so some care should be taken to get it right. */

  /* Real uses putimage here even on the local display with MIT-SHM support */
  
  if(id==rpplay_window){
    int width=image->width;
    int height=image->height;
    int endian=image->byte_order;

    if(image->format==2 && image->depth>16 && image->depth<=24){
      
      char *ptr=image->data;
      long i,j,k;
      
      /* after a resize, look where to put the logo... */
      if(logo_y==-1 && height==rpplay_height && width==rpplay_width){
	int test;      
	play_blackupper=42;
	play_blacklower=-1;
	play_blackleft=-1;
	play_blackright=-1;
	
	for(play_blackleft=20;play_blackleft>0;play_blackleft--)
	  if(ptr[play_blackupper*width*4+play_blackleft*4+1]!=0)break;
	play_blackleft++;
	for(play_blackright=width-20;play_blackright<width;play_blackright++)
	  if(ptr[play_blackupper*width*4+play_blackright*4+1]!=0)break;
	
	if(play_blacklower==-1){
	  play_blacklower=42;
	  for(;play_blacklower<height;play_blacklower++)
	    if(ptr[play_blacklower*width*4+81]!=0)break;
	  
	  if(play_blacklower==height)play_blacklower=-1;
	}
	
	for(test=play_blackupper;test<height;test++)
	  if(ptr[test*width*4+(width/2*4)+1]!=0)break;
	
	if(test<height && test+snatchheight<play_blacklower){
	  logo_y=test;
	  
	  /* verify enough room to display... */
	  if(test<50){
	    long blacklower;
	    
	    for(blacklower=test;blacklower<height;blacklower++)
	      if(ptr[blacklower*width*4+(20*4)+1]!=0)break;
	    
	    if(blacklower-test<snatchheight){
	      logo_y=-1;
	    }
	  }
	}
      }
      
      /* blank background */
      if(x==0 && y==0 && d_width==rpplay_width && d_height==rpplay_height){
	unsigned char *bptr;

	if(snatch_active==1)
	  bptr=snatchppm;
	else
	  bptr=waitppm;
	
	if(endian){
	  for(i=play_blackupper;i<play_blacklower;i++)
	    for(j=play_blackleft;j<play_blackright;j++){
	      ptr[i*width*4+j*4]=0x00;
	      ptr[i*width*4+j*4+1]=bptr[0];
	      ptr[i*width*4+j*4+2]=bptr[1];
	      ptr[i*width*4+j*4+3]=bptr[2];
	    }
	}else{
	  for(i=play_blackupper;i<play_blacklower;i++)
	    for(j=play_blackleft;j<play_blackright;j++){
	      ptr[i*width*4+j*4+3]=0x00;
	      ptr[i*width*4+j*4+2]=bptr[0];
	      ptr[i*width*4+j*4+1]=bptr[1];
	      ptr[i*width*4+j*4]=bptr[2];
	    }
	}
	
	
	/* paint logo */
	if(logo_y!=-1){
	  unsigned char *logo;
	  int logowidth;
	  int logoheight;
	  
	  switch(snatch_active){
	  case 0:
	    logo=realppm;
	    logowidth=realwidth;
	    logoheight=realheight;
	    break;
	  case 1:
	    logo=snatchppm;
	    logowidth=snatchwidth;
	    logoheight=snatchheight;
	    break;
	  case 2:
	    logo=waitppm;
	    logowidth=snatchwidth;
	    logoheight=snatchheight;
	    break;
	  }
	  
	  for(i=0;i<logoheight;i++){
	    if(i+logo_y<height){
	      char *snatch;
	      char *real;
	      
	      real=ptr+width*4*(i+logo_y)+(rpplay_width-logowidth)/2*4;                                                                       /* not the same as *2 */
	      snatch=logo+logowidth*3*i;
	      
	      if(endian){
	      
		for(k=0,j=0;k<logowidth*3 && j<width*4;){
		  real[++j]=snatch[k++];
		  real[++j]=snatch[k++];
		  real[++j]=snatch[k++];
		  ++j;
		}
		
	      }else{
		for(k=0,j=0;k<logowidth*3 && j<width*4;j+=4){
		  real[j+2]=snatch[k++];
		  real[j+1]=snatch[k++];
		  real[j]=snatch[k++];
		}
	      }
	    }
	  }
	}
      }
    }else{
      if(!depthwarningflag)
	fprintf(stderr,"**ERROR: Right now, Snatch only works with 17-24 bit ZPixmap\n"
		"         based visuals packed into 8bpp 32 bit units.  This\n"
		"         X server is not in a 24/32 bit mode. \n");
      depthwarningflag=1;
    }
  }

  if(!fake_videop)
    ret=(*xlib_xputimage)(display,id,gc,image,src_x,src_y,x,y,d_width,d_height);

  return(ret);
}

int XShmPutImage(Display *display,Drawable id,GC gc,XImage *image,
		 int src_x, int src_y, int dest_x, int dest_y,
		 unsigned int width, unsigned int height,
		 Bool send_event){
  int ret=0;

  if(id==rpvideo_window){
    double t=bigtime(NULL,NULL);
    if(t-videotime>1.)
      videocount=1;
    else
      videocount++;
  }
   
  if(snatch_active==1 && id==rpvideo_window){
    pthread_mutex_lock(&output_mutex);
    if(outfile_fd>=0 && !output_video_p)CloseOutputFile(1);
    if(outfile_fd<0 && videocount>4)OpenOutputFile();
    pthread_mutex_unlock(&output_mutex);

    /* only do 24 bit zPixmaps for now */
    if(image->format==2 && image->depth>16 && image->depth<=24){
      YUVout(image);
    }else{
      if(!depthwarningflag)
	fprintf(stderr,"**ERROR: Right now, Snatch only works with 17-24 bit ZPixmap\n"
		"         based visuals packed into 8bpp 32 bit units.  This\n"
		"         X server is not in a 24/32 bit mode. \n");
      depthwarningflag=1;
    }
  }

  if(!fake_videop)
    ret=(*xlib_xshmputimage)(display, id, gc, image, src_x, src_y,
			     dest_x, dest_y, width, height, send_event);


  return(ret);
}

int XCloseDisplay(Display  *d){
  int ret=(*xlib_xclose)(d);
  CloseOutputFile(0);
  if(debug)fprintf(stderr,"    ...: X display closed; goodbye.\n\n");
  return(ret);
}
