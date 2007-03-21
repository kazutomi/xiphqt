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

#ifndef __SLICE_H__
#define __SLICE_H__

#include <sys/time.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtksignal.h>

G_BEGIN_DECLS

#define SLICE_TYPE            (_sv_slice_get_type ())
#define SLICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SLICE_TYPE, _sv_slice_t))
#define SLICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SLICE_TYPE, _sv_slice_class_t))
#define IS_SLICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SLICE_TYPE))
#define IS_SLICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SLICE_TYPE))

typedef struct _sv_slice       _sv_slice_t;
typedef struct _sv_slice_class _sv_slice_class_t;

typedef struct _sv_slider _sv_slider_t;

struct _sv_slice {
  GtkWidget widget;

  _sv_slider_t *slider;
  int slicenum;

  int    thumb_active;
  int    thumb_focus;
  int    thumb_state;
  int    thumb_grab;
  double thumb_val;

  void (*callback)(void *,int);
  void *callback_data;

  void (*active_callback)(void *, int);
  void *active_callback_data;

};

struct _sv_slice_class{
  GtkWidgetClass parent_class;
  void (*slice) (_sv_slice_t *s);
};

extern GType      _sv_slice_get_type (void);
extern GtkWidget* _sv_slice_new (void (*callback)(void *,int), void *data);
extern void       _sv_slice_set_active(_sv_slice_t *s, int activep);
extern void       _sv_slice_thumb_set(_sv_slice_t *s,double v);
extern void       _sv_slice_set_active_callback(_sv_slice_t *s, void (*callback)(void *,int), void *data);
G_END_DECLS
#endif
