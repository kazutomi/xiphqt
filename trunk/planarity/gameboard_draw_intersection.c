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

static void draw_intersection(cairo_t *c,double x, double y){
  cairo_move_to(c,x-INTERSECTION_RADIUS,y);
  cairo_rel_line_to(c,INTERSECTION_RADIUS,-INTERSECTION_RADIUS);
  cairo_rel_line_to(c,INTERSECTION_RADIUS,INTERSECTION_RADIUS);
  cairo_rel_line_to(c,-INTERSECTION_RADIUS,INTERSECTION_RADIUS);
  cairo_close_path(c);
}

static void draw_many_intersection(Gameboard *g,cairo_t *c){
  cairo_matrix_t ma;
  int x2 = g->g.width/2;
  int y2 = g->g.height/2;
  int r = INTERSECTION_RADIUS*10;

  cairo_set_source_rgba (c, INTERSECTION_COLOR);
  cairo_set_line_width(c, INTERSECTION_LINE_WIDTH*10);

  cairo_move_to(c,x2-r,y2-r/4);
  cairo_rel_line_to(c,r,-r);
  cairo_rel_line_to(c,r,r);
  cairo_rel_line_to(c,-r,r);
  cairo_close_path(c);

  cairo_stroke(c);

  cairo_select_font_face (c, "Sans",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
  
  cairo_matrix_init_scale (&ma, 30.,34.);
  cairo_set_font_matrix (c,&ma);
  render_bordertext_centered(c,"rather many, really",x2,y2+r/4);

}

void draw_intersections(Gameboard *b, graph *g, cairo_t *c,
			int x,int y,int w,int h){
  
  double xx=x-(INTERSECTION_LINE_WIDTH*.5 + INTERSECTION_RADIUS);
  double x2=w+x+(INTERSECTION_LINE_WIDTH + INTERSECTION_RADIUS*2);
  double yy=y-(INTERSECTION_LINE_WIDTH*.5 + INTERSECTION_RADIUS);
  double y2=h+y+(INTERSECTION_LINE_WIDTH + INTERSECTION_RADIUS*2);
  
  if(g->active_intersections > 1000){
    // don't draw them all; potential for accidental CPU sink.
    draw_many_intersection(b,c);
  }else{
    cairo_set_source_rgba (c, INTERSECTION_COLOR);
    cairo_set_line_width(c, INTERSECTION_LINE_WIDTH);
    
    // walk the intersection list of all edges; draw if paired is a higher pointer
    edge *e=g->edges;
    while(e){
      intersection *i = e->i.next;
      while(i){
	if(i->paired > i){
	  double ix=i->x, iy=i->y;
	  
	  if(ix >= xx && ix <= x2 && iy >= yy && iy <= y2)
	    draw_intersection(c,ix,iy);
	}
	cairo_stroke(c); // incremental stroke!  It's easy to run
	// the path longer than the X server
	// allows.
	i=i->next;
      }
      e=e->next;
    }
  }
}
