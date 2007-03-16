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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include "internal.h"

#define LINETYPES 6
static propmap *line_name[LINETYPES+1] = {
  &(propmap){"line", 0,          NULL,NULL,NULL},
  &(propmap){"fat line", 1,      NULL,NULL,NULL},
  &(propmap){"no line", 5,       NULL,NULL,NULL},
  NULL
};

#define POINTTYPES 9
static propmap *point_name[POINTTYPES+1] = {
  &(propmap){"dot", 0,             NULL,NULL,NULL},
  &(propmap){"cross", 1,           NULL,NULL,NULL},
  &(propmap){"plus", 2,            NULL,NULL,NULL},
  &(propmap){"open circle", 3,     NULL,NULL,NULL},
  &(propmap){"open square", 4,     NULL,NULL,NULL},
  &(propmap){"open triangle", 5,   NULL,NULL,NULL},
  &(propmap){"solid circle", 6,    NULL,NULL,NULL},
  &(propmap){"solid square", 7,    NULL,NULL,NULL},
  &(propmap){"solid triangle", 8,  NULL,NULL,NULL},
  NULL
};

static void render_checks(cairo_t *c, int w, int h){
  /* default checked background */
  /* 16x16 'mid-checks' */ 
  int x,y;

  cairo_set_source_rgb (c, .5,.5,.5);
  cairo_paint(c);
  cairo_set_source_rgb (c, .314,.314,.314);

  for(y=0;y<h;y+=16){
    int phase = (y>>4)&1;
    for(x=0;x<w;x+=16){
      if(phase){
	cairo_rectangle(c,x,y,16.,16.);
	cairo_fill(c);
      }
      phase=!phase;
    }
  }
}

// called internally, assumes we hold lock
// redraws the data, does not compute the data
static int _sushiv_panelxy_remap(sushiv_panel_t *p, cairo_t *c){
  sushiv_panelxy_t *xy = p->subtype->pxy;
  Plot *plot = PLOT(p->private->graph);

  int plot_serialno = p->private->plot_serialno;
  int map_serialno = p->private->map_serialno;
  int xi,i,j;
  int pw = plot->x.pixels;
  int ph = plot->y.pixels;

  scalespace sx = p1->x;
  scalespace sy = p1->y;
  scalespace data_v = p1->data_v;
  scalespace px = plot->x;
  scalespace py = plot->y;
    
  /* do the panel and plot scales match?  If not, redraw the plot
     scales */
  
  if(memcmp(&sx,&px,sizeof(sx)) ||
     memcmp(&sy,&py,sizeof(sy))){

    plot->x = sx;
    plot->y = sy;
    
    gdk_threads_leave();
    plot_draw_scales(plot);
  }else
    gdk_threads_leave();

  /* blank frame to selected bg */
  switch(p->private->bg_type){
  case SUSHIV_BG_WHITE:
    cairo_set_source_rgb (c, 1.,1.,1.);
    cairo_paint(c);
    break;
  case SUSHIV_BG_BLACK:
    cairo_set_source_rgb (c, 0,0,0);
    cairo_paint(c);
    break;
  case SUSHIV_BG_CHECKS:
    render_checks(c,pw,ph);
    break;
  }

  gdk_threads_enter();
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno) return -1;

  if(xy->data_head){
    
    /* by objective */
    for(j=0;j<p->objectives;j++){
      if(xy->x_vec[j] && xy->y_vec[j] && !mapping_inactive_p(xy->mappings+j)){
	
	int dw = xy->data_v.pixels;
	double alpha = slider_get_value(xy->alpha_scale[j],0);
	int linetype = xy->linetype[j];
	int pointtype = xy->pointtype[j];
	u_int32_t color = mapping_calc(xy->mappings+j,1.,0);
      
	double xv[dw];
	double yv[dw];
	
	// copy the list data over
	memcpy(xv,xy->x_vec[j],sizeof(xv));
	memcpy(yv,xy->y_vec[j],sizeof(yv));
	gdk_threads_leave();

	/* by x */
	for(xi=0;xi<dw;xi++){
	  double xpixel = xv[xi];
	  double ypixel = yv[xi];

	  /* map data vector bin to x pixel location in the plot */
	  if(!isnan(xpixel))
	    xpixel = scalespace_pixel(&sx,xpixel)+.5;
	  
	  if(!isnan(ypixel))
	    ypixel = scalespace_pixel(&sy,ypixel)+.5;
	  
	  xv[xi] = xpixel;
	  yv[xi] = ypixel;
	}
	
	/* draw lines, if any */
	if(linetype != 5){
	  cairo_set_source_rgba(c,
				((color>>16)&0xff)/255.,
				((color>>8)&0xff)/255.,
				((color)&0xff)/255.,
				alpha);
	  if(linetype == 1)
	    cairo_set_line_width(c,2.);
	  else
	    cairo_set_line_width(c,1.);
	  
	  for(i=1;i<dw;i++){
	    
	    if(!isnan(yv[i-1]) && !isnan(yv[i]) &&
	       !isnan(xv[i-1]) && !isnan(xv[i]) &&
	       !(xv[i-1] < 0 && xv[i] < 0) &&
	       !(yv[i-1] < 0 && yv[i] < 0) &&
	       !(xv[i-1] > pw && xv[i] > pw) &&
	       !(yv[i-1] > ph && yv[i] > ph)){
	      
	      cairo_move_to(c,xv[i-1],yv[i-1]);
	      cairo_line_to(c,xv[i],yv[i]);
	      cairo_stroke(c);
	    }	      
	  }
	}

	/* now draw the points */
	if(pointtype > 0 || linetype == 5){
	  cairo_set_line_width(c,1.);

	  for(i=0;i<dw;i++){
	    if(!isnan(yv[i]) && 
	       !isnan(xv[i]) && 
	       !xv[i]<-10 &&
	       !yv[i]<-10 &&
	       !xv[i]-10>pw &&
	       !yv[i]-10>ph){

	      double xx,yy;
	      xx = xv[i];
	      yy = yv[i];

	      cairo_set_source_rgba(c,
				    ((color>>16)&0xff)/255.,
				    ((color>>8)&0xff)/255.,
				    ((color)&0xff)/255.,
				    alpha);
	      
	      switch(pointtype){
	      case 0: /* pixeldots */
		cairo_rectangle(c, xx-.5,yy-.5,1,1);
		cairo_fill(c);
		break;
	      case 1: /* X */
		cairo_move_to(c,xx-4,yy-4);
		cairo_line_to(c,xx+4,yy+4);
		cairo_move_to(c,xx+4,yy-4);
		cairo_line_to(c,xx-4,yy+4);
		break;
	      case 2: /* + */
		cairo_move_to(c,xx-4,yy);
		cairo_line_to(c,xx+4,yy);
		cairo_move_to(c,xx,yy-4);
		cairo_line_to(c,xx,yy+4);
		break;
	      case 3: case 6: /* circle */
		cairo_arc(c,xx,yy,4,0,2.*M_PI);
		break;
	      case 4: case 7: /* square */
		cairo_rectangle(c,xx-4,yy-4,8,8);
		break;
	      case 5: case 8: /* triangle */
		cairo_move_to(c,xx,yy-5);
		cairo_line_to(c,xx-4,yy+3);
		cairo_line_to(c,xx+4,yy+3);
		cairo_close_path(c);
		break;
	      }

	      if(pointtype>5){
		cairo_fill_preserve(c);
	      }

	      if(pointtype>0){
		if(p->private->bg_type == SUSHIV_BG_WHITE)
		  cairo_set_source_rgba(c,0.,0.,0.,alpha);
		else
		  cairo_set_source_rgba(c,1.,1.,1.,alpha);
		cairo_stroke(c);
	      }
	    }
	  }
	}
	
	gdk_threads_enter();
	if(plot_serialno != p->private->plot_serialno ||
	   map_serialno != p->private->map_serialno) return -1;
	
	
      }
    }
  }

  return 1;
}

static void sushiv_panelxy_print(sushiv_panel_t *p, cairo_t *c, int w, int h){
  Plot *plot = PLOT(p->private->graph);
  double pw = p->private->graph->allocation.width;
  double ph = p->private->graph->allocation.height;
  double scale;

  if(w/pw < h/ph)
    scale = w/pw;
  else
    scale = h/ph;

  cairo_matrix_t m;
  cairo_get_matrix(c,&m);
  cairo_matrix_scale(&m,scale,scale);
  cairo_set_matrix(c,&m);

  plot_print(plot, c, ph*scale, (void(*)(void *, cairo_t *))_sushiv_panelxy_remap, p);
}

static void update_legend(sushiv_panel_t *p){  
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter ();

  if(plot){
    int i,depth=0;
    char buffer[320];
    plot_legend_clear(plot);

    if(3-xy->data_v.decimal_exponent > depth) depth = 3-xy->data_v.decimal_exponent;
    if(3-xy->x.decimal_exponent > depth) depth = 3-xy->x.decimal_exponent;
    if(3-xy->y.decimal_exponent > depth) depth = 3-xy->y.decimal_exponent;

    // if crosshairs are active, add them to the fun
    if( plot->cross_active){
      snprintf(buffer,320,"%s = %+.*f",
	       xy->x_scale.name,
	       depth,
	       xy->x_val);
      plot_legend_add(plot,buffer);
      snprintf(buffer,320,"%s = %+.*f",
	       xy->y_scale.name,
	       depth,
	       xy->y_val);
      plot_legend_add(plot,buffer);

      if(p->dimensions)
	plot_legend_add(plot,NULL);
    }

    // add each dimension to the legend
    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;

      if(d != xy->x_d ||
	 plot->cross_active){
	
	snprintf(buffer,320,"%s = %+.*f",
		 p->dimension_list[i].d->name,
		 depth,
		 p->dimension_list[i].d->val);
	plot_legend_add(plot,buffer);
      }
    }

    gdk_threads_leave ();
  }
}

// call with lock
static double _determine_rerender_metric(sushiv_panel_t *p, int half){
  sushiv_panelxy_t *xy = p->subtype->xy;
  int on = p->objectives;
  double pw = p->private->graph->allocation.width;
  double ph = p->private->graph->allocation.height;
  int dw = xy->data_v.pixels;
  int off = (half?2;1);

  // if this is a discrete data set, size/view changes cannot affect
  // the data spacing; that's set by the discrete scale
  if(p->x_d->type != SUSHIV_DIM_CONTINUOUS) return -1;

  double xscale = scalespace_pixel(&xy->x,1.) - scalespace_pixel(&xy->x,0.);
  double yscale = scalespace_pixel(&xy->y,1.) - scalespace_pixel(&xy->y,0.);

  // by plane, look at the spacing between visible x/y points
  double max = -1;
  for(i=0;i<on;i++){

    if(xy->x_vec[j] && xy->y_vec[j] && !mapping_inactive_p(xy->mappings+i)){
      double xacc = 0;
      double yacc = 0;
      double count = 0;
      double *x = p->x_vec[i];
      double *y = p->y_vec[i];
      
      for(j = off; j+off<=dw, j+=off){
	if(!isnan(x[i-off]) &&
	   !isnan(y[i-off]) &&
	   !isnan(x[i]) &&
	   !isnan(y[i]) &&
	   !(x[i-off] < 0 && x[i] < 0) &&
	   !(y[i-off] < 0 && y[i] < 0) &&
	   !(x[i-off] > pw && x[i] > pw) &&
	   !(y[i-off] > ph && y[i] > ph)){
	  
	  xacc += (x[i-off]-x[i]) * (x[i-off]-x[i]);
	  yacc += (y[i-off]-y[i]) * (y[i-off]-y[i]);
	  count++;
	}
      }

      acc = sqrt(xacc*xscale*xscale + yacc*yscale*yscale);

      if(count>0)
	if(acc/count > max) max = acc/count;
    }
  }

  return max;

}

// call while locked
static int _mark_recompute_by_metric_change(sushiv_panel_t *p, int recursing){
  // discrete val dimensions are immune to rerender by metric changes
  if(p->x_d->type != SUSHIV_DIM_CONTINUOUS) return 0; 

  sushiv_panelxy_t *xy = p->subtype->xy;
  double target = (double) p->private->oversample_n / p->private->oversample_d;
  double full =  _determine_rerender_metric(p, 0);
  
  if(!recursing) xy->request_scaling = 0;

  if(full > target){
    // we want to halve the sample spacing.  But first make sure we're
    // not looping due to uncertainties in the metric.
    if(xy->request_scaling != -1){
      xy->request_scaling = 1; // double pixels, halve spacing
      _sushiv_panel_dirty_plot(p); // trigger recompute
      return 1;
    }
  } else {
    double half =  _determine_rerender_metric(p, 1);
    if(half < target){
      // we want to double the sample spacing.  But first make sure we're
      // not looping due to uncertainties in the metric.
      if(xy->request_scaling != 1){
	xy->request_scaling = -1; // halve pixels, double spacing
	_sushiv_panel_dirty_plot(p); // trigger recompute
	return 1;
      }
    }
  }

  xy->request_scaling = 0;
  return 0;
}

static void mapchange_callback_xy(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;
  sushiv_panelxy_t *xy = p->subtype->xy;
  int onum = optr - p->objective_list;
  
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  // update colormap
  // oh, the wasteful
  int pos = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
  solid_set_func(&xy->mappings[onum],pos);
  slider_set_gradient(xy->alpha_scale[onum], &xy->mappings[onum]);
  
  // a map view size change may trigger a progressive up/down render,
  // but will at least cause a remap
  if(!_mark_recompute_by_metric_change(p,0))
    _sushiv_panel_dirty_map(p);
  _sushiv_panel_dirty_legend(p);
  _sushiv_undo_resume(p->sushi);
}
XXX
static void alpha_callback_xy(void * in, int buttonstate){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;

  if(buttonstate == 0){
    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);
  }

  _sushiv_panel_dirty_map(p);
  _sushiv_panel_dirty_legend(p);

  if(buttonstate == 2)
    _sushiv_undo_resume(p->sushi);
}

static void linetype_callback_xy(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;
  sushiv_panelxy_t *xy = p->subtype->xy;
  int onum = optr - p->objective_list;
  
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  // update colormap
  int pos = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
  xy->linetype[onum] = line_name[pos].value;

  _sushiv_panel_dirty_map(p);
  _sushiv_undo_resume(p->sushi);
}

static void pointtype_callback_xy(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;
  sushiv_panelxy_t *xy = p->subtype->xy;
  int onum = optr - p->objective_list;
  
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  // update colormap
  int pos = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
  xy->pointtype[onum] = point_name[pos].value;

  _sushiv_panel_dirty_map(p);
  _sushiv_undo_resume(p->sushi);
}

static void map_callback_xy(void *in,int buttonstate){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel1d_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);
  
  if(buttonstate == 0){
    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);
  }

  // has new bracketing changed the plot range scale?
  if(xy->x_bracket[0] != slider_get_value(xy->x_slider,0) ||
     xy->x_bracket[1] != slider_get_value(xy->x_slider,1) ||
     xy->y_bracket[0] != slider_get_value(xy->y_slider,0) ||
     xy->y_bracket[1] != slider_get_value(xy->y_slider,1)){

    int w = plot->w.allocation.width;
    int h = plot->w.allocation.height;

    xy->x_bracket[0] = slider_get_value(xy->x_slider,0);
    xy->x_bracket[1] = slider_get_value(xy->x_slider,1);
    xy->y_bracket[0] = slider_get_value(xy->y_slider,0);
    xy->y_bracket[1] = slider_get_value(xy->y_slider,1);
    
    xy->x = scalespace_linear(xy->x_bracket[1],
			      xy->x_bracket[0],
			      w,
			      plot->scalespacing,
			      xy->x_scale->legend);
    xy->y = scalespace_linear(xy->y_bracket[1],
			      xy->y_bracket[0],
			      h,
			      plot->scalespacing,
			      xy->y_scale->legend);

  }
XXX
  // a map view size change may trigger a progressive up/down render,
  // but will at least cause a remap
  if(_determine_rerender_by_metric(p))
    _mark_recompute_xy(p);
  else
    _sushiv_panel_dirty_map(p);

  if(buttonstate == 2)
    _sushiv_undo_resume(p->sushi);
}

static void update_dim_sel(sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
  int i;

  // enable/disable dimension slider thumbs
  for(i=0;i<p->dimensions;i++){
    
    if(xy->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xy->dim_xb[i]))){
      
      // set the x dim flag
      xy->x_d = p->dimension_list[i].d;
      xy->x_widget = p->private->dim_scales[i];
      xy->x_dnum = i;
    }
    if(xy->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xy->dim_xb[i]))){
      // make all thumbs visible 
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],0,1);
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],2,1);
    }else{
      // make bracket thumbs invisible */
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],0,0);
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],2,0);
    }
  } 
}

static void compute_xy(sushiv_panel_t *p, 
		       int x_d, 
		       scalespace sxi,
		       double *dim_vals,
		       double **x_vec,
		       double **y_vec,
		       _sushiv_bythread_cache_1d *c){
  sushiv_panelxy_t *xy = p->subtype->xy;
  int i,j,fn=p->sushi->functions;
  int w = sxi.pixels;

  /* by x */
  for(j=0;j<w;j++){
    dim_vals[x_d] = scalespace_value(&sxi,j);

    /* by function */
    for(i=0;i<fn;i++){
      if(c->call[i]){
	sushiv_function_t *f = p->sushi->function_list[i];
	double *fout = c->fout[i];
      	c->call[i](dim_vals,fout);
      }
    }

    /* process function output by objective */
    /* xy panels currently only care about the XY output values; in the
       future, Z may also be relevant */
    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      int xoff = o->private->x_fout;
      int yoff = o->private->y_fout;
      sushiv_function_t *xf = o->private->x_func;
      sushiv_function_t *yf = o->private->y_func;
      x_vec[j] = c->fout[xf->number][xoff];
      y_vec[j] = c->fout[yf->number][yoff];
    }
  }
}

// call only from main gtk thread
void _mark_recompute_xy(sushiv_panel_t *p){
  if(!p->private->realized) return;
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);
  int w = plot->w.allocation.width;
  int h = plot->w.allocation.height;
  int dw = w;
  int i,j;

XXXX
  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){
      dw = _sushiv_dimension_scales(p1->x_d, 
				    p1->x_d->bracket[0],
				    p1->x_d->bracket[1],
				    w,dw * p->private->oversample_n / p->private->oversample_d,
				    plot->scalespacing,
				    p1->x_d->name,
				    &p1->x,
				    &p1->x_v,
				    &p1->x_i);

      p1->y = scalespace_linear(p1->range_bracket[1],
				p1->range_bracket[0],
				h,
				plot->scalespacing,
				p1->range_scale->legend);
    }

    if(p1->data_size != dw){
      if(p1->data_vec){

	// make new vec
	int i;
	for(i=0;i<p->objectives;i++){
	  double *new_vec = malloc(dw * sizeof(**p1->data_vec));

	  free(p1->data_vec[i]);
	  p1->data_vec[i] = new_vec;
	}
      }
    }
    
    p1->data_size = dw;
    p1->panel_w = w;
    p1->panel_h = h;
    
    if(!p1->data_vec){
      // allocate it

      p1->data_vec = calloc(p->objectives,sizeof(*p1->data_vec));
      for(i=0;i<p->objectives;i++)
	p1->data_vec[i] = malloc(dw*sizeof(**p1->data_vec));
      
    }

    // blank it 
    for(i=0;i<p->objectives;i++)
      for(j=0;j<dw;j++)
	p1->data_vec[i][j]=NAN;

    _sushiv_panel_dirty_plot(p);
  }
}

static void recompute_callback_1d(void *ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  _mark_recompute_1d(p);
}

void _sushiv_panel1d_mark_recompute_linked(sushiv_panel_t *p){
  int i;

  /* look to see if any 1d panels link to passed in panel */
  sushiv_instance_t *s = p->sushi;
  for(i=0;i<s->panels;i++){
    sushiv_panel_t *q = s->panel_list[i];
    if(q != p && q->type == SUSHIV_PANEL_1D){
      sushiv_panel1d_t *q1 = q->subtype->p1;
      if(q1->link_x == p)
	_mark_recompute_1d(q);
      else{
	if(q1->link_y == p)
	  _mark_recompute_1d(q);
      }
    }
  }
}

static void update_crosshair(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  sushiv_panel_t *link=p1->link_x;
  Plot *plot = PLOT(p->private->graph);
  double x=0;
  int i;

  if(!p->private->realized)return;
  
  if(p1->link_y)link=p1->link_y;

  if(link){
    for(i=0;i<link->dimensions;i++){
      sushiv_dimension_t *d = link->dimension_list[i].d;
      if(d == p1->x_d)
	x = link->dimension_list[i].d->val;
    }
  }else{
    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;
      if(d == p1->x_d)
	x = p->dimension_list[i].d->val;
    }
  }
  
  if(p1->flip)
    plot_set_crosshairs(plot,0,x);
  else
    plot_set_crosshairs(plot,x,0);
  
  // in independent panels, crosshairs snap to a pixel position; the
  // cached dimension value should be accurate with respect to the
  // crosshairs.  in linked panels, the crosshairs snap to a pixel
  // position in the master panel; that is handled in the master, not
  // here.
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    sushiv_panel1d_t *p1 = p->subtype->p1;
    if(d == p1->x_d){
      if(p1->flip)
	d->val = scalespace_value(&plot->x,plot_get_crosshair_ypixel(plot));
      else
	d->val = scalespace_value(&plot->x,plot_get_crosshair_xpixel(plot));
    }
  }
  _sushiv_panel_dirty_legend(p);
}

void _sushiv_panel1d_update_linked_crosshairs(sushiv_panel_t *p, int xflag, int yflag){
  int i;

  /* look to see if any 1d panels link to passed in panel */
  sushiv_instance_t *s = p->sushi;
  for(i=0;i<s->panels;i++){
    sushiv_panel_t *q = s->panel_list[i];
    if(q != p && q->type == SUSHIV_PANEL_1D){
      sushiv_panel1d_t *q1 = q->subtype->p1;
      if(q1->link_x == p){
	update_crosshair(q);
	if(yflag)
	  q->private->request_compute(q);
      }else{
	if(q1->link_y == p){
	  update_crosshair(q);
	  if(xflag)
	    q->private->request_compute(q);
	}
      }
    }
  }
}

static void center_callback_1d(sushiv_dimension_list_t *dptr){
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int axisp = (d == p1->x_d);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _mark_recompute_1d(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    update_crosshair(p);
  }
}

static void bracket_callback_1d(sushiv_dimension_list_t *dptr){
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int axisp = d == p1->x_d;
    
  if(axisp)
    _mark_recompute_1d(p);
    
}

static void dimchange_callback_1d(GtkWidget *button,gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);

    update_x_sel(p);
    update_crosshair(p);
    plot_unset_box(PLOT(p->private->graph));
    _mark_recompute_1d(p);

    _sushiv_undo_resume(p->sushi);
  }
}

static void crosshair_callback(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  sushiv_panel_t *link = p1->link_x;
  double x=PLOT(p->private->graph)->selx;
  int i;

  if(p1->flip)
    x=PLOT(p->private->graph)->sely;
  if(p1->link_y)
    link=p1->link_y;
  
  _sushiv_panel_dirty_legend(p);

  if(p1->link_x){
    // make it the master panel's problem.
    plot_set_crosshairs_snap(PLOT(link->private->graph),
			     x,
			     PLOT(link->private->graph)->sely);
    link->private->crosshair_action(link);
  }else if (p1->link_y){
    // make it the master panel's problem.
    plot_set_crosshairs_snap(PLOT(link->private->graph),
			     PLOT(link->private->graph)->selx,
			     x);
    link->private->crosshair_action(link);
  }else{

    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);

    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;
      sushiv_panel1d_t *p1 = p->subtype->p1;
      if(d == p1->x_d)
	_sushiv_dimension_set_value(p->private->dim_scales[i],1,x);
	            
      p->private->oldbox_active = 0;
    }
    _sushiv_undo_resume(p->sushi);
  }
}

static void box_callback(void *in, int state){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sushiv_undo_push(p->sushi);
    plot_box_vals(plot,p1->oldbox);
    p->private->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);

    crosshair_callback(p);
    
    _sushiv_dimension_set_value(p1->x_scale,0,p1->oldbox[0]);
    _sushiv_dimension_set_value(p1->x_scale,2,p1->oldbox[1]);
    p->private->oldbox_active = 0;
    _sushiv_undo_resume(p->sushi);
    break;
  }
  _sushiv_panel_update_menus(p);
}

void _maintain_cache_1d(sushiv_panel_t *p, _sushiv_bythread_cache_1d *c, int w){
  
  /* toplevel initialization */
  if(c->fout == 0){
    int i,j;
    
    /* determine which functions are actually needed */
    c->call = calloc(p->sushi->functions,sizeof(*c->call));
    c->fout = calloc(p->sushi->functions,sizeof(*c->fout));
    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      for(j=0;j<o->outputs;j++)
	c->call[o->function_map[j]]=
	  p->sushi->function_list[o->function_map[j]]->callback;
    }
  }

  /* once to begin, as well as anytime the data width changes */
  if(c->storage_width < w){
    int i;
    c->storage_width = w;

    for(i=0;i<p->sushi->functions;i++){
      if(c->call[i]){
	if(c->fout[i])free(c->fout[i]);
	c->fout[i] = malloc(w * p->sushi->function_list[i]->outputs *
			    sizeof(**c->fout));
      }
    }
  }
}

// subtype entry point for plot remaps; lock held
int _sushiv_panel1d_map_redraw(sushiv_panel_t *p, _sushiv_bythread_cache *c){
  if(p->private->map_progress_count)return 0;
  p->private->map_progress_count++;

  // render to a temp surface so that we can release the lock occasionally
  Plot *plot = PLOT(p->private->graph);
  cairo_surface_t *back = plot->back;
  cairo_surface_t *cs = cairo_surface_create_similar(back,CAIRO_CONTENT_COLOR,
						     cairo_image_surface_get_width(back),
						     cairo_image_surface_get_height(back));
  cairo_t *ct = cairo_create(cs);
  
  if(_sushiv_panel1d_remap(p,ct) == -1){ // returns -1 on abort
    cairo_destroy(ct);
    cairo_surface_destroy(cs);
  }else{
    // else complete
    cairo_surface_destroy(plot->back);
    plot->back = cs;
    cairo_destroy(ct);
    
    _sushiv_panel_clean_map(p);
    plot_expose_request(plot);
  }

  return 1;
}

// subtype entry point for legend redraws; lock held
int _sushiv_panel1d_legend_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  if(p->private->legend_progress_count)return 0;
  p->private->legend_progress_count++;
  update_legend(p);
  _sushiv_panel_clean_legend(p);

  gdk_threads_leave();
  plot_draw_scales(plot);
  gdk_threads_enter();

  plot_expose_request(plot);
  return 1;
}

// subtype entry point for recomputation; lock held
int _sushiv_panel1d_compute(sushiv_panel_t *p,
			    _sushiv_bythread_cache *c){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot;
  
  int dw,w,h,i,d;
  int serialno;
  int x_d=-1;
  scalespace sy;

  scalespace sx;
  scalespace sxv;
  scalespace sxi;

  dw = p1->data_size;
  w = p1->panel_w;
  h = p1->panel_h;

  sy = p1->y;

  sx = p1->x;
  sxv = p1->x_v;
  sxi = p1->x_i;
  
  if(p->private->plot_progress_count)
    return 0;

  serialno = p->private->plot_serialno;
  p->private->plot_progress_count++;
  d = p->dimensions;
  plot = PLOT(p->private->graph);
  
  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[p->sushi->dimensions];

  /* get iterator bounds, use iterator scale */
  x_d = p1->x_d->number;

  if(p1->flip){
    plot->x = sy;
    plot->y = sx;
    plot->x_v = sy;
    plot->y_v = sxv;
  }else{
    plot->x = sx;
    plot->y = sy;
    plot->x_v = sxv;
    plot->y_v = sy;
  }

  // Initialize local dimension value array
  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  _maintain_cache_1d(p,&c->p1,dw);
  
  /* unlock for computation */
  gdk_threads_leave ();

  plot_draw_scales(plot);
  compute_1d(p, serialno, x_d, sxi, dw, dim_vals, &c->p1);
  
  gdk_threads_enter ();

  if(serialno == p->private->plot_serialno){
    _sushiv_panel_dirty_map(p);
    _sushiv_panel_dirty_legend(p);
    _sushiv_panel_clean_plot(p);
  }
  return 1;
}

static void panel1d_undo_log(sushiv_panel_undo_t *u, sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int i;

  // alloc fields as necessary
  if(!u->mappings)
    u->mappings =  calloc(p->objectives,sizeof(*u->mappings));
  if(!u->scale_vals[0])
    u->scale_vals[0] =  calloc(1,sizeof(**u->scale_vals));
  if(!u->scale_vals[1])
    u->scale_vals[1] =  calloc(1,sizeof(**u->scale_vals));
  if(!u->scale_vals[2])
    u->scale_vals[2] =  calloc(p->objectives,sizeof(**u->scale_vals));

  // populate undo
  u->scale_vals[0][0] = slider_get_value(p1->range_slider,0);
  u->scale_vals[1][0] = slider_get_value(p1->range_slider,1);

  for(i=0;i<p->objectives;i++){
    u->mappings[i] = 
      (p1->mappings[i].mapnum<<24) | 
      (p1->linetype[i]<<16) |
      (p1->pointtype[i]<<8);
    u->scale_vals[2][i] = slider_get_value(p1->alpha_scale[i],0);
  }

  u->x_d = p1->x_dnum;
  u->box[0] = p1->oldbox[0];
  u->box[1] = p1->oldbox[1];

  u->box_active = p->private->oldbox_active;
  
}

static void panel1d_undo_restore(sushiv_panel_undo_t *u, sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);

  int i;
  
  // go in through widgets
   
  slider_set_value(p1->range_slider,0,u->scale_vals[0][0]);
  slider_set_value(p1->range_slider,1,u->scale_vals[1][0]);

  for(i=0;i<p->objectives;i++){
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->map_pulldowns[i]), (u->mappings[i]>>24)&0xff );
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->line_pulldowns[i]), (u->mappings[i]>>16)&0xff );
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->point_pulldowns[i]), (u->mappings[i]>>8)&0xff );
    slider_set_value(p1->alpha_scale[i],0,u->scale_vals[2][i]);
  }

  if(p1->dim_xb && u->x_d<p->dimensions && p1->dim_xb[u->x_d])
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p1->dim_xb[u->x_d]),TRUE);

  update_x_sel(p);

  if(u->box_active){
    p1->oldbox[0] = u->box[0];
    p1->oldbox[1] = u->box[1];
    plot_box_set(plot,u->box);
    p->private->oldbox_active = 1;
  }else{
    plot_unset_box(plot);
    p->private->oldbox_active = 0;
  }
}

void _sushiv_realize_panel1d(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  char buffer[160];
  int i;
  _sushiv_undo_suspend(p->sushi);

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sushiv_clean_exit), (void *)SIGINT);

  // add border to sides with hbox/padding 
  GtkWidget *borderbox =  gtk_hbox_new(0,0);
  gtk_container_add (GTK_CONTAINER (p->private->toplevel), borderbox);

  // main layout vbox
  p->private->topbox = gtk_vbox_new(0,0);
  gtk_box_pack_start(GTK_BOX(borderbox), p->private->topbox, 1,1,4);
  gtk_container_set_border_width (GTK_CONTAINER (p->private->toplevel), 1);

  /* spinner, top bar */
  {
    GtkWidget *hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), hbox, 0,0,0);
    gtk_box_pack_end(GTK_BOX(hbox),GTK_WIDGET(p->private->spinner),0,0,0);
  }

  /* plotbox, graph */
  {
    unsigned flags = 0;
    if(p1->flip)
      flags |= PLOT_NO_X_CROSS;
    else
      flags |= PLOT_NO_Y_CROSS;

    p1->graph_table = gtk_table_new(2,2,0);
    p->private->plotbox = p1->graph_table;
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p->private->plotbox, 1,1,2);

    p->private->graph = GTK_WIDGET(plot_new(recompute_callback_1d,p,
					    (void *)(void *)crosshair_callback,p,
					    box_callback,p,flags)); 
    if(p1->flip){
      gtk_table_attach(GTK_TABLE(p1->graph_table),p->private->graph,0,2,0,1,
		       GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,1);
    }else{
      gtk_table_attach(GTK_TABLE(p1->graph_table),p->private->graph,1,2,0,2,
		       GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,1);
    }
  }

  /* range slider, goes in the plotbox table */
  /* may be vertical to the left of the plot or along the bottom if the plot is flipped */
  {
    GtkWidget **sl = calloc(2,sizeof(*sl));
    int lo = p1->range_scale->val_list[0];
    int hi = p1->range_scale->val_list[p1->range_scale->vals-1];

    /* the range slices/slider */ 
    sl[0] = slice_new(map_callback_1d,p);
    sl[1] = slice_new(map_callback_1d,p);

    if(p1->flip){
      gtk_table_attach(GTK_TABLE(p1->graph_table),sl[0],0,1,1,2,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p1->graph_table),sl[1],1,2,1,2,
		       GTK_EXPAND|GTK_FILL,0,0,0);
    }else{
      gtk_table_attach(GTK_TABLE(p1->graph_table),sl[0],0,1,1,2,
		       GTK_SHRINK,GTK_EXPAND|GTK_FILL,0,0);
      gtk_table_attach(GTK_TABLE(p1->graph_table),sl[1],0,1,0,1,
		       GTK_SHRINK,GTK_EXPAND|GTK_FILL,0,0);
      gtk_table_set_col_spacing(GTK_TABLE(p1->graph_table),0,4);
    }

    p1->range_slider = slider_new((Slice **)sl,2,
				  p1->range_scale->label_list,
				  p1->range_scale->val_list,
				  p1->range_scale->vals,
				  (p1->flip?0:SLIDER_FLAG_VERTICAL));

    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],hi);
  }

  /* obj box */
  {
    p1->obj_table = gtk_table_new(p->objectives,5,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p1->obj_table, 0,0,1);

    /* pulldowns */
    p1->pointtype = calloc(p->objectives,sizeof(*p1->pointtype));
    p1->linetype = calloc(p->objectives,sizeof(*p1->linetype));
    p1->mappings = calloc(p->objectives,sizeof(*p1->mappings));
    p1->map_pulldowns = calloc(p->objectives,sizeof(*p1->map_pulldowns));
    p1->line_pulldowns = calloc(p->objectives,sizeof(*p1->line_pulldowns));
    p1->point_pulldowns = calloc(p->objectives,sizeof(*p1->point_pulldowns));
    p1->alpha_scale = calloc(p->objectives,sizeof(*p1->alpha_scale));

    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      
      /* label */
      GtkWidget *label = gtk_label_new(o->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p1->obj_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* mapping pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_markup();
	int j;
	for(j=0;j<num_solids();j++){
	  if(strcmp(solid_name(j),"inactive"))
	    snprintf(buffer,sizeof(buffer),"<span foreground=\"%s\">%s</span>",solid_name(j),solid_name(j));
	  else
	    snprintf(buffer,sizeof(buffer),"%s",solid_name(j));
	  
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), buffer);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (mapchange_callback_1d), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p1->obj_table),menu,1,2,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	p1->map_pulldowns[i] = menu;
	solid_setup(&p1->mappings[i],0.,1.,0);
      }
      
      /* line pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_text();
	int j;
	for(j=0;j<LINETYPES;j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), line_name[j]->left);
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (linetype_callback_1d), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p1->obj_table),menu,2,3,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	p1->line_pulldowns[i] = menu;
      }
      
      /* point pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_text();
	int j;
	for(j=0;j<POINTTYPES;j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), point_name[j]->left);
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (pointtype_callback_1d), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p1->obj_table),menu,3,4,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	p1->point_pulldowns[i] = menu;
      }
      
      /* alpha slider */
      {
	GtkWidget **sl = calloc(1, sizeof(*sl));
	sl[0] = slice_new(alpha_callback_1d,p->objective_list+i);
	
	gtk_table_attach(GTK_TABLE(p1->obj_table),sl[0],4,5,i,i+1,
			 GTK_EXPAND|GTK_FILL,0,0,0);
	
	p1->alpha_scale[i] = slider_new((Slice **)sl,1,
					(char *[]){"transparent","solid"},
					(double []){0.,1.},
					2,0);
	
	slider_set_gradient(p1->alpha_scale[i], &p1->mappings[i]);
	slice_thumb_set((Slice *)sl[0],1.);
	
      }
    }
  }

  /* dim box */
  if(p->dimensions){
    p1->dim_table = gtk_table_new(p->dimensions,3,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p1->dim_table, 0,0,4);

    p->private->dim_scales = calloc(p->dimensions,sizeof(*p->private->dim_scales));
    p1->dim_xb = calloc(p->dimensions,sizeof(*p1->dim_xb));
    GtkWidget *first_x = NULL;
    
    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p1->dim_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* x radio buttons */
      if(!(d->flags & SUSHIV_DIM_NO_X) && !p1->link_x && !p1->link_y){
	if(first_x)
	  p1->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
	else{
	  first_x = p1->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p1->dim_xb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p1->dim_table),p1->dim_xb[i],1,2,i,i+1,
			 0,0,3,0);
      }
      
      p->private->dim_scales[i] = 
	_sushiv_new_dimension_widget(p->dimension_list+i,center_callback_1d,bracket_callback_1d);
      
      gtk_table_attach(GTK_TABLE(p1->dim_table),
		       GTK_WIDGET(p->private->dim_scales[i]->t),
		       2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
    }
    
    for(i=0;i<p->dimensions;i++)
      if(p1->dim_xb[i])
	g_signal_connect (G_OBJECT (p1->dim_xb[i]), "toggled",
			  G_CALLBACK (dimchange_callback_1d), p);
    
    update_x_sel(p);
  }
  
  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_realize(GTK_WIDGET(p->private->spinner));
  gtk_widget_show_all(p->private->toplevel);

  _sushiv_undo_resume(p->sushi);
}


static int _save_panel1d(sushiv_panel_t *p, xmlNodePtr pn){  
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int ret=0,i;

  xmlNodePtr n;

  xmlNewProp(pn, (xmlChar *)"type", (xmlChar *)"1d");

  // box
  if(p->private->oldbox_active){
    xmlNodePtr boxn = xmlNewChild(pn, NULL, (xmlChar *) "box", NULL);
    xmlNewPropF(boxn, "x1", p1->oldbox[0]);
    xmlNewPropF(boxn, "x2", p1->oldbox[1]);
  }
  
  // objective map settings
  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = p->objective_list[i].o;

    xmlNodePtr on = xmlNewChild(pn, NULL, (xmlChar *) "objective", NULL);
    xmlNewPropI(on, "position", i);
    xmlNewPropI(on, "number", o->number);
    xmlNewPropS(on, "name", o->name);
    xmlNewPropS(on, "type", o->output_types);
    
    // right now Y is the only type; the below is Y-specific

    n = xmlNewChild(on, NULL, (xmlChar *) "y-map", NULL);
    xmlNewMapProp(n, "color", solid_map(), p1->mappings[i].mapnum);
    xmlNewMapProp(n, "line", line_name, p1->linetype[i]);    
    xmlNewMapProp(n, "point", point_name, p1->pointtype[i]);    
    xmlNewPropF(n, "alpha", slider_get_value(p1->alpha_scale[i],0));
  }

  // y scale
  n = xmlNewChild(pn, NULL, (xmlChar *) "range", NULL);
  xmlNewPropF(n, "low-bracket", slider_get_value(p1->range_slider,0));
  xmlNewPropF(n, "high-bracket", slider_get_value(p1->range_slider,1));

  // x/y dim selection
  n = xmlNewChild(pn, NULL, (xmlChar *) "axes", NULL);
  xmlNewPropI(n, "xpos", p1->x_dnum);

  return ret;
}

int _load_panel1d(sushiv_panel_t *p,
		  sushiv_panel_undo_t *u,
		  xmlNodePtr pn,
		  int warn){
  int i;

  // check type
  xmlCheckPropS(pn,"type","1d", "Panel %d type mismatch in save file.",p->number,&warn);
  
  // box
  u->box_active = 0;
  xmlGetChildPropFPreserve(pn, "box", "x1", &u->box[0]);
  xmlGetChildPropFPreserve(pn, "box", "x2", &u->box[1]);

  xmlNodePtr n = xmlGetChildS(pn, "box", NULL, NULL);
  if(n){
    u->box_active = 1;
    xmlFree(n);
  }
  
  // objective map settings
  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = p->objective_list[i].o;
    xmlNodePtr on = xmlGetChildI(pn, "objective", "position", i);
    if(!on){
      first_load_warning(&warn);
      fprintf(stderr,"No save data found for panel %d objective \"%s\".\n",p->number, o->name);
    }else{
      // check name, type
      xmlCheckPropS(on,"name",o->name, "Objectve position %d name mismatch in save file.",i,&warn);
      xmlCheckPropS(on,"type",o->output_types, "Objectve position %d type mismatch in save file.",i,&warn);
      
      // right now Y is the only type; the below is Y-specific
      // load maptype, values
      int color = (u->mappings[i]>>24)&0xff;
      int line = (u->mappings[i]>>16)&0xff;
      int point = (u->mappings[i]>>8)&0xff;

      xmlGetChildMapPreserve(on, "y-map", "color", solid_map(), &color,
		     "Panel %d objective unknown mapping setting", p->number, &warn);
      xmlGetChildMapPreserve(on, "y-map", "line", line_name, &line,
		     "Panel %d objective unknown mapping setting", p->number, &warn);
      xmlGetChildMapPreserve(on, "y-map", "point", point_name, &point,
		     "Panel %d objective unknown mapping setting", p->number, &warn);
      xmlGetChildPropF(on, "y-map", "alpha", &u->scale_vals[2][i]);

      u->mappings[i] = (color<<24) | (line<<16) | (point<<8);

      xmlFreeNode(on);
    }
  }

  // y scale
  xmlGetChildPropFPreserve(pn, "range", "low-bracket", &u->scale_vals[0][0]);
  xmlGetChildPropF(pn, "range", "high-bracket", &u->scale_vals[1][0]);

  // x/y dim selection
  xmlGetChildPropI(pn, "axes", "xpos", &u->x_d);

  return warn;
}

int sushiv_new_panel_1d(sushiv_instance_t *s,
			int number,
			const char *name,
			sushiv_scale_t *scale,
			int *objectives,
			int *dimensions,	
			unsigned flags){

  int ret = _sushiv_new_panel(s,number,name,objectives,dimensions,flags);
  sushiv_panel_t *p;
  sushiv_panel1d_t *p1;

  if(ret<0)return ret;
  p = s->panel_list[number];
  p1 = calloc(1, sizeof(*p1));
  p->subtype = calloc(1, sizeof(*p->subtype));

  p->subtype->p1 = p1;
  p->type = SUSHIV_PANEL_1D;
  p1->range_scale = scale;
  p->private->bg_type = SUSHIV_BG_WHITE;

  if(p->flags && SUSHIV_PANEL_FLIP)
    p1->flip=1;

  p->private->realize = _sushiv_realize_panel1d;
  p->private->map_action = _sushiv_panel1d_map_redraw;
  p->private->legend_action = _sushiv_panel1d_legend_redraw;
  p->private->compute_action = _sushiv_panel1d_compute;
  p->private->request_compute = _mark_recompute_1d;
  p->private->crosshair_action = crosshair_callback;
  p->private->print_action = sushiv_panel1d_print;
  p->private->save_action = _save_panel1d;
  p->private->load_action = _load_panel1d;

  p->private->undo_log = panel1d_undo_log;
  p->private->undo_restore = panel1d_undo_restore;
  p->private->def_oversample_n = p->private->oversample_n = 1;
  p->private->def_oversample_d = p->private->oversample_d = 8;
  
  return 0;
}

int sushiv_new_panel_1d_linked(sushiv_instance_t *s,
			       int number,
			       const char *name,
			       sushiv_scale_t *scale,
			       int *objectives,
			       int link,
			       unsigned flags){
  
  if(link < 0 || 
     link >= s->panels || 
     s->panel_list[link]==NULL ||
     s->panel_list[link]->type != SUSHIV_PANEL_2D){
    fprintf(stderr,"1d linked panel must be linked to preexisting 2d panel.\n");
    return -EINVAL;
  }

  int ret = sushiv_new_panel_1d(s,number,name,scale,objectives,(int []){-1},flags);

  sushiv_panel_t *p;
  sushiv_panel1d_t *p1;
  sushiv_panel_t *p2 = s->panel_list[link];

  if(ret<0)return ret;
  p = s->panel_list[number];
  p1 = p->subtype->p1;

  if(flags && SUSHIV_PANEL_LINK_Y)
    p1->link_y = p2;
  else
    p1->link_x = p2;

  return 0;
}
