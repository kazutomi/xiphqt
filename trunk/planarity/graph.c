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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "graph.h"
#include "random.h"
#include "timer.h"
#include "gameboard.h"
#define CHUNK 64

static vertex *vertex_pool=0;
static edge *edge_pool=0;
static edge_list *edge_list_pool=0;
static intersection *intersection_pool=0;

/* mesh/board state */

/************************ edge list maint operations ********************/
edge_list *add_edge_to_list(edge_list *l, edge *e){
  edge_list *ret;
  
  if(edge_list_pool==0){
    int i;
    edge_list_pool = calloc(CHUNK,sizeof(*edge_list_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      edge_list_pool[i].next=edge_list_pool+i+1;
  }

  ret=edge_list_pool;
  edge_list_pool=ret->next;

  ret->edge=e;
  ret->next=l;
  return ret;
}

/* releases the edge list but not the edges */
void release_edge_list(edge_list *el){
  if(el){
    edge_list *end=el;
    while(end->next)end=end->next;
  
    end->next = edge_list_pool;
    edge_list_pool = el;
  }
}

/* releases the edge list but not the edges */
static void release_vertex_edge_list(vertex *v){
  edge_list *el = v->edges;
  release_edge_list(el);
  v->edges=0;
}

/************************ intersection maint operations ********************/

static intersection *new_intersection(){
  intersection *ret;
  
  if(intersection_pool==0){
    int i;
    intersection_pool = calloc(CHUNK,sizeof(*intersection_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      intersection_pool[i].next=intersection_pool+i+1;
  }

  ret=intersection_pool;
  intersection_pool=ret->next;
  return ret;
}

static void add_paired_intersection(edge *a, edge *b, double x, double y){
  intersection *A=new_intersection();
  intersection *B=new_intersection();

  A->paired=B;
  B->paired=A;

  A->next=a->i.next;
  if(A->next)
    A->next->prev=A;
  A->prev=&(a->i);
  a->i.next=A;

  B->next=b->i.next;
  if(B->next)
    B->next->prev=B;
  B->prev=&(b->i);
  b->i.next=B;

  A->x=B->x=x;
  A->y=B->y=y;
}

static void release_intersection(intersection *i){
  memset(i,0,sizeof(*i));
  i->next=intersection_pool;
  intersection_pool=i;
}

static void release_intersection_list(edge *e){
  intersection *i=e->i.next;
  
  while(i!=0){
    intersection *next=i->next;
    release_intersection(i);
    i=next;
  }
  e->i.next=0;
}

static int release_paired_intersection_list(edge *e){
  intersection *i=e->i.next;
  int count=0;

  while(i){
    intersection *next=i->next;
    intersection *j=i->paired;

    j->prev->next=j->next;
    if(j->next)
      j->next->prev=j->prev;

    release_intersection(i);
    release_intersection(j);
    i=next;
    count++;
  }
  e->i.next=0;
  return count;
}

/************************ edge maint operations ******************************/
edge *new_edge(vertex *A, vertex *B){
  edge *ret;
  
  if(edge_pool==0){
    int i;
    edge_pool = calloc(CHUNK,sizeof(*edge_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      edge_pool[i].next=edge_pool+i+1;
  }

  ret=edge_pool;
  edge_pool=ret->next;

  ret->A=A;
  ret->B=B;
  ret->active=0;
  ret->i.next=0;

  return ret;
}

/* makes a new egde and adds it to the vertex and graph edge lists */
edge *add_edge(graph *g, vertex *A, vertex *B){
  edge *ret = new_edge(A,B);

  ret->next=g->edges;
  g->edges=ret;
  
  A->edges=add_edge_to_list(A->edges,ret);
  B->edges=add_edge_to_list(B->edges,ret);

  g->num_edges++;

  return ret;
}

/* adds existing edge to the vertex and graph edge lists, but only if
   it's not already there */
void insert_edge(graph *g, edge *e){
  vertex *A = e->A;
  vertex *B = e->B;
  
  if(exists_edge(A,B)){
    // already a matching edge; release this one
    release_intersection_list(e);
    e->next=edge_pool;
    edge_pool=e;
  }else{
    e->next=g->edges;
    g->edges=e;
    
    A->edges=add_edge_to_list(A->edges,e);
    B->edges=add_edge_to_list(B->edges,e);
  
    g->num_edges++;
  }
}

static void release_edges(graph *g){
  edge *e = g->edges;

  while(e){
    edge *next=e->next;
    release_intersection_list(e);
    e->next=edge_pool;
    edge_pool=e;
    e=next;
  }
  g->edges=0;
  g->num_edges=0;
  g->num_edges_active=0;
}

int intersectsV(vertex *L1, vertex *L2, vertex *M1, vertex *M2, double *xo, double *yo){
  // edges that share a vertex don't intersect by definition
  if(L1==M1) return 0;
  if(L1==M2) return 0;
  if(L2==M1) return 0;
  if(L2==M2) return 0;

  return intersects(L1->x,L1->y,L2->x,L2->y,M1->x,M1->y,M2->x,M2->y,xo,yo);
}

int intersects(int L1x, int L1y,
		int L2x, int L2y,
		int M1x, int M1y,
		int M2x, int M2y,
		double *xo, double *yo){

  /* y = ax + b */
  float La=0;
  float Lb=0;
  float Ma=0;
  float Mb=0;

  if(L1x != L2x){
    La = (float)(L2y - L1y) / (L2x - L1x);
    Lb = (L1y - L1x * La);
  }
  
  if(M1x != M2x){
    Ma = (float)(M2y - M1y) / (M2x - M1x);
    Mb = (M1y - M1x * Ma);
  }
  
  if(L1x == L2x){
    // L is vertical line

    if(M1x == M2x){
      // M is also a vertical line

      if(L1x == M1x){
	// L and M vertical on same x, overlap?
	if(M1y > L1y && 
	   M1y > L2y &&
	   M2y > L1y &&
	   M2y > L2y) return 0;
	if(M1y < L1y && 
	   M1y < L2y &&
	   M2y < L1y &&
	   M2y < L2y) return 0;

	{
	  double y1=max( min(M1y,M2y), min(L1y, L2y));
	  double y2=min( max(M1y,M2y), max(L1y, L2y));

	  *xo = M1x;
	  *yo = (y1+y2)*.5;

	}

      }else
	// L and M vertical, different x
	return 0;
      
    }else{
      // L vertical, M not vertical
      
      // needed if L is vertical and M is horizontal
      if(L1x < M1x && L1x < M2x) return 0;
      if(L1x > M1x && L1x > M2x) return 0;

      {
	float y = Ma*L1x + Mb;
	
	if(y < L1y && y < L2y) return 0;
	if(y > L1y && y > L2y) return 0;
	if(y < M1y && y < M2y) return 0;
	if(y > M1y && y > M2y) return 0;
	
	*xo = L1x;
	*yo=y;
      }
    }
  }else{

    if(M1x == M2x){
      // M vertical, L not vertical

      // needed if L is vertical and M is horizontal
      if(M1x < L1x && M1x < L2x) return 0;
      if(M1x > L1x && M1x > L2x) return 0;

      {
	float y = La*M1x + Lb;
	
	if(y < L1y && y < L2y) return 0;
	if(y > L1y && y > L2y) return 0;
	if(y < M1y && y < M2y) return 0;
	if(y > M1y && y > M2y) return 0;
	
	*xo = M1x;
	*yo=y;
      }
    }else{

      // L and M both have non-infinite slope
      if(La == Ma){
	//L and M have the same slope
	if(Mb != Lb) return 0; 
	
	// two segments on same line...
	if(M1x > L1x && 
	   M1x > L2x &&
	   M2x > L1x &&
	   M2x > L2x) return 0;
	if(M1x < L1x && 
	   M1x < L2x &&
	   M2x < L1x &&
	   M2x < L2x) return 0;
	
	{
	  double x1=max( min(M1x,M2x), min(L1x, L2x));
	  double x2=min( max(M1x,M2x), max(L1x, L2x));
	  double y1=max( min(M1y,M2y), min(L1y, L2y));
	  double y2=min( max(M1y,M2y), max(L1y, L2y));
	  
	  *xo = (x1+x2)*.5;
	  *yo = (y1+y2)*.5;
	  
	}
	
	
      }else{
	// finally typical case: L and M have different non-infinite slopes
	float x = (Mb-Lb) / (La - Ma);
	
	if(x < L1x && x < L2x) return 0;
	if(x > L1x && x > L2x) return 0;
	if(x < M1x && x < M2x) return 0;
	if(x > M1x && x > M2x) return 0;
	
	*xo = x;
	*yo = La*x + Lb;
      }
    }
  }

  return 1;
}

static void activate_edge(graph *g, edge *e){
  /* computes all intersections */
  if(!e->active && e->A->active && e->B->active){
    edge *test=g->edges;
    while(test){
      if(test != e && test->active){
	double x,y;
	if(intersectsV(e->A,e->B,test->A,test->B,&x,&y)){
	  add_paired_intersection(e,test,x,y);
	  g->active_intersections++;

	}
      }
      test=test->next;
    }
    e->active=1;
    g->num_edges_active++;
    if(g->num_edges_active == g->num_edges && g->original_intersections==0)
      g->original_intersections = g->active_intersections;
  }
}

static void deactivate_edge(graph *g, edge *e){
  /* releases all associated intersections */
  if(e->active){
    g->active_intersections -= 
      release_paired_intersection_list(e);
    g->num_edges_active--;
    e->active=0;
  }
}

int exists_edge(vertex *a, vertex *b){
  edge_list *el=a->edges;
  while(el){
    if(el->edge->A == b) return 1;
    if(el->edge->B == b) return 1;
    el=el->next;
  }
  return 0;
}

/*********************** vertex maint operations *************************/
static vertex *get_vertex(graph *g){
  vertex *ret;
  
  if(vertex_pool==0){
    int i;
    vertex_pool = calloc(CHUNK,sizeof(*vertex_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      vertex_pool[i].next=vertex_pool+i+1;
  }

  ret=vertex_pool;
  vertex_pool=ret->next;

  ret->x=0;
  ret->y=0;
  ret->active=0;
  ret->selected=0;
  ret->selected_volatile=0;
  ret->grabbed=0;
  ret->attached_to_grabbed=0;
  ret->fading=0;
  ret->edges=0;
  ret->num=g->vertex_num++;

  ret->next=g->verticies;
  g->verticies=ret;

  return ret;
}

static void release_vertex(vertex *v){
  release_vertex_edge_list(v);
  v->next=vertex_pool;
  vertex_pool=v;
}

static void set_num_verticies(graph *g, int num){
  /* do it the simple way; release all, link anew */
  vertex *v=g->verticies;

  while(v){
    vertex *next=v->next;
    release_vertex(v);
    v=next;
  }
  g->verticies=0;
  g->vertex_num=0;
  release_edges(g);

  while(num--)
    get_vertex(g);
  g->original_intersections = 0;
  g->active_intersections = 0;
  g->num_edges=0;        // hopefully redundant
  g->num_edges_active=0; // hopefully redundant
}

void graph_release(graph *g){
  set_num_verticies(g,0);
}

void activate_vertex(graph *g,vertex *v){
  edge_list *el=v->edges;
  v->active=1;
  while(el){
    activate_edge(g,el->edge);
    el=el->next;
  }
}

void deactivate_vertex(graph *g, vertex *v){
  edge_list *el=v->edges;
  while(el){
    edge_list *next=el->next;
    deactivate_edge(g,el->edge);
    el=next;
  }
  v->active=0;
}

void activate_verticies(graph *g){
  vertex *v=g->verticies;
  while(v){
    activate_vertex(g,v);
    v=v->next;
  }
}

void grab_vertex(graph *g, vertex *v){
  edge_list *el=v->edges;
  deactivate_vertex(g,v);
  while(el){
    edge_list *next=el->next;
    edge *e=el->edge;
    vertex *other=(e->A==v?e->B:e->A);
    other->attached_to_grabbed=1;
    el=next;
  }
  v->grabbed=1;
}

void grab_selected(graph *g){
  vertex *v = g->verticies;
  while(v){
    if(v->selected){
      edge_list *el=v->edges;
      deactivate_vertex(g,v);
      while(el){
	edge_list *next=el->next;
	edge *e=el->edge;
	vertex *other=(e->A==v?e->B:e->A);
	other->attached_to_grabbed=1;
	el=next;
      }
      v->grabbed=1;
    }
    v=v->next;
  }
}

void ungrab_vertex(graph *g,vertex *v){
  edge_list *el=v->edges;
  activate_vertex(g,v);
  while(el){
    edge_list *next=el->next;
    edge *e=el->edge;
    vertex *other=(e->A==v?e->B:e->A);
    other->attached_to_grabbed=0;
    el=next;
  }
  v->grabbed=0;
}

void ungrab_verticies(graph *g){
  vertex *v = g->verticies;
  while(v){
    if(v->grabbed){
      edge_list *el=v->edges;
      activate_vertex(g,v);
      while(el){
	edge_list *next=el->next;
	edge *e=el->edge;
	vertex *other=(e->A==v?e->B:e->A);
	other->attached_to_grabbed=0;
	el=next;
      }
      v->grabbed=0;
    }
    v=v->next;
  }
}

vertex *find_vertex(graph *g, int x, int y){
  vertex *v = g->verticies;
  vertex *match = 0;

  while(v){
    vertex *next=v->next;
    int xd=x-v->x;
    int yd=y-v->y;
    if(xd*xd + yd*yd <= V_RADIUS_SQ) match=v;
    v=next;
  }
  
  return match;
}

// tenative selection; must be confirmed if next call should not clear
void select_verticies(graph *g,int x1, int y1, int x2, int y2){
  vertex *v = g->verticies;

  if(x1>x2){
    int temp=x1;
    x1=x2;
    x2=temp;
  }

  if(y1>y2){
    int temp=y1;
    y1=y2;
    y2=temp;
  }
  x1-=V_RADIUS;
  x2+=V_RADIUS;
  y1-=V_RADIUS;
  y2+=V_RADIUS;

  while(v){

    if(v->selected_volatile)v->selected=0;

    if(!v->selected){
      if(v->x>=x1 && v->x<=x2 && v->y>=y1 && v->y<=y2){
	v->selected=1;
	v->selected_volatile=1;
      }
    }

    v=v->next;
  }
}

int num_selected_verticies(graph *g){
  vertex *v = g->verticies;
  int count=0;
  while(v){
    if(v->selected)count++;
    v=v->next;
  }
  return count;
}

void deselect_verticies(graph *g){
  vertex *v = g->verticies;
  
  while(v){
    v->selected=0;
    v->selected_volatile=0;
    v=v->next;
  }

}

void commit_volatile_selection(graph *g){
  vertex *v = g->verticies;
  
  while(v){
    v->selected_volatile=0;
    v=v->next;
  }
}

static vertex *split_vertex_list(vertex *v){
  vertex *half=v;
  vertex *prevhalf=v;

  while(v){
    v=v->next;
    if(v)v=v->next;
    prevhalf=half;
    half=half->next;
  }

  prevhalf->next=0;
  return half;
}

static vertex *randomize_helper(vertex *v){
  vertex *w=split_vertex_list(v);
  if(w){
    vertex *a = randomize_helper(v);
    vertex *b = randomize_helper(w);
    v=0;
    w=0;

    while(a && b){
      if(random_yes(64)&1){
	// pull off head of a
	if(w)
	  w=w->next=a;
	else
	  v=w=a;
	a=a->next;
      }else{
	// pull off head of b
	if(w)
	  w=w->next=b;
	else
	  v=w=b;
	b=b->next;
      }
    }
    if(a)
      w->next=a;
    if(b)
      w->next=b;
  }
  return v;
}

void randomize_verticies(graph *g){
  g->verticies=randomize_helper(g->verticies);
}

static void check_vertex_helper(graph *g, vertex *v, int reactivate){
  int flag=0;

  if(v->x>=g->width){
    v->x=g->width-1;
    flag=1;
  }
  if(v->x<0){
    v->x=0;
    flag=1;
  }
  if(v->y>=g->height){
    v->y=g->height-1;
    flag=0;
  }
  if(v->y<0){
    v->y=0;
    flag=1;
  }
  if(flag){
    if(v->edges){
      deactivate_vertex(g,v);
      if(reactivate)activate_vertex(g,v);
    }
  }
}

static void check_vertex(graph *g, vertex *v){
  check_vertex_helper(g,v,1);
}

void check_verticies(graph *g){
  vertex *v=g->verticies;
  while(v){
    vertex *next=v->next;
    check_vertex_helper(g,v,0);
    v=next;
  }

  v=g->verticies;
  while(v){
    vertex *next=v->next;
    activate_vertex(g,v);
    v=next;
  }
}

void move_vertex(graph *g, vertex *v, int x, int y){
  if(!v->grabbed) deactivate_vertex(g,v);
  v->x=x;
  v->y=y;
  check_vertex_helper(g,v,0);
  if(!v->grabbed) activate_vertex(g,v);
}

void move_selected_verticies(graph *g,int dx, int dy){
  vertex *v = g->verticies;
  /* move selected verticies; do not reactivate, done during ungrab */
  v=g->verticies;
  while(v){
    vertex *next=v->next;
    if(v->selected){
      v->x+=dx;
      v->y+=dy;
      check_vertex(g,v);
      //activate_vertex(g,v);
    }
    v=next;
  }

}

void scale_verticies(graph *g,float amount){
  vertex *v=g->verticies;
  int x=g->width/2;
  int y=g->height/2;

  while(v){
    vertex *next=v->next;
    deactivate_vertex(g,v);
    v->x=rint((v->x-x)*amount)+x;
    v->y=rint((v->y-y)*amount)+y;
    v=next;
  }

  v=g->verticies;
  while(v){
    vertex *next=v->next;
    check_vertex(g,v);
    activate_vertex(g,v);
    v=next;
  }
}

vertex *new_board(graph *g, int num_v){
  set_num_verticies(g,num_v);
  return g->verticies;
}

// take the mesh, which is always generated for a 'normal sized' board
// (right now, 800x600), set the original vertex coordinates to the
// 'nominal' board, and move the 'live' vertex positions to be
// centered on what the current board size is.

void impress_location(graph *g){
  int xd = (g->width-g->orig_width)>>1;
  int yd = (g->height-g->orig_height)>>1;
  vertex *v=g->verticies;
  while(v){
    v->orig_x=v->x;
    v->orig_y=v->y;
    v->x+=xd;
    v->y+=yd;
    v=v->next;
  }
}

/******** read/write board **********/

int graph_write(graph *g, FILE *f){
  int i;
  vertex *v=g->verticies;
  edge *e=g->edges;
  vertex **flat = alloca(g->vertex_num*sizeof(*flat));
  int *iflat = alloca(g->vertex_num*sizeof(*iflat));

  i=0;
  while(v){
    flat[v->num]=v;
    iflat[v->num]=i++;
    v=v->next;
  }

  fprintf(f,"scoring %ld %f %f %ld %c %d\n",
	  g->original_intersections,
	  g->intersection_mult, g->objective_mult, (long)get_timer(),
	  (g->objective_lessthan?'*':'='),g->objective);

  fprintf(f,"board %d %d %d %d\n",
	  g->width,g->height,g->orig_width,g->orig_height);

  v=g->verticies;
  while(v){
    fprintf(f,"vertex %d %d %d %d %d\n",
	    v->orig_x,v->orig_y,v->x,v->y,v->selected);
    v=v->next;
  }

  while(e){
    fprintf(f,"edge %d %d\n",iflat[e->A->num],iflat[e->B->num]); 
    e=e->next;
  }

  return 0;
}

int graph_read(graph *g,FILE *f){
  char *line=NULL,c;
  int i,x,y,ox,oy,sel,A,B;
  unsigned int n=0;
  vertex **flat,*v;
  long l;
  
  new_board(g,0);

  // get all verticies / scoring first
  while(getline(&line,&n,f)>0){
    
    if(sscanf(line,"vertex %d %d %d %d %d",
	      &ox,&oy,&x,&y,&sel)==5){
      
      v=get_vertex(g);
      v->orig_x=ox;
      v->orig_y=oy;
      v->x=x;
      v->y=y;
      v->selected=sel;
    }

    if(sscanf(line,"scoring %ld %f %f %ld %c %d\n",
	      &g->original_intersections,
	      &g->intersection_mult, &g->objective_mult, &l,
	      &c,&g->objective)==6){

      pause_timer();
      set_timer(l);
      if(c == '*')
	g->objective_lessthan = 1;
      else
	g->objective_lessthan = 0;
    }
 
    sscanf(line,"board %d %d %d %d",&g->width,&g->height,&g->orig_width,&g->orig_height);	

 }
    
  rewind(f);
  flat=alloca(g->vertex_num*sizeof(*flat));
  v=g->verticies;
  i=0;
  while(v){
    flat[v->num]=v;
    v=v->next;
  }
  
  // get edges next
  while(getline(&line,&n,f)>0){
    
    if(sscanf(line,"edge %d %d",&A,&B)==2){
      if(A>=0 && A<g->vertex_num && B>=0 && B<g->vertex_num)
	add_edge(g,flat[A],flat[B]);
      else
	fprintf(stderr,"WARNING: edge references out of range vertex in save file\n");
    }
    sscanf(line,"int %ld",&g->original_intersections);
  }
  
  rewind(f);
  free(line);

  return 0;
}

void graph_resize(graph *g, int width, int height){
  vertex *v=g->verticies;
  edge *e=g->edges;
  int xd=(width-g->width)*.5;
  int yd=(height-g->height)*.5;
  
  // recenter all the verticies; doesn't require recomputation
  while(v){
    v->x+=xd;
    v->y+=yd;
    v=v->next;
  }
  
  // recenter associated intersections as well; they all have
  // cached location (used only for drawing)
  while(e){
    intersection *i = e->i.next;
    while(i){
      if(i->paired > i){
	i->x+=xd;
	i->y+=yd;    
      }
      i=i->next;
    }
    e=e->next;
  }

  g->width=width;
  g->height=height;
  
  // verify all verticies are onscreen
  check_verticies(g);
}
