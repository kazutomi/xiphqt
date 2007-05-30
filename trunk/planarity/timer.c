/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
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

#include <time.h>
#include "timer.h"
#include "gettext.h"

static int paused;
static time_t begin_time_add;
static time_t begin_time;
static char timebuffer[160];

time_t get_timer(){
  if(paused)
    return begin_time_add;
  else{
    time_t ret = time(NULL);
    return ret - begin_time + begin_time_add;
  }
}

char *get_timer_string(){
  int  ho = get_timer() / 3600;
  int  mi = get_timer() / 60 - ho*60;
  int  se = get_timer() - ho*3600 - mi*60;
  
  if(ho){
    snprintf(timebuffer,160,_("%d:%02d:%02d"),ho,mi,se);
  }else if (mi){
    snprintf(timebuffer,160,_("%d:%02d"),mi,se);
  }else{
    snprintf(timebuffer,160,_("%d seconds"),se);
  }
  
  return timebuffer;
}

void set_timer(time_t off){
  begin_time_add = off;
  begin_time = time(NULL);
}

void pause_timer(){
  begin_time_add = get_timer();
  paused=1;
}

void unpause_timer(){
  paused=0;
  set_timer(begin_time_add);
}

int timer_paused_p(){
  return paused;
}

