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

#include "graph.h"
#include "random.h"
#include "gameboard.h"
#include "graph_arrange.h"

void arrange_verticies_circle(graph *g, float off1, float off2){
  vertex *v = g->verticies;
  int n = g->vertex_num;
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;
  int i;
  for(i=0;i<n;i++){
    v->x = rint( radius * cos( i*M_PI*2./n +off1) + (bw>>1));
    v->y = rint( radius * sin( i*M_PI*2./n +off2) + (bh>>1));
    v=v->next;
  }
}

void arrange_verticies_mesh(graph *g, int width, int height){
  vertex *v = g->verticies;
  int bw=g->orig_width;
  int bh=g->orig_height;
  int spacing=min((float)bw/width,(float)bh/height)*.9;
  int x,y;

  int xpad=(bw - (width-1)*spacing)/2;
  int ypad=(bh - (height-1)*spacing)/2;

  for(y=0;y<height;y++){
    for(x=0;x<width;x++){
      v->x=x*spacing+xpad;
      v->y=y*spacing+ypad;
      v=v->next;
    }
  }
}

void arrange_verticies_cloud(graph *g){
  vertex *v = g->verticies;
  int n = g->vertex_num;
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radiusx=min(bw,bh)*.55;
  int radiusy=min(bw,bh)*.40;
  int i;
  
  // first half form an outer perimiter

  for(i=0;i<n/2;i++){
    v->x = rint( radiusx * cos( i*M_PI*4./n) + (bw>>1));
    v->y = rint( radiusy * sin( i*M_PI*4./n) + (bh>>1));
    v=v->next;
  }

  // second third form an inner perimiter

  for(;i<n/2+n/3;i++){
    v->x = rint( radiusx * .7 * cos( i*M_PI*6./n) + (bw>>1));
    v->y = rint( radiusy * .7 * sin( i*M_PI*6./n) + (bh>>1));
    v=v->next;
  }

  for(;i<n;i++){
    v->x = rint( radiusx * .4 * cos( i*M_PI*12./n) + (bw>>1));
    v->y = rint( radiusy * .4 * sin( i*M_PI*12./n) + (bh>>1));
    v=v->next;
  }

}
