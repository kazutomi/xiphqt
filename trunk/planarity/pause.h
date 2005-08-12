#define PAUSE_BUTTON_BORDER 35
#define PAUSE_BUTTON_Y 25
#define PAUSEBOX_WIDTH 180
#define PAUSEBOX_HEIGHT 250

#define ABOUTBOX_WIDTH 320
#define ABOUTBOX_HEIGHT 400

extern void pause_game(Gameboard *g);
extern void about_game(Gameboard *g);

extern int pause_dialog_active();
extern int about_dialog_active();
