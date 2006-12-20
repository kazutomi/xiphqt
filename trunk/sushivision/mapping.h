/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
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
typedef struct {
  int mapnum;
  double low;
  double high;
  double i_range;
  u_int32_t (*mapfunc)(double val,u_int32_t mix);
} mapping;

extern int num_mappings();
extern char *mapping_name(int i);
extern void mapping_setup(mapping *m, double lo, double hi, int funcnum);
extern void mapping_set_lo(mapping *m, double lo);
extern void mapping_set_hi(mapping *m, double hi);
extern void mapping_set_func(mapping *m, int funcnum);
extern double mapping_val(mapping *m, double in);
extern u_int32_t mapping_calc(mapping *m, double in, u_int32_t mix);
extern int mapping_inactive_p(mapping *m);
