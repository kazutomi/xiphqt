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

extern void arrange_verticies_circle(graph *g, float off1, float off2);
extern void arrange_verticies_polygon(graph *g, int sides, float angle, float rad, 
				      int xoff, int yoff, float xa, float ya);
extern void arrange_verticies_polycircle(graph *g, int sides, float angle, float split,
					 int radplus,int xoff,int yoff);

extern void arrange_verticies_mesh(graph *g, int width, int height);
extern void arrange_verticies_nastymesh(graph *g, int w, int h, vertex **flat);


extern void arrange_region_star(graph *g);
extern void arrange_region_rainbow(graph *g);
extern void arrange_region_dashed_circle(graph *g);
extern void arrange_region_bifur(graph *g);
extern void arrange_region_dairyqueen(graph *g);
extern void arrange_region_cloud(graph *g);
extern void arrange_region_ring(graph *g);
extern void arrange_region_storm(graph *g);
extern void arrange_region_target(graph *g);
extern void arrange_region_plus(graph *g);
extern void arrange_region_hole3(graph *g);
extern void arrange_region_hole4(graph *g);
extern void arrange_region_ovals(graph *g);
