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
#include "dialog_pause.h"

extern char *version;

static void unpause_post (Gameboard *g){
  // back to buttonbar activity!
  pop_curtain(g);
  deploy_buttonbar(g);
  unpause_timer();
  g->about_dialog_active=0;
  g->pause_dialog_active=0;
} 

static void unpause_quit (Gameboard *g){
  gtk_main_quit();
} 

static void local_unpause (Gameboard *g){
  undeploy_buttons(g,unpause_post);
}

static void local_quit (Gameboard *g){
  undeploy_buttons(g,unpause_quit);
}

/* initialize the rather weird little animation engine */
static void setup_pause_buttons(Gameboard *g,int bw, int bh){
  int i;
  int w=g->g.width;
  int h=g->g.height;
  buttonstate *states=g->b.states;

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
    b->x = b->x_target = w/2 - bw/2 + PAUSE_BUTTON_BORDER;
    b->y_active = h/2 + bh/2 - PAUSE_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = 0;
  }

  {
    buttonstate *b=states+10;
    b->x = b->x_target = w/2 + bw/2 - PAUSE_BUTTON_BORDER;
    b->y_active = h/2 + bh/2 - PAUSE_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = SWEEP_DELTA;
  }

  for(i=0;i<NUMBUTTONS;i++)
    if(states[i].position)
      rollover_extents(g,states+i);  
}

static void draw_pausebox(Gameboard *g){
  int w= g->g.width;
  int h= g->g.height;

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
    char *time = get_timer_string();

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
    render_bordertext_centered(c,"Time Elapsed:", w/2,h/2-30);
    render_bordertext_centered(c,time, w/2,h/2);
  }

  cairo_destroy(c);
}

void pause_dialog(Gameboard *g){
  g->pause_dialog_active=1;
  // set up new buttons
  setup_pause_buttons(g,PAUSEBOX_WIDTH, PAUSEBOX_HEIGHT);

  // draw pausebox
  push_curtain(g,draw_pausebox);

  // deploy new buttons
  deploy_buttons(g,0);
}

// the 'about' box is nearly identical, including the fact it pauses the game.
// we just piggyback it here

static void draw_aboutbox(Gameboard *g){
  int w= g->g.width;
  int h= g->g.height;

  cairo_t *c = cairo_create(g->background);
  borderbox_path(c,
		 w/2 - ABOUTBOX_WIDTH/2,
		 h/2 - ABOUTBOX_HEIGHT/2,
		 ABOUTBOX_WIDTH,
		 ABOUTBOX_HEIGHT);
  cairo_set_source_rgb(c,1,1,1);
  cairo_fill(c);

  centerbox(c,
	    w/2 - ABOUTBOX_WIDTH/2,
	    h/2 - ABOUTBOX_HEIGHT/2,
	    ABOUTBOX_WIDTH,
	    SCOREHEIGHT);

  centerbox(c,
	    w/2 - ABOUTBOX_WIDTH/2 ,
	    h/2 + ABOUTBOX_HEIGHT/2 - SCOREHEIGHT,
	    ABOUTBOX_WIDTH,
	    SCOREHEIGHT);

  {
    cairo_matrix_t ma;
    int y = h/2-ABOUTBOX_HEIGHT/2+SCOREHEIGHT/2;
    cairo_select_font_face (c, "Sans",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);

    cairo_matrix_init_scale (&ma, 18.,18.);
    cairo_set_font_matrix (c,&ma);
    cairo_set_source_rgba (c, TEXT_COLOR);

    render_text_centered(c,"gPlanarity", w/2,y);
    cairo_select_font_face (c, "Sans",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    y+=45;
    render_bordertext_centered(c,"Untangle the mess!", w/2,y);
    y+=30;

    cairo_matrix_init_scale (&ma, 13.,13.);
    cairo_set_font_matrix (c,&ma);
    render_bordertext_centered(c,"Drag verticies to eliminate crossed lines.", w/2,y); y+=16;
    render_bordertext_centered(c,"The objective may be a complete solution or", w/2,y); y+=16;
    render_bordertext_centered(c,"getting as close as possible to solving an", w/2,y); y+=16;
    render_bordertext_centered(c,"unsolvable puzzle.  Work quickly and", w/2,y); y+=16;
    render_bordertext_centered(c,"exceed the objective for bonus points!", w/2,y); y+=16;

    y+=16;
    cairo_move_to (c, w/2-100,y);
    cairo_line_to (c, w/2+100,y);
    cairo_stroke(c);
    y+=32;

    cairo_matrix_init_scale (&ma, 12.,13.);
    cairo_set_font_matrix (c,&ma);
    render_bordertext_centered(c,"gPlanarity written by Monty <monty@xiph.org>",w/2,y);y+=17;
    render_bordertext_centered(c,"as a demonstration of Gtk+/Cairo",w/2,y);y+=32;

    render_bordertext_centered(c,"Original Flash version of Planarity by",w/2,y);y+=17;
    render_bordertext_centered(c,"John Tantalo <john.tantalo@case.edu>",w/2,y);y+=32;

    render_bordertext_centered(c,"Original game concept by Mary Radcliffe",w/2,y);y+=17;


    y = h/2+ABOUTBOX_HEIGHT/2-SCOREHEIGHT/2;
    cairo_select_font_face (c, "Sans",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);

    cairo_matrix_init_scale (&ma, 10.,11.);
    cairo_set_font_matrix (c,&ma);
    render_text_centered(c,version, w/2,y);

  }

  cairo_destroy(c);
}

void about_dialog(Gameboard *g){
  g->about_dialog_active=1;

  // set up new buttons
  setup_pause_buttons(g,ABOUTBOX_WIDTH,ABOUTBOX_HEIGHT);

  // draw about box
  push_curtain(g,draw_aboutbox);

  // deploy new buttons
  deploy_buttons(g,0);
}
