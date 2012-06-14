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

#include "waveform.h"
#include "io.h"

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

static int metareload = 0;

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
    if(ret==0) break;
    if(ret==-1){
      /* a pipe returned EOF; attempt reopen */
      pthread_mutex_lock(&blockbuffer_mutex);
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

    write(eventpipe[1],"",1);

  }

  /* eof on all inputs */
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

static fetchdata fetch_ret;
fetchdata *process_fetch(int span, int scale, float range,
                         int *process_in){
  int fi,i,k,ch;
  int process[total_ch];

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

  /* by channel */
  ch=0;
  for(fi=0;fi<inputs;fi++){
    int spann = ceil(rate[fi]/1000000.*span)+1;
    for(i=ch;i<ch+channels[fi];i++){
      if(process[i]){
        int offset=blocksize-spann;
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
