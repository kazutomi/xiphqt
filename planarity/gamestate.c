#define _GNU_SOURCE
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "graph.h"
#include "gameboard.h"
#include "generate.h"
#include "gamestate.h"
#include "buttons.h"
#include "buttonbar.h"
#include "finish.h"
#include "pause.h"

extern GtkWidget *toplevel_window;
extern Gameboard *gameboard;
extern graph maingraph;

static int width=800;
static int height=640;
static int orig_width=800;
static int orig_height=640;

static int initial_intersections;
static float intersection_mult=1.;
static int objective=0;
static int objective_lessthan=0;
static char objective_string[80];
static float objective_mult=1.;
static int paused=0;
static time_t begin_time_add=0;
static time_t begin_time;


time_t get_timer(){
  if(paused)
    return begin_time_add;
  else{
    time_t ret = time(NULL);
    return ret - begin_time + begin_time_add;
  }
}

void set_timer(time_t off){
  begin_time_add = off;
  begin_time = time(NULL);
}

void resize_board(int x, int y){
  width=x;
  height=y;
  check_verticies();
}

int get_board_width(){
  return width;
}

int get_board_height(){
  return height;
}

// graphs are originally generated to a 'nominal' minimum sized board.
int get_orig_width(){
  return orig_width;
}

int get_orig_height(){
  return orig_height;
}

void gamestate_generate(int level){
    generate_mesh_1(&maingraph,level);
    impress_location(&maingraph);
    initial_intersections = maingraph.original_intersections;
}

void gamestate_go(){
    gameboard_reset(gameboard);
    set_timer(0);
    deploy_buttonbar(gameboard);
    unpause();
}

#define RESET_DELTA 2;

void reset_board(){
  int flag=1;
  int hide_state = get_hide_lines(gameboard);
  hide_lines(gameboard);

  vertex *v=maingraph.verticies;
  while(v){
    deactivate_vertex(&maingraph,v);
    v=v->next;
  }

  while(flag){
    flag=0;

    v=maingraph.verticies;
    while(v){
      int bxd = (width - orig_width) >>1;
      int byd = (height - orig_height) >>1;
      int vox = v->orig_x + bxd;
      int voy = v->orig_y + byd;

      if(v->x != vox || v->y != voy){
	flag=1;
      
	invalidate_region_vertex(gameboard,v);
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
	invalidate_region_vertex(gameboard,v);
      }
      v=v->next;
    }
    gdk_window_process_all_updates();
    gdk_flush();
    
  }

  v=maingraph.verticies;
  while(v){
    activate_vertex(&maingraph,v);
    v=v->next;
  }

  if(!hide_state)show_lines(gameboard);
  if(maingraph.active_intersections <= get_objective()){
    deploy_check(gameboard);
  }else{
    undeploy_check(gameboard);
  }
  update_full(gameboard);
  
  // reset timer
  set_timer(0);
  unpause();
}

void pause(){
  begin_time_add = get_elapsed();
  paused=1;
}

void unpause(){
  paused=0;
  set_timer(begin_time_add);
}

int paused_p(){
  return paused;
}

void scale(double scale){
  int x=get_board_width()/2;
  int y=get_board_height()/2;
  int sel = selected(gameboard);
  // if selected, expand from center of selected mass
  if(sel){
    vertex *v=maingraph.verticies;
    double xac=0;
    double yac=0;
    while(v){
      if(v->selected){
	xac+=v->x;
	yac+=v->y;
      }
      v=v->next;
    }
    x = xac / selected(gameboard);
    y = yac / selected(gameboard);
  }

  vertex *v=maingraph.verticies;
  while(v){
  
    if(!sel || v->selected){
      int nx = rint((v->x - x)*scale+x);
      int ny = rint((v->y - y)*scale+y);
      
      if(nx<0)nx=0;
      if(nx>=width)nx=width-1;
      if(ny<0)ny=0;
      if(ny>=height)ny=height-1;

      deactivate_vertex(&maingraph,v);
      v->x = nx;
      v->y = ny;
    }
    v=v->next;
  }
  
  v=maingraph.verticies;
  while(v){
    activate_vertex(&maingraph,v);
    v=v->next;
  }

  update_full(gameboard);
}

void expand(){
  scale(1.2);
}

void shrink(){
  scale(.8);
}

void hide_show_lines(){
  if(get_hide_lines(gameboard))
    show_lines(gameboard);
  else
    hide_lines(gameboard);
}

void mark_intersections(){
  show_intersections(gameboard);
}

void finish_board(){
  if(maingraph.active_intersections<=initial_intersections){
    pause();
    levelstate_finish();
    undeploy_buttonbar(gameboard,finish_level_dialog);
  }
}

void quit(){
  pause();
  undeploy_buttonbar(gameboard,gtk_main_quit);
}

int get_score(){
  float intersection_score = (initial_intersections- maingraph.active_intersections)*
    intersection_mult;
  
  float obj_multiplier = 1;

  if(objective_lessthan)
    if(objective > maingraph.active_intersections)
      obj_multiplier += (objective-maingraph.active_intersections)*objective_mult;

  return rint( intersection_score * obj_multiplier );
}

int get_bonus(){
  float obj_multiplier = 1;

  if(objective_lessthan)
    if(objective > maingraph.active_intersections)
      obj_multiplier += (objective-maingraph.active_intersections)*objective_mult;
  
  if(get_elapsed()<initial_intersections)
    return rint ((initial_intersections-get_elapsed()) * obj_multiplier);
  
  return 0;
}

int get_initial_intersections(){
  return initial_intersections;
}

int get_objective(){
  return objective;
}

char *get_objective_string(){
  if(objective == 0)
    return "zero intersections";
  if(objective == 1){
    if(objective_lessthan){
      return "1 intersection or fewer";
    }else{
      return "1 intersection";
    }
  }else{
    snprintf(objective_string,80,"%d intersections%s",
	     objective,(objective_lessthan?
			" or fewer":""));
    return objective_string;
  }
}

int write_board(char *boarddir, char *basename){
  char *name;
  FILE *f;

  name=alloca(strlen(boarddir)+strlen(basename)+3);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,basename);
  
  f = fopen(name,"wb");
  if(f==NULL){
    fprintf(stderr,"ERROR:  Could not save board state for \"%s\":\n\t%s\n",
	    get_level_string(),strerror(errno));
    return errno;
  }

  graph_write(&maingraph,f);

  fprintf(f,"objective %c %d\n",
	  (objective_lessthan?'*':'='),objective);
  fprintf(f,"scoring %d %f %f %ld\n",
	  initial_intersections,intersection_mult,objective_mult,(long)get_elapsed());
  fprintf(f,"board %d %d %d %d\n",
	  width,height,orig_width,orig_height);

  gameboard_write(f, gameboard);

  fclose(f);

  strcat(name,".1");
  f = fopen(name,"wb");
  if(f==NULL){
    fprintf(stderr,"ERROR:  Could not save board state for \"%s\":\n\t%s\n",
	    get_level_string(),strerror(errno));
    return errno;
  }
  //gameboard_write_icon(f);
  
  fclose(f);
  return 0;
}

int read_board(char *boarddir,char *basename){
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
    if(errno != ENOENT){
      fprintf(stderr,"ERROR:  Could not read saved board state for \"%s\":\n\t%s\n",
	      get_level_string(),strerror(errno));
    }
    return errno;
  }
  
  graph_read(&maingraph,f);

  // get local game state
  while(getline(&line,&n,f)>0){
    int o;
    char c;
    long l;
    
    if (sscanf(line,"objective %c %d",&c,&o)==2){
      
      objective_lessthan = (c=='*');
      objective = o;

    }else if (sscanf(line,"scoring %d %f %f %ld",
		     &initial_intersections,&intersection_mult,
		     &objective_mult,&l)==4){
      paused=1;
      begin_time_add = l;
      
    }else{
      sscanf(line,"board %d %d %d %d",&width,&height,&orig_width,&orig_height);	

    }
  }
  

  rewind(f);
    
  gameboard_read(f,gameboard);
  fclose (f);
  free(line);

  gtk_window_resize(GTK_WINDOW(toplevel_window),width,height);
  gameboard_reset(gameboard);

  return 0;
}

