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
  
typedef struct sushiv_scale sushiv_scale_t;
typedef struct sushiv_panel sushiv_panel_t;
typedef struct sushiv_dimension sushiv_dimension_t;
typedef struct sushiv_objective sushiv_objective_t;

typedef struct sushiv_instance_internal sushiv_instance_internal_t;

typedef struct sushiv_instance {
  int dimensions;
  sushiv_dimension_t **dimension_list;

  int objectives;
  sushiv_objective_t **objective_list;

  int panels;
  struct sushiv_panel **panel_list;

  sushiv_instance_internal_t *private;
} sushiv_instance_t;

#define SUSHIV_NO_X 0x100
#define SUSHIV_NO_Y 0x200

struct sushiv_scale{
  int vals;
  double *val_list;
  char **label_list; 
};

typedef struct sushiv_panel_internal sushiv_panel_internal_t;

struct sushiv_dimension{ 
  int number;
  char *name;
  double bracket[2];
  double val;

  sushiv_scale_t *scale;
  unsigned flags;
  
  int (*callback)(sushiv_dimension_t *);
  sushiv_instance_t *sushi;
  sushiv_panel_internal_t *private;
};

typedef struct sushiv_objective_internal sushiv_objective_internal_t;

struct sushiv_objective { 
  int number;
  char *name;

  sushiv_scale_t *scale;
  unsigned flags;

  double (*callback)(double[]);
  sushiv_instance_t *sushi;
  sushiv_objective_internal_t *private;
};

enum sushiv_panel_type { SUSHIV_PANEL_1D, SUSHIV_PANEL_2D, SUSHIV_PANEL_XY };
typedef union sushiv_panel_subtype sushiv_panel_subtype_t;

typedef struct {
  sushiv_dimension_t *d;
  sushiv_panel_t *p;
} sushiv_dimension_list_t;

typedef struct {
  sushiv_objective_t *o;
  sushiv_panel_t *p;
} sushiv_objective_list_t;
 
struct sushiv_panel {
  int number;
  char *name;
  enum sushiv_panel_type type;

  int dimensions;
  sushiv_dimension_list_t *dimension_list;
  int objectives;
  sushiv_objective_list_t *objective_list;

  sushiv_instance_t *sushi;
  unsigned flags;

  sushiv_panel_subtype_t *subtype;
  sushiv_panel_internal_t *private;
};

extern sushiv_instance_t *sushiv_new_instance(void);

extern void scale_free(sushiv_scale_t *s);
extern sushiv_scale_t *scale_new(unsigned scalevals, double *scaleval_list);
extern int scale_set_scalelabels(sushiv_scale_t *s, char **scalelabel_list);

extern int sushiv_new_dimension(sushiv_instance_t *s,
				int number,
				const char *name,
				unsigned scalevals, 
				double *scaleval_list,
				int (*callback)(sushiv_dimension_t *),
				unsigned flags);

extern int sushiv_new_objective(sushiv_instance_t *s,
				int number,
				const char *name,
				unsigned scalevals, 
				double *scaleval_list,
				double (*callback)(double *),
				unsigned flags);

extern int sushiv_new_panel_2d(sushiv_instance_t *s,
			       int number,
			       const char *name, 
			       int *objectives,
			       int *dimensions,
			       unsigned flags);

extern int sushiv_new_panel_1d(sushiv_instance_t *s,
			       int number,
			       const char *name,
			       sushiv_scale_t *scale,
			       int *objectives,
			       int *dimensions,	
			       unsigned flags);

extern int sushiv_new_panel_1d_linked(sushiv_instance_t *s,
				      int number,
				      const char *name,
				      sushiv_scale_t *scale,
				      int *objectives,
				      sushiv_panel_t *s2d,
				      unsigned flags);

extern int sushiv_new_panel_xy(sushiv_instance_t *s,
			       int number,
			       const char *name,
			       sushiv_scale_t *xscale,
			       sushiv_scale_t *yscale,
			       int *objectives,
			       int *dimensions,	
			       unsigned flags);


extern int sushiv_submain(int argc, char *argv[]);

#endif
