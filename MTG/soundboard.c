/* Assumptions (Bad things the code does that work here, but might not
   work elsewhere):

   the atomic execution sections are a hack that can only work on uniprocessor

*/

/* TODO:

   make Esc behavior correct in editing menus by using temp cue/tag storage
   tag list menu
   sane arrow keying
   autocompletion
   logarhythmic fades
   proper printing out of file loading issues
   abstracted output
   allow sample name/tag change in edit menus */
#define CACHE_SAMPLES 44100*10
#define _REENTRANT 1
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/file.h>
#define __USE_GNU 1
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <curses.h>
#include <fcntl.h>

#include <sys/soundcard.h>
#include <sys/ioctl.h>

/* we need some cooperative multitasking to eliminate locking (and
   thus latency) in the realtime playback thread.
   pthread_setschedparam can give us this by way of implementing
   atomic execution blocks.  Convention here is that an 'atomic
   execution block' is actually a section wrapped in a realtime
   schedule change with a priority of 90; to be effective, this must
   be the highest realtime priority used in the application */

static int original_priority;
static int original_policy;
#define BEGIN_ATOMIC \
   { \
     struct sched_param param; \
     pthread_getschedparam(pthread_self(), &original_policy, &param); \
     if(param.sched_priority==90){ \
       fprintf(stderr,"ATOMIC sections do not stack at line %d\n",__LINE__); \
       exit(1); \
     } \
     original_priority=param.sched_priority; \
     param.sched_priority=90; \
     pthread_setschedparam(pthread_self(), SCHED_FIFO, &param); \
   }

#define END_ATOMIC \
   { \
     struct sched_param param; \
     param.sched_priority=original_priority; \
     pthread_setschedparam(pthread_self(), original_policy, &param); \
   }



#define int16 short
static char *tempdir="/tmp/beaverphonic/";
static char *lockfile="/tmp/beaverphonic/lock";
//static char *installdir="/usr/local/beaverphonic/";
static char *installdir="/home/xiphmont/MotherfishCVS/MTG/";
#define VERSION "$Id: soundboard.c,v 1.13 2003/10/02 17:11:58 xiphmont Exp $"

/******** channel mappings.  All hardwired for now... ***********/
// only OSS stereo builin for now
#define MAX_CHANNELS 4

#define OUTPUT_CHANNELS1 4
#define OUTPUT_CHANNELS2 0

#define INPUT_CHANNELS1 0
#define INPUT_CHANNELS2 0
#define INPUT_SAMPLE_BYTES 2
#define INPUT_SAMPLE_FMT AFMT_S16_LE


#define MAX_INPUT_CHANNELS (INPUT_CHANNELS1+INPUT_CHANNELS2)
#define MAX_OUTPUT_CHANNELS (OUTPUT_CHANNELS1+OUTPUT_CHANNELS2)

#define MAX_FILECHANNELS 2
#define CHANNEL_LABEL_LENGTH 50

int playback_bufsize=0;

typedef struct {
  char label[CHANNEL_LABEL_LENGTH];
  int  peak;
  /* real stuff not here yet */
} outchannel;
  
static outchannel channel_list[MAX_OUTPUT_CHANNELS]={
  {"house L",0},
  {"center L",0},
  {"center R",0},
  {"house R",0},
  {"",0},
  {"",0},
  {"",0},
  {"",0},
};
static outchannel rchannel_list[8]={
  {"1",0},
  {"2",0},
  {"3",0},
  {"4",0},
  {"5",0},
  {"6",0},
  {"7",0},
  {"8",0},
};

enum menutype {MENU_MAIN,MENU_KEYPRESS,MENU_ADD,MENU_EDIT,MENU_OUTPUT,MENU_QUIT};

pthread_mutex_t cache_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_mutex_t rec_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_mutex_t mmap_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_cond_t cache_cond=PTHREAD_COND_INITIALIZER;

static long main_master_volume=50;
char *program;
static int playback_buffer_minfill=0;
static int running=1;
static enum menutype menu=MENU_MAIN;
static int cue_list_position=0;
static int cue_list_number=0;
static int firstsave=0;
static int unsaved=0;

static char *playdevice1="/dev/dsp";
static char *playdevice2="/dev/dsp2";
static FILE *playfd1=NULL;
static FILE *playfd2=NULL;

static char *recdevice1="/dev/dsp1";
static char *recdevice2="/dev/dsp2";
static FILE *recfd1=NULL;
static FILE *recfd2=NULL;
int ttyfd;
int ttypipe[2];

static inline void _playback_remove(int i);
static void _cache_remove(int i);

pthread_t main_thread_id;
pthread_t playback_thread_id;
pthread_t record_thread_id;
pthread_t record_disk_thread_id;
pthread_t tty_thread_id;
pthread_t cache_thread_id;

void main_update_tags(int y);

void *m_realloc(void *in,int bytes){
  if(!in)
    return(malloc(bytes));
  return(realloc(in,bytes));
}

void addnlstr(const char *s,int n,char c){
  int len=strlen(s),i;
  addnstr(s,n);
  n-=len;
  for(i=0;i<n;i++)addch(c);
}

void switch_to_stderr(){
  def_prog_mode();           /* save current tty modes */
  endwin();                  /* restore original tty modes */
}

void switch_to_ncurses(){
  refresh();                 /* restore save modes, repaint screen */
}

int mgetch(){
  while(1){
    int ret=getch();
    if(ret>0)return(ret);
  }
}


/******** label abstraction code; use this for all alloced strings
          that need to be saved to config file (shared or not) */

typedef struct {
  char *text;
  int   refcount;
} label;

typedef int label_number;

static label      *label_list;
static int         label_count;

int new_label_number(){
  int i;
  for(i=0;i<label_count;i++)
    if(!label_list[i].refcount)break;
  return(i);
}

static void _alloc_label_if_needed(int number){
  if(number>=label_count){
    int prev=label_count;
    label_count=number+1;
    label_list=m_realloc(label_list,sizeof(*label_list)*label_count);
    memset(label_list+prev,0,sizeof(*label_list)*(label_count-prev));
  }
}

void edit_label(int number,char *t){
  label *label;
  _alloc_label_if_needed(number);

  label=label_list+number;
  label->text=m_realloc(label->text,strlen(t)+1);
  
  strcpy(label->text,t);
}

void acquire_label(int number){
  _alloc_label_if_needed(number);
  label_list[number].refcount++;
}

const char *label_text(int number){
  if(number<0 || 
     number>=label_count || 
     label_list[number].refcount==0 ||
     label_list[number].text==NULL)
    return"";
  return label_list[number].text;
}

void release_label(int number){
  label *label;
  if(number>=label_count)return;
  label=label_list+number;
  label->refcount--;
  if(label->refcount<0)
    label->refcount=0;
}

int save_label(FILE *f,int number){
  int count;
  if(number<=label_count){
    if(label_list[number].refcount){
      fprintf(f,"LBL:%d %d:%s",number,
	      strlen(label_text(number)),
	      label_text(number));
      
      count=fprintf(f,"\n");
      if(count<1)return(-1);
    }
  }
  return(0);
}

int save_labels(FILE *f){
  int i;
  for(i=label_count-1;i>=0;i--)
    if(save_label(f,i))return(-1);
  return(0);
}

int load_label(FILE *f){
  int len,count,number;
  char *temp;
  count=fscanf(f,":%d %d:",&number,&len);
  if(count<2){
    addnlstr("LOAD ERROR (LBL): too few fields.",80,' ');
    return(-1);
  }
  temp=alloca(len+1);
  count=fread(temp,1,len,f);
  temp[len]='\0';
  if(count<len){
    addnlstr("LOAD ERROR (LBL): EOF reading string.",80,' ');    
    return(-1);
  }
  edit_label(number,temp);
  return(0);
}

/************* tag abstraction **********************/

typedef int tag_number;

typedef struct {
  int          refcount;

  label_number sample_path;
  label_number sample_desc;
  long   loop_p;
  long   loop_start;
  long   loop_lapping;
  long   fade_out;

  void  *basemap;
  int16 *data;
  int    channels;
  long   samplelength;

  /* state */
  int    activep;
  int    cachep;
  
  long   sample_position;
  long   sample_lapping;
  long   sample_loop_start;
  long   sample_fade_start;

  long   cache_fill;

  double master_vol_current;
  double master_vol_target;
  double master_vol_slew;

  double outvol_current[MAX_FILECHANNELS][MAX_OUTPUT_CHANNELS];
  double outvol_target[MAX_FILECHANNELS][MAX_OUTPUT_CHANNELS];
  double outvol_slew[MAX_FILECHANNELS][MAX_OUTPUT_CHANNELS];

} tag;

struct sample_header{
  long sync;
  long channels;
};

static tag        *tag_list;
static int         tag_count;

static tag       **active_list;
static int         active_count;

static tag       **cache_list;
static int         cache_count;

int new_tag_number(){
  int i;
  for(i=0;i<tag_count;i++)
    if(!tag_list[i].refcount)break;
  return(i);
}

static void _alloc_tag_if_needed(int number){
  if(number>=tag_count){
    int prev=tag_count;
    BEGIN_ATOMIC
    tag_count=number+1;
    tag_list=m_realloc(tag_list,sizeof(*tag_list)*tag_count);
    active_list=m_realloc(active_list,sizeof(*active_list)*tag_count);
    cache_list=m_realloc(cache_list,sizeof(*cache_list)*tag_count);
    
    memset(tag_list+prev,0,sizeof(*tag_list)*(tag_count-prev));
    END_ATOMIC
  }
}

  /* UI convention: this has too many useful things to report if
     sample opening goes awry, and I don't want to define a huge
     message reporting system through ncurses in C, so we go back to
     tty-like for reporting here, waiting for a keypress if nonzero
     return */

int load_sample(tag *t,const char *path){
  char *template=alloca(strlen(tempdir)+20);
  char *tmp,*buffer;
  int ret=0;

  fprintf(stderr,"Loading %s...\n",path);
  /* parse the file; use preexisting tools via external Perl glue */
  /* select a temp file for the glue to convert file into */
  /* valid use of mktemp; only one Beaverphonic is running a time,
     and we need a *name*, not a *file*. */

  sprintf(template,"%sconversion.XXXXXX",tempdir);
  tmp=mktemp(template);
  if(!tmp)return(-1);

  buffer=alloca(strlen(installdir)+strlen(tmp)+strlen(path)+20);  
  fprintf(stderr,"\tcopying/converting...\n");
  sprintf(buffer,"%sconvert.pl \'%s\' \'%s\'",installdir,path,tmp);

  if(fork()){
    wait(&ret);
  }else{
    setuid(getuid());
    _exit(system(buffer));
  }
  if(ret)goto out;
  
  /* our temp file is a host-ordered 44.1kHz 16 bit PCM file, n
     channels uninterleaved.  first word is channels */
  fprintf(stderr,"\treading...\n");
  {
    FILE *cheat=fopen(tmp,"rb");
    struct sample_header head;
    int count;
    int fd;
    long length;
    if(!cheat){
      fprintf(stderr,"Failed to open converted file %s:\n\t(%s)\n",
	      tmp,strerror(errno));
      ret=-1;goto out;
    }

    /* find length,etc */
    if(fseek(cheat,-sizeof(head),SEEK_END)){
      fprintf(stderr,"Unable to seek to end of file!\n\t%s\n",
	      strerror(ferror(cheat)));
      fclose(cheat);
      ret=-1;goto out;
    }
    length=ftell(cheat);
    if(length<0){
      fprintf(stderr,"Unable to determine length of file!\n\t%s\n",
	      strerror(ferror(cheat)));
      fclose(cheat);
      ret=-1;goto out;
    }
    count=fread(&head,sizeof(head),1,cheat);

    fclose(cheat);
    if(count<1){
      fprintf(stderr,"Conversion file %s openable, but truncated\n",tmp);
      ret=-1;goto out;
    }
    if(head.sync!=55){
      fprintf(stderr,"Conversion file created by incorrect "
	      "version of convert.pl\n\t%s unreadable\n",tmp);
      ret=-1;goto out;
    }
    
    t->channels=head.channels;
    t->samplelength=(length-sizeof(head))/(t->channels*2);

    /* mmap the sample file */
    fprintf(stderr,"\tmmaping...\n");
    fd=open(tmp,O_RDONLY);
    if(fd<0){
      fprintf(stderr,"Unable to open %s fd for mmap:\n\t%d (%s)\n",
	      tmp,errno,strerror(errno));
      ret=-1;goto out;
    }
    t->basemap=
      mmap(NULL,
	   t->samplelength*sizeof(int16)*t->channels+sizeof(head),
	   PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);

    if(!t->basemap){
      fprintf(stderr,"Unable to mmap fd %d (%s):\n\t%d (%s)\n",
	      fd,tmp,errno,strerror(errno));
      ret=-1;goto out;
    }
    t->data=t->basemap+sizeof(head);
    if(madvise(t->basemap, 
	       t->samplelength*sizeof(int16)*t->channels+sizeof(head),
	       MADV_RANDOM)){
      fprintf(stderr,"madvise() failed for %s mmap:\n\t%d (%s)\n",
	      tmp,errno,strerror(errno));
      ret=-1;goto out;
    }
  }
  fprintf(stderr,"\tDONE\n\n");
  
 out:
  if(tmp)unlink(tmp);
  return(ret);
}

int unload_sample(tag *t){
  /* is this sample currently playing back? */
  int i;
  if(t->activep){
    for(i=0;i<active_count;i++)
      if(active_list[i]==t)_playback_remove(i);
  }
  if(t->cachep){
    for(i=0;i<cache_count;i++)
      if(cache_list[i]==t)_cache_remove(i);
  }

  pthread_mutex_lock(&mmap_mutex);
  if(t->basemap)
    munmap(t->basemap,
	   t->samplelength*sizeof(int16)*
	   t->channels+sizeof(struct sample_header));

  t->basemap=NULL;
  t->data=NULL;
  t->samplelength=0;
  t->channels=0;
  pthread_mutex_unlock(&mmap_mutex);

  return(0);
}

int edit_tag(int number,tag t){
  _alloc_tag_if_needed(number);

  tag_list[number]=t;

  /* state is zeroed when we go to production mode.  Done  */
  return(0);
}

void acquire_tag(int number){
  if(number<0)return;
  _alloc_tag_if_needed(number);
  tag_list[number].refcount++;
}

void release_tag(int number){
  if(number<0)return;
  if(number>=tag_count)return;
  tag_list[number].refcount--;
  if(tag_list[number].refcount==0){
    tag *t=tag_list+number;

    if(t->basemap)
      unload_sample(t); /* <- locked here; this gets it off the play
                           list and eliminates any playback thread
                           interdependancies; it can't get back on
                           active play list if we're here */
    release_label(t->sample_path);
    release_label(t->sample_desc);
  }
  if(tag_list[number].refcount<0)
    tag_list[number].refcount=0;
}

int save_tag(FILE *f,int number){
  tag *t;
  int count;

  if(number>=tag_count)return(0);
  t=tag_list+number;
  if(t->refcount){
    fprintf(f,"TAG:%d %d %d %ld %ld %ld %ld",
	    number,t->sample_path,t->sample_desc,
	    t->loop_p,t->loop_start,t->loop_lapping,
	    t->fade_out);
    count=fprintf(f,"\n");
    if(count<1)return(-1);
  }
  return(0);
}

int save_tags(FILE *f){
  int i;
  for(i=tag_count-1;i>=0;i--){
    if(save_tag(f,i))return(-1);
  }
  return(0);
}

int load_tag(FILE *f){
  tag t;
  int count,number;
  memset(&t,0,sizeof(t));
  
  count=fscanf(f,":%d %d %d %ld %ld %ld %ld",
	       &number,&t.sample_path,&t.sample_desc,&t.loop_p,&t.loop_start,
	       &t.loop_lapping,&t.fade_out);
  if(count<7){
    addnlstr("LOAD ERROR (TAG): too few fields.",80,' ');
    return(-1);
  }
  acquire_label(t.sample_path);
  if(load_sample(&t,label_text(t.sample_path))){
    release_label(t.sample_path);
    return(-1);
  }
  edit_tag(number,t);
  acquire_label(t.sample_desc);

  return(0);
}

/********************* cue abstraction ********************/

typedef struct {
  int  vol_master;
  long vol_ms;
  
  int  outvol[MAX_FILECHANNELS][MAX_OUTPUT_CHANNELS];
} mix;

typedef struct {
  label_number label;
  label_number cue_text;
  label_number cue_desc;

  tag_number   tag;
  int          tag_create_p; 
  mix          mix;
} cue;

static cue        *cue_list;
int                cue_count;
int                cue_storage;

void add_cue(int n,cue c){

  if(cue_count==cue_storage){
    BEGIN_ATOMIC
    cue_storage++;
    cue_storage*=2;
    cue_list=m_realloc(cue_list,cue_storage*sizeof(*cue_list));
    END_ATOMIC
  }

  /* copy insert */
  BEGIN_ATOMIC
  if(cue_count-n)
    memmove(cue_list+n+1,cue_list+n,sizeof(*cue_list)*(cue_count-n));
  cue_count++;

  cue_list[n]=c;
  END_ATOMIC
}

/* inefficient.  delete one, shift-copy list, delete one, shift-copy
   list, delete one, gaaaah.  Not worth optimizing */
static void _delete_cue_helper(int n){
  if(n>=0 && n<cue_count){
    BEGIN_ATOMIC
    release_label(cue_list[n].label);
    release_label(cue_list[n].cue_text);
    release_label(cue_list[n].cue_desc);
    release_tag(cue_list[n].tag);
    cue_count--;
    if(n<cue_count)
      memmove(cue_list+n,cue_list+n+1,sizeof(*cue_list)*(cue_count-n));
    END_ATOMIC
  }
}

/* slightly more complicated that just removing the cue from memory;
   we need to remove any tag mixer modification cues that follow if
   this cue creates a sample tag */

void delete_cue_single(int n){
  int i;
  if(n>=0 && n<cue_count){
    cue *c=cue_list+n;
    tag_number   tag=c->tag;

    if(c->tag_create_p){
      /* tag creation cue; have to delete following cues matching this
         tag */
      for(i=cue_count-1;i>n;i--)
	if(cue_list[i].tag==tag)
	  _delete_cue_helper(i);
    }
    _delete_cue_helper(n);
  }
}

/* this deletes all cues of a cue bank, and also chases the tags of
   sample creation cues */
void delete_cue_bank(int n){
  /* find first cue number */
  int first=n,last=n;

  while(first>0 && cue_list[first].label==cue_list[first-1].label)first--;
  while(last+1<cue_count && 
	cue_list[last].label==cue_list[last+1].label)last++;

  for(;last>=first;last--)
    delete_cue_single(last);
}
	    
int save_cue(FILE *f,int n){
  if(n<cue_count){
    int count,i;
    cue *c=cue_list+n;
    fprintf(f,"CUE:%d %d %d %d %d %d %ld :%d:%d:",
	    c->label,c->cue_text,c->cue_desc,
	    c->tag,c->tag_create_p,
	    c->mix.vol_master,
	    c->mix.vol_ms,
	    MAX_OUTPUT_CHANNELS,
	    MAX_FILECHANNELS);
    
    for(i=0;i<MAX_OUTPUT_CHANNELS;i++)
      fprintf(f,"<%d %d>",c->mix.outvol[0][i],c->mix.outvol[1][i]);
    count=fprintf(f,"\n");
    if(count<1)return(-1);
  }
  return(0);
}

int save_cues(FILE *f){
  int i;
  for(i=0;i<cue_count;i++)
    if(save_cue(f,i))return(-1);
  return(0);
}

int load_cue(FILE *f){
  cue c;
  int count,i,j;
  int maxchannels,maxwavch;
  memset(&c,0,sizeof(c));
  
  count=fscanf(f,":%d %d %d %d %d %d %ld :%d:%d:",
	       &c.label,&c.cue_text,&c.cue_desc,
	       &c.tag,&c.tag_create_p,
	       &c.mix.vol_master,
	       &c.mix.vol_ms,
	       &maxchannels,&maxwavch);
  if(count<9){
    addnlstr("LOAD ERROR (CUE): too few fields.",80,' ');
    return(-1);
  }
  if(c.tag!=-1){
    if(c.tag>=tag_count ||
       !tag_list[c.tag].basemap){
      addnlstr("LOAD ERROR (CUE): references a bad TAG",80,' ');
      return(-1);
    }
  }

  for(i=0;i<maxchannels;i++){
    int v;
    char ch=' ';
    count=fscanf(f,"%c",&ch);
    if(count<0 || ch!='<'){
      addnlstr("LOAD ERROR (CUE): parse error looking for '<'.",80,' ');
      return(-1);
    }
    for(j=0;j<maxwavch;j++){
      count=fscanf(f,"%d",&v);
      if(count<1){
	addnlstr("LOAD ERROR (CUE): parse error looking for value.",80,' ');
	return(-1);
      }
      if(j<MAX_FILECHANNELS)
	c.mix.outvol[j][i]=v;
    }
    count=fscanf(f,"%c",&ch);
    if(count<0 || ch!='>'){
      addnlstr("LOAD ERROR (CUE): parse error looking for '>'.",80,' ');
      return(-1);
    }
  }
  
  acquire_label(c.label);
  acquire_label(c.cue_text);
  acquire_label(c.cue_desc);

  acquire_tag(c.tag);
  add_cue(cue_count,c);

  return(0);
}

int load_val(FILE *f, long *val){
  int count;
  long t;  
  count=fscanf(f,": %ld",&t);
  if(count<1){
    addnlstr("LOAD ERROR (VAL): missing field.",80,' ');
    return(-1);
  }
  *val=t;
  return(0);
}

int save_val(FILE *f, char *p, long val){  
  int count;
  fprintf(f,"%s: %ld",p,val);
  count=fprintf(f,"\n");
  if(count<1)return(-1);
  return(0);
}

int load_program(FILE *f){
  char buf[3];
  int ret=0,count;

  move(0,0);
  attron(A_BOLD);

  while(1){
    if(fread(buf,1,3,f)<3){
      addnlstr("LOAD ERROR: truncated program.",80,' ');
      ret=-1;
      break;
    }
    if(!strncmp(buf,"TAG",3)){
      ret|=load_tag(f);
    }else if(!strncmp(buf,"CUE",3)){
      ret|=load_cue(f);
    }else if(!strncmp(buf,"LBL",3)){
      ret|=load_label(f);
    }else if(!strncmp(buf,"MAS",3)){
      ret|=load_val(f,&main_master_volume);
    }
    
    while(1){
      count=fgetc(f);
      if(count=='\n' || count==EOF)break;
    }
    if(count==EOF)break;
    while(1){
      count=fgetc(f);
      if(count!='\n' || count==EOF)break;
    }
    if(count!=EOF)
      ungetc(count,f);
    if(count==EOF)break;
  }

  attroff(A_BOLD);
  if(ret)mgetch();
  return(ret);
}
   
int save_program(FILE *f){
  int ret=0;
  ret|=save_labels(f);
  ret|=save_tags(f);
  ret|=save_cues(f);
  ret|=save_val(f,"MAS",main_master_volume);
  return ret;
}

#if (MAX_INPUT_CHANNELS)

/*************** threaded record ****************************/

#define REC_BLOCK1 (INPUT_CHANNELS1 * INPUT_SAMPLE_BYTES * 512) 
#define REC_BLOCK2 (INPUT_CHANNELS2 * INPUT_SAMPLE_BYTES * 512) 
#define REC_BLOCK  (REC_BLOCK1 + REC_BLOCK2)
unsigned char recordbuffer1[REC_BLOCK1];
unsigned char recordbuffer2[REC_BLOCK2];
unsigned char recordbuffer[REC_BLOCK*512];

pthread_mutex_t rec_buffer_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_cond_t rec_buffer_cond   = PTHREAD_COND_INITIALIZER;

int record_head=0;
int record_count=0;
int record_tail=0;
int rec_active1=0;
int rec_active2=0;
int rec_exit=0;
int rec_flush_req=0;
int rec_flush_ok=0;

int rec_buffer_disk_min=100;
int rec_buffer_dma_min=100;

/* writes a wav header without the length set. */
void PutNumLE(long num,FILE *f,int bytes){
  int i=0;
  while(bytes--){
    fputc((num>>(i<<3))&0xff,f);
    i++;
  }
}
void WriteWav(FILE *f,long channels,long rate,long bits,long duration){
  fseek(f,0,SEEK_SET);
  fprintf(f,"RIFF");
  PutNumLE(duration+44-8,f,4);
  fprintf(f,"WAVEfmt ");
  PutNumLE(16,f,4);
  PutNumLE(1,f,2);
  PutNumLE(channels,f,2);
  PutNumLE(rate,f,4);
  PutNumLE(rate*channels*((bits-1)/8+1),f,4);
  PutNumLE(((bits-1)/8+1)*channels,f,2);
  PutNumLE(bits,f,2);
  fprintf(f,"data");
  PutNumLE(duration,f,4);
}

void *record_disk_thread(void *dummy){
  FILE *recdiskfd=NULL;
  long filesize=0;

  struct sched_param param;
  param.sched_priority=78;
  if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)){
    fprintf(stderr,"Could not set realtime priority for caching; am I suid root?\n");
    exit(1);
  }

  while(1){
    /* open a file; name the capture by time/date */
    struct stat buf;
    int ret;

    while(1){

      pthread_mutex_lock(&rec_mutex);
      if(!rec_flush_req && recdiskfd){
	WriteWav(recdiskfd,MAX_INPUT_CHANNELS,44100,INPUT_SAMPLE_BYTES*8,filesize);
	fclose(recdiskfd);
	recdiskfd=NULL;
	rec_flush_ok=0;
      }
      pthread_mutex_unlock(&rec_mutex);

      while(1){
	/* lock and check count; wait if none to flush */
	pthread_mutex_lock(&rec_buffer_mutex);
	if(record_count<=0){
	  /* wait for it */
	  pthread_cond_wait(&rec_buffer_cond,&rec_buffer_mutex);
	  pthread_mutex_unlock(&rec_buffer_mutex);
	  pthread_mutex_lock(&rec_mutex);
	}else{
	  pthread_mutex_unlock(&rec_buffer_mutex);
	  pthread_mutex_lock(&rec_mutex);
	  break;
	}

	if(rec_exit)break;
	pthread_mutex_unlock(&rec_mutex);

      }
      /* rec mutex lock fell through */
      {
	int percent=rint(100.-record_count*100./sizeof(recordbuffer));
	if(rec_buffer_disk_min>percent)rec_buffer_disk_min=percent;
      }
      pthread_mutex_unlock(&rec_mutex);

      /* flush to disk */

      if(recdiskfd)ret=fwrite(recordbuffer+record_tail,1,REC_BLOCK,recdiskfd);

      /* update counters, alert dma that we have space in ring buffer */
      pthread_mutex_lock(&rec_buffer_mutex);
      record_tail+=REC_BLOCK;
      if((unsigned)record_tail>=sizeof(recordbuffer))record_tail=0;
      record_count-=REC_BLOCK;
      pthread_cond_signal(&rec_buffer_cond);
      pthread_mutex_unlock(&rec_buffer_mutex);
      filesize+=REC_BLOCK;

      pthread_mutex_lock(&rec_mutex);
      if(filesize>10*60*44100*MAX_INPUT_CHANNELS*INPUT_SAMPLE_BYTES)break;
      if(rec_flush_req && !recdiskfd)break;
      if(rec_exit)break;
      pthread_mutex_unlock(&rec_mutex);

    }
    
    if(rec_exit)break;
    pthread_mutex_unlock(&rec_mutex);

    if(recdiskfd){
      WriteWav(recdiskfd,MAX_INPUT_CHANNELS,44100,INPUT_SAMPLE_BYTES*8,filesize);
      fclose(recdiskfd);
      filesize=0;
    }

    ret=stat("record",&buf);
    if(ret){
      mkdir("record",0700);
      ret=stat("record",&buf);
    }
    if(!ret && S_ISDIR(buf.st_mode)){
      /* construct a new filename */
      struct tm *now;
      char buf2[4096];
      char buf1[256];
      time_t nows;
      nows=time(NULL);
      now=localtime(&nows);
      strftime(buf1,256,"%Y%m%d_%H:%M:%S",now);
      sprintf(buf2,"record/%s.wav",buf1);
      recdiskfd=fopen(buf2,"wb");
      if(recdiskfd){
	WriteWav(recdiskfd,MAX_INPUT_CHANNELS,44100,INPUT_SAMPLE_BYTES*8,-1);
	pthread_mutex_lock(&rec_mutex);
	rec_flush_ok=1;
	filesize=0;
      }else{
	pthread_mutex_lock(&rec_mutex);
	rec_flush_ok=0;
      }
      pthread_mutex_unlock(&rec_mutex);
    }
  }

  if(recdiskfd){
    WriteWav(recdiskfd,MAX_INPUT_CHANNELS,44100,INPUT_SAMPLE_BYTES*8,filesize);
    fclose(recdiskfd);
  }

  rec_active2=0;
  fprintf(stderr,"Record flush thread exit...\n");
  pthread_mutex_unlock(&rec_mutex);
  return(NULL);
}

void *record_thread(void *dummy){
  /* sound device startup */
  int i,j;

#if (INPUT_CHANNELS1)
  int fd1=fileno(recfd1);
#endif
#if (INPUT_CHANNELS2)
  int fd2=fileno(recfd2);
#endif
  int format=INPUT_SAMPLE_FMT;
  int channels1=INPUT_CHANNELS1;
  int channels2=INPUT_CHANNELS2;
  int rate=44100;
  long totalsize[2];
  int ret;
  audio_buf_info info,info2;

  struct sched_param param;
  param.sched_priority=89;
  if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)){
    fprintf(stderr,"Could not set realtime priority for caching; am I suid root?\n");
    exit(1);
  }

#if (INPUT_CHANNELS1)
  ret=ioctl(fd1,SNDCTL_DSP_SETFMT,&format);
  if(ret || format!=INPUT_SAMPLE_FMT){
    fprintf(stderr,"Could not set recording format\n");
    exit(1);
  }
  ret=ioctl(fd1,SNDCTL_DSP_CHANNELS,&channels1);
  if(ret || channels1!=INPUT_CHANNELS1){
    fprintf(stderr,"Could not set %d channel recording\n",INPUT_CHANNELS1);
    exit(1);
  }
  ret=ioctl(fd1,SNDCTL_DSP_SPEED,&rate);
  if(ret || rate!=44100){
    fprintf(stderr,"Could not set %dHz recording\n",44100);
    exit(1);
  }
  ioctl(fd1,SNDCTL_DSP_GETISPACE,&info);
  totalsize[0]=info.fragstotal*info.fragsize;
#endif
#if (INPUT_CHANNELS2)
  ret=ioctl(fd2,SNDCTL_DSP_SETFMT,&format);
  if(ret || format!=INPUT_SAMPLE_FMT){
    fprintf(stderr,"Could not set recording format\n");
    exit(1);
  }
  ret=ioctl(fd2,SNDCTL_DSP_CHANNELS,&channels2);
  if(ret || channels2!=INPUT_CHANNELS2){
    fprintf(stderr,"Could not set %d channel recording\n",INPUT_CHANNELS2);
    exit(1);
  }
  ret=ioctl(fd2,SNDCTL_DSP_SPEED,&rate);
  if(ret || rate!=44100){
    fprintf(stderr,"Could not set %dHz recording\n",44100);
    exit(1);
  }
  ioctl(fd2,SNDCTL_DSP_GETISPACE,&info);
  totalsize[1]=info.fragstotal*info.fragsize;
#endif

  pthread_create(&record_disk_thread_id,NULL,record_disk_thread,NULL);

  while(1){
    int ret;

    /* lock the buffer and check tail; block on space to record */
    while(1){
      pthread_mutex_lock(&rec_buffer_mutex);
      if(sizeof(recordbuffer)-record_count<REC_BLOCK)
	/* wait for it */
	pthread_cond_wait(&rec_buffer_cond,&rec_buffer_mutex);
      else
	break;
      pthread_mutex_unlock(&rec_buffer_mutex);

      pthread_mutex_lock(&rec_mutex);
      if(rec_exit)break;
      pthread_mutex_unlock(&rec_mutex);

    }
    pthread_mutex_unlock(&rec_buffer_mutex);

    /* update ISPACE min */
#if (INPUT_CHANNELS1)
    ioctl(fd1,SNDCTL_DSP_GETISPACE,&info);
    {
      int percent1=rint((totalsize[0]-info.bytes)*100./totalsize[0]);
      if(rec_buffer_dma_min>percent1)rec_buffer_dma_min=percent1;
    }
#endif
#if (INPUT_CHANNELS2)
    ioctl(fd2,SNDCTL_DSP_GETISPACE,&info2);
    {
      int percent2=rint((totalsize[1]-info2.bytes)*100./totalsize[1]);
      if(rec_buffer_dma_min>percent2)rec_buffer_dma_min=percent2;
    }
#endif

#if (INPUT_CHANNELS1)
    fread(recordbuffer1,1,REC_BLOCK1,recfd1);
#endif
#if (INPUT_CHANNELS2)
    fread(recordbuffer2,1,REC_BLOCK2,recfd2);
#endif 

    pthread_mutex_lock(&rec_mutex);
    {
      unsigned char *ptr1=recordbuffer1;
      unsigned char *ptr2=recordbuffer2;
      for(i=record_head;i<record_head+REC_BLOCK;){
	for(j=0;j<INPUT_CHANNELS1;j++){
	  recordbuffer[i++]=*ptr1++;
	  recordbuffer[i++]=*ptr1++;
	}
	for(j=0;j<INPUT_CHANNELS2;j++){
	  recordbuffer[i++]=*ptr2++;
	  recordbuffer[i++]=*ptr2++;
	}
      }
    }

    for(i=record_head;i<record_head+REC_BLOCK;)
      for(j=0;j<MAX_INPUT_CHANNELS;j++){
	//int val=((recordbuffer[i]<<8)|(recordbuffer[i+1]<<16)|(recordbuffer[i+2]<<24))>>8;
	int val=((recordbuffer[i]<<16)|(recordbuffer[i+1]<<24))>>8;
	if(labs(val)>rchannel_list[j].peak)
	  rchannel_list[j].peak=labs(val);
	i+=INPUT_SAMPLE_BYTES;
      }
    if(rec_exit)break;

    if(rec_flush_req){
      pthread_mutex_unlock(&rec_mutex);

      pthread_mutex_lock(&rec_buffer_mutex);
      record_head+=REC_BLOCK;
      if((unsigned)record_head>=sizeof(recordbuffer))record_head=0;
      record_count+=REC_BLOCK;
      pthread_cond_signal(&rec_buffer_cond);
      pthread_mutex_unlock(&rec_buffer_mutex);
    }else{
      pthread_mutex_unlock(&rec_mutex);
    }
  }

  rec_active1=0;
  fprintf(stderr,"Record thread exit...\n");
  pthread_mutex_unlock(&rec_mutex);
  
  return(NULL);
}
#endif

/*************** threaded precache ****************************/

static void wake_cache(){
  /* signal the precache thread to wake up and work */
  pthread_mutex_lock(&cache_mutex);
  pthread_cond_signal(&cache_cond);
  pthread_mutex_unlock(&cache_mutex);
}

/* master is singly locked upon call */
static void _cache_tag_lockahead(int i){
  tag *t=cache_list[i];
  long fill=t->cache_fill;
  long new;
  long bytes=t->samplelength;
  
  pthread_mutex_lock(&mmap_mutex);
  pthread_mutex_unlock(&cache_mutex);

  fill*=2*t->channels;
  new=fill+65536;
  bytes*=2*t->channels;

  if(new>bytes)new=bytes;
  if(mlock(t->data,new)){
    fprintf(stderr,"mlock failed: %s\n",strerror(errno));
  }
  new/=2;
  new/=t->channels;

  pthread_mutex_unlock(&mmap_mutex);
  pthread_mutex_lock(&cache_mutex);
  t->cache_fill=new;
}

static void _cache_remove(int i){
  /* remove the tag from the precaching list, then free its cache;
     this is the only place a cache free can happen, so we only need
     lock/check against async read */

  tag *t=cache_list[i];
  cache_count--;
  if(i<cache_count)
    memmove(cache_list+i,
	    cache_list+i+1,
	    (cache_count-i)*sizeof(*cache_list));
  t->cachep=0;
  munlock(t->basemap,t->samplelength*sizeof(int16)*t->channels+sizeof(struct sample_header));
}

/* unlocks cache of all tags not currently active */
static void cache_cull(){
  int i;
  pthread_mutex_lock(&cache_mutex);
  for(i=cache_count-1;i>=0;i--)
    if(!cache_list[i]->activep)_cache_remove(i);
  pthread_mutex_unlock(&cache_mutex);
  refresh();
  write(ttypipe[1],"",1);
}

static void _cache_add(int tagnum){
  tag *t=tag_list+tagnum;

  if(!t->basemap)return;
  if(t->cachep)return;
  cache_list[cache_count]=t;
  cache_count++;
  t->cache_fill=0;
  t->sample_position=0;
  t->cachep=1;
  refresh();
  write(ttypipe[1],"",1);
}

static void *cache_thread(void *dummy){
  /* complete our self-setup; we need to be a realtime thread just
     behind playback and record */
  int i;

  struct sched_param param;
  param.sched_priority=80;
  if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)){
    fprintf(stderr,"Could not set realtime priority for caching; am I suid root?\n");
    exit(1);
  }

  pthread_mutex_lock(&cache_mutex);    
  while(1){

    /* scan tags; service lowest fill.  Active has priority over
       precache */
    {
      long minfill_a=0;
      long mintag_a=-1;
      long minfill_p=0;
      long mintag_p=-1;
      for(i=0;i<cache_count;i++){
	tag *t=cache_list[i];
	if(t->cache_fill<t->samplelength){
	  long minfill=t->cache_fill-t->sample_position;

	  if(minfill<CACHE_SAMPLES){
	    
	    if(t->activep){
	      if(mintag_a == -1 || t->cache_fill<minfill_a){
		minfill_a=minfill;
		mintag_a=i;
	      }
	    }else{
	      if(mintag_p == -1 || t->cache_fill<minfill_p){
		minfill_p=minfill;
	      mintag_p=i;
	      }
	    }
	  }
	}
	if(mintag_a>=0){
	  _cache_tag_lockahead(mintag_a);
	  continue;
	}
	if(mintag_p>=0){
	  _cache_tag_lockahead(mintag_p);
	  continue;
	}
      }
    }
    
    /* if there was no work to do, we sleep and wait for a signal */
    pthread_cond_wait(&cache_cond, &cache_mutex);
  }
}

/*************** threaded playback ****************************/

static inline double _slew_ms(long ms,double now,double target){

  return((target-now)/(ms*44.1));
  
}

/* position in active list */
static inline void _playback_remove(int i){
  tag *t=active_list[i];
  t->activep=0;
  refresh();
  active_count--;
  if(i<active_count)
    memmove(active_list+i,
	    active_list+i+1,
	    (active_count-i)*sizeof(*active_list));
  for(i=cache_count-1;i>=0;i--)
    if(cache_list[i]==t)
      _cache_remove(i);
  write(ttypipe[1],"",1);
}

/* position in tag list */
static inline void _playback_add(int tagnum,int cuenum){
  tag *t=tag_list+tagnum;
  cue *c=cue_list+cuenum;
  int j,k;

  if(t->activep)return;
  if(!t->basemap)return;
  t->activep=1;
  refresh();

  active_list[active_count]=t;
  active_count++;
  t->sample_position=0;
  t->sample_lapping=t->loop_lapping*44.1;
  if(t->sample_lapping>(t->samplelength-t->sample_loop_start)/2)
    t->sample_lapping=(t->samplelength-t->sample_loop_start)/2;

  t->sample_loop_start=t->loop_start*44.1;
  if(t->sample_loop_start>t->samplelength)
    t->sample_loop_start=t->samplelength;

  t->sample_fade_start=t->samplelength-t->fade_out*44.1;
  if(t->sample_fade_start<0)t->sample_fade_start=0;

  if(c->mix.vol_ms==0)
    t->master_vol_current=c->mix.vol_master;
  else
    t->master_vol_current=0;

  t->master_vol_target=c->mix.vol_master;
  t->master_vol_slew=_slew_ms(c->mix.vol_ms,0,c->mix.vol_master);
  			   
  if(c->mix.vol_ms==0)
    for(j=0;j<MAX_OUTPUT_CHANNELS;j++)
      for(k=0;k<t->channels;k++){
	t->outvol_current[k][j]=c->mix.outvol[k][j];
	t->outvol_target[k][j]=c->mix.outvol[k][j];
	t->outvol_slew[k][j]=_slew_ms(c->mix.vol_ms,0,c->mix.outvol[k][j]);
      }
  else
    for(j=0;j<MAX_OUTPUT_CHANNELS;j++)
      for(k=0;k<t->channels;k++){
	t->outvol_current[k][j]=0;
	t->outvol_target[k][j]=c->mix.outvol[k][j];
	t->outvol_slew[k][j]=_slew_ms(c->mix.vol_ms,0,c->mix.outvol[k][j]);
      }

  _cache_add(tagnum);
  write(ttypipe[1],"",1);
}

/* position in tag list */
static inline void _playback_mix(int i,int cuenum){
  tag *t=tag_list+i;
  cue *c=cue_list+cuenum;
  int j,k;

  if(!t->activep)return;

  t->master_vol_target=c->mix.vol_master;
  t->master_vol_slew=_slew_ms(c->mix.vol_ms,t->master_vol_current,
			   c->mix.vol_master);
  			   
  for(j=0;j<MAX_OUTPUT_CHANNELS;j++)
    for(k=0;k<t->channels;k++){
      t->outvol_target[k][j]=c->mix.outvol[k][j];
      t->outvol_slew[k][j]=_slew_ms(c->mix.vol_ms,t->outvol_current[k][j],
				 c->mix.outvol[k][j]);
    }
}

static inline void _next_sample(int16 *out1,int ch1,int16 *out2,int ch2){
  int i,j,k;
  double staging[MAX_OUTPUT_CHANNELS];
  double mmv=main_master_volume*.0001;

  memset(staging,0,sizeof(staging));

  /* iterate through the active sample list */
  for(i=active_count-1;i>=0;i--){
    tag *t=active_list[i];
    for(j=0;j<t->channels;j++){
      double value;
      int lappoint=t->samplelength-t->sample_lapping;

      double *ov_slew=t->outvol_slew[j];
      double *ov_target=t->outvol_target[j];
      double *ov_current=t->outvol_current[j];

      /* get the base value, depending on loop or no */
      if(t->loop_p && t->sample_position>=lappoint){
	long looppos=t->sample_position-lappoint+t->sample_loop_start;
	double value2=t->data[looppos*t->channels+j];
	value=t->data[t->sample_position*t->channels+j];
	
	value=(value2*looppos/t->sample_lapping+
	  value-value*looppos/t->sample_lapping)*t->master_vol_current*.01;

      }else{
	value=t->data[t->sample_position*t->channels+j]*
	  t->master_vol_current*.01;
      }

      /* output split and mix */
      value*=mmv;
      for(k=0;k<MAX_OUTPUT_CHANNELS;k++){
	staging[k]+=value*ov_current[k];
	
	ov_current[k]+=ov_slew[k];
	if(ov_slew[k]>0. && 
	   ov_current[k]>ov_target[k]){
	  ov_slew[k]=0.;
	  ov_current[k]=ov_target[k];
	}
	if(ov_slew[k]<0. && 
	   ov_current[k]<ov_target[k]){
	  ov_slew[k]=0.;
	  ov_current[k]=ov_target[k];
	}
      }
    }

    /* update master volume */
    if(t->master_vol_slew){
      t->master_vol_current+=t->master_vol_slew;
      if(t->master_vol_slew>0 && t->master_vol_current>t->master_vol_target){
	t->master_vol_slew=0;
	t->master_vol_current=t->master_vol_target;
      }
      if(t->master_vol_slew<0 && t->master_vol_current<t->master_vol_target){
	t->master_vol_slew=0;
	t->master_vol_current=t->master_vol_target;
      }
      if(t->master_vol_current<0)
	_playback_remove(i);
    }

    /* determine if fade out has begun */
    if(t->sample_position==t->sample_fade_start && !t->loop_p){
      /* effect a master volume slew *now* */
      t->master_vol_slew=_slew_ms(t->fade_out,
				  t->master_vol_current,0);
      t->master_vol_target=0;
    }

    /* update playback position */
    t->sample_position++;
    if(t->sample_position>=t->samplelength){
      if(t->loop_p){
	t->sample_position=t->sample_loop_start+t->sample_lapping;
      }else{
	_playback_remove(i);
      }
    }

  }

  /* declipping, conversion */
  for(i=0;i<ch1;i++){
    if(channel_list[i].peak<fabs(staging[i]))
      channel_list[i].peak=fabs(staging[i]);
    if(staging[i]>32767.){
      out1[i]=32767;
    }else if(staging[i]<-32768.){
      out1[i]=-32768;
    }else
      out1[i]=(int)(rint(staging[i]));
  }

  for(i=0;i<ch2;i++){
    if(channel_list[i+ch1].peak<fabs(staging[i+ch1]))
      channel_list[i+ch1].peak=fabs(staging[i+ch1]);
    if(staging[i+ch1]>32767.){
      out2[i]=32767;
    }else if(staging[i+ch1]<-32768.){
      out2[i]=-32768;
    }else
      out2[i]=(int)(rint(staging[i+ch1]));
  }
}

static int playback_active=0;
static int playback_exit=0;

/* NO LOCKING in the playback thread.  We're the highest priority
   thread and will not be preempted.  The data structures we depend on
   are 'locked' at lower levels by atomic assignment blocks. */

void *playback_thread(void *dummy){
  /* sound device startup */
  audio_buf_info info;
#if (OUTPUT_CHANNELS1)
  int fd1=fileno(playfd1),i;
#endif
#if (OUTPUT_CHANNELS2)
  int fd2=fileno(playfd2);
#endif
  int format=AFMT_S16_NE;
  int channels1=OUTPUT_CHANNELS1;
  int channels2=OUTPUT_CHANNELS2;
  int rate=44100;
  long last=0;
  long delay=10;
  long totalsize;
  int fragment=0x7fff000d;
  int16 audiobuf1[256*OUTPUT_CHANNELS1];
  int16 audiobuf2[256*OUTPUT_CHANNELS2];
  int ret;

  /* realtime schedule setup */
  {
    struct sched_param param;
    param.sched_priority=89;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)){
      fprintf(stderr,"Could not set realtime priority for playback; am I suid root?\n");
      exit(1);
    }
  }

#if (OUTPUT_CHANNELS1)
  ioctl(fd1,SNDCTL_DSP_SETFRAGMENT,&fragment);
  ret=ioctl(fd1,SNDCTL_DSP_SETFMT,&format);
  if(ret || format!=AFMT_S16_NE){
    fprintf(stderr,"Could not set AFMT_S16_NE playback\n");
    exit(1);
  }
  ret=ioctl(fd1,SNDCTL_DSP_CHANNELS,&channels1);
  if(ret || channels1!=OUTPUT_CHANNELS1){
    fprintf(stderr,"Could not set %d channel playback\n",OUTPUT_CHANNELS1);
    exit(1);
  }
  ret=ioctl(fd1,SNDCTL_DSP_SPEED,&rate);
  if(ret || rate!=44100){
    fprintf(stderr,"Could not set %dHz playback\n",44100);
    exit(1);
  }
  ioctl(fd1,SNDCTL_DSP_GETOSPACE,&info);
  playback_buffer_minfill=totalsize=info.fragstotal*info.fragsize;
#else
  totalsize=0;
#endif
#if (OUTPUT_CHANNELS2)  
  ioctl(fd2,SNDCTL_DSP_SETFRAGMENT,&fragment);
  ret=ioctl(fd2,SNDCTL_DSP_SETFMT,&format);
  if(ret || format!=AFMT_S16_NE){
    fprintf(stderr,"Could not set AFMT_S16_NE playback\n");
    exit(1);
  }
  ret=ioctl(fd2,SNDCTL_DSP_CHANNELS,&channels2);
  if(ret || channels2!=OUTPUT_CHANNELS2){
    fprintf(stderr,"Could not set %d channel playback\n",OUTPUT_CHANNELS2);
    exit(1);
  }
  ret=ioctl(fd2,SNDCTL_DSP_SPEED,&rate);
  if(ret || rate!=44100){
    fprintf(stderr,"Could not set %dHz playback\n",44100);
    exit(1);
  }
  ioctl(fd2,SNDCTL_DSP_GETOSPACE,&info);
  playback_buffer_minfill=totalsize+=info.fragstotal*info.fragsize;
#endif

  while(!playback_exit){
    int samples;

    delay--;
    if(delay<0){
      delay=0;
      samples=0;
#if (OUTPUT_CHANNELS1)
      ioctl(fd1,SNDCTL_DSP_GETOSPACE,&info);
      playback_bufsize=totalsize;      
      samples=info.bytes;
#else
      playback_bufsize=0;      
      samples=0;
#endif
#if (OUTPUT_CHANNELS2)
      ioctl(fd2,SNDCTL_DSP_GETOSPACE,&info);
      playback_bufsize+=totalsize;      
      samples+=info.bytes;
#endif
      
      samples+=playback_bufsize-samples;
      
      if(playback_buffer_minfill>samples)
	playback_buffer_minfill=samples-64; // sample fragment
      
    }

    for(i=0;i<256;i++)
      _next_sample(audiobuf1+i*OUTPUT_CHANNELS1,
		   OUTPUT_CHANNELS1,
		   audiobuf2+i*OUTPUT_CHANNELS2,
		   OUTPUT_CHANNELS2);

    /* this is a calculated race; the race would not trip except in
       situations where our locking latency would also cause the
       realtime house of cards to come crashing down anyway */
    pthread_cond_signal(&cache_cond);

#if (OUTPUT_CHANNELS1)
    fwrite(audiobuf1,2*OUTPUT_CHANNELS1,256,playfd1);
#endif
#if (OUTPUT_CHANNELS2)
    fwrite(audiobuf2,2*OUTPUT_CHANNELS2,256,playfd2);
#endif
    
    {
      struct timeval tv;
      long foo;
      gettimeofday(&tv,NULL);
      foo=tv.tv_sec*10+tv.tv_usec/100000;
      if(last!=foo)
	write(ttypipe[1],"",1);
      last=foo;
    }

  }
  
  playback_active=0;
  
  /* sound device shutdown */
#if (OUTPUT_CHANNELS1)
  ioctl(fd1,SNDCTL_DSP_RESET);
#endif
#if (OUTPUT_CHANNELS2)
  ioctl(fd2,SNDCTL_DSP_RESET);
#endif
  fprintf(stderr,"Playback thread exit...\n");
  return(NULL);
}

/* cuenum is the base number */ 
/* add new tags before new mixes */
int play_cue(int cuenum){
  int x=cuenum;
  BEGIN_ATOMIC
  while(1){
    cue *c=cue_list+x;
    if(c->tag>=0){
      if(c->tag_create_p)
	_playback_add(c->tag,x);
    }
    x++;
    if(x>=cue_count || cue_list[x].tag==-1)break;
  }
  while(1){
    cue *c=cue_list+cuenum;
    if(c->tag>=0){
      if(!c->tag_create_p)
	_playback_mix(c->tag,cuenum);
    }
    cuenum++;
    if(cuenum>=cue_count || cue_list[cuenum].tag==-1)break;
  }
  END_ATOMIC
  return(0);
}

/* caches new queue.  cache cleanup is handled in caching thread */
int cache_cue(int cuenum){
  pthread_mutex_lock(&cache_mutex);

  while(cuenum<cue_count){
    cue *c=cue_list+cuenum;
    if(c->tag>=0){
      if(c->tag_create_p)
	_cache_add(c->tag);
    }
    cuenum++;
    if(cuenum>=cue_count || cue_list[cuenum].tag==-1)break;
  }

  pthread_mutex_unlock(&cache_mutex);

  wake_cache();
  return(0);
}

int play_sample(int cuenum){
  cue *c=cue_list+cuenum;
  BEGIN_ATOMIC
  if(c->tag>=0){
    if(c->tag_create_p)
      _playback_add(c->tag,cuenum);
    else
      _playback_mix(c->tag,cuenum);
  }

  END_ATOMIC
  return(0);
}

int halt_playback(){
  int i;
  BEGIN_ATOMIC
  for(i=0;i<tag_count;i++){
    tag *t=tag_list+i;

    if(t->activep){

      t->master_vol_target=-1;
      t->master_vol_slew=_slew_ms(100,t->master_vol_current,0);
      
    }
  }
  END_ATOMIC
  return(0);
}


/***************** simple form entry fields *******************/

enum field_type { FORM_YESNO, FORM_PERCENTAGE, FORM_PERCENTAGE_R, 
		  FORM_NUMBER, FORM_GENERIC,
                  FORM_BUTTON } ;
typedef struct {
  enum field_type type;
  int x;
  int y;
  int width;
  void *var;

  int active;
  int cursor;
} formfield;

typedef struct {
  formfield *fields;
  int count;
  int storage;
  int edit;

  int cursor;
} form;

void form_init(form *f,int maxstorage){
  memset(f,0,sizeof(*f));
  f->fields=calloc(maxstorage,sizeof(formfield));
  f->storage=maxstorage;
}

void form_clear(form *f){
  if(f->fields)free(f->fields);
  memset(f,0,sizeof(*f));
}

void draw_field(formfield *f,int edit,int focus){
  int y,x;
  int i;
  getyx(stdscr,y,x);
  move(f->y,f->x);

  if(f->type==FORM_BUTTON)edit=0;

  if(edit && f->active)
    attron(A_REVERSE);
  
  addch('[');

  if(f->active){

    if(edit){
      attrset(0);
    }else{
      if(focus){
	attron(A_REVERSE);
      }else{
	attron(A_BOLD);
      }
    }
    
    switch(f->type){
    case FORM_YESNO:
      {
	int *var=(int *)(f->var);
	char *s="No ";
	if(*var)
	  s="Yes";
	for(i=0;i<f->width-5;i++)
	  addch(' ');
	addstr(s);
      }
      break;
    case FORM_PERCENTAGE:
      {
	int var=*(int *)(f->var);
	char buf[80];
	if(var<0)var=0;
	if(var>100)var=100;
	snprintf(buf,80,"%*d",f->width-2,var);
	addstr(buf);
      }
      break;
    case FORM_PERCENTAGE_R:
      {
	int var=*(int *)(f->var);
	char buf[80];
	if(var<-1)var=-1;
	if(var>100)var=100;
	if(var==-1)
	  snprintf(buf,80,"%*s",f->width-2,"DEL");
	else
	  snprintf(buf,80,"%*d",f->width-2,var);
	addstr(buf);
      }
      break;
    case FORM_NUMBER:
      {
	long var=*(long *)(f->var);
	char buf[80];
	snprintf(buf,80,"%*ld",f->width-2,var);
	addstr(buf);
      }
      break;
    case FORM_GENERIC:case FORM_BUTTON:
      {
	char *var=(char *)(f->var);
	addnlstr(var,f->width-2,' ');
      }
      break;
    }
    
    attrset(0);
  }else{
    attrset(0);
    addnlstr("",f->width-2,'-');
  }

  if(edit &&
     f->active)
    attron(A_REVERSE);
  
  addch(']');

  attrset(0);

  /* cursor? */
  move(y,x);
  if(focus && edit && f->type==FORM_GENERIC){
    curs_set(1);
    move(f->y,f->x+1+f->cursor);
  }else{
    curs_set(0);
  } 
}

void form_redraw(form *f){
  int i;
  for(i=0;i<f->count;i++)
    draw_field(f->fields+i,f->edit,i==f->cursor);
}

int field_add(form *f,enum field_type type,int x,int y,int width,void *var){
  int n=f->count;
  if(f->storage==n)return(-1);
  /* add the struct, then draw contents */
  f->fields[n].type=type;
  f->fields[n].x=x;
  f->fields[n].y=y;
  f->fields[n].width=width;
  f->fields[n].var=var;
  f->fields[n].active=1;
  f->count++;

  draw_field(f->fields+n,f->edit,n==f->cursor);
  return(n);
}

void field_state(form *f,int n,int activep){
  if(n<f->count){
    f->fields[n].active=activep;
    draw_field(f->fields+n,f->edit,n==f->cursor);
  }
}    

void form_next_field(form *f){
  formfield *ff=f->fields+f->cursor;
  draw_field(f->fields+f->cursor,0,0);
  
  while(1){
    f->cursor++;
    if(f->cursor>=f->count)f->cursor=0;
    ff=f->fields+f->cursor;
    if(ff->active)break;
  }

  draw_field(f->fields+f->cursor,f->edit,1);
}

void form_prev_field(form *f){
  formfield *ff=f->fields+f->cursor;
  draw_field(f->fields+f->cursor,0,0);
  
  while(1){
    f->cursor--;
    if(f->cursor<0)f->cursor=f->count-1;
    ff=f->fields+f->cursor;
    if(ff->active)break;
  }
  
  draw_field(f->fields+f->cursor,f->edit,1);
}

/* returns >=0 if it does not handle the character */
int form_handle_char(form *f,int c){
  formfield *ff=f->fields+f->cursor;
  int ret=-1;

  switch(c){
  case KEY_ENTER:
  case '\n':
  case '\r':
    if(ff->type==FORM_BUTTON){
      f->edit=0;
      ret=KEY_ENTER;
    }else{
      if(f->edit){
	f->edit=0;
	//draw_field(f->fields+f->cursor,f->edit,1);    
	//form_next_field(f);
      }else{
	f->edit=1;
      }
    }
    break;
  case KEY_UP:
    form_prev_field(f);
    break;
  case KEY_DOWN:case '\t':
    form_next_field(f);
    break;
  default:
    if(f->edit){
      BEGIN_ATOMIC
      switch(ff->type){
      case FORM_YESNO:
	{
	  int *val=(int *)ff->var;
	  switch(c){
	  case 'y':case 'Y':
	    *val=1;
	    break;
	  case 'n':case 'N':
	    *val=0;
	    break;
	  case ' ':
	    if(*val)
	    *val=0;
	  else
	    *val=1;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
      case FORM_PERCENTAGE:
	{
	  int *val=(int *)ff->var;
	  switch(c){
	  case '=':case KEY_RIGHT:
	    (*val)++;
	    if(*val>100)*val=100;
	    break;
	  case '+':
	    (*val)+=10;
	    if(*val>100)*val=100;
	    break;
	  case '-':case KEY_LEFT:
	    (*val)--;
	    if(*val<0)*val=0;
	    break;
	  case '_':
	    (*val)-=10;
	    if(*val<0)*val=0;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
      case FORM_PERCENTAGE_R:
	{
	  int *val=(int *)ff->var;
	  switch(c){
	  case '=':case KEY_RIGHT:
	    (*val)++;
	    if(*val>100)*val=100;
	    break;
	  case '+':
	    (*val)+=10;
	    if(*val>100)*val=100;
	    break;
	  case '-':case KEY_LEFT:
	    (*val)--;
	    if(*val<-1)*val=-1;
	    break;
	  case '_':
	    (*val)-=10;
	    if(*val<-1)*val=-1;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
      case FORM_NUMBER:
	{
	  long *val=(long *)ff->var;
	  switch(c){
	  case '0':case '1':case '2':case '3':case '4':
	  case '5':case '6':case '7':case '8':case '9':
	    if(*val<(int)rint(pow(10,ff->width-3))){
	      (*val)*=10;
	      (*val)+=c-48;
	    }
	    break;
	  case KEY_BACKSPACE:case '\b':
	    (*val)/=10;
	    break;
	  case KEY_RIGHT:case '+':case '=':
	    if(*val<(int)rint(pow(10,ff->width-2)-1))
	      (*val)++;
	    break;
	  case KEY_LEFT:case '-':case '_':
	    if(*val>0)
	      (*val)--;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
	
	/* we assume the string for the GENERIC case is alloced to width */
      case FORM_GENERIC:
	{
	  char *val=(char *)ff->var;
	  const char *ctrl=unctrl(c);
	  switch(c){
	  case KEY_LEFT:
	    ff->cursor--;
	    if(ff->cursor<0)ff->cursor=0;
	    break;
	  case KEY_RIGHT:
	    ff->cursor++;
	    if(ff->cursor>(int)strlen(val))ff->cursor=strlen(val);
	    if(ff->cursor>ff->width-3)ff->cursor=ff->width-3;
	    break;
	  case KEY_BACKSPACE:case '\b':
	    if(ff->cursor>0){
	      memmove(val+ff->cursor-1,val+ff->cursor,strlen(val)-ff->cursor+1);
	      ff->cursor--;
	    }
	    break;
	  default:
	    if(isprint(c)){
	      if((int)strlen(val)<ff->width-3){
		memmove(val+ff->cursor+1,val+ff->cursor,strlen(val)-ff->cursor+1);
		val[ff->cursor]=c;
		ff->cursor++;
	      }
	    }else{
	      if(ctrl[0]=='^'){
		switch(ctrl[1]){
		case 'A':case 'a':
		  ff->cursor=0;
		  break;
		case 'E':case 'e':
		  ff->cursor=strlen(val);
		  break;
		case 'K':case 'k':
		  val[ff->cursor]='\0';
		  break;
		default:
		  ret=c;
		  break;
		}
	      }else{
		ret=c;
	      }
	    }
	    break;
	  }
	}
	break;
      default:
	ret=c;
	break;
      }
      END_ATOMIC

    }else{
      ret=c;
    }
  }
  draw_field(f->fields+f->cursor,f->edit,1);    
  return(ret);
}

/********************** main run screen ***********************/
void main_update_master(int n,int y){
  if(menu==MENU_MAIN){
    char buf[5];
    if(n>300)n=300;
    if(n<0)n=0;

    main_master_volume=n;
    
    move(y,8);
    addstr("master: ");
    sprintf(buf,"%3ld%%",main_master_volume);
    addstr(buf);
  }
}

void main_update_playbuffer(int y){
  if(menu==MENU_MAIN){
    char buf[20];
    int  n,nr;
    static int starve=0;
    static int starver1=0;
    static int starver2=0;

    n=playback_buffer_minfill;
    playback_buffer_minfill=playback_bufsize;
    
    if(n==0){
      starve=15;
    }
    starve--;
    if(starve<0){
      starve=0; /* reset */
    }
    
    if(playback_bufsize)
      n=rint(100.*n/playback_bufsize);
    else{
      n=0;
      starve=0;
    }

    move(y,4);
    addstr("playbuffer: ");
    sprintf(buf,"%3d%% %s",n,starve?"***STARVE***":"            ");
    addstr(buf);


    move(y+1,63);
#if (MAX_INPUT_CHANNELS)
    {
      int state=0;
      pthread_mutex_lock(&rec_mutex);
      if(rec_flush_req){
	if(rec_flush_ok)
	  state=2;
	else
	  state=1;
      }
      pthread_mutex_unlock(&rec_mutex);
    
      switch(state){
      case 0:
	addstr(" OFF         ");
	break;
      case 1:
	addstr(" STARTING    ");
	break;
      case 2:
	addstr(" RECORDING   ");
	break;
      }
    }

    nr=rec_buffer_dma_min;
    rec_buffer_dma_min=100;
    pthread_mutex_unlock(&rec_mutex);
    
    if(nr==0){
      starver1=15;
    }
    starver1--;
    if(starver1<0){
      starver1=0; /* reset */
    }
    
    move(y,42);
    addstr("recbuffer (DMA):");
    sprintf(buf," %3d%% %s",nr,starver1?"***OVERRUN***":"             ");
    addstr(buf);

    pthread_mutex_lock(&rec_mutex);
    nr=rec_buffer_disk_min;
    rec_buffer_disk_min=100;
    pthread_mutex_unlock(&rec_mutex);
    
    if(nr==0){
      starver2=15;
    }
    starver2--;
    if(starver2<0){
      starver2=0; /* reset */
    }
    
    move(y+1,42);
    addstr("recbuffer(disk):");
    sprintf(buf," %3d%% %s",nr,starver2?"***OVERRUN***":"");
    addstr(buf);
#endif
  }
}

#define todB_nn(x)   ((x)==0.f?-400.f:log((x))*8.6858896f)
static int dBmap[100]={0,0,1,1,1,1,1,1,1,1,
		    1,1,1,1,1,1,1,1,1,1,
		    1,1,1,1,1,1,1,1,1,1,
		    1,1,1,1,1,2,2,2,2,2,
		    2,2,2,2,2,2,2,2,2,2,
		    3,3,3,3,3,3,3,3,3,3,
		    3,3,4,4,4,4,4,4,4,4,
		    4,4,5,5,5,5,5,5,5,5,
		    6,6,6,6,6,6,7,7,7,7,
		    7,8,8,8,8,9,9,9,10,10};

static int clip[MAX_OUTPUT_CHANNELS];
void main_update_outchannel_levels(int y){
  int i,j;
  if(menu==MENU_MAIN){
    for(i=0;i<MAX_OUTPUT_CHANNELS;i++){
      int val;
      char buf[11];
      BEGIN_ATOMIC
      val=channel_list[i].peak;
      channel_list[i].peak=0;
      END_ATOMIC
      
      move(y+i+1,17);
      if(val>=32767){
	clip[i]=15;
	val=32767;
      }
      clip[i]--;
      if(clip[i]<0)clip[i]=0;
      if(clip[i]){
	attron(A_BOLD);
	addstr("CLIP");
      }else{
	addstr("+0dB");
      }
      attron(A_BOLD);

      move(y+i+1,6);
      val=rint(todB_nn(val/32768.)+100.);
      if(val<0)val=0;
      if(val>99)val=99;
      val=dBmap[val];
      for(j=0;j<val;j++)buf[j]='=';
      for(;j<10;j++)buf[j]=' ';
      buf[j]='\0';
      addstr(buf);
      attroff(A_BOLD);
    }
  }
}

static int rclip[MAX_INPUT_CHANNELS];
void main_update_inchannel_levels(int y){
  int i,j;
  if(menu==MENU_MAIN){
    for(i=0;i<MAX_INPUT_CHANNELS;i++){
      int val;
      char buf[11];
      val=rchannel_list[i].peak;
      rchannel_list[i].peak=0;
      
      move(y+i+1,55);
      if(val>=0x7fffff){
	rclip[i]=15;
	val=0x7fffff;
      }
      rclip[i]--;
      if(rclip[i]<0)rclip[i]=0;
      if(rclip[i]){
	attron(A_BOLD);
	addstr("CLIP");
      }else{
	addstr("+0dB");
      }
      attron(A_BOLD);

      move(y+i+1,44);
      val=rint(todB_nn(val/8388607.)+100.);
      if(val<0)val=0;
      if(val>99)val=99;
      val=dBmap[val];
      for(j=0;j<val;j++)buf[j]='=';
      for(;j<10;j++)buf[j]=' ';
      buf[j]='\0';
      addstr(buf);
      attroff(A_BOLD);
    }
  }
}

void main_update_channel_labels(int y){
  int i;
  char buf[80];
  if(menu==MENU_MAIN){
    for(i=0;i<MAX_OUTPUT_CHANNELS;i++){
      move(y+i+1,4);
      sprintf(buf,"-[          ]+0dB ");
      addstr(buf);
      addstr(channel_list[i].label);
    }
    for(i=0;i<MAX_INPUT_CHANNELS;i++){
      move(y+i+1,42);
      sprintf(buf,"-[          ]+0dB ");
      addstr(buf);
      addstr(rchannel_list[i].label);
    }
  }

  main_update_outchannel_levels(y);
  main_update_inchannel_levels(y);
}

void main_update_cues(int y){
  if(menu==MENU_MAIN){
    int cn=cue_list_number-1,i;

    for(i=-1;i<2;i++){
      move(y+i*3+3,0);
      if(i==0){
	addstr("NEXT => ");
	attron(A_REVERSE);
      }
      if(cn<0){
	move(y+i*3+3,8);
	addnlstr("",71,' ');
	move(y+i*3+4,8);
	addnlstr("**** BEGIN",71,' ');
      }else if(cn>=cue_count){
	move(y+i*3+3,8);
	addnlstr("****** END",71,' ');
	attroff(A_REVERSE);
	move(y+i*3+4,8);
	addnlstr("",71,' ');
	if(i==0){
	  move(y+i*3+6,8);
	  addnlstr("",71,' ');
	  move(y+i*3+7,8);
	  addnlstr("",71,' ');
	}
	break;
      }else{
	cue *c;
	c=cue_list+cn;

	move(y+i*3+3,8);
	addnlstr(label_text(c->label),12,' ');
	addnlstr(label_text(c->cue_text),59,' ');

	mvaddstr(y+i*3+4,8,"            ");
	addnlstr(label_text(c->cue_desc),59,' ');
      }
      attroff(A_BOLD);
      while(++cn<cue_count)
	if(cue_list[cn].tag==-1)break;

      attroff(A_REVERSE);
    }
  }
}

/* assumes the existing tags/labels are not changing out from
   underneath playback.  editing a tag *must* kill playback for
   stability */
void main_update_tags(int y){
  if(menu==MENU_MAIN){
    int i;
    static int last_tags=0;

    move(y,0);
    
    BEGIN_ATOMIC

    if(active_count){
      addstr("playing tags:");

      for(i=0;i<active_count;i++){
	int loop;
	int ms;
	char buf[20];
	int vol=active_list[i]->master_vol_current;
	label_number path;
	
	move(y+i,14);
	loop=active_list[i]->loop_p;
	ms=(active_list[i]->samplelength-active_list[i]->sample_position)/44.1;
	path=active_list[i]->sample_desc;
	
	if(loop)
	  snprintf(buf,20,"[loop %3d%%] ",vol);
	else
	  snprintf(buf,20,"[%3ds %3d%%] ",(ms+500)/1000,vol);
	addstr(buf);
	addnlstr(label_text(path),60,' ');
      }

    }else{
      addstr("             ");
    }

    for(i=active_count;i<last_tags;i++){
      move(y+i,14);
      addnlstr("",60,' ');
    }

    last_tags=active_count;
    END_ATOMIC
  }
}

static int editable=1;
void update_editable(){
  if(menu==MENU_MAIN){
    move(0,67);
    attron(A_BOLD);
    if(!editable)
      addstr(" EDIT LOCKED ");
    else
      addstr("             ");
    attroff(A_BOLD);
  }
}

void move_next_cue(){
  if(cue_list_number<cue_count){
    while(++cue_list_number<cue_count)
      if(cue_list[cue_list_number].tag==-1)break;
    cue_list_position++;
  }
  cache_cull();
  cache_cue(cue_list_number);
  wake_cache();
  main_update_cues(10+MAX_CHANNELS);
}

void move_prev_cue(){
  if(cue_list_number>0){
    while(--cue_list_number>0)
      if(cue_list[cue_list_number].tag==-1)break;
    cue_list_position--;
  }
  cache_cull();
  cache_cue(cue_list_number);
  wake_cache();
  main_update_cues(10+MAX_CHANNELS);
}

int save_top_level(char *fn){
  FILE *f;
  if(!firstsave){
    char *buf=alloca(strlen(fn)*2+20);
    sprintf(buf,"cp %s %s~ 2>/dev/null",fn,fn);
    /* create backup file */
    system(buf);
    firstsave=1;
  }
  move(0,0);
  attron(A_BOLD);
  
  f=fopen(fn,"w");
  if(f==NULL){
    char buf[80];
    sprintf(buf,"SAVE FAILED: %s",strerror(errno));
    addnlstr(buf,80,' ');
    attroff(A_BOLD);
    return(1);
  }

  if(save_program(f)){
    char buf[80];
    sprintf(buf,"SAVE FAILED: %s",strerror(ferror(f)));
    addnlstr(buf,80,' ');
    attroff(A_BOLD);    
    fclose(f);
    return(1);
  }
  fclose(f);
  addnlstr("PROGRAM SAVED (any key to continue)",80,' ');
  attroff(A_BOLD);    
  unsaved=0;
  return(0);
}

int main_menu(){
  clear();
  move(0,0);
  addnstr("MTG Beaverphonic build "VERSION": ",67);
  attron(A_BOLD);
  mvaddnstr(1,0,program, -1);
  attroff(A_BOLD);
  update_editable();

  mvvline(3,2,0,MAX_CHANNELS+5);
  mvvline(3,77,0,MAX_CHANNELS+5);
  mvvline(3,40,0,MAX_CHANNELS+5);
  mvhline(2,2,0,76);
  mvhline(7,2,0,76);
  mvhline(8+MAX_CHANNELS,2,0,76);
  mvaddch(2,2,ACS_ULCORNER);
  mvaddch(2,77,ACS_URCORNER);
  mvaddch(2,40,ACS_TTEE);
  mvaddch(8+MAX_CHANNELS,2,ACS_LLCORNER);
  mvaddch(8+MAX_CHANNELS,40,ACS_BTEE);
  mvaddch(8+MAX_CHANNELS,77,ACS_LRCORNER);

  mvaddch(7,2,ACS_LTEE);
  mvaddch(7,40,ACS_PLUS);
  mvaddch(7,77,ACS_RTEE);

  move(2,7);
  addstr(" output ");

  move(2,45);
  addstr(" input ");

  mvhline(9+MAX_CHANNELS,0,0,80);
  mvhline(18+MAX_CHANNELS,0,0,80);

  main_update_master(main_master_volume,5);
  main_update_playbuffer(4);
  main_update_channel_labels(7);

  main_update_cues(10+MAX_CHANNELS);
  main_update_tags(19+MAX_CHANNELS);
  curs_set(0);

  refresh();

  while(1){
    int ch=getch();
    switch(ch){
    case '?':
      return(MENU_KEYPRESS);
    case 'q':
      move(0,0);
      attron(A_BOLD);
      addnlstr("Really quit? [y/N] ",80,' ');
      attroff(A_BOLD);
      refresh();
      ch=mgetch();
      if(ch=='y'){
	if(unsaved){
	  move(0,0);
	  attron(A_BOLD);
	  addnlstr("Save changes first? [Y/n] ",80,' ');
	  attroff(A_BOLD);
	  ch=mgetch();
	  if(ch!='n' && ch!='N')save_top_level(program);
	}
	return(MENU_QUIT);
      }
      move(0,0);
      addnstr("MTG Beaverphonic build "VERSION": ",67);
      attron(A_BOLD);
      mvaddnstr(1,0,program,-1);
      attroff(A_BOLD);
      update_editable();
      break;
    case 'e':
      if(editable && cue_list_number<cue_count)return(MENU_EDIT);
      break;
    case 'a':
      if(editable)
	return(MENU_ADD);
      break;
    case 'd':
      if(editable){
	halt_playback();
	move(0,0);
	attron(A_BOLD);
	addnlstr("Really delete cue? [y/N] ",80,' ');
	attroff(A_BOLD);
	refresh();
	ch=mgetch();
	if(ch=='y'){
	  unsaved=1;
	  delete_cue_bank(cue_list_number);
	  main_update_cues(10+MAX_CHANNELS);
	}
	move(0,0);
	addstr("MTG Beaverphonic build "VERSION": ");
	attron(A_BOLD);
	addstr(program);
	attroff(A_BOLD);
      }

      break;
    case 'o':
      if(editable)
	return(MENU_OUTPUT);
      break;
    case 's':
      save_top_level(program);
      mgetch();
      return(MENU_MAIN);
    case '-':case '_':
      unsaved=1;
      main_update_master(main_master_volume-1,5);
      break;
    case '+':case '=':
      unsaved=1;
      main_update_master(main_master_volume+1,5);
      break;
    case ' ':
      play_cue(cue_list_number);
      move_next_cue();
      break;
    case KEY_UP:case '\b':case KEY_BACKSPACE:
      move_prev_cue();
      break;
    case KEY_DOWN:
      move_next_cue();
      break;
    case 'H':
      halt_playback();
      break;
    case 'l':
      if(editable)
	editable=0;
      else
	editable=1;
      update_editable();
    case 0:
      main_update_tags(19+MAX_CHANNELS);
      main_update_playbuffer(4);
      main_update_outchannel_levels(7);
      main_update_inchannel_levels(7);
      break;
#if MAX_INPUT_CHANNELS
    case 'r':
      pthread_mutex_lock(&rec_mutex);
      rec_flush_req=1;
      pthread_mutex_unlock(&rec_mutex);
      break;
    case 'R':
      pthread_mutex_lock(&rec_mutex);
      rec_flush_req=0;
      pthread_mutex_unlock(&rec_mutex);
      break;
#endif
    }
  }
}

int main_keypress_menu(){
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Keypresses for main menu ");
  attron(A_BOLD);
  mvaddstr(2,2, "        ?");
  mvaddstr(4,2, "    space");
  mvaddstr(5,2, "  up/down");
  mvaddstr(6,2, "backspace");
  mvaddstr(8,2, "        a");
  mvaddstr(9,2, "        e");
  mvaddstr(10,2,"        d");
  mvaddstr(11,2,"        o");
  mvaddstr(12,2,"        l");
  mvaddstr(14,2,"        s");
  mvaddstr(15,2,"      +/-");
  mvaddstr(16,2,"        H");
  mvaddstr(17,2,"        R");

  attroff(A_BOLD);
  mvaddstr(2,12,"keypress menu (you're there now)");
  mvaddstr(4,12,"play next cue");
  mvaddstr(5,12,"move cursor to prev/next cue");
  mvaddstr(6,12,"move cursor to previous cue");
  mvaddstr(8,12,"add new cue at current cursor position");
  mvaddstr(9,12,"edit cue at current cursor position");
  mvaddstr(10,12,"delete cue at current cursor position");
  mvaddstr(11,12,"output channel configuration");
  mvaddstr(12,12,"lock non-modifiable mode (production)");

  mvaddstr(14,12,"save program");
  mvaddstr(15,12,"master volume up/down");
  mvaddstr(16,12,"halt playback");
  mvaddstr(17,12,"record to disk");

  mvaddstr(19,12,"any key to return to main menu");
  refresh();
  mgetch();
  return(MENU_MAIN);
}

int add_cue_menu(){
  form f;

  char label[13]="";
  char text[61]="";
  char desc[61]="";

  form_init(&f,4);
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add new cue ");

  mvaddstr(2,2,"      cue label");
  mvaddstr(3,2,"       cue text");
  mvaddstr(4,2,"cue description");

  field_add(&f,FORM_GENERIC,18,2,12,label);
  field_add(&f,FORM_GENERIC,18,3,59,text);
  field_add(&f,FORM_GENERIC,18,4,59,desc);

  field_add(&f,FORM_BUTTON,68,6,9,"ADD CUE");
  
  refresh();
  
  while(1){
    int ch=form_handle_char(&f,mgetch());
    switch(ch){
    case KEY_ENTER:case 'x':case 'X':
      goto enter;
    case 27: /* esc */
      goto out;
    }
  }

 enter:
  {
    /* determine where in list cue is being added */
    cue c;
        
    unsaved=1;
    memset(&c,0,sizeof(c));
    c.tag=-1; /* placeholder cue for this cue bank */
	/* acquire label locks, populate */
    c.label=new_label_number();
    edit_label(c.label,label);
    acquire_label(c.label);
    c.cue_text=new_label_number();
    edit_label(c.cue_text,text);
    acquire_label(c.cue_text);
    c.cue_desc=new_label_number();
    edit_label(c.cue_desc,desc);
    acquire_label(c.cue_desc);
    
    add_cue(cue_list_number,c);
    move_next_cue();
  }
 out:
  form_clear(&f);
  return(MENU_MAIN);
}


void edit_keypress_menu(){
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Keypresses for cue edit menu ");
  attron(A_BOLD);
  mvaddstr(2,2, "        ?");
  mvaddstr(3,2, "  up/down");
  mvaddstr(4,2, "      tab");
  mvaddstr(5,2, "        x");

  mvaddstr(7,2, "        a");
  mvaddstr(8,2, "        m");
  mvaddstr(9,2, "        d");
  mvaddstr(10,2,"    enter");
  mvaddstr(11,2,"        l");
  mvaddstr(12,2,"    space");
  mvaddstr(13,2,"        H");

  attroff(A_BOLD);
  mvaddstr(2,12,"keypress menu (you're there now)");
  mvaddstr(3,12,"move cursor to prev/next field");
  mvaddstr(4,12,"move cursor to next field");
  mvaddstr(5,12,"return to main menu");
  mvaddstr(7,12,"add new sample to cue");
  mvaddstr(8,12,"add new mixer change to cue");
  mvaddstr(9,12,"delete highlighted action");
  mvaddstr(10,12,"edit highlighted action");
  mvaddstr(11,12,"list all samples and sample tags");
  mvaddstr(12,12,"play cue");
  mvaddstr(13,12,"halt playback");

  mvaddstr(15,12,"any key to return to cue edit menu");
  refresh();
  mgetch();
}

void edit_sample_menu(int n){
  int i,j;
  cue *c=cue_list+n;
  tag *t=tag_list+cue_list[n].tag;
  char tdesc[61]="";
  char buf[82];
  form f;

  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add/Edit sample cue ");
  
  mvaddstr(10,2,"loop                       crosslap        ms");
  mvaddstr(11,2,"volume master        % fade        /       ms");
  
  form_init(&f,7+MAX_OUTPUT_CHANNELS*t->channels);
  strcpy(tdesc,label_text(t->sample_desc));

  field_add(&f,FORM_GENERIC,18,7,59,tdesc);

  field_add(&f,FORM_YESNO,18,10,5,&t->loop_p);
  field_add(&f,FORM_NUMBER,38,10,7,&t->loop_lapping);

  field_add(&f,FORM_PERCENTAGE,18,11,5,&c->mix.vol_master);
  field_add(&f,FORM_NUMBER,30,11,7,&c->mix.vol_ms);
  field_add(&f,FORM_NUMBER,38,11,7,&t->fade_out);
  
  mvaddstr(2,2,"cue label        ");
  addnlstr(label_text(c->label),59,' ');
  mvaddstr(3,2,"cue text         ");
  addnlstr(label_text(c->cue_text),59,' ');
  mvaddstr(4,2,"cue description  ");
  addnlstr(label_text(c->cue_desc),59,' ');

  mvaddstr(6,2,"sample path      ");
  addnlstr(label_text(t->sample_path),59,' ');

  mvaddstr(7,2,"sample desc");
  mvaddstr(8,2,"sample tag       ");
  sprintf(buf,"%d",c->tag);
  addstr(buf);

  mvhline(13,3,0,74);
  mvhline(14+MAX_OUTPUT_CHANNELS,3,0,74);
  mvvline(14,2,0,MAX_OUTPUT_CHANNELS);
  mvvline(14,77,0,MAX_OUTPUT_CHANNELS);
  mvaddch(13,2,ACS_ULCORNER);
  mvaddch(13,77,ACS_URCORNER);
  mvaddch(14+MAX_OUTPUT_CHANNELS,2,ACS_LLCORNER);
  mvaddch(14+MAX_OUTPUT_CHANNELS,77,ACS_LRCORNER);
  
  if(t->channels==2){
    mvaddstr(13,5," L ");
    mvaddstr(13,10," R ");
  }else{
    for(i=0;i<t->channels;i++){
      mvaddch(13,6+i*5,' ');
      addch(48+i);
      addch(' ');
    }
  }

  for(i=0;i<MAX_OUTPUT_CHANNELS;i++){
    for(j=0;j<t->channels && j<MAX_FILECHANNELS;j++)
      field_add(&f,FORM_PERCENTAGE,4+j*5,14+i,5,&c->mix.outvol[j][i]);
    sprintf(buf,"%% ->%2d ",i);
    mvaddstr(14+i,4+t->channels*5,buf);
    addnlstr(channel_list[i].label,76-11-t->channels*5,' ');
  }

  field_add(&f,FORM_BUTTON,61,17+MAX_OUTPUT_CHANNELS,16,"ACCEPT CHANGES");


#if 0
 Add/edit sample to cue -------------------------------------------------------

  cue label       [          ]
  cue text        [                                                          ]
  cue description [                                                          ]

  sample path     [                                                          ]
  sample desc     [                                                          ]
  sample tag      0 

  loop            [No ]     crosslap [----]ms                           
  volume master   [100]% fade [  15]/[  15]ms
                            
  --- L -- R -----------------------------------------------------------------
 |  [100][  0]% -> 0 offstage left                                            |
 |  [  0][100]% -> 1 offstage right                                           |
 |  [  0][  0]% -> 2                                                          |
 |  [  0][  0]% -> 3                                                          |
 |  [  0][  0]% -> 4                                                          |
 |  [  0][  0]% -> 5                                                          |
 |  [  0][  0]% -> 6                                                          |
 |  [  0][  0]% -> 7                                                          |
  ----------------------------------------------------------------------------

#endif

  while(1){
    int ch=form_handle_char(&f,getch());
    
    switch(ch){
    case ' ':
      play_sample(n);
      break;
    case 'H':
      halt_playback();
      break;
    case 27:
      goto out;
    case 'x':case 'X':case KEY_ENTER:
      unsaved=1;
      edit_label(t->sample_desc,tdesc);
      goto out;
    }
  }
 out:
  form_clear(&f);
}

void edit_mix_menu(int n){
  int i,j;
  cue *c=cue_list+n;
  tag *t=tag_list+cue_list[n].tag;
  char tdesc[61]="";
  char buf[82];
  form f;

  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add/Edit mix cue ");
  
  mvaddstr(10,2,"volume master        % fade        ms");
  
  form_init(&f,3+MAX_OUTPUT_CHANNELS*t->channels);
  strcpy(tdesc,label_text(t->sample_desc));

  field_add(&f,FORM_PERCENTAGE_R,18,10,5,&c->mix.vol_master);
  field_add(&f,FORM_NUMBER,30,10,7,&c->mix.vol_ms);
  
  mvaddstr(2,2,"cue label        ");
  addnlstr(label_text(c->label),59,' ');
  mvaddstr(3,2,"cue text         ");
  addnlstr(label_text(c->cue_text),59,' ');
  mvaddstr(4,2,"cue description  ");
  addnlstr(label_text(c->cue_desc),59,' ');

  mvaddstr(6,2,"sample path      ");
  addnlstr(label_text(t->sample_path),59,' ');

  mvaddstr(7,2,"sample desc      ");
  addnlstr(label_text(t->sample_desc),59,' ');
  mvaddstr(8,2,"sample tag       ");
  sprintf(buf,"%d",c->tag);
  addstr(buf);

  mvhline(12,3,0,74);
  mvhline(13+MAX_OUTPUT_CHANNELS,3,0,74);
  mvvline(13,2,0,MAX_OUTPUT_CHANNELS);
  mvvline(13,77,0,MAX_OUTPUT_CHANNELS);
  mvaddch(12,2,ACS_ULCORNER);
  mvaddch(12,77,ACS_URCORNER);
  mvaddch(13+MAX_OUTPUT_CHANNELS,2,ACS_LLCORNER);
  mvaddch(13+MAX_OUTPUT_CHANNELS,77,ACS_LRCORNER);
  
  if(t->channels==2){
    mvaddstr(12,5," L ");
    mvaddstr(12,10," R ");
  }else{
    for(i=0;i<t->channels;i++){
      mvaddch(12,6+i*5,' ');
      addch(48+i);
      addch(' ');
    }
  }

  for(i=0;i<MAX_OUTPUT_CHANNELS;i++){
    for(j=0;j<t->channels && j<MAX_FILECHANNELS;j++)
      field_add(&f,FORM_PERCENTAGE,4+j*5,13+i,5,&c->mix.outvol[j][i]);
    sprintf(buf,"%% ->%2d ",i);
    mvaddstr(13+i,4+t->channels*5,buf);
    addnlstr(channel_list[i].label,76-11-t->channels*5,' ');
  }

  field_add(&f,FORM_BUTTON,61,16+MAX_OUTPUT_CHANNELS,16,"ACCEPT CHANGES");


#if 0
 Add/edit mix change ---------------------------------------------------------

  cue label       [          ]
  cue text        [                                                          ]
  cue description [                                                          ]

  modify tag      [   0]
 
  volume master   [100]% fade [  15]
                            
  --- L -- R -----------------------------------------------------------------
 |  [100][  0]% -> 0 offstage left                                            |
 |  [  0][100]% -> 1 offstage right                                           |
 |  [  0][  0]% -> 2                                                          |
 |  [  0][  0]% -> 3                                                          |
 |  [  0][  0]% -> 4                                                          |
 |  [  0][  0]% -> 5                                                          |
 |  [  0][  0]% -> 6                                                          |
 |  [  0][  0]% -> 7                                                          |
  ----------------------------------------------------------------------------

 (l: list tags)

#endif

  while(1){
    int ch=form_handle_char(&f,getch());
    
    switch(ch){
    case 27:
      goto out;
    case 'x':case 'X':case KEY_ENTER:
      unsaved=1;
      goto out;
    }
  }
 out:
  form_clear(&f);

}

int add_sample_menu(){
  /* two-stage... get sample first so the mixer is accurate */
  tag t;
  tag_number tagno;

  form f;
  char path[64]="";

  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add new sample to cue ");
  
  form_init(&f,1);
  mvaddstr(2,2,"sample path");
  field_add(&f,FORM_GENERIC,14,2,62,path);
  f.edit=1;
  form_redraw(&f);

  while(1){
    int ch=mgetch();
    int re=form_handle_char(&f,ch);

    if(re>=0 || !f.edit){
      if(ch==27){
	form_clear(&f);
	return(MENU_EDIT);
      }

      /* try to load the sample! */
      switch_to_stderr();
      memset(&t,0,sizeof(t));
      if(load_sample(&t,path)){
	fprintf(stderr,"Press enter to continue\n");
	getc(stdin);
	switch_to_ncurses();
	f.edit=1;
      }else{
	switch_to_ncurses();
	break;
      }
    }
  }

  unsaved=1;

  /* finish the tag */
  {
    t.sample_path=new_label_number();
    acquire_label(t.sample_path);
    edit_label(t.sample_path,path);

    t.sample_desc=new_label_number();
    acquire_label(t.sample_desc);
    edit_label(t.sample_desc,"");
    t.loop_p=0;
    t.loop_start=0;
    t.loop_lapping=1000;

    t.fade_out=15;
    
    tagno=new_tag_number();
    edit_tag(tagno,t);
    acquire_tag(tagno);
  }

  /* got it, add a new cue */
  {
    cue c;
    
    acquire_label(c.label=cue_list[cue_list_number].label);
    acquire_label(c.cue_text=cue_list[cue_list_number].cue_text);
    acquire_label(c.cue_desc=cue_list[cue_list_number].cue_desc);
    c.tag=tagno;
    c.tag_create_p=1;
    memset(&c.mix,0,sizeof(mix));
    c.mix.vol_master=100;
    c.mix.vol_ms=10;
    c.mix.outvol[0][0]=100;
    c.mix.outvol[1][1]=100;

    add_cue(cue_list_number+1,c);
  }

  /* go to main edit menu */
  edit_sample_menu(cue_list_number+1);

  return(MENU_EDIT);
}

int tagno_to_cueno(int tagno){
  int i;

  for(i=0;i<cue_count;i++)
    if(cue_list[i].tag==tagno)return(i);

  return(-1);
} 

int add_mix_menu(){
  tag_number tagno=0;

  form f;

  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add mixer change to cue ");
  
  form_init(&f,1);
  mvaddstr(2,2,"tag number ");
  field_add(&f,FORM_NUMBER,13,2,12,&tagno);
  f.edit=1;
  form_redraw(&f);

  mvaddstr(4,2,"'");
  attron(A_BOLD);
  addstr("l");
  attroff(A_BOLD);
  addstr("' for a list of sample tags                    ");

  while(1){
    int ch=mgetch();
    int re=form_handle_char(&f,ch);

    if(re>=0 || !f.edit){
      if(ch==27){
	form_clear(&f);
	return(MENU_EDIT);
      }
      if(tagno>=tag_count || tag_list[tagno].refcount<=0){
	mvaddstr(4,2,"Bad tag number; any key to continue.");
	mgetch();
	mvaddstr(4,2,"'");
	attron(A_BOLD);
	addstr("l");
	attroff(A_BOLD);
	addstr("' for a list of sample tags                    ");
	f.edit=1;
	form_redraw(&f);
      }else
	break;
    }
  }

  /* add the new cue */
  /* prefill with original mix data from sample */
  {
    cue c;
    int i,j;
    int createno=tagno_to_cueno(tagno);
    unsaved=1;

    acquire_tag(tagno);
    acquire_label(c.label=cue_list[cue_list_number].label);
    acquire_label(c.cue_text=cue_list[cue_list_number].cue_text);
    acquire_label(c.cue_desc=cue_list[cue_list_number].cue_desc);
    c.tag=tagno;
    c.tag_create_p=0;
    memset(&c.mix,0,sizeof(mix));
    if(createno>-1){
      c.mix.vol_master=cue_list[createno].mix.vol_master;
      c.mix.vol_ms=cue_list[createno].mix.vol_ms;
      for(j=0;j<MAX_FILECHANNELS;j++)
	for(i=0;i<MAX_OUTPUT_CHANNELS;i++)
	  c.mix.outvol[j][i]=cue_list[createno].mix.outvol[j][i];
    }
    add_cue(cue_list_number+1,c);
  }

  /* go to main edit menu */
  edit_mix_menu(cue_list_number+1);

  return(MENU_EDIT);
}

int edit_cue_menu(){
  form f;
  char label[12]="";
  char text[61]="";
  char desc[61]="";

  /* determine first and last cue in bank */
  int base=cue_list_number;
  int first=base+1;
  int last=first;
  int actions,i;

  while(last<cue_count && cue_list[last].tag!=-1)last++;
  actions=last-first;

  form_init(&f,4+actions);
  strcpy(label,label_text(cue_list[base].label));
  strcpy(text,label_text(cue_list[base].cue_text));
  strcpy(desc,label_text(cue_list[base].cue_desc));

  field_add(&f,FORM_GENERIC,18,2,12,label);
  field_add(&f,FORM_GENERIC,18,3,59,text);
  field_add(&f,FORM_GENERIC,18,4,59,desc);
  
  for(i=0;i<actions;i++){
    char *buf=alloca(81);
    int tag=cue_list[first+i].tag;
    snprintf(buf,80,"%s sample %d (%s)",
	     cue_list[first+i].tag_create_p?"ADD":"MIX",tag,
	     label_text(tag_list[tag].sample_path));
    field_add(&f,FORM_BUTTON,11,6+i,66,buf);
  }
  if(actions==0){
    mvaddstr(6,11,"--None--");
    i++;
  }
  field_add(&f,FORM_BUTTON,66,7+i,11,"MAIN MENU");

  while(1){
    int loop=1;
    clear();
    box(stdscr,0,0);
    mvaddstr(0,2," Cue edit ");

    mvaddstr(2,2,"      cue label");
    mvaddstr(3,2,"       cue text");
    mvaddstr(4,2,"cue description");
    mvaddstr(6,2,"actions:");
    form_redraw(&f);
    
    while(loop){
      int ch=form_handle_char(&f,mgetch());
      
      switch(ch){
      case '?':case '/':
	edit_keypress_menu(); 
	loop=0;
	break;
      case 27:
	goto out;
      case 'x':case 'X':
	unsaved=1;
	edit_label(cue_list[base].label,label);
	edit_label(cue_list[base].cue_text,text);
	edit_label(cue_list[base].cue_desc,desc);
	goto out;
      case 'a':case 'A':
	add_sample_menu();
	form_clear(&f);
	return(MENU_EDIT);
      case 'm':case 'M':
	add_mix_menu();
	form_clear(&f);
	return(MENU_EDIT);
      case 'd': case 'D':
	if(actions>0){
	  if(f.cursor>=3 && f.cursor<3+actions){
	    int n=first+f.cursor-3;
	    unsaved=1;
	    delete_cue_single(n);
	    form_clear(&f);
	    return(MENU_EDIT);
	  }
	}
	break;
      case 'l': case 'L':
	//list_tag_menu();
	loop=0;
	break;
      case ' ':
	play_cue(cue_list_number);
	break;
      case 'H':
	halt_playback();
	break;
      case KEY_ENTER:
	if(f.cursor==3+actions){
	  unsaved=1;
	  edit_label(cue_list[base].label,label);
	  edit_label(cue_list[base].cue_text,text);
	  edit_label(cue_list[base].cue_desc,desc);
	  goto out;
	}
	/* ... else we're an action edit */
	if(cue_list[first+f.cursor-3].tag_create_p)
	   edit_sample_menu(first+f.cursor-3);
	else
	   edit_mix_menu(first+f.cursor-3);
	loop=0;
	break;
      }
    }
  }
  
 out:
  form_clear(&f);
  return(MENU_MAIN);
}

int menu_output(){
  return(MENU_MAIN);
}

int main_loop(){
  
  cache_cue(0);
  wake_cache();

  while(running){
    switch(menu){
    case MENU_MAIN:
      menu=main_menu();
      break;
    case MENU_KEYPRESS:
      menu=main_keypress_menu();
      break;
    case MENU_QUIT:
      running=0;
      break;
    case MENU_ADD:
      menu=add_cue_menu();
      break;
    case MENU_OUTPUT:
      menu=menu_output();
      break;
    case MENU_EDIT:
      menu=edit_cue_menu();
      break;
    }
  }
  return 0;
}

pthread_mutex_t pipe_mutex=PTHREAD_MUTEX_INITIALIZER;

void *tty_thread(void *dummy){
  char buf;

  while(1){
    int ret=read(ttyfd,&buf,1);
    if(ret==1){
      write(ttypipe[1],&buf,1);
    }
    
    if(playback_exit)break;
  }
  return NULL;
}

int main(int gratuitously,char *different[]){
  int lf;

  if(gratuitously<2){
    fprintf(stderr,"Usage: beaverphonic <settingfile>\n");
    exit(1);
  }

  mkdir(tempdir,0777);

  /* lock against other instances */
  lf=open(lockfile,O_CREAT|O_RDWR,0770);
  if(lf<0){
    fprintf(stderr,"unable to open lock file: %s.\n",strerror(errno));
    exit(1);
  }
  
  if(flock(lf,LOCK_EX|LOCK_NB)){
    fprintf(stderr,"Another Beaverphonic process is running.\n"
	    "  (could not acquire lockfile)\n");
    exit(1);
  }

#if (OUTPUT_CHANNELS1)
  playfd1=fopen(playdevice1,"wb");
  if(!playfd1){
    fprintf(stderr,"unable to open audio device 1 for playback: %s.\n",strerror(errno));
    fprintf(stderr,"\nPress enter to continue\n");
    getc(stdin);
  }
#endif

#if (OUTPUT_CHANNELS2)
  playfd2=fopen(playdevice2,"wb");
  if(!playfd2){
    fprintf(stderr,"unable to open audio device 2 for playback: %s.\n",strerror(errno));
    fprintf(stderr,"\nPress enter to continue\n");
    getc(stdin);
  }
#endif

#if (INPUT_CHANNELS1)
  recfd1=fopen(recdevice1,"rb");
  if(!recfd1){
    fprintf(stderr,"unable to open audio device 1 for record: %s.\n",strerror(errno));
    fprintf(stderr,"\nPress enter to continue\n");
    getc(stdin);
  }
#endif

#if (INPUT_CHANNELS2)
  recfd2=fopen(recdevice2,"rb");
  if(!recfd2){
    fprintf(stderr,"unable to open audio device 2 for record: %s.\n",strerror(errno));
    fprintf(stderr,"\nPress enter to continue\n");
    getc(stdin);
  }
#endif
    
  /* set up the hack for interthread ncurses event triggering through
   input subversion */
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

  pthread_create(&tty_thread_id,NULL,tty_thread,NULL);
  
  {
    pthread_t dummy;
    playback_active=1;
    pthread_create(&playback_thread_id,NULL,playback_thread,NULL);
    pthread_create(&cache_thread_id,NULL,cache_thread,NULL);
  }

#if (MAX_INPUT_CHANNELS)
  {
    pthread_t dummy;
    rec_active1=1;
    rec_active2=1;
    pthread_create(&record_thread_id,NULL,record_thread,NULL);
  }
#endif

  pthread_create(&cache_thread_id,NULL,cache_thread,NULL);

  /* load the sound config if the file exists, else create it */
  initscr(); cbreak(); noecho();
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  use_default_colors();
  signal(SIGINT,SIG_IGN);
  
  clear();
  switch_to_stderr();
  program=strdup(different[1]);
  {
    FILE *f=fopen(program,"rb");
    if(f){
      if(load_program(f)){
	fprintf(stderr,"\nPress enter to continue\n");
	getc(stdin);
      }
      fclose(f);
    }
  }
  switch_to_ncurses();

  main_thread_id=pthread_self();

  main_loop();
  endwin();                  /* restore original tty modes */  
  close(lf);
  halt_playback();
  playback_exit=1;

#if MAX_INPUT_CHANNELS
  pthread_mutex_lock(&rec_mutex);
  rec_exit=1;
  pthread_mutex_unlock(&rec_mutex);

  /* wake the record producer/consumer if either is waiting */
  pthread_mutex_lock(&rec_buffer_mutex);
  pthread_cond_broadcast(&rec_buffer_cond);
  pthread_mutex_unlock(&rec_buffer_mutex);

  while(1){
    pthread_mutex_lock(&rec_mutex);
    if(!rec_active1 && !rec_active2)break;
    pthread_mutex_unlock(&rec_mutex);
    sched_yield();
  }
  pthread_mutex_unlock(&rec_mutex);
#endif 

  while(1){
    if(!playback_active)break;
    sched_yield();
  }

#if (OUTPUT_CHANNELS1)
  fclose(playfd1);
#endif
#if (OUTPUT_CHANNELS2)
  fclose(playfd2);
#endif
#if (INPUT_CHANNELS1)
  fclose(recfd1);
#endif
#if (INPUT_CHANNELS2)
  fclose(recfd2);
#endif
  unlink(lockfile);
  return 0;
}  

