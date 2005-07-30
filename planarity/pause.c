#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"
#include "gamestate.h"
#include "button_base.h"
#include "buttonbar.h"
#include "pause.h"
#include "box.h"

static gint timer;
static void (*callback)(Gameboard *);

/* perform a single frame of animation for all pause dialog buttons/rollovers */
static gboolean pause_animate_buttons(gpointer ptr){
  Gameboard *g=(Gameboard *)ptr;
  int ret=0;

  ret=animate_button_frame(g);

  if(!ret && timer!=0){
    g_source_remove(timer);
    timer=0;
  }

  if(!ret && callback)
    // undeploy finished... call the undeploy callback
    callback(g);

  return ret;
}

static void unpause_post (Gameboard *g){
  // back to buttonbar activity!
  pop_background(g);
  deploy_buttonbar(g);
  unpause();
} 

static void unpause_quit (Gameboard *g){
  quit();
} 

static void undeploy_buttons(Gameboard *g){
  // undeploy pause buttons
  button_clear_state(g);
  buttons_ready=0;

  {
    buttonstate *b=states; 
    b->target_x-=BUTTON_EXPOSE;
  }
  {
    buttonstate *b=states+10; 
    b->target_x+=BUTTON_EXPOSE;
  }
}

static void local_unpause (Gameboard *g){
  undeploy_buttons(g);
  callback = unpause_post;
  timer = g_timeout_add(BUTTON_ANIM_INTERVAL, pause_animate_buttons, (gpointer)g);
}

static void local_quit (Gameboard *g){
  undeploy_buttons(g);
  callback = unpause_quit;
  timer = g_timeout_add(BUTTON_ANIM_INTERVAL, pause_animate_buttons, (gpointer)g);
}

/* initialize the rather weird little animation engine */
static void setup_pause_buttons(Gameboard *g){
  int i;
  int w=get_board_width();
  int h=get_board_height();

  states[0].rollovertext="exit gPlanarity";
  states[10].rollovertext="resume game!";

  states[0].callback = local_quit;
  states[10].callback = local_unpause;

  for(i=0;i<NUMBUTTONS;i++)
    states[i].position=0;

  states[0].position = 2; //center;
  states[10].position = 2; //center;

  {
    buttonstate *b=states;
    b->target_x_active=
      b->x_active=
      b->target_x_active=
      b->target_x= 
      w/2 - PAUSEBOX_WIDTH/2 + PAUSE_BUTTON_BORDER;
    b->x=b->target_x_inactive=b->x_inactive=b->target_x - BUTTON_EXPOSE;
    b->y = h/2 + PAUSEBOX_HEIGHT/2 - PAUSE_BUTTON_Y;
  }

  {
    buttonstate *b=states+10;
    b->target_x_active=
      b->x_active=
      b->target_x_active=
      b->target_x= 
      w/2 + PAUSEBOX_WIDTH/2 - PAUSE_BUTTON_BORDER;
    b->x=b->target_x_inactive=b->x_inactive=b->target_x + BUTTON_EXPOSE;
    b->y = h/2 + PAUSEBOX_HEIGHT/2 - PAUSE_BUTTON_Y;
  }

  for(i=0;i<NUMBUTTONS;i++)
    if(states[i].position)
      rollover_extents(g,states+i);  
}

static void render_text_centered(cairo_t *c, char *s, int x, int y){
  cairo_text_extents_t ex;

  cairo_text_extents (c, s, &ex);
  cairo_move_to (c, x-(ex.width/2)-ex.x_bearing, y-(ex.height/2)-ex.y_bearing);
  cairo_show_text (c, s);  
}

static void draw_pausebox(Gameboard *g){
  int w= get_board_width();
  int h= get_board_height();

  push_background(g);
  
  cairo_t *c = cairo_create(g->background);
  borderbox_path(c,
		 w/2 - PAUSEBOX_WIDTH/2,
		 h/2 - PAUSEBOX_HEIGHT/2,
		 PAUSEBOX_WIDTH,
		 PAUSEBOX_HEIGHT);
  cairo_set_source_rgb(c,1,1,1);
  cairo_fill(c);

  centerbox(c,
	    w/2 - PAUSEBOX_WIDTH/2,
	    h/2 - PAUSEBOX_HEIGHT/2,
	    PAUSEBOX_WIDTH,
	    SCOREHEIGHT);

  centerbox(c,
	    w/2 - PAUSEBOX_WIDTH/2 ,
	    h/2 + PAUSEBOX_HEIGHT/2 - SCOREHEIGHT,
	    PAUSEBOX_WIDTH,
	    SCOREHEIGHT);

  {
    cairo_matrix_t ma;
    char time[160];
    int  ho = get_elapsed() / 3600;
    int  mi = get_elapsed() / 60 - ho*60;
    int  se = get_elapsed() - ho*3600 - mi*60;
    
    if(ho){
      snprintf(time,160,"%d:%02d:%02d",ho,mi,se);
    }else if (mi){
      snprintf(time,160,"%d:%02d",mi,se);
    }else{
      snprintf(time,160,"%d seconds",se);
    }

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);

    cairo_matrix_init_scale (&ma, 18.,18.);
    cairo_set_font_matrix (c,&ma);
    cairo_set_source_rgba (c, TEXT_COLOR);

    render_text_centered(c,"Game Paused", w/2,h/2-PAUSEBOX_HEIGHT/2+SCOREHEIGHT/2);
    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    render_text_centered(c,"Time Elapsed:", w/2,h/2-30);
    render_text_centered(c,time, w/2,h/2);
  }

  cairo_destroy(c);
}

static void pause_game_post_undeploy(Gameboard *g){
  // set up new buttons
  setup_pause_buttons(g);

  // draw pausebox
  draw_pausebox(g);

  // deploy new buttons
  callback=0;
  timer = g_timeout_add(BUTTON_ANIM_INTERVAL, pause_animate_buttons, (gpointer)g);
  buttons_ready=1;
}

void pause_game(Gameboard *g){
  // grab timer state
  pause();

  // undeploy buttonbar
  undeploy_buttonbar(g,pause_game_post_undeploy);
}
