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
#include "dialog_level.h"
#include "levelstate.h"
#include "main.h"

static void unlevel_post (Gameboard *g){
  g->level_dialog_active=0;
  pop_curtain(g);
  levelstate_go();
  enter_game(g);
} 

static void unlevel_quit (Gameboard *g){
  gtk_main_quit();
} 

static void local_go (Gameboard *g){
  undeploy_buttons(g,unlevel_post);
}

static void local_quit (Gameboard *g){
  undeploy_buttons(g,unlevel_quit);
}

/* initialize the rather weird little animation engine */
static void setup_level_buttons(Gameboard *g,int bw, int bh){
  int i;
  int w=g->g.width;
  int h=g->g.height;
  buttonstate *states=g->b.states;

  states[0].rollovertext="exit gPlanarity";
  states[2].rollovertext="reset level";
  states[10].rollovertext="play level!";

  states[0].callback = local_quit;
  states[2].callback = local_reset;
  states[10].callback = local_go;

  for(i=0;i<NUMBUTTONS;i++)
    states[i].position=0;

  states[0].position = 2; //center
  states[2].position = 0; //undeployed by default (redundant)
  states[10].position = 2; //center

  {
    buttonstate *b=states;
    b->x = b->x_target = w/2 - bw/2 + LEVEL_BUTTON_BORDER;
    b->y_active = h/2 + bh/2 - LEVEL_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = 0;
  }

  {
    buttonstate *b=states+2;
    b->x = b->x_target = w/2;
    b->y_active = h/2 + bh/2 - LEVEL_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = SWEEP_DELTA;
  }

  {
    buttonstate *b=states+10;
    b->x = b->x_target = w/2 + bw/2 - LEVEL_BUTTON_BORDER;
    b->y_active = h/2 + bh/2 - LEVEL_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = SWEEP_DELTA;
  }

  rollover_extents(g,states);  
  rollover_extents(g,states+2);  
  rollover_extents(g,states+10);  
}

static void draw_levelbox(Gameboard *g){
  int w= g->g.width;
  int h= g->g.height;

  cairo_t *c = cairo_create(g->background);
  borderbox_path(c,
		 w/2 - LEVELBOX_WIDTH/2,
		 h/2 - LEVELBOX_HEIGHT/2,
		 LEVELBOX_WIDTH,
		 LEVELBOX_HEIGHT);
  cairo_set_source_rgb(c,1,1,1);
  cairo_fill(c);

  centerbox(c,
	    w/2 - LEVELBOX_WIDTH/2,
	    h/2 - LEVELBOX_HEIGHT/2,
	    LEVELBOX_WIDTH,
	    SCOREHEIGHT);

  centerbox(c,
	    w/2 - LEVELBOX_WIDTH/2,
	    h/2 + LEVELBOX_HEIGHT/2 - SCOREHEIGHT,
	    LEVELBOX_WIDTH,
	    SCOREHEIGHT);

  {
    cairo_matrix_t ma;

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    
    cairo_matrix_init_scale (&ma, 18.,18.);
    cairo_set_font_matrix (c,&ma);
    cairo_set_source_rgba (c, TEXT_COLOR);

    render_text_centered(c,"Available Levels", w/2,h/2-LEVELBOX_HEIGHT/2+SCOREHEIGHT/2);

  }

  cairo_destroy(c);
}

void level_dialog(Gameboard *g, int advance){

  g->level_dialog_active=1;
  levelstate_write();

  if(advance)
    levelstate_next();

  // set up new buttons
  setup_level_buttons(g,LEVELBOX_WIDTH, LEVELBOX_HEIGHT);
  level_icons_init(g);

  // draw pausebox
  push_curtain(g,draw_levelbox);

  // deploy new buttons
  deploy_buttons(g,0);
}

