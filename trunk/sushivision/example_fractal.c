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

#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include "sushivision.h"

#define MAX_ITER 16384

sushiv_instance_t *s;

static double fractal_objective(double *d){
  int i;
  double z, zi, zz;
  const int max_iter = d[4];
  const double c=d[0],ci=d[1];

  z = d[2]; zi = d[3];
  for(i=0;i<max_iter;i++){
    zz = z*z - zi*zi + c;
    zi = 2.0*z*zi + ci;
    z  = zz;
    if (z*z + zi*zi > 4.0) return (double)i/MAX_ITER;
  }

  return NAN;
}

int sushiv_submain(int argc, char *argv[]){

  s=sushiv_new_instance();

  sushiv_new_dimension(s,0,"Re(c)",
		       5,(double []){-2.25,-0.75,0,0.25,0.75},
		       NULL,
		       SUSHIV_X_RANGE|SUSHIV_Y_RANGE);
  sushiv_new_dimension(s,1,"Im(c)",
		       5,(double []){-2,-1,0,1,2},
		       NULL,
		       SUSHIV_X_RANGE|SUSHIV_Y_RANGE);

  sushiv_new_dimension(s,2,"Re(z0)",
		       5,(double []){-2.25,-1,0,1,2.25},
		       NULL,
		       SUSHIV_X_RANGE|SUSHIV_Y_RANGE);
  sushiv_new_dimension(s,3,"Im(z0)",
		       5,(double []){-2.25,-1,0,1,2.25},
		       NULL,
		       SUSHIV_X_RANGE|SUSHIV_Y_RANGE);

  sushiv_new_dimension(s,4,"Iter",
		       8,(double []){128,256,512,1024,2048,4096,8192,MAX_ITER},
  		       NULL,
		       0);

  sushiv_new_objective(s,0,"fractal",fractal_objective,0);

  sushiv_new_panel_2d(s,0,"Mandel/Julia Fractal",4,
		      (double []){0, .01, .1, 1.0},
		      (int []){0,-1},
		      (int []){0,1,2,3,4,-1},
		      0);
  
  return 0;
}
