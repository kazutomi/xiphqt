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

/* invalidate edge region for efficient expose *******************/

void invalidate_edges(GtkWidget *widget, vertex *v){
  GdkRectangle r;
  
  if(v){
    edge_list *el=v->edges;
    while (el){
      edge *e=el->edge;
      r.x = min(e->A->x,e->B->x) - E_LINE;
      r.y = min(e->A->y,e->B->y) - E_LINE;
      r.width  = labs(e->B->x - e->A->x) + 1 + E_LINE*2;
      r.height = labs(e->B->y - e->A->y) + 1 + E_LINE*2;
      gdk_window_invalidate_rect (widget->window, &r, FALSE);
      el=el->next;
    }
  }
}

