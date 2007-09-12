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

#define _GNU_SOURCE
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "internal.h"

static double val_to_pixel(_sv_slider_t *s, double val);

static int total_slice_width(_sv_slider_t *s){
  int i;
  int count=0;
  if(s->flip)
    for(i=0;i<s->num_slices;i++)
      count += s->slices[i]->allocation.height;
  else
    for(i=0;i<s->num_slices;i++)
      count += s->slices[i]->allocation.width;
  return count;
}

static int slice_width(_sv_slider_t *s,int slices){
  int i;
  int count=0;
  if(s->flip)
    for(i=0;i<slices;i++)
      count += s->slices[i]->allocation.height;
  else
    for(i=0;i<slices;i++)
      count += s->slices[i]->allocation.width;
  return count;
}

static int total_slice_height(_sv_slider_t *s){
  int i;
  int max=0;
  if(s->flip){
    for(i=0;i<s->num_slices;i++)
      if(max<s->slices[i]->allocation.width)
	max = s->slices[i]->allocation.width;
  }else{
    for(i=0;i<s->num_slices;i++)
      if(max<s->slices[i]->allocation.height)
	max = s->slices[i]->allocation.height;
  }
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

static double shades[] = {1.15, 0.95, 0.896, 0.82, 0.7, 0.665, 0.5, 0.45, 0.4};

static void bg_set(GtkWidget *w, cairo_t *c){
  _sv_slice_t *sl = SLICE(w);
  GdkColor *bg = &w->style->bg[sl->thumb_state?GTK_STATE_ACTIVE:GTK_STATE_NORMAL];
  double shade_r=bg->red/65535.;
  double shade_g=bg->green/65535.;
  double shade_b=bg->blue/65535.;

  cairo_set_source_rgb (c, shade_r,shade_g,shade_b);
}

static void fg_shade(GtkWidget *w, cairo_t *c, int shade){
  _sv_slice_t *sl = SLICE(w);
  GdkColor *fg = &w->style->fg[sl->thumb_state?GTK_STATE_ACTIVE:GTK_STATE_NORMAL];
  double shade_r=fg->red*shades[shade]/65535;
  double shade_g=fg->green*shades[shade]/65535;
  double shade_b=fg->blue*shades[shade]/65535;

  cairo_set_source_rgb (c, shade_r,shade_g,shade_b);
}

static void parent_shade(_sv_slider_t *s, cairo_t *c, int shade){
  GtkWidget *parent=gtk_widget_get_parent(s->slices[0]);
  GdkColor *bg = &parent->style->bg[GTK_STATE_NORMAL];
  double shade_r=bg->red*shades[shade]/65535;
  double shade_g=bg->green*shades[shade]/65535;
  double shade_b=bg->blue*shades[shade]/65535;

  cairo_set_source_rgb (c, shade_r,shade_g,shade_b);
}

static void _sv_slider_draw_background(_sv_slider_t *s){
  if(!s->realized)return;
  
  int i;
  GtkWidget *parent=gtk_widget_get_parent(s->slices[0]);
  GdkColor *text = &s->slices[0]->style->text[0];
  GdkColor *bg = &parent->style->bg[0];
  int textborder=1;
  double textr=text->red;
  double textg=text->green;
  double textb=text->blue;

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

  cairo_rectangle (c, x+1, ty, w-2, th);
  bg_set(s->slices[0],c);
  cairo_fill (c);
  cairo_surface_flush(s->background);

  // Create trough innards
  if(s->gradient){
    // background map gradient 
    // this happens 'under' cairo
    u_int32_t *pixel=s->backdata+ty*s->w;
    
    for(i=tx;i<tx+tw;i++)
      pixel[i]=_sv_mapping_calc(s->gradient,_sv_slider_pixel_to_mapdel(s,i), pixel[i]);
    
    for(i=ty+1;i<ty+th;i++){
      memcpy(pixel+w,pixel,w*4);
      pixel+=s->w;
    }
  }else{
    // normal background
    textborder=0;
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
    int x=val_to_pixel(s,s->label_vals[i])+.5;
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
      cairo_set_source_rgba(c,1.,1.,1.,.5);
      cairo_set_line_width(c,2.5);
      cairo_move_to (c, x,y);
      cairo_text_path (c, s->label[i]); 
      cairo_stroke(c);
    }

    cairo_set_source_rgba(c,textr,textg,textb,1.);
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

void _sv_slider_draw(_sv_slider_t *s){
  if(!s->realized)return;

  int i;
  cairo_t *c;
  //int w=s->w;
  int h=s->h;
  int w=s->w;

  c = cairo_create(s->foreground);

  if(s->flip){
    cairo_matrix_t m = {0.,-1., 1.,0.,  0.,w};
    cairo_set_matrix(c,&m);
  }

  cairo_set_source_surface(c,s->background,0,0);
  cairo_rectangle(c,0,0,w,h);
  cairo_fill(c);

  // thumbs
  for(i=0;i<s->num_slices;i++){
    GtkWidget *sl = s->slices[i];
    double x = rint(val_to_pixel(s,((_sv_slice_t *)(s->slices[i]))->thumb_val))+.5;

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
	
	fg_shade(sl,c,2);
	cairo_fill_preserve(c);

	cairo_set_line_width(c,1);
	fg_shade(sl,c,7);
      
	if(((_sv_slice_t *)s->slices[i])->thumb_focus)
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
	
	  fg_shade(sl,c,2);
	  cairo_set_line_width(c,1);
	  cairo_fill_preserve(c);
	  fg_shade(sl,c,7);
	  if(((_sv_slice_t *)s->slices[i])->thumb_focus)
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
	
	  fg_shade(sl,c,2);
	  cairo_set_line_width(c,1);
	  cairo_fill_preserve(c);
	  fg_shade(sl,c,7);
	  if(((_sv_slice_t *)s->slices[i])->thumb_focus)
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

void _sv_slider_realize(_sv_slider_t *s){
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
    if(s->flip){
      s->foreground = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						  h,w);
    }else{
      s->foreground = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						  w,h);
    }

    s->w=w;
    s->h=h;

    s->xpad=h*.45;
    if(s->xpad<4)s->xpad=4;
    s->realized = 1;
    _sv_slider_draw_background(s);    
    _sv_slider_draw(s);

  }
  s->realized = 1;
}

static double val_to_pixel(_sv_slider_t *s,double v){
  int j;
  double ret=0;
  double neg = (s->neg? -1.: 1.);
  int tx=s->xpad;
  int tw=s->w - tx*2;

  v*=neg;

  if( v<s->label_vals[0]*neg){
    ret=0;
  }else if(v>s->label_vals[s->labels-1]*neg){
    ret=tw;
  }else{
    for(j=0;j<s->labels;j++){
      if(v>=s->label_vals[j]*neg && v<=s->label_vals[j+1]*neg){
	v*=neg;
	double del=(v-s->label_vals[j])/(s->label_vals[j+1]-s->label_vals[j]);
	double pixlo=rint((double)(j)/(s->labels-1)*tw);
	double pixhi=rint((double)(j+1)/(s->labels-1)*tw);
	ret=pixlo*(1.-del)+pixhi*del+tx;
	break;
      }
    }
  }

  return ret;
}

double _sv_slider_val_to_del(_sv_slider_t *s,double v){
  if(isnan(v))return NAN;
  int j=s->labels-1;

  if(s->neg){
    while(--j)
      if(v<=s->label_vals[j])break;
  }else{
    while(--j)
      if(v>s->label_vals[j])break;
  }
  return (j + (v-s->label_vals[j])*s->labeldel[j])*s->labelinv;
}


double _sv_slider_val_to_mapdel(_sv_slider_t *s,double v){
  int j=s->labels-1;
  if(isnan(v))return NAN;
  
  if(s->neg){

    if(v > s->al)return NAN;
    if(v >= s->lo)return 0.;
    if(v <= s->hi)return 1.;
    while(--j)
      if(v<=s->label_vals[j])break;

  }else{

    if(v < s->al)return NAN;
    if(v <= s->lo)return 0.;
    if(v >= s->hi)return 1.;
    while(--j)
      if(v>s->label_vals[j])break;

  }

  return v*s->labeldelB[j] + s->labelvalB[j];

  //labeldelB[j] = labeldel[j] * idelrange;
  //labelvalB[j] = (j-label_vals[j]*s->labeldel[j]-s->lodel)*s->idelrange;



  //return (j + (v-s->label_vals[j])*s->labeldel[j] - s->lodel) * s->idelrange;
}

void _sv_slider_expose_slice(_sv_slider_t *s, int slicenum){
  _sv_slice_t *slice = (_sv_slice_t *)(s->slices[slicenum]);
  GtkWidget *w = GTK_WIDGET(slice);
  
  if(GTK_WIDGET_REALIZED(w)){
    cairo_t *c = gdk_cairo_create(w->window);

    _sv_slider_realize(s);
    if(s->flip){
      cairo_set_source_surface(c,s->foreground,0, slice_width(s,slicenum+1)-total_slice_width(s));
    }else{
      cairo_set_source_surface(c,s->foreground,-slice_width(s,slicenum),0);
    }
    cairo_rectangle(c,0,0,w->allocation.width,w->allocation.height);
    cairo_fill(c);
  
    cairo_destroy(c);
  }
}

void _sv_slider_expose(_sv_slider_t *s){
  int i;
  for(i=0;i<s->num_slices;i++)
    _sv_slider_expose_slice(s,i);
}

void _sv_slider_size_request_slice(_sv_slider_t *s,GtkRequisition *requisition){
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
  if(w<200)w=200;

  if(s->flip){
    requisition->height = (w+s->num_slices-1)/s->num_slices;
    requisition->width = maxy+4+s->ypad*2;
  }else{
    requisition->width = (w+s->num_slices-1)/s->num_slices;
    requisition->height = maxy+4+s->ypad*2;
  }
  cairo_destroy(c);
  cairo_surface_destroy(dummy);
}

static double slice_adjust_pixel(_sv_slider_t *s,int slicenum, double x){
  double width = slice_width(s,slicenum);
  return x+width;
}

static double quant(_sv_slider_t *s, double val){
  if(s->quant_denom!=0.){
    val *= s->quant_denom;
    val /= s->quant_num;
    
    val = rint(val);
    
    val *= s->quant_num;
    val /= s->quant_denom;
  }
  return val;
}

double _sv_slider_pixel_to_val(_sv_slider_t *s,double x){
  int tx=s->xpad;
  int tw=s->w - tx*2;
  double del = (double)(x-tx)/tw;
  if(del<0)
    return quant(s,s->label_vals[0]);
  if(del>=1.)
    return quant(s,(s->label_vals[s->labels-1]));
  return _sv_slider_del_to_val(s,del);
}

double _sv_slider_pixel_to_del(_sv_slider_t *s,double x){
  int tx=s->xpad;
  int tw=s->w - tx*2;
  x-=tx;

  if(x<=0){
    return 0.;
  }else if (x>tw){
    return 1.;
  }else
    return x/tw;
}

double _sv_slider_pixel_to_mapdel(_sv_slider_t *s,double x){
  int tx=s->xpad;
  int tw=s->w - tx*2;
  x = ((x-tx)/tw - s->lodel)*s->idelrange;

  if (x<=0.) return 0.;
  if (x>=1.) return 1.;
  return x;
}

double _sv_slider_del_to_val(_sv_slider_t *s, double del){
  int base;
  if(isnan(del))return del;

  del *= (s->labels-1);
  base = floor(del);
  del -= base;
  
  return quant(s,( (1.-del)*s->label_vals[base] + del*s->label_vals[base+1] ));
}

void _sv_slider_vals_bound(_sv_slider_t *s,int slicenum){
  int i,flag=-1;
  _sv_slice_t *center = SLICE(s->slices[slicenum]);
  double min = (s->neg ? s->label_vals[s->labels-1] : s->label_vals[0]);
  double max = (s->neg ? s->label_vals[0] : s->label_vals[s->labels-1]);
  int flip = (s->neg? 1: 0);

  if(center->thumb_val < min)
    center->thumb_val = min;

  if(center->thumb_val > max)
    center->thumb_val = max;

  // now make sure other sliders have valid spacing
  if( (s->flags & _SV_SLIDER_FLAG_INDEPENDENT_MIDDLE) &&
      s->num_slices == 3)
    flag=1;
  
  for(i=slicenum-1; i>=0;i--){
    int i2 = i+1;
    if(i==flag)continue;
    if(i2 == flag)i2++;
    if(i2>=s->num_slices)continue;

    _sv_slice_t *sl = SLICE(s->slices[i]);
    _sv_slice_t *sl2 = SLICE(s->slices[i2]);
    if((sl->thumb_val>sl2->thumb_val)^flip)
      _sv_slice_thumb_set(sl,sl2->thumb_val);
  }
  
  for(i=slicenum+1; i<s->num_slices;i++){
    int i2 = i-1;
    if(i==flag)continue;
    if(i2 == flag)i2--;
    if(i2<0)continue;
  
    _sv_slice_t *sl = SLICE(s->slices[i]);
    _sv_slice_t *sl2 = SLICE(s->slices[i2]);
    if((sl->thumb_val<sl2->thumb_val)^flip)
      _sv_slice_thumb_set(sl,sl2->thumb_val);

  }
}

static int determine_thumb(_sv_slider_t *s,int slicenum,int x,int y){
  int i;
  int best=-1;
  float bestdist=s->w+1;
  int n = s->num_slices;

  if(s->flip){
    _sv_slice_t *sl = SLICE(s->slices[slicenum]);
    int temp = x;
    x = sl->widget.allocation.height - y -1;
    y = temp;
  }

  x=slice_adjust_pixel(s,slicenum,x);
  for(i=0;i<n;i++){
    _sv_slice_t *sl = SLICE(s->slices[i]);
    if(sl->thumb_active){
      float tx = val_to_pixel(s,sl->thumb_val) + i - s->num_slices/2;
      float ty = ((n==3 && i==1) ? 0:s->h);
      float d = hypot (x-tx,y-ty);
      if(d<bestdist){
	best=i;
	bestdist=d;
      }
    }
  }
  return best;
}

static int lit_thumb(_sv_slider_t *s){
  int i;
  for(i=0;i<s->num_slices;i++){
    _sv_slice_t *sl = SLICE(s->slices[i]);
    if(sl->thumb_state)return i;
  }
  return -1;
}

void _sv_slider_unlight(_sv_slider_t *s){
  int i;
  for(i=0;i<s->num_slices;i++){
    _sv_slice_t *sl = SLICE(s->slices[i]);
    if(!sl->thumb_grab)
      sl->thumb_state = 0;
  }
}

int _sv_slider_lightme(_sv_slider_t *s,int slicenum,int x,int y){
  int last = lit_thumb(s);
  int best = determine_thumb(s,slicenum,x,y);
  if(last != best){
    _sv_slider_unlight(s);
    if(best>-1){
      _sv_slice_t *sl = SLICE(s->slices[best]);
      sl->thumb_state=1;
    }
    return 1;
  }
  return 0;
}

void _sv_slider_button_press(_sv_slider_t *s,int slicenum,int x,int y){
  int i;
  for(i=0;i<s->num_slices;i++){
    _sv_slice_t *sl = SLICE(s->slices[i]);
    if(sl->thumb_state){
      sl->thumb_grab=1;
      sl->thumb_focus=1;
      gtk_widget_grab_focus(GTK_WIDGET(sl));

      if(sl->callback)sl->callback(sl->callback_data,0);
      _sv_slider_motion(s,slicenum,x,y);
    }else{
      sl->thumb_grab=0;
      sl->thumb_focus=0;
    }
  }
  _sv_slider_draw(s);
  _sv_slider_expose(s);
}

void _sv_slider_button_release(_sv_slider_t *s,int slicenum,int x,int y){
  int i;
  for(i=0;i<s->num_slices;i++){
    _sv_slice_t *sl = SLICE(s->slices[i]);

    if(sl->thumb_grab){
      sl->thumb_grab=0;
      if(sl->callback)sl->callback(sl->callback_data,2);
    }
  }
}

static void update_gradient(_sv_slider_t *s){
  if(s->gradient && s->num_slices==3){
    _sv_slice_t *sl = SLICE(s->slices[0]);
    _sv_slice_t *sa = SLICE(s->slices[1]);
    _sv_slice_t *sh = SLICE(s->slices[2]);
    double ldel = _sv_slider_val_to_del(s,sl->thumb_val);
    double hdel = _sv_slider_val_to_del(s,sh->thumb_val);
    
    s->al = sa->thumb_val;

    if(s->gradient->low != ldel ||
       s->gradient->high != hdel){
      int j;

      s->lo = sl->thumb_val;
      s->hi = sh->thumb_val;
      s->idelrange = 1./(hdel-ldel);
      s->lodel = ldel;

      for(j=0;j<s->labels-1;j++){
	s->labeldelB[j] = s->labeldel[j] * s->idelrange * s->labelinv;
	s->labelvalB[j] = (j-s->label_vals[j]*s->labeldel[j]-
			   s->lodel*(s->labels-1))*s->idelrange*s->labelinv;
      }

      _sv_mapping_set_lo(s->gradient,ldel);
      _sv_mapping_set_hi(s->gradient,hdel);
      _sv_slider_draw_background(s);
      _sv_slider_draw(s);
    }
    _sv_slider_expose(s);
  }
}

void _sv_slider_motion(_sv_slider_t *s,int slicenum,int x,int y){
  double altered[s->num_slices];
  int i, grabflag=0;
  _sv_slice_t *sl = SLICE(s->slices[slicenum]);
  int px = (s->flip?sl->widget.allocation.height-y-1 : x);

  for(i=0;i<s->num_slices;i++){
    sl = SLICE(s->slices[i]);
    altered[i] = sl->thumb_val;
  }

  /* is a thumb already grabbed? */
  for(i=0;i<s->num_slices;i++){
    sl = SLICE(s->slices[i]);
    if(sl->thumb_grab){      
      sl->thumb_val=
	_sv_slider_pixel_to_val(s,slice_adjust_pixel(s,slicenum,px));
      _sv_slider_vals_bound(s,i);
      grabflag = 1;
    }
  }

  // did a gradient get altered?
  update_gradient(s);

  if(grabflag){
    _sv_slider_draw(s);
    _sv_slider_expose(s);

    // call slice callbacks on all slices that were altered; value
    // bounding might have affected slices other than the grabbed one.

    for(i=0;i<s->num_slices;i++){
      _sv_slice_t *sl = SLICE(s->slices[i]);
    
      if(sl->thumb_val != altered[i])
	if(sl->callback)sl->callback(sl->callback_data,1);
    }

  }else{
    /* nothing grabbed right now; determine if we're in a thumb's area */
    if(_sv_slider_lightme(s,slicenum,x,y)){
      _sv_slider_draw(s);
      _sv_slider_expose(s);
    }
  }
}

gboolean _sv_slider_key_press(_sv_slider_t *s,GdkEventKey *event,int slicenum){
  _sv_slice_t *sl = (_sv_slice_t *)(s->slices[slicenum]);
  int shift = (event->state&GDK_SHIFT_MASK);
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK)return FALSE;
  
  /* non-control keypresses */
  switch(event->keyval){
  case GDK_Left:
    {
      double x = val_to_pixel(s,sl->thumb_val);
      while(sl->thumb_val > s->label_vals[0] &&
	    sl->thumb_val == _sv_slider_pixel_to_val(s,x))x--;
      if(shift)
	x-=9;
      sl->thumb_val=_sv_slider_pixel_to_val(s,x);
      _sv_slider_vals_bound(s,slicenum);
      // did a gradient get altered?
      update_gradient(s);
      
      if(sl->callback){
	sl->callback(sl->callback_data,0);
	sl->callback(sl->callback_data,1);
	sl->callback(sl->callback_data,2);
      }
      _sv_slider_draw(s);
      _sv_slider_expose(s);

    }
    return TRUE;

  case GDK_Right:
    {
      double x = val_to_pixel(s,sl->thumb_val);
      while(sl->thumb_val < s->label_vals[s->labels-1] &&
	    sl->thumb_val == _sv_slider_pixel_to_val(s,x))x++;
      if(shift)
	x+=9;
      sl->thumb_val=_sv_slider_pixel_to_val(s,x);
      _sv_slider_vals_bound(s,slicenum);
      // did a gradient get altered?
      update_gradient(s);
      
      if(sl->callback){
	sl->callback(sl->callback_data,0);
	sl->callback(sl->callback_data,1);
	sl->callback(sl->callback_data,2);
      }
      _sv_slider_draw(s);
      _sv_slider_expose(s);
    }
    return TRUE;
  }

  return FALSE; // keep processing
}

_sv_slider_t *_sv_slider_new(_sv_slice_t **slices, int num_slices, char **labels, double *label_vals, int num_labels,
			     unsigned flags){
  int i;
  _sv_slider_t *ret = calloc(1,sizeof(*ret)); 

  ret->slices = (GtkWidget **)slices;
  ret->num_slices = num_slices;
  ret->quant_num=0.;
  ret->quant_denom=0.;

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

  if(label_vals[0]>label_vals[1])
    ret->neg = 1;

  ret->flags=flags;
  if(flags & _SV_SLIDER_FLAG_VERTICAL) ret->flip = 1;

  ret->labelinv = 1./(ret->labels-1);
  ret->labeldel = calloc(ret->labels-1,sizeof(*ret->labeldel));
  for(i=0;i<ret->labels-1;i++)
    ret->labeldel[i] = 1./(ret->label_vals[i+1]-ret->label_vals[i]);
  ret->lo = ret->label_vals[0];
  ret->hi = ret->label_vals[ret->labels-1];
  ret->lodel = 0.;
  ret->idelrange = 1.;

  ret->labeldelB = calloc(ret->labels-1,sizeof(*ret->labeldelB));
  ret->labelvalB = calloc(ret->labels-1,sizeof(*ret->labelvalB));
  for(i=0;i<ret->labels-1;i++){
    ret->labeldelB[i] = ret->labeldel[i] * ret->labelinv;
    ret->labelvalB[i] = (i-ret->label_vals[i]*ret->labeldel[i])*ret->labelinv;
  }

  return ret;
}

void _sv_slider_set_gradient(_sv_slider_t *s, _sv_mapping_t *m){
  s->gradient = m;
  if(s->realized){
    _sv_slider_draw_background(s);
    _sv_slider_draw(s);
    _sv_slider_expose(s);
  }
}

void _sv_slider_set_thumb_active(_sv_slider_t *s, int thumbnum, int activep){
  _sv_slice_set_active(SLICE(s->slices[thumbnum]),activep);
}

double _sv_slider_get_value(_sv_slider_t *s, int thumbnum){
  GtkWidget *w;
  if(thumbnum >= s->num_slices)return 0;
  if(thumbnum < 0)return 0;
  w = s->slices[thumbnum];
  return SLICE(w)->thumb_val;
}

void _sv_slider_set_value(_sv_slider_t *s, int thumbnum, double v){
  GtkWidget *w;
  if(thumbnum >= s->num_slices)return;
  if(thumbnum < 0)return;
  w = s->slices[thumbnum];
  _sv_slice_thumb_set(SLICE(w),v);
  update_gradient(s);
}

void _sv_slider_set_quant(_sv_slider_t *s,double num, double denom){
  s->quant_num=num;
  s->quant_denom=denom;
}

double _sv_slider_print_height(_sv_slider_t *s){
  return (s->slices[0]->allocation.height - s->ypad*2)*1.2;
}
 
void _sv_slider_print(_sv_slider_t *s, cairo_t *c, int w, int h){
  cairo_save(c);
  double ypad = h*.1;
  double neg = (s->neg? -1.: 1.);

  // set clip region
  cairo_rectangle(c,0,ypad,w,h-ypad*2);
  cairo_clip(c);

  // determine start/end deltas
  // eliminate label sections that are completely unused
  int slices = s->num_slices;
  double lo = (slices>0?SLICE(s->slices[0])->thumb_val:s->label_vals[0]) * neg;
  double hi = (slices>0?SLICE(s->slices[slices-1])->thumb_val:s->label_vals[s->labels-1]) * neg;

  // alpha could push up the unused area
  if(slices==3 && SLICE(s->slices[1])->thumb_val*neg>lo)
    lo = SLICE(s->slices[1])->thumb_val*neg;

  // if lo>hi (due to alpha), show the whole scale empty
  if(lo>hi){
    lo = s->label_vals[0]*neg;
    hi = s->label_vals[s->labels-1]*neg;
  }

  int firstlabel=0;
  int lastlabel=s->labels-1;
  int i;

  for(i=s->labels-2;i>0;i--)
    if(lo>s->label_vals[i]*neg){
      firstlabel=i;
      break;
    }
  
  for(i=1;i<s->labels-1;i++)
    if(hi<s->label_vals[i]*neg){
      lastlabel=i;
      break;
    }
      
  double lodel = 1. / (s->labels-1) * firstlabel;
  double hidel = 1. / (s->labels-1) * lastlabel;
  double alphadel = (slices==3 ? 
		     _sv_slider_val_to_del(s,SLICE(s->slices[1])->thumb_val):0.);

  // create background image
  {
    cairo_surface_t *image = cairo_image_surface_create(CAIRO_FORMAT_RGB24,w,h);
    cairo_t *ci = cairo_create(image);
    int x,y;

    cairo_save(c);
    cairo_set_source_rgb (ci, .5,.5,.5);
    cairo_paint(ci);
    cairo_set_source_rgb (ci, .314,.314,.314);
    for(y=0;y<=h/2;y+=8){
      int phase = (y>>3)&1;
      for(x=0;x<w;x+=8){
	if(phase)
	  cairo_rectangle(ci,x,y+h/2.,8.,8.);
	else
	  cairo_rectangle(ci,x,h/2.-y-8,8.,8.);
	cairo_fill(ci);
	phase=!phase;
      }
    }

    for(y=0;y<h;y++){
      _sv_ucolor_t *line = (_sv_ucolor_t *)cairo_image_surface_get_data(image) + w*y;
      for(x=0;x<w;x++){
	double del = (hidel - lodel) / (w-1) * x + lodel;
	if(del>=alphadel)
	  line[x].u = _sv_mapping_calc(s->gradient, del, line[x].u);
      }
    }
    
    // composite background with correct resample filter
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(image);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
    cairo_set_source(c,pattern);
    cairo_rectangle(c,0,0,w,h);
    cairo_fill(c);

    cairo_destroy(ci);
    cairo_pattern_destroy(pattern);
    cairo_surface_destroy(image);
    cairo_restore(c);
  }

  // add labels
  cairo_set_font_size(c,h-ypad*2-3);
  for(i=firstlabel;i<=lastlabel;i++){
    double x = (double)(i-firstlabel) / (lastlabel - firstlabel) * (w-1);
    double y;
    cairo_text_extents_t ex;
    
    cairo_move_to(c,x+.5,ypad);
    cairo_line_to(c,x+.5,h-ypad);
    cairo_set_source_rgba(c,0,0,0,.8);
    cairo_set_line_width(c,1);
    cairo_stroke(c);

    cairo_text_extents (c, s->label[i], &ex);
    if(i>firstlabel){
   
      x-=2;
      x-=ex.width;
 
    }else{
      x+=2;
    }
    y = h/2. - ex.y_bearing/2.;

    cairo_set_source_rgba(c,1.,1.,1.,.5);
    cairo_set_line_width(c,2.5);
    cairo_move_to (c, x,y);
    cairo_text_path (c, s->label[i]); 
    cairo_stroke(c);

    cairo_set_source_rgba(c,0,0,0,1.);
    cairo_move_to (c, x,y);
    cairo_show_text (c, s->label[i]); 
  }
 
  cairo_restore(c);
}
