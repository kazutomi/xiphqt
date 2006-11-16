/*
 *
 *  sushivision copyright (C) 2006 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "plot.h"

static GtkWidgetClass *parent_class = NULL;

static void draw_scales_work(cairo_surface_t *s, scalespace xs, scalespace ys){
  int w = cairo_image_surface_get_width(s);
  int h = cairo_image_surface_get_height(s);
  cairo_t *c = cairo_create(s);
  int i=0,x,y;
  char buffer[80];
  int y_width=0;
  int x_height=0;

  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);

  // draw all axis lines, then stroke
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_XOR); 

  cairo_set_line_width(c,1.);
  cairo_set_source_rgba(c,.7,.7,1.,.3);

  i=0;
  x=scalespace_mark(&xs,i++);
  while(x < w){
    cairo_move_to(c,x+.5,0);
    cairo_line_to(c,x+.5,h);
    x=scalespace_mark(&xs,i++);
  }

  i=0;
  y=scalespace_mark(&ys,i++);
  while(y < h){
    cairo_move_to(c,0,h-y+.5);
    cairo_line_to(c,w,h-y+.5);
    y=scalespace_mark(&ys,i++);
  }
  cairo_stroke(c);
  cairo_restore(c);

  // text number labels
  cairo_select_font_face (c, "Sans",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (c, 10);
  cairo_set_line_width(c,2);

  i=0;
  y=scalespace_mark(&ys,i);
  scalespace_label(&ys,i++,buffer);

  while(y < h){
    cairo_text_extents_t extents;
    cairo_text_extents (c, buffer, &extents);

    if(extents.width > y_width) y_width = extents.width;

    if(y - extents.height > 0){
      
      double yy = h-y+.5-(extents.height/2 + extents.y_bearing);

      cairo_move_to(c,2, yy);
      cairo_set_source_rgba(c,0,0,0,.5);
      cairo_text_path (c, buffer);  
      cairo_stroke(c);
      
      cairo_set_source_rgba(c,1.,1.,1.,1.);
      cairo_move_to(c,2, yy);
      cairo_show_text (c, buffer);
    }

    y=scalespace_mark(&ys,i);
    scalespace_label(&ys,i++,buffer);
  }

  // set sideways text
  cairo_save(c);
  cairo_matrix_t m = {0.,-1., 1.,0.,  0.,h};
  cairo_set_matrix(c,&m);

  // text y scale label
  if(ys.legend){
    cairo_text_extents_t extents;
    cairo_text_extents (c, ys.legend, &extents);
    
    cairo_move_to(c,h/2 - extents.width/2+extents.x_bearing, y_width-extents.y_bearing+5);
    cairo_set_source_rgba(c,0,0,0,.5);
    cairo_text_path (c, ys.legend);  
    cairo_stroke(c);
    
    cairo_move_to(c,h/2 - extents.width/2+extents.x_bearing, y_width-extents.y_bearing+5);
    cairo_set_source_rgba(c,1.,1.,1.,1.);
    cairo_show_text (c, ys.legend);
  }

  i=0;
  x=scalespace_mark(&xs,i);
  scalespace_label(&xs,i++,buffer);
  
  while(x < w){
    cairo_text_extents_t extents;
    cairo_text_extents (c, buffer, &extents);

    if(extents.width > x_height) x_height = extents.width;

    if(x - extents.height > y_width+5 ){

      cairo_move_to(c,2, x+.5-(extents.height/2 + extents.y_bearing));
      cairo_set_source_rgba(c,0,0,0,.5);
      cairo_text_path (c, buffer);  
      cairo_stroke(c);
      
      cairo_move_to(c,2, x+.5-(extents.height/2 + extents.y_bearing));
      cairo_set_source_rgba(c,1.,1.,1.,1.);
      cairo_show_text (c, buffer);
    }

    x=scalespace_mark(&xs,i);
    scalespace_label(&xs,i++,buffer);
  }

  cairo_restore(c);
  // text x scale label
  if(xs.legend){
    cairo_text_extents_t extents;
    cairo_text_extents (c, xs.legend, &extents);
    
    cairo_move_to(c,w/2 - extents.width/2+extents.x_bearing, h - x_height+ extents.y_bearing-5);
    cairo_set_source_rgba(c,0,0,0,.5);
    cairo_text_path (c, xs.legend);  
    cairo_stroke(c);
    
    cairo_move_to(c,w/2 - extents.width/2+extents.x_bearing, h - x_height+ extents.y_bearing-5);
    cairo_set_source_rgba(c,1.,1.,1.,1.);
    cairo_show_text (c, xs.legend);
  }

  cairo_destroy(c);
}

static void draw_legend_work(Plot *p, cairo_surface_t *s){
  if(p->legend_entries && p->legend_list){
    int w = cairo_image_surface_get_width(s);
    //int h = cairo_image_surface_get_height(s);
    cairo_t *c = cairo_create(s);
    int i;
    int textw=0, texth=0;
    int totalh=0;
    int x,y;
    cairo_text_extents_t extents;
    
    gdk_threads_enter();
    int n = p->legend_entries;
    char *buffer[n];
    for(i=0;i<n;i++)
      buffer[i] = strdup(p->legend_list[i]);
    gdk_threads_leave();

    cairo_select_font_face (c, "Sans",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (c, 10);
    cairo_set_line_width(c,1);
    
    /* determine complete x/y extents of text */
    
    for(i=0;i<n;i++){
      cairo_text_extents (c, buffer[i], &extents);
      if(texth < extents.height)
	texth = extents.height;
      if(textw < extents.width)
	textw = extents.width;
    }
    
    y = 10+texth;
    texth = ceil(texth * 1.2+3);
    totalh = texth*n;
    
    x = w - textw - 10;
    
    for(i=0;i<n;i++){
      cairo_text_extents (c, buffer[i], &extents);
      x = w - extents.width - 10;
      
      cairo_move_to(c,x, y);
      cairo_text_path (c, buffer[i]); 

      cairo_set_source_rgba(c,0,0,0,.5);
      cairo_set_line_width(c,3);
      cairo_stroke(c);

      cairo_set_source_rgba(c,1.,1.,1.,1.);
      cairo_move_to(c,x, y);
      cairo_show_text (c, buffer[i]);

      y+=texth;
    }
    
    cairo_destroy(c);
    for(i=0;i<n;i++)
      free(buffer[i]);

    
  }
}

void plot_draw_scales(Plot *p){
  // render into a temporary surface; do it [potentially] outside the global Gtk lock.
  gdk_threads_enter();
  scalespace x = p->x;
  scalespace y = p->y;
  int w = GTK_WIDGET(p)->allocation.width;
  int h = GTK_WIDGET(p)->allocation.height;
  cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);
  gdk_threads_leave();
  
  draw_scales_work(s,x,y);
  draw_legend_work(p,s);
  
  gdk_threads_enter();
  // swap fore/temp
  cairo_surface_t *temp = p->fore;
  p->fore = s;
  cairo_surface_destroy(temp);
  plot_expose_request(p);
  gdk_threads_leave();
}

static void plot_init (Plot *p){
  // instance initialization
  p->scalespacing = 50;
  p->x = scalespace_linear(0.0,1.0,400,p->scalespacing,NULL);
  p->y = scalespace_linear(0.0,1.0,200,p->scalespacing,NULL);
}

static void plot_destroy (GtkObject *object){
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);

  GtkWidget *widget = GTK_WIDGET(object);
  Plot *p = PLOT (widget);
  // free local resources
  if(p->wc){
    cairo_destroy(p->wc);
    p->wc=0;
  }
  if(p->back){
    cairo_surface_destroy(p->back);
    p->back=0;
  }
  if(p->fore){
    cairo_surface_destroy(p->fore);
    p->fore=0;
  }
  if(p->stage){
    cairo_surface_destroy(p->stage);
    p->stage=0;
  }

}

static void box_corners(Plot *p, double vals[4]){
  GtkWidget *widget = GTK_WIDGET(p);
  double x1 = scalespace_pixel(&p->x,p->box_x1);
  double x2 = scalespace_pixel(&p->x,p->box_x2);
  double y1 = widget->allocation.height-
    scalespace_pixel(&p->y,p->box_y1);
  double y2 = widget->allocation.height-
    scalespace_pixel(&p->y,p->box_y2);

  vals[0] = (x1<x2 ? x1 : x2);
  vals[1] = (y1<y2 ? y1 : y2);
  vals[2] = fabs(x1-x2);
  vals[3] = fabs(y1-y2);
}

static int inside_box(Plot *p, int x, int y){
  double vals[4];
  box_corners(p,vals);
  
  return (x >= vals[0] && 
	  x <= vals[0]+vals[2] && 
	  y >= vals[1] && 
	  y <= vals[1]+vals[3]);
}

static void plot_draw (Plot *p,
		int x, int y, int w, int h){

  GtkWidget *widget = GTK_WIDGET(p);
  
  if (GTK_WIDGET_REALIZED (widget)){

    cairo_t *c = cairo_create(p->stage);
    cairo_set_source_surface(c,p->back,0,0);
    cairo_rectangle(c,x,y,w,h);
    cairo_fill(c);
    
    cairo_set_source_surface(c,p->fore,0,0);
    cairo_rectangle(c,x,y,w,h);
    cairo_fill(c);
    
    // transient foreground
    if(p->cross_active){
      double sx = plot_get_crosshair_xpixel(p);
      double sy = plot_get_crosshair_ypixel(p);
      cairo_set_source_rgba(c,1.,1.,1.,.8);
      cairo_set_line_width(c,1.);
      cairo_move_to(c,0,sy+.5);
      cairo_line_to(c,widget->allocation.width,sy+.5);
      cairo_move_to(c,sx+.5,0);
      cairo_line_to(c,sx+.5,widget->allocation.height);
      cairo_stroke(c);
    }

    if(p->box_active){
      double vals[4];
      box_corners(p,vals);

      cairo_rectangle(c,vals[0],vals[1],vals[2]+1,vals[3]+1);	
      if(p->box_active>1)
	cairo_set_source_rgba(c,1.,1.,.6,.4);
      else
	cairo_set_source_rgba(c,1.,1.,1.,.3);
      cairo_fill(c);
      cairo_rectangle(c,vals[0]+.5,vals[1]+.5,vals[2],vals[3]);
      if(p->box_active>1)
	cairo_set_source_rgba(c,1.,1.,.6,.9);
      else
	cairo_set_source_rgba(c,1.,1.,1.,.8);
      cairo_stroke(c);
    }

    if(p->widgetfocus){
      double dashes[] = {1.,  /* ink */
			 1.};
      cairo_save(c);
      cairo_set_dash (c, dashes, 2, .5);
      cairo_set_source_rgba(c,0.,0.,0.,1.);
      cairo_set_line_width(c,1.);
      cairo_rectangle(c,.5,.5,
		      widget->allocation.width-1.,
		      widget->allocation.height-1.);
      cairo_stroke_preserve (c);
      cairo_set_dash (c, dashes, 2, 1.5);
      cairo_set_source_rgba(c,1.,1.,1.,1.);
      cairo_stroke(c);
      cairo_restore(c);
    }

    cairo_destroy(c);

    // blit to window
    cairo_set_source_surface(p->wc,p->stage,0,0);
    cairo_rectangle(p->wc,x,y,w,h);
    cairo_fill(p->wc);

  }
}

static gint plot_expose (GtkWidget      *widget,
			 GdkEventExpose *event){
  if (GTK_WIDGET_REALIZED (widget)){
    Plot *p = PLOT (widget);

    int x = event->area.x;
    int y = event->area.y;
    int w = event->area.width;
    int h = event->area.height;

    plot_draw(p, x, y, w, h);

  }
  return FALSE;
}

static void plot_size_request (GtkWidget *widget,
			       GtkRequisition *requisition){
  requisition->width = 400; // XXX
  requisition->height = 200; // XXX
}

static void plot_realize (GtkWidget *widget){
  GdkWindowAttr attributes;
  gint      attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);

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
    GDK_STRUCTURE_MASK |
    GDK_ENTER_NOTIFY_MASK |
    GDK_LEAVE_NOTIFY_MASK;

  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window,
				   &attributes, attributes_mask);
  gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  gdk_window_set_user_data (widget->window, widget);
  gtk_widget_set_double_buffered (widget, FALSE);
}

static void plot_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation){
  Plot *p = PLOT (widget);
  
  if (GTK_WIDGET_REALIZED (widget)){

    if(p->wc)
      cairo_destroy(p->wc);
    if (p->fore)
      cairo_surface_destroy(p->fore);
    if (p->back)
      cairo_surface_destroy(p->back);
    if (p->stage)
      cairo_surface_destroy(p->stage);
    
    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
			    allocation->width, allocation->height);
    
    p->wc = gdk_cairo_create(widget->window);

    // the background is created from data
    p->datarect = calloc(allocation->width * allocation->height,4);
    
    p->back = cairo_image_surface_create_for_data ((unsigned char *)p->datarect,
						   CAIRO_FORMAT_RGB24,
						   allocation->width,
						   allocation->height,
						   allocation->width*4);

    p->stage = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
					  allocation->width,
					  allocation->height);
    p->fore = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					  allocation->width,
					  allocation->height);

  }

  widget->allocation = *allocation;
  p->x = scalespace_linear(p->x.lo,p->x.hi,widget->allocation.width,p->scalespacing,p->x.legend);
  p->y = scalespace_linear(p->y.lo,p->y.hi,widget->allocation.height,p->scalespacing,p->y.legend);
  plot_unset_box(p);
  plot_draw_scales(p);
  if(p->recompute_callback)p->recompute_callback(p->app_data);

}

static void box_check(Plot *p, int x, int y){
  if(p->box_active){
    double vals[4];
    box_corners(p,vals);
    
    if(inside_box(p,x,y) && !p->button_down)
      p->box_active = 2;
    else
      p->box_active = 1;
    
    plot_expose_request_partial(p,
				(int)(vals[0]),
				(int)(vals[1]),
				(int)(vals[2]+3), // account for floor/ceil *and* roundoff potential
				(int)(vals[3]+3));
  }
}

static gint mouse_motion(GtkWidget        *widget,
			 GdkEventMotion   *event){
  Plot *p = PLOT (widget);

  int x = event->x;
  int y = event->y;
  int bx = scalespace_pixel(&p->x,p->box_x1);
  int by = widget->allocation.height-scalespace_pixel(&p->y,p->box_y1);

  if(p->button_down){
    if(abs(bx - x)>5 ||
       abs(by - y)>5)
      p->box_active = 1;
    
    if(p->box_active){
      double vals[4];
      box_corners(p,vals);
      plot_expose_request_partial(p,
				  (int)(vals[0]),
				  (int)(vals[1]),
				  (int)(vals[2]+3),  // account for floor/ceil *and* roundoff potential
				  (int)(vals[3]+3));
    }
    
    p->box_x2 = scalespace_value(&p->x,x);
    p->box_y2 = scalespace_value(&p->y,widget->allocation.height-y);
  }

  box_check(p,x,y);

  return TRUE;
}

static gboolean mouse_press (GtkWidget        *widget,
			     GdkEventButton   *event){
  Plot *p = PLOT (widget);
 
  if(p->box_active && inside_box(p,event->x,event->y) && !p->button_down){

    p->selx = scalespace_value(&p->x,event->x);
    p->sely = scalespace_value(&p->y,widget->allocation.height-event->y);
    p->cross_active=1;

    if(p->box_callback)
      p->box_callback(p->cross_data,1);

    p->button_down=0;
    p->box_active=0;

  }else{
    p->box_x2=p->box_x1 = scalespace_value(&p->x,event->x);
    p->box_y2=p->box_y1 = scalespace_value(&p->y,widget->allocation.height-event->y);
    p->box_active = 0;
    p->button_down=1; 
  }
  gtk_widget_grab_focus(widget);
  return TRUE;
}
 
static gboolean mouse_release (GtkWidget        *widget,
			       GdkEventButton   *event){
  Plot *p = PLOT (widget);
  plot_expose_request(p);

  if(!p->box_active && p->button_down){
    p->selx = scalespace_value(&p->x,event->x);
    p->sely = scalespace_value(&p->y,widget->allocation.height-event->y);
    p->cross_active=1;

    if(p->crosshairs_callback)
      p->crosshairs_callback(p->cross_data);
    plot_expose_request(p);
  }

  p->button_down=0;
  box_check(p,event->x, event->y);

  if(p->box_active){
    if(p->box_callback)
      p->box_callback(p->cross_data,0);
  }  
  

  return TRUE;
}

static gboolean key_press(GtkWidget *widget,
			  GdkEventKey *event){
  Plot *p = PLOT(widget);

  int shift = (event->state&GDK_SHIFT_MASK);
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK)return FALSE;
  
  /* non-control keypresses */
  switch(event->keyval){
  case GDK_Escape:
    p->button_down=0;
    p->box_active=0;
    p->cross_active=0;
    plot_expose_request(p);

    return TRUE;
  case GDK_Return:
    // if box is active, effect it
    if(p->box_active){
      
      if(p->box_callback)
	p->box_callback(p->cross_data,1);
      
      p->button_down=0;
      p->box_active=0;
    }else{
      if(p->button_down){
	GdkEventButton event;
	event.x = scalespace_pixel(&p->x,p->selx);
	event.y = widget->allocation.height-
	  scalespace_pixel(&p->y,p->sely);
	
	mouse_release(widget,&event);
      }else{
	p->box_x2=p->box_x1 = p->selx;
	p->box_y2=p->box_y1 = p->sely;
	p->box_active = 1;
	p->button_down=1; 
      }
    }
    return TRUE;

  case GDK_Left:
    {
      double x = scalespace_pixel(&p->x,p->selx)-1;
      p->cross_active=1;
      if(shift)
	x-=9;
      p->selx = scalespace_value(&p->x,x);
      if(p->crosshairs_callback)
	p->crosshairs_callback(p->cross_data);

      if(p->button_down){
	p->box_active=1;
	p->box_x2 = p->selx;
      }else
	p->box_active=0;

      plot_expose_request(p);
    }
    return TRUE;

  case GDK_Right:
    {
      double x = scalespace_pixel(&p->x,p->selx)+1;
      p->cross_active=1;
      if(shift)
	x+=9;
      p->selx = scalespace_value(&p->x,x);
       if(p->crosshairs_callback)
	p->crosshairs_callback(p->cross_data);

      if(p->button_down){
	p->box_active=1;
	p->box_x2 = p->selx;
      }else
	p->box_active=0;

      plot_expose_request(p);

    }
    return TRUE;
  case GDK_Up:
    {
      double y = widget->allocation.height - scalespace_pixel(&p->y,p->sely)-1;
      p->cross_active=1;
      if(shift)
	y-=9;
      p->sely = scalespace_value(&p->y,widget->allocation.height - y);
      if(p->crosshairs_callback)
	p->crosshairs_callback(p->cross_data);

      if(p->button_down){
	p->box_active=1;
	p->box_y2 = p->sely;
      }else
	p->box_active=0;

      plot_expose_request(p);
    }
    return TRUE;
  case GDK_Down:
    {
      double y = widget->allocation.height - scalespace_pixel(&p->y,p->sely)+1;
      p->cross_active=1;
      if(shift)
	y+=9;
      p->sely = scalespace_value(&p->y,widget->allocation.height - y);
      if(p->crosshairs_callback)
	p->crosshairs_callback(p->cross_data);
      
      if(p->button_down){
	p->box_active=1;
	p->box_y2 = p->sely;
      }else
	p->box_active=0;
      
      plot_expose_request(p);

    }
    return TRUE;
  }


  return FALSE;
}

static gboolean plot_unfocus(GtkWidget        *widget,
			     GdkEventFocus       *event){
  Plot *p=PLOT(widget);
  p->widgetfocus=0;
  plot_expose_request(p);
  return TRUE;
}

static gboolean plot_refocus(GtkWidget        *widget,
			     GdkEventFocus       *event){
  Plot *p=PLOT(widget);
  p->widgetfocus=1;
  plot_expose_request(p);
  return TRUE;
}

static void plot_class_init (PlotClass * class) {

  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (GTK_TYPE_WIDGET);

  object_class->destroy = plot_destroy;

  widget_class->realize = plot_realize;
  widget_class->expose_event = plot_expose;
  widget_class->size_request = plot_size_request;
  widget_class->size_allocate = plot_size_allocate;
  widget_class->button_press_event = mouse_press;
  widget_class->button_release_event = mouse_release;
  widget_class->motion_notify_event = mouse_motion;
  //widget_class->enter_notify_event = plot_enter;
  //widget_class->leave_notify_event = plot_leave;
  widget_class->key_press_event = key_press;

  widget_class->focus_out_event = plot_unfocus;
  widget_class->focus_in_event = plot_refocus;

}

GType plot_get_type (void){

  static GType plot_type = 0;

  if (!plot_type)
    {
      static const GTypeInfo plot_info = {
        sizeof (PlotClass),
        NULL,
        NULL,
        (GClassInitFunc) plot_class_init,
        NULL,
        NULL,
        sizeof (Plot),
        0,
        (GInstanceInitFunc) plot_init,
	0
      };

      plot_type = g_type_register_static (GTK_TYPE_WIDGET, "Plot",
                                               &plot_info, 0);
    }
  
  return plot_type;
}

Plot *plot_new (void (*callback)(void *),void *app_data,
		void (*cross_callback)(void *),void *cross_data,
		void (*box_callback)(void *, int),void *box_data) {
  GtkWidget *g = GTK_WIDGET (g_object_new (PLOT_TYPE, NULL));
  Plot *p = PLOT (g);
  p->recompute_callback = callback;
  p->app_data = app_data;
  p->crosshairs_callback = cross_callback;
  p->cross_data = cross_data;
  p->box_callback = box_callback;
  p->box_data = box_data;
  return p;
}

void plot_expose_request(Plot *p){
  gdk_threads_enter();

  GtkWidget *widget = GTK_WIDGET(p);
  GdkRectangle r;
  
  if (GTK_WIDGET_REALIZED (widget)){
    r.x=0;
    r.y=0;
    r.width=widget->allocation.width;
    r.height=widget->allocation.height;
    
    gdk_window_invalidate_rect (widget->window, &r, FALSE);
  }
  gdk_threads_leave();
}

void plot_expose_request_partial(Plot *p,int x, int y, int w, int h){
  gdk_threads_enter();

  GtkWidget *widget = GTK_WIDGET(p);
  GdkRectangle r;
  
  r.x=x;
  r.y=y;
  r.width=w;
  r.height=h;
  
  gdk_window_invalidate_rect (widget->window, &r, FALSE);
  gdk_threads_leave();
}

void plot_expose_request_line(Plot *p, int num){
  gdk_threads_enter();
  GtkWidget *widget = GTK_WIDGET(p);
  GdkRectangle r;
  
  r.x=0;
  r.y=num;
  r.width=widget->allocation.width;
  r.height=1;
  
  gdk_window_invalidate_rect (widget->window, &r, FALSE);
  gdk_threads_leave();
}

void plot_set_x_scale(Plot *p, scalespace x){
  scalespace temp = p->x;
  p->x = x;
  if(memcmp(&temp,&p->x,sizeof(temp)))
    plot_draw_scales(p);
}

void plot_set_y_scale(Plot *p, scalespace y){
  //GtkWidget *widget = GTK_WIDGET(p);
  scalespace temp = p->y;
  p->y = y;
  if(memcmp(&temp,&p->y,sizeof(temp)))
    plot_draw_scales(p);
}

void plot_set_x_name(Plot *p, char *name){
  p->namex = name;
  plot_draw_scales(p); // releases one lock level
}

void plot_set_y_name(Plot *p, char *name){
  p->namey = name;
  plot_draw_scales(p); // releases one lock level
}

u_int32_t *plot_get_background_line(Plot *p, int num){
  GtkWidget *widget = GTK_WIDGET(p);
  return p->datarect + num*widget->allocation.width;
}

cairo_t *plot_get_background_cairo(Plot *p){
  gdk_threads_enter();
  cairo_t *ret= cairo_create(p->back);
  gdk_threads_leave();
  return ret;
}

void plot_set_crosshairs(Plot *p, double x, double y){
  gdk_threads_enter();
  p->selx = x;
  p->sely = y;
  p->cross_active=1;

  plot_expose_request(p);
  gdk_threads_leave();
}

int plot_get_crosshair_xpixel(Plot *p){
  scalespace x;
  double v;

  gdk_threads_enter();
  x = p->x;
  v = p->selx;
  gdk_threads_leave();

  return (int)rint(scalespace_pixel(&x,v));
}

int plot_get_crosshair_ypixel(Plot *p){
  GtkWidget *widget = GTK_WIDGET(p);
  scalespace y;
  double v;

  gdk_threads_enter();
  y = p->y;
  v = p->sely;
  gdk_threads_leave();

  return (int)rint(widget->allocation.height-scalespace_pixel(&y,v));
}

void plot_unset_box(Plot *p){
  gdk_threads_enter();
  p->box_active = 0;
  gdk_threads_leave();
}

void plot_box_vals(Plot *p, double ret[4]){
  gdk_threads_enter();
  ret[0] = (p->box_x1<p->box_x2?p->box_x1:p->box_x2);
  ret[1] = (p->box_x1>p->box_x2?p->box_x1:p->box_x2);

  ret[2] = (p->box_y1<p->box_y2?p->box_y1:p->box_y2);
  ret[3] = (p->box_y1>p->box_y2?p->box_y1:p->box_y2);
  gdk_threads_leave();
}

void plot_box_set(Plot *p, double vals[4]){
  gdk_threads_enter();

  p->box_x1=vals[0];
  p->box_x2=vals[1];
  p->box_y1=vals[2];
  p->box_y2=vals[3];
  p->box_active = 1;
  
  plot_expose_request(p);
  gdk_threads_leave();
}

void plot_legend_clear(Plot *p){
  int i;
  if(p->legend_list){
    for(i=0;i<p->legend_entries;i++)
      if(p->legend_list[i])
	free(p->legend_list[i]);
    free(p->legend_list);
    p->legend_list=NULL;
  }
  p->legend_entries=0;
}

void plot_legend_add(Plot *p, char *entry){
  if(!p->legend_list){
    p->legend_list = calloc(1, sizeof(*p->legend_list));
    p->legend_entries=1;
  }else{
    p->legend_entries++;
    p->legend_list = realloc(p->legend_list, p->legend_entries*sizeof(*p->legend_list));
  }

  if(entry)
    p->legend_list[p->legend_entries-1] = strdup(entry);
  else
    p->legend_list[p->legend_entries-1] = strdup("");
}
