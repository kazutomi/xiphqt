#include <math.h>

#include "graph.h"
#include "arrange.h"
#include "gamestate.h"

void arrange_verticies_circle(graph *g){
  vertex *v = g->verticies;
  int n = g->vertex_num;
  int bw=get_orig_width();
  int bh=get_orig_height();
  int radius=min(bw,bh)*.45;
  int i;
  for(i=0;i<n;i++){
    int x = rint( radius * cos( i*M_PI*2./n));
    int y = rint( radius * sin( i*M_PI*2./n));
    move_vertex(g,v,x+(bw>>1),y+(bh>>1));
    v=v->next;
  }
}

void arrange_verticies_mesh(graph *g, int width, int height){
  vertex *v = g->verticies;
  int bw=get_orig_width();
  int bh=get_orig_height();
  int spacing=min((float)bw/width,(float)bh/height)*.9;
  int x,y;

  int xpad=(bw - (width-1)*spacing)/2;
  int ypad=(bh - (height-1)*spacing)/2;

  for(y=0;y<height;y++){
    for(x=0;x<width;x++){
      move_vertex(g,v,x*spacing+xpad,y*spacing+ypad);
      v=v->next;
    }
  }
}
