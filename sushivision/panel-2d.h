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

// undo history
typedef struct sushiv_panel2d_undo {
  int *mappings;

  double *obj_vals[3];
  double *dim_vals[3];
  
  int x_d;
  int y_d;

  double box[4];
  int box_active;
} sushiv_panel2d_undo_t;

typedef struct sushiv_panel2d {

  GtkWidget *toplevel;
  GtkWidget *graph;
  GtkWidget *top_table;
  GtkWidget *dim_table;

  int data_w;
  int data_h;
  int serialno;
  double **data_rect;
  scalespace x;
  scalespace y;
  int scales_init;
  double oldbox[4];
  int oldbox_active;

  mapping *mappings;
  Slider    **range_scales;
  GtkWidget **range_pulldowns;
  double *alphadel;

  Slider **dim_scales;
  GtkWidget **dim_xb;
  GtkWidget **dim_yb;

  sushiv_dimension_t *x_d;
  sushiv_dimension_t *y_d;
  Slider *x_scale;
  Slider *y_scale;
  int x_dnum; // number of dimension within panel, not global instance
  int y_dnum; // number of dimension within panel, not global instance

  int last_line;
  int dirty_flag;

  sushiv_panel2d_undo_t **undo_stack;
  int undo_level;
  int undo_suspend;

} sushiv_panel2d_t;

extern void _sushiv_realize_panel2d(sushiv_panel_t *p);
extern int _sushiv_panel_cooperative_compute_2d(sushiv_panel_t *p);
extern void _sushiv_panel2d_map_redraw(sushiv_panel_t *p);
extern void _sushiv_panel2d_legend_redraw(sushiv_panel_t *p);
