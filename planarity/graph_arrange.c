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
#include "graph_region.h"

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

void arrange_verticies_polycircle(graph *g, int sides, float angle, float split,
				  int radplus,int xoff,int yoff){

  vertex *v = g->verticies;
  int n = g->vertex_num;
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45+radplus;
  float perleg,perarc,del,acc=0;
  int i;

  for(i=0;i<sides;i++){
    float ang0 = M_PI*2/sides*i + angle;
    float ang1 = M_PI*2/sides*i + (M_PI/sides*split) + angle;
    float ang2 = M_PI*2/sides*(i+1) - (M_PI/sides*split) + angle;

    int xA = sin(ang1)*radius+bw/2;
    int yA = -cos(ang1)*radius+bh/2;
    int xB = sin(ang2)*radius+bw/2;
    int yB = -cos(ang2)*radius+bh/2;

    float xD,yD,aD;

    if(i==0){
      perleg = hypot((xA-xB),(yA-yB));
      perarc = 2*radius*M_PI * split / sides;
      del = (perleg+perarc)*sides / n;
    }

    // populate the first arc segment
    aD = (ang1-ang0)/perarc*2;
    while(v && acc<=perarc/2){
      v->x = rint( sin(ang0 + aD*acc)*radius+bw/2)+xoff;
      v->y = rint(-cos(ang0 + aD*acc)*radius+bh/2)+yoff;
      v=v->next;
      acc+=del;
    }
    acc-=perarc/2;

    // populate the line segment
    xD = (xB-xA) / perleg;
    yD = (yB-yA) / perleg;
    
    while(v && acc<=perleg){
      v->x = rint(xA + xD*acc)+xoff;
      v->y = rint(yA + yD*acc)+yoff;
      v=v->next;
      acc+=del;
    }
    acc-=perleg;

    // populate the second arc segment
    while(v && acc<=perarc/2){
      v->x = rint( sin(ang2 + aD*acc)*radius+bw/2)+xoff;
      v->y = rint(-cos(ang2 + aD*acc)*radius+bh/2)+yoff;
      v=v->next;
      acc+=del;
    }
    acc-=perarc/2;
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

void arrange_region_star(graph *g){
  region_init();
  region_new_area(400,45,1);
  region_line_to(340,232);
  region_line_to(143,232);
  region_line_to(303,347);
  region_line_to(241,533);
  region_line_to(400,417);
  region_line_to(559,533);
  region_line_to(497,347);
  region_line_to(657,232);
  region_line_to(460,232);
  region_close_line();
}

void arrange_region_rainbow(graph *g){
  region_init();
  region_new_area(40,500,2);
  region_arc_to(760,500,-M_PI);
  region_line_to(660,500);
  region_arc_to(200,500,M_PI);
  region_close_line();
}

void arrange_region_dashed_circle(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;
  int i;

  region_init();

  for(i=0;i<20;i+=2){
    float phi0=(M_PI*2.f/20*(i-.5));
    float phi1=(M_PI*2.f/20*(i+.5));
    int x1=  cos(phi0)*radius+bw/2;
    int y1= -sin(phi0)*radius+bh/2;
    int x2=  cos(phi1)*radius+bw/2;
    int y2= -sin(phi1)*radius+bh/2;
    region_new_area(x1,y1,3);
    region_arc_to(x2,y2,M_PI/10);
  }
}

void arrange_region_bifur(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;

  region_init();
  region_new_area(21,60,2);
  region_line_to(bw/2-130,60);
  region_arc_to(bw/2+130,60,-M_PI*.25);
  region_line_to(bw-21,60);

  region_new_area(21,bh-60,1);
  region_line_to(bw/2-130,bh-60);
  region_arc_to(bw/2+130,bh-60,M_PI*.25);
  region_line_to(bw-21,bh-60);
}

void arrange_region_dairyqueen(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;

  float phi0=(M_PI/2 + M_PI*2.f/8);
  float phi1=(M_PI/2 - M_PI*2.f/8);

  int x1=  cos(phi0)*radius+bw/2;
  int x2=  cos(phi1)*radius+bw/2;

  int y1= -sin(phi0)*radius+bh/2;
  int y2=  sin(phi0)*radius+bh/2;

  region_init();

  region_new_area(x1,y1,2);
  region_arc_to(x2,y1,-M_PI/2);
  region_line_to(750,bh/2);
  region_line_to(x2,y2);
  region_arc_to(x1,y2,-M_PI/2);
  region_line_to(50,bh/2);
  region_close_line();
}

void arrange_region_cloud(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45-20;
  int i=0;
  float phi0=(M_PI*2.f/20*(i-.5)),phi1;
  int x1=  cos(phi0)*radius+bw/2,x2;
  int y1= -sin(phi0)*radius+bh/2,y2;

  region_init();
  region_new_area(x1,y1,1);
  for(i=0;i<19;i++){
    phi1=(M_PI*2.f/20*(i+.5));
    x2=  cos(phi1)*radius+bw/2;
    y2= -sin(phi1)*radius+bh/2;
    region_arc_to(x2,y2,M_PI*.7);
  }
  region_close_arc(M_PI*.7);
}

void arrange_region_ring(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;

  region_init();
  region_circle(bw/2,bh/2,radius,3);
  region_circle(bw/2,bh/2,radius*.4,1);
}

void arrange_region_storm(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;
  int i=0;

  region_init();

  for(i=0;i<10;i++){
    float phi0=(M_PI*2.f/10*(i-.5));
    float phi1=(M_PI*2.f/10*(i+.5));
    int x1=  cos(phi0)*radius+bw/2;
    int y1= -sin(phi0)*radius+bh/2;
    int x2=  cos(phi1)*radius*.8+bw/2;
    int y2= -sin(phi1)*radius*.8+bh/2;

    region_new_area(x1,y1,1);
    region_arc_to(x2,y2,M_PI*.2);
  }

  region_circle(bw/2,bh/2,radius*.2,0);
}

void arrange_region_target(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;

  region_init();
  region_circle(bw/2,bh/2,radius,3);
  region_circle(bw/2,bh/2,radius*.5,1);
  region_split_here();
  region_circle(bw/2,bh/2,radius*.35,3);
}

void arrange_region_plus(graph *g){
  region_init();

  region_new_area(316,43,2);
  region_arc_to(483,43,-M_PI*2/10);
  region_line_to(483,216);
  region_line_to(656,216);
  region_arc_to(656,384,-M_PI*2/10);
  region_line_to(483,384);
  region_line_to(483,556);
  region_arc_to(316,556,-M_PI*2/10);
  region_line_to(316,384);
  region_line_to(143,384);
  region_arc_to(143,216,-M_PI*2/10);
  region_line_to(316,216);
  region_close_line();
}

void arrange_region_hole3(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;

  region_init();
  region_circle(bw/2,bh/2,radius,3);

  region_new_area(400,200,2);
  region_line_to(313,349);
  region_line_to(487,349);
  region_close_line();
}

void arrange_region_hole4(graph *g){
  int bw=g->orig_width;
  int bh=g->orig_height;
  int radius=min(bw,bh)*.45;

  region_init();
  region_circle(bw/2,bh/2,radius,3);

  region_new_area(325,225,1);
  region_line_to(475,225);
  region_line_to(475,375);
  region_line_to(325,375);
  region_close_line();
}

void arrange_region_ovals(graph *g){


  region_init();
  region_new_area(270,163,2);
  region_arc_to(530,163,-M_PI*.99);
  region_line_to(530,437);
  region_arc_to(270,437,-M_PI*.99);
  region_close_line();
  region_split_here();

  region_new_area(80,263,2);
  region_arc_to(230,263,-M_PI*.99);
  region_line_to(230,337);
  region_arc_to(80,337,-M_PI*.99);
  region_close_line();
  region_split_here();

  region_new_area(570,263,2);
  region_arc_to(720,263,-M_PI*.99);
  region_line_to(720,337);
  region_arc_to(570,337,-M_PI*.99);
  region_close_line();
  region_split_here();
}
