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
	s->panel_list[i]->private->update_menus(s->panel_list[i]);
  }
}

void _sushiv_panel_undo_suspend(sushiv_panel_t *p){
  p->sushi->private->undo_suspend++;
}

void _sushiv_panel_undo_resume(sushiv_panel_t *p){
  p->sushi->private->undo_suspend--;
  if(p->sushi->private->undo_suspend<0){
    fprintf(stderr,"Internal error: undo suspend refcount count < 0\n");
    p->sushi->private->undo_suspend=0;
  }
}

void _sushiv_panel_undo_log(sushiv_panel_t *p){
  sushiv_instance_t *s = p->sushi;
  sushiv_panel_undo_t *u;
  int i,j;

  if(!p->sushi->private->undo_stack)
    s->private->undo_stack = calloc(2,s->panels*sizeof(*s->private->undo_stack));

  // log into a fresh entry; pop this level and all above it 
  if(s->private->undo_stack[s->private->undo_level]){
    i=s->private->undo_level;
    while(s->private->undo_stack[i]){
      for(j=0;j<s->panels;j++){
	u = s->private->undo_stack[i]+j;
	if(u->mappings) free(u->mappings);
	if(u->scale_vals[0]) free(u->scale_vals[0]);
	if(u->scale_vals[1]) free(u->scale_vals[1]);
	if(u->scale_vals[2]) free(u->scale_vals[2]);
	if(u->dim_vals[0]) free(u->dim_vals[0]);
	if(u->dim_vals[1]) free(u->dim_vals[1]);
	if(u->dim_vals[2]) free(u->dim_vals[2]);
      }
      free(s->private->undo_stack[i]);
      s->private->undo_stack[i]= NULL;
      i++;
    }
  }

  // alloc new undos
  u = s->private->undo_stack[s->private->undo_level]= calloc(s->panels,sizeof(*u));
  
  // pass off actual population to panels
  for(j=0;j<s->panels;j++)
    s->panel_list[j]->private->undo_log(u+j,s->panel_list[j]);
}

void _sushiv_panel_undo_restore(sushiv_panel_t *p){
  sushiv_instance_t *s = p->sushi;
  int i;
  
  for(i=0;i<s->panels;i++){
    sushiv_panel_undo_t *u = &s->private->undo_stack[s->private->undo_level][i];
    
    s->panel_list[i]->private->undo_restore(u,s->panel_list[i]);
    plot_expose_request(PLOT(s->panel_list[i]->private->graph));
  }
}

void _sushiv_panel_undo_push(sushiv_panel_t *p){
  sushiv_instance_t *s = p->sushi;
  
  if(s->private->undo_suspend)return;

  _sushiv_panel_undo_log(p);

  // realloc stack 
  s->private->undo_stack = 
    realloc(s->private->undo_stack,
	    (s->private->undo_level+3)*sizeof(*s->private->undo_stack));
  s->private->undo_level++;
  s->private->undo_stack[s->private->undo_level]=NULL;
  s->private->undo_stack[s->private->undo_level+1]=NULL;
  update_all_menus(s);
}

void _sushiv_panel_undo_up(sushiv_panel_t *p){
  sushiv_instance_t *s = p->sushi;
  
  if(!s->private->undo_stack)return;
  if(!s->private->undo_stack[s->private->undo_level])return;
  if(!s->private->undo_stack[s->private->undo_level+1])return;
  
  s->private->undo_level++;
  _sushiv_panel_undo_suspend(p);
  _sushiv_panel_undo_restore(p);
  _sushiv_panel_undo_resume(p);
  update_all_menus(s);
}

void _sushiv_panel_undo_down(sushiv_panel_t *p){
  sushiv_instance_t *s = p->sushi;

  if(!s->private->undo_stack)return;
  if(!s->private->undo_level)return;

  if(!s->private->undo_stack[s->private->undo_level+1])
    _sushiv_panel_undo_log(p);
  s->private->undo_level--;

  _sushiv_panel_undo_suspend(p);
  _sushiv_panel_undo_restore(p);
  _sushiv_panel_undo_resume(p);
  update_all_menus(s);
}
