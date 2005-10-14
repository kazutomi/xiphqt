/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "graph.h"
#include "timer.h"

static char objective_string[160];


int graphscore_get_raw_score(graph *g){
  return (int)ceil((g->original_intersections- g->active_intersections)*
		   g->intersection_mult);
}

int graphscore_get_multiplier_percent(graph *g){
  float obj_multiplier = 100;
  
  if(g->objective_lessthan)
    if(g->objective > g->active_intersections)
      obj_multiplier +=  100.f * g->objective_mult / g->objective * (g->objective - g->active_intersections);
  
  return ceil(obj_multiplier);
}

int graphscore_get_score(graph *g){
  return graphscore_get_raw_score(g)*graphscore_get_multiplier_percent(g)/100;
}

int graphscore_get_bonus(graph *g){
  int obj_multiplier = graphscore_get_multiplier_percent(g);
  
  if(get_timer()< g->original_intersections*g->intersection_mult)
    return ceil ((g->original_intersections*g->intersection_mult-get_timer()) * obj_multiplier / 100);
  
  return 0;
}

char *graphscore_objective_string(graph *g){
  if(g->objective == 0)
    return "zero intersections";
  if(g->objective == 1){
    if(g->objective_lessthan){
      return "1 intersection or fewer";
    }else{
      return "1 intersection";
    }
  }else{
    snprintf(objective_string,160,"%d intersections%s",
	     g->objective,(g->objective_lessthan?
			" or fewer":""));
    return objective_string;
  }
}


