/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

void *xmalloc(size_t size)
{
  void *p;
  p=malloc(size);
  if(p==NULL){
    fprintf(stderr, "ERROR: Memory exhausted.\n");
    exit(1);
  }
  return p;
}

void *xrealloc(void *p, size_t size)
{
  p=realloc(p, size);
  if(p==NULL){
    fprintf(stderr, "ERROR: Memory exhausted.\n");
    exit(1);
  }
  return p;
}

char *xstrdup(const char *src)
{
  char *dest;
  dest=xmalloc(strlen(src)+1);
  strcpy(dest, src);
  return dest;
}
