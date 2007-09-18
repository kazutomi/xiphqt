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

  // although we lock/protect the data and image memory allocation, we
  // generally don't write-lock updates to the data/image planes.
  // Because any write operation finishes with a status update that
  // flushes changes out to the next stage and all data flows in only
  // one direction in the rendering pipeline, any inconsistent/stale
  // data is corrected as soon as complete data is available.  

  float           *data;              // data size
  _sv_scalespace_t data_x;
  _sv_scalespace_t data_y;
  _sv_scalespace_t data_x_it;
  _sv_scalespace_t data_y_it;
  _sv_ucolor_t    *image; // panel size;
  _sv_scalespace_t image_x;
  _sv_scalespace_t image_y;

  // a data read lock is also used for coordinated non-exclusive
  // writes to different parts of the array; data flow is set up such
  // that reading inconsistent data/image values is only ever cosmetic
  // and temporary; event ordering will always guarantee consistent
  // values are flushed forward when a write is completed.  write
  // locking is only used to enforce serialized access to prevent
  // structural or control inconsistency.
  pthread_rwlock_t  data_m; 

  int              map_serialno;
  int              task;
  int              data_waiting; 
  int              data_incomplete; 
  int              data_next;
  int              image_next;
  int             *image_flags;
  // status locked by panel

  // resampling helpers; locked via data_lock/data_serialno
  unsigned char   *resample_xdelA;
  unsigned char   *resample_xdelB;
  int             *resample_xnumA;
  int             *resample_xnumB;
  float            resample_xscalemul;

  unsigned char   *resample_ydelA;
  unsigned char   *resample_ydelB;
  int             *resample_ynumA;
  int             *resample_ynumB;
  float           *resample_yscalemul;

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
  pthread_rwlock_t activelock; 
  pthread_mutex_t  panellock;
  int              busy;

  // pending computation payload
  int               recompute_pending;
  _sv_scalespace_t  plot_x;
  _sv_scalespace_t  plot_y;
  double           *dim_lo;
  double           *dim_v;
  double           *dim_hi;

  // composite 'background' plane
  _sv_plane_bg_t *bg;

  // objective planes
  int planes;
  _sv_plane_t **plane_list;
  int next_plane; 
  pthread_mutex_t  planelock; // locks plane status, not data
  
  // UI elements
  GtkWidget *obj_table;
  GtkWidget *dim_table;

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


} _sv_bythread_cache_2d_t;
