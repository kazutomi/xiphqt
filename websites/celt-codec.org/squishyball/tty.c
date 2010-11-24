/*
 *
 *  squishyball
 *
 *      Copyright (C) 2010 Xiph.Org
 *
 *  squishyball is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  squishyball is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with rtrecord; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

/* Encapsulate the curses/terminfo calls for presenting a little panel
   display at the bottom of a terminal window. */

#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include "mincurses.h"
#include "tty.h"



#if 0
void terminal_paint_ui(){
  int cols = columns;

  char topline[cols+1];
  char botline[cols+1];
  char dmabuf[cols+1];
  char diskbuf[cols+1];
  char hwbuf[cols+1];
  char linebuf[cols+1];

  int barchoice=-1;
  int barcols = cols - 25, barpad=0;
  int barlength=0;
  int i=0;

  topline[0]='\0';
  botline[0]='\0';
  dmabuf[0]='\0';
  diskbuf[0]='\0';
  hwbuf[0]='\0';
  linebuf[0]='\0';

  /* construct the top line */
  //snprintf(dmabuf,cols+1," DMA buffer: %3d%% ",(int)rint(dma_percent));
  //snprintf(diskbuf,cols+1," Disk buffer: %3d%% ",(int)rint(disk_percent));
  //snprintf(hwbuf,cols+1," %s ",device);
  //fill(topline,'_',cols);
  //print_into(topline,3,dmabuf);
  //print_into(topline,5+strlen(dmabuf),diskbuf);
  //print_into(topline,columns - strlen(hwbuf)-3,hwbuf);

  /* print the top line */
  //cursor_to(0,0);
  //print_hline(topline,-1);

  /* blank next line (useful if coming back after SIGSTOP) */
  //cursor_to(0,1);
  //clear_line();

  /* bargraphs */

  for(i=0;i<channels;i++){
    cursor_to(barpad,2+i);
    clear_to_cursor();
    snprintf(linebuf,cols+1,"%2d: [",i);
    putp(linebuf);

    if(barchoice>=0){
      /* determine bar lengths */
      int rms_pos = barpos(barchoice,rms_dB[i]);
      int peak_pos = barpos(barchoice,peak_dB[i]);

      if(peak_pos>barlength)peak_pos=barlength;
      if(rms_pos>peak_pos)rms_pos=peak_pos;

      /* color the bargraph */
      if(rms_pos){
	color(COLOR_BLACK,COLOR_WHITE);
	fwrite(bargraphs[barchoice],1,rms_pos,stdout);
      }
      if(peak_pos-rms_pos>0){
	color(COLOR_BLACK,COLOR_CYAN);
	fwrite(bargraphs[barchoice]+rms_pos,1,peak_pos-rms_pos,stdout);
      }
      if(barlength-peak_pos>0){
	color(COLOR_CYAN,COLOR_BLACK);
	fwrite(bargraphs[barchoice]+peak_pos,1,barlength-peak_pos,stdout);
      }

      /* the RMS indicator is always normal colors */
      unset_attributes();
    }

    if(weight){
      snprintf(linebuf,cols+1,"] %+6.1fdBA,",(double)rms_dB[i]);
    }else{
      snprintf(linebuf,cols+1,"] %+6.1fdB, ",(double)rms_dB[i]);
    }
    putp(linebuf);

    /* the peak indicator may read a number or CLIP */
    if(peak_clip[i]){
      color(COLOR_RED,-1);
      putp("**CLIP**");
      color(-1,-1);
    }else{
      snprintf(linebuf,cols+1,"%+6.1fdB",(double)peak_hold_dB[i]);
      putp(linebuf);
    }
    if(barpad + barcols < columns) clear_line();
  }

  /* blank next line (useful if coming back after SIGSTOP) */
  cursor_to(0,channels+2);
  clear_line();

  /* construct the bottom line */
  fill(botline,'_',cols);
  {
    int x=3;
    if(paused){
      print_into(botline,x," PAUSED ");
      x+=10;
    }
    if(hold){
      print_into(botline,x," HOLD ");
      x+=8;
    }
    if(dma_once_overrun){
      print_into(botline,x," DMA OVERRUN ");
      x+=16;
    }
    if(disk_once_overrun){
      print_into(botline,x," DISK OVERRUN ");
      x+=17;
    }
  }

  /* print the bottom line */
  cursor_to(0,channels+3);
  print_hline(botline,COLOR_RED);
  fflush(stdout);
}

void terminal_main_loop(int quiet){
  int i;
  int exiting=0;

  while(!exiting){
    char buf;
    if(!quiet){
      resetup_term();
      terminal_paint_ui();
    }
    read(STDIN_FILENO,&buf,1);

    if(!quiet){
      switch(buf){
      case 'p':
	paused = !paused;
	break;
      case 'h':
	hold = !hold;
	break;
      case ' ':
	dma_once_overrun=0;
	disk_once_overrun=0;
	for(i=0;i<channels;i++){
	  peak_clip[i]=0;
	  peak_hold_dB[i]=peak_dB[i];
	}
	break;
      case 'q':case 'Q':
	exiting=1;
	break;
      }
    }
  }
}
#endif
