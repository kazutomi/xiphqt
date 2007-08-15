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

sv_obj_t *sv_obj_new(int number,
		     char *name,
		     sv_func_t **function_map,
		     int *function_output_map,
		     char *output_type_map,
		     unsigned flags){
  
  sv_obj_t *o;
  sv_obj_internal_t *p;
  int i;
  int outputs = strlen(output_type_map);

  if(number<0){
    fprintf(stderr,"Objective number must be >= 0\n");
    errno = -EINVAL;
    return NULL;
  }
  
  if(number<_sv_objectives){
    if(_sv_objective_list[number]!=NULL){
      fprintf(stderr,"Objective number %d already exists\n",number);
    errno = -EINVAL;
    return NULL;
    }
  }else{
    if(_sv_objectives == 0){
      _sv_objective_list = calloc (number+1,sizeof(*_sv_objective_list));
    }else{
      _sv_objective_list = realloc (_sv_objective_list,(number+1) * sizeof(*_sv_objective_list));
      memset(_sv_objective_list + _sv_objectives, 0, sizeof(*_sv_objective_list)*(number +1 - _sv_objectives));
    }
    _sv_objectives=number+1;
  }
  
  o = _sv_objective_list[number] = calloc(1, sizeof(**_sv_objective_list));
  p = o->private = calloc(1,sizeof(*o->private));

  /* sanity check the maps */
  for(i=0;i<outputs;i++){
    if(!function_map[i]){
      fprintf(stderr,"Objectve %d (\"%s\"): function %d missing.\n",
	      number,name,i);
      errno = -EINVAL;
      return NULL;
    }
    if(function_output_map[i]<0 ||
       function_output_map[i]>=function_map[i]->outputs){
      fprintf(stderr,"Objectve %d (\"%s\"): function %d does not have an output %d.\n",
	      number,name,function_map[i]->number,function_output_map[i]);
      errno = -EINVAL;
      return NULL;
    }
    switch(output_type_map[i]){
    case 'X':
      if(p->x_func){
	fprintf(stderr,"Objective %d: More than one X dimension specified.\n",
		number);
	errno = -EINVAL;
	return NULL;
      }
      p->x_fout = function_output_map[i];
      p->x_func = (sv_func_t *)function_map[i];
      break;

    case 'Y':
      if(p->y_func){
	fprintf(stderr,"Objective %d: More than one Y dimension specified.\n",
		number);
	errno = -EINVAL;
	return NULL;
      }
      p->y_fout = function_output_map[i];
      p->y_func = (sv_func_t *)function_map[i];
      break;

    case 'Z':
      if(p->z_func){
	fprintf(stderr,"Objective %d: More than one Z dimension specified.\n",
		number);
	errno = -EINVAL;
	return NULL;
      }
      p->z_fout = function_output_map[i];
      p->z_func = (sv_func_t *)function_map[i];
      break;

    case 'M':
      if(p->m_func){
	fprintf(stderr,"Objective %d: More than one magnitude [M] dimension specified.\n",
		number);
	errno = -EINVAL;
	return NULL;
      }
      p->m_fout = function_output_map[i];
      p->m_func = (sv_func_t *)function_map[i];
      break;

    case 'E':
      if(p->e2_func){
	fprintf(stderr,"Objective %d: More than two error [E] dimensions specified.\n",
		number);
	errno = -EINVAL;
	return NULL;
      }
      if(p->e1_func){
	p->e2_fout = function_output_map[i];
	p->e2_func = (sv_func_t *)function_map[i];
      }else{
	p->e1_fout = function_output_map[i];
	p->e1_func = (sv_func_t *)function_map[i];
      }
      break;

    case 'P':
      if(p->p2_func){
	fprintf(stderr,"Objective %d: More than two phase [P] dimensions specified.\n",
		number);
	errno = -EINVAL;
	return NULL;
      }
      if(p->p1_func){
	p->p2_fout = function_output_map[i];
	p->p2_func = (sv_func_t *)function_map[i];
      }else{
	p->p1_fout = function_output_map[i];
	p->p1_func = (sv_func_t *)function_map[i];
      }
      break;

    default:
      fprintf(stderr,"Objective %d: '%c' is an usupported output type.\n",
	      number,output_type_map[i]);
      errno = -EINVAL;
      return NULL;
    }
  }

  o->number = number;
  o->name = strdup(name);
  o->output_types = strdup(output_type_map);
  o->type = SV_OBJ_BASIC;
  o->outputs = outputs;
  o->flags = flags;

  /* copy in the maps */
  o->function_map = malloc(outputs * sizeof(*o->function_map));
  o->output_map = malloc(outputs * sizeof(*o->output_map));
  memcpy(o->output_map,function_output_map,outputs * sizeof(*o->output_map));
  
  for(i=0;i<outputs;i++)
    o->function_map[i] = function_map[i]->number;
  
  return o;
}

// XXXX need to recompute after
// XXXX need to add scale cloning to compute to make this safe in callbacks
int sv_obj_set_scale(sv_obj_t *in,
		     sv_scale_t *scale){
  sv_obj_t *o = (sv_obj_t *)in; // unwrap

  if(o->scale)
    sv_scale_free(o->scale); // always a deep copy we own
  
  o->scale = (sv_scale_t *)sv_scale_copy(scale);

  // redraw the slider

  return 0;
}

// XXXX need to recompute after
// XXXX need to add scale cloning to compute to make this safe in callbacks
int sv_obj_make_scale(sv_obj_t *in,
		      unsigned scalevals, 
		      double *scaleval_list,
		      char **scalelabel_list,
		      unsigned flags){
  sv_obj_t *o = (sv_obj_t *)in; //unwrap

  sv_scale_t *scale = sv_scale_new(o->name,scalevals,scaleval_list,scalelabel_list,0);
  if(!scale)return errno;

  int ret = sv_obj_set_scale(o,scale);
  sv_scale_free(scale);
  return ret;
}
