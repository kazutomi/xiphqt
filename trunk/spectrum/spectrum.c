/*
 *
 *  gtk2 spectrum analyzer
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

#include "analyzer.h"
#include "io.h"
#include <signal.h>
#include <getopt.h>
#include <fenv.h>  // Thank you C99!
#include <fftw3.h>
#include <gtk/gtk.h>
#include "version.h"

int eventpipe[2];
char *version;
char *inputname[MAX_FILES];
int inputs=0;
int blocksize = 131072;
extern int plot_bold;

void handler(int sig){
  signal(sig,SIG_IGN);
  if(sig!=SIGINT){
    fprintf(stderr,
	    "\nTrapped signal %d; exiting!\n"
	    "This signal almost certainly indicates a bug in the analyzer;\n"
	    "If this version of the analyzer is newer than a few months old,\n"
	    "please email a detailed report of the crash along with\n"
	    "processor type, OS version, FFTW3 version, and as much\n"
	    "information as possible about what caused the crash.  The best\n"
	    "possible report will outline the exact steps needed to\n"
	    "reproduce the error, ensuring that I can fix the bug as\n"
	    "quickly as possible.\n\n"
	    "-- monty@xiph.org, spectrum revision %s\n\n",sig,version);
  }
  
  gtk_main_quit();
}

const char *optstring = "-r:c:EeBlb:suhF:T";

struct option options [] = {
        {"bold",no_argument,NULL,'T'},
        {"rate",required_argument,NULL,'r'},
        {"channels",required_argument,NULL,'c'},
        {"big-endian",no_argument,NULL,'E'},
        {"little-endian",no_argument,NULL,'e'},
        {"bits",required_argument,NULL,'b'},
        {"signed",no_argument,NULL,'s'},
        {"unsigned",no_argument,NULL,'u'},
        {"help",no_argument,NULL,'h'},
        {"fft-size",required_argument,NULL,'F'},

        {NULL,0,NULL,0}
};

static void usage(FILE *f){
  fprintf( f,
"\ngtk2 spectrum analyzer, revision %s\n\n"

"USAGE:\n\n"
"  spectrum [options] [file]\n\n"

"OPTIONS:\n\n"
"  -b --bits <bits>           : Force input to be read as 8, 16, 24 or 32 bit\n"
"                               PCM. Default bit depth is normally read from\n"
"                               the file/stream header or set to 16 bits\n"
"                               for raw input.\n"
"  -B -E --big-endian         : Force input to be read as big endian.\n"
"                               Default endianness is normally read from the\n"
"                               file/stream header or set to host"
"                               endianness for raw input.\n"
"  -c --channels <channels>   : Input channel number override; use to\n"
"                               specify the number of channels in a raw\n"
"                               input.  default: 1\n"
"  -e -l --little-endian      : Force input to be read as little endian.\n"
"                               Default endianness is normally read from the\n"
"                               file/stream header or set to host"
"                               endianness for raw input.\n"
"  -F --fft-size              : Set the size of the fft transform used. Valid\n"
"                               range 8192 to 262144, 131072 default.\n"
"  -h --help                  : print this help\n"
"  -r --rate <Hz>             : Input sample rate override in Hz; use to\n"
"                               specify the rate of a raw input.\n"
"                               default: 44100\n"
"  -s --signed                : Force input to be read as signed PCM.\n"
"                               Signedness is normally read from the \n"
"                               file/stream header or set to signed for raw\n"
"                               input.\n"
"  -T --bold                  : plot spectrum with thicker/bolder lines.\n"
"  -u --unsigned              : Force input to be read as unsigned PCM.\n"
"                               Signedness is normally read from the \n"
"                               file/stream header or set to signed for raw\n"
"                               input.\n\n"

"INPUT:\n\n"

" Spectrum takes raw, WAV or AIFF input either from stdin or from \n"
" file[s]/stream[s] specified on the command line.\n\n",version);

}

void parse_command_line(int argc, char **argv){
  int c,long_option_index;

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 1:
      /* file name that belongs to current group */
      if(inputs>=MAX_FILES){
	fprintf(stderr,"Maximum of MAX_FILES input files exceeded.  Oops.  Programmer was lazy.\n\n");
	exit(1);
      }
      inputname[inputs++]=strdup(optarg);
      break;
    case 'T':
      plot_bold = 1;
      break;
    case 'b':
      /* force bit width */
      {
	int a=atoi(optarg);
	bits[inputs]=a;
	if(a!=8 && a!=16 && a!=24 && a!=32){
	  usage(stderr);
	  exit(1);
	}
      }
      break;
    case 'F':
      blocksize = atoi(optarg);
      if(blocksize<8192 || blocksize>262144){
	usage(stderr);
	exit(1);
      }
      break;
    case 'B':case 'E':
      /* force big endian */
      bigendian[inputs]=1;
      break;
    case 'c':
      /* force channels */
      {
	int a=atoi(optarg);
	channels[inputs]=a;
	if(a<1 || a>32){
	  usage(stderr);
	  exit(1);
	}
      }
      break;
    case 'l':case 'e':
      /* force little endian */
      bigendian[inputs]=0;
      break;
    case 'h':
      usage(stdout);
      exit(0);
    case 'r':
      /* force rate */
      {
	int a=atoi(optarg);
	rate[inputs]=a;
	if(a<4000 || a>200000){
	  usage(stderr);
	  exit(1);
	}
      }
      break;
    case 's':
      /* force signed */
      signedp[inputs]=1;
      break;
    case 'u':
      /* force unsigned */
      signedp[inputs]=0;
      break;
    default:
      usage(stderr);
      exit(0);
    }
  }
}

static int sigill=0;
void sigill_handler(int sig){
  /* make sure */
  if(sig==SIGILL)sigill=1;
}

int main(int argc, char **argv){
  int fi;

  version=strstr(VERSION,"version.h");
  if(version){
    char *versionend=strchr(version,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend){
      int len=versionend-version-9;
      version=strdup(version+10);
      version[len-1]=0;
    }
  }else{
    version="";
  }

  /* parse command line and open all the input files */
  parse_command_line(argc, argv);

  /* We do not care about FPEs; rather, underflow is nominal case, and
     its better to ignore other traps in production than to crash the
     app.  Please inform the FPU of this. */

#ifndef DEBUG
  fedisableexcept(FE_INVALID);
  fedisableexcept(FE_INEXACT);
  fedisableexcept(FE_UNDERFLOW);
  fedisableexcept(FE_OVERFLOW);
#else
  feenableexcept(FE_INVALID);
  feenableexcept(FE_INEXACT);
  feenableexcept(FE_UNDERFLOW);
  feenableexcept(FE_OVERFLOW);
#endif 

  /* Linux Altivec support has a very annoying problem; by default,
     math on denormalized floats will simply crash the program.  FFTW3
     uses Altivec, so boom, but only random booms.
     
     By the C99 spec, the above exception configuration is also
     supposed to handle Altivec config, but doesn't.  So we use the
     below ugliness to both handle altivec and non-alitvec PPC. */

#ifdef __PPC
#include <altivec.h>
  signal(SIGILL,sigill_handler);
  
#if (defined __GNUC__) && (__GNUC__ == 3) && ! (defined __APPLE_CC__)
  __vector unsigned short noTrap = 
    (__vector unsigned short){0,0,0,0,0,0,0x1,0};
#else
  vector unsigned short noTrap = 
    (vector unsigned short)(0,0,0,0,0,0,0x1,0);
#endif

  vec_mtvscr(noTrap);
#endif

  /* easiest way to inform gtk of changes and not deal with locking
     issues around the UI */
  if(pipe(eventpipe)){
    fprintf(stderr,"Unable to open event pipe:\n"
            "  %s\n",strerror(errno));
    
    exit(1);
  }

  /* Allows event compression on the read side */
  if(fcntl(eventpipe[0], F_SETFL, O_NONBLOCK)){
    fprintf(stderr,"Unable to set O_NONBLOCK on event pipe:\n"
            "  %s\n",strerror(errno));
    
    exit(1);
  }

  //signal(SIGINT,handler);
  signal(SIGSEGV,handler);

  if(input_load())exit(1);

  /* select the full-block slice size: ~10fps */
  for(fi=0;fi<inputs;fi++){
    blockslice[fi]=rate[fi]/10;
    while(blockslice[fi]>blocksize/2)blockslice[fi]/=2;
  }

  /* go */
  panel_go(argc,argv);

  return(0);
}
