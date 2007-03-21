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

#define PLOT_TYPE            (_sv_plot_get_type ())
#define PLOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLOT_TYPE, _sv_plot_t))
#define PLOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLOT_TYPE, _sv_plot_class_t))
#define IS_PLOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLOT_TYPE))
#define IS_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLOT_TYPE))

typedef struct _sv_plot       _sv_plot_t;
typedef struct _sv_plot_class _sv_plot_class_t;

struct _sv_plot{
  GtkWidget w;
  cairo_t         *wc;
  cairo_surface_t *back;
  cairo_surface_t *fore;
  cairo_surface_t *stage;
  int widgetfocus;
  int bg_inv;
  int grid_mode;
  int resizable;

  int scalespacing;
  _sv_scalespace_t x;
  _sv_scalespace_t x_v;
  _sv_scalespace_t y;
  _sv_scalespace_t y_v;
  char *namex;
  char *namey;

  double selx;
  double sely;
  int cross_active;
  int legend_active;

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
  u_int32_t *legend_colors;
  char **legend_list;

  unsigned flags;
};

struct _sv_plot_class{
  GtkWidgetClass parent_class;
  void (*plot) (_sv_plot_t *m);
};

GType       _sv_plot_get_type        (void);
_sv_plot_t *_sv_plot_new (void (*callback)(void *),void *app_data,
			  void (*cross_callback)(void *),void *cross_data,
			  void (*box_callback)(void *,int),void *box_data,
			  unsigned flags);

G_END_DECLS

// the widget subclass half
int  _sv_plot_print(_sv_plot_t *p, cairo_t *c, double page_h, void (*datarender)(void *data,cairo_t *c), void *data);
void _sv_plot_set_bg_invert(_sv_plot_t *p, int setp);
void _sv_plot_expose_request(_sv_plot_t *p);
void _sv_plot_expose_request_partial(_sv_plot_t *p,int x, int y, int w, int h);
void _sv_plot_set_x_scale(_sv_plot_t *p, _sv_scalespace_t x);
void _sv_plot_set_y_scale(_sv_plot_t *p, _sv_scalespace_t y);
void _sv_plot_set_x_name(_sv_plot_t *p, char *name);
void _sv_plot_set_y_name(_sv_plot_t *p, char *name);
u_int32_t * _sv_plot_get_background_line(_sv_plot_t *p, int num);
cairo_t *_sv_plot_get_background_cairo(_sv_plot_t *p);
void _sv_plot_set_crosshairs(_sv_plot_t *p, double x, double y);
void _sv_plot_set_crosshairs_snap(_sv_plot_t *p, double x, double y);
void _sv_plot_snap_crosshairs(_sv_plot_t *p);
void _sv_plot_draw_scales(_sv_plot_t *p);
void _sv_plot_unset_box(_sv_plot_t *p);
void _sv_plot_box_vals(_sv_plot_t *p, double ret[4]);
void _sv_plot_box_set(_sv_plot_t *p, double vals[4]);
void _sv_plot_legend_add(_sv_plot_t *p, char *entry);
void _sv_plot_legend_add_with_color(_sv_plot_t *p, char *entry, u_int32_t color);
void _sv_plot_legend_clear(_sv_plot_t *p);
int  _sv_plot_get_crosshair_xpixel(_sv_plot_t *p);
int  _sv_plot_get_crosshair_ypixel(_sv_plot_t *p);
void _sv_plot_set_grid(_sv_plot_t *p, int mode);

void _sv_plot_do_enter(_sv_plot_t *p);
void _sv_plot_set_crossactive(_sv_plot_t *p, int active);
void _sv_plot_toggle_legend(_sv_plot_t *p);
void _sv_plot_set_legendactive(_sv_plot_t *p, int active);

void _sv_plot_resizable(_sv_plot_t *p, int rp);

#define _SV_PLOT_NO_X_CROSS 1
#define _SV_PLOT_NO_Y_CROSS 2

#define _SV_PLOT_GRID_NONE   0
#define _SV_PLOT_GRID_NORMAL 4
#define _SV_PLOT_GRID_TICS   8
#define _SV_PLOT_GRID_LIGHT  (256+4)
#define _SV_PLOT_GRID_DARK   (512+4)

#define _SV_PLOT_GRID_MODEMASK  0x00fc
#define _SV_PLOT_GRID_COLORMASK 0x0f00

#define _SV_PLOT_TEXT_DARK 0
#define _SV_PLOT_TEXT_LIGHT 1

#define _SV_PLOT_LEGEND_NONE 0
#define _SV_PLOT_LEGEND_SHADOW 1
#define _SV_PLOT_LEGEND_BOX 2
