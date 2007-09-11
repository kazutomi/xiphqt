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

int _sv_objectives=0;
sv_obj_t **_sv_objective_list=NULL;

static char *objective_axismap[]={
  "X","Y","Z"
};

#define map_axes ((int)sizeof(objective_axismap)/(int)sizeof(*objective_axismap))

int sv_obj_new(char *name,
	       void(*function)(double *,double *),
	       char *input_map,
	       char *output_map){

  sv_obj_t *o = NULL;
  int i,j;
  int number;
  _sv_token *decl = _sv_tokenize_declparam(name);
  _sv_tokenlist *in = NULL;
  _sv_tokenlist *out = NULL;
  
  if(!decl){
    fprintf(stderr,"sushivision: Unable to parse objective declaration \"%s\".\n",name);
    goto err;
  }

  if(_sv_objectives == 0){
    number=0;
    _sv_objective_list = calloc (number+1,sizeof(*_sv_objective_list));
    _sv_objectives=1;
  }else{
    for(number=0;number<_sv_objectives;number++)
      if(!_sv_objective_list[number])break;
    if(number==_sv_objectives){
      _sv_objectives=number+1;
      _sv_objective_list = realloc (_sv_objective_list,_sv_objectives * sizeof(*_sv_objective_list));
    }
  }
  
  o = _sv_objective_list[number] = calloc(1, sizeof(**_sv_objective_list));
  o->name = strdup(decl->name);
  o->legend = strdup(decl->label);
  o->number = number;
  o->function = function;

  /* parse and sanity check the maps */
  in = _sv_tokenize_noparamlist(input_map);
  out = _sv_tokenize_noparamlist(output_map);
  
  /* input dimensions */
  if(!in){
    fprintf(stderr,"sushivision: Unable to parse objective \"%s\" dimenension list \"%s\".\n",
	    o->name,input_map);
    goto err;
  }
      
  o->inputs = in->n;
  o->input_dims = calloc(in->n,sizeof(*o->input_dims));
  for(i=0;i<in->n;i++){
    sv_dim_t *id = sv_dim(in->list[i]->name);
    if(!id){
      fprintf(stderr,"sushivision: Dimension \"%s\" does not exist in declaration of objective \"%s\".\n",
	      in->list[i]->name,o->name);
      goto err;
    }
    o->input_dims[i] = id;
  }

  /* output axes */
  o->outputs = out->n;
  o->output_axes = malloc(map_axes * sizeof(*o->output_axes));
  for(i=0;i<map_axes;i++)
    o->output_axes[i]=-1;

  for(i=0;i<out->n;i++){
    char *s = out->list[i]->name;
    if(!s || !strcasecmp(s,"*")){
      // output unused by objective
      // do nothing
    }else{
      for(j=0;j<map_axes;j++){
	if(!strcasecmp(s,objective_axismap[j])){

	  if(o->output_axes[j] != -1){
	    fprintf(stderr,"sushivision: Objective \"%s\" declares multiple %s axis outputs.\n",
		    o->name,objective_axismap[j]);
	    goto err;
	  }
	  o->output_axes[j] = i;
	}
      }
      if(j==map_axes){
	fprintf(stderr,"sushivision: No such output axis \"%s\" in declaration of objective \"%s\"\n",
		s,o->name);
      }
    }
  }

  pthread_setspecific(_sv_obj_key, (void *)o);

  _sv_token_free(decl);
  _sv_tokenlist_free(in);
  _sv_tokenlist_free(out);

  return 0;

 err:
  // XXXXX

  return -EINVAL;
}

// XXXX need to recompute after
// XXXX need to add scale cloning to compute to make this safe in callbacks
int sv_obj_set_scale(sv_scale_t *scale){
  sv_obj_t *o = _sv_obj(0);

  if(o->scale)
    sv_scale_free(o->scale); // always a deep copy we own
  
  o->scale = (sv_scale_t *)sv_scale_copy(scale);

  // redraw the slider

  return 0;
}

// XXXX need to recompute after
// XXXX need to add scale cloning to compute to make this safe in callbacks
int sv_obj_make_scale(char *format){
  sv_obj_t *o = _sv_obj(0);
  sv_scale_t *scale;
  int ret;

  char *name=_sv_tokenize_escape(o->name);
  char *label=_sv_tokenize_escape(o->legend);
  char *arg=calloc(strlen(name)+strlen(label)+2,sizeof(*arg));

  strcat(arg,name);
  strcat(arg,":");
  strcat(arg,label);
  free(name);
  free(label);

  if(!o){
    free(arg);
    return -EINVAL;
  }
  scale = sv_scale_new(arg,format);
  free(arg);
  if(!scale)return errno;

  o->scale = scale;
  return ret;
}

sv_obj_t *_sv_obj(char *name){
  int i;
  
  if(name == NULL || name == 0 || !strcmp(name,"")){
    return (sv_obj_t *)pthread_getspecific(_sv_obj_key);
    
  }
  for(i=0;i<_sv_objectives;i++){
    sv_obj_t *o=_sv_objective_list[i];
    if(o && o->name && !strcmp(name,o->name)){
      pthread_setspecific(_sv_obj_key, (void *)o);
      return o;
    }
  }
  return NULL;
}

int sv_obj(char *name){
  sv_obj_t *o = _sv_obj(name);
  if(o)return 0;
  return -EINVAL;
}
