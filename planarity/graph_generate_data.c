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
#include <string.h>

#include "graph.h"
#include "graph_generate.h"

typedef struct d_vertex{
  int x;
  int y;
} d_vertex;

typedef struct d_edge{
  int a;
  int b;
} d_edge;

typedef struct d_level{
  int obj;
  int less;
  d_vertex *v;
  d_edge *e;
} d_level;  

static d_vertex v_level1[] = {
  {400,30},  {143,216},  {241,518},  {559,518},  {657,216},
  {282,138}, {210,362},  {400,500},  {590,362},  {518,138},
  {-1,-1},
};

static d_edge e_level1[] = {
  {0,5},  {5,1},  {0,2},  {0,3},  {0,9},  {9,4},  {1,6},  {6,2},
  {1,3},  {1,4},  {2,7},  {7,3},  {2,4},  {3,8},  {8,4},{-1,-1},
};

static d_level leveldata[] = {
  {1,0,v_level1,e_level1}



};

void generate_data(graph *g, int order){
  int i;
  d_level *l = leveldata+order;
  vertex *vlist;
  vertex **flat;

  // scan for number of verticies
  i=0;
  while(l->v[i].x != -1)i++;
  vlist=new_board(g, i);
  flat = alloca(i*sizeof(*flat));

  // build graph from data 
  // add verticies
  {
    vertex *v=vlist;
    i=0;
    while(v){
      v->x = l->v[i].x;
      v->y = l->v[i].y;
      flat[i++]=v;
      v=v->next;
    }
  }
  
  // add edges
  {
    int i=0;
    while(l->e[i].a != -1){
      add_edge(g,flat[l->e[i].a],flat[l->e[i].b]);
      i++;
    }
  }

  g->objective = l->obj;
  g->objective_lessthan = l->less;

}

