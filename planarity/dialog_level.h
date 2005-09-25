#define LEVEL_BUTTON_BORDER 35
#define LEVEL_BUTTON_Y 25
#define LEVELBOX_WIDTH 556
#define LEVELBOX_HEIGHT 370

#define ICON_DELTA 20

extern void level_dialog(Gameboard *g,int advance);
extern void render_level_icons(Gameboard *g, cairo_t *c, int ex,int ey, int ew, int eh);
extern void level_icons_init(Gameboard *g);

extern void level_mouse_motion(Gameboard *g, int x, int y);
extern void level_mouse_press(Gameboard *g, int x, int y);
extern void local_reset (Gameboard *g);
