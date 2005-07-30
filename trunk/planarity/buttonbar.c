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

/************************ the lower button bar *********************/

static int checkbutton_deployed=0;
static int sweeper=0;
static gint timer = 0;
static void (*undeploy_callback)(Gameboard *g);

/* perform a single frame of animation for all buttons/rollovers */
static gboolean buttonbar_animate_buttons(gpointer ptr){
  Gameboard *g=(Gameboard *)ptr;
  int ret=0;

  if(!g->first_expose)return 1;

  if(get_curtain(g)>0.){
    set_curtain(g,get_curtain(g)-.25);
    return 1;
  }
  set_curtain(g,0);

  ret=animate_button_frame(g);

  if(!ret && timer!=0){
    g_source_remove(timer);
    timer=0;
  }
  return ret;
}

/* helper that wraps animate-buttons for removing the buttons at the
   end of a level */
static gboolean buttonbar_deanimate_buttons(gpointer ptr){
  Gameboard *g=(Gameboard *)ptr;
  int ret=0,i;
  sweeper-=BUTTON_DELTA;
  if(sweeper< -(BUTTON_RADIUS +1)){
    sweeper= -(BUTTON_RADIUS +1);
  }else
    ret=1;

  if(get_curtain(g)==0.){

    int width=get_board_width();
    int w2=width/2;

    for(i=0;i<NUMBUTTONS;i++){
      buttonstate *b=states+i;
      if(b->position){
	if(b->target_x>w2 && width-sweeper>b->target_x)
	  b->target_x=width-sweeper;
	if(b->target_x<w2 && sweeper<b->target_x)
	  b->target_x=sweeper;
      }
    }
    ret|=animate_button_frame(ptr);
  }
  
  if(!ret){
    if(get_curtain(g)<.625){
      set_curtain(g,get_curtain(g)+.25);
      ret=1;
    }
  }

  if(!ret)
    // undeploy finished... call the undeploy callback
    undeploy_callback(g);

  return ret;
}

/******************** toplevel abstraction calls *********************/

/* initialize the rather weird little animation engine */
void setup_buttonbar(Gameboard *g){
  
  int count=BUTTONBAR_BORDER;
  int i;
  int w=get_board_width();
  int h=get_board_height();

  states[0].rollovertext="exit gPlanarity";
  states[1].rollovertext="back to menu";
  states[2].rollovertext="reset board";
  states[3].rollovertext="pause";
  states[4].rollovertext="help / about";
  states[5].rollovertext="expand";
  states[6].rollovertext="shrink";
  states[7].rollovertext="hide/show lines";
  states[8].rollovertext="mark intersections";
  states[9].rollovertext="click when finished!";

  states[0].callback = quit;
  states[2].callback = reset_board;
  states[3].callback = pause_game;
  states[4].callback = about_game;
  states[5].callback = expand;
  states[6].callback = shrink;
  states[7].callback = hide_show_lines;
  states[8].callback = mark_intersections;
  states[9].callback = finish_board;

  for(i=0;i<NUMBUTTONS;i++)
    states[i].position=0;

  states[0].position = 1; //left;
  states[1].position = 1; //left;
  states[2].position = 1; //left;
  states[3].position = 1; //left;
  states[4].position = 1; //left;
  states[5].position = 3; //right
  states[6].position = 3; //right
  states[7].position = 3; //right
  states[8].position = 3; //right
  states[9].position = 3; //right

  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *b=states+i;
    if(b->position == 1){
      b->x=0;
      b->target_x=count;
      b->x_active=b->target_x_active=count;
      b->x_inactive=b->target_x_inactive=count-BUTTON_EXPOSE;
      b->y = h - BUTTONBAR_Y_FROM_BOTTOM;
      count += BUTTONBAR_SPACING;
    }
  }
  count -= BUTTONBAR_SPACING;

  // assumes we will always have at least as many buttons left as right
  sweeper=count;
  
  count=w-BUTTONBAR_BORDER;
  for(i=NUMBUTTONS-1;i>=0;i--){
    buttonstate *b=states+i;
    if(b->position == 3){
      b->x=w;
      b->target_x=count;
      b->x_active=b->target_x_active=count;
      b->x_inactive=b->target_x_inactive=count+BUTTON_EXPOSE;
      b->y = h - BUTTONBAR_Y_FROM_BOTTOM;
      if(i!=9 || checkbutton_deployed) // special-case the checkbutton
	count -= BUTTONBAR_SPACING;
    }
  }
  
  // special-case the checkbutton
  if(!checkbutton_deployed){
    states[9].target_x_inactive=states[9].target_x_active+BUTTONBAR_SPACING;
    states[9].x_inactive=states[9].target_x_inactive;
    states[9].target_x=states[9].target_x_inactive;
  }

  for(i=0;i<NUMBUTTONS;i++)
    if(states[i].position)
      rollover_extents(g,states+i);
  
}

/* effects animated 'rollout' of buttons when level begins */
void deploy_buttonbar(Gameboard *g){
  if(!buttons_ready ){
    if(get_num_intersections() <= get_objective())
      checkbutton_deployed=1;
    else
      checkbutton_deployed=0;
    setup_buttonbar(g);
    timer = g_timeout_add(BUTTON_ANIM_INTERVAL, buttonbar_animate_buttons, (gpointer)g);
    buttons_ready=1;
  }

}

/* effects animated rollout of 'check' button */
void deploy_check(Gameboard *g){
  if(buttons_ready && !checkbutton_deployed){
    int i;
    for(i=5;i<9;i++){
      states[i].target_x-=BUTTONBAR_SPACING;
      states[i].target_x_active-=BUTTONBAR_SPACING;
      states[i].target_x_inactive-=BUTTONBAR_SPACING;
    }
    states[9].target_x=states[9].target_x_active;
    if(timer!=0)
      g_source_remove(timer);
    timer = g_timeout_add(BUTTON_ANIM_INTERVAL, buttonbar_animate_buttons, (gpointer)g);
    checkbutton_deployed=1;
  }
}

/* effects animated rollback of 'check' button */
void undeploy_check(Gameboard *g){
  if(checkbutton_deployed){
    int i;
    for(i=5;i<9;i++){
      states[i].target_x+=BUTTONBAR_SPACING;
      states[i].target_x_active+=BUTTONBAR_SPACING;
      states[i].target_x_inactive+=BUTTONBAR_SPACING;
    }
    states[9].target_x=states[9].target_x_inactive;
    if(timer!=0)
      g_source_remove(timer);
    timer = g_timeout_add(BUTTON_ANIM_INTERVAL, buttonbar_animate_buttons, (gpointer)g);
    checkbutton_deployed=0;
  }
}

/* effects animated rollback of buttons at end of level */
void undeploy_buttonbar(Gameboard *g, void (*callback)(Gameboard *g)){
  if(timer!=0)
    g_source_remove(timer);
  
  button_clear_state(g);
  buttons_ready=0;
  undeploy_callback=callback;
  checkbutton_deployed=0;

  g_timeout_add(BUTTON_ANIM_INTERVAL, buttonbar_deanimate_buttons, (gpointer)g);


}

