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

sv_instance_t *s;

static void fractal_objective(double *d, double *ret){
  int max_iter = d[4];
  int i;
  double z, zi, zz;
  double c=d[0],ci=d[1];
  
  ret[0]=NAN;
  ret[1]=1.;

  z = d[2]; zi = d[3];
  for(i=0;i<max_iter;i++){
    zz = z*z - zi*zi + c;
    zi = 2.0*z*zi + ci;
    z  = zz;
    if (z*z + zi*zi > 4.0){
      ret[0] = (double)i/max_iter;
      ret[1] = NAN;
      return;
    }
  }

  ret[1] = sqrt(z*z + zi*zi)/4.;
}

int sv_submain(int argc, char *argv[]){

  sv_instance_t *s = sv_new(0,"fractal");
  
  sv_dim_t *d0 = sv_dim_new(s, 0, "Re(c)", 0);
  sv_dim_make_scale(d0, 5, (double []){-2.25,-0.75,0,0.25,0.75}, NULL, 0);
  
  sv_dim_t *d1 = sv_dim_new(s, 1, "Im(c)", 0);
  sv_dim_make_scale(d1, 5, (double []){-2,-1,0,1,2}, NULL, 0);

  sv_dim_t *d2 = sv_dim_new(s, 2, "Re(z0)", 0);
  sv_dim_make_scale(d2, 5, (double []){-2.25,-1,0,1,2.25}, NULL, 0);

  sv_dim_t *d3 = sv_dim_new(s, 3, "Im(z0)", 0);
  sv_dim_make_scale(d3, 5, (double []){-2.25,-1,0,1,2.25}, NULL, 0);

  sv_dim_t *d4 = sv_dim_new(s, 4, "Max Iterations", 0);
  sv_dim_make_scale(d4, 4, (double []){100,1000,10000,100000},
		    (char *[]){"one hundred",
				 "one thousand",
				 "ten thousand",
				 "one hundred thousand"}, 0);
  sv_dim_set_picklist(d4);
  sv_dim_set_value(d4,1,100);

  sv_func_t *f = sv_func_new(s, 0, 2, fractal_objective, 0);
  
  sv_obj_t *o0 = sv_obj_new(s,0,"outer",
			    (sv_func_t *[]){f},
			    (int []){0},
			    "Y", 0);
  sv_obj_make_scale(o0, 5, (double []){0, .001, .01, .1, 1.0}, NULL, 0);
  
  sv_obj_t *o1 = sv_obj_new(s,1,"inner",
			    (sv_func_t *[]){f},
			    (int []){1},
			    "Y", 0);
  sv_obj_make_scale(o1, 5, (double []){0, .001, .01, .1, 1.0}, NULL, 0);
  
  sv_panel_new_2d(s,0,"Mandel/Julia Fractal",
		  (sv_obj_t *[]){o0,o1,NULL},
		  (sv_dim_t *[]){d0,d1,d2,d3,d4,NULL},
		  0);
  
  return 0;
}

/* sushiv_atexit is entirely optional and may be ommitted */
int sv_atexit(void){
  fprintf(stderr,"Done!\n");
  return 0;
}
