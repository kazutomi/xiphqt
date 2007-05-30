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
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "gettext.h"
#include "graph.h"
#include "gameboard.h"
#include "levelstate.h"


static GtkWidgetClass *parent_class = NULL;

static void gameboard_init (Gameboard *g){
  // instance initialization
  g->g.width=g->g.orig_width=800;
  g->g.height=g->g.orig_height=600;
  g->resize_w = g->g.width;
  g->resize_h = g->g.height;
  g->resize_timeout = 0;
}

static void gameboard_destroy (GtkObject *object){
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);

  // free local resources
}


static gint gameboard_expose (GtkWidget      *widget,
			      GdkEventExpose *event){
  if (GTK_WIDGET_REALIZED (widget)){
    Gameboard *g = GAMEBOARD (widget);
    gameboard_draw(g,event->area.x,event->area.y,
		   event->area.width,event->area.height);


    if(g->resize_h>0 && g->resize_w>0){
      if(widget->allocation.width != g->resize_w ||
	 widget->allocation.height != g->resize_h){
	if(time(NULL) > g->resize_timeout){
	  fprintf(stderr,_("\n\nERROR: The windowmanager appears to be ignoring resize requests.\n"
		  "This stands a pretty good chance of scrambling any saved board larger\n"
		  "than the default window size.\n\n"));
    fprintf(stderr,_("Clipping and/or expanding this board to the current window size...\n\n"));
	  g->resize_w = 0;
	  g->resize_h = 0;
	  resize_buttons(g,g->g.width, g->g.height, widget->allocation.width, widget->allocation.height);
	  graph_resize(&g->g, widget->allocation.width, widget->allocation.height);
	  update_score(g);
	  draw_buttonbar_box(g);
	  update_full(g);
	}
      }else{
	g->resize_h=0;
	g->resize_w=0;
      }
    }
  }
  return FALSE;
}

static void gameboard_size_request (GtkWidget *widget,
				    GtkRequisition *requisition){
  Gameboard *g = GAMEBOARD (widget);
  requisition->width = g->g.orig_width;
  requisition->height = g->g.orig_height;
}

static void gameboard_realize (GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  GdkWindowAttr attributes;
  gint      attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = 
    gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK|
    GDK_POINTER_MOTION_MASK|
    GDK_BUTTON_PRESS_MASK  |
    GDK_BUTTON_RELEASE_MASK|
    GDK_KEY_PRESS_MASK |
    GDK_KEY_RELEASE_MASK |
    GDK_STRUCTURE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window,
				   &attributes, attributes_mask);
  gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  gdk_window_set_user_data (widget->window, widget);
  gtk_widget_set_double_buffered (widget, FALSE);

  g->wc = gdk_cairo_create(widget->window);

  g->forescore = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  widget->allocation.width,
				  SCOREHEIGHT);

  g->forebutton = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  widget->allocation.width,
				  SCOREHEIGHT);

  g->background = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR,
				  widget->allocation.width,
				  widget->allocation.height);
  g->foreground = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR,
				  widget->allocation.width,
				  widget->allocation.height);  

  g->vertex = cache_vertex(g);
  g->vertex_lit = cache_vertex_lit(g);
  g->vertex_attached = cache_vertex_attached(g);
  g->vertex_grabbed = cache_vertex_grabbed(g);
  g->vertex_sel = cache_vertex_sel(g);
  g->vertex_ghost = cache_vertex_ghost(g);

  update_full(g); 
  update_score(g);
  draw_buttonbar_box(g);
  init_buttons(g);
  cache_curtain(g);
}

void gameboard_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation){
  Gameboard *g = GAMEBOARD (widget);

  if (GTK_WIDGET_REALIZED (widget)){

    if(allocation->width != g->resize_w &&
       allocation->height != g->resize_h &&
       g->resize_timeout == 0 ){

      fprintf(stderr,_("\n\nERROR: The window size granted by the windowmanager is not the\n"
	      "window size gPlanarity requested.  If the windowmanager is\n"
	      "configured to ignore application sizing requests, this stands\n"
	      "a pretty good chance of scrambling saved boards later (and\n"
	      "making everything look funny now).\n\n"));
      fprintf(stderr,_("Clipping and/or expanding this board to the current window size...\n\n"));

      g->resize_h=0;
      g->resize_w=0;
    }

    g->resize_timeout=1;

    if(widget->allocation.width == allocation->width &&
       widget->allocation.height == allocation->height) return;

    if(g->wc)
      cairo_destroy(g->wc);
    if (g->forescore)
      cairo_surface_destroy(g->forescore);
    if (g->forebutton)
      cairo_surface_destroy(g->forebutton);
    if (g->background)
      cairo_surface_destroy(g->background);
    if (g->foreground)
      cairo_surface_destroy(g->foreground);
    if (g->curtainp)
      cairo_pattern_destroy(g->curtainp);
    if (g->curtains)
      cairo_surface_destroy(g->curtains);


    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
			    allocation->width, allocation->height);
    
    g->wc = gdk_cairo_create(widget->window);

    g->background = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR, // don't need alpha
				    allocation->width,
				    allocation->height);
    g->forescore = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR_ALPHA,
				    allocation->width,
				    SCOREHEIGHT);
    g->forebutton = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR_ALPHA,
				    allocation->width,
				    SCOREHEIGHT);

    g->foreground = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR, // don't need alpha
				    allocation->width,
				    allocation->height);  

    cache_curtain(g);

    resize_buttons(g,g->g.width, g->g.height, allocation->width, allocation->height);
    graph_resize(&g->g, allocation->width, allocation->height);
  
    draw_buttonbar_box(g);
    update_score(g);
    update_full(g);
    
  }
  widget->allocation = *allocation;
}

static void gameboard_class_init (GameboardClass * class) {

  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (GTK_TYPE_WIDGET);

  object_class->destroy = gameboard_destroy;

  widget_class->realize = gameboard_realize;
  widget_class->expose_event = gameboard_expose;
  widget_class->size_request = gameboard_size_request;
  widget_class->size_allocate = gameboard_size_allocate;
  widget_class->button_press_event = mouse_press;
  widget_class->button_release_event = mouse_release;
  widget_class->motion_notify_event = mouse_motion;

}

GType gameboard_get_type (void){

  static GType gameboard_type = 0;

  if (!gameboard_type)
    {
      static const GTypeInfo gameboard_info = {
        sizeof (GameboardClass),
        NULL,
        NULL,
        (GClassInitFunc) gameboard_class_init,
        NULL,
        NULL,
        sizeof (Gameboard),
        0,
        (GInstanceInitFunc) gameboard_init,
	0
      };

      gameboard_type = g_type_register_static (GTK_TYPE_WIDGET, "Gameboard",
                                               &gameboard_info, 0);
    }
  
  return gameboard_type;
}

Gameboard *gameboard_new (void) {
  GtkWidget *g = GTK_WIDGET (g_object_new (GAMEBOARD_TYPE, NULL));
  Gameboard *gb = GAMEBOARD (g);
  return gb;
}
