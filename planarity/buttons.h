extern void path_button_help(cairo_t *c, double x, double y);
extern void path_button_back(cairo_t *c, double x, double y);
extern void path_button_reset(cairo_t *c, double x, double y);
extern void path_button_pause(cairo_t *c, double x, double y);
extern void path_button_exit(cairo_t *c, double x, double y);
extern void path_button_expand(cairo_t *c, double x, double y);
extern void path_button_shrink(cairo_t *c, double x, double y);
extern void path_button_lines(cairo_t *c, double x, double y);
extern void path_button_int(cairo_t *c, double x, double y);
extern void path_button_check(cairo_t *c, double x, double y);
extern void path_button_play(cairo_t *c, double x, double y);
 

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

#define BUTTON_RADIUS 14
#define BUTTON_Y_FROM_BOTTOM 25
#define BUTTON_LINE_WIDTH 1
#define BUTTON_TEXT_BORDER 15
#define BUTTON_TEXT_COLOR       .1,.1,.7,.8
#define BUTTON_TEXT_SIZE  15.,18.

#define BUTTON_ANIM_INTERVAL 15
#define BUTTON_LEFT 5
#define BUTTON_RIGHT 5
#define BUTTON_BORDER 35
#define BUTTON_SPACING 35
#define BUTTON_EXPOSE  50
#define BUTTON_DELTA 3
