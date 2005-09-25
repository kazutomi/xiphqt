#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

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
#define INTERSECTION_RADIUS 10
#define INTERSECTION_LINE_WIDTH 2

#define RESET_DELTA 2

#define B_LINE 1
#define B_BORDER 6.5
#define B_RADIUS 20
#define B_HUMP 130
#define B_COLOR         .1,.1,.7,.1
#define B_LINE_COLOR     0, 0,.7,.3
#define TEXT_COLOR      .0,.0,.7,.6
#define HIGH_COLOR      .7,.0,.0,.6

#define SCOREHEIGHT 50

#define ICON_WIDTH  160
#define ICON_HEIGHT 120

G_BEGIN_DECLS

#define GAMEBOARD_TYPE            (gameboard_get_type ())
#define GAMEBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMEBOARD_TYPE, Gameboard))
#define GAMEBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMEBOARD_TYPE, GameboardClass))
#define IS_GAMEBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMEBOARD_TYPE))
#define IS_GAMEBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMEBOARD_TYPE))

typedef struct _Gameboard       Gameboard;
typedef struct _GameboardClass  GameboardClass;

#define NUMBUTTONS 11

typedef struct {
  int position; // 0 inactive
                // 1 left
                // 2 center
                // 3 right
  int x_target;
  int x;

  int y_target;
  int y_inactive;
  int y_active;
  int y;
  int sweepdeploy;

  int alphad;
  double alpha;

  cairo_surface_t *idle;
  cairo_surface_t *lit;

  char *rollovertext;
  cairo_text_extents_t ex;

  int rollover;
  int press;

  void (*callback)();
} buttonstate;

typedef struct {
  buttonstate states[NUMBUTTONS];
  int buttons_ready;
  int allclear; // short-circuit hint
  int sweeper;
  int sweeperd;
  buttonstate *grabbed;
} buttongroup;

typedef struct {
  int num;
  double alpha;
  cairo_surface_t *icon;

  int x;
  int y;
  int w;
  int h;

} dialog_level_oneicon;

typedef struct {

  dialog_level_oneicon level_icons[5];
  int center_x;
  int level_lit;
  int reset_deployed;
  
  GdkRectangle text1;
  GdkRectangle text2;
  GdkRectangle text3;
  GdkRectangle text4;

} dialog_level_state;

struct _Gameboard{
  GtkWidget w;

  graph g;

  int pushed_curtain;
  void (*redraw_callback)(Gameboard *g);

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
  cairo_pattern_t *curtainp;
  cairo_surface_t *curtains;

  int delayed_background;
  int first_expose;
  int hide_lines;
  int show_intersections;
  int finish_dialog_active;
  int about_dialog_active;
  int pause_dialog_active;
  int level_dialog_active;

  buttongroup b;
  dialog_level_state d;

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

  int checkbutton_deployed;
  int buttonbar_sweeper;
  
  gint gtk_timer;
  void (*button_callback)(Gameboard *);

};

struct _GameboardClass{
  GtkWidgetClass parent_class;
  void (* gameboard) (Gameboard *m);
};

GType          gameboard_get_type        (void);
Gameboard*     gameboard_new             (void);

G_END_DECLS

extern void init_buttons(Gameboard *g);
extern buttonstate *find_button(Gameboard *g,int x,int y);
extern void button_set_state(Gameboard *g, buttonstate *b, int rollover, int press);
extern void rollover_extents(Gameboard *g, buttonstate *b);
extern gboolean animate_button_frame(gpointer ptr);
extern void expose_buttons(Gameboard *g,cairo_t *c, int x,int y,int w,int h);
extern void resize_buttons(Gameboard *g,int oldw,int oldh,int w,int h);
extern void button_clear_state(Gameboard *g);

extern void update_push(Gameboard *g, cairo_t *c);
extern void push_curtain(Gameboard *g,void(*redraw_callback)(Gameboard *g));
extern void pop_curtain(Gameboard *g);

extern void prepare_reenter_game(Gameboard *g);
extern void reenter_game(Gameboard *g);
extern void enter_game(Gameboard *g);
extern void quit_action(Gameboard *g);
extern void finish_action(Gameboard *g);
extern void expand_action(Gameboard *g);
extern void shrink_action(Gameboard *g);
extern void pause_action(Gameboard *g);
extern void about_action(Gameboard *g);
extern void level_action(Gameboard *g);
extern void set_hide_lines(Gameboard *g, int state);
extern void toggle_hide_lines(Gameboard *g);
extern void set_show_intersections(Gameboard *g, int state);
extern void toggle_show_intersections(Gameboard *g);
extern void reset_action(Gameboard *g);
extern int gameboard_write(char *basename, Gameboard *g);
extern int gameboard_read(char *basename, Gameboard *g);

extern void topbox (Gameboard *g,cairo_t *c, double w, double h);
extern void bottombox (Gameboard *g,cairo_t *c, double w, double h);
extern void centerbox (cairo_t *c, int x, int y, double w, double h);
extern void borderbox_path (cairo_t *c, double x, double y, double w, double h);

extern void setup_buttonbar(Gameboard *g);
extern void deploy_buttonbar(Gameboard *g);
extern void deploy_check(Gameboard *g);
extern void undeploy_check(Gameboard *g);

extern void update_draw(Gameboard *g);
extern void update_full(Gameboard *g);
extern void expose_full(Gameboard *g);
extern void update_full_delayed(Gameboard *g);
extern void update_add_vertex(Gameboard *g, vertex *v);
extern void gameboard_draw(Gameboard *g, int x, int y, int w, int h);
extern void draw_foreground(Gameboard *g,cairo_t *c,
			    int x,int y,int width,int height);
extern void draw_intersections(Gameboard *b,graph *g,cairo_t *c,
			       int x,int y,int w,int h);

extern void draw_score(Gameboard *g);
extern void update_score(Gameboard *g);

extern void draw_vertex(cairo_t *c,vertex *v,cairo_surface_t *s);
extern cairo_surface_t *cache_vertex(Gameboard *g);
extern cairo_surface_t *cache_vertex_sel(Gameboard *g);
extern cairo_surface_t *cache_vertex_grabbed(Gameboard *g);
extern cairo_surface_t *cache_vertex_lit(Gameboard *g);
extern cairo_surface_t *cache_vertex_attached(Gameboard *g);
extern cairo_surface_t *cache_vertex_ghost(Gameboard *g);
extern void invalidate_vertex_off(GtkWidget *widget, vertex *v, int dx, int dy);
extern void invalidate_vertex(Gameboard *g, vertex *v);
extern void invalidate_attached(GtkWidget *widget, vertex *v);
extern void invalidate_edges(GtkWidget *widget, vertex *v);
extern void draw_selection_rectangle(Gameboard *g,cairo_t *c);
extern void invalidate_selection(GtkWidget *widget);
extern void invalidate_verticies_selection(GtkWidget *widget);

extern void cache_curtain(Gameboard *g);
extern void draw_curtain(Gameboard *g);
extern void draw_buttonbar_box (Gameboard *g);

extern gint mouse_motion(GtkWidget *widget,GdkEventMotion *event);
extern gboolean mouse_press (GtkWidget *widget,GdkEventButton *event);
extern gboolean mouse_release (GtkWidget *widget,GdkEventButton *event);

extern void setup_background_edge(cairo_t *c);
extern void setup_foreground_edge(cairo_t *c);
extern void draw_edge(cairo_t *c,edge *e);
extern void finish_edge(cairo_t *c);

extern cairo_surface_t *gameboard_read_icon(char *filename, char *ext,Gameboard *b);
extern int gameboard_write_icon(char *filename, char *ext,Gameboard *b, graph *g,
				int lines, int intersections);
extern int gameboard_icon_exists(char *filename, char *ext);

extern void deploy_buttons(Gameboard *g, void (*callback)(Gameboard *));
extern void undeploy_buttons(Gameboard *g, void (*callback)(Gameboard *));

extern GdkRectangle render_text_centered(cairo_t *c, char *s, int x, int y);
extern GdkRectangle render_border_centered(cairo_t *c, char *s, int x, int y);
extern GdkRectangle render_bordertext_centered(cairo_t *c, char *s, int x, int y);

extern void gameboard_size_allocate (GtkWidget     *widget,
				     GtkAllocation *allocation);
