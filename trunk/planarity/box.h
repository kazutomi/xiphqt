#define B_LINE 1
#define B_BORDER 6.5
#define B_RADIUS 20
#define B_HUMP 130
#define B_COLOR         .1,.1,.7,.1
#define B_LINE_COLOR     0, 0,.7,.3
#define TEXT_COLOR      .0,.0,.7,.6


#define SCOREHEIGHT 50


extern void topbox (cairo_t *c, double w, double h);
extern void bottombox (cairo_t *c, double w, double h);
extern void centerbox (cairo_t *c, int x, int y, double w, double h);
extern void borderbox_path (cairo_t *c, int x, int y, double w, double h);
