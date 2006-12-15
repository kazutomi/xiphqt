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

#ifndef _SUSHIVISION_
#define _SUSHIVISION_
  
typedef struct sushiv_panel sushiv_panel_t;
typedef struct sushiv_dimension sushiv_dimension_t;
typedef struct sushiv_objective sushiv_objective_t;

typedef struct sushiv_instance {
  int dimensions;
  sushiv_dimension_t **dimension_list;

  int objectives;
  sushiv_objective_t **objective_list;

  int panels;
  struct sushiv_panel **panel_list;

  void *internal;
} sushiv_instance_t;

#define SUSHIV_X_RANGE 0x100
#define SUSHIV_Y_RANGE 0x200

struct sushiv_dimension{ 
  int number;
  char *name;
  double bracket[2];
  double val;

  int scale_vals;
  double *scale_val_list;
  char **scale_label_list; 
  unsigned flags;
  
  int (*callback)(sushiv_dimension_t *);
  sushiv_panel_t *panel;
  sushiv_instance_t *sushi;
  void *internal;
};

struct sushiv_objective { 
  int number;
  char *name;

  int scale_vals;
  double *scale_val_list;
  char **scale_label_list; 
  unsigned flags;

  double (*callback)(double[]);
  sushiv_panel_t *panel;
  sushiv_instance_t *sushi;
  void *internal;
};

enum sushiv_panel_type { SUSHIV_PANEL_1D, SUSHIV_PANEL_2D };
 
struct sushiv_panel {
  int number;
  char *name;
  enum sushiv_panel_type type;
  int realized;
  int maps_dirty;
  int legend_dirty;

  int dimensions;
  sushiv_dimension_t **dimension_list;
  int objectives;
  sushiv_objective_t **objective_list;

  sushiv_instance_t *sushi;
  void *internal;
  unsigned flags;
};

extern sushiv_instance_t *sushiv_new_instance(void);

extern int sushiv_new_dimension(sushiv_instance_t *s,
				int number,
				const char *name,
				unsigned scalevals, 
				double *scaleval_list,
				int (*callback)(sushiv_dimension_t *),
				unsigned flags);
extern int sushiv_dim_set_scale(sushiv_dimension_t *d, 
				unsigned scalevals, 
				double *scaleval_list);
extern int sushiv_dim_set_scalelabels(sushiv_dimension_t *d, 
				      char **scalelabel_list);

extern int sushiv_new_objective(sushiv_instance_t *s,
				int number,
				const char *name,
				unsigned scalevals,
				double *scaleval_list,
				double (*callback)(double *),
				unsigned flags);
extern int sushiv_objective_set_scale(sushiv_objective_t *o, 
				      unsigned scalevals, 
				      double *scaleval_list);
extern int sushiv_objective_set_scalelabels(sushiv_objective_t *o, 
					    char **scalelabel_list);

extern int sushiv_new_panel_2d(sushiv_instance_t *s,
			       int number,
			       const char *name, 
			       int *objectives,
			       int *dimensions,
			       unsigned flags);

extern int sushiv_new_panel_1d(sushiv_instance_t *s,
			       int number,
			       const char *name,
			       int *objectives,
			       int *dimensions,
			       unsigned flags);

extern int sushiv_submain(int argc, char *argv[]);

#endif
