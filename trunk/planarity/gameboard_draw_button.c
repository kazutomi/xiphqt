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
#include <gtk/gtk.h>

#include "graph.h"
#include "gameboard.h"
#include "gameboard_draw_button.h"


/************************* simple round icon drawing *********************/

void path_button_help(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);
  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,-9,-2);
  cairo_curve_to(c,-8,-7, -4.5,-12, 0,-12);
  cairo_curve_to(c, 7,-12,  9,-6, 9,-4);
  cairo_curve_to(c, 9,0 ,6,2, 5,2);

  cairo_curve_to(c, 4,2, 2.5,3, 2.5,6);
  cairo_line_to(c,-2.5,6);

  cairo_curve_to(c,-2.5,3, -3,1, 0,-1);
  cairo_curve_to(c, 11,-4, -3,-12, -4,-2);
  cairo_close_path(c);

  cairo_move_to(c,-2.5,8);
  cairo_line_to(c,2.5,8);
  cairo_line_to(c,2.5,12);
  cairo_line_to(c,-2.5,12);
  cairo_close_path(c);
  cairo_fill_preserve(c);

  cairo_restore(c);

}

void path_button_pause(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);
  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_rectangle(c,-7,-9,5,18);
  cairo_rectangle(c,2,-9,5,18);
  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_exit(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);
  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,-6,-9.5);
  cairo_line_to(c,0,-3.5);
  cairo_line_to(c,6,-9.5);
  cairo_line_to(c,9.5,-6);
  cairo_line_to(c,3.5,0);
  cairo_line_to(c,9.5,6);
  cairo_line_to(c,6,9.5);
  cairo_line_to(c,0,3.5);
  cairo_line_to(c,-6,9.5);
  cairo_line_to(c,-9.5,6);
  cairo_line_to(c,-3.5,0);
  cairo_line_to(c,-9.5,-6);
  cairo_close_path(c);

  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_back(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,0,-11);
  cairo_line_to(c,-9,0);
  cairo_line_to(c,-3,0);
  cairo_line_to(c,-3,10);
  cairo_line_to(c,3,10);
  cairo_line_to(c,3,0);
  cairo_line_to(c,9,0);
  cairo_close_path(c);

  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_reset(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,-11,-5);
  cairo_line_to(c,-12,1);
  cairo_line_to(c,-6,1);
  cairo_line_to(c,0, 11);
  cairo_line_to(c,6,1);
  cairo_line_to(c,12,1);
  cairo_line_to(c,11,-5);
  cairo_line_to(c,4,-5);
  cairo_line_to(c,0,2);
  cairo_line_to(c,-4,-5);
  cairo_close_path(c);

  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_expand(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,-8.5,-3);
  cairo_line_to(c,-2.5,-3);
  cairo_line_to(c,-3,-8.5);
  cairo_line_to(c,3, -8.5);
  cairo_line_to(c,3,-3);
  cairo_line_to(c,8.5,-3);
  cairo_line_to(c,8.5,3);
  cairo_line_to(c,3,3);
  cairo_line_to(c,3,8.5);
  cairo_line_to(c,-3,8.5);
  cairo_line_to(c,-3,3);
  cairo_line_to(c,-8.5,3);
  cairo_close_path(c);

  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_shrink(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,-10,-3);
  cairo_line_to(c,10,-3);
  cairo_line_to(c,10,3);
  cairo_line_to(c,-10,3);
  cairo_close_path(c);

  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_lines(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,6,-4);
  cairo_arc(c,0,-4,6,0,2*M_PI);

  cairo_move_to(c,0,2);
  cairo_line_to(c,0,10);
  cairo_close_path(c);

  cairo_move_to(c,2.68328,5.36656-4);
  cairo_rel_line_to(c,4,8);
  cairo_close_path(c);

  cairo_move_to(c,-2.68328,5.36656-4);
  cairo_rel_line_to(c,-4,8);
  cairo_close_path(c);

  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_int(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,8,0);
  cairo_line_to(c,0,8);
  cairo_line_to(c,-8,0);
  cairo_line_to(c,0,-8);

  cairo_close_path(c);
 
  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_check(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,8,-8);
  cairo_curve_to(c, 7,-7, 11,-7.3, 10,-6);
  cairo_line_to(c,0,9);
  cairo_curve_to(c, -1,9.1, -2,11, -3,10);
  cairo_line_to(c,-11,4);
  cairo_curve_to(c, -12,3, -10,.5, -9,1);
  cairo_line_to(c,-3,3);

  cairo_close_path(c);
 
  cairo_fill_preserve(c);
  cairo_restore(c);

}

void path_button_play(cairo_t *c, double x, double y){
  
  cairo_save(c);
  cairo_translate(c,x,y);
  cairo_set_fill_rule(c,CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_width(c,1);

  cairo_arc(c,0,0,14,0,2*M_PI);

  cairo_move_to(c,-8,-8);
  cairo_line_to(c,10,0);
  cairo_line_to(c,-8,8);
  cairo_close_path(c);
 
  cairo_fill_preserve(c);
  cairo_restore(c);

}

