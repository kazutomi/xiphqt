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

typedef struct {
  vertex **v;
  edge_list *embed_list;
  int width;
  int height;
} mesh;

typedef struct {
  int vnum[8];
  vertex *center;
  mesh   *m;
} neighbors_grid;

typedef struct {
  int vnum[8];
  int num;
} neighbors_list;

/* The 'embed_list' is a set of edges that don't obey or neighboring
   intersection calculation mode and are thus tracked
   seperately/explicitly.  They're added to the main graph after the
   rest of the graph is generated. */

/* add edge to the embed_list */
static void embedlist_add_edge(mesh *m, vertex *A, vertex *B){
  edge *e = new_edge(A,B);
  m->embed_list = add_edge_to_list(m->embed_list,e);
}

/* move embed_list edges into the real graph */
static void embedlist_add_to_mesh(graph *g, mesh *m){
  edge_list *el = m->embed_list;

  /* move the edges out of the embed_list and add them to the main graph */
  while(el){
    edge *e = el->edge;
    el->edge = 0;

    insert_edge(g,e);
    el=el->next;
  }

  release_edge_list(m->embed_list);
  m->embed_list=0; /* be pedantic */

}

static int embedlist_intersects(mesh *m, vertex *A, vertex *B){
  edge_list *el = m->embed_list;
  double dummy_x,dummy_y;
  
  while(el){
    edge *e = el->edge;
    
    if(intersects(A,B,e->A,e->B,&dummy_x,&dummy_y))
      return 1;

    el=el->next;
  }
  return 0;

}

static void embedlist_filter_intersections(neighbors_grid *ng){
  int i;
  vertex *A = ng->center;

  for(i=0;i<8;i++){
    if(ng->vnum[i] != -1){
      vertex *B = ng->m->v[ng->vnum[i]];

      if(embedlist_intersects(ng->m,A,B))
	ng->vnum[i]=-1;
    }
  }
}

static int embedlist_contains_vertex(mesh *m,vertex *v){
  edge_list *el = m->embed_list;
  
  while(el){
    edge *e = el->edge;

    if(e->A == v) return 1;
    if(e->B == v) return 1;

    el=el->next;
  }
  return 0;
}

static int embedlist_vertex_poisoned(mesh *m, vertex *v){
  return v->selected;
}

static void poison_vertex(mesh *m, vertex *v){
  v->selected=1;
}

/* neighboring intersection model */

static void populate_neighbors(int vnum, mesh *m, 
			       neighbors_grid *ng){
  int width = m->width;
  int y = vnum/width;
  int x = vnum - (y*width);
  int i;

  for(i=0;i<8;i++)ng->vnum[i]=-1;


  ng->center = m->v[vnum];
  ng->m = m;

  if(y-1 >= 0){
    if(x-1 >= 0)        ng->vnum[0]= (y-1)*width+(x-1);
                        ng->vnum[1]= (y-1)*width+x;
    if(x+1 <  m->width) ng->vnum[2]= (y-1)*width+(x+1);
  }

  if(x-1   >= 0)        ng->vnum[3]= y*width+(x-1);
  if(x+1   <  m->width) ng->vnum[4]= y*width+(x+1);

  if(y+1   < m->height){
    if(x-1 >= 0)        ng->vnum[5]= (y+1)*width+(x-1);
                        ng->vnum[6]= (y+1)*width+x;
    if(x+1 <  m->width) ng->vnum[7]= (y+1)*width+(x+1);
  }

}

// eliminate from neighbor structs the verticies that already have at
// least one edge
static void filter_spanned_neighbors(neighbors_grid *ng,
				     neighbors_list *nl){
  int i;
  int count=0;
  for(i=0;i<8;i++)
    if(ng->vnum[i]==-1 || ng->m->v[ng->vnum[i]]->edges){
      ng->vnum[i]=-1;
    }else{
      nl->vnum[count++]=ng->vnum[i];
    }
  nl->num=count;

}

// eliminate from neighbor struct any verticies to which we can't make
// an edge without crossing another edge.  Only 0,2,5,7 possible.
static void filter_intersections(neighbors_grid *ng){
  int i;
  for(i=0;i<8;i++){
    switch(i){
    case 0: 
      if(ng->vnum[1] != -1 && 
	 ng->vnum[3] != -1 &&
	 exists_edge(ng->m->v[ng->vnum[1]],
		     ng->m->v[ng->vnum[3]]))
	ng->vnum[i]=-1;
      break;
      
    case 2: 
      if(ng->vnum[1] != -1 && 
	 ng->vnum[4] != -1 &&
	 exists_edge(ng->m->v[ng->vnum[1]],
		     ng->m->v[ng->vnum[4]]))
	ng->vnum[i]=-1;
      break;
      
    case 5: 
      if(ng->vnum[3] != -1 && 
	 ng->vnum[6] != -1 &&
	 exists_edge(ng->m->v[ng->vnum[3]],
		     ng->m->v[ng->vnum[6]]))
	ng->vnum[i]=-1;
      break;
      
    case 7: 
      if(ng->vnum[4] != -1 && 
	 ng->vnum[6] != -1 &&
	 exists_edge(ng->m->v[ng->vnum[4]],
		     ng->m->v[ng->vnum[6]]))
	ng->vnum[i]=-1;
      break;
    } 
  }
  embedlist_filter_intersections(ng);
}

/* eliminate verticies we've already connected to */
static void filter_edges(neighbors_grid *ng,
			 neighbors_list *nl){

  vertex *v=ng->center;
  int count=0,i;
  for(i=0;i<8;i++){
    if(ng->vnum[i]!=-1){
      if(!exists_edge(v,ng->m->v[ng->vnum[i]]))
	nl->vnum[count++]=ng->vnum[i];
      else
	ng->vnum[i]=-1;
    }
  }
  nl->num=count;
}

static void random_populate(graph *g, int current, mesh *m, int min_connect, int prob_128){
  int num_edges=0,i;
  neighbors_grid ng;
  neighbors_list nl;
  populate_neighbors(current, m, &ng);
  filter_intersections(&ng);
  filter_edges(&ng,&nl);

  {
    edge_list *el=m->v[current]->edges;
    while(el){
      num_edges++;
      el=el->next;
    }
  }

  while(num_edges<min_connect && nl.num){
    int choice = random_number() % nl.num;
    add_edge(g,m->v[current], m->v[nl.vnum[choice]]);
    num_edges++;
    filter_intersections(&ng);
    filter_edges(&ng,&nl);
  }
  
  for(i=0;i<nl.num;i++)
    if(random_yes(prob_128)){
      num_edges++;
      add_edge(g,m->v[current], m->v[nl.vnum[i]]);
    }
}

static void span_depth_first(graph *g,int current, mesh *m){
  neighbors_grid ng;
  neighbors_list nl;

  while(1){
    populate_neighbors(current, m, &ng);
    // don't reverse the order of the next two
    filter_intersections(&ng);
    filter_spanned_neighbors(&ng,&nl);
    if(nl.num == 0) break;
    
    {
      int choice = random_number() % nl.num;
      add_edge(g,m->v[current], m->v[nl.vnum[choice]]);
      
      span_depth_first(g,nl.vnum[choice], m);
    }
  }
}

/* nastiness adds long edges along the outer perimeter to make it
   harder to rely on verticies always being near each other; mesh 2
   takes this further, but we can add some of the same flavor to
   mesh1. */

static void nasty_horizontal(graph *g, mesh *m, int A, int B, int limit){
  if(limit == 0) return;
  if(A+2 > B)return; /* adjacent is too close */

  add_edge(g,m->v[A],m->v[B]);

  A++;
  B--;
  nasty_horizontal(g,m,A,B,limit-1);
}

static void nasty_vertical(graph *g, mesh *m, int A, int B, int limit){
  if(limit == 0) return;
  if(A+(m->width*2) > B)return; /* adjacent is too close */

  add_edge(g,m->v[A],m->v[B]);

  A+=m->width;
  B-=m->width;
  nasty_vertical(g,m,A,B,limit-1);
}

/* Don't use this along with k5 embedding; the assumptions the
   nastiness algorithm makes about solvable conditions won't always
   coexist with the assumptions the k5 embedding makes about solvable
   conditions. */
static void mesh_nastiness(graph *g, mesh *m, int limit){

  nasty_horizontal(g,m,0,m->width-1, limit);
  nasty_horizontal(g,m,(m->height-1)*m->width,m->width*m->height-1, limit);

  nasty_vertical(g,m,0,(m->height-1)*m->width,limit);
  nasty_vertical(g,m,m->width-1,m->width*m->height-1, limit);
}

/* Embed one k5 in the solved graph */
/* Don't use this along with 'nastiness'; the assumptions the
   nastiness algorithm makes about solvable conditions won't always
   coexist with the assumptions that non-planar embedding makes about
   solvable conditions. */

static void mesh_embed_k5(graph *g, mesh *m,int x, int y){

  /* Add the k5s up front in their own special edge list; This list
     will also be checked explicitly by the various neighboring
     algorithms as the k5's edges don't all conceptually work within
     the implicit neighboring algorithm we're using.  Also, by using a
     special edge list and not adding the k5 edges to the vertex edge
     lists up front, we can still use the unmodified initial spanning
     walk algorithm. */

  int w = m->width;

  vertex *A = m->v[y*w+x+1];
  vertex *B = m->v[(y+1)*w+x+1];
  vertex *C = m->v[(y+1)*w+x+2];
  vertex *D = m->v[(y+1)*w+x+3];
  vertex *E = m->v[(y+2)*w+x];

  // poisoned verticies are already inside another kernel (the regular
  // mesh is deflectable and thus not really regular)
  if(embedlist_vertex_poisoned(m,A))return;
  if(embedlist_vertex_poisoned(m,B))return;
  if(embedlist_vertex_poisoned(m,C))return;
  if(embedlist_vertex_poisoned(m,D))return;
  if(embedlist_vertex_poisoned(m,E))return;

  // the way k5 works we don't need to poison the internal verticies

  embedlist_add_edge(m, A,B);
  embedlist_add_edge(m, A,C);
  embedlist_add_edge(m, A,D);
  embedlist_add_edge(m, A,E);
  embedlist_add_edge(m, B,C);
  embedlist_add_edge(m, B,D);
  embedlist_add_edge(m, B,E);
  embedlist_add_edge(m, C,D);
  embedlist_add_edge(m, C,E);
  embedlist_add_edge(m, D,E);
  g->objective++;
}

/* Embed one k3,3 in the solved graph */
/* Don't use this along with 'nastiness'; the assumptions the
   nastiness algorithm makes about solvable conditions won't always
   coexist with the assumptions that k5 embedding makes about solvable
   conditions. */
static void mesh_embed_k33(graph *g, mesh *m, int x, int y){

  /* same disclaimers as k5 */
  /* the k3,3 embedding works with the standard walk algorithm only
     because an edge with an endpoint exactly on another edge is
     considered an intersection. */
  /* the way it is added, the walk/population can add additional edges
     inside the embedded kernel; this is fine, the population will be
     certain not to introduce intersections. */

  int w = m->width;

  vertex *A = m->v[y*w+x];
  vertex *B = m->v[y*w+x+1];
  vertex *C = m->v[y*w+x+2];
  vertex *D = m->v[(y+1)*w+x];
  vertex *E = m->v[(y+1)*w+x+1];
  vertex *F = m->v[(y+1)*w+x+2];

  // poisoned verticies are already inside another kernel (the regular
  // mesh is deflectable and thus not really regular)
  if(embedlist_vertex_poisoned(m,A))return;
  if(embedlist_vertex_poisoned(m,B))return;
  if(embedlist_vertex_poisoned(m,C))return;
  if(embedlist_vertex_poisoned(m,D))return;
  if(embedlist_vertex_poisoned(m,E))return;
  if(embedlist_vertex_poisoned(m,F))return;

  // check that verticies we want to poison ourselves are not already in use
  if(embedlist_contains_vertex(m,B))return;
  if(embedlist_contains_vertex(m,E))return;
  /* B and E are internal according to x/y, but according to the
     position in the mesh, they're on the outside.  Poison them so
     that they're explicitly marked inside. */
  poison_vertex(m,B);
  poison_vertex(m,E);

  /* need to mode two of the intersections to avoid  unwanted
     intersections (not spurious; they are in fact intersections until
     moved) */

  B->y+=2;
  E->y-=2;
  
  embedlist_add_edge(m, A,C);
  embedlist_add_edge(m, A,D);
  embedlist_add_edge(m, A,E);
  embedlist_add_edge(m, B,C);
  embedlist_add_edge(m, B,D);
  embedlist_add_edge(m, B,E);
  embedlist_add_edge(m, C,F);
  embedlist_add_edge(m, D,F);
  embedlist_add_edge(m, E,F);
  g->objective++;

}

/* Embed one non-miminal k3,3 in the solved graph */
/* Don't use this along with 'nastiness'; the assumptions the
   nastiness algorithm makes about solvable conditions won't always
   coexist with the assumptions that k5 embedding makes about solvable
   conditions. */
static void mesh_embed_bigk33(graph *g, mesh *m, int x, int y){

  /* as above */

  int w = m->width;

  vertex *A = m->v[(y+2)*w+x];
  vertex *B = m->v[(y+1)*w+x+2];
  vertex *C = m->v[y*w+x+4];
  vertex *D = m->v[(y+4)*w+x+1];
  vertex *E = m->v[(y+3)*w+x+3];
  vertex *F = m->v[(y+2)*w+x+5];

  // poisoned verticies are already inside another kernel (the regular
  // mesh is deflectable and thus not really regular)
  if(embedlist_vertex_poisoned(m,A))return;
  if(embedlist_vertex_poisoned(m,B))return;
  if(embedlist_vertex_poisoned(m,C))return;
  if(embedlist_vertex_poisoned(m,D))return;
  if(embedlist_vertex_poisoned(m,E))return;
  if(embedlist_vertex_poisoned(m,F))return;

  // check that verticies we want to poison ourselves are not already in use
  if(embedlist_contains_vertex(m,B))return;
  if(embedlist_contains_vertex(m,E))return;
  /* B and E are internal according to x/y, but according to the
     position in the mesh, they're on the outside.  Poison them so
     that they're explicitly marked inside. */
  poison_vertex(m,B);
  poison_vertex(m,E);

  /* need to move two of the intersections to avoid  unwanted
     intersections (not spurious; they are in fact intersections until
     moved) */
  
  B->y+=2;
  E->y-=2;

  embedlist_add_edge(m, A,C);
  embedlist_add_edge(m, A,D);
  embedlist_add_edge(m, A,E);
  embedlist_add_edge(m, B,C);
  embedlist_add_edge(m, B,D);
  embedlist_add_edge(m, B,E);
  embedlist_add_edge(m, C,F);
  embedlist_add_edge(m, D,F);
  embedlist_add_edge(m, E,F);
  
  g->objective++;
}

static void mesh_embed_recurse(graph *g,mesh *m,int x, int y, int w, int h, int k5, int k33, int bigk33){
  int xd,yd,wd,hd;

  // not minimal spacing; the k33 needs vertical offset, but the others are larger just to space them out.
  if( bigk33 && w>=6 && h>=5 ){
    wd = 5;
    hd = 4;
    xd = random_number() % (w-wd);
    yd = random_number() % (h-hd);
    mesh_embed_bigk33(g,m,x+xd,y+yd);
  }else if(k5 && w>=4 && h>=3){
    wd = 3;
    hd = 2;
    xd = random_number() % (w-wd);
    yd = random_number() % (h-hd);
    mesh_embed_k5(g,m,x+xd,y+yd);
  }else if(k33 && w>=3 && h>=2 ){
    wd = 2;
    hd = 1;
    xd = random_number() % (w-wd);
    yd = random_number() % (h-hd);
    mesh_embed_k33(g,m,x+xd,y+yd);
  }else
    return;

  mesh_embed_recurse(g,m, x,         y,       w,    yd+1, k5,k33,bigk33);
  mesh_embed_recurse(g,m, x,   y+yd+hd,       w, h-yd-hd, k5,k33,bigk33);

  mesh_embed_recurse(g,m, x,      y+yd,    xd+1,    hd+1, k5,k33,bigk33);
  mesh_embed_recurse(g,m, x+xd+wd,y+yd, w-xd-wd,    hd+1, k5,k33,bigk33);
}

/* Embed k5s and k3,3s in the solved graph in such a way that we know each added
   non-planar kernel adds exactly one and only one certain intersection. */
/* Don't use this along with 'nastiness'; the assumptions the
   nastiness algorithm makes about solvable conditions won't always
   coexist with the assumptions that non-planar embedding makes about
   solvable conditions. */


static void mesh_embed_nonplanar(graph *g, mesh *m,int k5, int k33, int bigk33){
  // selection is used as a poison flag during embedding
  deselect_verticies(g);
  mesh_embed_recurse(g, m, 0,0,m->width,m->height, k5,k33,bigk33);
  deselect_verticies(g);
}

/* Initial generation setup */

static void mesh_setup(graph *g, mesh *m, int order, int divis){
  int flag=0;
  int wiggle=0;
  int n;
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
  m->embed_list=0;

  // used for rogue calcs
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

static void mesh_flatten(graph *g,mesh *m){
  /* a flat vector is easier to address while building the mesh */
  int i;
  vertex *v=g->verticies;
  for(i=0;i<m->width*m->height;i++){
    m->v[i]=v;
    v=v->next;
  }
}

static void generate_mesh(graph *g, mesh *m, 
			  int order, 
			  int density_128){

  /* first walk a random spanning tree */
  span_depth_first(g, 0, m);
  
  /* now iterate the whole mesh adding random edges */
  {
    int i;
    for(i=0;i<m->width*m->height;i++)
      random_populate(g, i, m, 2, density_128);
  }
}

void generate_simple(graph *g, int order){
  mesh m;
  random_seed(order);
  mesh_setup(g,&m, order, 0);
  m.v=alloca(m.width*m.height * sizeof(*m.v));
  mesh_flatten(g,&m);

  generate_mesh(g,&m,order,40);
  randomize_verticies(g);

  if((m.width*m.height)&1)
    arrange_verticies_circle(g,0,0);
  else
    arrange_verticies_circle(g,M_PI/2,M_PI/2);
}

void generate_sparse(graph *g, int order){
  mesh m;
  random_seed(order);
  mesh_setup(g,&m, order, 3);
  m.v=alloca(m.width*m.height * sizeof(*m.v));
  mesh_flatten(g,&m);

  generate_mesh(g,&m,order,2);
  mesh_nastiness(g,&m,-1);
  randomize_verticies(g);
  switch((order-1)%4){
  case 0:
    arrange_verticies_polygon(g,3,0,1.15,0,+65,1.1,1.);
    break;
  case 1:
    arrange_verticies_polygon(g,3,-M_PI/2,1.,+40,0,1.1,1.);
    break;
  case 2:
    arrange_verticies_polygon(g,3,-M_PI,1.15,0,-65,1.1,1.);
    break;
  case 3:
    arrange_verticies_polygon(g,3,-M_PI*3/2,1.,-40,0,1.1,1.);
    break;
  }
}

void generate_nasty(graph *g, int order){
  mesh m;
  random_seed(order);
  mesh_setup(g,&m, order,4);
  m.v=alloca(m.width*m.height * sizeof(*m.v));
  mesh_flatten(g,&m);

  generate_mesh(g,&m,order,32);
  mesh_nastiness(g,&m,-1);
  randomize_verticies(g);
  switch(order%2){
  case 0:
    arrange_verticies_polygon(g,4,0,1.,0,0,1.1,1.);
    break;
  case 1:
    arrange_verticies_polygon(g,4,M_PI/4,1.,0,0,1.2,1.1);
    break;
  }
}

void generate_embed(graph *g, int order){
  mesh m;
  random_seed(order+347);
  mesh_setup(g,&m, order, 6);
  m.v=alloca(m.width*m.height * sizeof(*m.v));
  mesh_flatten(g,&m);

  mesh_embed_nonplanar(g,&m,1,1,1);
  generate_mesh(g,&m,order,48);
  embedlist_add_to_mesh(g,&m);

  randomize_verticies(g);

  switch(order%2){
  case 0:
    arrange_verticies_polygon(g,6,0,1.,0,0,1.1,1.);
    break;
  case 1:
    arrange_verticies_polygon(g,6,M_PI/6,1.,0,0,1.1,1.);
    break;
  }
}

void generate_crest(graph *g, int order){
  int n;
  mesh m;
  random_seed(order);
  mesh_setup(g,&m, order,0);
  m.v=alloca(m.width*m.height * sizeof(*m.v));
  mesh_flatten(g,&m);

  generate_mesh(g,&m,order,128);
  n=m.width*m.height;
  arrange_verticies_circle(g,M_PI/n,M_PI/n);
}
