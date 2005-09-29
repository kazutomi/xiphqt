/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <stdlib.h>
#include "graph.h"
#include "gameboard.h"

static fade_list *pool=0;

#define CHUNK 32
static void fade_add_vertex(fade_state *f,vertex *v){
  fade_list *ret;

  if(pool==0){
    int i;
    pool = calloc(CHUNK,sizeof(*pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      pool[i].next=pool+i+1;
  }

  ret=pool;
  pool=ret->next;

  ret->v=v;

  ret->next = f->head;
  f->head = ret;
}

static gint animate_fade(gpointer ptr){
  Gameboard *g = (Gameboard *)ptr;
  fade_state *f = &g->fade;
  
  f->count--;
  if(f->count>0){
    fade_list *l = f->head;
  
    while(l){
      invalidate_vertex(g,l->v);
      l=l->next;
    } 
    
    return 1;
  }
  
  fade_cancel(g);
  return 0;
}

void fade_cancel(Gameboard *g){
  fade_state *f = &g->fade;
  fade_list *l = f->head;
  
  while(l){
    fade_list *n = l->next;

    /* invalidate the vertex */
    invalidate_vertex(g,l->v);

    l->next=pool;
    pool=l;
    l=n;
  }
  f->head = 0;
  f->count = 0;

  if(f->fade_timer)
    g_source_remove(f->fade_timer);
  f->fade_timer=0;
}

void fade_attached(Gameboard *g,vertex *v){
  fade_state *f = &g->fade;
  edge_list *el=v->edges;

  /* If a fade is already in progress, cancel it */
  fade_cancel(g);

  while(el){
    edge *e=el->edge;

    if(v == e->A){
      fade_add_vertex(f,e->B);
    }else{
      fade_add_vertex(f,e->A);
    }
    el=el->next;
  }

  f->count = FADE_FRAMES;

  f->fade_timer = g_timeout_add(FADE_ANIM_INTERVAL, animate_fade, (gpointer)g);
}

