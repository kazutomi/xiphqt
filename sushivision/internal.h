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

#include <signal.h>
extern void _sushiv_realize_panel(sushiv_panel_t *p);
extern void _sushiv_clean_exit(int sig);
extern int _sushiv_new_panel(sushiv_instance_t *s,
			     int number,
			     const char *name, 
			     unsigned scalevals,
			     double *scaleval_list,
			     int *objectives,
			     int *dimensions,
			     unsigned flags);

extern void _sushiv_panel_dirty_map(sushiv_panel_t *p);
extern void _sushiv_wake_workers(void);

extern int _sushiv_panel_cooperative_compute(sushiv_panel_t *p);

extern sig_atomic_t _sushiv_exiting;

#define SUSHIV_DIM_MASK 0x003
#define SUSHIV_X_DIM 0x001
#define SUSHIV_Y_DIM 0x002
