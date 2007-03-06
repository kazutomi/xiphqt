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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "internal.h"

/* encapsulates some amount of common undo/redo infrastructure */

static void update_all_menus(sushiv_instance_t *s){
  int i;
  if(s->panel_list){
    for(i=0;i<s->panels;i++)
      if(s->panel_list[i])
	_sushiv_panel_update_menus(s->panel_list[i]);
  }
}

void _sushiv_undo_suspend(sushiv_instance_t *s){
  s->private->undo_suspend++;
}

void _sushiv_undo_resume(sushiv_instance_t *s){
  s->private->undo_suspend--;
  if(s->private->undo_suspend<0){
    fprintf(stderr,"Internal error: undo suspend refcount count < 0\n");
    s->private->undo_suspend=0;
  }
}

void _sushiv_undo_log(sushiv_instance_t *s){
  sushiv_instance_undo_t *u;
  int i,j;

  if(!s->private->undo_stack)
    s->private->undo_stack = calloc(2,sizeof(*s->private->undo_stack));

  // log into a fresh entry; pop this level and all above it 
  if(s->private->undo_stack[s->private->undo_level]){
    i=s->private->undo_level;
    while(s->private->undo_stack[i]){
      u = s->private->undo_stack[i];

      if(u->dim_vals[0]) free(u->dim_vals[0]);
      if(u->dim_vals[1]) free(u->dim_vals[1]);
      if(u->dim_vals[2]) free(u->dim_vals[2]);
      
      if(u->panels){
	for(j=0;j<s->panels;j++){
	  sushiv_panel_undo_t *pu = u->panels+j;
	  if(pu->mappings) free(pu->mappings);
	  if(pu->scale_vals[0]) free(pu->scale_vals[0]);
	  if(pu->scale_vals[1]) free(pu->scale_vals[1]);
	  if(pu->scale_vals[2]) free(pu->scale_vals[2]);
	}
	free(u->panels);
      }
      free(s->private->undo_stack[i]);
      s->private->undo_stack[i]= NULL;
      i++;
    }
  }
  
  // alloc new undos
  u = s->private->undo_stack[s->private->undo_level] = calloc(1,sizeof(*u));
  u->panels = calloc(s->panels,sizeof(*u->panels));
  u->dim_vals[0] = calloc(s->dimensions,sizeof(**u->dim_vals));
  u->dim_vals[1] = calloc(s->dimensions,sizeof(**u->dim_vals));
  u->dim_vals[2] = calloc(s->dimensions,sizeof(**u->dim_vals));

  // save dim values
  for(i=0;i<s->dimensions;i++){
    sushiv_dimension_t *d = s->dimension_list[i];
    if(d){
      u->dim_vals[0][i] = d->bracket[0];
      u->dim_vals[1][i] = d->val;
      u->dim_vals[2][i] = d->bracket[1];
    }
  }

  // pass off panel population to panels
  for(j=0;j<s->panels;j++)
    if(s->panel_list[j])
      _sushiv_panel_undo_log(s->panel_list[j], u->panels+j);
}

void _sushiv_undo_restore(sushiv_instance_t *s){
  int i;
  sushiv_instance_undo_t *u = s->private->undo_stack[s->private->undo_level];

  // panels
  for(i=0;i<s->panels;i++)
    _sushiv_panel_undo_restore(s->panel_list[i],u->panels+i);

  // dims 
  for(i=0;i<s->dimensions;i++){
    sushiv_dimension_t *d = s->dimension_list[i];
    if(d){
      sushiv_dimension_set_value(s, d->number, 0, u->dim_vals[0][i]);
      sushiv_dimension_set_value(s, d->number, 1, u->dim_vals[1][i]);
      sushiv_dimension_set_value(s, d->number, 2, u->dim_vals[2][i]);
    }
  }
}

void _sushiv_undo_push(sushiv_instance_t *s){
  if(s->private->undo_suspend)return;

  _sushiv_undo_log(s);

  // realloc stack 
  s->private->undo_stack = 
    realloc(s->private->undo_stack,
	    (s->private->undo_level+3)*sizeof(*s->private->undo_stack));
  s->private->undo_level++;
  s->private->undo_stack[s->private->undo_level]=NULL;
  s->private->undo_stack[s->private->undo_level+1]=NULL;
  update_all_menus(s);
}

void _sushiv_undo_up(sushiv_instance_t *s){
  if(!s->private->undo_stack)return;
  if(!s->private->undo_stack[s->private->undo_level])return;
  if(!s->private->undo_stack[s->private->undo_level+1])return;
  
  s->private->undo_level++;
  _sushiv_undo_suspend(s);
  _sushiv_undo_restore(s);
  _sushiv_undo_resume(s);
  update_all_menus(s);
}

void _sushiv_undo_down(sushiv_instance_t *s){
  if(!s->private->undo_stack)return;
  if(!s->private->undo_level)return;

  if(!s->private->undo_stack[s->private->undo_level+1])
    _sushiv_undo_log(s);
  s->private->undo_level--;

  _sushiv_undo_suspend(s);
  _sushiv_undo_restore(s);
  _sushiv_undo_resume(s);
  update_all_menus(s);
}
