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

#include <unistd.h>
#include <stdlib.h>
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

static void usage(FILE *f){
  fprintf(f,
	  "snatch2wav 20011115\n\n"
	  "USAGE: snatch2wav [options] < infile { > outfile, | nextutil }\n\n"
	  "OPTIONS:\n"
	  "  -b <N>    : skip first <N> seconds of input file\n"
	  "  -c <N>    : convert to <N> output channels\n"
	  "  -h        : this information to stdout\n"
	  "  -n <N>    : output only up to the last frame beginning \n"
	  "              before <N> seconds elapsed from start of file\n"
	  "              (if preceeding or without -b) or from start of\n"
	  "              output (following -b)\n"
	  "  -q        : operate quietly\n"
	  "  -r <N>    : resample to output rate <N>\n\n");
}

const char *optstring = "b:c:hn:qr:";

extern double begin_time;
extern double end_time;

int main(int argc,char *argv[]){
  int done=0;
  int noisy =1;
  int c;

  video_timeahead=2; /* absolute minimum for sync to work */
  vidbuf_fps=30;     /* not critical in audio, but must still be set */

  while((c=getopt(argc,argv,optstring))!=EOF){
    switch(c){
    case 'b':
      begin_time=atof(optarg);
      break;
    case 'c':
      audbuf_channels=atoi(optarg);
      if(audbuf_channels<1)audbuf_channels=1;
      if(audbuf_channels>2)audbuf_channels=2;
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
    case 'r':
      audbuf_rate=atol(optarg);
      if(audbuf_rate<4000)audbuf_rate=4000;
      if(audbuf_rate>48000)audbuf_rate=48000;
      break;
    default:
      usage(stderr);
      exit(1);
    }
  }

  while(!done){
    done=snatch_iterator(stdin,stdout,1,0);

    if(noisy && audbuf_rate){
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
  



