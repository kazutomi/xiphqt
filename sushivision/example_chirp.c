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

sushiv_instance_t *s;

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

int sushiv_submain(int argc, char *argv[]){

  s=sushiv_new_instance();

  sushiv_new_dimension(s,0,"initial Hz",
		       4,(double []){1,10,100,1000},
		       NULL,SUSHIV_DIM_NO_X);
  sushiv_new_dimension(s,1,"initial amplitude",
		       3,(double []){0,.1,1},
		       NULL,SUSHIV_DIM_NO_X);
  sushiv_new_dimension(s,2,"initial phase",
		       3,(double []){-M_PI,0,M_PI},
		       NULL,SUSHIV_DIM_NO_X);
  sushiv_new_dimension(s,3,"delta amplitude",
		       3,(double []){0,-.1,-1},
		       NULL,SUSHIV_DIM_NO_X);
  sushiv_new_dimension(s,4,"delta frequency",
		       7,
		       (double []){-100*M_PI,-10*M_PI,-M_PI,0,M_PI,10*M_PI,100*M_PI},
		       NULL,
		       SUSHIV_DIM_NO_X);

  scale_set_scalelabels(s->dimension_list[4]->scale,(char *[]){"-100pi","-10pi","-pi","0","pi","10pi","100pi"});

  sushiv_new_dimension(s,5,"seconds",
		       5,(double []){0,.001,.01,.1,1},
		       NULL,0);
  
  sushiv_dimension_set_value(s,1,1,1.0);

  sushiv_new_function(s, 0, 6, 3, chirp, 0);

  sushiv_new_objective(s,0,"sin",
		       2,(double []){-1.5, 1.5},
		       (int []){0},
		       (int []){0},
		       "Y", 0);

  sushiv_new_objective(s,1,"phase",
		       2,(double []){-1.0, 1.0},
		       (int []){0},
		       (int []){1},
		       "Y", 0);

  sushiv_new_objective(s,2,"amplitude",
		       2,(double []){-1.0, 1.0},
		       (int []){0},
		       (int []){2},
		       "Y", 0);

  sushiv_new_panel_1d(s,2,"chirp",
		      s->objective_list[0]->scale,
		      (int []){0,1,2,-1},
		      (int []){0,1,2,3,4,5,-1},
		      0);

  return 0;
}
