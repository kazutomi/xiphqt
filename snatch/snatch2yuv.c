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

/* Snatch files can consist of multiple resolutions and rates.  This
   scales the given input files to the first rate/size it sees (or
   user selected format). */

#include <stdio.h>

extern int        vidbuf_height;
extern int        vidbuf_width;
extern double     vidbuf_fps;
extern int        ratecode;
extern int        video_timeahead;
			 
extern int scale_width;
extern int scale_height;

extern long long framesin;
extern long long framesout;
extern long long framesmissing;
extern long long framesdiscarded;
      
int snatch_iterator(FILE *in,FILE *out,int process_a,int process_v);

static int num_rates=9;
static double framerates[]={
  0., 23.976 ,24., 25., 29.970, 30., 50., 59.940, 60. };

int main(){
  int done=0;
  int noisy=1;

  ratecode=5;
  vidbuf_fps=30;
  video_timeahead=15;

  while(!done){
    done=snatch_iterator(stdin,stdout,0,1);

    if(noisy){
      long seconds=framesout/vidbuf_fps;
      long minutes=seconds/60;
      long hours;

      seconds-=minutes*60;
      hours=minutes/60;
      minutes-=hours*60;

      fprintf(stderr,"\rFrames %ld->%ld dropped:%ld missing:%ld  %ld:%02ld:%02ld",
	      (long)framesin,(long)framesout,(long)framesdiscarded,(long)framesmissing,
	      hours,minutes,seconds);
    }
  }
	      
  if(noisy)fprintf(stderr,"\n");
  return(0);
}
  



