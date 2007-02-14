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

typedef struct {
  char *legend;
  double lo;
  double hi;
  int neg;

  long long first_val;
  long first_pixel;
  
  long step_val;
  long step_pixel;

  int decimal_exponent;
  double m;

  int init;
  int pixels;
  int spacing;
  int massaged; // set if range had to be adjusted to avoid underflows
} scalespace;

int del_depth(double A, double B);
extern char **scale_generate_labels(unsigned scalevals, double *scaleval_list);
extern double scalespace_scaledel(scalespace *from, scalespace *to);
extern long scalespace_scalenum(scalespace *from, scalespace *to);
extern long scalespace_scaleden(scalespace *from, scalespace *to);
extern long scalespace_scaleoff(scalespace *from, scalespace *to);
extern double scalespace_value(scalespace *s, double pixel);
extern double scalespace_pixel(scalespace *s, double val);
extern int scalespace_mark(scalespace *s, int num);
extern double scalespace_label(scalespace *s, int num, char *buffer);
extern scalespace scalespace_linear (double lowpoint, double highpoint, int pixels, int max_spacing,char *name);
