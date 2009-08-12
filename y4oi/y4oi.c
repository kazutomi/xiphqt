/*
 *
 *  y4oi:
 *     A utility for doing several simple but essential operations
 *     on yuv4ogg interchange streams.
 *
 *     y4oi copyright (C) 2009 Monty <monty@xiph.org>
 *
 *  y4oi is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  y4oi is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#define _FILE_OFFSET_BITS 64
#define EPSILON 1e-6

typedef struct {
  double begin;
  double end;
} interval;

typedef struct {
  double num;
  double den;
} ratio;

static int verbose=0;
static double fill_secs=25.;
static double sync_secs=1.;
static ratio force_fps={0,0};
static int force_sync=0;
static int force_no_sync=0;
static int global_ended=0;

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <getopt.h>
#include <limits.h>

typedef enum {
  Cmono=0,
  C411ntscdv=1,
  C420jpeg=2,
  C420mpeg2=3,
  C420paldv=4,
  C420unknown=5,
  C422jpeg=6,
  C422smpte=7,
  C422unknown=8,
  C444=9,
} chromafmt;

static char *chromaformat[]={
  "mono",      //0
  "411ntscdv", //1
  "420jpeg",   //2 chroma sample is centered vertically and horizontally between luma samples
  "420mpeg2",  //3 chroma sample is centered vertically between lines, cosited horizontally */
  "420paldv",  //4 chroma sample is cosited vertically and horizontally */
  "420unknown",//5
  "422jpeg",   //6 chroma sample is horizontally centered between luma samples */
  "422smpte",  //7 chroma sample is cosited horizontally */
  "422unknown",//8
  "444",       //9
  NULL
};

static unsigned char chromabits[]={
  8,
  12,
  12,
  12,
  12,
  12,
  16,
  16,
  16,
  24
};

static char *chromaformat_long[]={
  "monochrome",            //0
  "4:1:1 [ntscdv chroma]", //1
  "4:2:0 [jpeg chroma]",   //2 chroma sample is centered vertically and horizontally between luma samples
  "4:2:0 [mpeg2 chroma]",  //3 chroma sample is centered vertically between lines, cosited horizontally */
  "4:2:0 [paldv chroma]",  //4 chroma sample is cosited vertically and horizontally */
  "4:2:0 [unknown chroma]",//5
  "4:2:2 [jpeg chroma]",   //6 chroma sample is horizontally centered between luma samples */
  "4:2:2 [smpte chroma]",  //7 chroma sample is cosited horizontally */
  "4:2:2 [unknown chroma]",//8
  "4:4:4",                 //9
  NULL
};

typedef enum {
  STREAM_INVALID=0,
  STREAM_VIDEO=1,
  STREAM_AUDIO=2
} stream_type;

typedef enum {
  INVALID=-1,
  PROGRESSIVE=0,
  TOP_FIRST=1,
  BOTTOM_FIRST=2
} interlace_type;

typedef struct frame_s frame_t;
struct frame_s {
  double pts;
  size_t len;
  int streamno;
  int presync;
  int ticks;
  double duration;

  unsigned char *data;
  FILE *swap;
  off_t swap_pos;

  frame_t *next;
  frame_t *prev;

  frame_t *f_next;
  frame_t *f_prev;
};

typedef struct {
  FILE *f[2];
  off_t head[2];
  off_t tail[2];
  int write;
} swap_t;

typedef struct stream_s stream_t;
struct stream_s{
  stream_type type;
  int stream_num;
  double tickduration;
  long bytes_per_tick;
  double tolerance;

  union stream_t {
    struct {
      int rate;
      int ch;
    } audio;
    struct {
      int fps_n;
      int fps_d;
      int pa_n;
      int pa_d;
      int frame_n;
      int frame_d;
      int format;
      int w;
      int h;
      interlace_type i;
    } video;
  }m;

  frame_t  *inq_head;
  frame_t  *inq_tail;
  swap_t    inswap;
  long      tick_depth;

  stream_t *next;
  stream_t *prev;
};

typedef struct {
  FILE *f;
  int eof;
  stream_t **streams;
  int num_streams;
  int synced;
  int seekable;
  frame_t *f_head;
  frame_t *f_tail;
} y4o_in_t;

static int y4o_read_container_header(FILE *f, int *sync){
  char line[80];
  char *p;
  int n;
  size_t ret,len;

  len=sizeof("YUV4OGG");
  ret = fread(line, 1, len,f);
  if (ret<len){
    if(feof(f))
      fprintf(stderr, "ERROR: EOF reading y4o file header\n");
    return 1;
  }

  if (strncmp(line, "YUV4OGG ", sizeof("YUV4OGG ")-1)){
    fprintf(stderr, "ERROR: cannot parse y4o file header\n");
    return 1;
  }

  /* proceed to get the tags... (overwrite the magic) */
  for (n = 0, p = line; n < 80; n++, p++) {
    if ((ret=fread(p,1,1,f))<1){
      if(feof(f))
        fprintf(stderr,"ERROR: EOF reading y4o file header\n");
      return 1;
    }
    if (*p == '\n') {
      *p = '\0';
      break;
    }
  }
  if (n >= 80) {
    fprintf(stderr,"ERROR: line too long reading y4o file header\n");
    return 1;
  }

  /* read tags */
  /* S%c = sync */

  {
    char *token, *value;
    char tag;

    *sync=-1;

    /* parse fields */
    for (token = strtok(line, " ");
         token != NULL;
         token = strtok(NULL, " ")) {
      if (token[0] == '\0') continue;   /* skip empty strings */
      tag = token[0];
      value = token + 1;
      switch (tag) {
      case 'S':
        switch(token[1]){
        case 'y':
        case 'Y':
          *sync = 1;
          break;
        case 'n':
        case 'N':
          *sync=0;
          break;
        default:
          fprintf(stderr,"ERROR: unknown sync tag setting in y4o header\n");
          return 1;
        }
        break;
      default:
        fprintf(stderr,"ERROR: unknown file tag in y4o header\n");
        return 1;
      }
    }
  }

  if(*sync==-1)return 1;

  return 0;
}

static stream_t *y4o_read_stream_header(FILE *f){
  char line[80];
  char *p;
  int n;
  size_t ret;
  int af=0,vf=0;
  stream_t *s;

  /* Check for past the stream headers and at first frame */
  line[0]=getc(f);
  ungetc(line[0],f);
  if(line[0]=='F'){
    return NULL;
  }

  s=calloc(1,sizeof(*s));

  ret = fread(line, 1, sizeof("AUDIO"), f);
  if (ret<sizeof("AUDIO")){
    if(feof(f))
      fprintf(stderr,"ERROR: EOF reading y4o stream header\n");
    goto err;
  }

  if (!strncmp(line, "VIDEO ", sizeof("VIDEO ")-1)) vf=1;
  else if (!strncmp(line, "AUDIO ", sizeof("AUDIO ")-1)) af=1;

  if(!(af||vf)){
    fprintf(stderr,"ERROR: unknown y4o stream header type\n");
    goto err;
  }

  if(vf){
    /* video stream! */
    /* proceed to get the tags... (overwrite the magic) */
    for (n = 0, p = line; n < 80; n++, p++) {
      if ((ret=fread(p, 1, 1, f))<1){
        if(feof(f))
          fprintf(stderr,"ERROR: EOF reading y4o video stream header\n");
        goto err;
      }
      if (*p == '\n') {
        *p = '\0';
        break;
      }
    }
    if (n >= 80) {
      fprintf(stderr,"ERROR: line too long reading y4o video stream header\n");
      goto err;
    }

    /* read tags */
    /* W%d = width
       H%d = height
       F%d:%d = fps
       I%c = interlace
       A%d:%d = pixel aspect
       C%s = chroma format
    */

    {
      char *token, *value;
      char tag;
      int i;
      s->m.video.format=-1;
      s->m.video.i=-1;

      /* parse fields */
      for (token = strtok(line, " ");
           token != NULL;
           token = strtok(NULL, " ")) {
        if (token[0] == '\0') continue;   /* skip empty strings */
        tag = token[0];
        value = token + 1;
        switch (tag) {
        case 'W':
          s->m.video.w = atoi(token+1);
          break;
        case 'H':
          s->m.video.h = atoi(token+1);
          break;
        case 'F':
          {
            char *pos=strchr(token+1,':');
            if(pos){
              *pos='\0';
              s->m.video.fps_n = atoi(token+1);
              s->m.video.fps_d = atoi(pos+1);
              *pos=':';
            }else{
              s->m.video.fps_n = atoi(token+1);
              s->m.video.fps_d = 1;
            }
          }
          break;
        case 'I':
          switch(token[1]){
          case 'p':
          case 'P':
            s->m.video.i=PROGRESSIVE;
            break;
          case 't':
          case 'T':
            s->m.video.i=TOP_FIRST;
            break;
          case 'b':
          case 'B':
            s->m.video.i=BOTTOM_FIRST;
            break;
          default:
            fprintf(stderr,"ERROR: unknown y4o video interlace setting\n");
            goto err;
          }
        case 'A':
          {
            char *pos=strchr(token+1,':');
            if(pos){
              *pos='\0';
              s->m.video.pa_n = atoi(token+1);
              s->m.video.pa_d = atoi(pos+1);
              *pos=':';
            }else{
              s->m.video.pa_n = atoi(token+1);
              s->m.video.pa_d = 1;
            }
          }
          break;
        case 'C':
          for(i=0;chromaformat[i];i++)
            if(!strcasecmp(chromaformat[i],token+1))break;
          if(!chromaformat[i]){
            fprintf(stderr,"ERROR: unknown y4o video chroma format\n");
            goto err;
          }
          s->m.video.format=i;
          break;
        default:
          fprintf(stderr,"ERROR: unknown y4o video stream tag\n");
          goto err;
        }
      }
    }

    if(s->m.video.fps_n>0 &&
       s->m.video.fps_d>0 &&
       s->m.video.pa_n>0 &&
       s->m.video.pa_d>0 &&
       s->m.video.format>=0 &&
       s->m.video.w>0 &&
       s->m.video.h>0 &&
       s->m.video.i>=0)
      s->type=STREAM_VIDEO;
    else{
      fprintf(stderr,"ERROR: missing flags in y4o video stream header\n");
      goto err;
    }

    {
      int dw = s->m.video.w*s->m.video.pa_n;
      int dh = s->m.video.h*s->m.video.pa_d;
      int d;
      for(d=1;d<10000;d++)
        if(fabs(rint(dw/(double)dh*d) - dw/(double)dh*d)<EPSILON)
          break;
      s->m.video.frame_n=rint(dw/(double)dh*d);
      s->m.video.frame_d=d;
    }

    if(force_fps.num>0){
      s->m.video.fps_n = force_fps.num;
      s->m.video.fps_d = force_fps.den;
    }

    s->tickduration = (double)s->m.video.fps_d/s->m.video.fps_n;
    s->bytes_per_tick = s->m.video.w*s->m.video.h*chromabits[s->m.video.format]/8;
    s->tolerance = (double)s->m.video.fps_d/(double)s->m.video.fps_n;

    return s;

  }

  if(af){
    /* audio stream! */
    /* proceed to get the tags... (overwrite the magic) */
    for (n = 0, p = line; n < 80; n++, p++) {
      if ((ret=fread(p, 1, 1, f))<1){
        if(feof(f))
          fprintf(stderr,"ERROR: EOF reading y4o audio stream header\n");
        goto err;
      }
      if (*p == '\n') {
        *p = '\0';
        break;
      }
    }
    if (n >= 80) {
      fprintf(stderr,"ERROR: line too long reading y4o audio stream header\n");
      goto err;
    }

    /* read tags */
    /* R%d = rate
       C%d = channels
       all interchange audio is 24 bit signed LE
    */

    {
      char *token, *value;
      char tag;

      /* parse fields */
      for (token = strtok(line, " ");
           token != NULL;
           token = strtok(NULL, " ")) {
        if (token[0] == '\0') continue;   /* skip empty strings */
        tag = token[0];
        value = token + 1;
        switch (tag) {
        case 'R':
          s->m.audio.rate = atoi(token+1);
          break;
        case 'C':
          s->m.audio.ch = atoi(token+1);
          break;
        default:
          fprintf(stderr,"ERROR: unknown y4o audio stream tag\n");
          goto err;
        }
      }
    }

    if(s->m.audio.rate>0 &&
       s->m.audio.ch>0)
      s->type=STREAM_AUDIO;
    else{
      fprintf(stderr,"ERROR: missing flags in y4o stream header\n");
      goto err;
    }

    s->tickduration = (double)1./s->m.audio.rate;
    s->bytes_per_tick = 3*s->m.audio.ch;
    s->tolerance = .01;

    return s;
  }

 err:
  s->type = STREAM_INVALID;
  return s;
}

static y4o_in_t *y4o_open_in(FILE *f){
  // id the file before anything else
  int i;
  y4o_in_t *y;
  stream_t *ll=NULL;

  if(y4o_read_container_header(f,&i))
    return NULL;

  y=calloc(1,sizeof(*y));
  y->synced=i;

  // seekable?
  if(!fseeko(f,0,SEEK_CUR))
    y->seekable=1;

  // read stream headers
  while(1){
    stream_t *s=y4o_read_stream_header(f);
    if(!s)
      break;
    if(s->type == STREAM_INVALID)
      fprintf(stderr,"ERROR: Stream #%d unreadable; trying to continue\n",y->num_streams);
    s->stream_num=y->num_streams++;
    s->prev=ll;

    if(!y->seekable){
      s->inswap.f[0]=tmpfile();
      s->inswap.f[1]=tmpfile();
    }

    if(verbose)
      switch(s->type){
      case STREAM_VIDEO:
        fprintf(stderr,"Stream #%d, VIDEO %dx%d %d:%d %s %.3ffps %s\n",
                s->stream_num, s->m.video.w,s->m.video.h, s->m.video.frame_n,s->m.video.frame_d,
                chromaformat_long[s->m.video.format],s->m.video.fps_n/(double)s->m.video.fps_d,
                s->m.video.i?"interlaced":"progressive");
        break;
      case STREAM_AUDIO:
        fprintf(stderr,"Stream #%d, AUDIO %dHz, %d channel(s) [24bit]\n",
                s->stream_num, s->m.audio.rate,s->m.audio.ch);
      default:
        break;
      }

    if(ll)ll->next=s;
    ll=s;
  }

  // finish setting up stream list
  y->streams = calloc(y->num_streams,sizeof(*y->streams));
  for(i=y->num_streams;i>0;i--){
    y->streams[i-1]=ll;
    ll=ll->prev;
  }
  y->f = f;

  return y;
}

static void y4o_close_in(y4o_in_t *y){
  int i;
  if(y){
    if(y->streams){
      for(i=0;i<y->num_streams;i++){
        stream_t *s=y->streams[i];
        /* free frame queue */
        while(s->inq_tail){
          frame_t *next=s->inq_tail->next;
          if(s->inq_tail->data)
            free(s->inq_tail->data);
          free(s->inq_tail);
          s->inq_tail=next;
        }
        /* free swapfiles */
        if(s->inswap.f[0])
          fclose(s->inswap.f[0]);
        if(s->inswap.f[1])
          fclose(s->inswap.f[1]);
        free(s);
      }
      free(y->streams);
    }
    fclose(y->f);
    free(y);
  }
}

/* Reads one frame, potentially splits it internally into multiple
   frames; returns the first of the split frames */
static frame_t *y4o_read_frame(y4o_in_t *y){
  FILE *f=y->f;
  int streamno;
  int length;
  double pts;
  char line[80];
  char *p;
  int n;
  size_t ret;
  frame_t *pret=NULL;

  ret = fread(line, 1, sizeof("FRAME"),f);
  if (ret<sizeof("FRAME"))
  {
    /* A clean EOF should end exactly at a frame-boundary */
    if( ret != 0 && feof(f) )
      fprintf(stderr,"ERROR: EOF reading y4o frame\n");
    y->eof=1;
    return NULL;
  }

  if (strncmp(line, "FRAME ", sizeof("FRAME ")-1)){
    fprintf(stderr,"ERROR: loss of y4o framing\n");
    return NULL;
  }

  /* proceed to get the tags... (overwrite the magic) */
  for (n = 0, p = line; n < 80; n++, p++) {
    if ((ret=fread(p, 1, 1, f))<1){
      if(feof(f))
        fprintf(stderr,"ERROR: EOF reading y4o frame\n");
      return NULL;
    }
    if (*p == '\n') {
      *p = '\0';           /* Replace linefeed by end of string */
      break;
    }
  }
  if (n >= 80) {
    fprintf(stderr,"ERROR: line too long reading y4o frame header\n");
    return NULL;
  }

  /* read tags */
  /* S%d = streamno
     L%d = length
     P%g = pts */

  {
    char *token, *value;
    char tag;

    streamno=-1;
    length=-1;
    pts=-1;

    /* parse fields */
    for (token = strtok(line, " ");
         token != NULL;
         token = strtok(NULL, " ")) {
      if (token[0] == '\0') continue;   /* skip empty strings */
      tag = token[0];
      value = token + 1;
      switch (tag) {
      case 'S':
        streamno = atoi(token+1);
        break;
      case 'L':
        length = atoi(token+1);
        break;
      case 'P':
        pts = atof(token+1);
        break;
      default:
        fprintf(stderr,"ERROR: unknown y4o frame tag\n");
        return NULL;
      }
    }
  }

  if(streamno>=y->num_streams){
    fprintf(stderr,"ERROR: error reading frame; streamno out of range\n");
    return NULL;
  }

  if(streamno==-1 || length==-1 || pts==-1){
    fprintf(stderr,"ERROR: missing y4o frame tags; frame unreadable\n");
    return NULL;
  }

  /* read frame */
  {
    /* if this is huge audio frame, break it into smaller frames.  It
       will ease buffering in the encoder. */
    stream_t *s = y->streams[streamno];
    int total_ticks=length/s->bytes_per_tick;
    int max_ticks=4096;
    int ticks_sofar;
    for(ticks_sofar=0;ticks_sofar<total_ticks;ticks_sofar+=max_ticks){
      int ticks = ticks_sofar+max_ticks>total_ticks ? total_ticks-ticks_sofar : max_ticks;
      int bytes = ticks*s->bytes_per_tick;
      double thispts = pts+ticks_sofar*s->tickduration;

      frame_t *p=calloc(1,sizeof(*p));

      if(!p){
        fprintf(stderr,"ERROR: unable to allocate memory for frame\n");
        return pret;
      }
      p->pts=thispts;
      p->len=bytes;

      if(s->inq_tail && y->seekable){
        /* there's already queued data and this stream is seekable. Save positioning
           and seek past */
        p->swap = f;
        p->swap_pos = ftello(f);
        if(fseeko(f,length,SEEK_CUR)){
          fprintf(stderr,"ERROR: unable to advance in frame data; %s\n",strerror(errno));
          return pret;
        }
      }else{
        unsigned char *data=malloc(bytes);
        if(!data){
          fprintf(stderr,"ERROR: unable to allocate memory for frame\n");
          free(p);
          return pret;
        }

        ret=fread(data,1,bytes,f);
        if(ret<bytes){
          if(feof(f)){
            fprintf(stderr,"ERROR: unable to read frame; EOF\n");
          }else{
            fprintf(stderr,"ERROR: unable to read frame; %s\n",strerror(errno));
          }
          free(p);
          free(data);
          return pret;
        }

        /* If there's already queued data, this read goes straight to
           bufferswap */
        if(s->inq_tail){
          p->swap=s->inswap.f[s->inswap.write];
          p->swap_pos=s->inswap.head[s->inswap.write];
          s->inswap.head[s->inswap.write]+=bytes;
          if(fwrite(data,1,bytes,p->swap)<bytes){
            fprintf(stderr,"ERROR: unable to write to swap; %s\n",strerror(errno));
            free(p);
            free(data);
            return pret;
          }
          free(data);
        }else{
          p->data=data;
        }
      }

      p->streamno = streamno;
      p->ticks = ticks;
      p->duration = ticks * s->tickduration;

      p->prev=s->inq_head;
      if(s->inq_head){
        s->inq_head->next=p;
      }else{
        s->inq_tail=p;
      }
      s->inq_head=p;
      s->tick_depth+=p->ticks;

      p->f_prev=y->f_head;
      if(y->f_head){
        y->f_head->f_next=p;
      }else{
        y->f_tail=p;
      }
      y->f_head=p;

      if(!pret) pret=p;
    }

    return pret;
  }
}

static void y4o_free_frame(frame_t *p){
  if(p->data)free(p->data);
  memset(p,0,sizeof(*p));
  free(p);
}

// depth-limited fill.
// bound attempt to prime any given queue at the point any other queue reaches fill_secs deep
// actual depth-of-queue is used, not PTS (as PTS is unreliable and could be way off ino the weeds)

static int limited_prime(y4o_in_t *y, int sno){
  int i;
  double clock[y->num_streams];
  stream_t *s=y->streams[sno];
  if(s->inq_tail) return 0;

  for(i=0;i<y->num_streams;i++){
    stream_t *si=y->streams[i];
    clock[i] = si->tick_depth*si->tickduration;
  }

  while(!s->inq_tail){
    frame_t *p=y4o_read_frame(y);
    if(!p) return 1;
    i=p->streamno;
    while(p){
      clock[i]+=p->duration;
      p=p->next;
    }
    if(clock[i]>fill_secs){
      fprintf(stderr,"ERROR: Buffer depth exceeded configured limit due to A/V skew;\n"
              "       aborting input stream.\n");
      return 1;
    }
  }
  return 0;
}

// bounded sync search
// fill sync/search streams a min of search_secs deep past prime point each, filling no queue more than fill_secs past prime point
// depth-of-queue is used, not PTS (as PTS is unreliable and could be way off ino the weeds)

static int limited_prime_sync(y4o_in_t *y, int sync, int sno){
  int i;
  double clock[y->num_streams];
  stream_t *sy=y->streams[sync];
  stream_t *sn=y->streams[sno];
  frame_t *p=NULL;

  if(limited_prime(y,sync)) return 1;
  if(limited_prime(y,sno)) return 1;

  /* if both are already deep enough, done */
  if(sy->tick_depth*sy->tickduration >= sync_secs &&
     sn->tick_depth*sn->tickduration >= sync_secs)
    return 0;

  /* Count/fill from sync point */
  memset(clock,0,sizeof(clock));
  p=sy->inq_tail;
  while(p){
    i=p->streamno;
    if(p==sn->inq_tail){
      for(i=0;i<y->num_streams;i++){
        clock[i] = 0;
      }
    }
    clock[i]+=p->duration;
    p=p->f_next;
  }

  while(clock[sync] < sync_secs ||
        clock[sno] < sync_secs){
    p=y4o_read_frame(y);
    if(!p) return 1;
    i=p->streamno;
    while(p){
      clock[i]+=p->duration;
      p=p->next;
    }
    if(clock[i]>fill_secs)return 1;
  }

  return 0;
}

/* release swap in use if any */
static void release_frame_swap(stream_t *s, frame_t *p){
  if(p->swap){
    int from = (p->swap==s->inswap.f[0]?0:(p->swap==s->inswap.f[1]?1:-1));
    /* 'swap' might have been the seekable input file. */
    if(from>=0){
      s->inswap.tail[from]=p->swap_pos+p->len;

      /* if this is currently the write queue, swap banks */
      if(s->inswap.write==from){
        int b=!from;
        s->inswap.write=b;
        s->inswap.tail[b]=s->inswap.head[b]=0;
        if(fseeko(s->inswap.f[b],0,SEEK_SET)){
          fprintf(stderr,"ERROR: unable to seek in swap file; %s\n",strerror(errno));
          exit(1);
        }
      }
    }
  }

  p->swap=NULL;
  p->swap_pos=0;
}

static void remove_frame_from_stream(y4o_in_t *y, stream_t *s, frame_t *p){
  /* pull out of the stream queue */
  if(p->prev){
    p->prev->next=p->next;
  }else{
    s->inq_tail=p->next;
  }
  if(p->next){
    p->next->prev=p->prev;
  }else{
    s->inq_head=p->prev;
  }
  s->tick_depth-=p->ticks;

  /* pull out of the file queue */
  if(p->f_prev){
    p->f_prev->f_next=p->f_next;
  }else{
    y->f_tail=p->f_next;
  }
  if(p->f_next){
    p->f_next->f_prev=p->f_prev;
  }else{
    y->f_head=p->f_prev;
  }

  p->next=NULL;
  p->prev=NULL;
  p->f_next=NULL;
  p->f_prev=NULL;
}

static int swap_in_frame(y4o_in_t *y, frame_t *p){
  if(!p->data){
    off_t savepos;

    /* fetch data from swap / seekable stream */
    size_t sl=p->len;
    unsigned char *d=malloc(sl);
    if(!d){
      fprintf(stderr,"ERROR: unable to allocate memory for frame\n");
      return 1;
    }

    if(p->swap == y->f)
      savepos=ftello(y->f);

    if(fseeko(p->swap,p->swap_pos,SEEK_SET)){
      fprintf(stderr,"ERROR: unable to seek in swap file; %s\n",strerror(errno));
      free(d);
      return 1;
    }
    if(fread(d,1,sl,p->swap)<sl){
      if(d)free(d);
      if(feof(p->swap)){
        fprintf(stderr,"ERROR: unable to read frame from swap; EOF\n");
      }else{
        fprintf(stderr,"ERROR: unable to read frame from swap; %s\n",strerror(errno));
      }
      free(d);
      return 1;
    }

    if(p->swap == y->f)
      if(fseeko(p->swap,savepos,SEEK_SET)){
        fprintf(stderr,"ERROR: unable to seek in input file; %s\n",strerror(errno));
        exit(1);
      }

    p->data=d;
  }
  return 0;
}

static int parse_time(char *s,double *t){
  long sec=0;
  long usec=0;
  char *pos=strchr(s,':');
  sec=atol(s);
  if(pos){
    char *pos2=strchr(++pos,':');
    sec*=60;
    sec+=atol(pos);
    if(pos2){
      pos2++;
      sec*=60;
      sec+=atol(pos2);
      pos=pos2;
    }
  }else
    pos=s;
  pos=strchr(pos,'.');
  if(pos){
    int digits = strlen(++pos);
    usec=atol(pos);
    while(digits++ < 6)
      usec*=10;
  }
  *t = sec+(usec/(double)1000000);
  return 0;
}

static int parse_interval(char *s,interval *i){
  char *pos=strchr(s,'-');
  if(!pos)return 1;
  *pos='\0';
  parse_time(s,&i->begin);
  parse_time(pos+1,&i->end);
  *pos='-';
  return 0;
}

static int parse_ratio(char *s, ratio *r){
  char *pos=strpbrk(optarg,":/");
  if(pos){
    r->num=atof(optarg);
    r->den=atof(pos+1);
    if(r->den==0)return 1;
  }else{
    r->num=atof(optarg);
    r->den=1;
  }
  if(r->num==0)return 1;
  return 0;
}

const char *optstring = "b:c:e:f:ho:sSv";
struct option options [] = {
  {"begin",required_argument,NULL,'b'},
  {"cut",required_argument,NULL,'c'},
  {"end",required_argument,NULL,'e'},
  {"force-fps",required_argument,NULL,'f'},
  {"help",no_argument,NULL,'h'},
  {"output",required_argument,NULL,'o'},
  {"force-sync",no_argument,NULL,'s'},
  {"force-no-sync",no_argument,NULL,'S'},
  {"verbose",required_argument,NULL,'v'},

  {NULL,0,NULL,0}
};


void usage(FILE *out){
  fprintf(out,
          "\ny4theora 20090718\n"
          "performs basic operations on yuv4ogg interchange streams in prep for\n"
          "theora encoding\n\n"

          "USAGE:\n"
          "  y4theora [options] instream [instream...]\n\n"

          "OPTIONS:\n"
          "  -b --begin HH:MM:SS.XX   : discard starting data up to specified\n"
          "                             begin time.  Time is measured relative to\n"
          "                             output clock\n"
          "  -c --cut BEGIN-END       : drop output data in the specified interval.\n"
          "                             Time is measured relative to output clock,\n"
          "                             but output timestamps will be re-written\n"
          "                             such that there is no gap.\n"
          "  -e --end HH:MM:SS.XX     : stop working at specified end time.  Time\n"
          "                             is measured relative to output clock\n"
          "  -f --force-fps N:D       : Override declared input fps\n"
          "  -h --help                : print this usage message to stdout and exit\n"
          "                             with status zero\n"
          "  -o --output FILE         : output file/pipe (stdout default)\n"
          "  -s --force-sync          : perform autosync even on streams marked as\n"
          "                             already synchronized\n"
          "  -S --force-no-sync       : do not perform stream sync and PTS rewrite\n"
          "                             even on unsynced streams\n"
          "  -v --verbose             : turn on all reports\n"
          "\n"
          );
}

static int search_offset(y4o_in_t *y, int sync, int sno,
                  long long *outclock, double *outoffsets, frame_t **lastframe){
  /* first check is against prev sync frame out */
  stream_t *s=y->streams[sno];

  if(sync==sno || sync==-1){
    s->inq_tail->presync=1;
    outoffsets[sno]=0.;
    return 0;
  }

  if(s->inq_tail->presync)
    return 0;
  else{
    stream_t *ss=y->streams[sync];
    frame_t *p = lastframe[sync];
    double sync_clock = outclock[sync]*ss->tickduration;
    double sno_clock = outclock[sno]*s->tickduration;
    double best_time;
    frame_t *best_frame = NULL;
    double sync_offset=0;
    double last_sno_pts;
    if(p){
      sync_offset = p->pts + p->duration - sync_clock;
      best_time = sno_clock - s->inq_tail->pts + sync_offset;
      if(fabs(best_time) < s->tolerance){
        s->inq_tail->presync=1;
        return 0;
      }
    }else if (ss->inq_tail)
      sync_offset = ss->inq_tail->pts-sync_clock;

    /* it would appear we're potentially out of tolerance.  pre-fill
       to the desired sync depth */
    if(limited_prime_sync(y, sync, sno))
      return 1;

    /* search from y's file tail up to s's tail packet just to make
       sure there's not an out of order packet from the sync stream */
    p=y->f_tail;
    last_sno_pts = s->inq_tail->pts;
    while(p!=s->inq_tail){
      if(p->streamno==sync){
        double time;
        sync_offset = p->pts - sync_clock;
        time = sno_clock - last_sno_pts + sync_offset;
        if(!best_frame || fabs(time) <= fabs(best_time)){
          best_time=time;
          best_frame=p;
        }
        sync_clock += p->duration;
      }
      p=p->f_next;
    }

    /* search forward sync_secs from next sync frame looking for tightest timing */
    {
      int count=0;
      double search_clock=sync_clock;
      while(sync_clock<search_clock+sync_secs && p){
        if(p->streamno==sync){
          sync_offset = p->pts - sync_clock;
          sync_clock += p->duration;
        }
        if(p->streamno==sno){
          count++;
          sno_clock += p->duration;
          last_sno_pts = p->pts+p->duration;
        }
        if(p->streamno==sync || p->streamno==sno){
          double time = sno_clock-last_sno_pts+sync_offset;
          if(!best_frame || fabs(time) <= fabs(best_time)){
            best_time=time;
            best_frame=p;
          }
        }

        p=p->f_next;
      }
    }

    outoffsets[sno]=best_time;

    /* mark presync up to the position of best timing */
    p=y->f_tail;
    while(p){
      if(p->streamno==sno)
        p->presync=1;
      if(p==best_frame)break;
      p=p->f_next;
    }
    return 0;
  }
}

static char timebuffer[80];
static char *make_time_string(double s){
  long hrs=s/60/60;
  long min=s/60-hrs*60;
  long sec=s-hrs*60*60-min*60;
  long hsec=(s-(int)s)*100;
  if(hrs>0){
    snprintf(timebuffer,80,"%ld:%02ld:%02ld.%02ld",hrs,min,sec,hsec);
  }else if(min>0){
    snprintf(timebuffer,80,"%ld:%02ld.%02ld",min,sec,hsec);
  }else{
    snprintf(timebuffer,80,"%ld.%02ld",sec,hsec);
  }
  return timebuffer;
}

/* a frame_pop/free that doesn't lock swap data into memory */
static int discard_head_frame(y4o_in_t *y,stream_t *s, long long *outclock){
  frame_t *p=s->inq_head;
  if(!p) return 1;

  // no need to release swap; it will evaporate on its own
  remove_frame_from_stream(y,s,p);

  if(verbose){
    if(s->type==STREAM_VIDEO){
      fprintf(stderr,"Stream %d [%s]: Dropping video frame to maintain sync\n",
              p->streamno,
              make_time_string((outclock[p->streamno]+s->tick_depth)*s->tickduration));
    }else{
      fprintf(stderr,"Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
              p->streamno,
              make_time_string((outclock[p->streamno]+s->tick_depth)*s->tickduration),
              p->duration);
    }
  }

  y4o_free_frame(p);

  return 0;
}

/* a frame_pull/free that doesn't lock swap data into memory */
static int discard_tail_frame(y4o_in_t *y,stream_t *s, long long *outclock){
  frame_t *p=s->inq_tail;
  if(!p) return 1;

  if(verbose){
    if(s->type==STREAM_VIDEO){
      fprintf(stderr,"Stream %d [%s]: Dropping video frame to maintain sync\n",
              p->streamno,
              make_time_string(outclock[p->streamno]*s->tickduration));
    }else{
      fprintf(stderr,"Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
              p->streamno,
              make_time_string(outclock[p->streamno]*s->tickduration),
              p->duration);
    }
  }

  release_frame_swap(s,p);
  remove_frame_from_stream(y,s,p);
  y4o_free_frame(p);
  return 0;
}

/* conditionally remove data from the head; in the case of audio it
   can discard partial frames */
static int trim_from_head(y4o_in_t *y,int sno,long long *outclock,double *outoffsets){
  stream_t *s=y->streams[sno];
  frame_t *p=s->inq_head;
  if(!p)return 1;
  if(s->type==STREAM_AUDIO){
    int samples = p->len/(s->m.audio.ch*3);
    int remsamples = outoffsets[sno]*s->m.audio.rate;
    if(samples<=remsamples){
      /* remove whole frame */
      if(samples==remsamples)
        outoffsets[sno] = 0.;
      else
        outoffsets[sno] -= p->duration;
      return discard_head_frame(y,s,outclock);
    }else{
      /* trim frame */
      if(remsamples>0){
        /* altering the len will not mess up swap */
        p->len -= remsamples * s->m.audio.ch*3;
        p->ticks -= remsamples;
        p->duration -= remsamples*s->tickduration;
        s->tick_depth -= remsamples;

        if(verbose)
          fprintf(stderr,"Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
                  p->streamno,
                  make_time_string((outclock[sno]+s->tick_depth)*s->tickduration),
                  outoffsets[sno]);

      }
      outoffsets[sno] = 0.;
    }
  }else if(s->type==STREAM_VIDEO){
    /* because video granularity is relatively coarse, only remove the
       frame if it gets us closer to the time goal */
    if(fabs(outoffsets[sno]-p->duration)<outoffsets[sno]){
      /* remove frame */
      outoffsets[sno] -= p->duration;
      if(outoffsets[sno]<0) outoffsets[sno]=0.;
      return discard_head_frame(y,s,outclock);
    }else
      outoffsets[sno]=0.;
  }else
    return 1;
  return 0;
}

/* conditionally remove data from the tail; in the case of audio it
   can discard partial frames */
static int trim_from_tail(y4o_in_t *y,int sno,long long *outclock,double *outoffsets){
  stream_t *s=y->streams[sno];
  frame_t *p=s->inq_tail;
  if(!p)return 1;

  if(s->type==STREAM_AUDIO){
    int samples = p->len/(s->m.audio.ch*3);
    int remsamples = outoffsets[sno]*s->m.audio.rate;
    if(samples<=remsamples){
      /* remove whole frame */
      if(samples==remsamples)
        outoffsets[sno] = 0.;
      else
        outoffsets[sno] -= p->duration;
      return discard_tail_frame(y,s,outclock);
    }else{
      /* trim frame */
      if(remsamples>0){
        int bytes = remsamples * s->m.audio.ch*3;

        if(verbose)
          fprintf(stderr,"Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
                  p->streamno,make_time_string(outclock[sno]*s->tickduration),
                  outoffsets[sno]);

        /* load frame data into memory and release swap */
        swap_in_frame(y,p);
        release_frame_swap(s,p);
        memmove(p->data,p->data+bytes,p->len-bytes);
        p->len -= bytes;
        p->duration -= remsamples*s->tickduration;
        p->ticks -= remsamples;
        s->tick_depth -= remsamples;
      }
      outoffsets[sno] = 0.;
    }
  }else if(s->type==STREAM_VIDEO){
    /* because video granularity is relatively coarse, only remove the
       frame if it gets us closer to the time goal */
    if(fabs(outoffsets[sno]-p->duration)<outoffsets[sno]){
      /* remove frame */
      outoffsets[sno] -= p->duration;
      if(outoffsets[sno]<0) outoffsets[sno]=0.;
      return discard_tail_frame(y,s,outclock);
    }else
      outoffsets[sno]=0.;
  }else
    return 1;
  return 0;
}

static void write_frame_i(FILE *outfile, int sno, unsigned char *b, int len, double pts){

  if(fprintf(outfile,"FRAME S%d L%d P%.3f\n",sno,len,pts)<0 ||
     fwrite(b,1,len,outfile)<len){
    fprintf(stderr,"ERROR: Unable to write to output; %s\n",strerror(errno));
    exit(1);
  }
}

typedef struct {
  long long begin;
  long long end; // one past
} cutentry;

typedef struct {
  int cuts;
  cutentry *cut;
  int ended;
} cutlist;

static int intervalcmp(const void *p1, const void *p2){
  cutentry *a=(cutentry *)p1;
  cutentry *b=(cutentry *)p2;

  return (int)(a->begin-b->begin);
}

/* Sanitize and distill the list of user-requested cuts into a more
   efficient form */

static cutlist *cuts_into_cutlist(y4o_in_t *y, interval *list, int n, ratio fps){
  cutlist *ret=calloc(y->num_streams,sizeof(*ret));
  cutentry *l = calloc(n,sizeof(*l));
  int i,j;

  /* Quantize the cut requests to primary video stream frames.  Video
     has much coarser resolution than audio, and we likely care about
     the primary stream over any subsidary streams.  Quantizing to the
     vid stream we care about most will result in the most predictable
     cut behavior. */

  for(i=0;i<n;i++){
    l[i].begin = (long long)rint(list[i].begin*fps.num/fps.den);
    if(list[i].end<0)
      l[i].end = -1;
    else
      l[i].end = (long long)rint(list[i].end*fps.num/fps.den);
  }

  /* sort by beginning time */
  qsort(l,n,sizeof(*l),intervalcmp);

  /* remove nonsensical cut regions */
  for(i=0;i<n;i++){
    int flag=0;
    /* zero or negative range check */
    if(l[i].begin>=l[i].end && !l[i].end<0) flag=1;
    if(flag){
      if(i+1<n)
        memcpy(&l[i],&l[i+1],(n-i-1)*sizeof(l[i]));
      n--;
    }
  }

  /* merge overlapping cut ranges */
  for(i=1;i<n;i++){
    if(l[i].begin<=l[i-1].end){
      if(l[i-1].end<l[i].end && !l[i-1].end<0)
        l[i-1].end=l[i].end;
      if(i+1<n)
        memcpy(&l[i],&l[i+1],(n-i-1)*sizeof(l[i]));
      n--;
    }
  }

  /* convert cut ranges into native tick units of each stream,
     tracking fractional ticks forward across cuts so there's no
     cumulative drift */
  for(j=0;j<y->num_streams;j++){
    stream_t *s = y->streams[j];
    double track=0.;
    ret[j].cut=calloc(n,sizeof(*ret[j].cut));
    for(i=0;i<n;i++){
      double b = l[i].begin*(double)fps.den/fps.num;
      double e = (l[i].end<0?-1:l[i].end*(double)fps.den/fps.num);
      long long bt = ceil(b/s->tickduration-EPSILON);
      long long et = (e<0?LONG_MAX:ceil(e/s->tickduration-EPSILON));
      double bf = b/s->tickduration - bt;
      double ef = (e<0?0:et - e/s->tickduration);

      /* compensate for cumulative fractional ticks.
         Rather than choosing the closest frame, we always choose the
         next; it is better for video to slightly lead than slightly
         lag. */

      track += bf+ef;
      if(track-EPSILON>1.){
        et--;
        track-=1;
      }
      if(track+EPSILON<0.){
        et++;
        track+=1;
      }

      ret[j].cut[i].begin = bt;
      ret[j].cut[i].end = et;
    }
    ret[j].cuts=n;

  }

  return ret;
}

/* Cut functionality injected here; thus the seperate out and cut clocks */
static void write_frame(FILE *outfile, y4o_in_t *y,
                        long long *outclock,
                        long long *cutclock, cutlist *cut,
                        frame_t *p){
  int sno = p->streamno;
  stream_t *s=y->streams[sno];
  double outpts = (force_no_sync?p->pts:cutclock[sno]*s->tickduration);
  cutlist *c=&cut[sno];

  /* does the next cut impact this frame? */
  if(c->cuts && outclock[sno]+p->ticks>c->cut[0].begin){
    /* is the entire frame to be cut? */
    if(outclock[sno]<c->cut[0].begin || outclock[sno]+p->ticks > c->cut[0].end){
      /* no, frame must be bisected. */
      /* is any of the frame prior to cut beginning? */
      if(outclock[sno]<c->cut[0].begin){
        /* yes; write the data prior to the cut, mutate current packet
           and push the rest back onto the stream tail for another
           pass */
        int beginticks=c->cut[0].begin-outclock[sno];
        int beginbytes=beginticks*s->bytes_per_tick;

        write_frame_i(outfile, sno, p->data, beginbytes, outpts);
        cutclock[sno]+=beginticks;
        outclock[sno]+=beginticks;

        /* mutate packet */
        p->data+=beginbytes;
        p->len-=beginbytes;
        p->ticks-=beginticks;

        /* recurse */
        write_frame(outfile, y, outclock, cutclock, cut, p);

        /* restore packet */
        p->data-=beginbytes;
        p->len+=beginbytes;
        p->ticks+=beginticks;

      }else{
        /* no; the beginning is cut.  Drop it, mutate current packet
           and push any remaining data at the end back onto the stream
           tail for another pass (don't just write it, there may be
           another cut here in this same frame) */
        int beginticks=c->cut[0].end-outclock[sno];
        int beginbytes=beginticks*s->bytes_per_tick;

        outclock[sno]+=beginticks;

        /* mutate packet */
        p->data+=beginbytes;
        p->len-=beginbytes;
        p->ticks-=beginticks;

        /* remove cut entry */
        memmove(c->cut,c->cut+1,(c->cuts-1)*sizeof(*c->cut));
        c->cuts--;

        /* recurse */
        write_frame(outfile, y, outclock, cutclock, cut, p);

        /* restore packet */
        p->data-=beginbytes;
        p->len+=beginbytes;
        p->ticks+=beginticks;

      }
    }else{
      /* drop whole frame */
      outclock[sno]+=p->ticks;
      if(c->cuts==1 && c->cut[0].end==LONG_MAX && !c->ended){
        c->ended=1;
        global_ended++;
      }
    }
  }else{
    /* No cuts, write the frame */
    write_frame_i(outfile, sno, p->data, p->len, outpts);
    cutclock[sno]+=p->ticks;
    outclock[sno]+=p->ticks;
  }
}

static void pull_write_frame(FILE *outfile, y4o_in_t *y,frame_t **lastframe,
                             long long *outclock,long long *cutclock, cutlist *cutticks,
                             int sno){
  stream_t *s = y->streams[sno];
  frame_t *p = s->inq_tail;

  if(!p) return;

  swap_in_frame(y,p);
  release_frame_swap(s,p);
  remove_frame_from_stream(y,s,p);
  write_frame(outfile,y,outclock,cutclock,cutticks,p);
  if(lastframe[sno])
    y4o_free_frame(lastframe[sno]);
  lastframe[sno]=p;

}

/* conditionally duplicates last frame in video stream, or generates a frame of audio silence */
static int duplicate_frame(FILE *outfile,y4o_in_t *y,frame_t **lastframe, long long *outclock,
                           long long *cutclock, cutlist *cutticks,
                           double *outoffsets, int sno){
  stream_t *s = y->streams[sno];
  frame_t *p = lastframe[sno];

  if(!p){
    /* no preceeding frame.  Look to next frame instead */
    if(limited_prime(y,sno)) return 1;
    p=s->inq_tail;
    swap_in_frame(y,p);
    release_frame_swap(s,p);
  }

  switch(s->type){
  case STREAM_VIDEO:
    /* we only dup the frame if doing so gets us closer to the ideal clock sync */
    if(fabs(outoffsets[sno]+p->duration)<fabs(outoffsets[sno])){
      /* dup it */
      if(verbose)
        fprintf(stderr,"Stream %d [%s]: Repeating video frame to maintain sync\n",
                p->streamno,make_time_string(outclock[sno]*s->tickduration));

      write_frame(outfile,y,outclock,cutclock,cutticks,p);
      outoffsets[sno]+=p->duration;
      if(outoffsets[sno]>0)outoffsets[sno]=0.;
    }else
      outoffsets[sno]=0.;
    break;
  case STREAM_AUDIO:
    /* can't dup audio.  Write silence (evenrually, we want to
       extrapolate and smooth the discontinuity) */
    {
      frame_t lp;
      /* number of samples already bounded by fill_secs */
      int samples = -outoffsets[sno] * s->m.audio.rate;
      int bytes = samples * s->m.audio.ch;
      unsigned char *data=calloc(bytes,1);

      if(verbose)
        fprintf(stderr,"Stream %d [%s]: Adding %gs of silence to maintain sync\n",
                p->streamno,make_time_string(outclock[sno]*s->tickduration),-outoffsets[sno]);

      lp.data=data;
      lp.len=bytes;
      lp.streamno=sno;
      lp.pts=(p?p->pts:0.);
      lp.ticks=samples;
      write_frame(outfile,y,outclock,cutclock,cutticks,&lp);
      free(data);
      if(p)p->pts-=outoffsets[sno];
      outoffsets[sno]=0.;
    }
    break;
  default:
    break;
  }
  return 0;
}

int main(int argc,char *const *argv){
  int c,long_option_index;

  long long *outclock=NULL;
  long long *cutclock=NULL;
  cutlist *cutticks=NULL;
  double *outoffsets=NULL;
  frame_t **lastframe=NULL;

  /* for time cuts */
  double begin=-1;
  double end=-1;
  interval *ccut=NULL;
  int ccuts=0;

  FILE *outfile = NULL;
  int outheader=0;

  FILE **infile = NULL;
  char *const*infilenames=NULL;
  int infiles=0,i;

  int primary_video=-1;
  int primary_audio=-1;
  int sync_stream=-1;

  y4o_in_t *ty=NULL;
  y4o_in_t *y=NULL;

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'h':
      usage(stdout);
      return 0;
    case 'v':
      verbose=1;
      break;
    case 'b':
      parse_time(optarg,&begin);
      break;
    case 'e':
      parse_time(optarg,&end);
      break;
    case 'f':
      if(parse_ratio(optarg,&force_fps)){
        fprintf(stderr,"ERROR: cannot parse fps argument '%s'\n",optarg);
        exit(1);
      }
      break;
    case 'c':
      if(!ccut)
        ccut=calloc(1,sizeof(*ccut));
      else
        ccut=realloc(ccut,(ccuts+1)*sizeof(*ccut));
      if(parse_interval(optarg,ccut+ccuts)){
        fprintf(stderr,"ERROR: cannot parse cut interval '%s'\n",optarg);
        exit(1);
      }
      ccuts++;
      break;
    case 's':
      force_sync=1;
      force_no_sync=0;
      break;
    case 'S':
      force_sync=0;
      force_no_sync=1;
      break;
    case 'o':
      if(outfile)
        fclose(outfile);
      outfile=fopen(optarg,"wb");
      if(!outfile){
        fprintf(stderr,"ERROR: Unable to open '%s' for output; %s\n",optarg,strerror(errno));
        exit(1);
      }
      break;
    default:
      usage(stderr);
      exit(1);
    }
  }

  if(optind<argc){
    /* assume that anything following the options must be an input filename */
    infiles=argc-optind;
    infile=calloc(infiles,sizeof(*infile));
    infilenames=argv+optind;

    for(i=0;i<infiles;i++){
      infile[i]=fopen(argv[optind+i],"rb");
      if(infile[i]==NULL){
        fprintf(stderr,"ERROR: Unable to open '%s' for input; %s\n",argv[optind+i],strerror(errno));
        exit(1);
      }
    }
    optind++;
  }

  if(outfile==NULL)outfile=stdout;

  for(i=0;i<infiles;i++){
    int j;

    if(verbose)
      fprintf(stderr,"Begin processing input stream \"%s\"...\n",infilenames[i]);

    ty=y4o_open_in(infile[i]);

    if(y==NULL){

      for(j=0;j<ty->num_streams;j++){
        stream_t *s = ty->streams[j];
        switch(s->type){
        case STREAM_VIDEO:
          if(primary_video<0)primary_video=j;
          if(verbose)
            fprintf(stderr,"Using stream %d as primary video stream\n",j);
          break;
        case STREAM_AUDIO:
          if(primary_audio<0)primary_audio=j;
          if(verbose)
            fprintf(stderr,"Using stream %d as primary audio stream\n",j);
          break;
        default:
          break;
        }
      }
      if(sync_stream==-1)
        sync_stream=primary_audio;
    }

    /* warn about force mismatches, unknown chroma spaces */
    if(!force_fps.num && primary_video>=0){
      force_fps.num=ty->streams[primary_video]->m.video.fps_n;
      force_fps.den=ty->streams[primary_video]->m.video.fps_d;
    }

    if(!cutticks){
      /* translate begin/end to cut intervals */
      if(begin!=-1){
        if(!ccut)
          ccut=calloc(1,sizeof(*ccut));
        else
          ccut=realloc(ccut,(ccuts+1)*sizeof(*ccut));
        ccut[ccuts].begin=0;
        ccut[ccuts].end=begin;
        ccuts++;
      }

      if(end!=-1){
        if(!ccut)
          ccut=calloc(1,sizeof(*ccut));
        else
          ccut=realloc(ccut,(ccuts+1)*sizeof(*ccut));
        ccut[ccuts].begin=end;
        ccut[ccuts].end=-1;
        ccuts++;
      }

      cutticks=cuts_into_cutlist(ty, ccut, ccuts, force_fps);

    }

    for(j=0;j<ty->num_streams;j++){
      switch(ty->streams[j]->type){
      case STREAM_VIDEO:
        if(fabs(force_fps.num/force_fps.den -
                ty->streams[j]->m.video.fps_n/(double)ty->streams[j]->m.video.fps_d)>EPSILON){
          fprintf(stderr,"Forcing stream %d file \"%s\" to %.3ffps.\n",
                  j,infilenames[i],force_fps.num/(double)force_fps.den);
        }
        if(ty->streams[j]->m.video.format == C420unknown){
          fprintf(stderr,"WARNING: Assuming mpeg2 chroma positioning for stream %d.\n",
                  j);
          ty->streams[j]->m.video.format = C420mpeg2;
        }
        if(ty->streams[j]->m.video.format == C422unknown){
          fprintf(stderr,"WARNING: Assuming smpte chroma positioning for stream %d.\n",
                  j);
          ty->streams[j]->m.video.format = C422smpte;
        }
        break;
      default:
        break;
      }
    }

    if(y){
      /* if opening a new file in sequence, it must have the same traits as the first */
      if(ty->num_streams!=y->num_streams){
        fprintf(stderr,"ERROR: Number of streams mismatch in file \"%s\".\n"
                "       Sequential input streams must have matching stream\n"
                "       numbers and types.\n",
                infilenames[i]);
        exit(1);
      }
      for(j=0;j<y->num_streams;j++){
        stream_t *r = y->streams[j];
        stream_t *s = ty->streams[j];

        if(s->type != r->type){
          fprintf(stderr,"ERROR: Stream %d type mismatch in file \"%s\".\n"
                  "       Sequential input streams must have matching stream\n"
                  "       numbers and types.\n",
                  j,infilenames[i]);
          exit(1);
        }

        /* eventually, we should be able to autoconvert and force all
           these settings */
        switch(s->type){
          case STREAM_VIDEO:
            if(s->m.video.w != r->m.video.w ||
               s->m.video.h != r->m.video.h){
              fprintf(stderr,"ERROR: Video dimensions mismatch in stream %d file \"%s\".\n"
                      "       Sequential video streams must have matching dimensions.\n",
                      j,infilenames[i]);
              exit(1);
            }
            if(abs(s->m.video.pa_n/(double)s->m.video.pa_d -
                   r->m.video.pa_n/(double)r->m.video.pa_d)>EPSILON){
              fprintf(stderr,"WARNING: Pixel aspect mismatch in stream %d file \"%s\".\n"
                      "         Forcing aspect to match original stream.\n",
                      j,infilenames[i]);
              s->m.video.pa_n = r->m.video.pa_n;
              s->m.video.pa_d = r->m.video.pa_d;
            }
            if(s->m.video.format != r->m.video.format){
              fprintf(stderr,"ERROR: Chroma format mismatch in stream %d file \"%s\".\n"
                      "       Sequential video streams must have matching chroma\n"
                      "       formats.\n",
                      j,infilenames[i]);
              exit(1);
            }
            if(s->m.video.i != r->m.video.i){
              fprintf(stderr,"ERROR: Interlacing format mismatch in stream %d file \"%s\".\n"
                      "       Sequential video streams must have matching interlacing\n"
                      "       formats.\n",
                      j,infilenames[i]);
              exit(1);
            }
            break;
        case STREAM_AUDIO:
          if(s->m.audio.rate != r->m.audio.rate){
            fprintf(stderr,"ERROR: Audio rate mismatch in stream %d file \"%s\"."
                    "       Sequential audio streams must have matching sampling rate\n"
                    "       and channels\n.",
                    j,infilenames[i]);
            exit(1);
          }
          if(s->m.audio.ch != r->m.audio.ch){
            fprintf(stderr,"ERROR: Audio rate mismatch in stream %d file \"%s\"."
                    "       Sequential audio streams must have matching sampling rate\n"
                    "       and channels\n.",
                    j,infilenames[i]);
            exit(1);
          }
          break;
        default:
          break;
        }
      }
    }

    /* ready output tracking if unallocated *****************************************************/
    if(!outclock){
      outclock = calloc(ty->num_streams,sizeof(*outclock));
      cutclock = calloc(ty->num_streams,sizeof(*cutclock));
      outoffsets = calloc(ty->num_streams,sizeof(*outoffsets));
      lastframe = calloc(ty->num_streams,sizeof(*lastframe));
    }

    /* prime input stream queues ****************************************************************/
    /* not necessary, but it allows reporting some timing information */
    for(j=0;j<ty->num_streams;j++){
      if(limited_prime(ty,j)){
        fprintf(stderr,"ERROR: Did not find start of stream %d within first %f seconds\n"
                "       of data.  Aborting.\n",j,sync_secs);
        exit(1);
      }else{
        if(verbose){
          int h=floor(ty->streams[j]->inq_tail->pts/60/60);
          int m=floor(ty->streams[j]->inq_tail->pts/60)-h*60;
          int s=floor(ty->streams[j]->inq_tail->pts)-h*60*60-m*60;
          int d=floor((ty->streams[j]->inq_tail->pts-floor(ty->streams[j]->inq_tail->pts))*100);
          fprintf(stderr,"Stream %d initial PTS %02d:%02d:%02d.%02d\n",
                  j,h,m,s,d);
        }
      }
    }

    /* initialize output if not already initialized *********************************************/
    if(!outheader){
      /* container header */
      fprintf(outfile,"YUV4OGG S%c\n",
              (!ty->synced && force_no_sync ? 'n' : 'y'));

      /* stream headers */
      for(j=0;j<ty->num_streams;j++){
        switch(ty->streams[j]->type){
        case STREAM_AUDIO:
          fprintf(outfile,"AUDIO R%d C%d\n",
                  ty->streams[j]->m.audio.rate,ty->streams[j]->m.audio.ch);
          break;
        case STREAM_VIDEO:
          fprintf(outfile,"VIDEO W%d H%d F%d:%d I%c A%d:%d C%s\n",
                  ty->streams[j]->m.video.w,
                  ty->streams[j]->m.video.h,
                  ty->streams[j]->m.video.fps_n,
                  ty->streams[j]->m.video.fps_d,
                  ty->streams[j]->m.video.i?(ty->streams[j]->m.video.i==TOP_FIRST?'t':'b'):'p',
                  ty->streams[j]->m.video.pa_n,
                  ty->streams[j]->m.video.pa_d,
                  chromaformat[ty->streams[j]->m.video.format]);
          break;
        default:
          break;
        }
      }
      outheader=1;
    }

    if(verbose) fprintf(stderr,"\n");

    if(!ty->synced && !force_no_sync){

      /* find clock start times of new substreams ***********************************************/
      {
        long long rec_outclock[ty->num_streams];
        for(j=0;j<ty->num_streams;j++){
          rec_outclock[j]=outclock[j];
          if(y)
            rec_outclock[j]+=y->streams[j]->tick_depth;
        }
        for(j=0;j<ty->num_streams;j++)
          search_offset(ty,sync_stream,j,outclock,outoffsets,lastframe);
      }

      /* trim holdover streams if needed given new offsets **************************************/
      if(y){
        stream_t *ss=y->streams[sync_stream];
        double sync_time = ss->tickduration*outclock[sync_stream];
        for(j=0;j<y->num_streams;j++){
          if(outoffsets[j]>0){
            /* we have too many frames; where to trim? */
            stream_t *ys = y->streams[j];
            double time = ys->tickduration*outclock[j];
            if(time>sync_time){
              /* trim holdover back, but no farther than sync_time */
              if(time-outoffsets[j]<sync_time)
                outoffsets[j]=time-sync_time;

              /* remove frames from head of holdover stream */
              while(outoffsets[j]>0)
                if(trim_from_head(y,j,outclock,outoffsets))break;
            }
          }
        }
      }

      /* sync loop *****************************************************************************/
      while(global_ended < ty->num_streams){
        double earliest=outclock[0]*ty->streams[0]->tickduration;
        int sno=0;

        /* look for lowest clock time of stream queues */
        for(j=1;j<ty->num_streams;j++){
          double time=outclock[j]*ty->streams[j]->tickduration;
          if(time<earliest){
            earliest=time;
            sno=j;
          }
        }

        /* we know which stream to pull first */
        if(y && y->streams[sno]->inq_tail){
          /* holdover stream; rules here are a little different.  We simply flush. */
          pull_write_frame(outfile,y,lastframe,outclock,cutclock,cutticks,sno);
        }else{
          stream_t *s=ty->streams[sno];
          if(outoffsets[sno]<0){
            /* there's an already-established gap in the stream.  dup it out */
            if(duplicate_frame(outfile,ty,lastframe,outclock,cutclock,cutticks,outoffsets,sno)){
              fprintf(stderr,"break\n");
              break;
            }
          }else if (outoffsets[sno]>0){
            trim_from_tail(ty,sno,outclock,outoffsets);
          }else{
            if(limited_prime(ty,sno)){
              fprintf(stderr,"break\n");
              break;
            }
            if(search_offset(ty,sync_stream,sno,outclock,outoffsets,lastframe)){
              fprintf(stderr,"break\n");
              break;
            }
            if(s->inq_tail->presync && outoffsets[sno]==0)
              pull_write_frame(outfile,ty,lastframe,outclock,cutclock,cutticks,sno);
          }
        }
        if(y && !y->f_tail){
          y4o_close_in(y);
          y=NULL;
        }
      }

      if(y && global_ended < ty->num_streams){
        /* got through loop without flushing all of y?  Alert the
           user, dump the frames */
        int flag=0;

        for(j=0;j<y->num_streams;j++){
          if(y->streams[j]->inq_tail){
            fprintf(stderr,"WARNING: Previous stream %d completely overlaps current stream?\n",j);
            flag=1;
          }
        }
        if(!flag)
          fprintf(stderr,"WARNING: Previous input stream queues emptied, but stream still open?\n");

        y4o_close_in(y);
      }

    }else{
      if(y) y4o_close_in(y);

      /* if we're not trying to sync, just write the frames in output clock order */
      while(global_ended < ty->num_streams){
        double earliest;
        int sno=-1;

        /* look for lowest clock time of stream queues */
        for(j=0;j<ty->num_streams;j++){
          stream_t *s = ty->streams[j];
          double time=outclock[j]*s->tickduration;
          limited_prime(ty,j);
          if(s->inq_tail && (sno==-1 || time<earliest)){
            earliest=time;
            sno=j;
          }
        }

        if(sno==-1)break;
        pull_write_frame(outfile,ty,lastframe,outclock,cutclock,cutticks,sno);
      }
    }

    y=ty;

  }

  if(y){
    if(verbose)
      fprintf(stderr,"Flushing end of stream\n\n");
    while(y->f_tail && global_ended < ty->num_streams){
      /* continue to obey clock order */
      double earliest=-1;
      int j,sno=-1;

      /* look for lowest clock time of remaning stream queues */
      for(j=0;j<ty->num_streams;j++){
        if(y->streams[j]->inq_tail){
          double time=outclock[j]*y->streams[j]->tickduration;
          if(sno==-1 || time<earliest){
            earliest=time;
            sno=j;
          }
        }
      }
      pull_write_frame(outfile,y,lastframe,outclock,cutclock,cutticks,sno);
    }
    if(verbose)
      fprintf(stderr,"Done processing all streams\n\n");

    if(cutticks){
      for(i=0;i<y->num_streams;i++)
        if(cutticks[i].cut)free(cutticks[i].cut);
      free(cutticks);
    }
    y4o_close_in(y);
  }else{
    usage(stderr);
  }
  if(outfile!=stdout)fclose(outfile);
  if(outclock)free(outclock);
  if(cutclock)free(cutclock);
  if(outoffsets)free(outoffsets);
  if(lastframe)free(lastframe);
  return 0;
}
