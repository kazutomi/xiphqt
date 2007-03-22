/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#ifndef _SUSHIMACRO_
#define _SUSHIMACRO_
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sushivision.h"

static sv_instance_t *svm_instance_curr = NULL; 
static int svm_instance_number = 0;

static double *svm_scale_vals_curr = NULL;
static int svm_scale_vsize_curr = 0;
static char **svm_scale_labels_curr = NULL;
static int svm_scale_lsize_curr = 0;
      
static sv_dim_t *svm_dim_curr = NULL;
static unsigned svm_scale_flags_curr = 0;
static sv_dim_t **svm_dim_list = NULL;
static int svm_dim_listsize = 0;

// helpers

#define svm_scale_check(a, b, c)					\
  ( svm_scale_vals_curr ?						\
    (fprintf(stderr,"No scale defined for %s %d (\"%s\")\n",#a,(b),c), \
     1 ):								\
    (svm_scale_labels_curr  && svm_scale_vsize_curr != svm_scale_lsize_curr ? \
     (fprintf(stderr,"Scale values and labels list size mismatch for %s %d (\"%s\")\n",#a,(b),c), \
      1 ):								\
     (0) ))

#define svm_dim_check(f) \
  ( !svm_dim_curr ?						\
    (fprintf(stderr,"No dimension currently set at %s\n",#f),	\
     1):							\
    (0) )

#define svm_dim_list_add(d) \
  ( (svm_dim_list && svm_dim_listsize ?					\
     (svm_dim_list = realloc(svm_dim_list, sizeof(*svm_dim_list)*(svm_dim_listsize+2))): \
     (svm_dim_list = calloc(2, sizeof(*svm_dim_list)))),		\
    svm_dim_list[svm_dim_listsize] = (d) ,				\
    svm_dim_listsize++)

#define svm_dim_list_clear()			\
  ( (svm_dim_list ?				\
     (free(svm_dim_list),			\
      svm_dim_list = NULL) : 0),		\
    svm_dim_listsize = 0 )

// toplevel

#define svm_new(name)					   \
  ( svm_instance_curr = sv_new(svm_instance_number, name), \
    svm_instance_number++,				   \
    svm_instance_curr )

#define svm_scale_vals(...)						\
  svm_scale_vals_curr = (double []){__VA_ARGS__};			\
  svm_scale_vsize_curr = (sizeof((double []){__VA_ARGS__})/sizeof(double));

#define svm_scale_labels(...)						\
  svm_scale_labels_curr = (char []){__VA_ARGS__};			\
  svm_scale_lsize_curr = (sizeof((char *[]){__VA_ARGS__})/sizeof(char *));

#define svm_scale_flags(number)			\
  (svm_scale_flags_curr = (number))

#define svm_dim(number, name)						\
  ( svm_scale_check(dimension,number,name) ?				\
    (svm_dim_curr = NULL ):						\
    (svm_dim_curr = sv_dim_new(svm_instance_curr, number, name, 0),	\
     sv_dim_make_scale(svm_dim_curr, svm_scale_vsize_curr, svm_scale_vals_curr, svm_scale_labels_curr, svm_scale_flags_curr), \
     svm_scale_vals_curr = NULL,					\
     svm_scale_labels_curr = NULL,					\
     svm_scale_vsize_curr = 0,						\
     svm_scale_lsize_curr = 0,						\
     svm_scale_flags_curr = 0,						\
     svm_dim_list_add(svm_dim_curr),					\
     svm_dim_curr) )

#define svm_dim_set(number) \
  ( !svm_instance_curr ?						\
    (fprintf(stderr,"No instance currently set while trying to retrieve dimension %d\n",(number)), \
     svm_dim_curr = NULL ):						\
    ( number<0 || number >= svm_instance_curr->dimensions ?		\
      (fprintf(stderr,"Dimension %d does not exist in instance \"%s\"\n", (number), svm_instance_curr->name), \
       svm_dim_curr = NULL) :						\
      (svm_dim_curr = svm_instance_curr->dimension_list[(number)])) )

#define svm_dim_discrete(num,den)					\
  ( svm_dim_check(svm_dim_curr)?					\
    1 :	sv_dim_set_discrete(svm_dim_curr,num,den))

#define svm_dim_value(a,b,c)						\
  ( svm_dim_check(svm_dim_disabled)?					\
    (1) :								\
    ( !isnan(a) ? sv_dim_set_value(svm_dim_curr,0,(a)) : 0,		\
      !isnan(b) ? sv_dim_set_value(svm_dim_curr,1,(b)) : 0,		\
      !isnan(c) ? sv_dim_set_value(svm_dim_curr,2,(c)) : 0,		\
      (0)))

#define svm_dim_list(...)						\
  ( svm_dim_list_clear(),						\
    svm_dim_listsize = sizeof((sv_dim_t *[]){__VA_ARGS__}) / sizeof(sv_dim_t *), \
    svm_dim_list = calloc(svm_dim_listsize + 1, sizeof(sv_dim_t *)),	\
    memcpy(svm_dim_list, (sv_dim_t *[]){__VA_ARGS__}, svm_dim_listsize * sizeof(sv_dim_t *)) )

#define svm_obj_simple(num,name,func,map)				\
  ( svm_func_list_clear(),						\
    svm_func_curr = sv_func(svm_instance_curr, svm_func_curr_num, strlen(map), func, 0), \
    svm_obj_curr = sv_obj_new(svm_obj_curr, num, name, (sv_func_t *[]){svm_func_curr}, \
			      (int []){0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},map,0), \
    (svm_scale_vals_curr ?						\
     (svm_scale_check(objective,number,name) ?				\
      (sv_obj_make_scale(svm_obj_curr, svm_scale_vsize_curr, svm_scale_vals_curr, svm_scale_labels_curr, svm_scale_flags_curr)):\
      (0)):0),								\
    svm_scale_clear(),							\
    svm_obj_curr)


#endif
