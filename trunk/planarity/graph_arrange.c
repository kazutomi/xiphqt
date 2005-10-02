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
    v->x = rint( radius * sin( i*M_PI*2./n +off1) + (bw>>1));
    v->y = rint( radius * -cos( i*M_PI*2./n +off2) + (bh>>1));
    v=v->next;
  }
}

void arrange_verticies_polygon(graph *g, int sides, float angle, float rad, 
			       int xoff, int yoff, float xstretch, float ystretch){
  vertex *v = g->verticies;
  int n = g->vertex_num;
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*rad*.45;
  float perleg,del,acc=0;
  int i;

  for(i=0;i<sides;i++){
    int xA = sin(M_PI*2/sides*i+angle)*radius+bw/2+xoff;
    int yA = -cos(M_PI*2/sides*i+angle)*radius+bh/2+yoff;
    int xB = sin(M_PI*2/sides*(i+1)+angle)*radius+bw/2+xoff;
    int yB = -cos(M_PI*2/sides*(i+1)+angle)*radius+bh/2+yoff;

    float xD,yD;

    if(i==0){
      perleg = hypot((xA-xB),(yA-yB));
      del = perleg*sides / n;
    }

    xD = (xB-xA) / perleg;
    yD = (yB-yA) / perleg;
    
    while(v && acc<=perleg){
      v->x = rint(((xA + xD*acc) - bw/2) * xstretch + bw/2);
      v->y = rint(((yA + yD*acc) - bh/2) * ystretch + bh/2);
      v=v->next;
      acc+=del;
    }
    acc-=perleg;
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

void arrange_verticies_nastymesh(graph *g, int w, int h, vertex **flat){
  int A = 0; 
  int B = w-1;
  int i;

  arrange_verticies_mesh(g,w,h);

  while(A<B){
    for(i=0;i<=A;i++){
      flat[i]->y-=10;
      flat[B+i]->y-=10;

      flat[i+(h-1)*w]->y+=10;
      flat[B+i+(h-1)*w]->y+=10;
    }
    A++;
    B--;
  }

  A = 0; 
  B = (h-1)*w;

  while(A<B){
    for(i=0;i<=A;i+=w){
      flat[i]->x-=10;
      flat[B+i]->x-=10;

      flat[i  +(w-1)]->x+=10;
      flat[B+i+(w-1)]->x+=10;

    }
    A+=w;
    B-=w;
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
