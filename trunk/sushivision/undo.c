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

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "internal.h"

/* encapsulates some amount of common undo/redo infrastructure */

void _sushiv_panel_undo_suspend(sushiv_panel_t *p){
  p->private->undo_suspend++;
}

void _sushiv_panel_undo_resume(sushiv_panel_t *p){
  p->private->undo_suspend--;
  if(p->private->undo_suspend<0){
    fprintf(stderr,"Internal error: undo suspend refcount count < 0\n");
    p->private->undo_suspend=0;
  }

  if(p->private->undo_suspend==0)
    _sushiv_panel_undo_log(p);
}

void _sushiv_panel_undo_log(sushiv_panel_t *p){
  sushiv_panel_undo_t *u;

  if(!p->private->undo_stack)
    p->private->undo_stack = calloc(2,sizeof(*p->private->undo_stack));

  // alloc new undo
  u = p->private->undo_stack[p->private->undo_level];
  if(!u)
    u = p->private->undo_stack[p->private->undo_level]= calloc(1,sizeof(*u));

  // pass off actual populaiton to panel subtype
  u->p = p;
  p->private->undo_log(u);
}

void _sushiv_panel_undo_restore(sushiv_panel_t *p){
  sushiv_panel_undo_t *u = p->private->undo_stack[p->private->undo_level];
  int remap_flag=0;
  int recomp_flag=0;

  p->private->undo_restore(u, &remap_flag, &recomp_flag);

  if(recomp_flag)
    p->private->request_compute(p);
  else if(remap_flag){
    p->private->map_redraw(p);
    p->private->legend_redraw(p);
  }else
    plot_expose_request(PLOT(p->private->graph));
}

void _sushiv_panel_undo_push(sushiv_panel_t *p){
  sushiv_panel_undo_t *u;
  int i;
  
  if(p->private->undo_suspend)return;

  _sushiv_panel_undo_log(p);

  if(p->private->undo_stack[p->private->undo_level+1]){
    /* pop levels above this one */
    i=p->private->undo_level+1;
    while(p->private->undo_stack[i]){
      u = p->private->undo_stack[i];
      if(u->mappings) free(u->mappings);
      if(u->scale_vals[0]) free(u->scale_vals[0]);
      if(u->scale_vals[1]) free(u->scale_vals[1]);
      if(u->scale_vals[2]) free(u->scale_vals[2]);
      if(u->obj_vals[0]) free(u->obj_vals[0]);
      if(u->obj_vals[1]) free(u->obj_vals[1]);
      if(u->obj_vals[2]) free(u->obj_vals[2]);
      if(u->dim_vals[0]) free(u->dim_vals[0]);
      if(u->dim_vals[1]) free(u->dim_vals[1]);
      if(u->dim_vals[2]) free(u->dim_vals[2]);
      free(u);
      p->private->undo_stack[i]= NULL;
      i++;
    }
  }

  // realloc stack 
  p->private->undo_stack = realloc(p->private->undo_stack,(p->private->undo_level+3)*sizeof(*p->private->undo_stack));
  p->private->undo_level++;
  p->private->undo_stack[p->private->undo_level]=0;
  p->private->undo_stack[p->private->undo_level+1]=0;
  p->private->update_menus(p);

}

void _sushiv_panel_undo_up(sushiv_panel_t *p){
  
  if(!p->private->undo_stack)return;
  if(!p->private->undo_stack[p->private->undo_level])return;
  if(!p->private->undo_stack[p->private->undo_level+1])return;
  
  p->private->undo_level++;
  _sushiv_panel_undo_suspend(p);
  _sushiv_panel_undo_restore(p);
  _sushiv_panel_undo_resume(p);
  p->private->update_menus(p);
}

void _sushiv_panel_undo_down(sushiv_panel_t *p){

  if(!p->private->undo_stack)return;
  if(!p->private->undo_level)return;
 
  _sushiv_panel_undo_log(p);
  p->private->undo_level--;

  _sushiv_panel_undo_suspend(p);
  _sushiv_panel_undo_restore(p);
  _sushiv_panel_undo_resume(p);
  p->private->update_menus(p);
}
