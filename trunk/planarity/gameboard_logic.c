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
}

void enter_game(Gameboard *g){
  set_timer(0);
  prepare_reenter_game(g);
  reenter_game(g);
}

static void scale(Gameboard *g,double scale){
  int x=g->g.width/2;
  int y=g->g.height/2;
  int sel = num_selected_verticies(&g->g);
  vertex *v;

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

  v=g->g.verticies;
  while(v){
  
    if(!sel || v->selected){
      int nx = rint((v->x - x)*scale+x);
      int ny = rint((v->y - y)*scale+y);
      
      if(nx<0)nx=0;
      if(nx>=g->g.width)nx=g->g.width-1;
      if(ny<0)ny=0;
      if(ny>=g->g.height)ny=g->g.height-1;

      deactivate_vertex(&g->g,v);
      v->x = nx;
      v->y = ny;
    }
    v=v->next;
  }
  
  v=g->g.verticies;
  while(v){
    activate_vertex(&g->g,v);
    v=v->next;
  }

  update_full(g);
}

static void gtk_main_quit_wrapper(Gameboard *g){
  gtk_main_quit();
}

static void level_wrapper(Gameboard *g){
  level_dialog(g,0);
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
    undeploy_buttons(g,finish_level_dialog);
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
  int flag=1;
  vertex *v=g->g.verticies;

  // hide intersections now; deactivating the verticies below will hide them anyway
  g->show_intersections = 0;
  expose_full(g);
  gdk_window_process_all_updates();
  gdk_flush();

  // deactivate all the verticies so they can be moved en-masse
  // without graph metadata updates at each move
  while(v){
    deactivate_vertex(&g->g,v);
    v=v->next;
  }

  // animate
  while(flag){
    flag=0;

    v=g->g.verticies;
    while(v){
      int bxd = (g->g.width - g->g.orig_width) >>1;
      int byd = (g->g.height - g->g.orig_height) >>1;
      int vox = v->orig_x + bxd;
      int voy = v->orig_y + byd;

      if(v->x != vox || v->y != voy){
	flag=1;
      
	invalidate_vertex(g,v);
	if(v->x<vox){
	  v->x+=RESET_DELTA;
	  if(v->x>vox)v->x=vox;
	}else{
	  v->x-=RESET_DELTA;
	  if(v->x<vox)v->x=vox;
	}
	if(v->y<voy){
	  v->y+=RESET_DELTA;
	  if(v->y>voy)v->y=voy;
	}else{
	  v->y-=RESET_DELTA;
	  if(v->y<voy)v->y=voy;
	}
	invalidate_vertex(g,v);
      }
      v=v->next;
    }
    gdk_window_process_all_updates();
    gdk_flush();
    
  }

  // reactivate all the verticies 
  activate_verticies(&g->g);

  // it's a reset; show lines is default. This also has the side
  // effect of forcing a full board redraw and expose
  set_hide_lines(g,0);

  // hey, the board could ahve come pre-solved
  if(g->g.active_intersections <= g->g.objective){
    deploy_check(g);
  }else{
    undeploy_check(g);
  }
  
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

  return 0;
}

