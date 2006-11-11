/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
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

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "mapping.h"
#include "slice.h"
#include "slider.h"

static void draw_and_expose(GtkWidget *widget){
  Slice *s=SLICE(widget);

  slider_draw(s->slider);
  slider_expose(s->slider);
}

static gboolean slice_expose(GtkWidget *widget, GdkEventExpose *event ){
  Slice *s=SLICE(widget);
  slider_expose_slice(s->slider,s->slicenum);
  return FALSE;
}

static void slice_size_request (GtkWidget *widget,GtkRequisition *requisition){
  Slice *s=SLICE(widget);
  
  slider_size_request_slice(s->slider,requisition);
}

static gboolean slice_focus (GtkWidget         *widget,
			     GtkDirectionType   direction){
  Slice *s=SLICE(widget);

  if(!s->thumb_active)return FALSE;
  if(s->thumb_focus)return TRUE;

  s->thumb_focus=1;
  gtk_widget_grab_focus(widget);
  draw_and_expose(widget);

  return TRUE;
}

static gint slice_motion(GtkWidget        *widget,
			 GdkEventMotion   *event){
  Slice *s=SLICE(widget);
  slider_motion(s->slider,s->slicenum,event->x,event->y);
  
  return TRUE;
}

static gint slice_enter(GtkWidget        *widget,
			GdkEventCrossing *event){
  Slice *s=SLICE(widget);
  slider_lightme(s->slider,s->slicenum,event->x,event->y);
  draw_and_expose(widget);
  return TRUE;
}

static gint slice_leave(GtkWidget        *widget,
			GdkEventCrossing *event){
  Slice *s=SLICE(widget);

  slider_unlight(s->slider);
  slider_draw(s->slider);
  slider_expose(s->slider);

  return TRUE;
}

static gboolean slice_button_press(GtkWidget        *widget,
				   GdkEventButton   *event){
  Slice *s=SLICE(widget);

  slider_button_press(s->slider,s->slicenum,event->x,event->y);
  return TRUE;
}

static gboolean slice_button_release(GtkWidget        *widget,
				     GdkEventButton   *event){
  Slice *s=SLICE(widget);
  slider_button_release(s->slider,s->slicenum,event->x,event->y);
  return TRUE;
}

static gboolean slice_unfocus(GtkWidget        *widget,
			      GdkEventFocus       *event){
  Slice *s=SLICE(widget);
  if(s->thumb_focus){
    s->thumb_focus=0;
    draw_and_expose(widget);
  }
  return TRUE;
}

static gboolean slice_refocus(GtkWidget        *widget,
			      GdkEventFocus       *event){
  Slice *s=SLICE(widget);
  if(!s->thumb_focus){
    s->thumb_focus=1;
    draw_and_expose(widget);
  }
  return TRUE;
}

static gboolean slice_key_press(GtkWidget *widget,GdkEventKey *event){
  Slice *s=SLICE(widget);

  return slider_key_press(s->slider,event,s->slicenum);
}

static void slice_state_changed(GtkWidget *w,GtkStateType ps){
  draw_and_expose(w);
}

static void slice_realize (GtkWidget *widget){
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
    GDK_FOCUS_CHANGE_MASK |
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

static void slice_size_allocate (GtkWidget     *widget,
				 GtkAllocation *allocation){
  //Slice *s = SLICE (widget);  
  if (GTK_WIDGET_REALIZED (widget)){
    
    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
                            allocation->width, allocation->height);
    
  }
  
  widget->allocation = *allocation;

}

static GtkWidgetClass *parent_class = NULL;

static void slice_class_init (SliceClass *class){
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  //GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  //object_class->destroy = slice_destroy;
  widget_class->realize = slice_realize;
  widget_class->expose_event = slice_expose;
  widget_class->size_request = slice_size_request;
  widget_class->size_allocate = slice_size_allocate;
  widget_class->key_press_event = slice_key_press;
  widget_class->button_press_event = slice_button_press;
  widget_class->button_release_event = slice_button_release;
  widget_class->enter_notify_event = slice_enter;
  widget_class->leave_notify_event = slice_leave;
  widget_class->motion_notify_event = slice_motion;
  widget_class->focus_out_event = slice_unfocus;
  widget_class->focus_in_event = slice_refocus;
  widget_class->state_changed = slice_state_changed;
}

static void slice_init (Slice *s){
}

GType slice_get_type (void){
  static GType m_type = 0;
  if (!m_type){
    static const GTypeInfo m_info={
      sizeof (SliceClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) slice_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (Slice),
      0,
      (GInstanceInitFunc) slice_init,
      0
    };
    
    m_type = g_type_register_static (GTK_TYPE_WIDGET, "Slider", &m_info, 0);
  }

  return m_type;
}

GtkWidget* slice_new (void (*callback)(void *,int), void *data){
  GtkWidget *ret= GTK_WIDGET (g_object_new (slice_get_type (), NULL));
  Slice *s=SLICE(ret);
  s->callback = callback;
  s->callback_data = data;
  s->thumb_active = 1;
  return ret;
}

void slice_set_active(Slice *s, int activep){
  s->thumb_active = activep;
  draw_and_expose(GTK_WIDGET(s));
}

void slice_thumb_set(Slice *s,double v){
  GtkWidget *w=GTK_WIDGET(s);
  
  if(s->thumb_val != v){
    s->thumb_val=v;
    slider_vals_bound(s->slider,s->slicenum);
    
    if(s->callback)s->callback(s->callback_data,5);
    draw_and_expose(w);
  }
}
