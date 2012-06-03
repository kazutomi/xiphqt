/*
 *
 *  gtk2 spectrum analyzer
 *
 *      Copyright (C) 2004-2012 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  The analyzer is distributed in the hope that it will be useful,
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

#ifndef __PLOT_H__
#define __PLOT_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "io.h"

G_BEGIN_DECLS

#define PLOT_TYPE            (plot_get_type ())
#define PLOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLOT_TYPE, Plot))
#define PLOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLOT_TYPE, PlotClass))
#define IS_PLOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLOT_TYPE))
#define IS_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLOT_TYPE))

typedef struct _Plot       Plot;
typedef struct _PlotClass  PlotClass;

typedef struct {
  int bits[MAX_FILES];
  int channels[MAX_FILES];
  int rate[MAX_FILES];
  int groups;
  int total_ch;
  int maxrate;
  int reload;
  int scale;
  int mode;
  int link;
  int phase_active;

  float **data;
  float *active;
  float ymax;
  float pmax;
  float pmin;
  int height;
  int width;
  int increment;
} fetchdata;

typedef struct {
  float ymax;
  float pmax;
  float pmin;
  float depth;
  float bold;
} plotparams;

struct _Plot{

  GtkDrawingArea canvas;
  GdkPixmap *backing;
  GdkGC     *drawgc;
  GdkGC     *dashes;
  GdkGC     *graygc;
  GdkGC     *phasegc;

  PangoLayout **lin_layout_200;
  PangoLayout **lin_layout_100;
  PangoLayout **lin_layout_50;
  PangoLayout **lin_layout;
  PangoLayout **log_layout;
  PangoLayout **iso_layout;
  PangoLayout **db_layout;
  PangoLayout **db_layout1;
  PangoLayout *db_layoutdB;
  PangoLayout *db_layoutN;
  PangoLayout **imp_layout;
  PangoLayout **phase_layout;

  int configured;
  int total_ch;
  int scale;
  int maxrate;
  int phase_active;
  int width;

  float lin_major;
  float lin_minor;
  int lin_mult;

  int xgrid[20];
  int xgrids;
  int xlgrid[20];
  int xlgrids;
  int xtic[200];
  int xtics;

  int ygrid[11];
  int ygrids;
  int ytic[200];
  int ytics;

  float padx;
  float phax;
  float pady;
};

struct _PlotClass{

  GtkDrawingAreaClass parent_class;
  void (* plot) (Plot *m);
};

GType          plot_get_type        (void);
GtkWidget*     plot_new             (void);
void	       plot_draw            (Plot *m, fetchdata *f, plotparams *pp);
int            plot_get_left_pad    (Plot *m);
int            plot_get_right_pad   (Plot *m);
int            plot_height          (Plot *m);
int            plot_width           (Plot *m);

GdkColor chcolor(int ch);
extern void replot(int scale_reset, int inactive_reset, int scale_damp);

G_END_DECLS

#endif




