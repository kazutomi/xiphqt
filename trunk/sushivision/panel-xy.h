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

typedef struct sushiv_panelxy {
  GtkWidget *graph_table;
  GtkWidget *obj_table;
  GtkWidget *dim_table;

  int panel_w;
  int panel_h;

  // panel x/y don't correspond to dimensions like on other panels
  scalespace x;
  scalespace y;
  double oldbox[4];
  sushiv_scale_t *x_scale;
  sushiv_scale_t *y_scale;
  Slider *x_slider;
  Slider *y_slider;
  double x_bracket[2];
  double y_bracket[2];
  int cross_objnum;
  
  scalespace data_v; // the x scale aligned to data vector's bins
  scalespace data_i; // the 'counting' scale used to iterate for compute
  double **x_vec;
  double **y_vec;
  int req_zoom;
  int curr_zoom;
  int prev_zoom;

  mapping *mappings;
  int *linetype;
  int *pointtype;
  GtkWidget **map_pulldowns;
  GtkWidget **line_pulldowns;
  GtkWidget **point_pulldowns;
  Slider **alpha_scale;

  GtkWidget **dim_xb;

  sushiv_dimension_t *x_d;
  sushiv_dim_widget_t *x_widget;
  int x_dnum; // number of dimension within panel, not global instance
} sushiv_panelxy_t;

typedef struct {
  void (**call)(double *, double *);
  double **fout; // [function number][outval_number]

} _sushiv_bythread_cache_xy;

