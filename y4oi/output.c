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
  char *wavfilename;
  char *yuvfilename;
  FILE *outfile;
  FILE *wavfile;
  int wavno;
  FILE *yuvfile;
  int yuvno;
  int wrote_header;

  size_t wavebytes;
  int wavch;
  int wavrate;
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
    }else if(!strcmp(*key,"wav")){
      if(!*val){
        yerror("No filename given for option 'wav'\n");
        exit(1);
      }
      i->wavfilename=strdup(*val);
      if(i->wavfile)
        fclose(i->wavfile);
      i->wavfile=fopen(i->wavfilename,"wb");
      if(!i->wavfile){
        yerror("Unable to open '%s' for output; %s\n",i->wavfilename,strerror(errno));
        exit(1);
      }
    }else if(!strcmp(*key,"yuv")){
      if(!*val){
        yerror("No filename given for option 'yuv'\n");
        exit(1);
      }
      i->yuvfilename=strdup(*val);
      if(i->yuvfile)
        fclose(i->yuvfile);
      i->yuvfile=fopen(i->yuvfilename,"wb");
      if(!i->yuvfile){
        yerror("Unable to open '%s' for output; %s\n",i->yuvfilename,strerror(errno));
        exit(1);
      }
    }else{
      yerror("No such option '%s'\n",*key);
    }

    key++;
    val++;
  }

  if(!i->outfile && !i->wavfile && !i->yuvfile){
    i->outfile=stdout;
    i->filename=strdup("stdout");
  }
}

static void PutNumLE(long num,FILE *f,int bytes){
  int i=0;
  while(bytes--){
    fputc((num>>(i<<3))&0xff,f);
    i++;
  }
}

static void waveheader(internal *i, size_t size){
  if(size>0x7fffffff)size=0x7fffffff;
  fprintf(i->wavfile,"RIFF");
  PutNumLE(size+44-8,i->wavfile,4);
  fprintf(i->wavfile,"WAVEfmt ");
  PutNumLE(16,i->wavfile,4);
  PutNumLE(1,i->wavfile,2);
  PutNumLE(i->wavch,i->wavfile,2);
  PutNumLE(i->wavrate,i->wavfile,4);
  PutNumLE(i->wavrate*i->wavch*2,i->wavfile,4); /* always output 16 bit */
  PutNumLE(i->wavch*2,i->wavfile,2); /* always output 16 bit */
  PutNumLE(16,i->wavfile,2);
  fprintf(i->wavfile,"data");
  PutNumLE(size,i->wavfile,4);
}

/* equivalent pixel format strings in YUV4MPEG2 */
char *translateformat1[]={
  NULL,
  NULL,
  "420jpeg",   //2 chroma sample is centered vertically and horizontally between luma samples
  "420mpeg2",       //3 chroma sample is centered vertically between lines, cosited horizontally */
  "420paldv",  //4 chroma sample is cosited vertically and horizontally */
  NULL,
  NULL,
  "422",       //7 chroma sample is cosited horizontally */
  NULL,
  "444",       //9
  NULL
};

char *translateformat2[]={
  NULL,        // no fallback
  NULL,        // no fallback
  "420jpeg",
  "420mpeg2",
  "420paldv",
  "420mpeg2",       // Use mpeg 420 as fallback
  "422",
  "422",
  "422",       // use mpeg 422 as a fallback
  "444",
  NULL
};

/* the output filter needs to wait until it has seen parameter frames
   from each stream before it can write an output header */
static void startup_header(fq_t *queue){
  internal *i = queue->internal;

  if(!i->wrote_header){
    int j;
    for(j=0;j<queue->streams;j++)
      if(!queue->stream_depth[j])break;

    if(j==queue->streams){

      if(i->outfile){
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
      }

      if(i->wavfile){

        for(j=0;j<queue->streams;j++){
          fq_frame_t *ptr = queue->tail;
          while(ptr && ptr->sno != j)
            ptr=ptr->next;

          if(!ptr){
            yerror("Output couldn't find stream number %d\n\n",j);
            exit(1);
          }

          if(ptr->st==STREAM_AUDIO){
            yinfo("stream %d -> %s\n",ptr->sno,i->wavfilename);
            i->wavno = ptr->sno;
            i->wavrate = ptr->sp.audio.rate;
            i->wavch = ptr->sp.audio.ch;
            waveheader(i,0x7fffffff); /* bogus size initially */
            /* amend with correct duration later if possible */
            break;
          }
        }
        if(i->wavno<0){
          ywarn("No audio streams found; closing WAV file\n");
          fclose(i->wavfile);
          i->wavfile=NULL;
        }
      }

      if(i->yuvfile){

        for(j=0;j<queue->streams;j++){
          fq_frame_t *ptr = queue->tail;
          while(ptr && ptr->sno != j)
            ptr=ptr->next;

          if(!ptr){
            yerror("Output couldn't find stream number %d\n\n",j);
            exit(1);
          }

          if(ptr->st==STREAM_VIDEO){
            yinfo("stream %d -> %s\n",ptr->sno,i->yuvfilename);
            i->yuvno = ptr->sno;

            if(!translateformat1[ptr->sp.video.format]){
              if(!translateformat2[ptr->sp.video.format]){
                yerror("No YUV4MPEG output fallback implemented for pixel format C%s.\n\n",
                       chromaformat[ptr->sp.video.format]);
                exit(1);
              }else{
                ywarn("YUV4MPEG2 has no equivalent to pixel format C%s\n",
                      chromaformat[ptr->sp.video.format]);
                ywarn("using C%s as fallback; colors may be shifted 1/2 pixel\n\n",
                      translateformat2[ptr->sp.video.format]);
              }
            }

            fprintf(i->yuvfile,"YUV4MPEG2 W%d H%d F%d:%d I%c A%d:%d C%s\n",
                    ptr->sp.video.w,
                    ptr->sp.video.h,
                    ptr->sp.video.fps_n,
                    ptr->sp.video.fps_d,
                    ptr->sp.video.i?(ptr->sp.video.i==TOP_FIRST?'t':'b'):'p',
                    ptr->sp.video.pa_n,
                    ptr->sp.video.pa_d,
                    translateformat2[ptr->sp.video.format]);
          }
        }

        if(i->yuvno<0){
          ywarn("No video streams found; closing YUV4MPEG2 file\n");
          fclose(i->yuvfile);
          i->yuvfile=NULL;
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

    i = queue->internal = calloc(1,sizeof(*i));
    parse_options(queue);
    i->wavno=-1;
    i->yuvno=-1;

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

        if(i->outfile){
          if(fprintf(i->outfile,"FRAME S%d L%d P%.3f\n",ptr->sno,ptr->body_size,ptr->pts)<0 ||
             fwrite(ptr->body,1,ptr->body_size,i->outfile)<(unsigned)ptr->body_size){
            yerror("Unable to write to output; %s\n",strerror(errno));
            exit(1);
          }
        }

        if(i->wavfile && ptr->sno==i->wavno){
          /* 24 -> 16 */
          int j;
          for(j=0;j<ptr->body_size;j+=3){
            fputc(ptr->body[j+1],i->wavfile);
            fputc(ptr->body[j+2],i->wavfile);
          }
        }

        if(i->yuvfile && ptr->sno==i->yuvno){
          if(fprintf(i->yuvfile,"FRAME\n")<0 ||
             fwrite(ptr->body,1,ptr->body_size,i->yuvfile)<(unsigned)ptr->body_size){
            yerror("Unable to write to YUV output; %s\n",strerror(errno));
            exit(1);
          }
        }
      }

      /* It's possible something follows this output filter. Safe to call
         if the queue is empty (eg, during QUEUE_INIT)*/
      filter_forward_frame(queue);

    }

    if(queue->state==QUEUE_FLUSH){
      if(i->outfile){
        fclose(i->outfile);
        i->outfile=NULL;
      }

      if(i->wavfile){
        /* update duration is possible */
        if(!fseek(i->wavfile,0,SEEK_SET))
          waveheader(i,i->wavebytes);

        fclose(i->wavfile);
        i->wavfile=NULL;
      }

      if(i->yuvfile){
        fclose(i->yuvfile);
        i->yuvfile=NULL;
      }

      queue->state=QUEUE_FINISHED;
    }
    break;

  case QUEUE_FINISHED:
    break;
  }

}
