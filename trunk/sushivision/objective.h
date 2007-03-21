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

struct _sv_obj_internal {
  sv_func_t *x_func;
  sv_func_t *y_func;
  sv_func_t *z_func;
  sv_func_t *e1_func;
  sv_func_t *e2_func;
  sv_func_t *p1_func;
  sv_func_t *p2_func;
  sv_func_t *m_func;

  int x_fout;
  int y_fout;
  int z_fout;
  int e1_fout;
  int e2_fout;
  int p1_fout;
  int p2_fout;
  int m_fout;
};

