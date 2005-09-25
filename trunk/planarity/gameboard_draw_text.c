#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"

GdkRectangle render_text_centered(cairo_t *c, char *s, int x, int y){
  cairo_text_extents_t ex;
  GdkRectangle r;

  cairo_text_extents (c, s, &ex);

  r.x=x-(ex.width/2)-ex.x_bearing;
  r.y=y-(ex.height/2)-ex.y_bearing;
  r.width=ex.width;
  r.height=ex.height;

  cairo_move_to (c, r.x, r.y);
  cairo_show_text (c, s);  

  return r;
}

GdkRectangle render_border_centered(cairo_t *c, char *s, int x, int y){
  cairo_text_extents_t ex;
  GdkRectangle r;

  cairo_text_extents (c, s, &ex);

  r.x=x-(ex.width/2)-ex.x_bearing-2;
  r.y=y-(ex.height/2)-ex.y_bearing-2;
  r.width=ex.width+5;
  r.height=ex.height+5;

  cairo_save(c);
  cairo_move_to (c, r.x+2, r.y+2 );  
  cairo_set_line_width(c,3);
  cairo_set_source_rgba(c,1,1,1,.9);
  cairo_text_path (c, s);  
  cairo_stroke(c);
  cairo_restore(c);

  return r;
}

GdkRectangle render_bordertext_centered(cairo_t *c, char *s, int x, int y){
  GdkRectangle r = render_border_centered(c,s,x,y);
  render_text_centered(c,s,x,y);
  return r;
}
