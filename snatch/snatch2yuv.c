/*
 *
 *  snatch2yuv.c
 *    
 *	Copyright (C) 2001 Monty
 *
 *  This file is part of snatch2yuv, a component of the "MJPEG tools"
 *  suite of open source tools for MJPEG and MPEG processing.
 *	
 *  snatch2yuv is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  snatch2yuv is distributed in the hope that it will be useful,
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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

extern double begin_time;
extern double end_time;

static double framerates[]={
  0., 23.976 ,24., 25., 29.970, 30., 50., 59.940, 60. };

static void usage(FILE *f){
  fprintf(f,
	  "snatch2yuv 20011115\n\n"
	  "USAGE: snatch2yuv [options] < infile { > outfile, | nextutil }\n\n"
	  "OPTIONS:\n"
	  "  -b <N>    : skip first <N> seconds of input file\n"
	  "  -f <N>    : output video in specific MPEG legal\n"
	  "              frames-per-second mode:  \n"
	  "              N = 1, 23.976fps (NTSC  3:2 pulldown)\n" 
          "                  2, 24.000fps (film, 1:2 12fps input)\n"
	  "                  3, 25.000fps (SECAM/PAL)\n"
	  "                  4, 29.970fps (NTSC)\n"
	  "                  5, 30.000fps (15fps@1:2, 10fps@1:3)\n"
	  "                  6, 50.000fps\n"
	  "                  7, 59.940fps\n"
	  "                  8, 60.000fps\n"
	  "              N=5, 30fps default\n"
	  "  -h        : this information to stdout\n"
	  "  -n <N>    : output only up to the last frame beginning \n"
	  "              before <N> seconds elapsed from start of file\n"
	  "              (if preceeding or without -b) or from start of\n"
	  "              output (following -b)\n"
	  "  -q        : operate quietly\n"
	  "  -s <WxH>  : crop/letterbox into an output framesize of\n"
	  "              width W by height H\n\n");
}

const char *optstring = "b:f:hn:qs:";

int main(int argc,char *argv[]){
  int done=0;
  int noisy=1;
  int c;

  ratecode=5;
  vidbuf_fps=30;
  video_timeahead=15;

  while((c=getopt(argc,argv,optstring))!=EOF){
    switch(c){
    case 'b':
      begin_time=atof(optarg);
      break;
    case 'f':
      ratecode=atoi(optarg);
      if(ratecode<1)ratecode=1;
      if(ratecode>8)ratecode=8;
      vidbuf_fps=framerates[ratecode];
      break;
    case 'h':
      usage(stdout);
      return(0);
    case 'n':
      end_time=begin_time+atof(optarg);
      break;
    case 'q':
      noisy=0;
      break;
    case 's':
      {
	char *h=strchr(optarg,'x');
	if(!h){
	  usage(stderr);
	  exit(1);
	}
	h++;
	vidbuf_width=atoi(optarg);
	vidbuf_height=atoi(h);

	if(vidbuf_width<2)vidbuf_width=2;
	if(vidbuf_height<2)vidbuf_height=2;
      }
      break;
    default:
      usage(stderr);
      exit(1);
    }
  }


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
  



