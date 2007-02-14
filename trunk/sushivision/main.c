/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include <pthread.h>
#include <dlfcn.h>
#include "internal.h"
#include "sushi-gtkrc.h"

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mc = PTHREAD_COND_INITIALIZER;
sig_atomic_t _sushiv_exiting=0;
static int wake_pending = 0;
static int num_threads;

static int instances=0;
static sushiv_instance_t **instance_list;

void _sushiv_wake_workers(){
  if(instances){
    pthread_mutex_lock(&m);
    wake_pending = num_threads;
    pthread_cond_broadcast(&mc);
    pthread_mutex_unlock(&m);
  }
}

void _sushiv_clean_exit(int sig){
  _sushiv_exiting = 1;
  _sushiv_wake_workers();

  //signal(sig,SIG_IGN);
  if(sig!=SIGINT)
    fprintf(stderr,
            "\nTrapped signal %d; exiting!\n",sig);

  gtk_main_quit();
}

static int num_proccies(){
  FILE *f = fopen("/proc/cpuinfo","r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int num=1,arg;

  if (f == NULL) return 1;
  while ((read = getline(&line, &len, f)) != -1) {
    if(sscanf(line,"processor : %d",&arg)==1)
      if(arg+1>num)num=arg+1;
  }
  if (line)
    free(line);

  fprintf(stderr,"Number of processors: %d\n",num);
  fclose(f);
  return num;
}

static void *worker_thread(void *dummy){
  /* set up temporary working space for function rendering; this saves
     continuously recreating it in the loop below */
  _sushiv_compute_cache **c; // [instance][panel]
  int i,j;
  
  c = calloc(instances,sizeof(*c));
  for(j=0;j<instances;j++){
    sushiv_instance_t *s = instance_list[j];
    c[j] = calloc(s->panels,sizeof(**c));
  }

  while(1){
    if(_sushiv_exiting)break;
    
    // look for work
    {
      int flag=0;
      // by instance
      for(j=0;j<instances;j++){
	sushiv_instance_t *s = instance_list[j];
				 
	for(i=0;i<s->panels;i++){
	  sushiv_panel_t *p = s->panel_list[i];

	  if(_sushiv_exiting)break;

	  // pending remap work?
	  gdk_threads_enter();
	  if(p->private->realized && p->private->graph){

	    if(p->private->map_active){
	      plot_set_busy(PLOT(p->private->graph));
	      flag |= p->private->map_action(p); // may drop lock internally
	      if(!p->private->map_active)
		set_map_throttle_time(p);
	    }
	    
	    // pending legend work?
	    if(p->private->legend_active){
	      plot_set_busy(PLOT(p->private->graph));
	      flag |= p->private->legend_action(p); // may drop lock internally
	    }
	    
	    // pending computation work?
	    if(p->private->plot_active){
	      plot_set_busy(PLOT(p->private->graph));
	      flag |= p->private->compute_action(p,&c[j][i]); // may drop lock internally
	    }
	    
	    if(!p->private->plot_active &&
	       !p->private->legend_active &&
	       !p->private->map_active)
	      plot_set_idle(PLOT(p->private->graph));
	  }
	  gdk_threads_leave ();
	}
      }
      if(flag==1)continue;
    }
    
    // nothing to do, wait
    pthread_mutex_lock(&m);
    if(!wake_pending)
      pthread_cond_wait(&mc,&m);
    else
      wake_pending--;
    pthread_mutex_unlock(&m);
  }
  
  pthread_mutex_unlock(&m);
  return 0;
}

static char * gtkrc_string(){
  return _SUSHI_GTKRC_STRING;
}

static void sushiv_realize_instance(sushiv_instance_t *s){
  int i;
  for(i=0;i<s->panels;i++)
    _sushiv_realize_panel(s->panel_list[i]);
  for(i=0;i<s->panels;i++)
    s->panel_list[i]->private->request_compute(s->panel_list[i]);
}

static void sushiv_realize_all(void){
  int i;
  for(i=0;i<instances;i++)
    sushiv_realize_instance(instance_list[i]);
}

/* externally visible interface */

sushiv_instance_t *sushiv_new_instance(void) {
  sushiv_instance_t *ret=calloc(1,sizeof(*ret));
  ret->private = calloc(1,sizeof(*ret->private));

  if(instances){
    instance_list = realloc(instance_list,(instances+1)*sizeof(*instance_list));
  }else{
    instance_list = malloc((instances+1)*sizeof(*instance_list));
  }
  instance_list[instances] = ret;
  instances++;
  
  return ret;
}

int main (int argc, char *argv[]){
  int ret;

  num_threads = num_proccies();

  gtk_init (&argc, &argv);
  g_thread_init (NULL);

  gtk_mutex_fixup();
  gdk_threads_init ();
  gtk_rc_parse_string(gtkrc_string());
  gtk_rc_add_default_file("sushi-gtkrc");

  ret = sushiv_submain(argc,argv);
  if(ret)return ret;
  
  sushiv_realize_all();
  
  {
    pthread_t dummy;
    int threads = num_threads;
    while(threads--)
      pthread_create(&dummy, NULL, &worker_thread,NULL);
  }

  signal(SIGINT,_sushiv_clean_exit);
  //signal(SIGSEGV,_sushiv_clean_exit);

  gtk_button3_fixup();
  gtk_main ();
  
  {
    int (*optional_exit)(void) = dlsym(RTLD_DEFAULT, "sushiv_atexit");
    if(optional_exit)
      return optional_exit();
  }

  return 0;
}

