#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "graph.h"
#include "gameboard.h"
#include "generate.h"
#include "gamestate.h"
#include "buttons.h"
#include "buttonbar.h"
#include "finish.h"

Gameboard *gameboard;

static int width=800;
static int height=640;

static int level=0;
static int score=0;

static int initial_intersections;
static int objective=0;
static int objective_lessthan=0;
static int paused=0;
static time_t begin_time_add=0;
static time_t begin_time;

time_t get_elapsed(){
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

static gboolean key_press(GtkWidget *w,GdkEventKey *event,gpointer in){

  if(event->keyval == GDK_q && event->state&GDK_CONTROL_MASK) 
    gtk_main_quit();

  return FALSE;
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

void setup_board(){
  generate_mesh_1(level);
  impress_location();
  initial_intersections = get_num_intersections();
  gameboard_reset(gameboard);

  //gdk_flush();
  deploy_buttonbar(gameboard);
  set_timer(0);
  unpause();
}

#define RESET_DELTA 2;

void reset_board(){
  int flag=1;
  int hide_state = get_hide_lines(gameboard);
  hide_lines(gameboard);

  vertex *v=get_verticies();
  while(v){
    deactivate_vertex(v);
    v=v->next;
  }

  while(flag){
    flag=0;

    v=get_verticies();
    while(v){
      if(v->x != v->orig_x || v->y != v->orig_y){
	flag=1;
      
	invalidate_region_vertex(gameboard,v);
	if(v->x<v->orig_x){
	  v->x+=RESET_DELTA;
	  if(v->x>v->orig_x)v->x=v->orig_x;
	}else{
	  v->x-=RESET_DELTA;
	  if(v->x<v->orig_x)v->x=v->orig_x;
	}
	if(v->y<v->orig_y){
	  v->y+=RESET_DELTA;
	  if(v->y>v->orig_y)v->y=v->orig_y;
	}else{
	  v->y-=RESET_DELTA;
	  if(v->y<v->orig_y)v->y=v->orig_y;
	}
	invalidate_region_vertex(gameboard,v);
      }
      v=v->next;
    }
    gdk_window_process_all_updates();
    gdk_flush();
    
  }

  v=get_verticies();
  while(v){
    activate_vertex(v);
    v=v->next;
  }

  if(!hide_state)show_lines(gameboard);
  if(get_num_intersections() <= get_objective()){
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
    vertex *v=get_verticies();
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

  vertex *v=get_verticies();
  while(v){
  
    if(!sel || v->selected){
      int nx = rint((v->x - x)*scale+x);
      int ny = rint((v->y - y)*scale+y);
      
      if(nx<0)nx=0;
      if(nx>=width)nx=width-1;
      if(ny<0)ny=0;
      if(ny>=height)ny=height-1;

      deactivate_vertex(v);
      v->x = nx;
      v->y = ny;
    }
    v=v->next;
  }
  
  v=get_verticies();
  while(v){
    activate_vertex(v);
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
  if(get_num_intersections()<=initial_intersections){
    pause();
    score+=initial_intersections;
    if(get_elapsed()<initial_intersections)
      score+=initial_intersections-get_elapsed();
    level++;
    undeploy_buttonbar(gameboard,finish_level_dialog);
  }
}

void quit(){
  undeploy_buttonbar(gameboard,gtk_main_quit);
}

int get_score(){
  return score + initial_intersections-get_num_intersections();
}

int get_raw_score(){
  return score;
}

int get_initial_intersections(){
  return initial_intersections;
}

int get_objective(){
  return objective;
}

char *get_objective_string(){
  return "zero intersections";
}

int get_level(){
  return level;
}

char *get_level_string(){
  return "original-style";
}

int main(int argc, char *argv[]){
  GtkWidget *window;
  gtk_init (&argc, &argv);
  
  window   = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "key-press-event",
                    G_CALLBACK (key_press), window);
  
  gameboard = gameboard_new ();

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET(gameboard));
  gtk_widget_show_all (window);

  setup_board(gameboard);

  gtk_main ();
  return 0;
}
