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

#define LEVEL_BUTTON_BORDER 35
#define LEVEL_BUTTON_Y 25
#define LEVELBOX_WIDTH 560
#define LEVELBOX_HEIGHT 370

#define ICON_DELTA 20

extern void level_dialog(Gameboard *g,int advance);
extern void render_level_icons(Gameboard *g, cairo_t *c, int ex,int ey, int ew, int eh);
extern void level_icons_init(Gameboard *g);

extern void level_mouse_motion(Gameboard *g, int x, int y);
extern void level_mouse_press(Gameboard *g, int x, int y);
extern void local_reset (Gameboard *g);
