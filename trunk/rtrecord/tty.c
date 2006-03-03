/*
 *
 *  rtrecord
 *    
 *      Copyright (C) 2006 Red Hat Inc.
 *
 *  rtrecord is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  rtrecord is distributed in the hope that it will be useful,
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include <ncurses.h>
#include <curses.h>
#include <term.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "rtrecord.h"

TTY orig;
TTY term;

static int panel_lines=0;
static char *start_graphics=0;
static char *end_graphics=0;
static char *cup_str=0;
static char *cud_str=0;
static char *cursor_col=0;
static char *fore_string=0;
static char *back_string=0;
static char *unset=0;
static char *clear_str=0;
static char *clear_to=0;

static int initted=0;
static int cursor_line_offset=0;

static float disk_percent=100;
static float dma_percent=100;
static float *peak_dB=0;
static float *peak_hold_dB=0;
static float *rms_dB=0;
static int   *peak_clip=0;

static int dma_once_overrun=0;
static int disk_once_overrun=0;
static int hold = 0;

static char *bargraphs[]={
  "  -120|  -100|   -80|   -60|   -40|   -30|   -20|   -10|    -6|    -3|    +0",
  "  -100|   -80|   -60|   -40|   -30|   -20|   -10|    -6|    -3|    +0",
  "  -100|   -80|   -60|   -40|   -20|   -10|    -6|    -3|    +0",
  "   -80|   -60|   -40|   -20|   -10|    -6|    -3|    +0",
  "   -80|   -60|   -40|   -20|   -10|    -6|    +0",
  "   -60|   -40|   -20|   -10|    -6|    +0",
  "   -60|   -40|   -20|    -6|    +0",
  "   -60|   -20|    -6|    +0",
  "   -60|   -20|    +0",
  "   -20|    +0",
  "    +0",
  0
};
static int bar_locations[]={0,6,13,20,27,34,41,48,55,62,69,76};
static float barposts[][12]={
  {-150,-120,-100,-80,-60,-40,-30,-20,-10,-6,-3,0},
  {-140,-100,-80,-60,-40,-30,-20,-10,-6,-3,0},
  {-140,-100,-80,-60,-40,-20,-10,-6,-3,0},
  {-120,-80,-60,-40,-20,-10,-6,-3,0},
  {-120,-80,-60,-40,-20,-10,-6,0},
  {-120,-60,-40,-20,-10,-6,0},
  {-120,-60,-40,-20,-6,0},
  {-120,-60,-20,-6,0},
  {-120,-60,-20,-6,0},
  {-120,-60,-20,0},
  {-100,-20,0},
  {-100,0},
  {0}
};
static int barvals[]={11,10,9,8,7,6,5,4,3,2,1,0};

// relative-line cursor addressing; no idea where the cursor actually
// is absolutely */
static void cursor_to(int x, int y){
  int yoff = y - cursor_line_offset;
  
  while(yoff<0){
    putp(cup_str);
    cursor_line_offset--;
    yoff++;
  }
  while(yoff>0){
    putp(cud_str);
    cursor_line_offset++;
    yoff--;
  }

  putp(tparm(cursor_col,x));  
  fflush(stdout);

}

extern void _nc_init_acs(void);
static void enable_graphics(void){
  char *g = tigetstr("enacs");
  if(g && g != (char *)-1)
    putp(g);

  start_graphics=tigetstr("smacs");
  end_graphics=tigetstr("rmacs");
  _nc_init_acs();
}

static void hide_cursor(){
  char *g = tigetstr("civis");
  if(g) putp(g);
}

static void show_cursor(){
  char *g = tigetstr("cvvis");
  if(g) putp(g);
}

static void insert_lines(int n){
  int i;
  for(i=0;i<n-1;i++)
    printf("\r\n");
  cursor_line_offset=n-1;
}

static void setup_term_buf(void){
  /* If curses is being provided by ncurses, we don't actually have
     several of the 'canned' things like cbreak/noecho.  Do it by
     hand */
  if (cur_term != 0) {
    term = cur_term->Nttyb;
#ifdef TERMIOS
    term.c_lflag &= ~ICANON;
    term.c_iflag &= ~ICRNL;
    term.c_lflag &= ~(ECHO | ECHONL);
    term.c_iflag &= ~(ICRNL | INLCR | IGNCR);
    term.c_oflag &= ~(ONLCR);
    term.c_lflag |= ISIG;
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
#else
    term.sg_flags |= CBREAK;
    term.sg_flags &= ~(ECHO | CRMOD);
#endif
    SET_TTY(STDOUT_FILENO,&term);
    cur_term->Nttyb = term;
  }
}

static void resetup_term(void){
  SET_TTY(STDOUT_FILENO,&term);
  cur_term->Nttyb = term;
}

void terminal_init_panel(){

  if(!initted){
    /* canned curses/termcap setup.  We don't want full ncurses as we
       don't want to take over the whole terminal screen */
    
    GET_TTY(STDOUT_FILENO,&orig);
    setupterm(0,STDOUT_FILENO,0);
    setup_term_buf();
    enable_graphics();
    panel_lines = channels+4;
    
    cup_str = tigetstr("cuu1");
    cud_str = tigetstr("cud1");
    cursor_col = tigetstr("hpa");

    initted=1;

    if(!cup_str || cup_str==(char *)-1 ||
       !cud_str || cud_str==(char *)-1 ||
       !cursor_col || cursor_col==(char *)-1
       ){
      SET_TTY(STDOUT_FILENO,&orig);
      initted=0;
      fprintf(stderr,"Terminal provides insufficient cursor manipulation; rtrecord can run\n"
	      "in quiet mode (-q) only.\n");
      //exit(1);
    }

    hide_cursor();
    insert_lines(panel_lines);
    fflush(stdout);
  }
}

void terminal_remove_panel(){
  if(initted){
    char *delline = tigetstr("dl");
    
    if(delline && delline!=(char *)-1){
      cursor_to(0,0);
      putp(tparm(delline,panel_lines));
    }
    show_cursor();
    SET_TTY(STDOUT_FILENO,&orig);
    initted=0;
    fflush(stdout);
  }
}

static void clear_line(void){
  if(!clear_str){
    clear_str = tigetstr("el");
    if(!clear_str)clear_str=(char *)-1;
  }
  
  if(clear_str!=(char *)-1)
    putp(clear_str);
}

static void clear_to_cursor(void){
  if(!clear_to){
    clear_to = tigetstr("el1");
    if(!clear_to)clear_to=(char *)-1;
  }
  
  if(clear_to!=(char *)-1)
    putp(clear_to);
}

static void graphics_mode(void){
  if(start_graphics && start_graphics!=(char *)-1)
    putp(start_graphics);
}

static void text_mode(void){
  if(end_graphics && end_graphics!=(char *)-1)
    putp(end_graphics);
}

static int barpos(int barchoice, float dB){
  int i;
  if(dB < barposts[barchoice][0]) return 0;
  for(i=0;i<barvals[barchoice];i++)
    if(dB < barposts[barchoice][i+1]){
      float val0 = barposts[barchoice][i];
      float val1 = barposts[barchoice][i+1];
      return ((dB - val0) / (val1 - val0)) * 
	(bar_locations[i+1] - bar_locations[i]) +
	bar_locations[i] + 1;
    }
  
  return barposts[barchoice][i];
}

static void unset_attributes(){
  if(!unset){
    unset = tigetstr("sgr0");
    if(!unset)unset=(char *)-1;
  }
  if(unset!=(char *)-1)
    putp(unset);
}

static void fill(char *buf,char c,int cols){
  int i;
  for(i=0;i<cols;i++)
    buf[i]=c;
  buf[i]=0;
}

static void print_into(char *buf,int pos, char *s){
  int len = strlen(buf);
  int len2 = strlen(s);
  int i;
  for(i=0; i+pos<len && i<len2; i++)
    buf[i+pos]=s[i];
}

static void foreground(int c){
  if(!fore_string){
    fore_string = tigetstr("setaf");
    if(!fore_string)fore_string=(char *)-1;
  }
  if(fore_string!=(char *)-1)
    putp(tparm(fore_string,c));
}

static void background(int c){
  if(!back_string){
    back_string = tigetstr("setab");
    if(!back_string)back_string=(char *)-1;
  }
  if(back_string!=(char *)-1)
    putp(tparm(back_string,c));
}

static void color(int f,int b){
  if(f==-1 || b==-1)
    unset_attributes();
  if(f!=-1)
    foreground(f);
  if(b!=-1)
    background(b);
}

static void print_hline(char *s,int textcolor){
  int pos=0,last=0;

  while(s[pos]){
    /* draw line */
    while(s[pos] && s[pos]=='_')pos++;
    if(pos>last){
      if(start_graphics){
	graphics_mode();
	for(;last<pos;last++)
	  printf("%c",(int)ACS_HLINE);
	text_mode();
      }else{
	fwrite(s+last,1,pos-last,stdout);
      }
    }
    
    /* draw text */
    while(s[pos] && s[pos]!='_')pos++;
    if(pos>last){
      if(textcolor>=0)
	foreground(textcolor);
      fwrite(s+last,1,pos-last,stdout);
      if(textcolor>=0)
	unset_attributes();
      last = pos;
    }
  }
}

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
  
  /* determine which bargraph scale to use */
  while(barvals[i]){
    if((signed)strlen(bargraphs[i])<=barcols){
      barchoice = i;
      barlength = strlen(bargraphs[i]);
      break;
    }
    i++;
  }

  /* didn't find one short enough */
  if(barchoice==-1) barlength=0;

  /* determine actual spacing of bars */
  barcols = 25 + barlength;
  barpad = (columns-barcols)/2;

  /* determine per-channel feedback if disk thread has set some since
     last update */
    /* alloc structures if they haven't been alloced */
  if(!peak_dB){
    peak_dB=calloc(channels,sizeof(*peak_dB));
    for(i=0;i<channels;i++)
      peak_dB[i]=-150.;
  }
  if(!peak_hold_dB){
    peak_hold_dB=calloc(channels,sizeof(*peak_hold_dB));
    for(i=0;i<channels;i++)
      peak_hold_dB[i]=-150.;
  }
  if(!rms_dB){
    rms_dB=calloc(channels,sizeof(*rms_dB));
    for(i=0;i<channels;i++)
      rms_dB[i]=-150.;
  }
  if(!peak_clip)
    peak_clip=calloc(channels,sizeof(*peak_clip));
    
  if(disk_mark.bank!=disk_mark.read){
    /* we have data to refresh display */

    /* compute feedback */
    if(!hold){
      for(i=0;i<channels;i++){
	float peak = todB(pcm_data[!disk_mark.bank][i].peak *.00000000046566128730);
	if(peak>peak_dB[i])
	  peak_dB[i] = peak;
	else{
	  peak_dB[i] *= (1.-smooth);
	  peak_dB[i] += smooth* peak;
	}
	
	rms_dB[i] *= (1.-smooth);
	
	rms_dB[i] += smooth* todB(sqrt(pcm_data[!disk_mark.bank][i].weight_num/
				       pcm_data[!disk_mark.bank][i].weight_denom));
	if(pcm_data[!disk_mark.bank][i].pcm_clip)peak_clip[i]=1;
	if(peak_dB[i]>peak_hold_dB[i])peak_hold_dB[i] = peak_dB[i];
      }
    }

    disk_mark.read=disk_mark.bank;
  }

  /* determine buffer feedback if dma thread has set some since
     last update */
  if(dma_mark.bank!=dma_mark.read){
    if(dma_data[!dma_mark.bank].dmabuffer_overrun)
      dma_once_overrun=1;
    if(dma_data[!dma_mark.bank].diskbuffer_overrun)
      disk_once_overrun=1;

    dma_percent = (dma_data[!dma_mark.bank].dmabuffer_min*100./dmabuffer_frames);
    disk_percent = (dma_data[!dma_mark.bank].diskbuffer_min*100./diskbuffer_frames);

    dma_mark.read=dma_mark.bank;
  }

  /* construct the top line */
  snprintf(dmabuf,cols+1," DMA buffer: %3d%% ",(int)rint(dma_percent));
  snprintf(diskbuf,cols+1," Disk buffer: %3d%% ",(int)rint(disk_percent));
  snprintf(hwbuf,cols+1," %s ",device);
  fill(topline,'_',cols);
  print_into(topline,3,dmabuf);
  print_into(topline,5+strlen(dmabuf),diskbuf);
  print_into(topline,columns - strlen(hwbuf)-3,hwbuf);

  /* print the top line */
  cursor_to(0,0);
  print_hline(topline,-1);

  /* blank next line (useful if coming back after SIGSTOP) */
  cursor_to(0,1);
  clear_line();

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

#if 0
---- DMA buffer: 100% -- Disk buffer: 100% ----------------------------------------------- :1,0 -----
  
 1: [  -120|  -100|   -80|   -60|   -40|   -30|   -20|   -10|    -6|    -3|    0]  -70.0dB, -100.0dB 
 2: [                                                                          0]  -70.0dB, -100.0dB 
 3: [                                                                          0]  -70.0dB, -100.0dB 
 4: [                                                                          0]  -70.0dB, -100.0dB 
 5: [                                                                          0]  -70.0dB, -100.0dB 
 6: [                                                                          0]  -70.0dB, -100.0dB 

-----------------------------------------------------------------------------------------------------
#endif
  
