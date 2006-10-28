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

#define _GNU_SOURCE
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "mapping.h"
#include "slice.h"
#include "slider.h"

static int total_slice_width(Slider *s){
  int i;
  int count=0;
  for(i=0;i<s->num_slices;i++)
    count += s->slices[i]->allocation.width;
  return count;
}

static int slice_width(Slider *s,int slices){
  int i;
  int count=0;
  for(i=0;i<slices;i++)
    count += s->slices[i]->allocation.width;
  return count;
}

static int total_slice_height(Slider *s){
  int i;
  int max=0;
  for(i=0;i<s->num_slices;i++)
    if(max<s->slices[i]->allocation.height)
      max = s->slices[i]->allocation.height;
  return max;
}

/* guess where I came from. */
static void rounded_rectangle (cairo_t *c,
			       double x, double y, double w, double h,
			       double radius)
{
  cairo_move_to (c, x+radius, y);
  cairo_arc (c, x+w-radius, y+radius, radius, M_PI * 1.5, M_PI * 2);
  cairo_arc (c, x+w-radius, y+h-radius, radius, 0, M_PI * 0.5);
  cairo_arc (c, x+radius,   y+h-radius, radius, M_PI * 0.5, M_PI);
  cairo_arc (c, x+radius,   y+radius,   radius, M_PI, M_PI * 1.5);
}

double shades[] = {1.15, 0.95, 0.896, 0.82, 0.7, 0.665, 0.5, 0.45, 0.4};

static void set_shade(GtkWidget *w, cairo_t *c, int shade){
  Slice *sl = SLICE(w);
  GdkColor *bg = &w->style->bg[sl->thumb_state?GTK_STATE_ACTIVE:GTK_STATE_NORMAL];
  double shade_r=bg->red*shades[shade]/65535;
  double shade_g=bg->green*shades[shade]/65535;
  double shade_b=bg->blue*shades[shade]/65535;

  cairo_set_source_rgb (c, shade_r,shade_g,shade_b);
}

static void parent_shade(Slider *s, cairo_t *c, int shade){
  GtkWidget *parent=gtk_widget_get_parent(s->slices[0]);
  GdkColor *bg = &parent->style->bg[GTK_STATE_NORMAL];
  double shade_r=bg->red*shades[shade]/65535;
  double shade_g=bg->green*shades[shade]/65535;
  double shade_b=bg->blue*shades[shade]/65535;

  cairo_set_source_rgb (c, shade_r,shade_g,shade_b);
}

void slider_draw_background(Slider *s){
  int i;
  GtkWidget *parent=gtk_widget_get_parent(s->slices[0]);
  GdkColor *bg = &parent->style->bg[0];
  GdkColor *fg = &s->slices[0]->style->fg[0];
  int textborder=1;
  double textr=1.;
  double textg=1.;
  double textb=1.;

  int x=0;
  int y=0;
  int w=s->w;
  int h=s->h;
  
  int tx=x;
  int ty=y+s->ypad;
  int tw=w;
  int th=h - s->ypad*2;
  cairo_pattern_t *pattern;

  // prepare background 
  cairo_t *c = cairo_create(s->background);
  
  // Fill with bg color
  gdk_cairo_set_source_color(c,bg); 
  cairo_rectangle(c,0,0,w,h);
  cairo_fill(c);

  // Create trough innards
 if(s->gradient){
    // background map gradient 
    u_int32_t *pixel=s->backdata+ty*s->w;
    
    for(i=tx;i<tx+tw;i++)
      pixel[i]=mapping_calc(s->gradient,slider_pixel_to_del(s,0,i));
    
    for(i=ty+1;i<ty+th;i++){
      memcpy(pixel+w,pixel,w*4);
      pixel+=s->w;
    }
 
  }else{
    // normal background
    textr=fg->red;
    textg=fg->green;
    textb=fg->blue;
    textborder=0;
 
    cairo_rectangle (c, x+1, ty, w-2, th);
    parent_shade(s,c,3);
    cairo_fill (c);
  }

  // Top shadow 
  cairo_rectangle (c, tx+1, ty, tw-2, 4);
  pattern = cairo_pattern_create_linear (0, ty-1, 0, ty+3);
  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0., 0., 0., 0.2);	
  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0., 0., 0., 0.);	
  cairo_set_source (c, pattern);
  cairo_fill (c);
  cairo_pattern_destroy (pattern);

  // Left shadow 
  cairo_rectangle (c, tx+1, ty, tx+4, th);
  pattern = cairo_pattern_create_linear (tx, ty-1, tx+3, ty-1);
  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0., 0., 0., 0.1);	
  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0., 0., 0., 0.);	
  cairo_set_source (c, pattern);
  cairo_fill (c);
  cairo_pattern_destroy (pattern);
 
  // lines & labels
  for(i=0;i<s->labels;i++){
    int x=rint(((float)i)/(s->labels-1)*(s->w - s->xpad*2 -1))+s->xpad;
    int y=s->h-s->ypad-1.5;
    
    cairo_move_to(c,x+.5,s->ypad+2);
    cairo_line_to(c,x+.5,s->h-s->ypad-2);
    cairo_set_source_rgba(c,textr,textg,textb,.8);
    cairo_set_line_width(c,1);
    cairo_stroke(c);

    if(i>0){
      cairo_text_extents_t ex;
      cairo_text_extents (c, s->label[i], &ex);
      
      x-=2;
      x-=ex.width;
 
    }else{
      x+=2;
    }

    if(textborder){
      cairo_set_source_rgba(c,0,0,0,.8);
      cairo_set_line_width(c,2);
      cairo_move_to (c, x,y);
      cairo_text_path (c, s->label[i]); 
      cairo_stroke(c);
    }

    cairo_set_source_rgba(c,textr,textg,textb,.8);
    cairo_move_to (c, x,y);
    cairo_show_text (c, s->label[i]); 
  }

  // Draw border 
  rounded_rectangle (c, tx+0.5, ty-0.5, tw-1, th+1, 1.5);
  parent_shade(s,c,7);
  cairo_set_line_width (c, 1.0);
  cairo_stroke (c);
	
  cairo_destroy(c);
}

void slider_realize(Slider *s){
  int w = total_slice_width(s);
  int h = total_slice_height(s);
  if(s->background == 0 || w != s->w || h != s->h){

    if(s->background)
      cairo_surface_destroy(s->background);

    if(s->foreground)
      cairo_surface_destroy(s->foreground);

    if(s->backdata)
      free(s->backdata);

    s->backdata = calloc(w*h,4);
    
    s->background = cairo_image_surface_create_for_data ((unsigned char *)s->backdata,
							 CAIRO_FORMAT_RGB24,
							 w,h,w*4);
    s->foreground = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						w,h);

    s->w=w;
    s->h=h;

    s->xpad=h*.45;
    if(s->xpad<4)s->xpad=4;
    slider_draw_background(s);    
    slider_draw(s);

  }
}

static double val_to_pixel(Slider *s,double v){
  int j;
  double ret=0;

  int tx=s->xpad;
  int tw=s->w - tx*2;
  
  if(v<s->label_vals[0]){
    ret=0;
  }else if(v>s->label_vals[s->labels-1]){
    ret=tw;
  }else{
    for(j=0;j<s->labels;j++){
      if(v>=s->label_vals[j] && v<=s->label_vals[j+1]){
        double del=(v-s->label_vals[j])/(s->label_vals[j+1]-s->label_vals[j]);
        double pixlo=rint((double)j/(s->labels-1)*tw);
        double pixhi=rint((double)(j+1)/(s->labels-1)*tw);
        ret=pixlo*(1.-del)+pixhi*del+tx;
        break;
      }
    }
  }
  return ret;
}

double slider_val_to_del(Slider *s,double v){
  if(isnan(v))return NAN;

  int j;
  double ret=0;
  
  for(j=0;j<s->labels;j++){
    if(v<=s->label_vals[j+1] || (j+1)==s->labels){
      double del=(v-s->label_vals[j])/(s->label_vals[j+1]-s->label_vals[j]);
      return (j+del)/(s->labels-1);
    }
  }

  return NAN;
}

void slider_draw(Slider *s){
  int i;
  cairo_t *c;
  //int w=s->w;
  int h=s->h;

  c = cairo_create(s->foreground);
  cairo_set_source_surface(c,s->background,0,0);
  cairo_rectangle(c,0,0,s->w,s->h);
  cairo_fill(c);

  // thumbs
  for(i=0;i<s->num_slices;i++){
    GtkWidget *sl = s->slices[i];
    double x = val_to_pixel(s,((Slice *)(s->slices[i]))->thumb_val)+.5;
    double rad = 2.;
    
    float y = rint(h/2)+.5;
    float xd = y*.575;
    float rx = 1.73*rad;
    cairo_pattern_t *pattern;

    if(SLICE(sl)->thumb_active){
      if ((s->num_slices == 1) || (s->num_slices == 3 && i==1)){
	// outline
	cairo_move_to(c,x,s->h/2);
	cairo_arc_negative(c, x+xd-rx, rad+.5, rad, 30.*(M_PI/180.), 270.*(M_PI/180.));
	cairo_arc_negative(c, x-xd+rx, rad+.5, rad, 270.*(M_PI/180.), 150.*(M_PI/180.));
	cairo_close_path(c);
	
	set_shade(sl,c,2);
	cairo_fill_preserve(c);

	cairo_set_line_width(c,1);
	set_shade(sl,c,7);
      
	if(((Slice *)s->slices[i])->thumb_focus)
	  cairo_set_source_rgba(c,0,0,0,1);

	cairo_stroke_preserve(c);
	cairo_set_dash (c, NULL, 0, 0.);

	// top highlight
	pattern = cairo_pattern_create_linear (0, 0, 0, 4);
	cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1., 1., 1., 0.2);	
	cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1., 1., 1., 0.);	
	cairo_set_source (c, pattern);
	cairo_fill_preserve (c);
	cairo_pattern_destroy (pattern);

	// Left highlight
	pattern = cairo_pattern_create_linear (x-xd+3, 0, x-xd+6, 0);
	cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1., 1., 1., 0.1);	
	cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1., 1., 1., 0.);	
	cairo_set_source (c, pattern);
	cairo_fill (c);
	cairo_pattern_destroy (pattern);

	// needle shadow
	cairo_set_line_width(c,2);
	cairo_move_to(c,x,s->h/2+3);
	cairo_line_to(c,x,s->ypad/2);
	cairo_set_source_rgba(c,0.,0.,0.,.5);
	cairo_stroke(c);

	// needle
	cairo_set_line_width(c,1);
	cairo_move_to(c,x,s->h/2+2);
	cairo_line_to(c,x,s->ypad/2);
	cairo_set_source_rgb(c,1.,1.,0);
	cairo_stroke(c);

      }else{
	if(i==0){
	  // bracket left
	
	  // outline
	  cairo_move_to(c,x,s->h/2);
	  cairo_line_to(c,x-xd/2,s->h/2);
	  cairo_arc_negative(c, x-xd*3/2+rx, h-rad-.5, rad, 210.*(M_PI/180.), 90.*(M_PI/180.));
	  cairo_line_to(c, x, h-.5);
	  cairo_close_path(c);
	
	  set_shade(sl,c,2);
	  cairo_set_line_width(c,1);
	  cairo_fill_preserve(c);
	  set_shade(sl,c,7);
	  if(((Slice *)s->slices[i])->thumb_focus)
	    cairo_set_source_rgba(c,0,0,0,1);
	  cairo_stroke_preserve(c);
	
	  // top highlight
	  pattern = cairo_pattern_create_linear (0, y, 0, y+4);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1., 1., 1., 0.2);	
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1., 1., 1., 0.);	
	  cairo_set_source (c, pattern);
	  cairo_fill_preserve (c);
	  cairo_pattern_destroy (pattern);
	
	  // Left highlight
	  pattern = cairo_pattern_create_linear (x-xd*3/2+3, 0, x-xd*3/2+6, 0);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1., 1., 1., 0.1);	
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1., 1., 1., 0.);	
	  cairo_set_source (c, pattern);
	  cairo_fill (c);
	  cairo_pattern_destroy (pattern);
	}else{

	  // bracket right
	
	  // outline
	  cairo_move_to(c,x,s->h/2);
	  cairo_line_to(c,x+xd/2,s->h/2);
	  cairo_arc(c, x+xd*3/2-rx, h-rad-.5, rad, 330.*(M_PI/180.), 90.*(M_PI/180.));
	  cairo_line_to(c, x, h-.5);
	  cairo_close_path(c);
	
	  set_shade(sl,c,2);
	  cairo_set_line_width(c,1);
	  cairo_fill_preserve(c);
	  set_shade(sl,c,7);
	  if(((Slice *)s->slices[i])->thumb_focus)
	    cairo_set_source_rgba(c,0,0,0,1);
	  cairo_stroke_preserve(c);
	
	  // top highlight
	  pattern = cairo_pattern_create_linear (0, y, 0, y+4);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1., 1., 1., 0.2);	
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1., 1., 1., 0.);	
	  cairo_set_source (c, pattern);
	  cairo_fill_preserve (c);
	  cairo_pattern_destroy (pattern);
	
	  // Left highlight
	  pattern = cairo_pattern_create_linear (x+1, 0, x+4, 0);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1., 1., 1., 0.1);	
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1., 1., 1., 0.);	
	  cairo_set_source (c, pattern);
	  cairo_fill (c);
	  cairo_pattern_destroy (pattern);

	}
	// needle shadow
	cairo_set_line_width(c,2);
	cairo_move_to(c,x,s->h/2-3);
	cairo_line_to(c,x,h-s->ypad/2);
	cairo_set_source_rgba(c,0.,0.,0.,.5);
	cairo_stroke(c);

	// needle
	cairo_set_line_width(c,1);
	cairo_move_to(c,x,s->h/2-2);
	cairo_line_to(c,x,h-s->ypad/2);
	cairo_set_source_rgb(c,1.,1.,0);
	cairo_stroke(c);
      }
    }
  }

  cairo_destroy(c);
}

void slider_expose_slice(Slider *s, int slicenum){
  Slice *slice = (Slice *)(s->slices[slicenum]);
  GtkWidget *w = GTK_WIDGET(slice);
  
  if(GTK_WIDGET_REALIZED(w)){
    cairo_t *c = gdk_cairo_create(w->window);

    slider_realize(s);
    cairo_set_source_surface(c,s->foreground,-slice_width(s,slicenum),0);
    cairo_rectangle(c,0,0,w->allocation.width,w->allocation.height);
    cairo_fill(c);
  
    cairo_destroy(c);
  }
}

void slider_expose(Slider *s){
  int i;
  for(i=0;i<s->num_slices;i++)
    slider_expose_slice(s,i);
}

void slider_size_request_slice(Slider *s,GtkRequisition *requisition){
  int maxx=0,x0=0,x1=0,maxy=0,i,w;
  
  // need a dummy surface to find text sizes 
  cairo_surface_t *dummy=cairo_image_surface_create(CAIRO_FORMAT_RGB24,1,1);
  cairo_t *c = cairo_create(dummy);
  
  // find the widest label
  for(i=0;i<s->labels;i++){
    cairo_text_extents_t ex;
    cairo_text_extents(c, s->label[i], &ex);
    if(i==0) x0 = ex.width;
    if(i==1) x1 = ex.width;
    if(ex.width > maxx)maxx=ex.width;
    if(ex.height > maxy)maxy=ex.height;
  }

  maxx*=1.5;
  // also check first + second label width
  if(x0+x1*1.2 > maxx)maxx=(x0+x1)*1.2;

  w = (maxx+2)*s->labels+4;
  requisition->width = (w+s->num_slices-1)/s->num_slices;
  requisition->height = maxy+4+s->ypad*2;

  cairo_destroy(c);
  cairo_surface_destroy(dummy);
}

static double slice_adjust_pixel(Slider *s,int slicenum, double x){
  double width = slice_width(s,slicenum);
  return x+width;
}

double slider_pixel_to_val(Slider *s,int slicenum,double x){
  int j;
  int tx=s->xpad;
  int tw=s->w - tx*2;

  x=slice_adjust_pixel(s,slicenum,x);

  for(j=0;j<s->labels-1;j++){

    double pixlo=rint((float)j/(s->labels-1)*(tw-1))+tx;
    double pixhi=rint((float)(j+1)/(s->labels-1)*(tw-1))+tx;

    if(x>=pixlo && x<=pixhi){
      if(pixlo==pixhi)return s->label_vals[j];
      double del=(float)(x-pixlo)/(pixhi-pixlo);
      return (1.-del)*s->label_vals[j] + del*s->label_vals[j+1];
    }
  }
  if(x<tx)
    return s->label_vals[0];
  else
    return (s->label_vals[s->labels-1]);
}

double slider_pixel_to_del(Slider *s,int slicenum,double x){
  int tx=s->xpad;
  int tw=s->w - tx*2;

  x=slice_adjust_pixel(s,slicenum,x-tx);

  if(x<=0){
    return 0.;
  }else if (x>tw){
    return 1.;
  }else
    return x/tw;
}

void slider_vals_bound(Slider *s,int slicenum){
  int i,flag=0;
  Slice *center = SLICE(s->slices[slicenum]);
  double min = s->label_vals[0];
  double max = s->label_vals[s->labels-1];
  if(center->thumb_val < min)
    center->thumb_val = min;

  if(center->thumb_val > max)
    center->thumb_val = max;

  // now make sure other sliders have valid spacing
  if( (s->flags & SLIDER_FLAG_INDEPENDENT_MIDDLE) &&
      s->num_slices == 3)
    flag=1;
  
  for(i=slicenum-1; i>=0;i--){
    if(flag && (i==0 || i==1))continue;

    Slice *sl = SLICE(s->slices[i]);
    Slice *sl2 = SLICE(s->slices[i+1]);
    if(sl->thumb_val>sl2->thumb_val)
      sl->thumb_val=sl2->thumb_val;
  }
  
  for(i=slicenum+1; i<s->num_slices;i++){
    if(flag && (i==2 || i==1))continue;

    Slice *sl = SLICE(s->slices[i]);
    Slice *sl2 = SLICE(s->slices[i-1]);
    if(sl->thumb_val<sl2->thumb_val)
      sl->thumb_val=sl2->thumb_val;
  }
}

static int determine_thumb(Slider *s,int slicenum,int x,int y){
  int i;
  int best=-1;
  float bestdist=s->w+1;

  x=slice_adjust_pixel(s,slicenum,x);
  for(i=0;i<s->num_slices;i++){
    Slice *sl = SLICE(s->slices[i]);
    if(sl->thumb_active){
      float tpix = val_to_pixel(s,sl->thumb_val) + i - s->num_slices/2;
      float d = fabs(x - tpix);
      if(d<bestdist){
	best=i;
	bestdist=d;
      }
    }
  }
  return best;
}

static int lit_thumb(Slider *s){
  int i;
  for(i=0;i<s->num_slices;i++){
    Slice *sl = SLICE(s->slices[i]);
    if(sl->thumb_state)return i;
  }
  return -1;
}

void slider_unlight(Slider *s){
  int i;
  for(i=0;i<s->num_slices;i++){
    Slice *sl = SLICE(s->slices[i]);
    if(!sl->thumb_grab)
      sl->thumb_state = 0;
  }
}

int slider_lightme(Slider *s,int slicenum,int x,int y){
  int last = lit_thumb(s);
  int best = determine_thumb(s,slicenum,x,y);
  if(last != best){
    slider_unlight(s);
    if(best>-1){
      Slice *sl = SLICE(s->slices[best]);
      sl->thumb_state=1;
    }
    return 1;
  }
  return 0;
}

void slider_button_press(Slider *s,int slicenum,int x,int y){
  int i;
  for(i=0;i<s->num_slices;i++){
    Slice *sl = SLICE(s->slices[i]);
    if(sl->thumb_state){
      sl->thumb_grab=1;
      sl->thumb_focus=1;
      gtk_widget_grab_focus(GTK_WIDGET(sl));

      slider_motion(s,slicenum,x,y);
    }
  }
  slider_draw(s);
  slider_expose(s);
}

void slider_button_release(Slider *s,int slicenum,int x,int y){
  int i;
  for(i=0;i<s->num_slices;i++){
    Slice *sl = SLICE(s->slices[i]);
    sl->thumb_grab=0;
  }
}

void slider_motion(Slider *s,int slicenum,int x,int y){
  int altered=0;
  int i;
  
  /* is a thumb already grabbed? */
  for(i=0;i<s->num_slices;i++){
    Slice *sl = SLICE(s->slices[i]);
    if(sl->thumb_grab){      
      sl->thumb_val=slider_pixel_to_val(s,slicenum,x);
      slider_vals_bound(s,i);
      altered=i+1;
    }
  }

  // did a gradient get altered?
  if(s->gradient && s->num_slices>=2){
    Slice *sl = SLICE(s->slices[0]);
    Slice *sh = SLICE(s->slices[s->num_slices-1]);
    if(s->gradient->low != sl->thumb_val ||
       s->gradient->high != sh->thumb_val){

      mapping_set_lo(s->gradient,slider_val_to_del(s,sl->thumb_val));
      mapping_set_hi(s->gradient,slider_val_to_del(s,sh->thumb_val));
      slider_draw_background(s);
    }
  }
  if(altered){
    Slice *sl = SLICE(s->slices[altered-1]);
    
    if(sl->callback)sl->callback(sl->callback_data);
    slider_draw(s);
    slider_expose(s);
  }else{
    /* nothing grabbed right now; determine if we're in a a thumb's area */
    if(slider_lightme(s,slicenum,x,y)){
      slider_draw(s);
      slider_expose(s);
    }
  }
}

gboolean slider_key_press(Slider *s,GdkEventKey *event){

  return FALSE; // keep processing
}

Slider *slider_new(Slice **slices, int num_slices, char **labels, double *label_vals, int num_labels,
		   unsigned flags){
  int i;
  Slider *ret = calloc(1,sizeof(*ret)); 

  ret->slices = (GtkWidget **)slices;
  ret->num_slices = num_slices;

  ret->label = calloc(num_labels,sizeof(*ret->label));
  for(i=0;i<num_labels;i++)
    ret->label[i]=strdup(labels[i]);

  ret->label_vals = calloc(num_labels,sizeof(*ret->label_vals));
  memcpy(ret->label_vals,label_vals,sizeof(*ret->label_vals)*num_labels);
  ret->labels = num_labels;

  // set up each slice
  for(i=0;i<num_slices;i++){
    slices[i]->slider = ret;
    slices[i]->slicenum = i;
  }
  ret->ypad=8;
  ret->xpad=5;
  //ret->minstep=minstep;
  //ret->step=step;

  ret->flags=flags;
  return ret;
}

void slider_set_gradient(Slider *s, mapping *m){
  s->gradient = m;
}

void slider_set_thumb_active(Slider *s, int thumbnum, int activep){
  slice_set_active(SLICE(s->slices[thumbnum]),activep);
}

double slider_get_value(Slider *s, int thumbnum){
  GtkWidget *w;
  if(thumbnum >= s->num_slices)return 0;
  if(thumbnum < 0)return 0;
  w = s->slices[thumbnum];
  return SLICE(w)->thumb_val;
}

void slider_set_value(Slider *s, int thumbnum, double v){
  GtkWidget *w;
  if(thumbnum >= s->num_slices)return;
  if(thumbnum < 0)return;
  w = s->slices[thumbnum];
  slice_thumb_set(SLICE(w),v);
}
