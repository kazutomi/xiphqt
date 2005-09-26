/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "timer.h"
#include "gameboard.h"
#include "gameboard_draw_button.h"
#include "gameboard_logic_buttonbar.h"
#include "dialog_finish.h"
#include "dialog_pause.h"
#include "dialog_level.h"

/************************ the lower button bar *********************/

/* initialize the rather weird little animation engine */
void setup_buttonbar(Gameboard *g){
  
  buttonstate *states=g->b.states;
  int xcount=BUTTONBAR_BORDER;
  int dcount=0;
  int i;
  int w=g->g.width;
  int h=g->g.height;

  states[0].rollovertext="exit gPlanarity";
  states[1].rollovertext="level selection menu";
  states[2].rollovertext="reset board";
  states[3].rollovertext="pause";
  states[4].rollovertext="help / about";
  states[5].rollovertext="expand";
  states[6].rollovertext="shrink";
  states[7].rollovertext="hide/show lines";
  states[8].rollovertext="mark intersections";
  states[9].rollovertext="click when finished!";

  states[0].callback = quit_action;
  states[1].callback = level_action;
  states[2].callback = reset_action;
  states[3].callback = pause_action;
  states[4].callback = about_action;
  states[5].callback = expand_action;
  states[6].callback = shrink_action;
  states[7].callback = toggle_hide_lines;
  states[8].callback = toggle_show_intersections;
  states[9].callback = finish_action;

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

      b->x = b->x_target = xcount;
      b->y_active = h - BUTTONBAR_Y_FROM_BOTTOM;
      b->y = b->y_target = b->y_inactive = h - BUTTONBAR_Y_FROM_BOTTOM + BUTTON_EXPOSE;
      b->sweepdeploy = dcount;
      xcount += BUTTONBAR_SPACING;
      dcount += SWEEP_DELTA;
      rollover_extents(g,states+i);
    }
  }
  
  xcount = w-BUTTONBAR_BORDER;
  dcount = 0;
  for(i=NUMBUTTONS-1;i>=0;i--){
    buttonstate *b=states+i;
    if(b->position == 3){

      b->x = b->x_target = xcount;
      b->y_active = h - BUTTONBAR_Y_FROM_BOTTOM;
      b->y = b->y_target = b->y_inactive = h - BUTTONBAR_Y_FROM_BOTTOM + BUTTON_EXPOSE;
      b->sweepdeploy = dcount;
      
      if(i!=9 || g->checkbutton_deployed){ // special-case the checkbutton
	xcount -= BUTTONBAR_SPACING;
	dcount += SWEEP_DELTA;
      }else{
	states[9].position = 0; //deactivate it for the deploy
      }
      rollover_extents(g,states+i);
    }
  }
}

/* effects animated 'rollout' of buttons when level begins */
void deploy_buttonbar(Gameboard *g){
  if(!g->b.buttons_ready ){
    if(g->g.active_intersections <= g->g.objective)
      g->checkbutton_deployed=1;
    else
      g->checkbutton_deployed=0;
    setup_buttonbar(g);
    deploy_buttons(g,0);
  }
  
}

/* effects animated rollout of 'check' button */
void deploy_check(Gameboard *g){
  buttonstate *states=g->b.states;
  if(g->b.buttons_ready && !g->checkbutton_deployed){
    int i;
    for(i=5;i<9;i++){
      states[i].x_target-=BUTTONBAR_SPACING;
      states[i].sweepdeploy += SWEEP_DELTA;
    }

    states[9].position = 3; //activate it
    states[9].y_target = states[9].y_active;
    states[i].sweepdeploy = 0;

    if(g->gtk_timer!=0)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
    g->checkbutton_deployed=1;
  }
}

/* effects animated rollback of 'check' button */
void undeploy_check(Gameboard *g){
  buttonstate *states=g->b.states;
  if(g->checkbutton_deployed){
    int i;
    for(i=5;i<9;i++){
      states[i].x_target+=BUTTONBAR_SPACING;
      states[i].sweepdeploy -= SWEEP_DELTA;
    }
    states[9].y_target=states[9].y_inactive;

    if(g->gtk_timer!=0)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
    g->checkbutton_deployed=0;
  }
}
