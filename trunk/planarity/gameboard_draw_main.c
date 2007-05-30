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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "graph.h"
#include "gameboard.h"
#include "dialog_level.h"
#include "main.h"

/* The gPlanarity gameboard consists of a background and a foreground.
   The background is a predrawn backdrop of anything not currently
   being animated.  The foreground is 'active' elements of the UI and
   anything that floats above them.  There are also two translucent
   bars, top and bottom, that hold the score and buttonbar
   respectively.  These are prerendered surfaces that are blitted onto
   the foreground during exposes */

void draw_foreground(Gameboard *g,cairo_t *c,int x,int y,int w,int h){

  /* Edges attached to the grabbed vertex are drawn here */

  if(g->grabbed_vertex && !g->hide_lines){
    edge_list *el=g->grabbed_vertex->edges;
    setup_foreground_edge(c);
    while(el){
      edge *e=el->edge;
      /* no need to check rectangle; if they're to be drawn, they'll
	 always be in the rect due to expose combining */
      draw_edge(c,e);
      el=el->next;
    }
    finish_edge(c);
  }

  if(g->group_drag){
    vertex *v = g->g.verticies;
    setup_foreground_edge(c);

    while(v){
      if(v->grabbed){
	/* no need to check rectangle; if they're to be drawn, they'll
	   always be in the rect due to expose combining */
	draw_edges(c,v,g->dragx,g->dragy);
      }
      v=v->next;
    }
    finish_edge(c);
  }
  
  /* verticies drawn over the edges */
  {
    vertex *v = g->g.verticies;
    float alpha = 1.*g->fade.count/FADE_FRAMES;

    int clipx = x-V_RADIUS;
    int clipw = x+w+V_RADIUS;
    int clipy = y-V_RADIUS;
    int cliph = y+h+V_RADIUS;
    
    while(v){

      if(v->grabbed && g->group_drag){
	vertex tv;
	tv.x=v->x+g->dragx;
	tv.y=v->y+g->dragy;
	/* is the vertex in the expose rectangle? */
	if(tv.x>=clipx && tv.x<=clipw &&
	   tv.y>=clipy && tv.y<=cliph){
	  
	  draw_vertex(c,&tv,g->vertex_ghost);
	}
      }else{ 
	/* is the vertex in the expose rectangle? */
	if(v->x>=clipx && v->x<=clipw &&
	   v->y>=clipy && v->y<=cliph){
	  
	  if(v == g->grabbed_vertex && !g->group_drag) {
	    draw_vertex(c,v,g->vertex_grabbed);      
	  } else if( v->selected ){
	    draw_vertex(c,v,g->vertex_sel);
	    if(v->fading)
	      draw_vertex_with_alpha(c,v,g->vertex_ghost,alpha);
	  } else if ( v == g->lit_vertex){
	    draw_vertex(c,v,g->vertex_lit);
	  } else if (v->attached_to_grabbed){
	    draw_vertex(c,v,g->vertex_attached);
	  }else{
	    draw_vertex(c,v,g->vertex);
	    if(v->fading)
	      draw_vertex_with_alpha(c,v,g->vertex_attached,alpha);
	  }
	}
      }
      
      v=v->next;
    }
  }

  if(g->selection_grab)
    draw_selection_rectangle(g,c);
}

static void draw_background(Gameboard *g,cairo_t *c){
  edge *e=g->g.edges;

  cairo_set_source_rgb(c,1,1,1);
  cairo_paint(c);
  
  if(!g->hide_lines){
    setup_background_edge(c);
    while(e){
      if(e->active)
	draw_edge(c,e);
      e=e->next;
    }
    finish_edge(c);
  }
}

static void draw_background_realpart(Gameboard *g,cairo_t *c,
				     int x, int y, int w, int h){
  edge *e=g->g.edges;
  int x2 = x+w-1+E_LINE;
  int y2 = y+h-1+E_LINE;
  x-=E_LINE;
  y-=E_LINE;

  cairo_set_source_rgb(c,1,1,1);
  cairo_paint(c);
  
  if(!g->hide_lines){
    setup_background_edge(c);
    while(e){
      int ex1 = e->A->x;
      int ex2 = e->B->x;
      int ey1 = e->A->y;
      int ey2 = e->B->y;

      if(ex1<ex2){
	if(x>ex2 || x2<ex1){e=e->next;continue;}
      }else{
	if(x>ex1 || x2<ex2){e=e->next;continue;}
      }

      if(ey1<ey2){
	if(y>ey2 || y2<ey1){e=e->next;continue;}
      }else{
	if(y>ey1 || y2<ey2){e=e->next;continue;}
      }

      draw_edge(c,e);

      e=e->next;
    }
    finish_edge(c);
  }
}


/* Several very-high-level drawing sequences triggered by various
   actions in the game *********************************************/

/* request that the background plane be redrawn from scratch to match
   current game state and the full window invalidated to request an
   expose.  As other layers are always drawn on the fly, this is
   effectively a full refresh of the screen.
   This is done as sparingly as possible and only within large state
   changes as it's not fast enough for animation. */

void update_full(Gameboard *g){
  cairo_t *c = cairo_create(g->background);

  g->delayed_background=0;

  // render the far background plane
  draw_background(g,c);
  update_push(g,c);
  cairo_destroy(c);
  expose_full(g);
}

void expose_full(Gameboard *g){
  GtkWidget *widget = GTK_WIDGET(g);
  GdkRectangle r;
  
  r.x=0;
  r.y=0;
  r.width=g->g.width;
  r.height=g->g.height;

  gdk_window_invalidate_rect (widget->window, &r, FALSE);
    
}

void update_draw(Gameboard *g){
  cairo_t *c = cairo_create(g->background);

  g->delayed_background=0;

  // render the far background plane
  draw_background(g,c);
  update_push(g,c);
  cairo_destroy(c);
  
  gameboard_draw(g,0,0,g->g.width,g->g.height);
}

/* request a full redraw update to be processed immediately *after*
   the next expose.  This is used to increase apparent interactivity
   of a vertex 'grab' where the background must be redrawn, but the
   vertex itself should react immediately */
void update_full_delayed(Gameboard *g){
  g->delayed_background=1;
}

/* specialized update request for when re-adding a grabbed vertex's
   edges to the background.  Doesn't require a full redraw. */
void update_add_vertex(Gameboard *g, vertex *v){
  GtkWidget *widget = GTK_WIDGET(g);
  edge_list *el=v->edges;
  cairo_t *c = cairo_create(g->background);

  if(!g->hide_lines){
    setup_background_edge(c);
    while(el){
      edge *e=el->edge;
      
      draw_edge(c,e);
      
      el=el->next;
    }
    finish_edge(c);
  }
  cairo_destroy(c);

  invalidate_attached(widget,v);    
}

/* as above for re-inserting group drag edges to the background.
   Doesn't require a full redraw. */
void update_add_selgroup(Gameboard *g){
  if(!g->hide_lines){
    GtkWidget *widget = GTK_WIDGET(g);
    cairo_t *c = cairo_create(g->background);
    vertex *v = g->g.verticies;
    
    setup_background_edge(c);
    
    while(v){
      if(v->grabbed){
	edge_list *el=v->edges;
	while(el){
	  edge *e=el->edge;
	  
	  // draw the edge only once if it spans two grabbed verticies
	  if(e->A->grabbed==0 || e->B->grabbed==0 || v==e->A) 
	    draw_edge(c,e);
	  
	  el=el->next;
	}
      }
      invalidate_attached(widget,v);    
      v=v->next;
    }
    finish_edge(c);
    cairo_destroy(c);
  }
}

/* top level draw function called by expose.  May also be called
   directly to immediately render a region of the board without going
   through the expose mechanism (necessary for some button draw
   operations where expose combining causes huge, slow in-server alpha
   blends that are undesirable) */
void gameboard_draw(Gameboard *g, int x, int y, int w, int h){
  cairo_t *c = cairo_create(g->foreground);
  if (w==0 || h==0) return;

  if(g->realtime_background){
    draw_background_realpart(g,c,x,y,w,h);
  }else{
    // copy background to foreground draw buffer
    cairo_set_source_surface(c,g->background,0,0);
    cairo_rectangle(c,x,y,w,h);
    cairo_fill(c);
  }

  if(!g->pushed_curtain){
    draw_foreground(g,c,x,y,w,h);
    
    // copy in any of the score or button surfaces?
    if(y<SCOREHEIGHT){
      cairo_set_source_surface(c,g->forescore,0,0);
      cairo_rectangle(c, x,y,w,
		      min(SCOREHEIGHT-y,h));
      cairo_fill(c);
    }
    if(y+h>g->w.allocation.height-SCOREHEIGHT){
      cairo_set_source_surface(c,g->forebutton,0,
			       g->w.allocation.height-SCOREHEIGHT);
      cairo_rectangle(c, x,y,w,h);
      cairo_fill(c);
    }
    
    if(g->show_intersections)
      draw_intersections(g,&g->g,c,x,y,w,h);
  }
  
  render_level_icons(g,c,x,y,w,h);
  expose_buttons(g,c,x,y,w,h);

  cairo_destroy(c);
  
  // blit to window
  cairo_set_source_surface(g->wc,g->foreground,0,0);
  cairo_rectangle(g->wc,x,y,w,h);
  cairo_fill(g->wc);

  if(g->delayed_background)update_full(g);
  g->first_expose=1;

}

cairo_surface_t *gameboard_read_icon(char *filename, char *ext, Gameboard *b){
  char *name=alloca(strlen(boarddir)+strlen(filename)+strlen(ext)+2);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,filename);
  strcat(name,".");
  strcat(name,ext);
  
  cairo_surface_t *s = cairo_image_surface_create_from_png(name);

  if(s==NULL || cairo_surface_status(s)!=CAIRO_STATUS_SUCCESS)
    fprintf(stderr,_("ERROR:  Could not load board icon \"%s\"\n"),
	    name);

  cairo_t *c = cairo_create(s);
  borderbox_path(c,1.5,1.5,ICON_WIDTH-3,ICON_HEIGHT-3);
  cairo_set_operator (c, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_line_width(c,B_LINE);
  //cairo_set_source_rgba (c, B_COLOR);
  //cairo_fill_preserve (c);
  cairo_set_source_rgba (c, B_LINE_COLOR);
  cairo_stroke (c);

  cairo_destroy(c);
  return s;
}

int gameboard_write_icon(char *filename, char *ext, Gameboard *b, graph *g,
			 int lines, int intersections){
  char *name=alloca(strlen(boarddir)+strlen(filename)+strlen(ext)+2);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,filename);
  strcat(name,".");
  strcat(name,ext);
  
  {
    cairo_surface_t *s = 
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
				  ICON_WIDTH,
				  ICON_HEIGHT);
    cairo_t *c = cairo_create(s);
    edge *e=g->edges;
    vertex *v = g->verticies;
      
    cairo_save(c);
    cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
    cairo_set_source_rgba (c, 1,1,1,1);
    cairo_paint(c);
    cairo_restore(c);
    cairo_scale(c,(float)ICON_WIDTH/g->width,(float)ICON_HEIGHT/g->height);

    if(lines){
      setup_background_edge(c);
      while(e){
	draw_edge(c,e);
	e=e->next;
      }
      finish_edge(c);
    }

    cairo_set_line_width(c,V_LINE);
    while(v){
      // dirty, but we may not have the cached vertex surface yet.
      cairo_arc(c,v->x,v->y,V_RADIUS,0,2*M_PI);
      cairo_set_source_rgb(c,V_FILL_IDLE_COLOR);
      cairo_fill_preserve(c);
      cairo_set_source_rgb(c,V_LINE_COLOR);
      cairo_stroke(c);

      v=v->next;
    }

    if(intersections)
      draw_intersections(b,g,c,0,0,g->width,g->height);

    if(cairo_surface_write_to_png(s,name) != CAIRO_STATUS_SUCCESS){
      fprintf(stderr,_("ERROR:  Could not save board icon \"%s\"\n"),
	      name);
      return -1;
    }

    cairo_destroy(c);
    cairo_surface_destroy(s);
  }

  return 0;

}

int gameboard_icon_exists(char *filename, char *ext){
  struct stat s;
  char *name=alloca(strlen(boarddir)+strlen(filename)+strlen(ext)+2);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,filename);
  strcat(name,".");
  strcat(name,ext);

  if(stat(name,&s))return 0;
  return 1;

}
