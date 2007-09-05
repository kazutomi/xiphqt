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

typedef struct {

  GtkWidget *graph_table;
  GtkWidget *obj_table;
  GtkWidget *dim_table;

  int panel_w;
  int panel_h;
  int data_size;
  double **data_vec;

  _sv_scalespace_t y;

  _sv_scalespace_t x;   // the x scale aligned to panel's pixel context
  _sv_scalespace_t x_v; // the x scale aligned to data vector's bins
  _sv_scalespace_t x_i; // the 'counting' scale used to iterate for compute

  int scales_init;
  double oldbox[4];

  int flip;
  sv_scale_t *range_scale;
  _sv_slider_t *range_slider;
  double range_bracket[2];
  
  _sv_mapping_t *mappings;
  int *linetype;
  int *pointtype;
  GtkWidget **map_pulldowns;
  GtkWidget **line_pulldowns;
  GtkWidget **point_pulldowns;
  _sv_slider_t **alpha_scale;

  GtkWidget **dim_xb;

  _sv_dim_widget_t *x_scale;
  int x_dnum; // number of dimension within panel, not global instance
} _sv_panel1d_t;

typedef struct {
  void (**call)(double *, double *);
  double **fout; // [function number][outval_number*x]
  int storage_width;

} _sv_bythread_cache_1d_t;

