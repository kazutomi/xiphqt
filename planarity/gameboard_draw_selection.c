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
