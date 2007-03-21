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

int sv_submain(int argc, char *argv[]){
  int i,j;
  for(i=0;i<=MAX_TEETH;i++)
    for(j=0;j<=MAX_TEETH;j++)
      mult[i][j] = factored_mult(i,j);

  sv_instance_t *s = sv_new(0,"spirograph");

  sv_dim_t *d0 = sv_dim_new(s,0,"ring teeth",0);
  sv_dim_make_scale(d0,2,(double []){11,MAX_TEETH},NULL,0);
  sv_dim_set_discrete(d0,1,1);

  sv_dim_t *d1 = sv_dim_new(s,1,"wheel teeth",0);
  sv_dim_make_scale(d1,2,(double []){7,MAX_TEETH},NULL,0);
  sv_dim_set_discrete(d1,1,1);
		    
  sv_dim_t *d2 = sv_dim_new(s,2,"wheel pen",0);
  sv_dim_make_scale(d2,2,(double []){0,MAX_TEETH},NULL,0);

  sv_dim_t *d3 = sv_dim_new(s,3,"trace",0);
  sv_dim_make_scale(d3,2,(double []){0,1},(char *[]){"start","end"},0);

  sv_func_t *f0 = sv_func_new(s, 0, 2, inner, 0);
  sv_func_t *f1 = sv_func_new(s, 1, 2, outer, 0);
  
  sv_obj_t *o0 = sv_obj_new(s,0,"inner",
			    (sv_func_t *[]){f0,f0},
			    (int []){0,1},
			    "XY", 0);
  
  sv_obj_t *o1 = sv_obj_new(s,1,"outer",
			    (sv_func_t *[]){f1,f1},
			    (int []){0,1},
			    "XY", 0);
  
  sv_scale_t *axis = sv_scale_new(NULL,3,(double []){-MAX_TEETH*3,0,MAX_TEETH*3},NULL,0);
  
  sv_panel_new_xy(s,0,"spirograph (TM)",
		  axis,axis,
		  (sv_obj_t *[]){o0,o1,NULL},
		  (sv_dim_t *[]){d3,d0,d1,d2,NULL},
		  0);

  sv_dim_set_value(d0,1,100);
  sv_dim_set_value(d1,1,70);
  sv_dim_set_value(d2,1,50);

  return 0;
}
