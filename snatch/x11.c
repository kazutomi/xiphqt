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
static long rpplay_width=0;
static long rpplay_height=0;

static long logo_y=-1;
static long logo_prev=-1;

static long play_blackleft=-1;
static long play_blackright=-1;
static long play_blackupper=-1;
static long play_blacklower=-1;

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

  if((int)XKeycodeToKeysym(Xdisplay,c,0)==keysym){
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

    if(n>=32 &&  !memcmp(data,"AuthDialogShell\0RCACoreAppShell\0",32)){
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
    if(n>=40 &&  !memcmp(data,"OpenLocationDialogShell\0RCACoreAppShell\0",40)){
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
    if(n>=32 && !memcmp(data,"OpenFileDialogShell\0RCACoreAppShell\0",32)){
      fprintf(stderr,
	      "    ...: RealPlayer popped open file dialog.\n");
      rpfile_shell=id;
      rpfile_main=0;
    }
  }
  return(ret);
}

static unsigned int crc_lookup[256]={
  0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,
  0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
  0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,
  0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
  0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,
  0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
  0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,
  0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
  0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,
  0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
  0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,
  0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
  0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,
  0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
  0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,
  0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
  0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,
  0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
  0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,
  0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
  0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,
  0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
  0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,
  0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
  0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,
  0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
  0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,
  0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
  0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,
  0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
  0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,
  0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
  0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,
  0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
  0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,
  0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
  0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,
  0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
  0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,
  0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
  0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,
  0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
  0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,
  0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
  0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,
  0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
  0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,
  0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
  0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,
  0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
  0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,
  0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
  0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,
  0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
  0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,
  0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
  0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,
  0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
  0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,
  0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
  0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,
  0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
  0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,
  0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4};

/* yes, it's additional CPU load where we don't want any, but the
   savings in required disk bandwidth is more than worth it (YUV 2:4:0
   is half the size) */
/* also, hash the image: Real (for some reason) appears to blit at
   30fps regardless of video framerate, so there are lotsa dupes.
   Hash to see if the frame actually changed.  Again, CPU overhead is
   likely more than made up for in bandwidth savings */

static char *workbuffer;
static long worksize;
static unsigned int saved_hash=0;

static unsigned int zimage_hash(XImage *image){
  long bytes=image->bytes_per_line*image->height;
  unsigned char *data=(unsigned char *)image->data;
  unsigned int crc_reg=0;
  int i;

  for(i=0;i<bytes;i++)
    crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)&0xff)^data[i]];
  return crc_reg;
}

void YUVout(XImage *image){
  unsigned int hash=zimage_hash(image);
  if(hash!=saved_hash){
    saved_hash=hash;
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
  }else{
    fprintf(stderr,"Dupe!\n");
  }
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
      if(x==0 && y==0 && (int)d_width==rpplay_width && 
	 (int)d_height==rpplay_height){
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
	  unsigned char *logo=NULL;
	  int logowidth=-1;
	  int logoheight=-1;
	  
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
