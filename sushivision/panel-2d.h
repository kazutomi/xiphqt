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

  GtkWidget *obj_table;
  GtkWidget *dim_table;

  /* only run those functions used by this panel */
  int used_functions;
  sv_func_t **used_function_list;

  unsigned char *bg_todo;
  int bg_next_line;
  int bg_first_line;
  int bg_last_line;

  /**** Y PLANES ******/
  int y_obj_num;
  int **y_map; // indirected, dw*dh
  _sv_ucolor_t **y_planes; // indirected, dw*dh
  unsigned char **y_planetodo; // indirected, dh
  int partial_remap;

  int y_next_plane; // which y plane to issue next render
  int y_next_line; // incremented when a line is claimed, per plane [0-ph)

  sv_obj_t **y_obj_list; // list of objectives with a y plane
  int *y_obj_to_panel; /* maps from position in condensed list to position in full list */
  int *y_obj_from_panel; /* maps from position in full list to position in condensed list */
  int *y_fout_offset; 
  
  /* cached resampling helpers */
  int resample_serialno;
  unsigned char *ydelA;
  unsigned char *ydelB;
  int *ynumA;
  int *ynumB;
  float yscalemul;

  /* scales and data -> display scale mapping */
  _sv_scalespace_t x;
  _sv_scalespace_t x_v;
  _sv_scalespace_t x_i;
  _sv_scalespace_t y;
  _sv_scalespace_t y_v;
  _sv_scalespace_t y_i;

  int scales_init;
  double oldbox[4];

  _sv_mapping_t    *mappings;
  _sv_slider_t    **range_scales;
  GtkWidget **range_pulldowns;
  double     *alphadel;

  GtkWidget **dim_xb;
  GtkWidget **dim_yb;

  sv_dim_t *x_d;
  sv_dim_t *y_d;
  _sv_dim_widget_t *x_scale;
  _sv_dim_widget_t *y_scale;
  int x_dnum; // panel, not global list context
  int y_dnum; // panel, not global list context

} _sv_panel2d_t;

typedef struct {
  double *fout; // [function number * outval_number]

  int **y_map; // [y_obj_list[i]][px]
  int storage_width;

  /* cached resampling helpers; x is here becasue locking overhead
     would be prohibitive to share between threads */
  int serialno;
  unsigned char *xdelA;
  unsigned char *xdelB;
  int *xnumA;
  int *xnumB;
  float xscalemul;
  
} _sv_bythread_cache_2d_t;

