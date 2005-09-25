#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"

void draw_buttonbar_box (Gameboard *g){
  cairo_t *c = cairo_create(g->forebutton);
  
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);

  bottombox(g,c,g->g.width,SCOREHEIGHT);
  cairo_destroy(c);
}

