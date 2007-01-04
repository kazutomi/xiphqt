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

static void fractal_objective(double *d, double *ret){
  int max_iter = d[4];
  int i;
  double z, zi, zz;
  const double c=d[0],ci=d[1];
  
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

int sushiv_submain(int argc, char *argv[]){

  s=sushiv_new_instance();

  sushiv_new_dimension(s,0,"Re(c)",
		       5,(double []){-2.25,-0.75,0,0.25,0.75},
		       NULL,0);
  sushiv_new_dimension(s,1,"Im(c)",
		       5,(double []){-2,-1,0,1,2},
		       NULL,0);

  sushiv_new_dimension(s,2,"Re(z0)",
		       5,(double []){-2.25,-1,0,1,2.25},
		       NULL,0);
  sushiv_new_dimension(s,3,"Im(z0)",
		       5,(double []){-2.25,-1,0,1,2.25},
		       NULL,0);

  sushiv_new_dimension_picklist(s,4,"Max Iterations",
  				4,
  				(double []){100,1000,10000,100000},
  				(char *[]){"one hundred",
  					     "one thousand",
  					     "ten thousand",
  					     "one hundred thousand"},
				NULL,0);
  sushiv_dimension_set_value(s,4,1,10000);

  sushiv_new_function(s, 0, 5, 2, fractal_objective, 0);

  sushiv_new_objective(s,0,"outer",
		       5,(double []){0, .001, .01, .1, 1.0},
		       (int []){0},
		       (int []){0},
		       "Y", 0);

  sushiv_new_objective(s,1,"inner",
		       5,(double []){0, .001, .01, .1, 1.0},
		       (int []){0},
		       (int []){1},
		       "Y", 0);
  
  sushiv_new_panel_2d(s,0,"Mandel/Julia Fractal",
		      (int []){0,1,-1},
		      (int []){0,1,2,3,4,-1},
		      0);

  sushiv_new_panel_1d_linked(s,1,"X Slice",s->objective_list[0]->scale,
			     (int []){0,1,-1},
			     0,0);

  sushiv_new_panel_1d_linked(s,2,"Y Slice",s->objective_list[0]->scale,
			     (int []){0,1,-1},
			     0,SUSHIV_PANEL_LINK_Y | SUSHIV_PANEL_FLIP);
  
  return 0;
}

/* sushiv_atexit is entirely optional and may be ommitted */
int sushiv_atexit(void){
  fprintf(stderr,"Done!\n");
  return 0;

}
