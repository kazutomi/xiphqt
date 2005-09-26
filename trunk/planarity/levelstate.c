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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "graph.h"
#include "levelstate.h"
#include "gameboard.h"
#include "dialog_pause.h"
#include "dialog_finish.h"
#include "dialog_level.h"
#include "main.h"
#include "graph_generate.h"

#define CHUNK 64
#define SAVENAME "levelstate"

typedef struct levelstate{
  struct levelstate *prev;
  struct levelstate *next;

  graphmeta gm;

  int in_progress;
  long highscore;
  
} levelstate;

static levelstate *head=0;
static levelstate *tail=0;
static levelstate *curr=0;
static levelstate *pool=0;
static int graph_dirty = 1;

static int aboutflag = 0;
static int pauseflag = 0;
static int finishflag = 0;
static int selectflag = 0;

static int level_limit = 1;
 
static levelstate *new_level(){
  levelstate *ret;
  int num=0;

  if(pool==0){
    int i;
    pool = calloc(CHUNK,sizeof(*pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's ->next points to nothing */
      pool[i].next=pool+i+1;
  }

  ret=pool;
  if(tail)
    num=tail->gm.num+1;

  if(generate_get_meta(num, &ret->gm)) return 0;

  pool=ret->next;

  ret->next=0;
  ret->prev=tail;
    
  if(tail){
    ret->prev->next=ret;
  }else{
    head=ret;
  }
  tail=ret;

  ret->highscore=0;
  ret->in_progress=0;

  /* write out a 'fresh' board icon if it doesn't already exist */
  if(!gameboard_icon_exists(ret->gm.id,"1")){
    // generate a graph we can use to make the icon
    graph g;
    memset(&g,0,sizeof(g));
    g.width=gameboard->g.orig_width;
    g.height=gameboard->g.orig_height;
    g.orig_width=gameboard->g.orig_width;
    g.orig_height=gameboard->g.orig_height;

    generate_board(&g,num);
    impress_location(&g);

    gameboard_write_icon(ret->gm.id,"1",gameboard,&g,1,1);
   
    // releases the temporary graph memory
    graph_release(&g);
  }

  
  return ret;
}

static levelstate *ensure_level_num(int level){

  if(level<0)return 0;

  if(!tail)new_level();

  if(level <= tail->gm.num){
    // find it in the existing levels
    levelstate *l=tail;
    while(l){
      if(level == l->gm.num) return l;
      l=l->prev;
    }
    return 0;
  }else{
    // make new levels to fill 
      
    while(tail->gm.num<level)
      new_level();

    return tail;
  }
}

static levelstate *ensure_level(char *name){
  int level=generate_find_number(name);
  return ensure_level_num(level);
}

int levelstate_write(){
  FILE *f;
  char *name=alloca(strlen(statedir)+strlen(SAVENAME)+1);
  name[0]=0;
  strcat(name,statedir);
  strcat(name,SAVENAME);

  if(!graph_dirty && (curr->in_progress || gameboard->finish_dialog_active)){
    gameboard_write(curr->gm.id,gameboard);
    gameboard_write_icon(curr->gm.id,"2",gameboard,&gameboard->g,
			 !gameboard->hide_lines,gameboard->show_intersections);

  }

  f = fopen(name,"wb");
  if(f==NULL){
    fprintf(stderr,"ERROR:  Could not save game state file \"%s\":\n\t%s\n",
	    curr->gm.id,strerror(errno));
    return errno;
  }
  
  fprintf(f,"current %d : %s\n",strlen(curr->gm.id),curr->gm.id);

  {
    levelstate *l=head;
    while(l){
      fprintf(f,"level %ld %d %d : %s\n",
	      l->highscore,l->in_progress,
	      strlen(l->gm.id),l->gm.id);
      l=l->next;
    }
  }

  if(gameboard->about_dialog_active)
    fprintf(f,"about 1\n");
  if(gameboard->pause_dialog_active)
    fprintf(f,"pause 1\n");
  if(gameboard->finish_dialog_active)
    fprintf(f,"finish 1\n");
  if(gameboard->level_dialog_active)
    fprintf(f,"select 1\n");
	  
  fclose(f);
  return 0;
}

// also functions as the levelstate init; always called once upon startup
int levelstate_read(){
  char *line=NULL;
  size_t n=0;

  FILE *f;
  char *name=alloca(strlen(statedir)+strlen(SAVENAME)+1);
  name[0]=0;
  strcat(name,statedir);
  strcat(name,SAVENAME);

  if(!head)new_level();
  if(!curr)curr=head;

  f = fopen(name,"rb");
  if(f==NULL){
    if(errno!=ENOENT){ 
      fprintf(stderr,"ERROR:  Could not read game state file \"%s\":\n\t%s\n",
	      curr->gm.id,strerror(errno));
    }
    return errno;
  }
  
  // get all levels we've seen.
  while(getline(&line,&n,f)>0){
    long l;
    int i;
    unsigned int j;
    if (sscanf(line,"level %ld %d %d : ",&l,&i,&j)==3){
      char *name=strchr(line,':');
      // guard against bad edits
      if(name &&
	 (strlen(line) - (name - line + 2) >= j)){
	levelstate *le;
	name += 2;
	name[j]=0;
	le = ensure_level(name);
	if(le){
	  le->highscore=l;
	  le->in_progress=i;

	  if(le->highscore)
	    if(le->gm.unlock_plus + le->gm.num > level_limit)
	      level_limit = le->gm.unlock_plus + le->gm.num;
	}
      }
    }
  }

  rewind(f);

  // get current
  while(getline(&line,&n,f)>0){
    int i;
    if (sscanf(line,"current %d : ",&i)==1){
      char *name=strchr(line,':');
      // guard against bad edits
      if(name &&
	 (strlen(line) - (name - line + 2) >= (unsigned)i)){
	levelstate *le;
	name += 2;
	name[i]=0;
	le = ensure_level(name);
	if(le)
	  curr=le;
      }
    }

    if(sscanf(line,"about %d",&i)==1)
      if(i==1)
	aboutflag=1;
    
    if(sscanf(line,"pause %d",&i)==1)
      if(i==1)
	pauseflag=1;
    
    if(sscanf(line,"finish %d",&i)==1)
      if(i==1)
	finishflag=1;

    if(sscanf(line,"select %d",&i)==1)
      if(i==1)
	selectflag=1;
	  
  }

  return 0;
}

void levelstate_resume(){

  levelstate_go();

  if(pauseflag){
    prepare_reenter_game(gameboard);
    pause_dialog(gameboard);
  }else if (aboutflag){
    prepare_reenter_game(gameboard);
    about_dialog(gameboard);
  }else if (finishflag){
    prepare_reenter_game(gameboard);
    finish_level_dialog(gameboard);
  }else if (selectflag){
    prepare_reenter_game(gameboard);
    level_dialog(gameboard,0);
  }else{
    prepare_reenter_game(gameboard);
    reenter_game(gameboard);
  }
  aboutflag=0;
  pauseflag=0;
  finishflag=0;
  selectflag=0;

}

long levelstate_total_hiscore(){
  long score=0;
  levelstate *l=head;

  while(l){
    score+=l->highscore;
    l=l->next;
  }
  return score;
}

long levelstate_get_hiscore(){
  if(!curr)return 0;
  return curr->highscore;
}

int levelstate_next(){
  if(!curr->next)
    new_level();

  if(curr->next){
    curr=curr->next;
    graph_dirty=1;
    return 1;
  }
  return 0;
}

int levelstate_prev(){
  if(curr->prev){
    curr=curr->prev;
    graph_dirty=1;
    return 1;
  }
  return 0;
}

int get_level_num(){
  return curr->gm.num;
}

char *get_level_desc(){
  return curr->gm.desc;
}

void levelstate_finish(){
  int score = graphscore_get_score(&gameboard->g) + 
    graphscore_get_bonus(&gameboard->g);
  curr->in_progress=0;
  if(score > curr->highscore)
    curr->highscore = score;

  if(curr->gm.unlock_plus + curr->gm.num > level_limit)
    level_limit = curr->gm.unlock_plus + curr->gm.num;

}

void levelstate_reset(){
  curr->in_progress=0;
  graph_dirty=1;
}

int levelstate_in_progress(){
  return curr->in_progress;
}

int levelstate_limit(){
  return level_limit;
}

/* commit to the currently selected level and set the game state to
   readiness using it */
void levelstate_go(){

  // we need to load the board if we're currently playing the board or sitting in the finish dialog right after
  if(curr->in_progress || finishflag){
    if(gameboard_read(curr->gm.id,gameboard)){
      /* not on disk or couldn't load it.  clear level state flags and get a fresh version */
      aboutflag=0;
      pauseflag=0;
      finishflag=0;
      selectflag=0;
      generate_board(&gameboard->g,curr->gm.num);
      activate_verticies(&gameboard->g);
      impress_location(&gameboard->g);
    }
  }else{
    /* no board in progress; fetch a new board */
    generate_board(&gameboard->g,curr->gm.num);
    activate_verticies(&gameboard->g);
    impress_location(&gameboard->g);
  }
  curr->in_progress=1;
  graph_dirty=0;
}

cairo_surface_t *levelstate_get_icon(int num){
  levelstate *l=ensure_level_num(num);
  if(l==0)return 0;
  return gameboard_read_icon(l->gm.id,(l->in_progress?"2":"1"),gameboard);
}
