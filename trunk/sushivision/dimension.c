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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "sushivision.h"
#include "internal.h"
#include "scale.h"

int sushiv_dim_set_scale(sushiv_dimension_t *d, unsigned scalevals, double *scaleval_list){
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

int sushiv_dim_set_scalelabels(sushiv_dimension_t *d, char **scalelabel_list){
  int i;
  for(i=0;i<d->scale_vals;i++){
    if(d->scale_label_list[i])
      free(d->scale_label_list[i]);
    d->scale_label_list[i] = strdup(scalelabel_list[i]);
  }
  return 0;
}

int sushiv_new_dimension(sushiv_instance_t *s,
			 int number,
			 const char *name,
			 unsigned scalevals, double *scaleval_list,
			 int (*callback)(sushiv_dimension_t *),
			 unsigned flags){
  sushiv_dimension_t *d;
  
  if(number<0){
    fprintf(stderr,"Dimension number must be >= 0\n");
    return -EINVAL;
  }

  if(number<s->dimensions){
    if(s->dimension_list[number]!=NULL){
      fprintf(stderr,"Dimension number %d already exists\n",number);
      return -EINVAL;
    }
  }else{
    if(s->dimensions == 0){
      s->dimension_list = calloc (number+1,sizeof(*s->dimension_list));
    }else{
      s->dimension_list = realloc (s->dimension_list,(number+1) * sizeof(*s->dimension_list));
      memset(s->dimension_list + s->dimensions, 0, sizeof(*s->dimension_list)*(number + 1 - s->dimensions));
    }
    s->dimensions=number+1;
  }

  d = s->dimension_list[number] = calloc(1, sizeof(**s->dimension_list));
  d->number = number;
  d->name = strdup(name);
  d->flags = flags;
  d->sushi = s;
  d->callback = callback;
  d->panel = NULL;
  return sushiv_dim_set_scale(d, scalevals, scaleval_list);
}
