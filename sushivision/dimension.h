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


#define SV_DIM_NO_X 0x100
#define SV_DIM_NO_Y 0x200

enum sv_dim_type { SV_DIM_CONTINUOUS, 
		   SV_DIM_DISCRETE, 
		   SV_DIM_PICKLIST};

typedef struct {
  sv_dim_list_t *dl;
  GtkWidget *t;
  
  /* one or the other */
  _sv_slider_t *scale;
  GtkWidget *menu;
  GtkWidget *entry[3];

  /* calls with the callback data and a state flag:
     0: begin set
     1: continue set
     2: end set
     3: programmatic modification
  */
  void (*center_callback)(sv_dim_list_t *);
  void (*bracket_callback)(sv_dim_list_t *);
} _sv_dim_widget_t;

typedef struct sv_dim_data{ 

  char *legend;
  enum sv_dim_type type;

  double floor;
  double lo;
  double val;
  fouble hi;
  double ceil;
  
  long numerator;
  long denominator;

  unsigned flags;
  
} sv_dim_data_t;

typedef struct sv_dim_lookup{ 

  sv_scale_t *scale;
  
  int (*callback)(sv_dim_t *);

  int widgets;
  _sv_dim_widget_t **widget_list;

  int (*value_callback)(sv_dim_t *d, void *data);
  void *value_callback_data;

} sv_dim_lookup_t;

struct sv_dim{ 
  int number;
  char *name;
  
  sv_dim_data_t data;
  sv_dim_look_t look;
};


extern int _sv_dim_scales(sv_dim_t *d,
			  double lo,
			  double hi,
			  int panel_w, int data_w,
			  int spacing,
			  char *legend,
			  _sv_scalespace_t *panel, 
			  _sv_scalespace_t *data, 
			  _sv_scalespace_t *iter);
extern int _sv_dim_scales_from_panel(sv_dim_t *d,
				     _sv_scalespace_t panel,
				     int data_w,
				     _sv_scalespace_t *data, 
				     _sv_scalespace_t *iter);

extern int _sv_dim_set_thumb(sv_dim_t *d, int thumb, double val);
extern int _sv_dim_widget_set_thumb(_sv_dim_widget_t *d, int thumb, double val);
extern void _sv_dim_widget_set_thumb_active(_sv_dim_widget_t *dw, int thumb, int active);
extern _sv_dim_widget_t *_sv_dim_widget_new(sv_dim_list_t *dl,   
					   void (*center_callback)(sv_dim_list_t *),
					   void (*bracket_callback)(sv_dim_list_t *));
extern int _sv_dim_save(sv_dim_t *d, xmlNodePtr instance);
extern int _sv_dim_load(sv_dim_t *d, _sv_undo_t *u, xmlNodePtr dn, int warn);
