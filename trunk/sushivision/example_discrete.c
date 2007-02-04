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

static void discrete_objective(double *d, double *ret){

  int X = rint(d[0]);
  int Y = rint(d[1]);

  if(!(X%100) && !(Y%100)) 
    ret[0]=1.;
  else
    ret[0]=0.;
}

int sushiv_submain(int argc, char *argv[]){

  s=sushiv_new_instance();

  sushiv_new_dimension_discrete(s,0,"A",
				5,(double []){-500,-10,0,10,500},
				NULL,1,1,0);
  sushiv_new_dimension_discrete(s,1,"B",
				5,(double []){-500,-10,0,10,500},
				NULL,1,1,0);

  sushiv_dimension_set_value(s,0,0,-2);
  sushiv_dimension_set_value(s,0,2,2);
  sushiv_dimension_set_value(s,1,0,-2);
  sushiv_dimension_set_value(s,1,2,2);

  sushiv_new_function(s, 0, 2, 1, discrete_objective, 0);

  sushiv_new_objective(s,0,"test pattern",
		       2,(double []){0, 1.0},
		       (int []){0},
		       (int []){0},
		       "Y", 0);

  sushiv_new_panel_2d(s,0,"Discrete data example",
		      (int []){0,-1},
		      (int []){0,1,-1},
		      0);

  sushiv_new_panel_1d_linked(s,1,"X Slice",s->objective_list[0]->scale,
			     (int []){0,-1},
			     0,0);

  sushiv_new_panel_1d_linked(s,2,"Y Slice",s->objective_list[0]->scale,
			     (int []){0,-1},
			     0,SUSHIV_PANEL_LINK_Y | SUSHIV_PANEL_FLIP);


  return 0;
}
