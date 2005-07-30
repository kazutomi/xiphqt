#define NUMBUTTONS 11

typedef struct {
  int position; // 0 inactive
                // 1 left
                // 2 center
                // 3 right

  int target_x;
  int target_x_inactive;
  int target_x_active;

  int x;
  int x_inactive;
  int x_active;
  int y;

  cairo_surface_t *idle;
  cairo_surface_t *lit;

  char *rollovertext;
  cairo_text_extents_t ex;

  int alphad;
  double alpha;

  int rollover;
  int press;

  void (*callback)();
} buttonstate;

buttonstate states[NUMBUTTONS];


extern void init_buttons(Gameboard *g);
extern void rollover_extents(Gameboard *g, buttonstate *b);
extern gboolean animate_button_frame(gpointer ptr);
extern void expose_buttons(Gameboard *g,cairo_t *c,int x, int y, int w, int h);
extern void resize_buttons(int w,int h);
extern int button_motion_event(Gameboard *g, GdkEventMotion *event, int focusable);
extern int button_button_press(Gameboard *g, GdkEventButton *event, int focusable);
extern int button_button_release(Gameboard *g, GdkEventButton *event, int focusable);
extern void button_clear_state(Gameboard *g);
extern int buttons_ready;

#define BUTTON_QUIT_IDLE_FILL   .7,.1,.1,.3
#define BUTTON_QUIT_IDLE_PATH   .7,.1,.1,.6

#define BUTTON_QUIT_LIT_FILL    .7,.1,.1,.5
#define BUTTON_QUIT_LIT_PATH    .7,.1,.1,.6

#define BUTTON_IDLE_FILL        .1,.1,.7,.3
#define BUTTON_IDLE_PATH        .1,.1,.7,.6

#define BUTTON_LIT_FILL         .1,.1,.7,.6
#define BUTTON_LIT_PATH         .1,.1,.7,.6

#define BUTTON_CHECK_IDLE_FILL  .1,.5,.1,.3
#define BUTTON_CHECK_IDLE_PATH  .1,.5,.1,.6

#define BUTTON_CHECK_LIT_FILL   .1,.5,.1,.6
#define BUTTON_CHECK_LIT_PATH   .1,.5,.1,.6

#define BUTTON_RADIUS                    14
#define BUTTON_LINE_WIDTH                 1
#define BUTTON_TEXT_BORDER               15
#define BUTTON_TEXT_COLOR       .1,.1,.7,.8
#define BUTTON_TEXT_SIZE            15.,18.

#define BUTTON_ANIM_INTERVAL             15
#define BUTTON_EXPOSE                    50
#define BUTTON_DELTA                      3
