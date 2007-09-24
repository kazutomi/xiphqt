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

typedef union  sv_plane sv_plane_t;
typedef struct sv_plane_bg sv_plane_bg_t;
typedef struct sv_plane_2d sv_plane_2d_t;

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

struct sv_plane_common {
  int              plane_type;
  sv_obj_t        *o;
  sv_plane_t      *share_next;
  sv_plane_t      *share_prev;
  sv_panel_t      *panel;

  void (*recompute_setup)(sv_plane_t *, sv_panel_t *);
  int (*image_resize)(sv_plane_t *, sv_panel_t *);
  int (*data_resize)(sv_plane_t *, sv_panel_t *);
  int (*image_work)(sv_plane_t *, sv_panel_t *);
  int (*data_work)(sv_plane_t *, sv_panel_t *);
} sv_plane_common_t;

struct sv_plane_bg {
  sv_plane_common_t c;

  // image data and concurrency tracking
  sv_ucolor_t    *image;

  // status
  int              image_serialno;
  int              image_waiting;
  int              image_incomplete;
  int              image_nextline;
  sv_scalespace_t  image_x;
  sv_scalespace_t  image_y;
  unsigned char   *image_status; // rendering flags

};

struct sv_plane_2d {
  sv_plane_common_t c;

  // data
  float           *data;  
  float           *pending_data;  
  sv_ucolor_t     *image;
  sv_ucolor_t     *pending_image;
  int             *map;

  // status 
  sv_scalespace_t  data_x;
  sv_scalespace_t  data_y;
  sv_scalespace_t  pending_data_x;
  sv_scalespace_t  pending_data_y;
  int              data_waiting; 
  int              data_incomplete; 
  int              data_task;
  int              data_next;

  int              image_serialno;
  sv_scalespace_t  image_x; 
  sv_scalespace_t  image_y;
  int              image_task;
  int              image_next;
  int             *image_flags;
  
  // remap payload

  // resampling helpers
  unsigned char   *resample_xdelA;
  unsigned char   *resample_xdelB;
  int             *resample_xnumA;
  int             *resample_xnumB;
  float            resample_xscalemul;

  unsigned char   *resample_ydelA;
  unsigned char   *resample_ydelB;
  int             *resample_ynumA;
  int             *resample_ynumB;
  float            resample_yscalemul;

  // ui elements; use gdk lock
  sv_mapping_t    *mapping;
  sv_slider_t     *scale;
  GtkWidget       *range_pulldown;
  double           alphadel;

};

typedef union {
  sv_plane_bg_t    pbg;
  sv_plane_2d_t    p2d;
} sv_plane_t;

