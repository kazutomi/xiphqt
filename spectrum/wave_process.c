/*
 *
 *  gtk2 waveform viewer
 *    
 *      Copyright \(C\) 2004-2012 Monty
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

pthread_mutex_t feedback_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
int feedback_increment=0;
float **buffercopies;
float **plot_data=0;

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

sig_atomic_t acc_clear=0;
sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

static void init_process(void){
  int i;
  if(plot_data==NULL){
    plot_data=calloc(total_ch,sizeof(*plot_data));
    buffercopies=calloc(total_ch,sizeof(*buffercopies));
    for(i=0;i<total_ch;i++){
      plot_data[i]=calloc(blocksize,sizeof(**plot_data));
      buffercopies[i]=calloc(blocksize,sizeof(**buffercopies));
    }
  }
}

/* return 0 on EOF, 1 otherwise */
static int process(){
  int fi,ch,i;

  if(acc_rewind)
    rewind_files();
  acc_rewind=0;

  if(input_read(acc_loop,1))
    return 0;

  pthread_mutex_lock(&feedback_mutex);
  ch=0;
  for(fi=0;fi<inputs;fi++){
    for(i=ch;i<ch+channels[fi];i++)
      memcpy(buffercopies[i],blockbuffer[i],blocksize*sizeof(**buffercopies));
    ch+=channels[fi];
  }
  pthread_mutex_unlock(&feedback_mutex);

  feedback_increment++;
  write(eventpipe[1],"",1);
  return 1;
}

void *process_thread(void *dummy){
  init_process();
  while(!process_exit && process());
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

float **process_fetch(int *blockslice,int *overslice,int span,
                      int scale,float range){
  int fi,i,j,k,ch;

  init_process();
  if(!blockslice || !overslice || !blockbuffer)return NULL;

  /* by channel */
  ch=0;
  for(fi=0;fi<inputs;fi++){
    /* When the blockslice and overslice are equal, the data
       buffer contains only one copy of the span.  When the
       overslice is much smaller than the blockslice, we have
       ceil(blockslice/overslice) back-to-back spans */

    int copies = (int)ceil(blockslice[fi]/overslice[fi]);
    int spann = ceil(rate[fi]/1000000.*span)+1;

    for(i=ch;i<ch+channels[fi];i++){
      int offset=blocksize-spann;
      float *plotdatap=plot_data[i];
      for(j=0;j<copies;j++){
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
        offset-=overslice[fi];
      }
    }
    ch+=channels[fi];
  }

  return plot_data;
}
