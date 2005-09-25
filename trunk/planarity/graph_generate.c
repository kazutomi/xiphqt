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
} gen_instance;

#define FINITE_LEVELS 1
static gen_instance i_list[FINITE_LEVELS]={ 
  {"mesh1", 1, "\"original\" board number one",   generate_mesh_1, 1.,1. },
  //{"mesh1", 2, "\"original\" board number two",   generate_mesh_1, 1.,1. },
  //{"mesh1", 3, "\"original\" board number three", generate_mesh_1, 1.,1. },
  //{"mesh1", 4, "\"original\" board number four",  generate_mesh_1, 1.,1. },
  //{"mesh1", 5, "\"original\" board number five",  generate_mesh_1, 1.,1. },
};

#define LOOP_LEVELS 1
static gen_instance i_list_loop[LOOP_LEVELS]={ 
  {"mesh1", 2, "\"original\" board number %d",    generate_mesh_1, 1.,1. },
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
	  return FINITE_LEVELS + (order - i_list_loop[i].instancenum)*FINITE_LEVELS + i;
	
	
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
