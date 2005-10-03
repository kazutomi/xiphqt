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
#include <math.h>

#include "graph.h"
#include "random.h"
#include "gameboard.h"
#include "graph_generate.h"
#include "graph_arrange.h"

/* Mesh1 has three primary considerations in mind:
   1) By default, act like the algorithm in the original planarity
   2) Conform to a population contraint that is easy to work with/modify
   3) Playability; short edges result in graphs that are easier to solve.

   Mesh2 is intended to be a freeform populator with two different
   uses; harder levels that disrupt the easy solution algorithms that
   mesh1 allows, as well as being able to densely populate arbitrary
   regions.  */

typedef struct {
  graph *g;
  int width;
  int height;
} mesh;

// check for intersections with other edges
static int check_intersects_edge(mesh *m, edge *e){
  edge *ge = m->g->edges;
  while(ge){
    double xo,yo;
    if(intersects(ge->A,ge->B,e->A,e->B,&xo,&yo))return 1;
    ge=ge->next;
  }
  return 0;
}

static float dot(vertex *A, vertex *B, vertex *C){
  return (float)(B->x-A->x)*(float)(C->x-B->x) + 
    (float)(B->y-A->y)*(float)(C->y-B->y);
}

static float cross(vertex *A, vertex *B, vertex *C){
  return (float)(B->x-A->x)*(float)(C->y-A->y) - 
    (float)(B->y-A->y)*(float)(C->x-A->x);
}

static float sq_point_distance(vertex *A, vertex *B){
  float xd = A->x-B->x;
  float yd = A->y-B->y;
  return xd*xd+yd*yd;
}

static float sq_line_distance(edge *e, vertex *v){

  if(dot(e->A,e->B,v) > 0)
    return sq_point_distance(e->B,v);
  if(dot(e->B,e->A,v) > 0)
    return sq_point_distance(e->A,v);

  {
    float c = cross(e->A,e->B,v);
    return c*c/sq_point_distance(e->A,e->B);
  }
}

// Does this edge pass within ten pixels of another vertex
static int check_intersects_vertex(mesh *m, edge *e){
  vertex *v = m->g->verticies;

  while(v){
    if(v!=e->A && v!=e->B && sq_line_distance(e,v)<100)return 1;
    v=v->next;
  }

  return 0;
}

// Although very inefficient, it is simple and correct.  Even
// impossibly large boards generate in a fraction of a second on old
// boxen.  There's likely no need to bother optimizing this step of
// board creation. */
static void span_depth_first2(mesh *m,vertex *current, float length_limit){

  while(1){
    // Mark/count all possible choices
    int count=0;
    int count2=0;
    int choice;
    vertex *v = m->g->verticies;

    while(v){
      v->selected = 0;
      if(!v->edges && v!=current){
	if(sq_point_distance(v,current)<=length_limit){
	  edge e;
	  e.A = v;
	  e.B = current;
	  if(!check_intersects_edge(m,&e)){
	    if(!check_intersects_vertex(m,&e)){
	      v->selected = 1;
	      count++;
	    }
	  }
	}
      }
      v=v->next;
    }

    if(count == 0) return;
    
    choice = random_number()%count;
    count2 = 0;
    v = m->g->verticies;
    while(v){
      if(v->selected){
	if(count2++ == choice){
	  add_edge(m->g,v,current);
	  span_depth_first2(m,v, length_limit);
	  break;
	}
      }
      v=v->next;
    }
    
    if(count == 1) return; // because we just took care of it
  }
}

static void random_populate(mesh *m,vertex *current,int dense_128, float length_limit){
  int num_edges=0,count=0;
  edge_list *el=current->edges;
  vertex *v = m->g->verticies;
  while(el){
    num_edges++;
    el=el->next;
  }

  // mark all possible choices
  while(v){
    v->selected = 0;
    if(v!=current){
      if(sq_point_distance(v,current)<=length_limit){
	if(!exists_edge(v,current)){
	  edge e;
	  e.A = v;
	  e.B = current;
	  if(!check_intersects_edge(m,&e)){
	    if(!check_intersects_vertex(m,&e)){
	      v->selected=1;
	      count++;
	    }
	  }
	}
      }
    }
    v=v->next;
  }

  // make sure no vertex is a leaf
  if(num_edges<2){
    int choice = random_number() % count;
    count = 0;
    
    v = m->g->verticies;
    while(v){
      if(v->selected){
	if(count++ == choice){
	  add_edge(m->g,v,current);
	  v->selected=0;
	  break;
	}
      }
      v=v->next;
    }
  }

  // now, random populate
  v = m->g->verticies;
  while(v){
    if(v->selected && random_yes(dense_128)){
      add_edge(m->g,v,current);
      v->selected=0;
    }
    v=v->next;
  }
}

/* Initial generation setup */

static void mesh_setup(graph *g, mesh *m, int order, int divis){
  int flag=0;
  int wiggle=0;
  int n;
  m->g = g;
  m->width=3;
  m->height=2;

  {
    while(--order){
      if(flag){
	flag=0;
	m->height+=1;
      }else{
	flag=1;
	m->width+=2;
      }
    }
  }
  n=m->width*m->height;

  // is this divisible by our requested divisor if any?
  if(divis>0 && n%divis){
    while(1){
      wiggle++;

      if(!((n+wiggle)%divis)) break;

      if(n-wiggle>6 && !((n-wiggle)%divis)){
	wiggle = -wiggle;
	break;
      }
    }

    // refactor the rectangular mesh's dimensions.
    {
      int h = (int)sqrt(n+wiggle),w;

      while( (n+wiggle)%h )h--;

      if(h==1){
	// double it and be content with a working result
	h=2;
	w=(n+wiggle);
      }else{
	// good factoring
	w = (n+wiggle)/h;
      }

      m->width=w;
      m->height=h;
    }
  }

  new_board(g, m->width * m->height);

  // used for intersection calcs
  {
    int x,y;
    vertex *v = g->verticies;
    for(y=0;y<m->height;y++)
      for(x=0;x<m->width;x++){
	v->x=x*50; // not a random number
	v->y=y*50; // not a random number
	v=v->next;
      }
  }

  g->objective = 0;
  g->objective_lessthan = 0;

}

static void generate_mesh2(mesh *m, int density_128, float length_limit){ 
  vertex *v;

  length_limit*=50;
  length_limit*=length_limit;
  
  /* first walk a random spanning tree */
  span_depth_first2(m, m->g->verticies, length_limit);
  
  /* now iterate the whole mesh adding random edges */
  v=m->g->verticies;
  while(v){
    random_populate(m, v, density_128, length_limit);
    v=v->next;
  }
  deselect_verticies(m->g);
}

void generate_freeform(graph *g, int order){
  mesh m;
  random_seed(order+1);
  mesh_setup(g, &m, order, 0);

  generate_mesh2(&m,48,5);
  //arrange_verticies_mesh(g,m.width,m.height);

  randomize_verticies(g);
  if(order*.03<.3)
    arrange_verticies_polycircle(g,4,0,order*.03,0,0,0);
  else
    arrange_verticies_polycircle(g,4,0,.3,0,0,0);
}
