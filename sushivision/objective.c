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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "internal.h"

int obj_y(sushiv_objective_t *o){
  switch(o->type){
  case SUSHIV_OBJ_XY:
  case SUSHIV_OBJ_XYZ:
    return 1;
  default:
    return 0;
  }
}

static int _sushiv_new_objective(sushiv_instance_t *s,
				 int number,
				 const char *name,
				 unsigned scalevals,
				 double *scaleval_list,
				 int type,
				 int outputs,
				 int *function_map,
				 int *output_map,
				 unsigned flags){
  sushiv_objective_t *o;
  int i;

  if(number<0){
    fprintf(stderr,"Objective number must be >= 0\n");
    return -EINVAL;
  }
  
  if(number<s->objectives){
    if(s->objective_list[number]!=NULL){
      fprintf(stderr,"Objective number %d already exists\n",number);
      return -EINVAL;
    }
  }else{
    if(s->objectives == 0){
      s->objective_list = calloc (number+1,sizeof(*s->objective_list));
    }else{
      s->objective_list = realloc (s->objective_list,(number+1) * sizeof(*s->objective_list));
      memset(s->objective_list + s->objectives, 0, sizeof(*s->objective_list)*(number +1 - s->objectives));
    }
    s->objectives=number+1;
  }

  /* sanity check the maps */
  for(i=0;i<outputs;i++){
    if(function_map[i]<0 || 
       function_map[i]>=s->functions ||
       !s->function_list[function_map[i]]){
      fprintf(stderr,"Objectve %d: function %d does not exist.\n",
	      number,function_map[i]);
      return -EINVAL;
    }
    if(output_map[i]<0 ||
       output_map[i]>=s->function_list[function_map[i]]->outputs){
      fprintf(stderr,"Objectve %d: function %d does not have an output %d.\n",
	      number,function_map[i],output_map[i]);
      return -EINVAL;
    }
  }

  o = s->objective_list[number] = calloc(1, sizeof(**s->objective_list));
  o->number = number;
  o->name = strdup(name);
  o->type = type;
  o->outputs = outputs;
  o->flags = flags;
  o->sushi = s;

  if(scalevals>0 && scaleval_list)
    o->scale=scale_new(scalevals, scaleval_list, name);

  /* copy in the maps */
  o->function_map = malloc(outputs * sizeof(*o->function_map));
  o->output_map = malloc(outputs * sizeof(*o->output_map));
  memcpy(o->function_map,function_map,outputs * sizeof(*o->function_map));
  memcpy(o->output_map,output_map,outputs * sizeof(*o->output_map));

  return 0;
}

int sushiv_new_objective_Y(sushiv_instance_t *s,
			   int number,
			   const char *name,
			   unsigned scalevals, 
			   double *scaleval_list,
			   int function_number,
			   int function_output,
			   unsigned flags){
  return _sushiv_new_objective(s,number,name, 
			       scalevals,scaleval_list,
			       SUSHIV_OBJ_Y,
			       1,
			       (int []){function_number},
			       (int []){function_output},
			       flags);
}

int sushiv_new_objective_XY(sushiv_instance_t *s,
			    int number,
			    const char *name,
			    unsigned scalevals, 
			    double *scaleval_list,
			    int function_number[2],
			    int function_output[2],
			    unsigned flags){
  return _sushiv_new_objective(s,number,name, 
			       scalevals,scaleval_list,
			       SUSHIV_OBJ_XY,
			       2,
			       function_number,
			       function_output,
			       flags);
}

int sushiv_new_objective_YZ(sushiv_instance_t *s,
			    int number,
			    const char *name,
			    unsigned scalevals, 
			    double *scaleval_list,
			    int function_number[2],
			    int function_output[2],
			    unsigned flags){
  return _sushiv_new_objective(s,number,name, 
			       scalevals,scaleval_list,
			       SUSHIV_OBJ_YZ,
			       2,
			       function_number,
			       function_output,
			       flags);
}

int sushiv_new_objective_XYZ(sushiv_instance_t *s,
			     int number,
			     const char *name,
			     unsigned scalevals, 
			     double *scaleval_list,
			     int function_number[3],
			     int function_output[3],
			     unsigned flags){
  return _sushiv_new_objective(s,number,name, 
			       scalevals,scaleval_list,
			       SUSHIV_OBJ_XYZ,
			       3,
			       function_number,
			       function_output,
			       flags);
}
