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

#include "y4oi.h"

typedef struct {
  char *filename;
  FILE *outfile;
  int wrote_header;
} internal;

static void parse_options(fq_t *queue){
  internal *i = queue->internal;
  char **key = queue->option_keys;
  char **val = queue->option_vals;
  while(key && *key){

    if(!strcmp(*key,"file")){
      if(!*val){
        yerror("No filename given for option 'file'\n");
        exit(1);
      }
      i->filename=strdup(*val);
      if(i->outfile)
        fclose(i->outfile);
      i->outfile=fopen(i->filename,"wb");
      if(!i->outfile){
        yerror("Unable to open '%s' for output; %s\n",i->filename,strerror(errno));
        exit(1);
      }
    }else{
      yerror("No such option '%s'\n",*key);
    }

    key++;
    val++;
  }

  if(!i->outfile){
    i->outfile=stdout;
    i->filename=strdup("stdout");
  }
}

/* the output filter needs to wait until it has seen parameter frames
   from each stream before it can write an output header */
static void startup_header(fq_t *queue){
  internal *i = queue->internal;

  if(!i->wrote_header){
    int j;
    for(j=0;j<queue->streams;j++)
      if(!queue->stream_depth[j])break;

    if(j==queue->streams){

      /* container header */
      fprintf(i->outfile,"YUV4OGG S%c\n", (queue->tail->synced ? 'n' : 'y'));

      /* stream headers */
      for(j=0;j<queue->streams;j++){
        /* find first stream of the given type */
        fq_frame_t *ptr = queue->tail;
        while(ptr && ptr->sno != j)
          ptr=ptr->next;

        if(!ptr){
          yerror("Output couldn't find stream number %d\n\n",j);
          exit(1);
        }

        switch(ptr->st){
        case STREAM_AUDIO:
          fprintf(i->outfile,"AUDIO R%d C%d\n",
                  ptr->sp.audio.rate,ptr->sp.audio.ch);
          break;
        case STREAM_VIDEO:
          fprintf(i->outfile,"VIDEO W%d H%d F%d:%d I%c A%d:%d C%s\n",
                  ptr->sp.video.w,
                  ptr->sp.video.h,
                  ptr->sp.video.fps_n,
                  ptr->sp.video.fps_d,
                  ptr->sp.video.i?(ptr->sp.video.i==TOP_FIRST?'t':'b'):'p',
                  ptr->sp.video.pa_n,
                  ptr->sp.video.pa_d,
                  chromaformat[ptr->sp.video.format]);
          break;
        default:
          yerror("Unknown stream type in output filter.\n");
          exit(1);
        }
      }
      i->wrote_header=1;
    }else
      return;
  }
}

void filter_output_process(fq_t *queue){
  internal *i = queue->internal;
  fq_frame_t *ptr = NULL;

  switch(queue->state){
  case QUEUE_INIT:
    /* called once during filter setup immediately after allocating
       queue. The queue is uninitialized; only filter options are
       filled in. */

    queue->internal = calloc(1,sizeof(*i));
    parse_options(queue);

    break;
  case QUEUE_STARTUP:
    /* called once for each stream as a way of passing through initial
       parameters of each stream.  Queue is fully initialized and
       holds only one or more empty frames bearing parameters. */
    if(!queue->tail) return;
    startup_header(queue);

    break;
  case QUEUE_PROCESS:
    /* Called for each submission of a new frame (of any stream type)
       during normal processing */
  case QUEUE_FLUSH:
    /* Same as QUEUE_PROCESS except that no further frames will be
       submitted; the filter is directed to flush any queued frames */
    if(!i->wrote_header){
      yerror("Output filter did not see all stream parameters before\n"
             "\tbeginning of processing.\n");
      exit(1);
    }
    if(!queue->tail) break;

    while((ptr=queue->tail)){
      if(ptr->body){
        if(fprintf(i->outfile,"FRAME S%d L%d P%.3f\n",ptr->sno,ptr->body_size,ptr->pts)<0 ||
           fwrite(ptr->body,1,ptr->body_size,i->outfile)<(unsigned)ptr->body_size){
          yerror("Unable to write to output; %s\n",strerror(errno));
          exit(1);
        }
      }

      /* It's possible something follows this output filter. Safe to call
         if the queue is empty (eg, during QUEUE_INIT)*/
      filter_forward_frame(queue);

    }

    if(queue->state==QUEUE_FLUSH){
      fclose(i->outfile);
      i->outfile=NULL;
      queue->state=QUEUE_FINISHED;
    }
    break;

  case QUEUE_FINISHED:
    break;
  }

}
