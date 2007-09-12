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

#include <sys/types.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
typedef struct{
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
} _sv_ccolor_t;
#else
typedef struct{
  unsigned char a;
  unsigned char r;
  unsigned char g;
  unsigned char b;
} _sv_ccolor_t;
#endif 

typedef union {
  _sv_ccolor_t c;
  u_int32_t u;
} _sv_ucolor_t;

typedef struct{
  long a;
  long r;
  long g;
  long b;
} _sv_lcolor_t;


typedef struct {
  int mapnum;
  float low;
  float high;
  float i_range;
  
} _sv_mapping_t;


extern int       _sv_mapping_names();
extern char     *_sv_mapping_name(int i);

extern int       _sv_solid_names();
extern char     *_sv_solid_name(int i);

extern void (*mapfunc[])(int, int, _sv_lcolor_t *);
extern _sv_ucolor_t (*mixfunc[])(_sv_ucolor_t, _sv_ucolor_t);
extern _sv_propmap_t *mapnames[];
extern void (*mapsolid[])(int, int, _sv_lcolor_t *);
extern _sv_ucolor_t (*mixsolid[])(_sv_ucolor_t, _sv_ucolor_t);
extern _sv_propmap_t *solidnames[];
