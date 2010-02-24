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

#include "y4oi.h"

extern void filter_output_process(fq_t *);

static filter_t filter_list[] = {
  {
    "output",
    "y4o stream output filter",
    filter_output_process,
    1
  },
  {
    NULL,
    NULL,
    NULL,
    0
  }
};

static fq_t *filter_stack = NULL;
static fq_t *filter_head = NULL;

static void free_frame(fq_frame_t *frame){
  if(frame){
    if(frame->body)free(frame->body);
    free(frame);
  }
}

static void push_frame(fq_t *queue, fq_frame_t *frame){
  if(frame){

    /* Turn implicit state into explicit state */
    switch(queue->state){
    case QUEUE_INIT:
      queue->state=QUEUE_STARTUP;
      break;
    case QUEUE_STARTUP:
      if(frame->body)
        queue->state=QUEUE_PROCESS;
      break;
    case QUEUE_PROCESS:
    case QUEUE_FLUSH:
    case QUEUE_FINISHED:
      break;
    }

    /* append frame */
    if(queue->head == NULL){
      queue->head = queue->tail = frame;
    }else{
      queue->head->next = frame;
      frame->prev = queue->head;
      queue->head = frame;
    }

    /* there may be some lazy init to handle */
    if(!queue->stream_depth){
      queue->streams=frame->sn;
      queue->stream_depth = calloc(frame->sn,sizeof(*queue->stream_depth));
    }
    if(queue->streams != frame->sn || frame->sno<0 || frame->sno>=queue->streams){
      yerror("number of media streams changed; aborting.\n");
      exit(1);
    }

    queue->stream_depth[frame->sno]++;
  }
}

static fq_frame_t *pull_frame(fq_t *fq){
  fq_frame_t *frame = fq->tail;
  if(frame){
    fq->tail = frame->next;
    if(frame->next)
      frame->next->prev = NULL;
    else
      fq->head = NULL;
    frame->next=frame->prev=NULL;
    fq->stream_depth[frame->sno]--;
  }
  return frame;
}

void filter_forward_frame(fq_t *fq){
  if(fq->tail){
    fq_frame_t *frame = pull_frame(fq);
    if(fq->next){
      push_frame(fq->next,frame);
      fq->next->filter->process(fq->next);
    }else{
      if(frame)
        free_frame(frame);
    }
  }
}

void filter_drop_frame(fq_t *fq){
  if(fq->tail){
    fq_frame_t *frame = pull_frame(fq);
    if(frame)
      free_frame(frame);
  }
}

void filter_submit_frame(stream_t *s, unsigned char *b, int len, double pts){
  if(filter_stack){
    unsigned char *body=NULL;
    fq_frame_t *frame=calloc(1,sizeof(*frame));
    y4o_in_t *y = s->y;

    if(len)
      body = malloc(len);
    memcpy(body,b,len);

    frame->body = body;
    frame->body_size=len;
    frame->sn = y->num_streams;
    frame->sno = s->stream_num;
    frame->st = y->streams[frame->sno]->type;
    frame->sp = y->streams[frame->sno]->m;
    frame->pts = pts;
    frame->bos = s->bos;
    frame->synced = (y->synced || !force_no_sync);
    s->bos = 0;

    push_frame(filter_stack,frame);
    filter_stack->filter->process(filter_stack);
  }
}

void filter_flush(){
  fq_t *fq = filter_stack;
  while(fq){
    if(fq->state != QUEUE_FLUSH && fq->state != QUEUE_FINISHED){
      fq->state = QUEUE_FLUSH;
      fq->filter->process(fq);
    }
    fq=fq->next;
  }
}

static char *pretrim(char *in){
  while(in && *in && *in<32) in++;
  return in;
}

static void posttrim(char *in){
  char *last=in;
  while(in && *in){
    if(*in>=32) last=in;
    in++;
  }
  if(last && *last && *last>=32)
    last[1]=0;
}

#undef NAME
#define NAME "y4oi"

int filter_append(char *cmd){
  /* "filtername:option=value:option=value,filtername..." */
  int err = 0;
  char *c = cmd?strdup(cmd):NULL;
  char *fptr = pretrim(c);

  while(fptr && *fptr){
    filter_t *f = filter_list;
    char *eptr;
    char *optr=NULL;
    char *nptr;

    /* pick off the filter name */
    eptr = strchr(fptr,':');
    if(eptr){
      optr=eptr+1;
      eptr[0]=0;
    }
    posttrim(fptr);
    pretrim(optr);

    /* get pointer for next filter */
    nptr = strchr(fptr,',');
    if(nptr){
      nptr[0]=0;
      nptr++;
    }

    /* search for the filter name */
    while(f->name){
      if(!strcmp(f->name,fptr)){
        /* found the filter */
        /* parse option key/val pairs */
        /* count the colons */
        int count=0;
        char *p=optr;
        char **key;
        char **val;
        while(p && *p){
          count++;
          p=strchr(p,':');
          if(p)p++;
        }

        /* alloc and fill */
        key = calloc(count+1,sizeof(*key));
        val = calloc(count+1,sizeof(*val));
        p=optr;
        count=0;
        while(p && *p){
          char *n=strchr(p,':');
          char *v;
          if(n){
            n[0]=0;
            n++;
          }

          v=strchr(p,'=');
          if(v){
            v[0]=0;
            v++;
          }

          posttrim(p);
          posttrim(v);

          if(*p){
            key[count]=strdup(p);
            if(v && *v)
              val[count]=strdup(v);
            else
              val[count]=strdup("");
          }else{
            yerror("error parsing '%s' filter options\n",f->name);
            err=1;
          }
          count++;
          p=pretrim(n);
        }

        /* construct/append the filter */
        {
          fq_t *fq = calloc(1,sizeof(*fq));
          fq->filter = f;
          fq->option_keys=key;
          fq->option_vals=val;
          fq->state = QUEUE_INIT;

          if(filter_stack){
            filter_head->next = fq;
          }else{
            filter_head = filter_stack = fq;
          }

          /* call the filter with no frames or preset to give it an option to parse/check options */
          fq->filter->process(fq);
        }

        break;
      }
      f++;
    }

    if(!f->name){
      yerror("unknown filter '%s'\n\n",fptr);
      err=1;
    }

    fptr=pretrim(nptr);
  }
  if(c)free(c);
  return err;
}

int filter_is_last_output(void){
  if(!filter_head) return 0;
  return  filter_head->filter->is_type_output_p;
}
