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

// plane mutex policy:  
//
//  gdk_m -> panel_m -> obj_m -> dim_m -> scale_m
//  |
//  -> z.data_m -> z.image_m -> bg.image_m 
//  |
//  -> z.status_m -> bg.status_m

#define PLANE_Z 1
 
typedef union  _sv_plane _sv_plane_t;
typedef struct _sv_plane_proto _sv_plane_proto_t;
typedef struct _sv_plane_bg _sv_plane_bg_t;
typedef struct _sv_plane_2d _sv_plane_2d_t;

struct _sv_plane_proto {
  // in common with all plane types
  // common fields locked via panel
  int plane_type;
  sv_obj_t *o;
  _sv_plane_t *share_next;
  _sv_plane_t *share_prev;
  _sv_panel2d_t   *panel;
};

// bg plane mutex policy: [mutexes from one other plane] -> image -> status -> [back to other plane]
struct _sv_plane_bg {
  // in common with all plane types
  // common fields locked via panel
  int             plane_type;
  sv_obj_t       *o;
  _sv_plane_t    *share_next;
  _sv_plane_t    *share_prev;
  _sv_panel2d_t  *panel;

  // image data
   _sv_ucolor_t  *image; // panel size
  int             image_serialno;
  int             image_waiting;
  int             image_incomplete;
  int             image_nextline;
  pthread_mutex_t image_m; 

  // concurrency tracking
  unsigned char  *line_status; // rendering flags
  pthread_mutex_t status_m;
};

struct sv_zmap {

  double *label_vals;
  int labels;
  int neg;
  double al;
  double lo;
  double hi;
  double lodel;
  double *labeldelB;
  double *labelvalB;

};

// z-plane mutex policy: data -> image plane -> [bg mutextes] -> status
struct _sv_plane_2d {
  // in common with all plane types
  // common fields locked via panel
  int             plane_type; // == Z
  sv_obj_t       *o;
  _sv_plane_t    *share_next;
  _sv_plane_t    *share_prev;
  _sv_panel2d_t  *panel;

  // subtype 
  // objective data
  float          *data;              // data size
  int             data_serialno;     
  int             xscale_waiting; 
  int             xscale_incomplete; 
  int             yscale_waiting;   
  int             yscale_incomplete;   
  int             compute_waiting;
  int             compute_incomplete;
  int             data_nextline;
  pthread_mutex_t data_m;

  // image plane
  _sv_ucolor_t   *image; // panel size;
  int             image_serialno;
  struct sv_zmap  image_map;
  int             image_waiting;
  int             image_incomplete;
  int             image_nextline;
  pthread_mutex_t image_m;

  // concurrency tracking
  unsigned char  *lineflags; // rendering flags
  pthread_mutex_t status_m;

  // ui elements; use gdk lock
  _sv_mapping_t   *mapping;
  _sv_slider_t    *scale;
  GtkWidget       *range_pulldown;
  double           alphadel;

};

union {
  _sv_plane_proto_t proto;
  _sv_plane_bg_t bg;

  _sv_plane_2d_t p2d;
} _sv_plane;

typedef struct {

  GtkWidget *obj_table;
  GtkWidget *dim_table;

  _sv_plane_bg_t *bg;

  int planes;
  _sv_plane_t **plane_list;
  int next_plane; 
  
  /* cached z-plane resampling helpers */
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

  GtkWidget **dim_xb; // X axis selector buttons
  GtkWidget **dim_yb; // Y axis selector buttons

  _sv_dim_widget_t *x_scale; // pointer to current X axis dimwidget
  _sv_dim_widget_t *y_scale; // pointer to current Y axis dimwidget
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
