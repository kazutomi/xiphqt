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

typedef struct sushiv_panel1d {

  sushiv_panel_t *link_x;
  sushiv_panel_t *link_y;

  GtkWidget *top_table;
  GtkWidget *obj_table;
  GtkWidget *dim_table;
  GtkWidget *popmenu;
  GtkWidget *graphmenu;

  int panel_w;
  int panel_h;
  int data_size;
  int serialno;
  double **data_vec;

  scalespace x;
  scalespace vs;
  scalespace y;

  int scales_init;
  double oldbox[4];
  int oldbox_active;

  int flip;
  sushiv_scale_t *range_scale;
  Slider *range_slider;
  double range_bracket[2];
  
  mapping *mappings;
  int *linetype;
  int *pointtype;
  GtkWidget **map_pulldowns;
  GtkWidget **line_pulldowns;
  GtkWidget **point_pulldowns;
  Slider **alpha_scale;

  GtkWidget **dim_xb;

  sushiv_dimension_t *x_d;
  sushiv_dim_widget_t *x_scale;
  int x_dnum; // number of dimension within panel, not global instance

  int last_line;

  int peak_count;
} sushiv_panel1d_t;

