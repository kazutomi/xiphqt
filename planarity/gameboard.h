#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkdrawingarea.h>


#define V_RADIUS 8
#define V_LINE 2
#define V_LINE_COLOR         0, 0, 0
#define V_FILL_IDLE_COLOR   .2,.2, 1
#define V_FILL_LIT_COLOR     1, 1, 1
#define V_FILL_ADJ_COLOR     1,.2,.2

#define E_LINE 1.5
#define E_LINE_F_COLOR       0, 0, 0, 1
#define E_LINE_B_COLOR      .5,.5,.5, 1
#define SELECTBOX_COLOR     .2,.8,.8,.3

#define INTERSECTION_COLOR  1,.1,.1,.8
#define INTERSECTION_RADIUS 6
#define INTERSECTION_LINE_WIDTH 2

G_BEGIN_DECLS

#define GAMEBOARD_TYPE            (gameboard_get_type ())
#define GAMEBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMEBOARD_TYPE, Gameboard))
#define GAMEBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMEBOARD_TYPE, GameboardClass))
#define IS_GAMEBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMEBOARD_TYPE))
#define IS_GAMEBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMEBOARD_TYPE))

typedef struct _Gameboard       Gameboard;
typedef struct _GameboardClass  GameboardClass;

struct _Gameboard{
  GtkWidget w;

  cairo_t         *wc;
  cairo_surface_t *vertex;
  cairo_surface_t *vertex_lit;
  cairo_surface_t *vertex_grabbed;
  cairo_surface_t *vertex_attached;
  cairo_surface_t *vertex_sel;
  cairo_surface_t *vertex_ghost;
  cairo_surface_t *forescore;
  cairo_surface_t *forebutton;
  cairo_surface_t *background;
  cairo_surface_t *foreground;
  int delayed_background;

  vertex *grabbed_vertex;
  vertex *lit_vertex;
  int group_drag;
  int button_grabbed;
  
  int grabx;
  int graby;
  int dragx;
  int dragy;
  int graboffx;
  int graboffy;
  
  int selection_grab;
  int selection_active;
  int selectionx;
  int selectiony;
  int selectionw;
  int selectionh;

  int hide_lines;

  int first_expose;
  int show_intersections;
};

struct _GameboardClass{
  GtkWidgetClass parent_class;
  void (* gameboard) (Gameboard *m);
};

GType          gameboard_get_type        (void);
Gameboard*     gameboard_new             (void);

G_END_DECLS

extern void main_board(int argc, char *argv[]);
extern void run_immediate_expose(Gameboard *g,int x, int y, int w, int h);
extern void gameboard_reset(Gameboard *g);
extern void hide_lines(Gameboard *g);
extern void show_lines(Gameboard *g);
extern int get_hide_lines(Gameboard *g);
extern void show_intersections(Gameboard *g);
extern void hide_intersections(Gameboard *g);
extern void invalidate_region_vertex(Gameboard *g, vertex *v);
extern int selected(Gameboard *g);
extern void update_full(Gameboard *g);
