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

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "internal.h"

static void _sv_slice_draw_and_expose(GtkWidget *widget){
  _sv_slice_t *s=SLICE(widget);

  _sv_slider_draw(s->slider);
  _sv_slider_expose(s->slider);
}

static gboolean _sv_slice_expose(GtkWidget *widget, GdkEventExpose *event ){
  _sv_slice_t *s=SLICE(widget);
  _sv_slider_expose_slice(s->slider,s->slicenum);
  return FALSE;
}

static void _sv_slice_size_request (GtkWidget *widget,GtkRequisition *requisition){
  _sv_slice_t *s=SLICE(widget);
  
  _sv_slider_size_request_slice(s->slider,requisition);
}

static gint _sv_slice_motion(GtkWidget        *widget,
			 GdkEventMotion   *event){
  _sv_slice_t *s=SLICE(widget);
  _sv_slider_motion(s->slider,s->slicenum,event->x,event->y);
  
  return TRUE;
}

static gint _sv_slice_enter(GtkWidget        *widget,
			GdkEventCrossing *event){
  _sv_slice_t *s=SLICE(widget);
  _sv_slider_lightme(s->slider,s->slicenum,event->x,event->y);
  _sv_slice_draw_and_expose(widget);
  return TRUE;
}

static gint _sv_slice_leave(GtkWidget        *widget,
			GdkEventCrossing *event){
  _sv_slice_t *s=SLICE(widget);

  _sv_slider_unlight(s->slider);
  _sv_slider_draw(s->slider);
  _sv_slider_expose(s->slider);

  return TRUE;
}

static gboolean _sv_slice_button_press(GtkWidget        *widget,
				   GdkEventButton   *event){
  _sv_slice_t *s=SLICE(widget);
  if(event->button == 3)return FALSE;
  
  if(event->button == 1)
    _sv_slider_button_press(s->slider,s->slicenum,event->x,event->y);

  return TRUE;
}

static gboolean _sv_slice_button_release(GtkWidget        *widget,
				     GdkEventButton   *event){
  _sv_slice_t *s=SLICE(widget);
  if(event->button == 3)return FALSE;

  if(event->button == 1)
    _sv_slider_button_release(s->slider,s->slicenum,event->x,event->y);

  return TRUE;
}

static gboolean _sv_slice_unfocus(GtkWidget        *widget,
			      GdkEventFocus       *event){
  _sv_slice_t *s=SLICE(widget);
  if(s->thumb_focus){
    s->thumb_focus=0;
    _sv_slice_draw_and_expose(widget);
  }
  return TRUE;
}

static gboolean _sv_slice_refocus(GtkWidget        *widget,
			      GdkEventFocus       *event){
  _sv_slice_t *s=SLICE(widget);
  if(!s->thumb_focus){
    s->thumb_focus=1;
    _sv_slice_draw_and_expose(widget);
  }
  return TRUE;
}

static gboolean _sv_slice_key_press(GtkWidget *widget,GdkEventKey *event){
  _sv_slice_t *s=SLICE(widget);

  return _sv_slider_key_press(s->slider,event,s->slicenum);
}

static void _sv_slice_state_changed(GtkWidget *w,GtkStateType ps){
  _sv_slice_draw_and_expose(w);
}

static void _sv_slice_realize (GtkWidget *widget){
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

static void _sv_slice_size_allocate (GtkWidget     *widget,
				 GtkAllocation *allocation){
  //_sv_slice_t *s = SLICE (widget);  
  if (GTK_WIDGET_REALIZED (widget)){
    
    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
                            allocation->width, allocation->height);
    
  }
  
  widget->allocation = *allocation;

}

static GtkWidgetClass *parent_class = NULL;

static void _sv_slice_class_init (_sv_slice_class_t *class){
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  //GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  //object_class->destroy = slice_destroy;
  widget_class->realize = _sv_slice_realize;
  widget_class->expose_event = _sv_slice_expose;
  widget_class->size_request = _sv_slice_size_request;
  widget_class->size_allocate = _sv_slice_size_allocate;
  widget_class->key_press_event = _sv_slice_key_press;
  widget_class->button_press_event = _sv_slice_button_press;
  widget_class->button_release_event = _sv_slice_button_release;
  widget_class->enter_notify_event = _sv_slice_enter;
  widget_class->leave_notify_event = _sv_slice_leave;
  widget_class->motion_notify_event = _sv_slice_motion;
  widget_class->focus_out_event = _sv_slice_unfocus;
  widget_class->focus_in_event = _sv_slice_refocus;
  widget_class->state_changed = _sv_slice_state_changed;
}

static void _sv_slice_init (_sv_slice_t *s){
}

GType _sv_slice_get_type (void){
  static GType m_type = 0;
  if (!m_type){
    static const GTypeInfo m_info={
      sizeof (_sv_slice_class_t),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) _sv_slice_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (_sv_slice_t),
      0,
      (GInstanceInitFunc) _sv_slice_init,
      0
    };
    
    m_type = g_type_register_static (GTK_TYPE_WIDGET, "Slider", &m_info, 0);
  }

  return m_type;
}

GtkWidget* _sv_slice_new (void (*callback)(void *,int), void *data){
  GtkWidget *ret= GTK_WIDGET (g_object_new (_sv_slice_get_type (), NULL));
  _sv_slice_t *s=SLICE(ret);
  s->callback = callback;
  s->callback_data = data;
  s->thumb_active = 1;
  return ret;
}

void _sv_slice_set_active(_sv_slice_t *s, int activep){
  s->thumb_active = activep;
  if(s->active_callback)
    s->active_callback(s->active_callback_data, activep);
  _sv_slice_draw_and_expose(GTK_WIDGET(s));
}

void _sv_slice_thumb_set(_sv_slice_t *s,double v){
  GtkWidget *w=GTK_WIDGET(s);
  
  if(s->thumb_val != v){
    s->thumb_val=v;
    _sv_slider_vals_bound(s->slider,s->slicenum);
    
    if(s->callback)s->callback(s->callback_data,1);
    _sv_slice_draw_and_expose(w);
  }
}

void _sv_slice_set_active_callback(_sv_slice_t *s, void (*callback)(void *,int), void *data){
  s->active_callback = callback;
  s->active_callback_data = data;
}
