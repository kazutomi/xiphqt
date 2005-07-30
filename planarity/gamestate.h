#include <time.h>

extern void resize_board(int x, int y);
extern int get_board_width();
extern int get_board_height();
extern void setup_board();
extern void finish_board();
extern void hide_show_lines();
extern void mark_intersections();
extern void reset_board();
extern void expand();
extern void shrink();
extern int get_score();
extern int get_objective();
extern char *get_objective_string();
extern int get_level();
extern char *get_level_string();
extern void quit();

extern void pause();
extern void unpause();
extern int paused_p();
extern time_t get_elapsed();
extern void set_timer(time_t off);
extern int get_initial_intersections();
extern int get_raw_score();
