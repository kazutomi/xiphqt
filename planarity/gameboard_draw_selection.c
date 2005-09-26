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


// draw selection box 
void draw_selection_rectangle(Gameboard *g,cairo_t *c){
  cairo_set_source_rgba(c,SELECTBOX_COLOR);
  cairo_rectangle(c,g->selectionx,
		  g->selectiony,
		  g->selectionw,
		  g->selectionh);
  cairo_fill(c);
}

// invalidate the selection box plus enough area to catch any verticies
void invalidate_selection(GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  GdkRectangle r;
  r.x = g->selectionx - (V_RADIUS + V_LINE)*2;
  r.y = g->selectiony - (V_RADIUS + V_LINE)*2;
  r.width =  g->selectionw + (V_RADIUS + V_LINE)*4;
  r.height = g->selectionh + (V_RADIUS + V_LINE)*4;
  
  gdk_window_invalidate_rect (widget->window, &r, FALSE);
}

// invalidate the selection box plus enough area to catch any verticies
void invalidate_verticies_selection(GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  vertex *v=g->g.verticies;
  while(v){
    if(v->selected)
      invalidate_vertex_off(widget,v,g->dragx,g->dragy);
    v=v->next;
  }
}
