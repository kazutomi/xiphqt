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

#define FINITE_LEVELS 23
static gen_instance i_list[FINITE_LEVELS]={ 

  {"simple",   1, "A Small Beginning",                              generate_simple,    1.,1., 1 }, // 1
  {"simple",   2, "My First Real Level(tm)",                        generate_simple,    1.,1., 2 }, // 2
  {"data",     0, "My First Mission Impossible(tm)",                generate_data,     20.,1., 3 }, // 3
  {"simple",   3, "Larger But Not Harder",                          generate_simple,    1.,1., 2 }, // 4
  {"crest",    5, "The Trick Is It's Easy",                         generate_crest,     1.,1., 2 }, // 5

  {"simple",   4, "Practice Before the Handbasket: One of Three",   generate_simple,    1.,1., 1 }, // 6
  {"simple",   5, "Practice Before the Handbasket: Two of Three",   generate_simple,    1.,1., 1 }, // 7
  {"simple",   6, "Practice Before the Handbasket: Three of Three", generate_simple,    1.,1., 1 }, // 8

  {"sparse",   4, "Tough and Stringy",                              generate_sparse,    1.2,1., 2 }, // 9
  {"sparse",   5, "Threadbare",                                     generate_sparse,    1.2,1., 2 }, // 10

  {"nasty",    4, "The Bumpy Circles Are Slightly More Difficult",  generate_nasty,    1.5,1., 3 }, // 11
  {"nasty",    5, "Three is a Magic Number",                        generate_nasty,    1.5,1., 3 }, // 12
  {"nasty",    6, "Last Call For (Sort of) Triangles (For Now)",    generate_nasty,    1.5,1., 3 }, // 13

  {"free",     4, "Something Only Subtly Different",                generate_freeform, 1.5,1., 3 }, // 14
  {"free",     5, "It Can Roll! Granted, Not Very Well",            generate_freeform, 1.5,1., 3 }, // 15
  {"free",     6, "If you squint, It's a Rounded Brick",            generate_freeform, 1.5,1., 3 }, // 16

  {"rogue",    5, "A New Objective",                                generate_rogue,    1.6,1., 3 }, // 17
  {"rogue",    6, "How Low Can You Go?",                            generate_rogue,    1.6,1., 3 }, // 18
  {"rogue",    7, "Industrial Military Complex",                    generate_rogue,    1.6,1., 4 }, // 19

  {"embed",    4, "The Hexagon is a Subtle And Wily Beast",         generate_embed,     2.,1.,  4 }, // 20
  {"embed",    5, "No, Really, The Hexagon Puzzles Are Harder",     generate_embed,     3.,1.,  5 }, // 21
  {"embed",    6, "Cursed?  Call 1-800-HEX-A-GON Today!",           generate_embed,     4.,1.,  6 }, // 22

  {"simple",   7, "Round but Straightforward",                      generate_simple,    1.,1.,  4 }, // 23




  //{"meshS",10, "Tough and Stringy",           generate_mesh_1S, 2.,1., 3 }, // 8
  //{"cloud", 9, "More of a Mess Than Usual",   generate_mesh_1_cloud, 2.,1., 3 }, // 9
};

#define LOOP_LEVELS 7
static gen_instance i_list_loop[LOOP_LEVELS]={ 
  {"dense",  8, "Algorithm: Original/Dense (Order: %d)", generate_dense,     .8,1., 5 }, // n
  {"simple", 8, "Algorithm: Original (Order: %d)",       generate_simple,    1.,1., 5 }, // n
  {"sparse", 8, "Algorithm: Original/Sparse (Order: %d)",generate_sparse,   1.2,1., 5 }, // n
  {"nasty",  8, "Algorithm: Nasty (Order: %d)",          generate_nasty,    1.5,1., 5 }, // n
  {"free",   8, "Algorithm: Freeform/4 (Order: %d)",     generate_freeform, 1.5,1., 5 }, // n
  {"rogue",  8, "Algorithm: Rogue (Order: %d)",          generate_rogue,    1.6,1., 5 }, // n
  {"embed",  8, "Algorithm: Embed (Order: %d)",          generate_embed,     4.,1., 5 }, // n
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

    if(asprintf(&gm->id,"%s %d",i_list_loop[classnum].class,ordernum)==-1){
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
