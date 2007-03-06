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


typedef struct {
  sushiv_dimension_list_t *dl;
  GtkWidget *t;

  /* one or the other */
  Slider *scale;
  GtkWidget *menu;
  GtkWidget *entry[3];

  /* don't rely on intuited state; that could be fragile. If we don't
     want updates to recurse, be explicit about it! */
  int center_updating;
  int bracket_updating;

  /* calls with the callback data and a state flag:
     0: begin set
     1: continue set
     2: end set
     3: programmatic modification
  */
  void (*center_callback)(sushiv_dimension_list_t *);
  void (*bracket_callback)(sushiv_dimension_list_t *);
} sushiv_dim_widget_t;

struct sushiv_dimension_internal {
  long discrete_numerator;
  long discrete_denominator;

  int widgets;
  sushiv_dim_widget_t **widget_list;
};

extern int _sushiv_dimension_scales(sushiv_dimension_t *d,
				    double lo,
				    double hi,
				    int panel_w, int data_w,
				    int spacing,
				    char *legend,
				    scalespace *panel, 
				    scalespace *data, 
				    scalespace *iter);
extern int _sushiv_dimension_scales_from_panel(sushiv_dimension_t *d,
					       scalespace panel,
					       int data_w,
					       scalespace *data, 
					       scalespace *iter);

extern void _sushiv_dimension_set_value(sushiv_dim_widget_t *d, int thumb, double val);
extern void _sushiv_dim_widget_set_thumb_active(sushiv_dim_widget_t *dw, int thumb, int active);
extern sushiv_dim_widget_t *_sushiv_new_dimension_widget(sushiv_dimension_list_t *dl,   
							 void (*center_callback)(sushiv_dimension_list_t *),
							 void (*bracket_callback)(sushiv_dimension_list_t *));
extern int _save_dimension(sushiv_dimension_t *d, xmlNodePtr instance);
