#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern time_t get_timer();
extern char *get_timer_string();
extern void set_timer(time_t off);
extern void pause_timer();
extern void unpause_timer();
extern int timer_paused_p();

