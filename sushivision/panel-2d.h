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

typedef struct sushiv_panel2d {

  GtkWidget *top_table;
  GtkWidget *dim_table;
  GtkWidget *popmenu;
  GtkWidget *graphmenu;

  /* only run those functions used by this panel */
  int used_functions;
  sushiv_function_t **used_function_list;

  /**** Y PLANES ******/
  int y_obj_num;
  int **y_map; // indirected, dw*dh

  sushiv_objective_t **y_obj_list; // list of objectives with a y plane
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
  scalespace x;
  scalespace x_v;
  scalespace x_i;
  scalespace y;
  scalespace y_v;
  scalespace y_i;

  int scales_init;
  double oldbox[4];
  int oldbox_active;

  mapping    *mappings;
  Slider    **range_scales;
  GtkWidget **range_pulldowns;
  double     *alphadel;

  GtkWidget **dim_xb;
  GtkWidget **dim_yb;

  sushiv_dimension_t *x_d;
  sushiv_dimension_t *y_d;
  sushiv_dim_widget_t *x_scale;
  sushiv_dim_widget_t *y_scale;
  int x_dnum; // panel, not global list context
  int y_dnum; // panel, not global list context

} sushiv_panel2d_t;

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
  
} _sushiv_bythread_cache_2d;

