/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
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
#include "sushivision.h"
#include "mapping.h"
#include "plot.h"
#include "slice.h"
#include "slider.h"
#include "panel-2d.h"
#include "internal.h"

void _sushiv_realize_panel(sushiv_panel_t *p){
  if(!p->realized){
    switch(p->type){
    case SUSHIV_PANEL_1D:
      //_sushiv_realize_panel1d(p);
      break;
    case SUSHIV_PANEL_2D:
      _sushiv_realize_panel2d(p);
      break;
    }
    p->realized=1;
  }
}

static void _sushiv_panel_map_redraw(sushiv_panel_t *p){
  if(p->maps_dirty){
    p->maps_dirty = 0;
    switch(p->type){
    case SUSHIV_PANEL_1D:
      //_sushiv_panel1d_map_redraw(p);
      break;
    case SUSHIV_PANEL_2D:
      _sushiv_panel2d_map_redraw(p);
      break;
    }
  }
}

static void _sushiv_panel_legend_redraw(sushiv_panel_t *p){
  if(p->legend_dirty){
    p->legend_dirty = 0;
    switch(p->type){
    case SUSHIV_PANEL_1D:
      //_sushiv_panel1d_legend_redraw(p);
      break;
    case SUSHIV_PANEL_2D:
      _sushiv_panel2d_legend_redraw(p);
      break;
    }
  }
}

int _sushiv_panel_cooperative_compute(sushiv_panel_t *p){
  if(p->realized){
    if(p->type == SUSHIV_PANEL_2D)
      return _sushiv_panel_cooperative_compute_2d(p);
  }
  return 0;
}

/* doesn't take an unbounded period, but shouldn't be
   synchronous in the interests of responsiveness. */
static gboolean _map_idle_work(gpointer ptr){
  sushiv_instance_t *s = (sushiv_instance_t *)ptr;
  int i;
  
  for(i=0;i<s->panels;i++)
    _sushiv_panel_map_redraw(s->panel_list[i]);
  
  return FALSE;
}

static gboolean _legend_idle_work(gpointer ptr){
  sushiv_instance_t *s = (sushiv_instance_t *)ptr;
  int i;
  
  for(i=0;i<s->panels;i++)
    _sushiv_panel_legend_redraw(s->panel_list[i]);
  
  return FALSE;
}


void _sushiv_panel_dirty_map(sushiv_panel_t *p){
  p->maps_dirty = 1;
  g_idle_add(_map_idle_work,p->sushi);
}

void _sushiv_panel_dirty_legend(sushiv_panel_t *p){
  p->legend_dirty = 1;
  g_idle_add(_legend_idle_work,p->sushi);
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

