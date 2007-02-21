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

static void decide_text_inv(sushiv_panel_t *p){
  if(p->private->graph){
    Plot *plot = PLOT(p->private->graph);
    if(p->private->bg_type == SUSHIV_BG_WHITE)
      plot_set_bg_invert(plot,0);
    else
      plot_set_bg_invert(plot,1);
  }
}

static void recompute_if_running(sushiv_panel_t *p){
  if(p->private->realized && p->private->graph)
    _sushiv_panel_dirty_plot(p);
}

static void redraw_if_running(sushiv_panel_t *p){
  if(p->private->realized && p->private->graph){
    plot_draw_scales(PLOT(p->private->graph));
    _sushiv_panel_dirty_map(p);
    _sushiv_panel_dirty_legend(p);
  }
}

static void refg_if_running(sushiv_panel_t *p){
  if(p->private->realized && p->private->graph){
    plot_draw_scales(PLOT(p->private->graph));
    _sushiv_panel_dirty_legend(p);
  }
}

static void wrap_exit(sushiv_panel_t *dummy){
  _sushiv_clean_exit(SIGINT);
}

static void wrap_enter(sushiv_panel_t *p){
  plot_do_enter(PLOT(p->private->graph));
}

static void wrap_escape(sushiv_panel_t *p){
  plot_do_escape(PLOT(p->private->graph));
}

static int _sushiv_panel_background_i(sushiv_panel_t *p,
				      enum sushiv_background bg){
  
  sushiv_panel_internal_t *pi = p->private;
  
  pi->bg_type = bg;
  
  decide_text_inv(p);
  redraw_if_running(p);
  _sushiv_panel_update_menus(p);
  return 0;
}

static void white_bg(sushiv_panel_t *p){
  _sushiv_panel_background_i(p,SUSHIV_BG_WHITE);
}
static void black_bg(sushiv_panel_t *p){
  _sushiv_panel_background_i(p,SUSHIV_BG_BLACK);
}
static void checked_bg(sushiv_panel_t *p){
  _sushiv_panel_background_i(p,SUSHIV_BG_CHECKS);
}
static void black_text(sushiv_panel_t *p){
  plot_set_bg_invert(PLOT(p->private->graph),0);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void white_text(sushiv_panel_t *p){
  plot_set_bg_invert(PLOT(p->private->graph),1);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void grid_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),PLOT_GRID_NORMAL);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void tic_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),PLOT_GRID_TICS);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void no_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),0);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}

static menuitem *menu[]={
  &(menuitem){"Undo","[<i>bksp</i>]",NULL,&_sushiv_panel_undo_down},
  &(menuitem){"Redo","[<i>space</i>]",NULL,&_sushiv_panel_undo_up},

  &(menuitem){"",NULL,NULL,NULL},

  &(menuitem){"Start zoom box","[<i>enter</i>]",NULL,&wrap_enter},
  &(menuitem){"Clear readout","[<i>escape</i>]",NULL,&wrap_escape},

  &(menuitem){"",NULL,NULL,NULL},

  &(menuitem){"Background","...",NULL,NULL},
  &(menuitem){"Text color","...",NULL,NULL},
  &(menuitem){"Scales","...",NULL,NULL},

  &(menuitem){"",NULL,NULL,NULL},

  &(menuitem){"Quit","[<i>q</i>]",NULL,&wrap_exit},

  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_bg[]={
  &(menuitem){"<span foreground=\"white\">white</span>",NULL,NULL,&white_bg},
  &(menuitem){"<span foreground=\"black\">black</span>",NULL,NULL,&black_bg},
  &(menuitem){"checks",NULL,NULL,&checked_bg},
  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_text[]={
  &(menuitem){"<span foreground=\"black\">black</span>",NULL,NULL,&black_text},
  &(menuitem){"<span foreground=\"white\">white</span>",NULL,NULL,&white_text},
  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_scales[]={
  &(menuitem){"grid",NULL,NULL,grid_scale},
  &(menuitem){"tics",NULL,NULL,tic_scale},
  &(menuitem){"none",NULL,NULL,no_scale},
  &(menuitem){NULL,NULL,NULL,NULL}
};

void _sushiv_panel_update_menus(sushiv_panel_t *p){

  // is undo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_level){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),0),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),0),TRUE);
  }

  // is redo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level] ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level+1]){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),1),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),1),TRUE);
  }

  // are we starting or enacting a zoom box?
  if(p->private->oldbox_active){ 
    gtk_menu_alter_item_label(GTK_MENU(p->private->popmenu),3,"Zoom to box");
  }else{
    gtk_menu_alter_item_label(GTK_MENU(p->private->popmenu),3,"Start zoom box");
  }

  // make sure menu reflects plot configuration
  switch(p->private->bg_type){ 
  case SUSHIV_BG_WHITE:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),6,menu_bg[0]->left);
    break;
  case SUSHIV_BG_BLACK:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),6,menu_bg[1]->left);
    break;
  default:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),6,menu_bg[2]->left);
    break;
  }

  switch(PLOT(p->private->graph)->bg_inv){ 
  case 0:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),7,menu_text[0]->left);
    break;
  default:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),7,menu_text[1]->left);
    break;
  }

  switch(PLOT(p->private->graph)->grid_mode){ 
  case PLOT_GRID_NORMAL:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),8,menu_scales[0]->left);
    break;
  case PLOT_GRID_TICS:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),8,menu_scales[1]->left);
    break;
  default:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),8,menu_scales[2]->left);
    break;
  }
}

void _sushiv_realize_panel(sushiv_panel_t *p){
  if(!p->private->realized){
    p->private->realize(p);
    p->private->realized=1;

    // generic things that happen in all panel realizations...

    // text black or white in the plot?
    decide_text_inv(p);

    // panel right-click menus
    GtkWidget *bgmenu = gtk_menu_new_twocol(NULL,menu_bg,p);
    GtkWidget *textmenu = gtk_menu_new_twocol(NULL,menu_text,p);
    GtkWidget *scalemenu = gtk_menu_new_twocol(NULL,menu_scales,p);

    // not thread safe, we're not threading yet
    menu[6]->submenu = bgmenu;
    menu[7]->submenu = textmenu;
    menu[8]->submenu = scalemenu;

    p->private->popmenu = gtk_menu_new_twocol(p->private->toplevel, menu, p);
    _sushiv_panel_update_menus(p);

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

int sushiv_panel_oversample(sushiv_instance_t *s,
			    int number,
			    int numer,
			    int denom){
  
  if(number<0){
    fprintf(stderr,"sushiv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>s->panels || !s->panel_list[number]){
    fprintf(stderr,"sushiv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sushiv_panel_t *p = s->panel_list[number];
  sushiv_panel_internal_t *pi = p->private;

  if(denom == 0){
    fprintf(stderr,"sushiv_panel_oversample: A denominator of zero is invalid\n");
    return -EINVAL;
  }

  pi->oversample_n = numer;
  pi->oversample_d = denom;
  recompute_if_running(p);
  return 0;
}

void render_checks(ucolor *c, int w, int y){
  /* default checked background */
  /* 16x16 'mid-checks' */ 
  int x,j;
  
  int phase = (y>>4)&1;
  for(x=0;x<w;){
    u_int32_t phaseval = 0xff505050UL;
    if(phase) phaseval = 0xff808080UL;
    for(j=0;j<16 && x<w;j++,x++)
      c[x].u = phaseval;
    phase=!phase;
  }
}

int sushiv_panel_background(sushiv_instance_t *s,
			    int number,
			    enum sushiv_background bg){

  if(number<0){
    fprintf(stderr,"sushiv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>s->panels || !s->panel_list[number]){
    fprintf(stderr,"sushiv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sushiv_panel_t *p = s->panel_list[number];
  return _sushiv_panel_background_i(p,bg);
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

