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
#include "graph_region.h"

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
  int active_current;
  int active_max;
} mesh;

// check for intersections with other edges
static int check_intersects_edge(mesh *m, edge *e, int intersections){
  edge *ge = m->g->edges;
  int count=0;

  while(ge){
    double xo,yo;

    // edges that aren't in this region don't exist (for
    // now) by definition
    if(ge->A->active == m->active_current || ge->B->active == m->active_current){
      // edges that share a vertex don't intersect by definition
      if(ge->A!=e->A && ge->A!=e->B && ge->B!=e->A && ge->B!=e->B)
	if(intersects(ge->A->orig_x,ge->A->orig_y,
		      ge->B->orig_x,ge->B->orig_y,
		      e->A->orig_x,e->A->orig_y,
		      e->B->orig_x,e->B->orig_y,
		      &xo,&yo)){
	  count++;
	  if(count>intersections)return 1;
	}
    }
    ge=ge->next;
  }
  return 0;
}

static float dot(vertex *A, vertex *B, vertex *C){
  return (float)(B->orig_x-A->orig_x)*(float)(C->orig_x-B->orig_x) + 
    (float)(B->orig_y-A->orig_y)*(float)(C->orig_y-B->orig_y);
}

static float cross(vertex *A, vertex *B, vertex *C){
  return (float)(B->orig_x-A->orig_x)*(float)(C->orig_y-A->orig_y) - 
    (float)(B->orig_y-A->orig_y)*(float)(C->orig_x-A->orig_x);
}

static float sq_point_distance(vertex *A, vertex *B){
  float xd = A->orig_x-B->orig_x;
  float yd = A->orig_y-B->orig_y;
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
    if(v->active == m->active_current)
      if(v!=e->A && v!=e->B && sq_line_distance(e,v)<16)return 1;
    v=v->next;
  }

  return 0;
}

static int select_available(mesh *m,vertex *current,float length_limit,int intersections){
  int count=0;
  vertex *v = m->g->verticies;

  // mark all possible choices
  while(v){
    v->selected = 0;
    if(v!=current){
      if(length_limit==0 || sq_point_distance(v,current)<=length_limit){
	if(!exists_edge(v,current)){
	  edge e;
	  e.A = v;
	  e.B = current;
	  if(!region_intersects(&e)){
	    if(!check_intersects_edge(m,&e,intersections)){
	      if(!check_intersects_vertex(m,&e)){
		v->selected=1;
		count++;
	      }
	    }
	  }
	}
      }
    }
    v=v->next;
  }

  return count;
}

// Although very inefficient, it is simple and correct.  Even
// impossibly large boards generate in a fraction of a second on old
// boxen.  There's likely no need to bother optimizing this step of
// board creation. */

typedef struct insort{
  int metric;
  vertex *v;
} insort;

static int insort_c(const void *a, const void *b){
  insort *A=(insort *)a;
  insort *B=(insort *)b;
  return(A->metric-B->metric);
}

static vertex *vertex_num_sel(graph *g,int num){
  vertex *v=g->verticies;
  if(num<0)return 0;
  while(v){
    if(v->selected){
      if(!num)
	break;
      else
	num--;
    }
    v=v->next;
  }
  return v;
}

static void prepopulate(mesh *m,int length_limit){
  // sort all verticies in ascending order by their number of potential edges 
  int i=0;
  int num=0;
  insort index[m->g->vertex_num];
  vertex *v=m->g->verticies;

  while(v){
    if(v->active == m->active_current){
      index[num].v=v;
      index[num].metric = select_available(m,v,0,0);
      num++;
    }
    v=v->next;
  }
  qsort(index,num,sizeof(*index),insort_c);

  // populate in ascending order
  for(i=0;i<num;i++){
    int intersections=0;
    int edges=0;
    v = index[i].v;
    
    // does this vertex already have edges?
    {
      edge_list *el=v->edges;
      while(el){
	edges++;
	el=el->next;
	if(edges>=2)break;
      }
    }
    if(edges>=2)continue;

    // it's possible some intersections will be necessary, but go for
    // fewest possible
    while(edges<2 && intersections<10){
      int count = select_available(m,v,length_limit,intersections);
      if(count){
	vertex *short0=0;
	vertex *short1=0;
	vertex *w=m->g->verticies;
	long d0;
	long d1;
	
	if(length_limit){
	  // choose two at random 
	  int a=random_number()%count;
	  int b=-1;

	  if(count>1)
	    while(b==-1){
	      b=random_number()%count;
	      if(b==a)b=-1;
	    }

	  short0=vertex_num_sel(m->g,a);
	  short1=vertex_num_sel(m->g,b);

	}else{
	  // used with region-constrined meshes
	  // of the possible edges, choose the shortest two
	  while(w){
	    if(w!=v && w->selected){
	      int xd=w->orig_x-v->orig_x;
	      int yd=w->orig_y-v->orig_y;
	      long d=xd*xd+yd*yd;
	      if(!short0){
		short0=w;
		d0=d;
	      }else if(!short1 || d<d1){
		if(d<d0){
		  short1=short0;
		  d1=d0;
		  short0=w;
		  d0=d;
		}else{
		  short1=w;
		  d1=d;
		}
	      }
	    }
	    w=w->next;
	  }
	}
	
	if(short0){
	  add_edge(m->g,v,short0);
	  edges++;
	  m->g->objective +=intersections;
	  if(intersections)m->g->objective_lessthan=1;
	}
	if(edges<2 && short1){
	  add_edge(m->g,v,short1);
	  edges++;
	  m->g->objective +=intersections;
	  if(intersections)m->g->objective_lessthan=1;
	}
      }
      intersections++;
    }
  }
}

// the spanning walk is to make an attempt at a single, connected graph.
static void span_depth_first2(mesh *m,vertex *current, float length_limit){
  
  current->grabbed=1; // overloaded; "we walked this already"
  while(1){
    // prefer walking along edges that already exist
    {
      edge_list *el=current->edges;
      
      while(el){
	edge *e=el->edge;
	vertex *v;
	if(e->A==current)
	  v=e->B;
	else
	  v=e->A;

	if(!v->grabbed){
	  span_depth_first2(m, v, length_limit);
	}

	el=el->next;
      }
    }

    // now walk any possible edges that have not been walked
    {
      int count=select_available(m,current,length_limit,0);
      int count2=0;
      int choice;
      vertex *v = m->g->verticies;
      
      // filter out already-walked edges 
      while(v){
	if(v->grabbed && v->selected){ // grabbed is also overloaded to mean walked
	  v->selected = 0;
	  count--;
	}
	v=v->next;
      }
     
      if(count == 0) return;
      
      choice = random_number()%count;
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
}

static void random_populate(mesh *m,vertex *current,int dense_128, float length_limit){
  int count=select_available(m,current,length_limit,0);
  if(count){
    vertex *v = m->g->verticies;
    while(v){
      if(v->active == m->active_current && v->selected && random_yes(dense_128)){
	add_edge(m->g,v,current);
	v->selected=0;
      }
      v=v->next;
    }
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
  region_init(); // clear it

  // used for intersection calcs
  {
    int x,y;
    vertex *v = g->verticies;
    for(y=0;y<m->height;y++)
      for(x=0;x<m->width;x++){
	v->orig_x=x*50; // not a random number
	v->orig_y=y*50; // not a random number
	v=v->next;
      }
  }

  g->objective = 0;
  g->objective_lessthan = 0;
  m->active_max=0;
}

static void generate_mesh2(mesh *m, int density_128, float length_limit){ 
  vertex *v;
  int i;

  length_limit*=50;
  length_limit*=length_limit;

  for(i=0;i<=m->active_max;i++){
    m->active_current=i;

    if(have_region())
      prepopulate(m,0);
    
    /* connect the graph into as few discrete sections as possible */
    v = m->g->verticies;
    while(v){
      v->grabbed = 0;
      v=v->next;
    }
    
    v = m->g->verticies;
    // make sure we walk all verticies
    while(v){
      if(v->active == m->active_current && !v->grabbed)
	span_depth_first2(m, m->g->verticies, length_limit);
      v=v->next;
    }
    
    if(!have_region())
      prepopulate(m,length_limit);
    
    /* now iterate the whole mesh adding random edges */
    v=m->g->verticies;
    while(v){
      random_populate(m, v, density_128, length_limit);
    v=v->next;
    }
  }
}

void generate_freeform(graph *g, int order){
  mesh m;
  random_seed(order+1);
  mesh_setup(g, &m, order, 0);

  generate_mesh2(&m,48,4);

  randomize_verticies(g);
  if(order*.03<.3)
    arrange_verticies_polycircle(g,4,0,order*.03,0,0,0);
  else
    arrange_verticies_polycircle(g,4,0,.3,0,0,0);

}

void generate_shape(graph *g, int order){
  int mod=0;
  int dens=64;
  int min=8;
  mesh m;
  random_seed(order+1);

  switch(order%13){
  case 0: // star
    mod=10; break;
  case 1:
    break;
  case 2: // dashed circle
    dens=48; break;
  case 3: // bifur
    dens=80; break;
  case 4:
    break;
  case 5:
    min = 12;
    dens = 10;
    break;
  case 6:
    min = 10;
    break;
  case 7:
    min = 10;
    break;
  case 8:
    min = 10;
    break;
  case 9:
    min = 10;
    break;
  case 10: // ring
    dens=128;
    min = 11;
    break;
  case 11:
    min = 12;
    break;
  case 12: // target
    min = 14;
    break;
  }

  mesh_setup(g, &m, (order>min?order:min), mod);
  randomize_verticies(g);

  switch(order % 13){
  case 0: // star
    arrange_region_star(g); break; //4
  case 1: // rainbow
    arrange_region_rainbow(g); break; //9
  case 2: // dashed circle
    arrange_region_dashed_circle(g); break; //0
  case 3: // bifur
    arrange_region_bifur(g); break; //0
  case 4: // dairyqueen
    arrange_region_dairyqueen(g); break; //0
  case 5: // cloud
    arrange_region_cloud(g); break; //0
  case 6: // storm
    arrange_region_storm(g); break; //11
  case 7: // plus;
      arrange_region_plus(g); break; //2
  case 8: 
    arrange_region_hole3(g); break; //4
  case 9: 
    arrange_region_hole4(g); break; //15
  case 10: // ring
    arrange_region_ring(g); break; //29
  case 11: 
    arrange_region_ovals(g); break; //95
  case 12: // target
    arrange_region_target(g); break; //108
  }

  m.active_max=region_layout(g);
  generate_mesh2(&m,dens,0);
}
