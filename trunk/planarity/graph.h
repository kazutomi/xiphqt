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

#define V_RADIUS_SQ (V_RADIUS*V_RADIUS)
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

typedef struct vertex {
  int num;
  int x;
  int y;
  int orig_x;
  int orig_y;

  int active;
  int selected_volatile;
  int selected;
  int grabbed;
  int attached_to_grabbed;
  int fading;
  struct edge_list *edges;
  struct vertex *next;
} vertex;

typedef struct intersection {
  struct intersection *prev;
  struct intersection *next;
  struct intersection *paired;
  double x;
  double y;
} intersection;

typedef struct edge{
  vertex *A;
  vertex *B;

  int active;

  intersection i; // correct, not a pointer
  struct edge *next;
} edge;

typedef struct edge_list{
  edge *edge;
  struct edge_list *next;
} edge_list;

typedef struct graph {
  vertex *verticies;
  int     vertex_num;
  edge *edges;
  long active_intersections;

  int num_edges;
  int num_edges_active;

  int width;
  int height;
  int orig_width;
  int orig_height;

  // scoring related metadata
  long  original_intersections;
  float intersection_mult;
  int   objective;
  int   objective_lessthan;
  float objective_mult;

} graph;

typedef struct graphmeta{
  int num;
  char *id;
  char *desc;
  void (*gen)(graph *,int arg);
  int gen_arg;
  int unlock_plus;
} graphmeta;

#include <stdio.h>

extern vertex *new_board(graph *g, int num_v);
extern vertex *find_vertex(graph *g, int x, int y);

extern edge_list *add_edge_to_list(edge_list *l, edge *e);
extern void release_edge_list(edge_list *el);
extern edge *new_edge(vertex *A, vertex *B);
extern void release_edge_list(edge_list *el);
extern void insert_edge(graph *g, edge *e);
extern int intersectsV(vertex *L1, vertex *L2, vertex *M1, vertex *M2, 
		       double *xo, double *yo);
extern int intersects(int L1x, int L1y, int L2x, int L2y,
		      int M1x, int M1y, int M2x, int M2y,
		      double *xo, double *yo);
extern void move_vertex(graph *g, vertex *v, int x, int y);
extern void grab_vertex(graph *g, vertex *v);
extern void grab_selected(graph *g);
extern void ungrab_vertex(graph *g,vertex *v);
extern void ungrab_verticies(graph *g);
extern void activate_vertex(graph *g, vertex *v);
extern void deactivate_vertex(graph *g, vertex *v);
extern void select_verticies(graph *g, int x1, int y1, int x2, int y2);
extern void deselect_verticies(graph *g);
extern void move_selected_verticies(graph *g, int dx, int dy);
extern void scale_verticies(graph *g, float amount);
extern void randomize_verticies(graph *g);
extern edge *add_edge(graph *g,vertex *A, vertex *B);
extern int exists_edge(vertex *a, vertex *b);
extern vertex *get_verticies();
extern edge *get_edges();
extern int num_selected_verticies(graph *g);
extern int get_num_intersections();
extern int get_max_intersections();
extern void check_verticies();
extern void impress_location();
extern void commit_volatile_selection();
extern vertex *get_vertex();
extern void activate_verticies();
extern int graph_write(graph *g, FILE *f);
extern int graph_read(graph *g, FILE *f);
extern void graph_release(graph *g);

extern int   graphscore_get_score(graph *g);
extern int   graphscore_get_raw_score(graph *g);
extern int   graphscore_get_multiplier_percent(graph *g);
extern int   graphscore_get_bonus(graph *g);
extern char *graphscore_objective_string(graph *g);
