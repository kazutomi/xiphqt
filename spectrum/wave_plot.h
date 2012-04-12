/*
 *
 *  gtk2 waveform viewer
 *    
 *      Copyright (C) 2004 Monty
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
#include <gtk/gtkcontainer.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkdrawingarea.h>

G_BEGIN_DECLS

#define PLOT_TYPE            (plot_get_type ())
#define PLOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLOT_TYPE, Plot))
#define PLOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLOT_TYPE, PlotClass))
#define IS_PLOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLOT_TYPE))
#define IS_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLOT_TYPE))

typedef struct _Plot       Plot;
typedef struct _PlotClass  PlotClass;

struct _Plot{

  GtkDrawingArea canvas;  
  GdkPixmap *backing;
  GdkGC     *drawgc;

  PangoLayout ***x_layout;
  PangoLayout ****y_layout;

  int configured;
  float **ydata;
  int size;

  int groups;
  int *ch;
  int *ch_active;
  int total_ch;

  int range;
  int scale;
  int interval;
  int span;

  int rchoice;
  int schoice;
  int spanchoice;

  int *blockslice;
  int *overslice;

  int *rate;

  int xgrid[20];
  int xgrids;
  int xtic[200];
  int xtics;

  int ygrid[5];
  int ygrids;
  int ytic[50];
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
GtkWidget*     plot_new             (int n, int inputs, int *channels, int *rate);
void	       plot_refresh         (Plot *m);
void	       plot_setting         (Plot *p, int range, int scale, int interval, int span,
                                     int rangechoice, int scalechoice, int spanchoice,
                                     int *blockslice, int *overslice);
void	       plot_draw            (Plot *m);
void	       plot_clear           (Plot *m);
int 	       plot_width           (Plot *m);
float**        plot_get             (Plot *m);
void           plot_set_active      (Plot *m, int *);

GdkColor chcolor(int ch);

G_END_DECLS

#endif




