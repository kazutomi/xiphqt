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

static void update_all_menus(){
  int i;
  if(_sv_panel_list){
    for(i=0;i<_sv_panels;i++)
      if(_sv_panel_list[i])
	_sv_panel_update_menus(_sv_panel_list[i]);
  }
}

void _sv_undo_suspend(){
  _sv_undo_suspended++;
}

void _sv_undo_resume(){
  _sv_undo_suspended--;
  if(_sv_undo_suspended<0){
    fprintf(stderr,"Internal error: undo suspend refcount count < 0\n");
    _sv_undo_suspended=0;
  }
}

void _sv_undo_pop(){
  _sv_undo_t *u;
  int i,j;
  if(!_sv_undo_stack)
    _sv_undo_stack = calloc(2,sizeof(*_sv_undo_stack));
  
  if(_sv_undo_stack[_sv_undo_level]){
    i=_sv_undo_level;
    while(_sv_undo_stack[i]){
      u = _sv_undo_stack[i];
      
      if(u->dim_vals[0]) free(u->dim_vals[0]);
      if(u->dim_vals[1]) free(u->dim_vals[1]);
      if(u->dim_vals[2]) free(u->dim_vals[2]);
      
      if(u->panels){
	for(j=0;j<_sv_panels;j++){
	  _sv_panel_undo_t *pu = u->panels+j;
	  if(pu->mappings) free(pu->mappings);
	  if(pu->scale_vals[0]) free(pu->scale_vals[0]);
	  if(pu->scale_vals[1]) free(pu->scale_vals[1]);
	  if(pu->scale_vals[2]) free(pu->scale_vals[2]);
	}
	free(u->panels);
      }
      free(_sv_undo_stack[i]);
      _sv_undo_stack[i]= NULL;
      i++;
    }
  }
  
  // alloc new undos
  u = _sv_undo_stack[_sv_undo_level] = calloc(1,sizeof(*u));
  u->panels = calloc(_sv_panels,sizeof(*u->panels));
  u->dim_vals[0] = calloc(_sv_dimensions,sizeof(**u->dim_vals));
  u->dim_vals[1] = calloc(_sv_dimensions,sizeof(**u->dim_vals));
  u->dim_vals[2] = calloc(_sv_dimensions,sizeof(**u->dim_vals));
}

void _sv_undo_log(){
  _sv_undo_t *u;
  int i,j;

  // log into a fresh entry; pop this level and all above it 
  _sv_undo_pop();
  u = _sv_undo_stack[_sv_undo_level];
  
  // save dim values
  for(i=0;i<_sv_dimensions;i++){
    sv_dim_t *d = _sv_dimension_list[i];
    if(d){
      u->dim_vals[0][i] = d->bracket[0];
      u->dim_vals[1][i] = d->val;
      u->dim_vals[2][i] = d->bracket[1];
    }
  }

  // pass off panel population to panels
  for(j=0;j<_sv_panels;j++)
    if(_sv_panel_list[j])
      _sv_panel_undo_log(_sv_panel_list[j], u->panels+j);
}

void _sv_undo_restore(){
  int i;
  _sv_undo_t *u = _sv_undo_stack[_sv_undo_level];

  // dims 
  // need to happen first as setting dims can have side effect (like activating crosshairs)
  for(i=0;i<_sv_dimensions;i++){
    sv_dim_t *d = _sv_dimension_list[i];
    if(d){
      _sv_dim_set_thumb(d, 0, u->dim_vals[0][i]);
      _sv_dim_set_thumb(d, 1, u->dim_vals[1][i]);
      _sv_dim_set_thumb(d, 2, u->dim_vals[2][i]);
    }
  }

  // panels
  for(i=0;i<_sv_panels;i++){
    sv_panel_t *p = _sv_panel_list[i];
    if(p)
      _sv_panel_undo_restore(_sv_panel_list[i],u->panels+i);
  }

}

void _sv_undo_push(){
  if(_sv_undo_suspended)return;

  _sv_undo_log();

  // realloc stack 
  _sv_undo_stack = 
    realloc(_sv_undo_stack,
	    (_sv_undo_level+3)*sizeof(*_sv_undo_stack));
  _sv_undo_level++;
  _sv_undo_stack[_sv_undo_level]=NULL;
  _sv_undo_stack[_sv_undo_level+1]=NULL;
  update_all_menus();
}

void _sv_undo_up(){
  if(!_sv_undo_stack)return;
  if(!_sv_undo_stack[_sv_undo_level])return;
  if(!_sv_undo_stack[_sv_undo_level+1])return;
  
  _sv_undo_level++;
  _sv_undo_suspend();
  _sv_undo_restore();
  _sv_undo_resume();
  update_all_menus();
}

void _sv_undo_down(){
  if(!_sv_undo_stack)return;
  if(!_sv_undo_level)return;

  if(!_sv_undo_stack[_sv_undo_level+1])
    _sv_undo_log();
  _sv_undo_level--;

  _sv_undo_suspend();
  _sv_undo_restore();
  _sv_undo_resume();
  update_all_menus();
}


// load piggybacks off the undo infrastructure
