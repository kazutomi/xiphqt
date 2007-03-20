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

#define MAX_TEETH 100

sushiv_instance_t *s;
int mult[MAX_TEETH+1][MAX_TEETH+1];

static void inner(double *d, double *ret){
  double R = d[0];
  double r = d[1];
  double p = d[2];
  double t = d[3]*M_PI*2.*mult[(int)R][(int)r];
  
  ret[0] = (R-r) * cos(t) + p * cos((R-r)*t/r);
  ret[1] = (R-r) * sin(t) - p * sin((R-r)*t/r);
}

static void outer(double *d, double *ret){
  double R = d[0];
  double r = d[1];
  double p = d[2];
  double t = d[3]*M_PI*2.*mult[(int)R][(int)r];

  ret[0] = (R+r) * cos(t) - p * cos((R+r)*t/r);
  ret[1] = (R+r) * sin(t) + p * sin((R+r)*t/r);
}

int factored_mult(int x, int y){
  int d = 2;
  while(d<x){
    if((x / d * d) == x &&
       (y / d * d) == y){
      x/=d;
      y/=d;
    }else{
      d++;
    }
  }
  return y;
}

int sushiv_submain(int argc, char *argv[]){
  int i,j;
  for(i=0;i<=MAX_TEETH;i++)
    for(j=0;j<=MAX_TEETH;j++)
      mult[i][j] = factored_mult(i,j);

  s=sushiv_new_instance(0,"spirograph");

  sushiv_new_dimension_discrete(s,0,"ring teeth",
				2,(double []){11,MAX_TEETH},
				NULL,1,1,0);
  sushiv_new_dimension_discrete(s,1,"wheel teeth",
				2,(double []){7,MAX_TEETH},
				NULL,1,1,0);
  sushiv_new_dimension(s,2,"wheel pen",
		       2,(double []){0,MAX_TEETH},
		       NULL,0);
  sushiv_new_dimension(s,3,"trace",
		       2,(double []){0,1},
		       NULL,0);

  scale_set_scalelabels(s->dimension_list[3]->scale,(char *[]){"start","end"});

  sushiv_new_function(s, 0, 2, inner, 0);
  sushiv_new_function(s, 1, 2, outer, 0);

  sushiv_new_objective(s,0,"inner",
		       0,NULL,
		       (int []){0,0},
		       (int []){0,1},
		       "XY", 0);

  sushiv_new_objective(s,1,"outer",
		       0,NULL,
		       (int []){1,1},
		       (int []){0,1},
		       "XY", 0);

  sushiv_scale_t *axis = scale_new(3,(double []){-MAX_TEETH*3,0,MAX_TEETH*3},NULL);

  sushiv_new_panel_xy(s,2,"spirograph (TM)",
		      axis,axis,
		      (int []){0,1,-1},
		      (int []){3,0,1,2,-1},
		      0);

  sushiv_dimension_set_value(s,0,1,100);
  sushiv_dimension_set_value(s,1,1,70);
  sushiv_dimension_set_value(s,2,1,50);

  return 0;
}
