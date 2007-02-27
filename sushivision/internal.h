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

#include <time.h>
#include <signal.h>
#include <gtk/gtk.h>
#include "sushivision.h"
#include "mapping.h"
#include "slice.h"
#include "spinner.h"
#include "slider.h"
#include "scale.h"
#include "gtksucks.h"
#include "plot.h"
#include "dimension.h"
#include "objective.h"
#include "panel-1d.h"
#include "panel-2d.h"

union sushiv_panel_subtype {
  sushiv_panel1d_t *p1;
  sushiv_panel2d_t *p2;
};

// undo history; the panel types vary slightly, but only slightly, so
// for now we use a master undo type which leaves one or two fields
// unused for a given panel.
typedef struct sushiv_panel_undo {
  u_int32_t *mappings;

  double *scale_vals[3];
  double *dim_vals[3];
  
  int x_d;
  int y_d;

  double box[4];
  int box_active;
} sushiv_panel_undo_t;

typedef union {
  _sushiv_bythread_cache_1d p1;
  _sushiv_bythread_cache_2d p2;
} _sushiv_bythread_cache;

struct sushiv_panel_internal {
  GtkWidget *toplevel;
  GtkWidget *topbox;
  GtkWidget *plotbox;
  GtkWidget *graph;
  Spinner *spinner;
  GtkWidget *popmenu;

  enum sushiv_background bg_type;
  sushiv_dim_widget_t **dim_scales;
  int oldbox_active;

  int realized;

  int map_active;
  int map_progress_count;
  int map_complete_count;
  int map_serialno;

  int legend_active;
  int legend_progress_count;
  int legend_complete_count;
  int legend_serialno;

  int plot_active;
  int plot_progress_count;
  int plot_complete_count;
  int plot_serialno;

  time_t last_map_throttle;

  int oversample_n;
  int oversample_d;
  int def_oversample_n;
  int def_oversample_d;

  int menu_cursamp;

  // function bundles 
  void (*realize)(sushiv_panel_t *p);
  int (*map_action)(sushiv_panel_t *p, _sushiv_bythread_cache *c);
  int (*legend_action)(sushiv_panel_t *p);
  int (*compute_action)(sushiv_panel_t *p, _sushiv_bythread_cache *c);
  void (*request_compute)(sushiv_panel_t *p);
  void (*crosshair_action)(sushiv_panel_t *p);
  void (*print_action)(sushiv_panel_t *p, cairo_t *c, int w, int h);

  void (*undo_log)(sushiv_panel_undo_t *u, sushiv_panel_t *p);
  void (*undo_restore)(sushiv_panel_undo_t *u, sushiv_panel_t *p);
};

struct sushiv_instance_internal {
  int undo_level;
  int undo_suspend;
  sushiv_panel_undo_t **undo_stack;
};

extern void _sushiv_realize_panel(sushiv_panel_t *p);
extern void _sushiv_clean_exit(int sig);
extern void _sushiv_wake_workers();

extern int _sushiv_new_panel(sushiv_instance_t *s,
			     int number,
			     const char *name, 
			     int *objectives,
			     int *dimensions,
			     unsigned flags);

extern void set_map_throttle_time(sushiv_panel_t *p);
extern void _sushiv_panel_dirty_map(sushiv_panel_t *p);
extern void _sushiv_panel_dirty_map_throttled(sushiv_panel_t *p);
extern void _sushiv_panel_dirty_legend(sushiv_panel_t *p);
extern void _sushiv_panel_dirty_plot(sushiv_panel_t *p);
extern void _sushiv_panel_recompute(sushiv_panel_t *p);
extern void _sushiv_panel_clean_map(sushiv_panel_t *p);
extern void _sushiv_panel_clean_legend(sushiv_panel_t *p);
extern void _sushiv_panel_clean_plot(sushiv_panel_t *p);

extern void _sushiv_panel_undo_log(sushiv_panel_t *p);
extern void _sushiv_panel_undo_push(sushiv_panel_t *p);
extern void _sushiv_panel_undo_suspend(sushiv_panel_t *p);
extern void _sushiv_panel_undo_resume(sushiv_panel_t *p);
extern void _sushiv_panel_undo_restore(sushiv_panel_t *p);
extern void _sushiv_panel_undo_up(sushiv_panel_t *p);
extern void _sushiv_panel_undo_down(sushiv_panel_t *p);

extern void _sushiv_panel1d_mark_recompute_linked(sushiv_panel_t *p); 
extern void _sushiv_panel1d_update_linked_crosshairs(sushiv_panel_t *p, int xflag, int yflag); 
extern void _sushiv_panel_update_menus(sushiv_panel_t *p);

extern sig_atomic_t _sushiv_exiting;
