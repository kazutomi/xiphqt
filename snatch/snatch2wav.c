/*
 *
 *  snatch2wav.c
 *    
 *	Copyright (C) 2001 Monty
 *
 *  This file is part of snatch2wav, a component of the "MJPEG tools"
 *  suite of open source tools for MJPEG and MPEG processing.
 *	
 *  snatch2wav is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  snatch2wav is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *
 */

/* Snatch files can consist of multiple depths, rates and channels.
   This resamples the given input files to the first rate/number of
   channels it sees (or user selected format). */

#include <stdio.h>

extern long       audbuf_rate;
extern int        audbuf_channels;
extern long long  audbuf_samples;

extern long long samplesin;
extern long long samplesout;
extern long long samplesmissing;
extern long long samplesdiscarded;

extern double vidbuf_fps;
extern int video_timeahead; 

int snatch_iterator(FILE *in,FILE *out,int process_a,int process_v);

int main(){
  int done=0;
  int noisy =1;

  video_timeahead=2; /* absolute minimum for sync to work */


  vidbuf_fps=30;

  while(!done){
    done=snatch_iterator(stdin,stdout,1,0);

    if(noisy){
      long seconds=samplesout/audbuf_rate;
      long minutes=seconds/60;
      long hours;

      seconds-=minutes*60;
      hours=minutes/60;
      minutes-=hours*60;

      fprintf(stderr,"\rSamples %ld->%ld dropped:%ld missing:%ld  %ld:%02ld:%02ld",
	      (long)samplesin,(long)samplesout,(long)samplesdiscarded,(long)samplesmissing,
	      hours,minutes,seconds);
    }
  }
	      
  if(noisy)fprintf(stderr,"\n");
  return(0);
}
  



