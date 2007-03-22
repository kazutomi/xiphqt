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
#include "sushimacro.h"

static void discrete_objective(double *d, double *ret){

  int X = rint(d[0]);
  int Y = rint(d[1]);

  if(!(X%100) && !(Y%100)) 
    ret[0]=0.;
  else
    ret[0]=1.;
}

int sv_submain(int argc, char *argv[]){

  svm_new("alignment test");

  svm_scale_vals(-500,-10,0,10,500);
  svm_dim(0,"A");
  svm_dim_discrete(1,1);
  svm_dim_value(-2,NAN,2);

  svm_scale_vals(-500,-10,0,10,500);
  svm_dim(1,"B");
  svm_dim_discrete(1,1);
  svm_dim_value(-2,NAN,2);

  //sv_func_t *f = sv_func_new(s, 0, 1, discrete_objective, 0);
#if 0
  //sv_obj_t *o0 = sv_obj_new(s,0,"test pattern",
  //		    (sv_func_t *[]){f},
  //		    (int []){0},
  //		    "Y",0);
  //sv_obj_make_scale(o0, 2, (double []){0, 1.0}, NULL, 0);
  
  svm_obj_simple(0,"test pattern",f,"Y");

  //sv_panel_t *p2 = sv_panel_new_2d(s,0,"Discrete data example",
  //			   (sv_obj_t *[]){o0,NULL},
  //			   (sv_dim_t *[]){d0,d1,NULL},
  //			   0);
  
  svm_panel_2d(0,"Discrete data example");

  //sv_panel_t *px = sv_panel_new_1d(s,1,"X Slice", s->objective_list[0]->scale,
  //			   (sv_obj_t *[]){o0,NULL},
  //			   NULL,0);
  //sv_panel_link_1d(px, p2, SV_PANEL_LINK_X);
  
  svm_scale(0, 1.0);
  svm_obj_list(0);
  svm_panel_1d(1,"X Slice");
  svm_panel_linkx(0);

  //sv_panel_t *py = sv_panel_new_1d(s,2,"Y Slice", s->objective_list[0]->scale,
  //			   (sv_obj_t *[]){o0,NULL},
  //			   NULL,SV_PANEL_FLIP);
  //sv_panel_link_1d(py, p2, SV_PANEL_LINK_Y);

  svm_scale(0, 1.0);
  svm_obj_list(0);
  svm_panel_1d(2,"Y Slice");
  svm_panel_linky(0);
#endif  
  return 0;
}
