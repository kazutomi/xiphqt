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

// plane mutex locking order: forward in plane list, down the plane struct

typedef union  _sv_plane _sv_plane_t;
typedef struct _sv_plane_proto _sv_plane_proto_t;
typedef struct _sv_plane_bg _sv_plane_bg_t;
typedef struct _sv_plane_2d _sv_plane_2d_t;

struct _sv_plane_proto {
  // in common with all plane types

  // only GTK/API manipulates the common data
  // GTK/API read access: GDK lock
  // GTK/API write access: GDK lock -> panel.memlock(write);
  // worker read access: panel.memlock(read)
  int              plane_type;
  sv_obj_t        *o;
  _sv_plane_t     *share_next;
  _sv_plane_t     *share_prev;
  _sv_panel2d_t   *panel;
};

struct _sv_plane_bg {
  int              plane_type;
  sv_obj_t        *o;
  _sv_plane_t     *share_next;
  _sv_plane_t     *share_prev;
  _sv_panel2d_t   *panel;

  // image data and concurrency tracking
   _sv_ucolor_t   *image; // panel size
  int              image_serialno;
  int              image_waiting;
  int              image_incomplete;
  int              image_nextline;
  unsigned char   *image_status; // rendering flags
  pthread_rwlock_t image_m; 

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
  int              plane_type; // == Z
  sv_obj_t        *o;
  _sv_plane_t     *share_next;
  _sv_plane_t     *share_prev;
  _sv_panel2d_t   *panel;

  // subtype 
  // objective data
  float           *data;              // data size
  _sv_scalespace_t data_x;
  _sv_scalespace_t data_y;
  _sv_scalespace_t data_x_it;
  _sv_scalespace_t data_y_it;
  int              data_serialno;     
  int              data_xscale_waiting; 
  int              data_xscale_incomplete; 
  int              data_yscale_waiting;   
  int              data_yscale_incomplete;   
  int              data_compute_waiting;
  int              data_compute_incomplete;
  int              data_nextline;
  unsigned char   *data_flags;
  pthread_mutex_t  data_m;

  // image plane
  _sv_ucolor_t    *image; // panel size;
  int              image_serialno;
  _sv_scalespace_t image_x;
  _sv_scalespace_t image_y;
  struct sv_zmap   image_map;
  int              image_waiting;
  int              image_incomplete;
  int              image_nextline;
  unsigned char   *image_flags;
  pthread_mutex_t  image_m;

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
  pthread_rwlock_t memlock;  
  pthread_rwlock_t datalock; 

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
