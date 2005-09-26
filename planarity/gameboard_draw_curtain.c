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

/* cache the curtain surface/pattern ******************************/

#define CW 4
void cache_curtain(Gameboard *g){
  int x,y;
  cairo_t *c;
  g->curtains=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  CW,CW);
  
  c = cairo_create(g->curtains);
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);
      
  cairo_set_line_width(c,1);
  cairo_set_source_rgba (c, 0,0,0,.5);
  
  for(y=0;y<CW;y++){
    for(x=y&1;x<CW;x+=2){
      cairo_move_to(c,x+.5,y);
      cairo_rel_line_to(c,0,1);
    }
  }
  cairo_stroke(c);
  cairo_destroy(c);
  
  g->curtainp=cairo_pattern_create_for_surface (g->curtains);
  cairo_pattern_set_extend (g->curtainp, CAIRO_EXTEND_REPEAT);

}

void draw_curtain(Gameboard *g){
  
  cairo_t *c = cairo_create(g->background);
  
  cairo_set_source (c, g->curtainp);
  cairo_paint(c);
  cairo_destroy(c);
}  
