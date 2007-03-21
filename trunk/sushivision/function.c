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

sv_func_t *sv_func_new(sv_instance_t *in,
		       int number,
		       int out_vals,
		       void(*callback)(double *,double *),
		       unsigned flags){
  sv_instance_t *s = (sv_instance_t *)in; // unwrap
  sv_func_t *f;

  if(number<0){
    fprintf(stderr,"Function number must be >= 0\n");
    errno = -EINVAL;
    return NULL;
  }

  if(number<s->functions){
    if(s->function_list[number]!=NULL){
      fprintf(stderr,"Function number %d already exists\n",number);
      errno = -EINVAL;
      return NULL;
    }
  }else{
    if(s->functions == 0){
      s->function_list = calloc (number+1,sizeof(*s->function_list));
    }else{
      s->function_list = realloc (s->function_list,(number+1) * sizeof(*s->function_list));
      memset(s->function_list + s->functions, 0, sizeof(*s->function_list)*(number +1 - s->functions));
    }
    s->functions=number+1;
  }

  f = s->function_list[number] = calloc(1, sizeof(**s->function_list));
  f->number = number;
  f->flags = flags;
  f->sushi = s;
  f->callback = callback;
  f->outputs = out_vals;
  f->type = SV_FUNC_BASIC;

  return f;
}
