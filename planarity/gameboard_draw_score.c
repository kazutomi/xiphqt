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

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "graph.h"
#include "gameboard.h"
#include "levelstate.h"

void draw_score(Gameboard *g){
  char level_string[160];
  char score_string[160];
  char int_string[160];
  char obj_string[160];
  cairo_text_extents_t extentsL;
  cairo_text_extents_t extentsS;
  cairo_text_extents_t extentsO;
  cairo_text_extents_t extentsI;
  cairo_matrix_t m;

  cairo_t *c = cairo_create(g->forescore);

  // clear the pane
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);

  topbox(g,c,g->g.width,SCOREHEIGHT);

  cairo_select_font_face (c, "Arial",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);

  cairo_matrix_init_scale (&m, 12.,15.);
  cairo_set_font_matrix (c,&m);
  cairo_set_source_rgba (c, TEXT_COLOR);

  snprintf(level_string,160,"Level %d: %s",get_level_num()+1,get_level_desc());
  snprintf(score_string,160,"Score: %d",graphscore_get_score(&g->g));
  snprintf(int_string,160,"Intersections: %ld",g->g.active_intersections);
  snprintf(obj_string,160,"Objective: %s",graphscore_objective_string(&g->g));

  cairo_text_extents (c, level_string, &extentsL);
  cairo_text_extents (c, obj_string, &extentsO);
  cairo_text_extents (c, int_string, &extentsI);
  cairo_text_extents (c, score_string, &extentsS);

  /*
  text_h = extentsL.height;
  text_h = max(text_h,extentsO.height);
  text_h = max(text_h,extentsI.height);
  text_h = max(text_h,extentsS.height);
  */

  int ty1 = 23;
  int ty2 = 38;

  cairo_move_to (c, 15, ty1);
  cairo_show_text (c, int_string);  
  cairo_move_to (c, 15, ty2);
  cairo_show_text (c, score_string);  

  cairo_move_to (c, g->g.width-extentsL.width-15, ty1);
  cairo_show_text (c, level_string);  
  cairo_move_to (c, g->g.width-extentsO.width-15, ty2);
  cairo_show_text (c, obj_string);  

  cairo_destroy(c);
}

void update_score(Gameboard *g){
  GdkRectangle r;
  draw_score(g);
  
  r.x=0;
  r.y=0;
  r.width=g->g.width;
  r.height = SCOREHEIGHT;
  gdk_window_invalidate_rect (g->w.window, &r, FALSE);

}

