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
#include <string.h>
#include <stdlib.h>
#include "graph.h"
#include "graph_generate.h"

typedef struct {
  char *class;
  int instancenum;
  char *desc;
  void (*gen)(graph *g, int num);
  float intersection_mult;
  float objective_mult;
  int unlock;
} gen_instance;

#define FINITE_LEVELS 3
static gen_instance i_list[FINITE_LEVELS]={ 
  {"mesh1", 1, "a small beginning",        generate_mesh_1, 1.,1., 1 }, // 1
  {"mesh1", 2, "a bit larger",             generate_mesh_1, 1.,1., 2 }, // 2
  {"data" , 0, "canine... minus four",     generate_data,   1.,1.5,3 }, // 3
  {"mesh1", 3, "much like level two",      generate_mesh_1, 1.,1., 3 }, // 4
};

#define LOOP_LEVELS 1
static gen_instance i_list_loop[LOOP_LEVELS]={ 
  {"mesh1", 4, "\"original\" board number %d",    generate_mesh_1, 1.,1., 2 }, // n
};

int generate_find_number(char *id){
  int i;
  char buffer[160];
  
  for(i=0;i<FINITE_LEVELS;i++){
    snprintf(buffer,160,"%s %d",i_list[i].class,i_list[i].instancenum);
    if(!strcmp(id,buffer))
      return i;
  }
  
  {  
    char *arg = strchr(id,' ');
    if(arg){
      unsigned int len = arg-id;

      for(i=0;i<LOOP_LEVELS;i++){
	if(strlen(i_list_loop[i].class)==len &&
	   !strncmp(i_list_loop[i].class,id,len)){

	  // class match, determine the level number
	  int order = atoi(arg+1);
	  return FINITE_LEVELS + (order - i_list_loop[i].instancenum)*LOOP_LEVELS + i;
	
	
	}
      }
    }
  }

  return -1;
}

int generate_get_meta(int num, graphmeta *gm){
  if(num<FINITE_LEVELS){

    /* part of the finite list */

    gm->num = num;
    gm->desc = i_list[num].desc;
    gm->unlock_plus = i_list[num].unlock+1;
    if(asprintf(&gm->id,"%s %d",i_list[num].class,i_list[num].instancenum)==-1){
      fprintf(stderr,"Couldn't allocate memory for level name.\n");
      return -1;
    }
    return 0;
  }else{

    /* past the finite list; one of the loop levels */
    int classnum = (num - FINITE_LEVELS) % LOOP_LEVELS;
    int ordernum = (num - FINITE_LEVELS) / LOOP_LEVELS + 
      i_list_loop[classnum].instancenum;
    
    gm->num = num;
    gm->unlock_plus = i_list_loop[classnum].unlock+1;
    if(asprintf(&gm->desc,i_list_loop[classnum].desc,ordernum)==-1){
      fprintf(stderr,"Couldn't allocate memory for level desciption.\n");
      return -1;
    }

    if(asprintf(&gm->id,"%s %d",i_list[classnum].class,ordernum)==-1){
      fprintf(stderr,"Couldn't allocate memory for level name.\n");
      return -1;
    }
    
    return 0;

  }
}

void generate_board(graph *g,int num){
  gen_instance *gi;
  if(num<FINITE_LEVELS){
    gi = i_list+num;

    gi->gen(g,gi->instancenum);
  }else{
    int classnum = (num - FINITE_LEVELS) % LOOP_LEVELS;
    int ordernum = (num - FINITE_LEVELS) / LOOP_LEVELS + 
      i_list_loop[classnum].instancenum;

    gi = i_list_loop+classnum;
    gi->gen(g,ordernum);
  }

  g->objective_mult = gi->objective_mult;
  g->intersection_mult = gi->intersection_mult;
}
