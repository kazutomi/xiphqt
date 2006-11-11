/*
 *
 *  sushivision copyright (C) 2005 Monty <monty@xiph.org>
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
#include "scale.h"

G_BEGIN_DECLS

#define PLOT_TYPE            (plot_get_type ())
#define PLOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLOT_TYPE, Plot))
#define PLOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLOT_TYPE, PlotClass))
#define IS_PLOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLOT_TYPE))
#define IS_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLOT_TYPE))

typedef struct _Plot       Plot;
typedef struct _PlotClass  PlotClass;

struct _Plot{
  GtkWidget w;
  cairo_t         *wc;
  cairo_t         *bc;
  cairo_surface_t *back;
  cairo_surface_t *fore;
  cairo_surface_t *stage;
  int widgetfocus;

  int scalespacing;
  scalespace x;
  scalespace y;
  char *namex;
  char *namey;

  double selx;
  double sely;
  int cross_active;

  double box_x1;
  double box_y1;
  double box_x2;
  double box_y2;
  int box_active;
  void *box_data;
  void (*box_callback)(void *,int);
  int button_down;
  
  u_int32_t *datarect;
  void *app_data;
  void (*recompute_callback)(void *);
  void *cross_data;
  void (*crosshairs_callback)(void *);

  int legend_entries;
  char **legend_list;
};

struct _PlotClass{
  GtkWidgetClass parent_class;
  void (*plot) (Plot *m);
};

GType     plot_get_type        (void);
Plot     *plot_new (void (*callback)(void *),void *app_data,
		    void (*cross_callback)(void *),void *cross_data,
		    void (*box_callback)(void *,int),void *box_data);

G_END_DECLS

// the widget subclass half
void plot_expose_request(Plot *p);
void plot_expose_request_line(Plot *p, int num);
void plot_expose_request_partial(Plot *p,int x, int y, int w, int h);
void plot_set_x_scale(Plot *p, scalespace x);
void plot_set_y_scale(Plot *p, scalespace y);
void plot_set_x_name(Plot *p, char *name);
void plot_set_y_name(Plot *p, char *name);
u_int32_t * plot_get_background_line(Plot *p, int num);
cairo_t *plot_get_background_cairo(Plot *p);
void plot_set_crosshairs(Plot *p, double x, double y);
void plot_draw_scales(Plot *p);
void plot_unset_box(Plot *p);
void plot_box_vals(Plot *p, double ret[4]);
void plot_box_set(Plot *p, double vals[4]);
void plot_legend_add(Plot *p, char *entry);
void plot_legend_clear(Plot *p);
int plot_get_crosshair_xpixel(Plot *p);
int plot_get_crosshair_ypixel(Plot *p);
