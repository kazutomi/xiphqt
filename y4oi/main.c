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

#define NAME "sync"
#include "y4oi.h"

int verbose=0;
static double fill_secs=25.;
static double sync_secs=3.;
ratio force_fps={0,0};
static int force_sync=0;
int force_no_sync=0;
int force_non_interlaced=0;
char *lastprint="";
static int no_rewrite=0;
static int global_ended=0;

const char *optstring = "b:c:e:f:F:ho:sSvqNi";
struct option options [] = {
  {"begin",required_argument,NULL,'b'},
  {"cut",required_argument,NULL,'c'},
  {"end",required_argument,NULL,'e'},
  {"force-fps",required_argument,NULL,'f'},
  {"filter",required_argument,NULL,'F'},
  {"force-non-interlaced",no_argument,NULL,'i'},
  {"help",no_argument,NULL,'h'},
  {"no-rewrite",no_argument,NULL,'N'},
  {"output",required_argument,NULL,'o'},
  {"force-sync",no_argument,NULL,'s'},
  {"force-no-sync",no_argument,NULL,'S'},
  {"quiet",no_argument,NULL,'q'},
  {"verbose",required_argument,NULL,'v'},

  {NULL,0,NULL,0}
};

void usage(FILE *out){
  fprintf(out,
          "\ny4oi 20100222\n"
          "performs basic operations on yuv4ogg interchange streams in prep for\n"
          "theora encoding\n\n"

          "USAGE:\n"
          "  y4oi [options] instream [instream...]\n\n"

          "OPTIONS:\n"
          "  -b --begin HH:MM:SS.XX    : discard starting data up to specified\n"
          "                              begin time.  Time is measured relative to\n"
          "                              output clock\n"
          "  -c --cut BEGIN-END        : drop output data in the specified interval.\n"
          "                              Time is measured relative to output clock,\n"
          "                              but output timestamps will be re-written\n"
          "                              such that there is no gap.\n"
          "  -e --end HH:MM:SS.XX      : stop working at specified end time.  Time\n"
          "                              is measured relative to output clock\n"
          "  -f --force-fps N:D        : Override declared input fps\n"
          "  -F --filter filterchain   : Apply one or more filters to stream; the\n"
          "                              filter chain syntax is of the form:\n"
          "                              filter1:option=value:option=value,filter2...\n"
          "  -h --help                 : print this usage message to stdout and exit\n"
          "                              with status zero\n"
          "  -i --force-non-interlaced : Force input to be treated as noninterlaced\n"
          "                              regardless of stream flag\n"
          "  -N --no-rewrite           : do not rewrite output PTS values.  Retain\n"
          "                              original PTS values even if input streams\n"
          "                              were synchronized.\n"
          "  -o --output FILE          : output file/pipe (stdout default)\n"
          "  -q --quiet                : completely silent operation\n"
          "  -s --force-sync           : perform autosync even on streams marked as\n"
          "                              already synchronized\n"
          "  -S --force-no-sync        : do not perform stream sync even on unsynced\n"
          "                              streams\n"
          "  -v --verbose              : increase verbosity level by one\n"
          "\n"
          "VIDEO FILTERS/OPTIONS:\n"
          "\n"
          "OUTPUT FILTERS/OPTIONS:\n"
          "  output                    : output y4o stream to a file or pipe.\n"
          "               file=<value> . destination file; default is stdout\n\n"
          );
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

/* depth-limited fill: bounded attempt to prime any given queue at the
 point any other queue reaches fill_secs deep actual depth-of-queue is
 used, not PTS (as PTS is unreliable and could be way off into the
 weeds) */

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
      yerror("Buffer depth exceeded configured limit due to A/V skew; aborting input stream.\n");
      return 1;
    }
  }
  return 0;
}

/* bounded sync search: fill sync/search streams a min of search_secs
 deep past prime point each, filling no queue more than fill_secs past
 prime point depth-of-queue is used, not PTS (as PTS is unreliable and
 could be way off ino the weeds) */

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
          best_frame=s->inq_tail;
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

/* a frame_pop/free that doesn't lock swap data into memory */
static int discard_head_frame(y4o_in_t *y,stream_t *s, long long *outclock){
  frame_t *p=s->inq_head;
  if(!p) return 1;

  if(s->type==STREAM_VIDEO){
    yverbose("Stream %d [%s]: Dropping video frame to maintain sync\n",
             p->streamno,
             make_time_string((outclock[p->streamno]+s->tick_depth)*s->tickduration));
  }else{
    yverbose("Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
             p->streamno,
             make_time_string((outclock[p->streamno]+s->tick_depth)*s->tickduration),
             p->duration);
  }

  y4o_free_frame(p);

  return 0;
}

/* a frame_pull/free that doesn't lock swap data into memory */
static int discard_tail_frame(y4o_in_t *y,stream_t *s, long long *outclock){
  frame_t *p=s->inq_tail;
  if(!p) return 1;

  if(s->type==STREAM_VIDEO){
    yverbose("Stream %d [%s]: Dropping video frame to maintain sync\n",
             p->streamno,
             make_time_string(outclock[p->streamno]*s->tickduration));
  }else{
    yverbose("Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
             p->streamno,
             make_time_string(outclock[p->streamno]*s->tickduration),
             p->duration);
  }

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

        yverbose("Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
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

        yverbose("Stream %d [%s]: Dropping %gs of audio to maintain sync\n",
                 p->streamno,make_time_string(outclock[sno]*s->tickduration),
                 outoffsets[sno]);

        /* load frame data into memory and release swap */
        y4o_lock_frame(p);
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

      if(e>=0){
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
      }

      ret[j].cut[i].begin = bt;
      ret[j].cut[i].end = et;
    }
    ret[j].cuts=n;

  }

  return ret;
}

/* Cut functionality injected here; thus the seperate out and cut clocks */
static void write_frame(y4o_in_t *y,
                        long long *outclock,
                        long long *cutclock, cutlist *cut,
                        frame_t *p){
  int sno = p->streamno;
  stream_t *s=y->streams[sno];
  double outpts = (no_rewrite?p->pts:cutclock[sno]*s->tickduration);
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

        filter_submit_frame(s, p->data, beginbytes, outpts);
        cutclock[sno]+=beginticks;
        outclock[sno]+=beginticks;

        /* mutate packet */
        p->data+=beginbytes;
        p->len-=beginbytes;
        p->ticks-=beginticks;

        /* recurse */
        write_frame(y, outclock, cutclock, cut, p);

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
        write_frame(y, outclock, cutclock, cut, p);

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
    filter_submit_frame(s, p->data, p->len, outpts);
    cutclock[sno]+=p->ticks;
    outclock[sno]+=p->ticks;
  }
}

static void pull_write_frame(y4o_in_t *y,frame_t **lastframe,
                             long long *outclock,long long *cutclock, cutlist *cutticks,
                             int sno){
  stream_t *s = y->streams[sno];
  frame_t *p = s->inq_tail;

  if(!p) return;

  y4o_pull_frame(p);
  write_frame(y,outclock,cutclock,cutticks,p);
  if(lastframe[sno])
    y4o_free_frame(lastframe[sno]);
  lastframe[sno]=p;

}

/* conditionally duplicates last frame in video stream, or generates a frame of audio silence */
static int duplicate_frame(y4o_in_t *y,frame_t **lastframe, long long *outclock,
                           long long *cutclock, cutlist *cutticks,
                           double *outoffsets, int sno){
  stream_t *s = y->streams[sno];
  frame_t *p = lastframe[sno];

  if(!p){
    /* no preceeding frame.  Look to next frame instead */
    if(limited_prime(y,sno)) return 1;
    p=s->inq_tail;
    y4o_lock_frame(p);
  }

  switch(s->type){
  case STREAM_VIDEO:
    /* we only dup the frame if doing so gets us closer to the ideal clock sync */
    if(fabs(outoffsets[sno]+p->duration)<fabs(outoffsets[sno])){
      /* dup it */
      yverbose("Stream %d [%s]: Repeating video frame to maintain sync\n",
               p->streamno,make_time_string(outclock[sno]*s->tickduration));

      write_frame(y,outclock,cutclock,cutticks,p);
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

      yverbose("Stream %d [%s]: Adding %gs of silence to maintain sync\n",
               p->streamno,make_time_string(outclock[sno]*s->tickduration),-outoffsets[sno]);

      lp.data=data;
      lp.len=bytes;
      lp.streamno=sno;
      lp.pts=(p?p->pts:0.);
      lp.ticks=samples;
      write_frame(y,outclock,cutclock,cutticks,&lp);
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

static int parse_time(char *s,double *t){
  double      secf;
  long        secl;
  const char *pos;
  char       *end;
  int         err;
  err=0;
  secl=0;
  pos=strchr(optarg,':');
  if(pos!=NULL){
    char *pos2;
    secl=strtol(optarg,&end,10)*60;
    err|=pos!=end;
    pos2=strchr(++pos,':');
    if(pos2!=NULL){
      secl=(secl+strtol(pos,&end,10))*60;
      err|=pos2!=end;
      pos=pos2+1;
    }
  }
  else pos=optarg;
  secf=strtod(pos,&end);
  if(err||*end!='\0')return -1;

  *t = secl+secf;
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

#undef NAME
#define NAME NULL

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

  int filterprime=0;

  FILE **infile = NULL;
  char *const*infilenames=NULL;
  int infiles=0,i;

  char *outfile=NULL;

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
      verbose++;
      break;
    case 'q':
      verbose=-1;
      break;
    case 'b':
      parse_time(optarg,&begin);
      break;
    case 'e':
      parse_time(optarg,&end);
      break;
    case 'f':
      if(parse_ratio(optarg,&force_fps)){
        yerror("cannot parse fps argument '%s'\n",optarg);
        exit(1);
      }
      break;
    case 'F':
      if(filter_append(optarg)){
        exit(1);
      }
      break;
    case 'c':
      if(!ccut)
        ccut=calloc(1,sizeof(*ccut));
      else
        ccut=realloc(ccut,(ccuts+1)*sizeof(*ccut));
      if(parse_interval(optarg,ccut+ccuts)){
        yerror("cannot parse cut interval '%s'\n",optarg);
        exit(1);
      }
      ccuts++;
      break;
    case 's':
      force_sync=1;
      force_no_sync=0;
      break;
    case 'i':
      force_non_interlaced=1;
      break;
    case 'S':
      force_sync=0;
      force_no_sync=1;
      break;
    case 'N':
      no_rewrite=1;
      break;
    case 'o':
      if(outfile)
        free(outfile);
      outfile=strdup(optarg);
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
        yerror("Unable to open '%s' for input; %s\n",argv[optind+i],strerror(errno));
        exit(1);
      }
    }
    optind++;
  }

  /* is the end of the filter stack explicitly an output filter? */
  if(!filter_is_last_output()){
    char *buffer=NULL;
    /* add an implicit output filter */
    if(outfile){
      asprintf(&buffer,"output:file=%s",outfile);
    }else{
      asprintf(&buffer,"output");
    }

    if(filter_append(buffer)){
      exit(1);
    }
    free(buffer);
  }

  for(i=0;i<infiles;i++){
    int j;

    yinfo("Begin processing input stream \"%s\"...\n",infilenames[i]);

#undef NAME
#define NAME "input"

    ty=y4o_open_in(infile[i]);

    if(y==NULL){

      for(j=0;j<ty->num_streams;j++){
        stream_t *s = ty->streams[j];
        switch(s->type){
        case STREAM_VIDEO:
          if(primary_video<0)primary_video=j;
          yverbose("Using stream %d as primary video stream\n",j);
          break;
        case STREAM_AUDIO:
          if(primary_audio<0)primary_audio=j;
          yverbose("Using stream %d as primary audio stream\n",j);
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
          yinfo("Forcing stream %d file \"%s\" to %.3ffps.\n",
                  j,infilenames[i],force_fps.num/(double)force_fps.den);
        }
        if(ty->streams[j]->m.video.format == C420unknown){
          ywarn("Assuming mpeg2 chroma positioning for stream %d.\n",
                  j);
          ty->streams[j]->m.video.format = C420mpeg2;
        }
        if(ty->streams[j]->m.video.format == C422unknown){
          ywarn("Assuming smpte chroma positioning for stream %d.\n",
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
        yerror("Number of streams mismatch in file \"%s\".\n"
               "\tSequential input streams must have matching stream\n"
               "\tnumbers and types.\n",
               infilenames[i]);
        exit(1);
      }
      for(j=0;j<y->num_streams;j++){
        stream_t *r = y->streams[j];
        stream_t *s = ty->streams[j];

        if(s->type != r->type){
          yerror("Stream %d type mismatch in file \"%s\".\n"
                 "\tSequential input streams must have matching stream\n"
                 "\tnumbers and types.\n",
                  j,infilenames[i]);
          exit(1);
        }

        /* eventually, we should be able to autoconvert and force all
           these settings */
        switch(s->type){
          case STREAM_VIDEO:
            if(s->m.video.w != r->m.video.w ||
               s->m.video.h != r->m.video.h){
              yerror("Video dimensions mismatch in stream %d file \"%s\".\n"
                      "\tSequential video streams must have matching dimensions.\n",
                      j,infilenames[i]);
              exit(1);
            }
            if(abs(s->m.video.pa_n/(double)s->m.video.pa_d -
                   r->m.video.pa_n/(double)r->m.video.pa_d)>EPSILON){
              ywarn("Pixel aspect mismatch in stream %d file \"%s\".\n"
                    "\tForcing aspect to match original stream.\n",
                    j,infilenames[i]);
              s->m.video.pa_n = r->m.video.pa_n;
              s->m.video.pa_d = r->m.video.pa_d;
            }
            if(s->m.video.format != r->m.video.format){
              yerror("Chroma format mismatch in stream %d file \"%s\".\n"
                     "\tSequential video streams must have matching chroma\n"
                     "\tformats.\n",
                     j,infilenames[i]);
              exit(1);
            }
            if(s->m.video.i != r->m.video.i){
              yerror("Interlacing format mismatch in stream %d file \"%s\".\n"
                     "\tSequential video streams must have matching interlacing\n"
                     "\tformats.\n",
                     j,infilenames[i]);
              exit(1);
            }
            break;
        case STREAM_AUDIO:
          if(s->m.audio.rate != r->m.audio.rate){
            yerror("Audio rate mismatch in stream %d file \"%s\"."
                   "\tSequential audio streams must have matching sampling rate\n"
                   "\tand channels\n.",
                   j,infilenames[i]);
            exit(1);
          }
          if(s->m.audio.ch != r->m.audio.ch){
            yerror("Audio rate mismatch in stream %d file \"%s\"."
                   "\tSequential audio streams must have matching sampling rate\n"
                   "\tand channels\n.",
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

#undef NAME
#define NAME "sync"

    /* prime input stream queues ****************************************************************/
    /* not necessary, but it allows reporting some timing information */
    for(j=0;j<ty->num_streams;j++){
      if(limited_prime(ty,j)){
        yerror("Did not find start of stream %d within first %f seconds\n"
               "\tof data.  Aborting.\n",j,sync_secs);
        exit(1);
      }else{
        if(verbose){
          int h=floor(ty->streams[j]->inq_tail->pts/60/60);
          int m=floor(ty->streams[j]->inq_tail->pts/60)-h*60;
          int s=floor(ty->streams[j]->inq_tail->pts)-h*60*60-m*60;
          int d=floor((ty->streams[j]->inq_tail->pts-floor(ty->streams[j]->inq_tail->pts))*100);
          yverbose("Stream %d initial PTS %02d:%02d:%02d.%02d\n",
                j,h,m,s,d);
        }
      }
    }

    /* prime the filter chain with empty frames that carry stream
       params; this makes sure we write an output header in timely fashion  *********************/
    if(!filterprime){
      for(j=0;j<ty->num_streams;j++)
        filter_submit_frame(ty->streams[j], NULL, 0, -1);
      filterprime=1;
    }

    yverbose("\n");

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
          pull_write_frame(y,lastframe,outclock,cutclock,cutticks,sno);
        }else{
          stream_t *s=ty->streams[sno];
          if(limited_prime(ty,sno)){
            break;
          }
          if(outoffsets[sno]<0){
            /* there's an already-established gap in the stream.  dup it out */
            if(duplicate_frame(ty,lastframe,outclock,cutclock,cutticks,outoffsets,sno)){
              break;
            }
          }else if (outoffsets[sno]>0){
            trim_from_tail(ty,sno,outclock,outoffsets);
          }else{
            if(search_offset(ty,sync_stream,sno,outclock,outoffsets,lastframe)){
              break;
            }
            if(s->inq_tail->presync && outoffsets[sno]==0)
              pull_write_frame(ty,lastframe,outclock,cutclock,cutticks,sno);
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
            ywarn("Previous stream %d completely overlaps current stream?\n",j);
            flag=1;
          }
        }
        if(!flag)
          ywarn("Previous input stream queues emptied, but stream still open?\n");

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
        pull_write_frame(ty,lastframe,outclock,cutclock,cutticks,sno);
      }
    }

    y=ty;

  }

#undef NAME
#define NAME NULL

  if(y){
    yinfo("Flushing end of stream...\n");
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
      pull_write_frame(y,lastframe,outclock,cutclock,cutticks,sno);
    }
    filter_flush();
    yinfo("Done processing all streams\n");

    if(cutticks){
      for(i=0;i<y->num_streams;i++)
        if(cutticks[i].cut)free(cutticks[i].cut);
      free(cutticks);
    }
    y4o_close_in(y);
  }else{
    usage(stderr);
  }
  if(outclock)free(outclock);
  if(cutclock)free(cutclock);
  if(outoffsets)free(outoffsets);
  if(lastframe)free(lastframe);
  return 0;
}
