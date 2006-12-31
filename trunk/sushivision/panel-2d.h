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

typedef struct sushiv_panel2d {

  GtkWidget *top_table;
  GtkWidget *dim_table;
  GtkWidget *popmenu;
  GtkWidget *graphmenu;

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

  GtkWidget **dim_xb;
  GtkWidget **dim_yb;

  sushiv_dimension_t *x_d;
  sushiv_dimension_t *y_d;
  sushiv_dim_widget_t *x_scale;
  sushiv_dim_widget_t *y_scale;
  int x_dnum; // number of dimension within panel, not global instance
  int y_dnum; // number of dimension within panel, not global instance

  int last_line;
  int dirty_flag;

  int peak_count;

} sushiv_panel2d_t;

