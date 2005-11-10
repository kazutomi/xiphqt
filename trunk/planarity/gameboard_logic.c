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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "graph.h"
#include "timer.h"
#include "gameboard.h"
#include "levelstate.h"
#include "dialog_finish.h"
#include "dialog_pause.h"
#include "dialog_level.h"
#include "main.h"

/* game state helpers ************************************************************************************/

void prepare_reenter_game(Gameboard *g){
  g->lit_vertex=0;
  g->grabbed_vertex=0;
  g->delayed_background=0;
  
  g->group_drag=0;
  g->button_grabbed=0;
  g->selection_grab=0;
  g->selection_active=num_selected_verticies(&g->g);
  
  update_full(g);
  update_score(g);
}

void reenter_game(Gameboard *g){
  deploy_buttonbar(g);
  unpause_timer();
  set_in_progress();
}

void enter_game(Gameboard *g){
  set_timer(0);
  prepare_reenter_game(g);
  reenter_game(g);
}

static void animate_verticies(Gameboard *g, 
			      int *x_target,
			      int *y_target,
			      float *delta){
  int flag=1,count=0;
  vertex *v=g->g.verticies;
  float *tx=alloca(g->g.vertex_num * sizeof(*tx));
  float *ty=alloca(g->g.vertex_num * sizeof(*ty));

  // hide intersections now; deactivating the verticies below will hide them anyway
  g->show_intersections = 0;
  g->realtime_background = 1;
  update_full(g);

  // deactivate all the verticies so they can be moved en-masse
  // without graph metadata updates at each move
  v=g->g.verticies;
  while(v){
    deactivate_vertex(&g->g,v);
    tx[count]=v->x;
    ty[count++]=v->y;
    v=v->next;
  }

  // animate
  while(flag){
    count=0;
    flag=0;
    
    v=g->g.verticies;
    while(v){
      if(v->x!=x_target[count] || v->y!=y_target[count]){
	
	float xd = x_target[count] - tx[count];
	float yd = y_target[count] - ty[count];
	float m = hypot(xd,yd);

	tx[count]+= xd/m*delta[count];
	ty[count]+= yd/m*delta[count];
	
	invalidate_vertex(g,v);
	invalidate_edges(GTK_WIDGET(g),v,0,0);

	v->x = rint(tx[count]);
	v->y = rint(ty[count]);

	if( (v->x > x_target[count] && xd > 0) ||
	    (v->x < x_target[count] && xd < 0) ||
	    (v->y > y_target[count] && yd > 0) ||
	    (v->y < y_target[count] && yd < 0) ){
	  v->x = x_target[count];
	  v->y = y_target[count];
	}else
	  flag=1;

	invalidate_vertex(g,v);
	invalidate_edges(GTK_WIDGET(g),v,0,0);

      }	    
      count++;
      v=v->next;
    }

    gdk_window_process_all_updates();
    gdk_flush();
    
  }

  // reactivate all the verticies 
  activate_verticies(&g->g);

  // update the score 
  update_score(g);

  // it's a reset; show lines is default. This also has the side
  // effect of forcing a full board redraw and expose
  g->realtime_background=0;
  update_full(g);

  // possible
  if(g->g.active_intersections <= g->g.objective){
    deploy_check(g);
  }else{
    undeploy_check(g);
  }

}


static void scale(Gameboard *g,double scale){
  int x=g->g.width/2;
  int y=g->g.height/2;
  int sel = num_selected_verticies(&g->g);
  vertex *v;
  int *tx=alloca(g->g.vertex_num * sizeof(*tx));
  int *ty=alloca(g->g.vertex_num * sizeof(*ty));
  float *tm=alloca(g->g.vertex_num * sizeof(*ty));
  int i=0;
  int minx=0;
  int maxx=g->g.width-1;
  int miny=0;
  int maxy=g->g.height-1;
  int okflag=0;

  // if selected, expand from center of selected mass
  if(sel){
    double xac=0;
    double yac=0;

    v=g->g.verticies;
    while(v){
      if(v->selected){
	xac+=v->x;
	yac+=v->y;
      }
      v=v->next;
    }
    x = xac / sel;
    y = yac / sel;
  }

  while(!okflag){
    okflag=1;
    i=0;
    v=g->g.verticies;
    while(v){
      
      if(!sel || v->selected){
	float nx = rint((v->x - x)*scale+x);
	float ny = rint((v->y - y)*scale+y);
	float m = hypot(nx - v->x,ny-v->y)/5;
	
	if(nx<minx){
	  scale = (float)(minx-x) / (v->x-x);
	  okflag=0;
	}

	if(nx>maxx){
	  scale = (float)(maxx-x) / (v->x - x);
	  okflag=0;
	}

	if(ny<miny){
	  scale = (float)(miny-y) / (v->y-y);
	  okflag=0;
	}

	if(ny>maxy){
	  scale = (float)(maxy-y) / (v->y - y);
	  okflag=0;
	}
	if(m<1)m=1;
      
	tx[i] = rint(nx);
	ty[i] = rint(ny);
	tm[i++] = m;
      }else{
	tx[i] = v->x;
	ty[i] = v->y;
	tm[i++] = 0;
      }
      v=v->next;
    }
  }
  
  if(scale>=.99999 && scale<=1.00001){
    ungrab_verticies(&g->g);
    v=g->g.verticies;
    while(v){
      if(v->x<2 || v->x>=g->g.width-3)v->grabbed=1;
      if(v->y<2 || v->y>=g->g.height-3)v->grabbed=1;
      v=v->next;
    }
    fade_marked(g);
    ungrab_verticies(&g->g);
  }else{
    animate_verticies(g,tx,ty,tm);
  }
}

static void gtk_main_quit_wrapper(Gameboard *g){
  fade_cancel(g);
  gtk_main_quit();
}

static void level_wrapper(Gameboard *g){
  fade_cancel(g);
  level_dialog(g,0);
}

static void finish_wrapper(Gameboard *g){
  fade_cancel(g);
  finish_level_dialog(g);
}

/* toplevel main gameboard action entry points; when a button is
   clicked and decoded to a specific action or a game state change
   triggers an action, one of the below functions is the entry
   point **************************************************************************************************************/

void quit_action(Gameboard *g){
  pause_timer();
  undeploy_buttons(g,gtk_main_quit_wrapper);
}

void level_action(Gameboard *g){
  pause_timer();
  undeploy_buttons(g,level_wrapper);
}

void finish_action(Gameboard *g){
  if(g->g.active_intersections<=g->g.original_intersections){
    pause_timer();
    levelstate_finish();
    undeploy_buttons(g,finish_wrapper);
  }
}

void pause_action(Gameboard *g){
  pause_timer();
  undeploy_buttons(g,pause_dialog);
}

void about_action(Gameboard *g){
  pause_timer();
  undeploy_buttons(g,about_dialog);
}

void expand_action(Gameboard *g){
  scale(g,1.2);
}

void shrink_action(Gameboard *g){
  scale(g,.8);
}

void set_hide_lines(Gameboard *g, int state){
  g->hide_lines=state;
  update_full(g);
}

void toggle_hide_lines(Gameboard *g){
  g->hide_lines= !g->hide_lines;
  update_full(g);
}

void set_show_intersections(Gameboard *g, int state){
  if(g->show_intersections != state){
    g->show_intersections=state;
    expose_full(g);
  }
}

void toggle_show_intersections(Gameboard *g){
  g->show_intersections=!g->show_intersections;
  expose_full(g);
}

void reset_action(Gameboard *g){
  vertex *v=g->g.verticies;
  int *target_x = alloca(g->g.vertex_num * sizeof(*target_x));
  int *target_y = alloca(g->g.vertex_num * sizeof(*target_y));
  float *target_m = alloca(g->g.vertex_num * sizeof(*target_m));
  int i=0;
  int xd = (g->g.width-g->g.orig_width)>>1;
  int yd = (g->g.height-g->g.orig_height)>>1;

  while(v){
    target_x[i]=v->orig_x+xd;
    target_y[i]=v->orig_y+yd;
    target_m[i++]=RESET_DELTA;
    v=v->next;
  }

  animate_verticies(g,target_x,target_y,target_m);
  
  // reset timer
  set_timer(0);
  unpause_timer();
}


/***************** save/load gameboard the widget state we want to be persistent **************/

// there are only a few things; lines, intersections
int gameboard_write(char *basename, Gameboard *g){
  char *name;
  FILE *f;

  name=alloca(strlen(boarddir)+strlen(basename)+1);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,basename);
  
  f = fopen(name,"wb");
  if(f==NULL){
    fprintf(stderr,"ERROR:  Could not save board state for \"%s\":\n\t%s\n",
	    get_level_desc(),strerror(errno));
    return errno;
  }

  graph_write(&g->g,f);

  if(g->hide_lines)
    fprintf(f,"hide_lines 1\n");
  if(g->show_intersections)
    fprintf(f,"show_intersections 1\n");
  
  fclose(f);

  return 0;
}

int gameboard_read(char *basename, Gameboard *g){
  FILE *f;
  char *name;
  char *line=NULL;
  size_t n=0;

  name=alloca(strlen(boarddir)+strlen(basename)+3);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,basename);
  
  f = fopen(name,"rb");
  if(f==NULL){
    fprintf(stderr,"ERROR:  Could not read saved board state for \"%s\":\n\t%s\n",
	    get_level_desc(),strerror(errno));
    return errno;
  }
  
  graph_read(&g->g,f);

  // get local game state
  while(getline(&line,&n,f)>0){
    int i;
    
    if (sscanf(line,"hide_lines %d",&i)==1)
      if(i)
	g->hide_lines = 1;
    
    if (sscanf(line,"show_intersections %d",&i)==1)
      if(i)
	g->show_intersections = 1;
    
  }
  
  fclose (f);
  free(line);
  request_resize(g->g.width,g->g.height);
  activate_verticies(&g->g);

  return 0;
}

