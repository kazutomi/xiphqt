/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
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
#include "sushivision.h"
#include "internal.h"
#include "scale.h"

int sushiv_objective_set_scale(sushiv_objective_t *d, unsigned scalevals, 
			       double *scaleval_list){
  int i;
  
  if(scalevals<2){
    fprintf(stderr,"Scale requires at least two scale values.");
    return -EINVAL;
  }
  
  if(d->scale_val_list)free(d->scale_val_list);
  if(d->scale_label_list){
    for(i=0;i<d->scale_vals;i++)
      free(d->scale_label_list[i]);
    free(d->scale_label_list);
  }

  // copy values
  d->scale_vals = scalevals;
  d->scale_val_list = calloc(scalevals,sizeof(*d->scale_val_list));
  for(i=0;i<(int)scalevals;i++)
    d->scale_val_list[i] = scaleval_list[i];
  
  // generate labels
  d->scale_label_list = scale_generate_labels(scalevals,scaleval_list);
  
  return 0;
}

int sushiv_objective_set_scalelabels(sushiv_objective_t *d, 
				     char **scalelabel_list){
  int i;
  for(i=0;i<d->scale_vals;i++){
    if(d->scale_label_list[i])
      free(d->scale_label_list[i]);
    d->scale_label_list[i] = strdup(scalelabel_list[i]);
  }
  return 0;
}

int sushiv_new_objective(sushiv_instance_t *s,
			 int number,
			 const char *name,
			 unsigned scalevals,
			 double *scaleval_list,
			 double(*callback)(double *),
			 unsigned flags){
  sushiv_objective_t *o;
  
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
  o->number = number;
  o->name = strdup(name);
  o->flags = flags;
  o->sushi = s;
  o->callback = callback;

  sushiv_objective_set_scale(o, scalevals, scaleval_list);

  return 0;
}
