/*
 *
 *  y4oi:
 *     A utility for doing several simple but essential operations
 *     on yuv4ogg interchange streams.
 *
 *     y4oi copyright (C) 2010 Monty <monty@xiph.org>
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

#define NAME "input"
#include "y4oi.h"

char *chromaformat[]={
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

unsigned char chromabits[]={
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

char *chromaformat_long[]={
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

static int read_container_header(FILE *f, int *sync){
  char line[80];
  char *p;
  int n;
  size_t ret,len;

  len=sizeof("YUV4OGG");
  ret = fread(line, 1, len,f);
  if (ret<len){
    if(feof(f))
      yerror("EOF reading y4o file header\n");
    return 1;
  }

  if (strncmp(line, "YUV4OGG ", sizeof("YUV4OGG ")-1)){
    yerror("cannot parse y4o file header\n");
    return 1;
  }

  /* proceed to get the tags... (overwrite the magic) */
  for (n = 0, p = line; n < 80; n++, p++) {
    if ((ret=fread(p,1,1,f))<1){
      if(feof(f))
        yerror("EOF reading y4o file header\n");
      return 1;
    }
    if (*p == '\n') {
      *p = '\0';
      break;
    }
  }
  if (n >= 80) {
    yerror("line too long reading y4o file header\n");
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
          yerror("unknown sync tag setting in y4o header\n");
          return 1;
        }
        break;
      default:
        yerror("unknown file tag in y4o header\n");
        return 1;
      }
    }
  }

  if(*sync==-1)return 1;

  return 0;
}

static stream_t *read_stream_header(FILE *f){
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
      yerror("EOF reading y4o stream header\n");
    goto err;
  }

  if (!strncmp(line, "VIDEO ", sizeof("VIDEO ")-1)) vf=1;
  else if (!strncmp(line, "AUDIO ", sizeof("AUDIO ")-1)) af=1;

  if(!(af||vf)){
    yerror("unknown y4o stream header type\n");
    goto err;
  }

  if(vf){
    /* video stream! */
    /* proceed to get the tags... (overwrite the magic) */
    for (n = 0, p = line; n < 80; n++, p++) {
      if ((ret=fread(p, 1, 1, f))<1){
        if(feof(f))
          yerror("EOF reading y4o video stream header\n");
        goto err;
      }
      if (*p == '\n') {
        *p = '\0';
        break;
      }
    }
    if (n >= 80) {
      yerror("line too long reading y4o video stream header\n");
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
            yerror("unknown y4o video interlace setting\n");
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
            yerror("unknown y4o video chroma format\n");
            goto err;
          }
          s->m.video.format=i;
          break;
        default:
          yerror("unknown y4o video stream tag\n");
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
      yerror("missing flags in y4o video stream header\n");
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
          yerror("EOF reading y4o audio stream header\n");
        goto err;
      }
      if (*p == '\n') {
        *p = '\0';
        break;
      }
    }
    if (n >= 80) {
      yerror("line too long reading y4o audio stream header\n");
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
          yerror("unknown y4o audio stream tag\n");
          goto err;
        }
      }
    }

    if(s->m.audio.rate>0 &&
       s->m.audio.ch>0)
      s->type=STREAM_AUDIO;
    else{
      yerror("missing flags in y4o stream header\n");
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

y4o_in_t *y4o_open_in(FILE *f){
  // id the file before anything else
  int i;
  y4o_in_t *y;
  stream_t *ll=NULL;

  if(read_container_header(f,&i))
    return NULL;

  y=calloc(1,sizeof(*y));
  y->synced=i;

  // seekable?
  if(!fseeko(f,0,SEEK_CUR))
    y->seekable=1;

  // read stream headers
  while(1){
    stream_t *s=read_stream_header(f);
    if(!s)
      break;
    if(s->type == STREAM_INVALID)
      yerror("Stream #%d unreadable; trying to continue\n",y->num_streams);
    s->stream_num=y->num_streams++;
    s->prev=ll;
    s->y=y;
    s->bos=1;

    if(!y->seekable){
      s->inswap.f[0]=tmpfile();
      s->inswap.f[1]=tmpfile();
    }

    if(verbose)
      switch(s->type){
      case STREAM_VIDEO:
        yinfo("Stream #%d, VIDEO %dx%d %d:%d %s %.3ffps %s\n",
              s->stream_num, s->m.video.w,s->m.video.h, s->m.video.frame_n,s->m.video.frame_d,
              chromaformat_long[s->m.video.format],s->m.video.fps_n/(double)s->m.video.fps_d,
              s->m.video.i?"interlaced":"progressive");
        if(s->m.video.i && force_non_interlaced)
          yinfo("FORCING NON-INTERLACED\n");
        break;
      case STREAM_AUDIO:
        yinfo("Stream #%d, AUDIO %dHz, %d channel(s) [24bit]\n",
              s->stream_num, s->m.audio.rate,s->m.audio.ch);
      default:
        break;
      }

    if(s->type==STREAM_VIDEO && force_non_interlaced) s->m.video.i=PROGRESSIVE;
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

void y4o_close_in(y4o_in_t *y){
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
frame_t *y4o_read_frame(y4o_in_t *y){
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
      yerror("EOF reading y4o frame\n");
    y->eof=1;
    return NULL;
  }

  if (strncmp(line, "FRAME ", sizeof("FRAME ")-1)){
    yerror("loss of y4o framing\n");
    return NULL;
  }

  /* proceed to get the tags... (overwrite the magic) */
  for (n = 0, p = line; n < 80; n++, p++) {
    if ((ret=fread(p, 1, 1, f))<1){
      if(feof(f))
        yerror("EOF reading y4o frame\n");
      return NULL;
    }
    if (*p == '\n') {
      *p = '\0';           /* Replace linefeed by end of string */
      break;
    }
  }
  if (n >= 80) {
    yerror("line too long reading y4o frame header\n");
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
        yerror("unknown y4o frame tag\n");
        return NULL;
      }
    }
  }

  if(streamno>=y->num_streams){
    yerror("error reading frame; streamno out of range\n");
    return NULL;
  }

  if(streamno==-1 || length==-1 || pts==-1){
    yerror("missing y4o frame tags; frame unreadable\n");
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
        yerror("unable to allocate memory for frame\n");
        return pret;
      }
      p->pts=thispts;
      p->len=bytes;
      p->s=s;

      if(s->inq_tail && y->seekable){
        /* there's already queued data and this stream is seekable. Save positioning
           and seek past */
        p->swap = f;
        p->swap_pos = ftello(f);
        if(fseeko(f,bytes,SEEK_CUR)){
          yerror("unable to advance in frame data; %s\n",strerror(errno));
          return pret;
        }
      }else{
        unsigned char *data=malloc(bytes);
        if(!data){
          yerror("unable to allocate memory for frame\n");
          free(p);
          return pret;
        }

        ret=fread(data,1,bytes,f);
        if(ret<(unsigned)bytes){
          if(feof(f)){
            yerror("unable to read frame; EOF\n");
          }else{
            yerror("unable to read frame; %s\n",strerror(errno));
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
          if(fwrite(data,1,bytes,p->swap)<(unsigned)bytes){
            yerror("unable to write to swap; %s\n",strerror(errno));
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

static void release_frame_swap(frame_t *p){
  if(p->swap){
    int from = (p->swap==p->s->inswap.f[0]?0:(p->swap==p->s->inswap.f[1]?1:-1));
    /* 'swap' might have been the seekable input file. */
    if(from>=0){
      p->s->inswap.tail[from]=p->swap_pos+p->len;

      /* if this is currently the write queue, swap banks */
      if(p->s->inswap.write==from){
        int b=!from;
        p->s->inswap.write=b;
        p->s->inswap.tail[b]=p->s->inswap.head[b]=0;
        if(fseeko(p->s->inswap.f[b],0,SEEK_SET)){
          yerror("unable to seek in swap file; %s\n",strerror(errno));
          exit(1);
        }
      }
    }
  }

  p->swap=NULL;
  p->swap_pos=0;
}

int y4o_lock_frame(frame_t *p){
  if(p->swap && p->s && !p->data){
    off_t savepos;
    y4o_in_t *y = p->s->y;
    /* fetch data from swap / seekable stream */
    size_t sl=p->len;
    unsigned char *d=malloc(sl);
    if(!d){
      yerror("unable to allocate memory for frame\n");
      return 1;
    }

    if(p->swap == y->f)
      savepos=ftello(y->f);

    if(fseeko(p->swap,p->swap_pos,SEEK_SET)){
      yerror("unable to seek in swap file; %s\n",strerror(errno));
      free(d);
      return 1;
    }
    if(fread(d,1,sl,p->swap)<sl){
      if(d)free(d);
      if(feof(p->swap)){
        yerror("unable to read frame from swap; EOF\n");
      }else{
        yerror("unable to read frame from swap; %s\n",strerror(errno));
      }
      free(d);
      return 1;
    }

    if(p->swap == y->f)
      if(fseeko(p->swap,savepos,SEEK_SET)){
        yerror("unable to seek in input file; %s\n",strerror(errno));
        exit(1);
      }

    p->data=d;
  }
  release_frame_swap(p);
  return 0;
}

void y4o_pull_frame(frame_t *p){
  /* pull out of the stream queue */
  stream_t *s=p->s;
  y4o_lock_frame(p);
  if(s){
    y4o_in_t *y=s->y;
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
    p->s=NULL;
  }
}

void y4o_free_frame(frame_t *p){
  release_frame_swap(p);
  y4o_pull_frame(p);
  if(p->data)free(p->data);
  memset(p,0,sizeof(*p));
  free(p);
}

