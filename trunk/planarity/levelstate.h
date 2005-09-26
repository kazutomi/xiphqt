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

extern int levelstate_write();
extern int levelstate_read();
extern long levelstate_total_hiscore();
extern long levelstate_get_hiscore();
extern int levelstate_in_progress();
extern int levelstate_next();
extern int levelstate_prev();
extern int get_level_num();
extern char *get_level_desc();
extern void levelstate_finish();
extern void levelstate_go();
extern void levelstate_resume();
extern void levelstate_reset();
extern void set_in_progress();
extern int levelstate_limit();
extern cairo_surface_t* levelstate_get_icon(int num);
