/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
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
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "spinner.h"

static GtkWidgetClass *parent_class = NULL;

#define DIA 4
#define SPA 6

static void _sv_spinner_draw(GtkWidget *wg, cairo_surface_t *s,int n){
  cairo_t *c = cairo_create(s);
  int w = cairo_image_surface_get_width(s);
  int h = cairo_image_surface_get_height(s);
  int i = n;
  double alpha = (n>=0?1.:0.);

  if(n==-1)i=n=7;

  GdkColor *bg = &wg->style->bg[GTK_STATE_NORMAL];
  double shade_r=bg->red/65535.;
  double shade_g=bg->green/65535.;
  double shade_b=bg->blue/65535.;
  cairo_set_source_rgb (c, shade_r,shade_g,shade_b);
  cairo_paint(c);
  
  do{
    double x = w/2 - 7*SPA/2 + i*SPA +.5;
    double y = h/2 + .5;
    
    cairo_set_source_rgba (c, 0,0,.5,1.);
    cairo_arc(c,x,y,DIA*.5,0,M_PI*2.);
    cairo_fill(c);

    cairo_set_source_rgba (c, 1.,1.,1.,alpha);
    cairo_arc(c,x,y,DIA*.5,0,M_PI*2.);
    cairo_fill(c);

    i--;
    alpha *=.8;
    if(i<0)i=7;
  }while(i!=n);
}

static void _sv_spinner_init (_sv_spinner_t *p){
  // instance initialization
}

static void _sv_spinner_destroy (GtkObject *object){
  int i;
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);

  GtkWidget *widget = GTK_WIDGET(object);
  _sv_spinner_t *p = SPINNER (widget);
  // free local resources
  if(p->wc){
    cairo_destroy(p->wc);
    p->wc=0;
  }
  if(p->b){
    for(i=0;i<9;i++)
      if(p->b[i]){
	cairo_surface_destroy(p->b[i]);
	p->b[i]=0;
      }
  }
}

static gint _sv_spinner_expose (GtkWidget      *widget,
				GdkEventExpose *event){
  if (GTK_WIDGET_REALIZED (widget)){
    _sv_spinner_t *sp = SPINNER (widget);
    pthread_mutex_lock(spinner->spinner_m);
    int frame = (sp->busy?sp->busy_count+1:0);
    pthread_mutex_unlock(spinner->spinner_m);

    // blit to window
    if(sp->b && sp->b[frame]){
      cairo_set_source_surface(sp->wc,
			       sp->b[frame],0,0);
      cairo_paint(sp->wc);
      gdk_flush();
    }
  }
  return FALSE;
}

static void _sv_spinner_size_request (GtkWidget *widget,
				  GtkRequisition *requisition){
  requisition->width = SPA*7 + DIA + 2;
  requisition->height = DIA + 2;
}

static void _sv_spinner_realize (GtkWidget *widget){
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
    GDK_EXPOSURE_MASK;

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

static void _sv_spinner_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation){
  _sv_spinner_t *p = SPINNER (widget);
  int i; 

  if (GTK_WIDGET_REALIZED (widget)){

    if(p->wc)
      cairo_destroy(p->wc);

    if(p->b){
      for(i=0;i<9;i++)
	if(p->b[i]){
	  cairo_surface_destroy(p->b[i]);
	  p->b[i]=0;
	}
    }

    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
			    allocation->width, allocation->height);
    
    p->wc = gdk_cairo_create(widget->window);

    for(i=0;i<9;i++){
      p->b[i] = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
					   allocation->width,
					   allocation->height);
      _sv_spinner_draw(widget, p->b[i],i-1);
    }
  }

  widget->allocation = *allocation;
}

static void _sv_spinner_class_init (_sv_spinner_class_t * class) {
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  parent_class = gtk_type_class (GTK_TYPE_WIDGET);

  object_class->destroy = _sv_spinner_destroy;
  widget_class->realize = _sv_spinner_realize;
  widget_class->expose_event = _sv_spinner_expose;
  widget_class->size_request = _sv_spinner_size_request;
  widget_class->size_allocate = _sv_spinner_size_allocate;

}

GType _sv_spinner_get_type (void){

  static GType spinner_type = 0;

  if (!spinner_type)
    {
      static const GTypeInfo spinner_info = {
        sizeof (_sv_spinner_class_t),
        NULL,
        NULL,
        (GClassInitFunc) _sv_spinner_class_init,
        NULL,
        NULL,
        sizeof (_sv_spinner_t),
        0,
        (GInstanceInitFunc) _sv_spinner_init,
	0
      };

      spinner_type = g_type_register_static (GTK_TYPE_WIDGET, "Spinner",
                                               &spinner_info, 0);
    }
  
  return spinner_type;
}

static void *spinthread(sv_spinner_t *spinner){
  pthread_mutex_lock(spinner->spinner_m);
  while(p->busy_pending){
    pthread_mutex_unlock(spinner->spinner_m);
    gdk_threads_enter();
    _sv_spinner_expose(GTK_WIDGET(p),NULL);
    gdk_threads_leave();
    usleep(100000);
    pthread_mutex_lock(spinner->spinner_m);
    p->busy_count++;
    if(p->busy_count>7)
      p->busy_count=0;
  }

  spinner->busy=0;
  pthread_mutex_unlock(spinner->spinner_m);
  _sv_spinner_expose(GTK_WIDGET(p),NULL); 
  return 0;
}

sv_spinner_t *_sv_spinner_new (){
  GtkWidget *g = GTK_WIDGET (g_object_new (SPINNER_TYPE, NULL));
  sv_spinner_t *p = SPINNER (g);
  p->spinner_m = PTHREAD_MUTEX_INITIALIZER;
  return p;
}

void _sv_spinner_set_busy(_sv_spinner_t *p){
  if(!p)return;
  pthread_mutex_lock(spinner->spinner_m);
  if(!p->busy){
    pthread_t dummy;
    p->busy=1;
    p->busy_pending=1;
    pthread_create(&dummy, NULL, &spinthread, NULL);
  }
  pthread_mutex_unlock(spinner->spinner_m);
}

void _sv_spinner_set_idle(_sv_spinner_t *p){
  if(!p)return;
  pthread_mutex_lock(spinner->spinner_m);
  p->busy_pending=0;
  pthread_mutex_unlock(spinner->spinner_m);
}

