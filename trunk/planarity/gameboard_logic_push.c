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

void update_push(Gameboard *g, cairo_t *c){

  if(g->pushed_curtain){
    int w = g->g.width;
    int h = g->g.height;

    draw_foreground(g,c,0,0,w,h);
    
    // copy in the score and button surfaces
    cairo_set_source_surface(c,g->forescore,0,0);
    cairo_rectangle(c, 0,0,w,
		    min(SCOREHEIGHT,h));
    cairo_fill(c);
    
    cairo_set_source_surface(c,g->forebutton,0,h-SCOREHEIGHT);
    cairo_rectangle(c, 0,0,w,h);
    cairo_fill(c);
    
    if(g->show_intersections)
      draw_intersections(g,&g->g,c,0,0,w,h);

    cairo_set_source (c, g->curtainp);
    cairo_paint(c);

    if(g->redraw_callback)g->redraw_callback(g);
  }
}

void push_curtain(Gameboard *g,void(*redraw_callback)(Gameboard *g)){
  if(!g->pushed_curtain){ 
    cairo_t *c = cairo_create(g->background);
    g->pushed_curtain=1;    
    g->redraw_callback=redraw_callback;

    update_push(g,c);

    cairo_destroy(c);
    expose_full(g);

    //gameboard_draw(g,0,0,w,h);
  }
}

void pop_curtain(Gameboard *g){
  if(g->pushed_curtain){
    g->pushed_curtain=0;
    update_full(g);
  }
}
