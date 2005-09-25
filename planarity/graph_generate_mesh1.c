#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "gameboard.h"
#include "graph_generate.h"
#include "graph_arrange.h"

typedef struct {
  vertex **v;
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
}

// eliminate verticies we've already connected to
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

static void random_populate(graph *g, int current, mesh *m, int min_connect, float prob){
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
    int choice = random() % nl.num;
    add_edge(g,m->v[current], m->v[nl.vnum[choice]]);
    num_edges++;
    filter_intersections(&ng);
    filter_edges(&ng,&nl);
  }
  
  for(i=0;i<nl.num;i++)
    if(random()<RAND_MAX*prob){
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
      int choice = random() % nl.num;
      add_edge(g,m->v[current], m->v[nl.vnum[choice]]);
      
      span_depth_first(g,nl.vnum[choice], m);
    }
  }
}

void generate_mesh_1(graph *g, int order){
  int flag=0;
  mesh m;
  m.width=3;
  m.height=2;
  vertex *vlist;

  srandom(order);
  {
    while(--order){
      if(flag){
	flag=0;
	m.height+=1;
      }else{
	flag=1;
	m.width+=2;
      }
    }
  }

  vlist=new_board(g, m.width * m.height);

  /* a flat vector is easier to address while building the mesh */
  {
    int i;
    vertex *v=vlist;
    m.v=alloca(m.width*m.height * sizeof(*m.v));
    for(i=0;i<m.width*m.height;i++){
      m.v[i]=v;
      v=v->next;
    }
  }

  /* first walk a random spanning tree */
  span_depth_first(g, 0, &m);
  
  /* now iterate the whole mesh adding random edges */
  {
    int i;
    for(i=0;i<m.width*m.height;i++)
      random_populate(g, i, &m, 2, .25);
  }

  randomize_verticies(g);
  arrange_verticies_circle(g);

  //arrange_verticies_mesh(m.width,m.height);
}

