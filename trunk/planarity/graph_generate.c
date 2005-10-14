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

#define FINITE_LEVELS 79
static gen_instance i_list[FINITE_LEVELS]={ 

  {"simple",   1, "A Small Beginning",                              generate_simple,    1.,4., 1 }, // 1
  {"simple",   2, "My First Real Level(tm)",                        generate_simple,    1.,4., 2 }, // 2
  {"data",     0, "My First Mission Impossible(tm)",                generate_data,     20.,4., 3 }, // 3
  {"simple",   3, "Larger But Not Harder",                          generate_simple,    1.,4., 2 }, // 4
  {"crest",    5, "The Trick Is It's Easy",                         generate_crest,     1.,4., 2 }, // 5

  {"simple",   4, "Practice Before the Handbasket: One of Three",   generate_simple,    1.,4., 2 }, // 6
  {"simple",   5, "Practice Before the Handbasket: Two of Three",   generate_simple,    1.,4., 2 }, // 7
  {"simple",   6, "Practice Before the Handbasket: Three of Three", generate_simple,    1.,4., 2 }, // 8

  {"sparse",   4, "Tough and Stringy",                              generate_sparse,    1.2,4., 2 }, // 9
  {"sparse",   5, "Threadbare",                                     generate_sparse,    1.2,4., 2 }, // 10

  {"nasty",    4, "The Bumpy Circles Are Slightly More Difficult",  generate_nasty,    1.5,4., 3 }, // 11
  {"nasty",    5, "Three is a Magic Number",                        generate_nasty,    1.5,4., 3 }, // 12
  {"nasty",    6, "Last Call For (Sort of) Triangles (For Now)",    generate_nasty,    1.5,4., 3 }, // 13

  {"free",     4, "Something Only Subtly Different",                generate_freeform, 1.5,4., 3 }, // 14
  {"free",     5, "It Can Roll! Granted, Not Very Well",            generate_freeform, 1.5,4., 3 }, // 15
  {"free",     6, "If You Squint, It's a Round Brick",              generate_freeform, 1.5,4., 3 }, // 16

  {"rogue",    5, "A New Objective",                                generate_rogue,    1.6,4., 3 }, // 17
  {"rogue",    6, "How Low Can You Go?",                            generate_rogue,    1.6,4., 3 }, // 18
  {"rogue",    7, "Military Industrial Complex",                    generate_rogue,    1.6,4., 4 }, // 19

  {"embed",    4, "The Hexagon is a Subtle and Wily Beast",         generate_embed,     2.,4.,  4 }, // 20
  {"embed",    5, "No, Really, The Hexagon Puzzles Are Harder",     generate_embed,     3.,4.,  5 }, // 21
  {"embed",    6, "Cursed?  Call 1-800-HEX-A-GON Today!",           generate_embed,     4.,4.,  6 }, // 22

  {"simple",   7, "Round but Straightforward",                      generate_simple,    1.,4.,  4 }, // 23

  {"shape",    0, "A Star Is Born... Then Solved",                  generate_shape,    1.,2.,  6 }, // 24
  {"shape",    1, "Oh, Rain*bows*...",                              generate_shape,    1.,2.,  6 }, // 25
  {"shape",    2, "Solve Along the Dotted Line",                    generate_shape,    1.,2.,  6 }, // 26
  {"shape",    3, "Using All Available Space",                      generate_shape,    1.,2.,  6 }, // 27
  {"shape",    4, "Brainfreeze",                                    generate_shape,    1.,2.,  6 }, // 28
  {"shape",    6, "Tropical Storm Invest",                          generate_shape,    1.,2.,  6 }, // 30

  {"dense",  8, "Algorithm: Original/Dense (Order: 8)", generate_dense,     .8,1., 5 }, // 31
  {"simple", 8, "Algorithm: Original (Order: 8)",       generate_simple,    1.,1., 6 }, // 32
  {"sparse", 8, "Algorithm: Original/Sparse (Order: 8)",generate_sparse,   1.2,1., 7 }, // 33
  {"nasty",  8, "Algorithm: Nasty (Order: 8)",          generate_nasty,    1.5,1., 8 }, // 34
  {"free",   8, "Algorithm: Freeform/4 (Order: 8)",     generate_freeform, 1.5,1., 9 }, // 35
  {"rogue",  8, "Algorithm: Rogue (Order: 8)",          generate_rogue,    1.6,1.,10 }, // 36
  {"embed",  8, "Algorithm: Embed (Order: 8)",          generate_embed,     4.,1.,10 }, // 37
  {"shape",  7, "Operator",                             generate_shape,     1.,2.,10 }, // 38

  {"dense",  9, "Algorithm: Original/Dense (Order: 9)", generate_dense,     .8,1., 5 }, // 39
  {"simple", 9, "Algorithm: Original (Order: 9)",       generate_simple,    1.,1., 6 }, // 40
  {"sparse", 9, "Algorithm: Original/Sparse (Order: 9)",generate_sparse,   1.2,1., 7 }, // 41
  {"nasty",  9, "Algorithm: Nasty (Order: 9)",          generate_nasty,    1.5,1., 8 }, // 42
  {"free",   9, "Algorithm: Freeform/4 (Order: 9)",     generate_freeform, 1.5,1., 9 }, // 43
  {"rogue",  9, "Algorithm: Rogue (Order: 9)",          generate_rogue,    1.6,1.,10 }, // 44
  {"embed",  9, "Algorithm: Embed (Order: 9)",          generate_embed,     4.,1.,10 }, // 45
  {"shape",  8, "The Inside Is Pointy",                 generate_shape,     1.,2.,10 }, // 46

  {"dense", 10, "Algorithm: Original/Dense (Order: 10)", generate_dense,     .8,1., 5 }, // 47
  {"simple",10, "Algorithm: Original (Order: 10)",       generate_simple,    1.,1., 6 }, // 48
  {"sparse",10, "Algorithm: Original/Sparse (Order: 10)",generate_sparse,   1.2,1., 7 }, // 49
  {"nasty", 10, "Algorithm: Nasty (Order: 10)",          generate_nasty,    1.5,1., 8 }, // 50
  {"free",  10, "Algorithm: Freeform/4 (Order: 10)",     generate_freeform, 1.5,1., 9 }, // 51
  {"rogue", 10, "Algorithm: Rogue (Order: 10)",          generate_rogue,    1.6,1.,10 }, // 52
  {"embed", 10, "Algorithm: Embed (Order: 10)",          generate_embed,     4.,1.,10 }, // 53
  {"shape",  9, "Riches and Good Luck",                  generate_shape,     1.,2.,10 }, // 54

  {"dense", 11, "Algorithm: Original/Dense (Order: 11)", generate_dense,     .8,1., 5 }, // 55
  {"simple",11, "Algorithm: Original (Order: 11)",       generate_simple,    1.,1., 6 }, // 56
  {"sparse",11, "Algorithm: Original/Sparse (Order: 11)",generate_sparse,   1.2,1., 7 }, // 57
  {"nasty", 11, "Algorithm: Nasty (Order: 11)",          generate_nasty,    1.5,1., 8 }, // 58
  {"free",  11, "Algorithm: Freeform/4 (Order: 11)",     generate_freeform, 1.5,1., 9 }, // 59
  {"rogue", 11, "Algorithm: Rogue (Order: 11)",          generate_rogue,    1.6,1.,10 }, // 60
  {"embed", 11, "Algorithm: Embed (Order: 11)",          generate_embed,     4.,1.,10 }, // 61
  {"shape", 10, "Mmmm... Doughnut",                      generate_shape,     1.,2.,10 }, // 62

  {"dense", 12, "Algorithm: Original/Dense (Order: 12)", generate_dense,     .8,1., 5 }, // 63
  {"simple",12, "Algorithm: Original (Order: 12)",       generate_simple,    1.,1., 6 }, // 64
  {"sparse",12, "Algorithm: Original/Sparse (Order: 12)",generate_sparse,   1.2,1., 7 }, // 65
  {"nasty", 12, "Algorithm: Nasty (Order: 12)",          generate_nasty,    1.5,1., 8 }, // 66
  {"free",  12, "Algorithm: Freeform/4 (Order: 12)",     generate_freeform, 1.5,1., 9 }, // 67
  {"rogue", 12, "Algorithm: Rogue (Order: 12)",          generate_rogue,    1.6,1.,10 }, // 68
  {"embed", 12, "Algorithm: Embed (Order: 12)",          generate_embed,     4.,1.,10 }, // 69
  {"shape", 11, "Quick and Dirty, or Slow and Steady",   generate_shape,     1.,1.,10 }, // 70
  {"shape",  5, "Little Fluffy Clouds",                   generate_shape,    1.,2., 6 }, // 29

  {"dense", 13, "Algorithm: Original/Dense (Order: 13)", generate_dense,     .8,1., 5 }, // 71
  {"simple",13, "Algorithm: Original (Order: 13)",       generate_simple,    1.,1., 6 }, // 72
  {"sparse",13, "Algorithm: Original/Sparse (Order: 13)",generate_sparse,   1.2,1., 7 }, // 73
  {"nasty", 13, "Algorithm: Nasty (Order: 13)",          generate_nasty,    1.5,1., 8 }, // 74
  {"free",  13, "Algorithm: Freeform/4 (Order: 13)",     generate_freeform, 1.5,1., 9 }, // 75
  {"rogue", 13, "Algorithm: Rogue (Order: 13)",          generate_rogue,    1.6,1.,10 }, // 76
  {"embed", 13, "Algorithm: Embed (Order: 13)",          generate_embed,     4.,1.,10 }, // 77
  {"shape", 12, "A Sudden Urge To Go Shopping",          generate_shape,     1.,1.,10 }, // 78
  {"shape", 13, "Sweet Reward",                          generate_shape,     1.,1.,10 }, // 79

};

#define LOOP_LEVELS 8
static gen_instance i_list_loop[LOOP_LEVELS]={ 
  {"dense", 14, "Algorithm: Original/Dense (Order: %d)", generate_dense,     .8,1., 5 }, // n
  {"simple",14, "Algorithm: Original (Order: %d)",       generate_simple,    1.,1., 6 }, // n
  {"sparse",14, "Algorithm: Original/Sparse (Order: %d)",generate_sparse,   1.2,1., 7 }, // n
  {"nasty", 14, "Algorithm: Nasty (Order: %d)",          generate_nasty,    1.5,1., 8 }, // n
  {"free",  14, "Algorithm: Freeform/4 (Order: %d)",     generate_freeform, 1.5,1., 9 }, // n
  {"rogue", 14, "Algorithm: Rogue (Order: %d)",          generate_rogue,    1.6,1.,10 }, // n
  {"embed", 14, "Algorithm: Embed (Order: %d)",          generate_embed,     4.,1.,10 }, // n
  {"shape", 14, "Algorithm: Region (Order: %d)",         generate_shape,     1.,2.,10 },
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
