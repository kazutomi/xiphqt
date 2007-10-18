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

#define STATUS_IDLE 0
#define STATUS_BUSY 1
#define STATUS_WORKING 2

struct sv_panel {
  pthread_rwlock_t panel_m;
  pthread_mutex_t  status_m;

  int number;
  char *name;
  char *legend;

  int dimensions;
  int *dimension_list;

  int objectives;
  int *objective_list;

  // axis 0 == X 
  // axis 1 == Y 
  // axis 2 == Z
  // >2 == auxiliary axes
  char            **axis_names;
  int              *axis_dims;
  int               axes;

  // mem and data locked by status_m
  int               recompute_pending;
  int               dims; 
  int               w; 
  int               h;
  sv_dim_data_t    *dim_data;

  // locked by panel_m
  sv_plane_bg_t *bg; // composite background plane
  int planes;
  sv_plane_t **plane_list;
  int next_plane; 

  // UI objects
  GtkWidget *obj_table;
  GtkWidget *dim_table;
  sv_spinner_t *spinner;

  //sv_dimwidget_t *dw;
  //sv_objwidget_t *ow;

  double selectbox[4]; // x1,y1,x2,y2

} sv_panel_t;
