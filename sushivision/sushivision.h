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

#ifndef _SUSHIVISION_
#define _SUSHIVISION_

typedef struct sv_panel sv_panel_t;
typedef struct sv_dim   sv_dim_t;
typedef struct sv_obj   sv_obj_t;

/* toplevel ******************************************************/

extern int sv_init(void);
extern int sv_exit(void);
extern int sv_wake(void);
extern int sv_join(void);
extern int sv_suspend(int);
extern int sv_unsuspend(void);
extern int sv_save(char *filename);
extern int sv_load(char *filename);

/* dimensions ****************************************************/

sv_dim_t            *sv_dim_new (char *decl);

sv_dim_t                *sv_dim (char *name);

int                sv_dim_scale (char *format);

int                sv_dim_value (double val);

int              sv_dim_bracket (double lo,
				 double hi);

int       sv_dim_callback_value (int (*callback)(sv_dim_t *, void*),
				 void *callback_data);

/* objectives ****************************************************/
int                  sv_obj_new (char *decl,
				 void (*function)(double *,double *),
				 char *inputs,
				 char *outputs);

sv_obj_t                *sv_obj (char *name);

int                sv_obj_scale (char *format);

/* panels ********************************************************/

sv_panel_t        *sv_panel_new (char *decl,
				 char *obj,
				 char *dim);

sv_panel_t            *sv_panel (char *name);


int sv_panel_callback_recompute (int (*callback)(sv_panel_t *p,void *data),
				 void *data);

int           sv_panel_resample (int numerator,
				 int denominator);

int         sv_panel_background (char *background);

int               sv_panel_axis (char *axis,
				 sv_dim_t *d);

#endif
