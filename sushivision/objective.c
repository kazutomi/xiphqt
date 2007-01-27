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

int sushiv_new_objective(sushiv_instance_t *s,
			 int number,
			 const char *name,
			 unsigned scalevals,
			 double *scaleval_list,
			 int *function_map,
			 int *output_map,
			 char *output_types,
			 unsigned flags){
  sushiv_objective_t *o;
  sushiv_objective_internal_t *p;
  int i;
  int outputs = strlen(output_types);

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
  
  o = s->objective_list[number] = calloc(1, sizeof(**s->objective_list));
  p = o->private = calloc(1,sizeof(*o->private));

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
    switch(output_types[i]){
    case 'X':
      if(p->x_func){
	fprintf(stderr,"Objective %d: More than one X dimension specified.\n",
		number);
	return -EINVAL;
      }
      p->x_fout = output_map[i];
      p->x_func = s->function_list[function_map[i]];
      break;

    case 'Y':
      if(p->y_func){
	fprintf(stderr,"Objective %d: More than one Y dimension specified.\n",
		number);
	return -EINVAL;
      }
      p->y_fout = output_map[i];
      p->y_func = s->function_list[function_map[i]];
      break;

    case 'Z':
      if(p->z_func){
	fprintf(stderr,"Objective %d: More than one Z dimension specified.\n",
		number);
	return -EINVAL;
      }
      p->z_fout = output_map[i];
      p->z_func = s->function_list[function_map[i]];
      break;

    case 'M':
      if(p->m_func){
	fprintf(stderr,"Objective %d: More than one magnitude [M] dimension specified.\n",
		number);
	return -EINVAL;
      }
      p->m_fout = output_map[i];
      p->m_func = s->function_list[function_map[i]];
      break;

    case 'E':
      if(p->e2_func){
	fprintf(stderr,"Objective %d: More than two error [E] dimensions specified.\n",
		number);
	return -EINVAL;
      }
      if(p->e1_func){
	p->e2_fout = output_map[i];
	p->e2_func = s->function_list[function_map[i]];
      }else{
	p->e1_fout = output_map[i];
	p->e1_func = s->function_list[function_map[i]];
      }
      break;

    case 'P':
      if(p->p2_func){
	fprintf(stderr,"Objective %d: More than two phase [P] dimensions specified.\n",
		number);
	return -EINVAL;
      }
      if(p->p1_func){
	p->p2_fout = output_map[i];
	p->p2_func = s->function_list[function_map[i]];
      }else{
	p->p1_fout = output_map[i];
	p->p1_func = s->function_list[function_map[i]];
      }
      break;

    default:
      fprintf(stderr,"Objective %d: '%c' is an usupported output type.\n",
	      number,output_types[i]);
      return -EINVAL;
    }
  }

  o->number = number;
  o->name = strdup(name);
  o->output_types = strdup(output_types);
  o->type = SUSHIV_OBJ_BASIC;
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
