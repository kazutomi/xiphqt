/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 1994-2007              *
 * the Xiph.Org FOundation http://www.xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: implement granule simplex solution space search algortihm
 last mod: $Id$

 ********************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

typedef struct local_minimum{
  int *coefficients;
  double cost;
} lmin_t;

lmin_t *minimum_list = NULL;
int minimum_list_size = 0;

int lookup_minimum(int *c){
  int i,j;
  
  for(i=0;i<minimum_list_size;i++){
    lmin_t *min = minimum_list+i;
    for(j=0;j<num_coefficients;j++)
      if(min->coefficients[j] != c[j])break;
    if(j==num_coefficients) return i;
  }
  return -1;
}

int log_minimum(int *c, double cost){
  int j;

  int *mc = malloc(num_coefficients * sizeof(*mc));

  if(!minimum_list){
    minimum_list = malloc(sizeof(*minimum_list));
  }else{
    minimum_list = realloc(minimum_list, (minimum_list_size+1)*sizeof(*minimum_list));
  }
   
  minimum_list[minimum_list_size].coefficients = mc;
  for(j=0;j<num_coefficients;j++) mc[j] = c[j];
  minimum_list_size++;

  return minimum_list_size-1;
}

double gradient_walk(int *c, int m,
		     int min_bound, int max_bound,
		     double (*cost_func)(int *)){
  
  int last_change = m-1;
  int cur_dim = 0;
  double cost = cost_func(c);

  while(1){
    double val = c[cur_dim];
    c[cur_dim] = val+1;
    double up_cost = lift_cost(c);
    c[cur_dim] = val-1;
    double down_cost = lift_cost(c);

    if(up_cost<cost && val+1 <= max_bound){

      c[cur_dim] = val+1;
      last_change = cur_dim;
      cost = up_cost;
      
    }else if (down_cost<cost && val-1 >= min_bound){

      c[cur_dim] = val-1;
      last_change = cur_dim;
      cost = down_cost;
      
    }else{

      c[cur_dim] = val;
      if(cur_dim == last_change)break;

    }

    cur_dim++;
    if(cur_dim >= m) cur_dim = 0;

  }

  return cost;
}

void grow_vertex(int *c, int m,
		 int *allowed_movements,
		 int min_bound, int max_bound,
		 double (*cost_func)(int *)){



}

void grow_new_granule(int *c, int m,
		      int min_bound, int max_bound,
		      double (*cost_func)(int *)){
  int i;
  // Start by growing a new simplex by allowing each vertex to move
  // outward from the center (the local minimum). A vertex continues
  // moving so long as it is moving uphill. For this new simplex, the
  // allowed vertex movements are arbitrary (but must be orthogonal).

  int *movemask = calloc(m,sizeof(*movemask));
  int **new_c = malloc((m+1) * sizeof(*new_c));
  int **secondary_c = malloc((m+1) * sizeof(*secondary_c));

  for(i=0;i<(m+1);i++){
    new_c[i] = malloc(m * sizeof(**new_c));
    memcpy(new_c[i],c,sizeof(*c)*m);

    // set up orthogonal move mask for this vertex
    memset(movemask,0,sizeof(*movemask)*m);
    if(i>0)movemask[i-1]=-1;
    if(i<m)movemask[i]=1;

    // grow vertex to maximum
    grow_vertex(new_c[i], m, movemask, min_bound, max_bound, cost_func);
  }

  // this primary simplex almost certainly does not fill its
  // convergence zone; recursively spawn secondary simplexes with one
  // unconstrained vertex from the center of each face of this
  // simplex.  The new vertex must grow outward (reflect the simplex
  // through each face).
  secondary_c[0] = malloc(m * sizeof(**new_c));

  for(i=0;i<(m+1);i++){
    // build a new d-1-tope 'plane'; all the points in the current
    // simplex minus the i'th one.
    
    // fill in the fixed verticies [1 through m]
    for(j=1;j<m;j++)
      if(j<=i)
	secondary_c[j] = new_c[j-1];
      else
	secondary_c[j] = new_c[j];

    // first vertex is the unconstrained one
    center_of_tope(secondary_c[0], secondary_c+1, m);

    // set movemask [reverse of the i'th vertex's movemask]
    memset(movemask,0,sizeof(*movemask)*m);
    if(i>0)movemask[i-1]=1;
    if(i<m)movemask[i]=-1;

    grow_secondary_granule(secondary_c, m, movemask, min_bound, max_bound, cost_func);
  }

  // Each vertex is sitting on a ridge.  Pop over the ridge and
  // walk gradient down to a minimum. If it's a new minimum, grow a
  // new granule.

  // this is here and not above to allow each granule to fill out
  // completely before beginning work on another.
  for(i=0;i<(m+1);i++){
    // re-set up orthogonal move mask for this vertex
    memset(movemask,0,sizeof(*movemask)*m);
    if(i>0)movemask[i-1]=-1;
    if(i<m)movemask[i]=1;
    crest_walk_and_recurse(new_c[i], m, movemask, min_bound, max_bound, cost_func);
    free(new_c[i]);
  }
  
  free(movemask);
  free(new_c);
}

double granule_simplex(int *c, int m,
		       int min_bound, int max_bound,
		       double (*cost_func)(int *)){

  // begin at a local minimum; walk passed in c
  double cost = gradient_walk(c, m, min_bound, max_bound, cost_func);
  log_minimum(c,cost);

  grow_new_granule(c); // enter recursion

  // report all minimums (debugging purposes), select global
  
}
