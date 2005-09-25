#include <string.h>
#include <stdio.h>
#include <math.h>
#include "graph.h"
#include "timer.h"

static char objective_string[160];

int graphscore_get_score(graph *g){
  int intersection_score = (int)ceil((g->original_intersections- g->active_intersections)*
				     g->intersection_mult);
  float obj_multiplier = 1;
  
  if(g->objective_lessthan)
    if(g->objective > g->active_intersections)
      obj_multiplier += (g->objective-g->active_intersections)*g->objective_mult;

  return ceil( intersection_score * obj_multiplier );
}

int graphscore_get_bonus(graph *g){
  float obj_multiplier = 1;

  if(g->objective_lessthan)
    if(g->objective > g->active_intersections)
      obj_multiplier += (g->objective-g->active_intersections)*g->objective_mult;
  
  if(get_timer()< g->original_intersections*g->intersection_mult)
    return ceil ((g->original_intersections*g->intersection_mult-get_timer()) * obj_multiplier);
  
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


