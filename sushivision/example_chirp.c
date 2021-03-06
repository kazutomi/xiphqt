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

static void chirp(double *d, double *ret){
  double freq = d[0];
  double ampl = d[1];
  double phas = d[2];
  double damp = d[3];
  double dpha = d[4];
  double t    = d[5];
  
  /* 0: sin
     2: ph
     3: a */
  double phase_t = (phas + freq*t*M_PI*2 + dpha*t*t);
  double cycles = floor(phase_t/(M_PI*2));
  double norm_phase = phase_t - cycles*M_PI*2;
  if(norm_phase > M_PI) norm_phase -= M_PI*2;

  ret[2] = (ampl + damp*t);
  ret[1] = norm_phase / M_PI;
  ret[0] = sin(phase_t) * (ampl + damp*t);
}

int sv_submain(int argc, char *argv[]){
  
  sv_instance_t *s = sv_new(0,"chirp");
  
  sv_dim_t *d0 = sv_dim_new(s,0,"initial Hz",SV_DIM_NO_X);
  sv_dim_make_scale(d0,4,(double []){1,10,100,1000},NULL,0);
  
  sv_dim_t *d1 = sv_dim_new(s,1,"initial amplitude",SV_DIM_NO_X);
  sv_dim_make_scale(d1,3,(double []){0,.1,1},NULL,0);
  sv_dim_set_value(d1,1,1.0);
  
  sv_dim_t *d2 = sv_dim_new(s,2,"initial phase",SV_DIM_NO_X);
  sv_dim_make_scale(d2,3,(double []){-M_PI,0,M_PI},NULL,0);
  
  sv_dim_t *d3 = sv_dim_new(s,3,"delta amplitude",SV_DIM_NO_X);
  sv_dim_make_scale(d3,3,(double []){0,-.1,-1},NULL,0);
  
  sv_dim_t *d4 = sv_dim_new(s,4,"delta frequency",SV_DIM_NO_X);
  sv_dim_make_scale(d4,7,(double []){-100*M_PI,-10*M_PI,-M_PI,0,M_PI,10*M_PI,100*M_PI},
		    (char *[]){"-100pi","-10pi","-pi","0","pi","10pi","100pi"}, 0);
  
  sv_dim_t *d5 = sv_dim_new(s,5,"seconds",0);
  sv_dim_make_scale(d5,5,(double []){0,.001,.01,.1,1},NULL,0); 
  
  sv_func_t *f = sv_func_new(s, 0, 3, chirp, 0);
  
  sv_obj_t *o0 = sv_obj_new(s,0,"sin",
			    (sv_func_t *[]){f},
			    (int []){0},
			    "Y", 0);
  sv_obj_make_scale(o0, 2,(double []){-1.5, 1.5}, NULL, 0);
  
  sv_obj_t *o1 = sv_obj_new(s,1,"phase",
			    (sv_func_t *[]){f},
			    (int []){1},
			    "Y", 0);
  sv_obj_make_scale(o1, 2,(double []){-1.0, 1.0}, NULL, 0);
  
  sv_obj_t *o2 = sv_obj_new(s,2,"amplitude",
			    (sv_func_t *[]){f},
			    (int []){2},
			    "Y", 0);
  sv_obj_make_scale(o2, 2,(double []){-1.0, 1.0}, NULL, 0);
  
  sv_panel_new_1d(s,2,"chirp",
		  s->objective_list[0]->scale,
		  (sv_obj_t *[]){o0,o1,o2,NULL},
		  (sv_dim_t *[]){d0,d1,d2,d3,d4,d5,NULL},
		  0);

  return 0;
}
