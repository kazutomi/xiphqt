/*
 *
 *  squishyio
 *
 *      Copyright (C) 2010-2012 Xiph.Org
 *
 *  squishyball is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  squishyball is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with rtrecord; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#ifndef _SqI__H_
#define _SqI__H_

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
typedef struct pcm_struct pcm_t;

struct pcm_struct {
  char *name;
  int rate;
  int ch;
  int savebits;
  int savedither;
  char *matrix;
  float **data;
  off_t samples;
};

extern pcm_t *squishyio_load_file(const char *path);
extern int squishyio_save_file(const char *path, pcm_t *pcm,int clobber);
extern void free_pcm(pcm_t *pcm);
#endif
