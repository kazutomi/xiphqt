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

typedef struct sushiv_instance_undo sushiv_instance_undo_t;

#include <time.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "sushivision.h"

// used to glue numeric settings to semantic labels for menus/save files
typedef struct propmap {
  char *left;
  int value;

  char *right;
  struct propmap **submenu;
  void (*callback)(sushiv_panel_t *, GtkWidget *);
} propmap;

#include "mapping.h"
#include "slice.h"
#include "spinner.h"
#include "slider.h"
#include "scale.h"
#include "plot.h"
#include "dimension.h"
#include "objective.h"
#include "panel-1d.h"
#include "panel-2d.h"
#include "xml.h"
#include "gtksucks.h"

union sushiv_panel_subtype {
  sushiv_panel1d_t *p1;
  sushiv_panel2d_t *p2;
};

// undo history; the panel types vary slightly, but only slightly, so
// for now we use a master undo type which leaves one or two fields
// unused for a given panel.
typedef struct sushiv_panel_undo {
  int *mappings;
  double *scale_vals[3];
  
  int x_d;
  int y_d;

  double box[4];
  int box_active;

  int grid_mode;
  int cross_mode;
  int legend_mode;
  int bg_mode;
  int text_mode;
  int menu_cursamp;
  int oversample_n;
  int oversample_d;

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

  int menu_cursamp;
  int oversample_n;
  int oversample_d;
  int def_oversample_n;
  int def_oversample_d;


  // function bundles 
  void (*realize)(sushiv_panel_t *p);
  int (*map_action)(sushiv_panel_t *p, _sushiv_bythread_cache *c);
  int (*legend_action)(sushiv_panel_t *p);
  int (*compute_action)(sushiv_panel_t *p, _sushiv_bythread_cache *c);
  void (*request_compute)(sushiv_panel_t *p);
  void (*crosshair_action)(sushiv_panel_t *p);
  void (*print_action)(sushiv_panel_t *p, cairo_t *c, int w, int h);
  int (*save_action)(sushiv_panel_t *p, xmlNodePtr pn);
  int (*load_action)(sushiv_panel_t *p, sushiv_panel_undo_t *u, xmlNodePtr pn, int warn);

  void (*undo_log)(sushiv_panel_undo_t *u, sushiv_panel_t *p);
  void (*undo_restore)(sushiv_panel_undo_t *u, sushiv_panel_t *p);
};

struct sushiv_instance_undo {
  sushiv_panel_undo_t *panels;
  double *dim_vals[3];
};

struct sushiv_instance_internal {
  int undo_level;
  int undo_suspend;
  sushiv_instance_undo_t **undo_stack;
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

extern void _sushiv_undo_log(sushiv_instance_t *s);
extern void _sushiv_undo_push(sushiv_instance_t *s);
extern void _sushiv_undo_pop(sushiv_instance_t *s);
extern void _sushiv_undo_suspend(sushiv_instance_t *s);
extern void _sushiv_undo_resume(sushiv_instance_t *s);
extern void _sushiv_undo_restore(sushiv_instance_t *s);
extern void _sushiv_undo_up(sushiv_instance_t *s);
extern void _sushiv_undo_down(sushiv_instance_t *s);

extern void _sushiv_panel_undo_log(sushiv_panel_t *p, sushiv_panel_undo_t *u);
extern void _sushiv_panel_undo_restore(sushiv_panel_t *p, sushiv_panel_undo_t *u);

extern void _sushiv_panel1d_mark_recompute_linked(sushiv_panel_t *p); 
extern void _sushiv_panel1d_update_linked_crosshairs(sushiv_panel_t *p, int xflag, int yflag); 
extern void _sushiv_panel_update_menus(sushiv_panel_t *p);

extern int save_main();
extern int load_main();
extern int _save_panel(sushiv_panel_t *p, xmlNodePtr instance);
extern int _load_panel(sushiv_panel_t *p, sushiv_panel_undo_t *u, xmlNodePtr instance, int warn);
extern void first_load_warning(int *);

extern sig_atomic_t _sushiv_exiting;
extern char *filebase;
extern char *filename;
extern char *dirname;
extern char *cwdname;
