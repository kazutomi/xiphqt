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
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include "internal.h"

extern void _sushiv_wake_workers(void);

void _sushiv_realize_panel(sushiv_panel_t *p){
  if(!p->private->realized){
    p->private->realize(p);
    p->private->realized=1;
  }
}

void set_map_throttle_time(sushiv_panel_t *p){
  struct timeval now;
  gettimeofday(&now,NULL);

  p->private->last_map_throttle = now.tv_sec*1000 + now.tv_usec/1000;
}

static int test_throttle_time(sushiv_panel_t *p){
  struct timeval now;
  long test;
  gettimeofday(&now,NULL);
  
  test = now.tv_sec*1000 + now.tv_usec/1000;
  if(p->private->last_map_throttle + 500 < test) 
    return 1;

  return 0;  
}

// the following is slightly odd; we want map and legend updates to
// fire when the UI is otherwise idle (only good way to do event
// compression in gtk), but we don't want it processed in the main UI
// thread because of render latencies.  Thus we have a map/legend
// chain register an idle handler that then wakes the worker threads
// and has them render the map/legend changes

static gboolean _idle_map_fire(gpointer ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  gdk_threads_enter ();
  p->private->map_active = 1;
  p->private->map_serialno++;
  p->private->map_progress_count=0;
  p->private->map_complete_count=0;
  gdk_threads_leave ();
  _sushiv_wake_workers();
  return FALSE;
}

static gboolean _idle_legend_fire(gpointer ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  gdk_threads_enter ();
  p->private->legend_active = 1;
  p->private->legend_serialno++;
  p->private->legend_progress_count=0;
  p->private->legend_complete_count=0;
  gdk_threads_leave ();
  _sushiv_wake_workers();
  return FALSE;
}

/* use these to request a render/compute action.  Do panel
   subtype-specific setup, then wake workers with one of the below */
void _sushiv_panel_dirty_plot(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->plot_active = 1;
  p->private->plot_serialno++;
  p->private->plot_progress_count=0;
  p->private->plot_complete_count=0;
  gdk_threads_leave ();
  _sushiv_wake_workers();
}

void _sushiv_panel_dirty_map(sushiv_panel_t *p){
  gdk_threads_enter ();
  g_idle_add(_idle_map_fire,p);
  gdk_threads_leave ();
}

void _sushiv_panel_dirty_map_immediate(sushiv_panel_t *p){
  _idle_map_fire(p);
}

void _sushiv_panel_dirty_map_throttled(sushiv_panel_t *p){
  gdk_threads_enter ();
  if(!p->private->map_active && test_throttle_time(p)){
     _idle_map_fire(p);
  }
  gdk_threads_leave ();
}

void _sushiv_panel_dirty_legend(sushiv_panel_t *p){
  gdk_threads_enter ();
  g_idle_add(_idle_legend_fire,p);
  gdk_threads_leave ();
}

/* use these to signal a computation is completed */
void _sushiv_panel_clean_plot(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->plot_active = 0;
  gdk_threads_leave ();
}

void _sushiv_panel_clean_map(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->map_active = 0;
  gdk_threads_leave ();
}

void _sushiv_panel_clean_legend(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->legend_active = 0;
  gdk_threads_leave ();
}

extern int sushiv_panel_oversample(sushiv_instance_t *s,
				   int number,
				   int numer,
				   int denom){

  sushiv_panel_t *p = s->panel_list[number];
  sushiv_panel_internal_t *pi = p->private;

  if(denom == 0)return -EINVAL;

  pi->oversample_n = numer;
  pi->oversample_d = denom;
  return 0;
}

int _sushiv_new_panel(sushiv_instance_t *s,
		      int number,
		      const char *name, 
		      int *objectives,
		      int *dimensions,
		      unsigned flags){
  
  sushiv_panel_t *p;
  int i;

  if(number<0){
    fprintf(stderr,"Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number<s->panels){
    if(s->panel_list[number]!=NULL){
      fprintf(stderr,"Panel number %d already exists\n",number);
      return -EINVAL;
    }
  }else{
    if(s->panels == 0){
      s->panel_list = calloc (number+1,sizeof(*s->panel_list));
    }else{
      s->panel_list = realloc (s->panel_list,(number+1) * sizeof(*s->panel_list));
      memset(s->panel_list + s->panels, 0, sizeof(*s->panel_list)*(number+1 - s->panels));
    }
    s->panels = number+1; 
  }

  p = s->panel_list[number] = calloc(1, sizeof(**s->panel_list));

  p->number = number;
  p->name = strdup(name);
  p->flags = flags;
  p->sushi = s;
  p->private = calloc(1, sizeof(*p->private));
  p->private->spinner = spinner_new();
  p->private->oversample_n = 1;
  p->private->oversample_d = 1;

  i=0;
  while(objectives && objectives[i]>=0)i++;
  p->objectives = i;
  p->objective_list = malloc(i*sizeof(*p->objective_list));
  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = s->objective_list[objectives[i]];
    p->objective_list[i].o = o;
    p->objective_list[i].p = p;
  }

  i=0;
  while(dimensions && dimensions[i]>=0)i++;
  p->dimensions = i;
  p->dimension_list = malloc(i*sizeof(*p->dimension_list));
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = s->dimension_list[dimensions[i]];
    p->dimension_list[i].d = d;
    p->dimension_list[i].p = p;
  }

  return number;
}

