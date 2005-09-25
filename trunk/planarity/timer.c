#include <time.h>
#include "timer.h"

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
    snprintf(timebuffer,160,"%d:%02d:%02d",ho,mi,se);
  }else if (mi){
    snprintf(timebuffer,160,"%d:%02d",mi,se);
  }else{
    snprintf(timebuffer,160,"%d seconds",se);
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

