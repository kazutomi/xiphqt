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
  long step_pixel;

  int five_exponent;
  int two_exponent;
  double expm;

  int init;
  int pixels;
  int spacing;
  int massaged; // set if range had to be adjusted to avoid underflows
} _sv_scalespace_t;

extern char **_sv_scale_generate_labels(unsigned scalevals, double *scaleval_list);
extern double _sv_scalespace_scaledel(_sv_scalespace_t *from, _sv_scalespace_t *to);
extern long _sv_scalespace_scalenum(_sv_scalespace_t *from, _sv_scalespace_t *to);
extern long _sv_scalespace_scaleden(_sv_scalespace_t *from, _sv_scalespace_t *to);
extern long _sv_scalespace_scaleoff(_sv_scalespace_t *from, _sv_scalespace_t *to);
extern long _sv_scalespace_scalebin(_sv_scalespace_t *from, _sv_scalespace_t *to);
extern double _sv_scalespace_value(_sv_scalespace_t *s, double pixel);
extern double _sv_scalespace_pixel(_sv_scalespace_t *s, double val);
extern int _sv_scalespace_mark(_sv_scalespace_t *s, int num);
extern double _sv_scalespace_label(_sv_scalespace_t *s, int num, char *buffer);
extern _sv_scalespace_t _sv_scalespace_linear (double lowpoint, double highpoint, int pixels, int max_spacing,char *name);
extern void _sv_scalespace_double(_sv_scalespace_t *s);
extern int _sv_scalespace_decimal_exponent(_sv_scalespace_t *s);
