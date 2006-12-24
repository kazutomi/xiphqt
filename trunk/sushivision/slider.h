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

struct _Slider {
  GtkWidget **slices;
  int num_slices;
  
  u_int32_t *backdata;
  cairo_surface_t *background;
  cairo_surface_t *foreground;
  int w;
  int h;
  mapping *gradient;
  int xpad;
  int ypad;

  char **label;
  double *label_vals;
  int labels;
  int neg;
  int flags;

  //double minstep;
  //double step;
};

#define SLIDER_FLAG_INDEPENDENT_MIDDLE 0x1

extern void slider_draw_background(Slider *s);
extern void slider_realize(Slider *s);
extern void slider_draw(Slider *);
extern void slider_expose_slice(Slider *s, int slicenum);
extern void slider_expose(Slider *s);
extern void slider_size_request_slice(Slider *s,GtkRequisition *requisition);
extern double slider_pixel_to_val(Slider *slider,double x);
extern double slider_pixel_to_del(Slider *slider,double x);
extern double slider_val_to_del(Slider *slider,double v);
extern void slider_vals_bound(Slider *slider,int slicenum);
extern int slider_lightme(Slider *slider,int slicenum,int x,int y);
extern void slider_unlight(Slider *slider);
extern void slider_button_press(Slider *slider,int slicenum,int x,int y);
extern void slider_button_release(Slider *s,int slicenum,int x,int y);
extern void slider_motion(Slider *s,int slicenum,int x,int y);
extern gboolean slider_key_press(Slider *slider,GdkEventKey *event,int slicenum);
extern Slider *slider_new(Slice **slices, int num_slices, 
			  char **labels, double *label_vals, int num_labels,
			  unsigned flags);
extern void slider_set_thumb_active(Slider *s, int thumbnum, int activep);
extern void slider_set_gradient(Slider *s, mapping *m);
extern double slider_get_value(Slider *s, int thumbnum);
extern void slider_set_value(Slider *s, int thumbnum, double v);
extern double slider_del_to_val(Slider *s, double del);
