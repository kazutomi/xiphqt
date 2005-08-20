#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"
#include "gamestate.h"
#include "button_base.h"
#include "buttonbar.h"
#include "finish.h"
#include "box.h"

static int ui_next=0;
static gint timer;
static void (*callback)(Gameboard *);

/* perform a single frame of animation for all pause dialog buttons/rollovers */
static gboolean finish_animate_buttons(gpointer ptr){
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

static void finish_post (Gameboard *g){
  // back to buttonbar activity!
  ui_next=0;
  pop_background(g);
  levelstate_next();
  levelstate_go();
  gamestate_go();
} 

static void finish_quit (Gameboard *g){
  gtk_main_quit();
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
    buttonstate *b=states+1; 
    b->target_x-=BUTTON_EXPOSE;
  }
  {
    buttonstate *b=states+10; 
    b->target_x+=BUTTON_EXPOSE;
  }
}

static void local_go (Gameboard *g){
  undeploy_buttons(g);
  callback = finish_post;
  timer = g_timeout_add(BUTTON_ANIM_INTERVAL, finish_animate_buttons, (gpointer)g);
}

static void local_quit (Gameboard *g){
  undeploy_buttons(g);
  callback = finish_quit;
  timer = g_timeout_add(BUTTON_ANIM_INTERVAL, finish_animate_buttons, (gpointer)g);
}

/* initialize the rather weird little animation engine */
static void setup_finish_buttons(Gameboard *g,int bw, int bh){
  int i;
  int w=get_board_width();
  int h=get_board_height();

  states[0].rollovertext="exit gPlanarity";
  states[1].rollovertext="level menu";
  states[10].rollovertext="next level!";

  states[0].callback = local_quit;
  states[1].callback = 0; // for now
  states[10].callback = local_go;

  for(i=0;i<NUMBUTTONS;i++)
    states[i].position=0;

  states[0].position = 2; //center;
  states[1].position = 2; //center;
  states[10].position = 2; //center;

  {
    buttonstate *b=states;
    b->target_x_active=
      b->x_active=
      b->target_x_active=
      b->target_x= 
      w/2 - bw/2 + FINISH_BUTTON_BORDER;
    b->x=b->target_x_inactive=b->x_inactive=b->target_x - BUTTON_EXPOSE;
    b->y = h/2 + bh/2 - FINISH_BUTTON_Y;
  }

  {
    buttonstate *b=states+1;
    b->target_x_active=
      b->x_active=
      b->target_x_active=
      b->target_x= w/2;
    b->x=b->target_x_inactive=b->x_inactive=b->target_x - BUTTON_EXPOSE;
    b->y = h/2 + bh/2 - FINISH_BUTTON_Y;
  }

  {
    buttonstate *b=states+10;
    b->target_x_active=
      b->x_active=
      b->target_x_active=
      b->target_x= 
      w/2 + bw/2 - FINISH_BUTTON_BORDER;
    b->x=b->target_x_inactive=b->x_inactive=b->target_x + BUTTON_EXPOSE;
    b->y = h/2 + bh/2 - FINISH_BUTTON_Y;
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

static void draw_finishbox(Gameboard *g){
  int w= get_board_width();
  int h= get_board_height();

  cairo_t *c = cairo_create(g->background);
  borderbox_path(c,
		 w/2 - FINISHBOX_WIDTH/2,
		 h/2 - FINISHBOX_HEIGHT/2,
		 FINISHBOX_WIDTH,
		 FINISHBOX_HEIGHT);
  cairo_set_source_rgb(c,1,1,1);
  cairo_fill(c);

  centerbox(c,
	    w/2 - FINISHBOX_WIDTH/2,
	    h/2 - FINISHBOX_HEIGHT/2,
	    FINISHBOX_WIDTH,
	    SCOREHEIGHT);

  centerbox(c,
	    w/2 - FINISHBOX_WIDTH/2 ,
	    h/2 + FINISHBOX_HEIGHT/2 - SCOREHEIGHT,
	    FINISHBOX_WIDTH,
	    SCOREHEIGHT);

  {
    cairo_matrix_t ma;
    char time[160];
    char buffer[160];
    int  ho = get_elapsed() / 3600;
    int  mi = get_elapsed() / 60 - ho*60;
    int  se = get_elapsed() - ho*3600 - mi*60;
    int y;
    int time_bonus=get_initial_intersections()-get_elapsed();
    if(time_bonus<0)time_bonus=0;

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

    y=h/2-FINISHBOX_HEIGHT/2+SCOREHEIGHT/2;
    render_text_centered(c,"Level Complete!", w/2,y);y+=45;

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_matrix_init_scale (&ma, 16.,16.);
    cairo_set_font_matrix (c,&ma);

    snprintf(buffer,160,"Elapsed: %s",time);
    render_text_centered(c,buffer, w/2,y);y+=35;


    snprintf(buffer,160,"Score: %d",get_initial_intersections());
    render_text_centered(c,buffer, w/2,y);y+=24;
    snprintf(buffer,160,"Bonus: %d",time_bonus);
    render_text_centered(c,buffer, w/2,y);y+=45;

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    snprintf(buffer,160,"Total score: %d",get_raw_score());
    render_text_centered(c,buffer, w/2,y);

  }

  cairo_destroy(c);
}

static void finish_post_undeploy(Gameboard *g){
  // set up new buttons
  setup_finish_buttons(g,FINISHBOX_WIDTH, FINISHBOX_HEIGHT);

  // draw pausebox
  push_curtain(g,draw_finishbox);

  // deploy new buttons
  callback=0;
  timer = g_timeout_add(BUTTON_ANIM_INTERVAL, finish_animate_buttons, (gpointer)g);
  buttons_ready=1;
}

void finish_level_dialog(Gameboard *g){
  // undeploy buttonbar
  ui_next=1;
  push_background(g,0);
  undeploy_buttonbar(g,finish_post_undeploy);
}

int finish_dialog_active(){
  return ui_next;
}
