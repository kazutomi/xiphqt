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

#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include "sushivision.h"

static void discrete_objective(double *d, double *ret){

  int X = rint(d[0]);
  int Y = rint(d[1]);

  if(!(X%100) && !(Y%100)) 
    ret[0]=0.;
  else
    ret[0]=1.;
}

int sv_submain(int argc, char *argv[]){

  sv_instance_t *s = sv_new(0,NULL);

  sv_dim_t *d0 = sv_dim_new(s,0,"A",0);
  sv_dim_make_scale(d0, 5,(double []){-500,-10,0,10,500}, NULL, 0);
  sv_dim_set_discrete(d0, 1, 1);

  sv_dim_t *d1 = sv_dim_new(s,1,"B",0);
  sv_dim_make_scale(d1, 5,(double []){-500,-10,0,10,500}, NULL, 0);
  sv_dim_set_discrete(d1, 1, 1);

  sv_dim_set_value(d0,0,-2);
  sv_dim_set_value(d0,2,2);
  sv_dim_set_value(d1,0,-2);
  sv_dim_set_value(d1,2,2);

  sv_func_t *f = sv_func_new(s, 0, 1, discrete_objective, 0);

  sv_obj_t *o0 = sv_obj_new(s,0,"test pattern",
			    (sv_func_t *[]){f},
			    (int []){0},
			    "Y",0);
  sv_obj_make_scale(o0, 2, (double []){0, 1.0}, NULL, 0);
  
  sv_panel_t *p2 = sv_panel_new_2d(s,0,"Discrete data example",
				   (sv_obj_t *[]){o0,NULL},
				   (sv_dim_t *[]){d0,d1,NULL},
				   0);
  
  sv_panel_t *px = sv_panel_new_1d(s,1,"X Slice", s->objective_list[0]->scale,
				   (sv_obj_t *[]){o0,NULL},
				   NULL,0);
  sv_panel_link_1d(px, p2, SV_PANEL_LINK_X);
  
  sv_panel_t *py = sv_panel_new_1d(s,2,"Y Slice", s->objective_list[0]->scale,
				   (sv_obj_t *[]){o0,NULL},
				   NULL,SV_PANEL_FLIP);
  sv_panel_link_1d(py, p2, SV_PANEL_LINK_Y);
  
  return 0;
}
