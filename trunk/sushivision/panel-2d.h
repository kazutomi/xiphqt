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

#define PLANE_Z 1
 
union {
  _sv_planez_t z;
} _sv_plane_t;

typedef struct {
  // common
  int plane_type;
  int working;
  sv_obj_t *o;
  _sv_plane_t *share_next;
  _sv_plane_t *share_prev;
  
  // subtype 
  float         *data;   // native data size
  int32_t       *map;    // native data size
  _sv_ucolor_t  *buffer; // resampled data size;

  unsigned char *line_status; 
  int            progress;
} _sv_planez_t;

typedef struct {

  GtkWidget *obj_table;
  GtkWidget *dim_table;

  unsigned char *bg_todo;
  int bg_next_line;
  int bg_first_line;
  int bg_last_line;

  int planes;
  _sv_plane_t **plane_list;
  int next_plane; 
  
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

  _sv_dim_widget_t *x_scale;
  _sv_dim_widget_t *y_scale;
  int x_dnum; // panel, not global list context
  int y_dnum; // panel, not global list context

} _sv_panel2d_t;

typedef struct {
  double *fout; 
  int fout_size;

  /* cached resampling helpers; x is here becasue locking overhead
     would be prohibitive to share between threads */
  int serialno;
  unsigned char *xdelA;
  unsigned char *xdelB;
  int *xnumA;
  int *xnumB;
  float xscalemul;

} _sv_bythread_cache_2d_t;
