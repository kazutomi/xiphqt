#define _GNU_SOURCE
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "graph.h"
#include "gameboard.h"
#include "generate.h"
#include "gamestate.h"
#include "buttons.h"
#include "buttonbar.h"
#include "finish.h"
#include "version.h"

#define boardstate "/.gPlanarity/boards/"
#define mainstate "/.gPlanarity/"
Gameboard *gameboard;

static int width=800;
static int height=640;

static int level=0;
static int score=0;

static int initial_intersections;
static float intersection_mult=1.;
static int objective=0;
static int objective_lessthan=0;
static float objective_mult=1.;
static int paused=0;
static time_t begin_time_add=0;
static time_t begin_time;
static char *version = "";

static char *boarddir;
static char *statedir;

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

char *get_version_string(){
  return version;
}

static int dir_create(char *name){
  if(mkdir(name,0700)){
    switch(errno){
    case EEXIST:
      // this is ok
      return 0;
    default:
      fprintf(stderr,"ERROR:  Could not create directory (%s) to save game state:\n\t%s\n",
	      name,strerror(errno));
      return errno;
    }
  }
  return 0;
}     

int write_board(char *basename){
  vertex *v=get_verticies();
  edge *e=get_edges();
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

  while(v){
    fprintf(f,"vertex %d %d %d %d %d\n",
	    v->orig_x,v->orig_y,v->x,v->y,v->selected);
    v=v->next;
  }

  while(e){
    fprintf(f,"edge %d %d\n",e->A->num,e->B->num); 
    e=e->next;
  }

  fprintf(f,"objective %c %d\n",
	  (objective_lessthan?'*':'='),objective);
  fprintf(f,"scoring %d %f %f %ld\n",
	  initial_intersections,intersection_mult,objective_mult,(long)get_elapsed());
  fprintf(f,"board %d %d\n",
	  width,height);
	  
  //gameboard_write(f);

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

int read_board(char *basename){
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
  
  // successful open 
  // read the board
  new_board(0);

  {
    int i,x,y,ox,oy,sel,count=0,A,B;
    vertex **flat,*v;

    // get all verticies first
    while(getline(&line,&n,f)>0){
      
      {
	if(sscanf(line,"vertex %d %d %d %d %d\n",
		  &ox,&oy,&x,&y,&sel)==5){
	  
	  v=get_vertex();
	  v->orig_x=ox;
	  v->orig_y=oy;
	  v->x=x;
	  v->y=y;
	  v->selected=sel;
	  v->num=count++;
	}
      }
    }
    
    rewind(f);
    flat=alloca(count*sizeof(*flat));
    v=get_verticies();
    i=0;
    while(v){
      flat[i++]=v;
      v=v->next;
    }

    // get edges and other next
    while(getline(&line,&n,f)>0){
      int o;
      char c;
      long l;

      if(sscanf(line,"edge %d %d\n",&A,&B)==2){
	if(A>=0 && A<count && B>=0 && B<count)
	  add_edge(flat[A],flat[B]);
	else
	  fprintf(stderr,"WARNING: edge references out of range vertex in save file\n");
	
      }else if (sscanf(line,"objective %c %d\n",&c,&o)==2){
	
	objective_lessthan = (c=='*');
	objective = o;
      
      }else if (sscanf(line,"scoring %d %f %f %ld\n",
		       &initial_intersections,&intersection_mult,
		       &objective_mult,&l)==4){
	paused=1;
	begin_time_add = l;
	
      }else 
	sscanf(line,"board %d %d\n",&width,&height);	
    }
  }
  

  rewind(f);
    
  //read_gameboard(f);
  fclose (f);
  return 0;
}


int main(int argc, char *argv[]){
  GtkWidget *window;
  char *homedir = getenv("home");
  if(!homedir)
    homedir = getenv("HOME");
  if(!homedir)
    homedir = getenv("homedir");
  if(!homedir)
    homedir = getenv("HOMEDIR");
  if(!homedir){
    fprintf(stderr,"No homedir environment variable set!  gPlanarity will be\n"
	    "unable to permanently save any progress or board state.\n");
    boarddir=NULL;
    statedir=NULL;
  }else{
    boarddir=calloc(strlen(homedir)+strlen(boardstate)+1,1);
    strcat(boarddir,homedir);
    strcat(boarddir,boardstate);

    statedir=calloc(strlen(homedir)+strlen(mainstate)+1,1);
    strcat(statedir,homedir);
    strcat(statedir,mainstate);

    dir_create(statedir);
    dir_create(boarddir);
  }

  version=strstr(VERSION,"version.h");
  if(version){
    char *versionend=strchr(version,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend){
      int len=versionend-version-9;
      version=strdup(version+10);
      version[len-1]=0;
    }
  }else{
    version="";
  }

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
