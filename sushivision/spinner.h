/*
 *
 *  sushivision copyright (C) 2005-2007 Monty <monty@xiph.org>
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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/types.h>
#include <sys/time.h>

G_BEGIN_DECLS

#define SPINNER_TYPE            (_sv_spinner_get_type ())
#define SPINNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPINNER_TYPE, _sv_spinner_t))
#define SPINNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SPINNER_TYPE, _sv_spinner_class_t))
#define IS_SPINNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPINNER_TYPE))
#define IS_SPINNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SPINNER_TYPE))

typedef struct _sv_spinner       _sv_spinner_t;
typedef struct _sv_spinner_class _sv_spinner_class_t;

struct _sv_spinner{
  GtkWidget w;
  cairo_t         *wc;
  cairo_surface_t *b[9];

  int busy;
  int busy_count;
  struct timeval last;
};

struct _sv_spinner_class{
  GtkWidgetClass parent_class;
  void (*spinner) (_sv_spinner_t *m);
};

GType          _sv_spinner_get_type        (void);
_sv_spinner_t *_sv_spinner_new (void);

G_END_DECLS

// the widget subclass half
void _sv_spinner_set_busy(_sv_spinner_t *p);
void _sv_spinner_set_idle(_sv_spinner_t *p);
