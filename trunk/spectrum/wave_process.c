/*
 *
 *  gtk2 waveform viewer
 *
 *      Copyright (C) 2004-2012 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  The analyzer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#include <math.h>
#include <fftw3.h>
#include "waveform.h"
#include "io.h"

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

int ch_to_fi(int ch){
  int fi,ci;
  for(fi=0,ci=0;fi<inputs;fi++){
    if(ch>=ci && ch<ci+channels[fi]) return fi;
    ci+=channels[fi];
  }
  return -1;
}

typedef struct {
  fftwf_plan fft_f[2];
  fftwf_plan fft_i[2];

  float *lap[2];
  int laphead;
  int lapfill;
  float *window_f;
  float *window_i;
  int rate;
  int holdoffd;
  int spansamples;

  int sample_n;
  int oversample_n;
  int oversample_factor;

  /* distance back from head of blockbuffer */
  int lappos; /* head of the lap */
  int triggersearch; /* point to start next search */
} triggerstate;

static int metareload = 0;
static triggerstate *trigger=NULL;
static int trigger_channel=0;
static int trigger_type=0;
static double lasttrigger=-1;

static triggerstate *trigger_create(int rate, int holdoffd, int span){
  triggerstate *t = calloc(1,sizeof(*t));
  int i;

  /* hardwired but reasonable */
  t->sample_n = 512;
  t->oversample_factor = 16;
  t->oversample_n = t->sample_n * t->oversample_factor;

  t->rate=rate;
  t->holdoffd=holdoffd;
  t->spansamples=rate/1000000.*span;

  for(i=0; i<2; i++){
    t->lap[i] = fftwf_malloc((t->oversample_n*2+2)*sizeof(**t->lap));
    t->fft_f[i] = fftwf_plan_dft_r2c_1d(t->sample_n*2,t->lap[i],
                                        (fftwf_complex *)t->lap[i],
                                        FFTW_ESTIMATE);
    t->fft_i[i] = fftwf_plan_dft_c2r_1d(t->oversample_n*2,
                                        (fftwf_complex *)t->lap[i],t->lap[i],
                                        FFTW_ESTIMATE);
  }

  /* highly redundant; make it work first */
  t->window_f = malloc(t->sample_n*sizeof(*t->window_f));
  t->window_i = malloc(t->oversample_n*sizeof(*t->window_i));
  for(i=0;i<t->sample_n;i++)
    t->window_f[i] = sin(i*M_PI/t->sample_n);
  for(i=0;i<t->oversample_n;i++)
    t->window_i[i] = sin(i*M_PI/t->oversample_n);

  return t;
}

static void trigger_destroy(triggerstate *t){
  if(t){
    int i;
    for(i=0;i<2;i++){
      if(t->lap[i]) free(t->lap[i]);
      if(t->fft_f[i]) fftwf_destroy_plan(t->fft_f[i]);
      if(t->fft_i[i]) fftwf_destroy_plan(t->fft_i[i]);
    }
    if(t->window_f)free(t->window_f);
    if(t->window_i)free(t->window_i);
    free(t);
  }
}

/* returns nonzero if there's enough data to bother trying a trigger search */
static int trigger_advance(triggerstate *t, int blockslice){
  t->lappos += blockslice;
  t->triggersearch += blockslice * t->oversample_factor;
  if(lasttrigger>=0)
    lasttrigger += (double)blockslice/t->rate;
  if(lasttrigger>1) lasttrigger=-1;

  if(t->triggersearch < t->spansamples * t->oversample_factor) return 0;
  if(t->triggersearch < t->oversample_n/2) return 0;
  return 1;
}

static int trigger_try_lap(triggerstate *t, float *blockbuffer, int bn){
  int i;

  int lap_end = t->lappos * t->oversample_factor;
  int overlap_end = (t->lappos + t->sample_n/2) * t->oversample_factor;

  /* don't process any oversamples/overlaps that we don't need to */
  while(t->triggersearch < lap_end-t->sample_n/2){
    t->lapfill=0;

    if(t->lappos<t->sample_n/2) return -1; /* out of data, off the deep end */

    t->lappos -= t->sample_n/2;
    lap_end -= t->oversample_n/2;
    overlap_end -= t->oversample_n/2;
  }

  if(t->lapfill==0){
    if(t->lappos<t->sample_n/2) return -1; /* out of data, off the deep end */

    /* advance buffers and oversample */
    t->lapfill++;
    t->lappos-=t->sample_n/2;

    if(t->lappos > bn-t->sample_n){
      /* we got *way* behind. ~restart at head */
      t->lappos = t->sample_n/2;
    }

    /* copy/window */
    float *work = t->lap[t->laphead];
    float *src = blockbuffer+bn-t->lappos-t->sample_n;
    memset(work,0, sizeof(**t->lap)*(t->oversample_n*2+2));

    work+=t->sample_n/2;
    for(i=0;i<t->sample_n;i++)
      work[i] = src[i]*t->window_f[i];

    /* transform */
    fftwf_execute(t->fft_f[t->laphead]);
    fftwf_execute(t->fft_i[t->laphead]);

    /* rewindow */
    work-=t->sample_n/2;
    work+=t->oversample_n/2;
    for(i=0;i<t->oversample_n;i++)
      work[i] *= t->window_i[i];

    /* switch */
    t->laphead = ((t->laphead+1)&1);

    /* leave the edges unzeroed; they're not used past this */
  }

  if(t->lappos<t->sample_n/2) return -1; /* out of data, off the deep end */

  {
    /* advance buffers and oversample into head */
    t->lapfill++;
    t->lappos-=t->sample_n/2;

    /* copy/window */
    float *work = t->lap[t->laphead];
    float *src = blockbuffer+bn-t->lappos-t->sample_n;
    memset(work,0, sizeof(**t->lap)*(t->oversample_n*2+2));

    work+=t->sample_n/2;
    for(i=0;i<t->sample_n;i++)
      work[i] = src[i]*t->window_f[i];

    /* transform */
    fftwf_execute(t->fft_f[t->laphead]);
    fftwf_execute(t->fft_i[t->laphead]);

    /* rewindow */
    work-=t->sample_n/2;
    work+=t->oversample_n/2;
    for(i=0;i<t->oversample_n;i++)
      work[i] *= t->window_i[i];

    /* overlap-add */
    work-=t->oversample_n/2;
    float *a = t->lap[(t->laphead+1)&1] + t->oversample_n;
    float *b = work+t->oversample_n/2;
    for(i=0;i<t->oversample_n/2;i++)
      work[i] = a[i]+b[i];

    /* switch */
    t->laphead = ((t->laphead+1)&1);
  }

  return 0;
}

/* returns location of trigger as number of seconds behind head of
   blockbuffer, or <0 if none */
static float trigger_search(triggerstate *t, float *blockbuffer, int bn, int triggertype){
  if(t->lappos+t->sample_n>bn || t->lappos<0){
    /* we got behind */
    t->lappos=bn-t->sample_n;
    t->lapfill=0;
  }

  while(1){
    if(t->lapfill<2){
      if(trigger_try_lap(t,blockbuffer, blocksize)){
        return -1; /* need more blockbuffer data */
      }
    }else{

      /* trigger limit can't reach past head + span */
      int span_begin = t->spansamples * t->oversample_factor;

      if(t->triggersearch <= span_begin){
        return -1; /* need more blockbuffer data */
      }else{
        /* distance back from logical head of the sample buffer to lap tail */
        int overlap_end = (t->lappos + t->sample_n/2) * t->oversample_factor;

        if(t->triggersearch <= overlap_end){
          if(trigger_try_lap(t,blockbuffer,blocksize)){
            return -1; /* need more blockbuffer data */
          }
        }else{

          int overlap_begin = (t->lappos + t->sample_n) * t->oversample_factor;
          if(t->triggersearch > overlap_begin){
            /* fell off beginning of overlap area-- we got behind processing */
            t->triggersearch = overlap_begin;
          }

          float *lap = t->lap[(t->laphead+1)&1];
          int lapoff = t->oversample_n - t->triggersearch +
            t->lappos*t->oversample_factor;

          float prev = (lapoff>0 ? lap[lapoff-1] : t->lap[t->laphead][t->oversample_n-1]);
          lap+=lapoff;

          while(t->triggersearch>overlap_end &&
                t->triggersearch>span_begin){

            switch(triggertype){
            case 1: /* +0 */
              if(prev<=0 && *lap>0){
                /* linear interpolation can further refine the result in most cases */
                float x = *lap / (*lap-prev);
                float ret = (t->triggersearch+x)/
                  (t->rate*t->oversample_factor);
                /* apply holdoff */
                t->triggersearch -= t->rate*t->oversample_factor/t->holdoffd;
                /* return trigger */
                return ret;
              }
              break;
            case 2: /* -0 */
              if(prev>0 && *lap<=0){
                /* linear interpolation can further refine the result in most cases */
                float x = *lap / (*lap-prev);
                float ret = (t->triggersearch+x)/
                  (t->rate*t->oversample_factor);
                /* apply holdoff */
                t->triggersearch -= t->rate*t->oversample_factor/t->holdoffd;
                /* return trigger */
                return ret;
              }
              break;
            }
            t->triggersearch--;
            prev=*lap++;
          }
        }
      }
    }
  }
}

static void process_init(){
  if(blocksize==0){
    int fi;
    /* set block size equal to maximum input rate + epsilon */
    /* (maximum display width: 1s, maximum update interval 1s) */
    for(fi=0;fi<inputs;fi++)
      if(rate[fi]>blocksize)blocksize=rate[fi]+16;
  }
}

/* return 0 on EOF, 1 otherwise */
void *process_thread(void *dummy){
  int ret;

  while(!process_exit){

    process_init();

    if(acc_rewind) rewind_files();
    acc_rewind=0;

    ret=input_read(acc_loop,1);
    if(ret==0){
      pthread_mutex_unlock(&blockbuffer_mutex);
      break;
    }
    if(ret==-1){
      /* a pipe returned EOF; attempt reopen */
      if(pipe_reload()){
        blocksize=0;
        metareload=1;
        pthread_mutex_unlock(&blockbuffer_mutex);
        write(eventpipe[1],"",1);
        continue;
      }else{
        pthread_mutex_unlock(&blockbuffer_mutex);
        break;
      }
    }

    /* advance trigger [if any] by blockslice */
    if(trigger){
      int ret;
      int fi = ch_to_fi(trigger_channel);
      ret=trigger_advance(trigger,blockslice_adv[fi]);
      pthread_mutex_unlock(&blockbuffer_mutex);
      if(ret)write(eventpipe[1],"",1);
    }else{
      pthread_mutex_unlock(&blockbuffer_mutex);
      write(eventpipe[1],"",1);
    }
  }

  /* eof on all inputs */
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

/* everything below called from UI thread only */

void set_trigger(int ttype, int tch, int sliced, int span){
  pthread_mutex_lock(&blockbuffer_mutex);
  if(!blockbuffer){
    pthread_mutex_unlock(&blockbuffer_mutex);
    return;
  }

  /* if trigger params have changed, clear out current state */
  if(trigger_type!=ttype || trigger_channel!=tch){
    trigger_destroy(trigger);
    trigger=NULL;
  }

  /* set up trigger if one if called for */
  if(ttype && !trigger){
    int fi = ch_to_fi(tch);
    trigger_channel = tch;
    trigger_type = ttype;
    trigger = trigger_create(rate[fi], sliced, span);
    blockslice_frac = 200;
  }else{
    blockslice_frac = sliced;
  }
  lasttrigger=-1;
  pthread_mutex_unlock(&blockbuffer_mutex);
}


static fetchdata fetch_ret;
fetchdata *process_fetch(int span, int scale, float range,
                         int *process_in){
  int fi,i,k,ch;
  int process[total_ch];
  int samppos[total_ch];

  pthread_mutex_lock(&blockbuffer_mutex);
  if(!blockbuffer){
    pthread_mutex_unlock(&blockbuffer_mutex);
    return NULL;
  }

  if(metareload){
    if(fetch_ret.data){
      for(i=0;i<fetch_ret.total_ch;i++)
        if(fetch_ret.data[i])free(fetch_ret.data[i]);
      free(fetch_ret.data);
      fetch_ret.data=NULL;
    }
    if(fetch_ret.active){
      free(fetch_ret.active);
      fetch_ret.active=NULL;
    }
    if(trigger){
      int sliced = trigger->holdoffd;
      trigger_destroy(trigger);
      trigger = NULL;
      set_trigger(trigger_type, trigger_channel, sliced, span);
    }
  }

  if(!fetch_ret.data){
    fetch_ret.data = calloc(total_ch,sizeof(*fetch_ret.data));
    for(i=0;i<total_ch;i++)
      fetch_ret.data[i]=calloc(blocksize,sizeof(**fetch_ret.data));
  }

  if(!fetch_ret.active)
    fetch_ret.active = calloc(total_ch,sizeof(*fetch_ret.active));

  /* the passed in process array doesn't necesarily match the
     current channel structure.  Copy group by group. */
  {
    int ch_now=0;
    int ch_in=0;
    for(i=0;i<inputs;i++){
      int ci;
      for(ci=0;ci<channels[i] && ci<fetch_ret.channels[i];ci++)
        process[ch_now+ci] = process_in[ch_in+ci];
      for(;ci<channels[i];ci++)
        process[ch_now+ci] = 0;
      ch_now+=channels[i];
      ch_in+=fetch_ret.channels[i];
    }
    memcpy(fetch_ret.active,process,total_ch*sizeof(*process));
  }

  fetch_ret.groups=inputs;
  fetch_ret.scale=scale;
  fetch_ret.span=span;
  fetch_ret.range=range;
  fetch_ret.total_ch=total_ch;

  memcpy(fetch_ret.bits,bits,sizeof(fetch_ret.bits));
  memcpy(fetch_ret.channels,channels,sizeof(fetch_ret.channels));
  memcpy(fetch_ret.rate,rate,sizeof(fetch_ret.rate));

  fetch_ret.reload=metareload;
  metareload=0;

  {
    float sec=-1;
    if(trigger){
      int fi = ch_to_fi(trigger_channel);

      /* read position determined by trigger */
      if(process_active || lasttrigger<0){
        sec = trigger_search(trigger, blockbuffer[trigger_channel], blocksize, trigger_type);
      }
      if(sec<0){
        sec = lasttrigger;
        /* lasttrigger follows the channel, not the master clock */
        sec += (blockslice_count/1000000.) - (blockslice_cursor[fi]/(float)rate[fi]);
      }else{
        lasttrigger = sec;
        /* position returned from the trigger search is in terms of the
           trigger channel, not the blockslice cursor; convert it */
        sec += (blockslice_count/1000000.) - (blockslice_cursor[fi]/(float)rate[fi]);

      }
    }

    /* determine sample offsets and sample process position */
    /* fractional sample offsets come from two sources:
       1) non-integer ratio of sample clock : sweep period
       2) non-sampled-aligned trigger

       When a trigger is active, the zero time position on the graph
       is aligned to the trigger, and all else is adjusted to match.
       Offsets must be in the range (-2:-1] (we start one sample early
       when drawing); the read sample position of each channel is
       massaged to bring this in line if necessary */

    for(fi=0;fi<inputs;fi++){
      if(blockslice_eof[fi] || sec<0){
        /* at EOF or when untriggered, we freeze to last sample */
        float sample_pos = rate[fi]/1000000.*span;
        samppos[fi] = blocksize - ceilf(sample_pos) ;
        fetch_ret.offsets[fi] = sample_pos - ceilf(sample_pos);
      }else{
        float head_sec = blockslice_cursor[fi]/(float)rate[fi];
        float read_sec = blockslice_count/1000000.;
        float head_offset = (head_sec-read_sec)*rate[fi];
        float sample_pos = sec*rate[fi] + head_offset;
        samppos[fi] = blocksize - ceilf(sample_pos);
        fetch_ret.offsets[fi] = sample_pos - ceilf(sample_pos);
      }
    }
  }

  /* by channel */
  ch=0;
  for(fi=0;fi<inputs;fi++){
    int spann = ceil(rate[fi]/1000000.*span)+2;
    for(i=ch;i<ch+channels[fi];i++){
      if(process[i]){
        int offset = samppos[fi];
        float *plotdatap=fetch_ret.data[i];
        float *data=blockbuffer[i]+offset;
        if(scale){
          float drange=todB(range)-scale;
          for(k=0;k<spann;k++){
            if(data[k]<0){
              *plotdatap=-(todB(data[k])-scale)/drange;
              if(*plotdatap>0.)*plotdatap=0.;
            }else{
              *plotdatap=(todB(data[k])-scale)/drange;
              if(*plotdatap<0.)*plotdatap=0.;
            }
            plotdatap++;
          }
        }else{
          for(k=0;k<spann;k++)
            *(plotdatap++)=(data[k]/range);
        }
      }
    }
    ch+=channels[fi];
  }

  pthread_mutex_unlock(&blockbuffer_mutex);
  return &fetch_ret;
}
