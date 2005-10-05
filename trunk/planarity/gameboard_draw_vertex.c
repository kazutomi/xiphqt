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

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "graph.h"
#include "gameboard.h"

/* draw/cache vertex surfaces; direct surface copies are faster than
   always redrawing lots of circles */

void draw_vertex(cairo_t *c,vertex *v,cairo_surface_t *s){
  cairo_set_source_surface(c,
			   s,
			   v->x-V_LINE-V_RADIUS,
			   v->y-V_LINE-V_RADIUS);
  cairo_paint(c);
}      

void draw_vertex_with_alpha(cairo_t *c,vertex *v,cairo_surface_t *s,float alpha){
  cairo_set_source_surface(c,
			   s,
			   v->x-V_LINE-V_RADIUS,
			   v->y-V_LINE-V_RADIUS);
  cairo_paint_with_alpha(c,alpha);
}      

// normal unlit vertex
cairo_surface_t *cache_vertex(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_IDLE_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}

// selected vertex
cairo_surface_t *cache_vertex_sel(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS*.5,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_IDLE_COLOR);
  cairo_fill(c);

  cairo_destroy(c);
  return ret;
}

// grabbed vertex
cairo_surface_t *cache_vertex_grabbed(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS*.5,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_ADJ_COLOR);
  cairo_fill(c);

  cairo_destroy(c);
  return ret;
}

// vertex under mouse rollover
cairo_surface_t *cache_vertex_lit(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}

// verticies attached to grabbed vertex
cairo_surface_t *cache_vertex_attached(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_ADJ_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}

// vertex being dragged in a group
cairo_surface_t *cache_vertex_ghost(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS*.5,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_ADJ_COLOR);
  cairo_fill(c);

  cairo_destroy(c);
  return ret;
}


/* region invalidation operations; do exposes efficiently! **********/

// invalidate the box around a single offset vertex
void invalidate_vertex_off(GtkWidget *widget, 
					 vertex *v, int dx, int dy){
  if(v){
    GdkRectangle r;
    r.x = v->x - V_RADIUS - V_LINE + dx;
    r.y = v->y - V_RADIUS - V_LINE + dy;
    r.width =  (V_RADIUS + V_LINE)*2;
    r.height = (V_RADIUS + V_LINE)*2;
    
    gdk_window_invalidate_rect (widget->window, &r, FALSE);
  }
}

// invalidate the box around a single vertex
void invalidate_vertex(Gameboard *g, vertex *v){
  invalidate_vertex_off(&g->w,v,0,0);
}

// invalidate a vertex and any other attached verticies
void invalidate_attached(GtkWidget *widget, vertex *v){
  if(v){
    edge_list *el=v->edges;
    while (el){
      edge *e=el->edge;
      if(e->A != v)invalidate_vertex(GAMEBOARD(widget),e->A);
      if(e->B != v)invalidate_vertex(GAMEBOARD(widget),e->B);
      el=el->next;
    }
    invalidate_vertex(GAMEBOARD(widget),v);
  }
}

