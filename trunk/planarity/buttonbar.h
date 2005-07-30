extern void setup_buttonbar(Gameboard *g);
extern void deploy_buttonbar(Gameboard *g);
extern void undeploy_buttonbar(Gameboard *g, void (*callback)());
extern void deploy_check(Gameboard *g);
extern void undeploy_check(Gameboard *g);

#define BUTTONBAR_Y_FROM_BOTTOM 25
#define BUTTONBAR_LINE_WIDTH 1
#define BUTTONBAR_TEXT_BORDER 15
#define BUTTONBAR_TEXT_COLOR       .1,.1,.7,.8
#define BUTTONBAR_TEXT_SIZE  15.,18.

#define BUTTONBAR_ANIM_INTERVAL 15
#define BUTTONBAR_EXPOSE  50
#define BUTTONBAR_DELTA 3

#define BUTTONBAR_LEFT 5
#define BUTTONBAR_RIGHT 5
#define BUTTONBAR_BUTTONS 5
#define BUTTONBAR_BORDER 35
#define BUTTONBAR_SPACING 35
