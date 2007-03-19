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

static void clear_xy_data(sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
  int i;
  
  if(xy->x_vec){
    for(i=0;i<p->objectives;i++){
      if(xy->x_vec[i]){
	free(xy->x_vec[i]);
	xy->x_vec[i]=0;
      }
    }
  }

  if(xy->y_vec){
    for(i=0;i<p->objectives;i++){
      if(xy->y_vec[i]){
	free(xy->y_vec[i]);
	xy->y_vec[i]=0;
      }
    }
  }
}

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
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);

  int plot_serialno = p->private->plot_serialno;
  int map_serialno = p->private->map_serialno;
  int xi,i,j;
  int pw = plot->x.pixels;
  int ph = plot->y.pixels;

  scalespace sx = xy->x;
  scalespace sy = xy->y;
  scalespace data_v = xy->data_v;
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

  if(xy->x_vec && xy->y_vec){
    
    /* by objective */
    for(j=0;j<p->objectives;j++){
      if(xy->x_vec[j] && xy->y_vec[j] && !mapping_inactive_p(xy->mappings+j)){
	
	int dw = data_v.pixels;
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
	       xy->x_scale->legend,
	       depth,
	       plot->selx);
      plot_legend_add(plot,buffer);
      snprintf(buffer,320,"%s = %+.*f",
	       xy->y_scale->legend,
	       depth,
	       plot->sely);
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
static double _determine_rerender_metric(sushiv_panel_t *p, int off){
  sushiv_panelxy_t *xy = p->subtype->xy;
  int on = p->objectives;
  double pw = p->private->graph->allocation.width;
  double ph = p->private->graph->allocation.height;
  int dw = xy->data_v.pixels;

  // if this is a discrete data set, size/view changes cannot affect
  // the data spacing; that's set by the discrete scale
  if(xy->x_d->type != SUSHIV_DIM_CONTINUOUS) return -1;

  double xscale = scalespace_pixel(&xy->x,1.) - scalespace_pixel(&xy->x,0.);
  double yscale = scalespace_pixel(&xy->y,1.) - scalespace_pixel(&xy->y,0.);

  // by plane, look at the spacing between visible x/y points
  double max = -1;
  int i,j;
  for(i=0;i<on;i++){

    if(xy->x_vec[j] && xy->y_vec[j] && !mapping_inactive_p(xy->mappings+i)){
      double xacc = 0;
      double yacc = 0;
      double count = 0;
      double *x = xy->x_vec[i];
      double *y = xy->y_vec[i];
      
      for(j = off; j<dw; j++){
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

      double acc = sqrt(xacc*xscale*xscale + yacc*yscale*yscale);

      if(count>0)
	if(acc/count > max) max = acc/count;
    }
  }

  return max;

}

// call while locked
static int _mark_recompute_by_metric(sushiv_panel_t *p, int recursing){
  sushiv_panelxy_t *xy = p->subtype->xy;

  // discrete val dimensions are immune to rerender by metric changes
  if(xy->x_d->type != SUSHIV_DIM_CONTINUOUS) return 0; 

  double target = (double) p->private->oversample_n / p->private->oversample_d;
  double full =  _determine_rerender_metric(p, 1);

  if(full > target){
    // we want to halve the sample spacing.  But first make sure we're
    // not looping due to uncertainties in the metric.
    if(recursing && xy->prev_zoom > xy->curr_zoom) return 0;

    // also make sure our zoom level doesn't underflow
    if(xy->data_v.massaged) return 0; 

    xy->prev_zoom = xy->curr_zoom;
    xy->curr_zoom++;

    _sushiv_panel_dirty_plot(p); // trigger recompute
    return 1;
  } else {
    double half =  _determine_rerender_metric(p, 2);
    if(half < target){
      // we want to double the sample spacing.  But first make sure we're
      // not looping due to uncertainties in the metric.
      if(recursing && xy->prev_zoom < xy->curr_zoom) return 0;

      // also make sure our zoom level doesn't overrflow
      if(xy->curr_zoom == 0) return 0;

      xy->prev_zoom = xy->curr_zoom;
      xy->curr_zoom--;
      
      _sushiv_panel_dirty_plot(p); // trigger recompute
      return 1;
      
    }
  }

  xy->prev_zoom = xy->curr_zoom;
  return 0;
}

static void mapchange_callback_xy(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;
  sushiv_panelxy_t *xy = p->subtype->xy;
  int onum = optr - p->objective_list;
  Plot *plot = PLOT(p->private->graph);

  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  // update colormap
  // oh, the wasteful
  int pos = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
  solid_set_func(&xy->mappings[onum],pos);
  slider_set_gradient(xy->alpha_scale[onum], &xy->mappings[onum]);
  
  // if the mapping has become inactive and the crosshairs point to
  // this objective, inactivate the crosshairs.
  if(xy->cross_objnum == onum)
    plot->cross_active = 0;

  // a map view size change may trigger a progressive up/down render,
  // but will at least cause a remap
  _mark_recompute_by_metric(p,0);

  _sushiv_panel_dirty_map(p);
  _sushiv_panel_dirty_legend(p);

  _sushiv_undo_resume(p->sushi);
}

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
  xy->linetype[onum] = line_name[pos]->value;

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
  xy->pointtype[onum] = point_name[pos]->value;

  _sushiv_panel_dirty_map(p);
  _sushiv_undo_resume(p->sushi);
}

static void map_callback_xy(void *in,int buttonstate){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panelxy_t *xy = p->subtype->xy;
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
    
  
    plot->x = xy->x = scalespace_linear(xy->x_bracket[1],
					xy->x_bracket[0],
					w,
					plot->scalespacing,
					xy->x_scale->legend);
    plot->y = xy->y = scalespace_linear(xy->y_bracket[1],
					xy->y_bracket[0],
					h,
					plot->scalespacing,
					xy->y_scale->legend);

    // a map view size change may trigger a progressive up/down render,
    // but will at least cause a remap
    _mark_recompute_by_metric(p,0);
    
    _sushiv_panel_dirty_map(p);
    
  }

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
		       int serialno,
		       int x_d, 
		       scalespace sxi,
		       double *dim_vals,
		       char *prefilled,
		       double **x_vec,
		       double **y_vec,
		       _sushiv_bythread_cache_xy *c){

  int i,j,fn=p->sushi->functions;
  int w = sxi.pixels;

  /* by x */
  for(j=0;j<w;j++){
    if(!prefilled[j]){
      dim_vals[x_d] = scalespace_value(&sxi,j);
      
      /* by function */
      for(i=0;i<fn;i++){
	if(c->call[i]){
	  double *fout = c->fout[i];
	  c->call[i](dim_vals,fout);
	}
      }
      
      /* process function output by objective */
      /* xy panels currently only care about the XY output values; in the
	 future, Z and others may also be relevant */
      for(i=0;i<p->objectives;i++){
	sushiv_objective_t *o = p->objective_list[i].o;
	int xoff = o->private->x_fout;
	int yoff = o->private->y_fout;
	sushiv_function_t *xf = o->private->x_func;
	sushiv_function_t *yf = o->private->y_func;
	x_vec[i][j] = c->fout[xf->number][xoff];
	y_vec[i][j] = c->fout[yf->number][yoff];
      }
    }

    if(!(j&0x3f)){
      gdk_threads_enter();
      
      if(serialno != p->private->plot_serialno){
	gdk_threads_leave();
	return;
      }

      spinner_set_busy(p->private->spinner);
      gdk_threads_leave();
    }
  }
}

// call with lock
void _mark_recompute_xy(sushiv_panel_t *p){
  if(!p->private->realized) return;

  _sushiv_panel_dirty_plot(p);
}

static void recompute_callback_xy(void *ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);
  int w = plot->w.allocation.width;
  int h = plot->w.allocation.height;

  plot->x = xy->x = scalespace_linear(xy->x_bracket[0],
				      xy->x_bracket[1],
				      w,
				      plot->scalespacing,
				      xy->x_scale->legend);
  plot->y =  xy->y = scalespace_linear(xy->y_bracket[1],
				       xy->y_bracket[0],
				       h,
				       plot->scalespacing,
				       xy->y_scale->legend);

  // always recompute, but also update zoom
  if(!_mark_recompute_by_metric(p,0))
    _mark_recompute_xy(p);
}

static void update_crosshair(sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);


  if(!p->private->realized)return;

  // crosshairs snap to the x/y location of a point; however, with
  // multiple objectives, there are probably multiple possible points.
  // So, if we're currently pointing to a point for a given objective,
  // update to a point on the same objective.  Otherwise if crosshairs
  // are inactive, do nothing.

  if(!plot->cross_active)return;
  if(xy->cross_objnum<0 || xy->cross_objnum >= p->objectives)return;
  if(!xy->x_vec || !xy->y_vec)return;
  if(!xy->x_vec[xy->cross_objnum] || !xy->y_vec[xy->cross_objnum])return;

  // get bin number of dim value
  int x_bin = scalespace_pixel(&xy->data_v, xy->x_d->val);
  double x = xy->x_vec[xy->cross_objnum][x_bin];
  double y = xy->y_vec[xy->cross_objnum][x_bin];

  plot_set_crosshairs(plot,x,y);
  
  _sushiv_panel_dirty_legend(p);
}

static void center_callback_xy(sushiv_dimension_list_t *dptr){
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panelxy_t *xy = p->subtype->xy;
  int axisp = (d == xy->x_d);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _mark_recompute_xy(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    update_crosshair(p);
  }
}

static void bracket_callback_xy(sushiv_dimension_list_t *dptr){
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panelxy_t *xy = p->subtype->xy;
  int axisp = (d == xy->x_d);
    
  // always need to recompute, amy also need to update zoom

  if(axisp)
    if(!_mark_recompute_by_metric(p,0))
      _mark_recompute_xy(p);
  
}

static void dimchange_callback_xy(GtkWidget *button,gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);

    update_dim_sel(p);
    
    // clear data vectors so that none of the data is reused.
    clear_xy_data(p);

    update_crosshair(p); // which is to say, deactivate it
    plot_unset_box(PLOT(p->private->graph));

    if(!_mark_recompute_by_metric(p,0))
      _mark_recompute_xy(p);

    _sushiv_undo_resume(p->sushi);
  }
}

static void crosshair_callback(sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
  double x=PLOT(p->private->graph)->selx;
  double y=PLOT(p->private->graph)->sely;
  int i,j;

  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);
  
  // snap crosshairs to the closest plotted x/y point
  int besto=-1;
  int bestbin=-1;
  double bestdist;

  for(i=0;i<p->objectives;i++){
    if(xy->x_vec[i] && xy->y_vec[i] && !mapping_inactive_p(xy->mappings+i)){
      for(j=0;j<xy->data_v.pixels;j++){
	double xd = x - xy->x_vec[i][j];
	double yd = y - xy->y_vec[i][j];
	double dist = xd*xd + yd*yd;
	if(besto==-1 || dist<bestdist){
	  besto = i;
	  bestbin = j;
	  bestdist = dist;
	}
      }
    }
  }

  if(besto>-1){
    x = xy->x_vec[besto][bestbin];
    y = xy->y_vec[besto][bestbin];

    PLOT(p->private->graph)->selx = x;
    PLOT(p->private->graph)->sely = y;
    xy->cross_objnum = besto;
    
    double dimval = scalespace_value(&xy->data_v, bestbin);
    sushiv_dimension_set_value(p->sushi,xy->x_d->number,1,dimval);  
  }
  
  p->private->oldbox_active = 0;
  _sushiv_undo_resume(p->sushi);
  _sushiv_panel_dirty_legend(p);

}

static void box_callback(void *in, int state){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sushiv_undo_push(p->sushi);
    plot_box_vals(plot,xy->oldbox);
    p->private->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);

    crosshair_callback(p);
    map_callback_xy(p,1);
    p->private->oldbox_active = 0;
    _sushiv_undo_resume(p->sushi);
    break;
  }
  _sushiv_panel_update_menus(p);
}

void _maintain_cache_xy(sushiv_panel_t *p, _sushiv_bythread_cache_xy *c, int w){
  
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

    for(i=0;i<p->sushi->functions;i++){
      if(c->call[i]){
	c->fout[i] = malloc(p->sushi->function_list[i]->outputs *
			    sizeof(**c->fout));
      }
    }
  }
}

// subtype entry point for plot remaps; lock held
int _sushiv_panelxy_map_redraw(sushiv_panel_t *p, _sushiv_bythread_cache *c){
  if(p->private->map_progress_count)return 0;
  p->private->map_progress_count++;

  // render to a temp surface so that we can release the lock occasionally
  Plot *plot = PLOT(p->private->graph);
  cairo_surface_t *back = plot->back;
  cairo_surface_t *cs = cairo_surface_create_similar(back,CAIRO_CONTENT_COLOR,
						     cairo_image_surface_get_width(back),
						     cairo_image_surface_get_height(back));
  cairo_t *ct = cairo_create(cs);
  
  if(_sushiv_panelxy_remap(p,ct) == -1){ // returns -1 on abort
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
int _sushiv_panelxy_legend_redraw(sushiv_panel_t *p){
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

// dim scales are autozoomed; we want the initial values to quantize
// to the same grid regardless of zoom level or starting bracket as
// well as only encompass the desired range
static int _generate_dimscale_xy(sushiv_dimension_t *d, int zoom, scalespace *v, scalespace *i){
  scalespace x;
  
  if(d->type != SUSHIV_DIM_CONTINUOUS){
    // non-continuous is unaffected by zoom
    _sushiv_dimension_scales(d, d->bracket[0], d->bracket[1], 2, 2, 1, d->name, &x, v, i);
    return 0;
  }

  // continuous dimensions are, in some ways, handled like a discrete dim.
  double lo = d->scale->val_list[0];
  double hi = d->scale->val_list[d->scale->vals-1];
  _sushiv_dimension_scales(d, lo, hi, 2, 2, 1, NULL, &x, v, i);
  
  // generate a new low that is quantized to our coarsest grid
  lo = scalespace_value(v,0);

  // choose a new resolution, make new scales 

  //  this is iterative, not direct computation, so that at each level
  // we have a small adjustment (as opposed to one huge adjustment at
  // the end) as well as more resolution headroom at each level
  int sofar = 0;
  while (sofar<zoom){
    int pixels = v->pixels << 1;

    _sushiv_dimension_scales(d, lo, hi, pixels, pixels, 1, NULL, &x, v, i);
    if(v->massaged)return 1;

    // clip scales down the the desired part of the range
    while(v->pixels>2 &&  scalespace_value(v,1) < d->bracket[0]){
      v->first_pixel--;
      i->first_pixel--;
      v->pixels--;
      i->pixels--;
    }

    while(v->pixels>2 &&  scalespace_value(v,v->pixels-1) > d->bracket[1]){
      v->pixels--;
      i->pixels--;
    }
  }

  return 0;
}

static void _rescale_xy(scalespace *old, double **oldx, double **oldy,
			scalespace *new, double **newx, double **newy,
			char *prefilled, int objectives){

  if(!oldx || !oldy)return;

  int newi = 0;
  int oldi = 0;
  int j;

  long num = scalespace_scalenum(new,old);
  long den = scalespace_scalenum(new,old);
  long oldpos = scalespace_scaleoff(new,old);
  long newpos = 0;

  while(newi < new->pixels && oldi < old->pixels){

    if(oldpos == newpos &&
       oldi >= 0){
      prefilled[newi] = 1;
      for(j=0;j<objectives;j++){
	if(oldx[j])
	  newx[j][newi] = oldx[j][oldi];
	if(oldy[j])
	  newy[j][newi] = oldy[j][oldi];
      }
    }

    while(newi < new->pixels && newi < oldi ){
      newpos += den;
      newi++;
    }
    while(oldi < old->pixels && oldi < newi ){
      oldpos += num;
      oldi++;
    }
  }
}

// subtype entry point for recomputation; lock held
int _sushiv_panelxy_compute(sushiv_panel_t *p,
			    _sushiv_bythread_cache *c){
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot;
  
  int dw,w,h,i,d;
  int serialno;
  int x_d=-1;

  scalespace sxv = xy->data_v;
  plot = PLOT(p->private->graph);

  dw = sxv.pixels;
  w = plot->w.allocation.width;
  h = plot->w.allocation.height;

  // this computation is single-threaded for now
  if(p->private->plot_progress_count)
    return 0;

  serialno = p->private->plot_serialno;
  p->private->plot_progress_count++;
  d = p->dimensions;
  
  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[p->sushi->dimensions];

  /* get iterator bounds, use iterator scale */
  x_d = xy->x_d->number;

  /* generate a new data_v/data_i */
  scalespace newv;
  scalespace newi;
  _generate_dimscale_xy(xy->x_d, xy->curr_zoom, &newv, &newi);
  dw = newv.pixels;

  /* compare new/old data scales; pre-fill the data vec with values
     from old data vector if it can be reused */
  double *new_x_vec[p->objectives];
  double *new_y_vec[p->objectives];
  char *prefilled = calloc(dw,sizeof(*prefilled));
  for(i=0;i<p->objectives;i++){
    new_x_vec[i] = calloc(dw,sizeof(**new_x_vec));
    new_y_vec[i] = calloc(dw,sizeof(**new_y_vec));
  }
  _rescale_xy(&sxv,xy->x_vec,xy->y_vec,   
	      &newv,new_x_vec,new_y_vec,prefilled, p->objectives);

  // Initialize local dimension value array
  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  _maintain_cache_xy(p,&c->xy,dw);
  
  /* unlock for computation */
  gdk_threads_leave ();

  plot_draw_scales(plot);
  compute_xy(p, serialno, x_d, newi, dim_vals, prefilled, new_x_vec, new_y_vec, &c->xy);
  
  gdk_threads_enter ();

  if(serialno == p->private->plot_serialno){
    // replace data vectors
    p->private->plot_serialno++;
    clear_xy_data(p);
    if(!xy->x_vec)
      xy->x_vec = calloc(p->objectives, sizeof(*xy->x_vec));
    if(!xy->y_vec)
      xy->y_vec = calloc(p->objectives, sizeof(*xy->y_vec));
    for(i=0;i<p->objectives;i++){
      xy->x_vec[i] = new_x_vec[i];
      xy->y_vec[i] = new_y_vec[i];
    }
    free(prefilled);
    // replace scales;
    xy->data_v = newv;
    xy->data_i = newi;

    _sushiv_panel_dirty_map(p);
    _sushiv_panel_dirty_legend(p);
    _sushiv_panel_clean_plot(p);
  }else{
    for(i=0;i<p->objectives;i++){
      free(new_x_vec[i]);
      free(new_y_vec[i]);
    }
    free(prefilled);
  }

  return 1;
}

static void panelxy_undo_log(sushiv_panel_undo_t *u, sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);
  int i;

  // alloc fields as necessary
  if(!u->mappings)
    u->mappings =  calloc(p->objectives,sizeof(*u->mappings));
  if(!u->scale_vals[0])
    u->scale_vals[0] =  calloc(3,sizeof(**u->scale_vals));
  if(!u->scale_vals[1])
    u->scale_vals[1] =  calloc(3,sizeof(**u->scale_vals));
  if(!u->scale_vals[2])
    u->scale_vals[2] =  calloc(p->objectives,sizeof(**u->scale_vals));

  // populate undo
  u->scale_vals[0][0] = slider_get_value(xy->x_slider,0);
  u->scale_vals[1][0] = slider_get_value(xy->x_slider,1);
  u->scale_vals[0][1] = plot->selx;
  u->scale_vals[1][1] = plot->sely;
  u->scale_vals[0][2] = slider_get_value(xy->y_slider,0);
  u->scale_vals[1][2] = slider_get_value(xy->y_slider,1);

  for(i=0;i<p->objectives;i++){
    u->mappings[i] = 
      (xy->mappings[i].mapnum<<24) | 
      (xy->linetype[i]<<16) |
      (xy->pointtype[i]<<8);
    u->scale_vals[2][i] = slider_get_value(xy->alpha_scale[i],0);
  }

  u->x_d = xy->x_dnum;
  u->box[0] = xy->oldbox[0];
  u->box[1] = xy->oldbox[1];
  u->box[2] = xy->oldbox[2];
  u->box[3] = xy->oldbox[3];

  u->box_active = p->private->oldbox_active;
  
}

static void panelxy_undo_restore(sushiv_panel_undo_t *u, sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);

  int i;
  
  // go in through widgets
   
  slider_set_value(xy->x_slider,0,u->scale_vals[0][0]);
  slider_set_value(xy->x_slider,1,u->scale_vals[1][0]);
  plot->selx = u->scale_vals[0][1];
  plot->sely = u->scale_vals[1][1];
  slider_set_value(xy->y_slider,0,u->scale_vals[0][2]);
  slider_set_value(xy->y_slider,1,u->scale_vals[1][2]);

  for(i=0;i<p->objectives;i++){
    gtk_combo_box_set_active(GTK_COMBO_BOX(xy->map_pulldowns[i]), (u->mappings[i]>>24)&0xff );
    gtk_combo_box_set_active(GTK_COMBO_BOX(xy->line_pulldowns[i]), (u->mappings[i]>>16)&0xff );
    gtk_combo_box_set_active(GTK_COMBO_BOX(xy->point_pulldowns[i]), (u->mappings[i]>>8)&0xff );
    slider_set_value(xy->alpha_scale[i],0,u->scale_vals[2][i]);
  }

  if(xy->dim_xb && u->x_d<p->dimensions && xy->dim_xb[u->x_d])
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xy->dim_xb[u->x_d]),TRUE);

  update_dim_sel(p);
  crosshair_callback(p);

  if(u->box_active){
    xy->oldbox[0] = u->box[0];
    xy->oldbox[1] = u->box[1];
    xy->oldbox[2] = u->box[2];
    xy->oldbox[3] = u->box[3];
    plot_box_set(plot,u->box);
    p->private->oldbox_active = 1;
  }else{
    plot_unset_box(plot);
    p->private->oldbox_active = 0;
  }
}

void _sushiv_realize_panelxy(sushiv_panel_t *p){
  sushiv_panelxy_t *xy = p->subtype->xy;
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
    xy->graph_table = gtk_table_new(3,3,0);
    p->private->plotbox = xy->graph_table;
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p->private->plotbox, 1,1,2);

    p->private->graph = GTK_WIDGET(plot_new(recompute_callback_xy,p,
					    (void *)(void *)crosshair_callback,p,
					    box_callback,p,0));
    
    gtk_table_attach(GTK_TABLE(xy->graph_table),p->private->graph,1,3,0,2,
		     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,1);
  }

  /* X range slider */
  {
    GtkWidget **sl = calloc(2,sizeof(*sl));
    int lo = xy->x_scale->val_list[0];
    int hi = xy->x_scale->val_list[xy->x_scale->vals-1];

    /* the range slices/slider */ 
    sl[0] = slice_new(map_callback_xy,p);
    sl[1] = slice_new(map_callback_xy,p);

    gtk_table_attach(GTK_TABLE(xy->graph_table),sl[0],1,2,2,3,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(xy->graph_table),sl[1],2,3,2,3,
		     GTK_EXPAND|GTK_FILL,0,0,0);

    xy->x_slider = slider_new((Slice **)sl,2,
			      xy->x_scale->label_list,
			      xy->x_scale->val_list,
			      xy->x_scale->vals,0);
    
    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],hi);
  }

  /* Y range slider */
  {
    GtkWidget **sl = calloc(2,sizeof(*sl));
    int lo = xy->y_scale->val_list[0];
    int hi = xy->y_scale->val_list[xy->y_scale->vals-1];

    /* the range slices/slider */ 
    sl[0] = slice_new(map_callback_xy,p);
    sl[1] = slice_new(map_callback_xy,p);

    gtk_table_attach(GTK_TABLE(xy->graph_table),sl[0],0,1,1,2,
		     GTK_SHRINK,GTK_EXPAND|GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(xy->graph_table),sl[1],0,1,0,1,
		     GTK_SHRINK,GTK_EXPAND|GTK_FILL,0,0);
    gtk_table_set_col_spacing(GTK_TABLE(xy->graph_table),0,4);
    
    xy->y_slider = slider_new((Slice **)sl,2,
			      xy->y_scale->label_list,
			      xy->y_scale->val_list,
			      xy->y_scale->vals,
			      SLIDER_FLAG_VERTICAL);

    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],hi);
  }

  /* obj box */
  {
    xy->obj_table = gtk_table_new(p->objectives,5,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), xy->obj_table, 0,0,1);

    /* pulldowns */
    xy->pointtype = calloc(p->objectives,sizeof(*xy->pointtype));
    xy->linetype = calloc(p->objectives,sizeof(*xy->linetype));
    xy->mappings = calloc(p->objectives,sizeof(*xy->mappings));
    xy->map_pulldowns = calloc(p->objectives,sizeof(*xy->map_pulldowns));
    xy->line_pulldowns = calloc(p->objectives,sizeof(*xy->line_pulldowns));
    xy->point_pulldowns = calloc(p->objectives,sizeof(*xy->point_pulldowns));
    xy->alpha_scale = calloc(p->objectives,sizeof(*xy->alpha_scale));

    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      
      /* label */
      GtkWidget *label = gtk_label_new(o->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(xy->obj_table),label,0,1,i,i+1,
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
			  G_CALLBACK (mapchange_callback_xy), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(xy->obj_table),menu,1,2,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	xy->map_pulldowns[i] = menu;
	solid_setup(&xy->mappings[i],0.,1.,0);
      }
      
      /* line pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_text();
	int j;
	for(j=0;j<LINETYPES;j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), line_name[j]->left);
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (linetype_callback_xy), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(xy->obj_table),menu,2,3,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	xy->line_pulldowns[i] = menu;
      }
      
      /* point pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_text();
	int j;
	for(j=0;j<POINTTYPES;j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), point_name[j]->left);
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (pointtype_callback_xy), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(xy->obj_table),menu,3,4,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	xy->point_pulldowns[i] = menu;
      }
      
      /* alpha slider */
      {
	GtkWidget **sl = calloc(1, sizeof(*sl));
	sl[0] = slice_new(alpha_callback_xy,p->objective_list+i);
	
	gtk_table_attach(GTK_TABLE(xy->obj_table),sl[0],4,5,i,i+1,
			 GTK_EXPAND|GTK_FILL,0,0,0);
	
	xy->alpha_scale[i] = slider_new((Slice **)sl,1,
					(char *[]){"transparent","solid"},
					(double []){0.,1.},
					2,0);
	
	slider_set_gradient(xy->alpha_scale[i], &xy->mappings[i]);
	slice_thumb_set((Slice *)sl[0],1.);
	
      }
    }
  }

  /* dim box */
  if(p->dimensions){
    xy->dim_table = gtk_table_new(p->dimensions,3,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), xy->dim_table, 0,0,4);

    p->private->dim_scales = calloc(p->dimensions,sizeof(*p->private->dim_scales));
    xy->dim_xb = calloc(p->dimensions,sizeof(*xy->dim_xb));
    GtkWidget *first_x = NULL;
    
    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(xy->dim_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* x radio buttons */
      if(!(d->flags & SUSHIV_DIM_NO_X)){
	if(first_x)
	  xy->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
	else{
	  first_x = xy->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xy->dim_xb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(xy->dim_table),xy->dim_xb[i],1,2,i,i+1,
			 0,0,3,0);
      }
      
      p->private->dim_scales[i] = 
	_sushiv_new_dimension_widget(p->dimension_list+i,center_callback_xy,bracket_callback_xy);
      
      gtk_table_attach(GTK_TABLE(xy->dim_table),
		       GTK_WIDGET(p->private->dim_scales[i]->t),
		       2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
    }
    
    for(i=0;i<p->dimensions;i++)
      if(xy->dim_xb[i])
	g_signal_connect (G_OBJECT (xy->dim_xb[i]), "toggled",
			  G_CALLBACK (dimchange_callback_xy), p);
    
    update_dim_sel(p);
  }
  
  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_realize(GTK_WIDGET(p->private->spinner));
  gtk_widget_show_all(p->private->toplevel);

  _sushiv_undo_resume(p->sushi);
}


static int _save_panelxy(sushiv_panel_t *p, xmlNodePtr pn){  
  sushiv_panelxy_t *xy = p->subtype->xy;
  Plot *plot = PLOT(p->private->graph);
  int ret=0,i;

  xmlNodePtr n;

  xmlNewProp(pn, (xmlChar *)"type", (xmlChar *)"xy");

  // box
  if(p->private->oldbox_active){
    xmlNodePtr boxn = xmlNewChild(pn, NULL, (xmlChar *) "box", NULL);
    xmlNewPropF(boxn, "x1", xy->oldbox[0]);
    xmlNewPropF(boxn, "x2", xy->oldbox[1]);
    xmlNewPropF(boxn, "y1", xy->oldbox[2]);
    xmlNewPropF(boxn, "y2", xy->oldbox[3]);
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
    xmlNewMapProp(n, "color", solid_map(), xy->mappings[i].mapnum);
    xmlNewMapProp(n, "line", line_name, xy->linetype[i]);    
    xmlNewMapProp(n, "point", point_name, xy->pointtype[i]);    
    xmlNewPropF(n, "alpha", slider_get_value(xy->alpha_scale[i],0));
  }

  // x/y scale
  n = xmlNewChild(pn, NULL, (xmlChar *) "range", NULL);
  xmlNewPropF(n, "x-low-bracket", slider_get_value(xy->x_slider,0));
  xmlNewPropF(n, "x-high-bracket", slider_get_value(xy->x_slider,1));
  xmlNewPropF(n, "y-low-bracket", slider_get_value(xy->y_slider,0));
  xmlNewPropF(n, "y-high-bracket", slider_get_value(xy->y_slider,1));
  xmlNewPropF(n, "x-cross", plot->selx);
  xmlNewPropF(n, "y-cross", plot->sely);

  // x/y dim selection
  n = xmlNewChild(pn, NULL, (xmlChar *) "axes", NULL);
  xmlNewPropI(n, "xpos", xy->x_dnum);

  return ret;
}

int _load_panelxy(sushiv_panel_t *p,
		  sushiv_panel_undo_t *u,
		  xmlNodePtr pn,
		  int warn){
  int i;

  // check type
  xmlCheckPropS(pn,"type","xy", "Panel %d type mismatch in save file.",p->number,&warn);
  
  // box
  u->box_active = 0;
  xmlGetChildPropFPreserve(pn, "box", "x1", &u->box[0]);
  xmlGetChildPropFPreserve(pn, "box", "x2", &u->box[1]);
  xmlGetChildPropFPreserve(pn, "box", "y1", &u->box[2]);
  xmlGetChildPropFPreserve(pn, "box", "y2", &u->box[3]);

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

  xmlGetChildPropFPreserve(pn, "range", "x-low-bracket", &u->scale_vals[0][0]);
  xmlGetChildPropFPreserve(pn, "range", "x-high-bracket", &u->scale_vals[1][0]);
  xmlGetChildPropFPreserve(pn, "range", "y-low-bracket", &u->scale_vals[0][2]);
  xmlGetChildPropFPreserve(pn, "range", "y-high-bracket", &u->scale_vals[1][2]);
  xmlGetChildPropFPreserve(pn, "range", "x-cross", &u->scale_vals[0][1]);
  xmlGetChildPropF(pn, "range", "y-cross", &u->scale_vals[1][1]);

  // x/y dim selection
  xmlGetChildPropI(pn, "axes", "xpos", &u->x_d);

  return warn;
}

int sushiv_new_panel_xy(sushiv_instance_t *s,
			int number,
			const char *name,
			sushiv_scale_t *xscale,
			sushiv_scale_t *yscale,
			int *objectives,
			int *dimensions,	
			unsigned flags){

  int ret = _sushiv_new_panel(s,number,name,objectives,dimensions,flags);
  sushiv_panel_t *p;
  sushiv_panelxy_t *xy;

  if(ret<0)return ret;
  p = s->panel_list[number];
  xy = calloc(1, sizeof(*xy));
  p->subtype = calloc(1, sizeof(*p->subtype));

  p->subtype->xy = xy;
  p->type = SUSHIV_PANEL_XY;
  xy->x_scale = xscale;
  xy->y_scale = yscale;
  p->private->bg_type = SUSHIV_BG_WHITE;

  p->private->realize = _sushiv_realize_panelxy;
  p->private->map_action = _sushiv_panelxy_map_redraw;
  p->private->legend_action = _sushiv_panelxy_legend_redraw;
  p->private->compute_action = _sushiv_panelxy_compute;
  p->private->request_compute = _mark_recompute_xy;
  p->private->crosshair_action = crosshair_callback;
  p->private->print_action = sushiv_panelxy_print;
  p->private->save_action = _save_panelxy;
  p->private->load_action = _load_panelxy;

  p->private->undo_log = panelxy_undo_log;
  p->private->undo_restore = panelxy_undo_restore;
  p->private->def_oversample_n = p->private->oversample_n = 1;
  p->private->def_oversample_d = p->private->oversample_d = 8;
  
  return 0;
}

