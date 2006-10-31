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

  mapping *mappings;
  Slider    **range_scales;
  GtkWidget **range_pulldowns;
  double *alphadel;

  Slider **dim_scales;
  GtkWidget **dim_xb;
  GtkWidget **dim_yb;

  sushiv_dimension_t *x_d;
  sushiv_dimension_t *y_d;

  int last_line;
  int dirty_flag;
} sushiv_panel2d_t;

extern void _sushiv_realize_panel2d(sushiv_panel_t *p);
extern int _sushiv_panel_cooperative_compute_2d(sushiv_panel_t *p);
extern void _sushiv_panel2d_map_redraw(sushiv_panel_t *p);
