/* graph abstraction */

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

extern void resize_board(int x, int y);
extern vertex *new_board(int num_v);
extern vertex *find_vertex(int x, int y);
extern void move_vertex(vertex *v, int x, int y);
extern void grab_vertex(vertex *v);
extern void ungrab_vertex(vertex *v);
extern void activate_vertex(vertex *v);
extern void deactivate_vertex(vertex *v);
extern void select_verticies(int x1, int y1, int x2, int y2);
extern void deselect_verticies();
extern void move_selected_verticies(int dx, int dy);
extern void scale_verticies(float amount);
extern void randomize_verticies();
extern edge *add_edge(vertex *A, vertex *B);
extern int exists_edge(vertex *a, vertex *b);
extern int get_board_width();
extern int get_board_height();
extern vertex *get_verticies();
extern edge *get_edges();
extern int num_selected_verticies();
extern int get_num_intersections();
extern int get_max_intersections();
extern void check_verticies();
extern void impress_location();
extern void commit_volatile_selection();
extern vertex *get_vertex();
