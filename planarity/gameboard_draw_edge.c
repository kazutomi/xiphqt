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


/******** draw edges ********************************************/

void setup_background_edge(cairo_t *c){
  cairo_set_line_width(c,E_LINE);
  cairo_set_source_rgba(c,E_LINE_B_COLOR);
}

void setup_foreground_edge(cairo_t *c){
  cairo_set_line_width(c,E_LINE);
  cairo_set_source_rgba(c,E_LINE_F_COLOR);
}

void draw_edge(cairo_t *c,edge *e){
  cairo_move_to(c,e->A->x,e->A->y);
  cairo_line_to(c,e->B->x,e->B->y);
}

void finish_edge(cairo_t *c){
  cairo_stroke(c);
}

void draw_edges(cairo_t *c, vertex *v, int offx, int offy){  
  if(v){
    edge_list *el=v->edges;
    while (el){
      edge *e=el->edge;

      if(e->A->grabbed==0 || e->B->grabbed==0 || v==e->A){
	if(e->A->grabbed)
	  cairo_move_to(c,e->A->x+offx,e->A->y+offy);
	else
	  cairo_move_to(c,e->A->x,e->A->y);
	
	if(e->B->grabbed)
	  cairo_line_to(c,e->B->x+offx,e->B->y+offy);
	else
	  cairo_line_to(c,e->B->x,e->B->y);
      }
      el=el->next;
    }
  }
}

/* invalidate edge region for efficient expose *******************/

void invalidate_edges(GtkWidget *widget, vertex *v, int offx, int offy){
  GdkRectangle r;
  
  if(v){
    edge_list *el=v->edges;
    while (el){
      edge *e=el->edge;

      if(e->A->grabbed==0 || e->B->grabbed==0 || v==e->A){
	int Ax = e->A->x + (e->A->grabbed?offx:0);
	int Ay = e->A->y + (e->A->grabbed?offy:0);
	int Bx = e->B->x + (e->B->grabbed?offx:0);
	int By = e->B->y + (e->B->grabbed?offy:0);
	
	r.x = min(Ax,Bx) - E_LINE;
	r.y = min(Ay,By) - E_LINE;
	r.width  = labs(Bx - Ax) + 1 + E_LINE*2;
	r.height = labs(By - Ay) + 1 + E_LINE*2;
	
	gdk_window_invalidate_rect (widget->window, &r, FALSE);
      }
      el=el->next;
    }
  }
}

