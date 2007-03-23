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
static _sv_propmap_t *line_name[LINETYPES+1] = {
  &(_sv_propmap_t){"line", 0,          NULL,NULL,NULL},
  &(_sv_propmap_t){"fat line", 1,      NULL,NULL,NULL},
  &(_sv_propmap_t){"fill above", 2,    NULL,NULL,NULL},
  &(_sv_propmap_t){"fill below", 3,    NULL,NULL,NULL},
  &(_sv_propmap_t){"fill to zero", 4,  NULL,NULL,NULL},
  &(_sv_propmap_t){"no line", 5,       NULL,NULL,NULL},
  NULL
};

#define POINTTYPES 9
static _sv_propmap_t *point_name[POINTTYPES+1] = {
  &(_sv_propmap_t){"dot", 0,             NULL,NULL,NULL},
  &(_sv_propmap_t){"cross", 1,           NULL,NULL,NULL},
  &(_sv_propmap_t){"plus", 2,            NULL,NULL,NULL},
  &(_sv_propmap_t){"open circle", 3,     NULL,NULL,NULL},
  &(_sv_propmap_t){"open square", 4,     NULL,NULL,NULL},
  &(_sv_propmap_t){"open triangle", 5,   NULL,NULL,NULL},
  &(_sv_propmap_t){"solid circle", 6,    NULL,NULL,NULL},
  &(_sv_propmap_t){"solid square", 7,    NULL,NULL,NULL},
  &(_sv_propmap_t){"solid triangle", 8,  NULL,NULL,NULL},
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
static int _sv_panel1d_remap(sv_panel_t *p, cairo_t *c){
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot = PLOT(p->private->graph);

  int plot_serialno = p->private->plot_serialno;
  int map_serialno = p->private->map_serialno;
  int xi,i,j;
  int pw = plot->x.pixels;
  int ph = plot->y.pixels;
  int dw = p1->data_size;
  double r = (p1->flip?p1->panel_w:p1->panel_h);
  
  _sv_scalespace_t rx = (p1->flip?p1->y:p1->x);
  _sv_scalespace_t ry = (p1->flip?p1->x:p1->y);
  _sv_scalespace_t sx = p1->x;
  _sv_scalespace_t sy = p1->y;
  _sv_scalespace_t sx_v = p1->x_v;
  _sv_scalespace_t px = plot->x;
  _sv_scalespace_t py = plot->y;
    
  /* do the panel and plot scales match?  If not, redraw the plot
     scales */
  
  if(memcmp(&rx,&px,sizeof(rx)) ||
     memcmp(&ry,&py,sizeof(ry))){

    plot->x = rx;
    plot->y = ry;
    
    gdk_threads_leave();
    _sv_plot_draw_scales(plot);
  }else
    gdk_threads_leave();

  /* blank frame to selected bg */
  switch(p->private->bg_type){
  case SV_BG_WHITE:
    cairo_set_source_rgb (c, 1.,1.,1.);
    cairo_paint(c);
    break;
  case SV_BG_BLACK:
    cairo_set_source_rgb (c, 0,0,0);
    cairo_paint(c);
    break;
  case SV_BG_CHECKS:
    render_checks(c,pw,ph);
    break;
  }

  gdk_threads_enter();
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno) return -1;

  if(p1->data_vec){
    
    /* by objective */
    for(j=0;j<p->objectives;j++){
      if(p1->data_vec[j] && !_sv_mapping_inactive_p(p1->mappings+j)){
	
	double alpha = _sv_slider_get_value(p1->alpha_scale[j],0);
	int linetype = p1->linetype[j];
	int pointtype = p1->pointtype[j];
	u_int32_t color = _sv_mapping_calc(p1->mappings+j,1.,0);
      
	double xv[dw];
	double yv[dw];
	double data_vec[dw];
	
	memcpy(data_vec,p1->data_vec[j],sizeof(data_vec));
	gdk_threads_leave();

	/* by x */
	for(xi=0;xi<dw;xi++){
	  double val = data_vec[xi];
	  double xpixel = xi;
	  double ypixel = NAN;
	  
	  /* map data vector bin to x pixel location in the plot */
	  xpixel = _sv_scalespace_pixel(&sx,_sv_scalespace_value(&sx_v,xpixel))+.5;
	  
	  /* map/render result */
	  if(!isnan(val))
	    ypixel = _sv_scalespace_pixel(&sy,val)+.5;
	  
	  xv[xi] = xpixel;
	  yv[xi] = ypixel;
	}
	
	/* draw areas, if any */
	if(linetype>1 && linetype < 5){
	  double yA=-1;
	  if(linetype == 2) /* fill above */
	    yA= (p1->flip?r:-1);
	  if(linetype == 3) /* fill below */
	    yA = (p1->flip?-1:r);
	  if(linetype == 4) /* fill to zero */
	    yA = _sv_scalespace_pixel(&sy,0.)+.5;
	  
	  cairo_set_source_rgba(c,
				((color>>16)&0xff)/255.,
				((color>>8)&0xff)/255.,
				((color)&0xff)/255.,
				alpha*.75);
	  
	  if(!isnan(yv[0])){
	    if(p1->flip){
	      cairo_move_to(c,yA,xv[0]+.5);
	      cairo_line_to(c,yv[0],xv[0]+.5);
	    }else{
	      cairo_move_to(c,xv[0]-.5,yA);
	      cairo_line_to(c,xv[0]-.5,yv[0]);
	    }
	  }
	  
	  for(i=1;i<dw;i++){
	    
	    if(isnan(yv[i])){
	      if(!isnan(yv[i-1])){
		/* close off the area */
		if(p1->flip){
		  cairo_line_to(c,yv[i-1],xv[i-1]-.5);
		  cairo_line_to(c,yA,xv[i-1]-.5);
		}else{
		  cairo_line_to(c,xv[i-1]+.5,yv[i-1]);
		  cairo_line_to(c,xv[i-1]+.5,yA);
		}
		cairo_close_path(c);
	      }
	    }else{
	      if(isnan(yv[i-1])){
		if(p1->flip){
		  cairo_move_to(c,yA,xv[i]+.5);
		  cairo_line_to(c,yv[i],xv[i]+.5);
		}else{
		  cairo_move_to(c,xv[i]-.5,yA);
		  cairo_line_to(c,xv[i]-.5,yv[i]);
		}
	      }else{
		if(p1->flip){
		  cairo_line_to(c,yv[i],xv[i]);
		}else{
		  cairo_line_to(c,xv[i],yv[i]);
		}
	      }
	    }
	  }
	    
	  if(!isnan(yv[i-1])){
	    /* close off the area */
	    if(p1->flip){
	      cairo_line_to(c,yv[i-1],xv[i-1]-.5);
	      cairo_line_to(c,yA,xv[i-1]-.5);
	    }else{
	      cairo_line_to(c,xv[i-1]+.5,yv[i-1]);
	      cairo_line_to(c,xv[i-1]+.5,yA);
	    }
	    cairo_close_path(c);
	  }
	    
	  cairo_fill(c);
	}
	  
	/* now draw the lines */
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
	      
	    if(!isnan(yv[i-1]) && !isnan(yv[i])){
		
	      if(p1->flip){
		cairo_move_to(c,yv[i-1],xv[i-1]);
		cairo_line_to(c,yv[i],xv[i]);
	      }else{
		cairo_move_to(c,xv[i-1],yv[i-1]);
		cairo_line_to(c,xv[i],yv[i]);
	      }
	      cairo_stroke(c);
	    }	      
	  }
	}

	/* now draw the points */
	if(pointtype > 0 || linetype == 5){
	  cairo_set_line_width(c,1.);

	  for(i=0;i<dw;i++){
	    if(!isnan(yv[i])){
	      double xx,yy;
	      if(p1->flip){
		xx = yv[i];
		yy = xv[i];
	      }else{
		xx = xv[i];
		yy = yv[i];
	      }

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
		if(p->private->bg_type == SV_BG_WHITE)
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

static void _sv_panel1d_print(sv_panel_t *p, cairo_t *c, int w, int h){
  _sv_plot_t *plot = PLOT(p->private->graph);
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

  _sv_plot_print(plot, c, ph*scale, (void(*)(void *, cairo_t *))_sv_panel1d_remap, p);
}

static void _sv_panel1d_update_legend(sv_panel_t *p){  
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot = PLOT(p->private->graph);

  gdk_threads_enter ();

  if(plot){
    int i,depth=0;
    char buffer[320];
    _sv_plot_legend_clear(plot);

    if(3-_sv_scalespace_decimal_exponent(&p1->x_v) > depth) 
      depth = 3-_sv_scalespace_decimal_exponent(&p1->x_v);

    // add each dimension to the legend
    for(i=0;i<p->dimensions;i++){
      sv_dim_t *d = p->dimension_list[i].d;
      // display decimal precision relative to bracket
      //int depth = del_depth(p->dimension_list[i].d->bracket[0],
      //		    p->dimension_list[i].d->bracket[1]) + offset;
      if( d!=p1->x_d ||
	  plot->cross_active){
	
	snprintf(buffer,320,"%s = %+.*f",
		 p->dimension_list[i].d->name,
		 depth,
		 p->dimension_list[i].d->val);
	_sv_plot_legend_add(plot,buffer);
      }
    }

    // linked? add the linked dimension value to the legend
    if(p1->link_x || p1->link_y){
      sv_dim_t *d;
      int depth=0;
      if(p1->link_x)
	d = p1->link_x->subtype->p2->x_d;
      else
	d = p1->link_y->subtype->p2->y_d;
      
      // add each dimension to the legend
      // display decimal precision relative to display scales
      
      snprintf(buffer,320,"%s = %+.*f",
	       d->name,
	       depth,
	       d->val);
      _sv_plot_legend_add(plot,buffer);
    }

    // one space 
    _sv_plot_legend_add(plot,NULL);

    // add each active objective to the legend
    // choose the value under the crosshairs 
    if(plot->cross_active){
      double val = (p1->flip?plot->sely:plot->selx);
      int bin = rint(_sv_scalespace_pixel(&p1->x_v, val));

      for(i=0;i<p->objectives;i++){
	if(!_sv_mapping_inactive_p(p1->mappings+i)){
	  u_int32_t color = _sv_mapping_calc(p1->mappings+i,1.,0);
	  
	  snprintf(buffer,320,"%s",
		   p->objective_list[i].o->name);
	  
	  if(bin>=0 && bin<p1->data_size){
	    
	    float val = p1->data_vec[i][bin];
	    
	    if(!isnan(val)){
	      snprintf(buffer,320,"%s = %f",
		       p->objective_list[i].o->name,
		       val);
	    }
	  }
	
	  _sv_plot_legend_add_with_color(plot,buffer,color | 0xff000000);
	}
      }
    }
    gdk_threads_leave ();
  }
}

static void _sv_panel1d_mapchange_callback(GtkWidget *w,gpointer in){
  sv_obj_list_t *optr = (sv_obj_list_t *)in;
  sv_panel_t *p = optr->p;
  _sv_panel1d_t *p1 = p->subtype->p1;
  int onum = optr - p->objective_list;
  
  _sv_undo_push(p->sushi);
  _sv_undo_suspend(p->sushi);

  // update colormap
  // oh, the wasteful
  _sv_solid_set_func(&p1->mappings[onum],
		     gtk_combo_box_get_active(GTK_COMBO_BOX(w)));
  _sv_slider_set_gradient(p1->alpha_scale[onum], &p1->mappings[onum]);
  
  _sv_panel_dirty_map(p);
  _sv_panel_dirty_legend(p);
  _sv_undo_resume(p->sushi);
}

static void _sv_panel1d_alpha_callback(void * in, int buttonstate){
  sv_obj_list_t *optr = (sv_obj_list_t *)in;
  sv_panel_t *p = optr->p;
  //  _sv_panel1d_t *p1 = p->subtype->p1;
  //  int onum = optr - p->objective_list;

  if(buttonstate == 0){
    _sv_undo_push(p->sushi);
    _sv_undo_suspend(p->sushi);
  }

  _sv_panel_dirty_map(p);
  _sv_panel_dirty_legend(p);

  if(buttonstate == 2)
    _sv_undo_resume(p->sushi);
}

static void _sv_panel1d_linetype_callback(GtkWidget *w,gpointer in){
  sv_obj_list_t *optr = (sv_obj_list_t *)in;
  sv_panel_t *p = optr->p;
  _sv_panel1d_t *p1 = p->subtype->p1;
  int onum = optr - p->objective_list;
  
  _sv_undo_push(p->sushi);
  _sv_undo_suspend(p->sushi);

  // update colormap
  int pos = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
  p1->linetype[onum] = line_name[pos]->value;

  _sv_panel_dirty_map(p);
  _sv_undo_resume(p->sushi);
}

static void _sv_panel1d_pointtype_callback(GtkWidget *w,gpointer in){
  sv_obj_list_t *optr = (sv_obj_list_t *)in;
  sv_panel_t *p = optr->p;
  _sv_panel1d_t *p1 = p->subtype->p1;
  int onum = optr - p->objective_list;
  
  _sv_undo_push(p->sushi);
  _sv_undo_suspend(p->sushi);

  // update colormap
  int pos = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
  p1->pointtype[onum] = point_name[pos]->value;

  _sv_panel_dirty_map(p);
  _sv_undo_resume(p->sushi);
}

static void _sv_panel1d_map_callback(void *in,int buttonstate){
  sv_panel_t *p = (sv_panel_t *)in;
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot = PLOT(p->private->graph);
  
  if(buttonstate == 0){
    _sv_undo_push(p->sushi);
    _sv_undo_suspend(p->sushi);
  }

  // has new bracketing changed the plot range scale?
  if(p1->range_bracket[0] != _sv_slider_get_value(p1->range_slider,0) ||
     p1->range_bracket[1] != _sv_slider_get_value(p1->range_slider,1)){

    int w = plot->w.allocation.width;
    int h = plot->w.allocation.height;

    p1->range_bracket[0] = _sv_slider_get_value(p1->range_slider,0);
    p1->range_bracket[1] = _sv_slider_get_value(p1->range_slider,1);
    
    if(p1->flip)
      p1->y = _sv_scalespace_linear(p1->range_bracket[0],
				    p1->range_bracket[1],
				    w,
				    plot->scalespacing,
				    p1->range_scale->legend);
    
    else
      p1->y = _sv_scalespace_linear(p1->range_bracket[1],
				    p1->range_bracket[0],
				    h,
				    plot->scalespacing,
				    p1->range_scale->legend);
    
  }
  
  //redraw the plot
  _sv_panel_dirty_map(p);
  if(buttonstate == 2)
    _sv_undo_resume(p->sushi);
}

static void _sv_panel1d_update_xsel(sv_panel_t *p){
  _sv_panel1d_t *p1 = p->subtype->p1;
  int i;

  // enable/disable dimension slider thumbs
  // enable/disable objective 'point' dropdowns
  for(i=0;i<p->dimensions;i++){
    
    if(p1->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p1->dim_xb[i]))){
      
      // set the x dim flag
      p1->x_d = p->dimension_list[i].d;
      p1->x_scale = p->private->dim_scales[i];
      p1->x_dnum = i;
    }
    if(p1->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p1->dim_xb[i]))){
      // make all thumbs visible 
      _sv_dim_widget_set_thumb_active(p->private->dim_scales[i],0,1);
      _sv_dim_widget_set_thumb_active(p->private->dim_scales[i],2,1);
    }else{
      // make bracket thumbs invisible */
      _sv_dim_widget_set_thumb_active(p->private->dim_scales[i],0,0);
      _sv_dim_widget_set_thumb_active(p->private->dim_scales[i],2,0);
    }
  } 
}

static void _sv_panel1d_compute_line(sv_panel_t *p, 
				     int serialno,
				     int x_d, 
				     _sv_scalespace_t sxi,
				     int w, 
				     double *dim_vals,
				     _sv_bythread_cache_1d_t *c){
  _sv_panel1d_t *p1 = p->subtype->p1;
  double work[w];
  int i,j,fn=p->sushi->functions;

  /* by function */
  for(i=0;i<fn;i++){
    if(c->call[i]){
      sv_func_t *f = p->sushi->function_list[i];
      int step = f->outputs;
      double *fout = c->fout[i];
      
      /* by x */
      for(j=0;j<w;j++){
	dim_vals[x_d] = _sv_scalespace_value(&sxi,j);
	c->call[i](dim_vals,fout);
	fout+=step;
      }
    }
  }

  /* process function output by objective */
  /* 1d panels currently only care about the Y output value; in the
     future, Z may also be relevant */
  for(i=0;i<p->objectives;i++){
    sv_obj_t *o = p->objective_list[i].o;
    int offset = o->private->y_fout;
    sv_func_t *f = o->private->y_func;
    if(f){
      int step = f->outputs;
      double *fout = c->fout[f->number]+offset;
      
      /* map result from function output to objective output */
      for(j=0;j<w;j++){
	work[j] = *fout;
	fout+=step;
      }
      
      gdk_threads_enter (); // misuse me as a global mutex
      if(p->private->plot_serialno == serialno){
	/* store result in panel */
	memcpy(p1->data_vec[i],work,w*sizeof(*work));
	gdk_threads_leave (); // misuse me as a global mutex 
      }else{
	gdk_threads_leave (); // misuse me as a global mutex 
	break;
      }
    }
  }
}

// call only from main gtk thread
void _sv_panel1d_mark_recompute(sv_panel_t *p){
  if(!p->private->realized) return;
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot = PLOT(p->private->graph);
  int w = plot->w.allocation.width;
  int h = plot->w.allocation.height;
  int dw = w;
  sv_panel_t *link = (p1->link_x ? p1->link_x : p1->link_y);
  _sv_panel2d_t *p2 = (link?link->subtype->p2:NULL);
  int i,j;

  if(p1->link_x){
    dw = p2->x_v.pixels;
    p1->x_d = p2->x_d;
    p1->x_scale = p2->x_scale;
  }
  if(p1->link_y){
    dw = p2->y_v.pixels;
    p1->x_d = p2->y_d;
    p1->x_scale = p2->y_scale;
  }

  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){
    if(p1->flip){
      dw = _sv_dim_scales(p1->x_d, 
			  p1->x_d->bracket[1],
			  p1->x_d->bracket[0],
			  h,dw * p->private->oversample_n / p->private->oversample_d,
			  plot->scalespacing,
			  p1->x_d->name,
			  &p1->x,
			  &p1->x_v,
			  &p1->x_i);
      
      p1->y = _sv_scalespace_linear(p1->range_bracket[0],
				    p1->range_bracket[1],
				    w,
				    plot->scalespacing,
				    p1->range_scale->legend);
      
    }else{
      dw = _sv_dim_scales(p1->x_d, 
				    p1->x_d->bracket[0],
				    p1->x_d->bracket[1],
				    w,dw * p->private->oversample_n / p->private->oversample_d,
				    plot->scalespacing,
				    p1->x_d->name,
				    &p1->x,
				    &p1->x_v,
				    &p1->x_i);

      p1->y = _sv_scalespace_linear(p1->range_bracket[1],
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

    _sv_panel_dirty_plot(p);
  }
}

static void _sv_panel1d_recompute_callback(void *ptr){
  sv_panel_t *p = (sv_panel_t *)ptr;
  _sv_panel1d_mark_recompute(p);
}

void _sv_panel1d_mark_recompute_linked(sv_panel_t *p){
  int i;

  /* look to see if any 1d panels link to passed in panel */
  sv_instance_t *s = p->sushi;
  for(i=0;i<s->panels;i++){
    sv_panel_t *q = s->panel_list[i];
    if(q != p && q->type == SV_PANEL_1D){
      _sv_panel1d_t *q1 = q->subtype->p1;
      if(q1->link_x == p)
	_sv_panel1d_mark_recompute(q);
      else{
	if(q1->link_y == p)
	  _sv_panel1d_mark_recompute(q);
      }
    }
  }
}

static void _sv_panel1d_update_crosshair(sv_panel_t *p){
  _sv_panel1d_t *p1 = p->subtype->p1;
  sv_panel_t *link=p1->link_x;
  _sv_plot_t *plot = PLOT(p->private->graph);
  double x=0;
  int i;

  if(!p->private->realized)return;
  
  if(p1->link_y)link=p1->link_y;

  if(link){
    for(i=0;i<link->dimensions;i++){
      sv_dim_t *d = link->dimension_list[i].d;
      if(d == p1->x_d)
	x = link->dimension_list[i].d->val;
    }
  }else{
    for(i=0;i<p->dimensions;i++){
      sv_dim_t *d = p->dimension_list[i].d;
      if(d == p1->x_d)
	x = p->dimension_list[i].d->val;
    }
  }
  
  if(p1->flip)
    _sv_plot_set_crosshairs(plot,0,x);
  else
    _sv_plot_set_crosshairs(plot,x,0);
  
  // in independent panels, crosshairs snap to a pixel position; the
  // cached dimension value should be accurate with respect to the
  // crosshairs.  in linked panels, the crosshairs snap to a pixel
  // position in the master panel; that is handled in the master, not
  // here.
  for(i=0;i<p->dimensions;i++){
    sv_dim_t *d = p->dimension_list[i].d;
    _sv_panel1d_t *p1 = p->subtype->p1;
    if(d == p1->x_d){
      if(p1->flip)
	d->val = _sv_scalespace_value(&plot->x,_sv_plot_get_crosshair_ypixel(plot));
      else
	d->val = _sv_scalespace_value(&plot->x,_sv_plot_get_crosshair_xpixel(plot));
    }
  }
  _sv_panel_dirty_legend(p);
}

void _sv_panel1d_update_linked_crosshairs(sv_panel_t *p, int xflag, int yflag){
  int i;

  /* look to see if any 1d panels link to passed in panel */
  sv_instance_t *s = p->sushi;
  for(i=0;i<s->panels;i++){
    sv_panel_t *q = s->panel_list[i];
    if(q != p && q->type == SV_PANEL_1D){
      _sv_panel1d_t *q1 = q->subtype->p1;
      if(q1->link_x == p){
	_sv_panel1d_update_crosshair(q);
	if(yflag)
	  q->private->request_compute(q);
      }else{
	if(q1->link_y == p){
	  _sv_panel1d_update_crosshair(q);
	  if(xflag)
	    q->private->request_compute(q);
	}
      }
    }
  }
}

static void _sv_panel1d_center_callback(sv_dim_list_t *dptr){
  sv_dim_t *d = dptr->d;
  sv_panel_t *p = dptr->p;
  _sv_panel1d_t *p1 = p->subtype->p1;
  int axisp = (d == p1->x_d);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _sv_panel1d_mark_recompute(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    _sv_panel1d_update_crosshair(p);
  }
}

static void _sv_panel1d_bracket_callback(sv_dim_list_t *dptr){
  sv_dim_t *d = dptr->d;
  sv_panel_t *p = dptr->p;
  _sv_panel1d_t *p1 = p->subtype->p1;
  int axisp = d == p1->x_d;
    
  if(axisp)
    _sv_panel1d_mark_recompute(p);
    
}

static void _sv_panel1d_dimchange_callback(GtkWidget *button,gpointer in){
  sv_panel_t *p = (sv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sv_undo_push(p->sushi);
    _sv_undo_suspend(p->sushi);

    _sv_panel1d_update_xsel(p);
    _sv_panel1d_update_crosshair(p);
    _sv_plot_unset_box(PLOT(p->private->graph));
    _sv_panel1d_mark_recompute(p);

    _sv_undo_resume(p->sushi);
  }
}

static void _sv_panel1d_crosshair_callback(sv_panel_t *p){
  _sv_panel1d_t *p1 = p->subtype->p1;
  sv_panel_t *link = p1->link_x;
  double x=PLOT(p->private->graph)->selx;
  int i;

  if(p1->flip)
    x=PLOT(p->private->graph)->sely;
  if(p1->link_y)
    link=p1->link_y;
  
  _sv_panel_dirty_legend(p);

  if(p1->link_x){
    // make it the master panel's problem.
    _sv_plot_set_crosshairs_snap(PLOT(link->private->graph),
			     x,
			     PLOT(link->private->graph)->sely);
    link->private->crosshair_action(link);
  }else if (p1->link_y){
    // make it the master panel's problem.
    _sv_plot_set_crosshairs_snap(PLOT(link->private->graph),
			     PLOT(link->private->graph)->selx,
			     x);
    link->private->crosshair_action(link);
  }else{

    _sv_undo_push(p->sushi);
    _sv_undo_suspend(p->sushi);

    for(i=0;i<p->dimensions;i++){
      sv_dim_t *d = p->dimension_list[i].d;
      _sv_panel1d_t *p1 = p->subtype->p1;
      if(d == p1->x_d)
	_sv_dim_set_value(p->private->dim_scales[i],1,x);
	            
      p->private->oldbox_active = 0;
    }
    _sv_undo_resume(p->sushi);
  }
}

static void _sv_panel1d_box_callback(void *in, int state){
  sv_panel_t *p = (sv_panel_t *)in;
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sv_undo_push(p->sushi);
    _sv_plot_box_vals(plot,p1->oldbox);
    p->private->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sv_undo_push(p->sushi);
    _sv_undo_suspend(p->sushi);

    _sv_panel1d_crosshair_callback(p);
    
    _sv_dim_set_value(p1->x_scale,0,p1->oldbox[0]);
    _sv_dim_set_value(p1->x_scale,2,p1->oldbox[1]);
    p->private->oldbox_active = 0;
    _sv_undo_resume(p->sushi);
    break;
  }
  _sv_panel_update_menus(p);
}

void _sv_panel1d_maintain_cache(sv_panel_t *p, _sv_bythread_cache_1d_t *c, int w){
  
  /* toplevel initialization */
  if(c->fout == 0){
    int i,j;
    
    /* determine which functions are actually needed */
    c->call = calloc(p->sushi->functions,sizeof(*c->call));
    c->fout = calloc(p->sushi->functions,sizeof(*c->fout));
    for(i=0;i<p->objectives;i++){
      sv_obj_t *o = p->objective_list[i].o;
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
int _sv_panel1d_map_redraw(sv_panel_t *p, _sv_bythread_cache_t *c){
  if(p->private->map_progress_count)return 0;
  p->private->map_progress_count++;

  // render to a temp surface so that we can release the lock occasionally
  _sv_plot_t *plot = PLOT(p->private->graph);
  cairo_surface_t *back = plot->back;
  cairo_surface_t *cs = cairo_surface_create_similar(back,CAIRO_CONTENT_COLOR,
						     cairo_image_surface_get_width(back),
						     cairo_image_surface_get_height(back));
  cairo_t *ct = cairo_create(cs);
  
  if(_sv_panel1d_remap(p,ct) == -1){ // returns -1 on abort
    cairo_destroy(ct);
    cairo_surface_destroy(cs);
  }else{
    // else complete
    cairo_surface_destroy(plot->back);
    plot->back = cs;
    cairo_destroy(ct);
    
    _sv_panel_clean_map(p);
    _sv_plot_expose_request(plot);
  }

  return 1;
}

// subtype entry point for legend redraws; lock held
int _sv_panel1d_legend_redraw(sv_panel_t *p){
  _sv_plot_t *plot = PLOT(p->private->graph);

  if(p->private->legend_progress_count)return 0;
  p->private->legend_progress_count++;
  _sv_panel1d_update_legend(p);
  _sv_panel_clean_legend(p);

  gdk_threads_leave();
  _sv_plot_draw_scales(plot);
  gdk_threads_enter();

  _sv_plot_expose_request(plot);
  return 1;
}

// subtype entry point for recomputation; lock held
int _sv_panel1d_compute(sv_panel_t *p,
			_sv_bythread_cache_t *c){
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot;
  
  int dw,w,h,i,d;
  int serialno;
  int x_d=-1;
  _sv_scalespace_t sy;

  _sv_scalespace_t sx;
  _sv_scalespace_t sxv;
  _sv_scalespace_t sxi;

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
    sv_dim_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  _sv_panel1d_maintain_cache(p,&c->p1,dw);
  
  /* unlock for computation */
  gdk_threads_leave ();

  _sv_panel1d_compute_line(p, serialno, x_d, sxi, dw, dim_vals, &c->p1);
  
  gdk_threads_enter ();

  if(serialno == p->private->plot_serialno){
    _sv_panel_dirty_map(p);
    _sv_panel_dirty_legend(p);
    _sv_panel_clean_plot(p);
  }
  return 1;
}

static void _sv_panel1d_undo_log(_sv_panel_undo_t *u, sv_panel_t *p){
  _sv_panel1d_t *p1 = p->subtype->p1;
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
  u->scale_vals[0][0] = _sv_slider_get_value(p1->range_slider,0);
  u->scale_vals[1][0] = _sv_slider_get_value(p1->range_slider,1);

  for(i=0;i<p->objectives;i++){
    u->mappings[i] = 
      (p1->mappings[i].mapnum<<24) | 
      (p1->linetype[i]<<16) |
      (p1->pointtype[i]<<8);
    u->scale_vals[2][i] = _sv_slider_get_value(p1->alpha_scale[i],0);
  }

  u->x_d = p1->x_dnum;
  u->box[0] = p1->oldbox[0];
  u->box[1] = p1->oldbox[1];

  u->box_active = p->private->oldbox_active;
  
}

static void _sv_panel1d_undo_restore(_sv_panel_undo_t *u, sv_panel_t *p){
  _sv_panel1d_t *p1 = p->subtype->p1;
  _sv_plot_t *plot = PLOT(p->private->graph);

  int i;
  
  // go in through widgets
   
  _sv_slider_set_value(p1->range_slider,0,u->scale_vals[0][0]);
  _sv_slider_set_value(p1->range_slider,1,u->scale_vals[1][0]);

  for(i=0;i<p->objectives;i++){
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->map_pulldowns[i]), (u->mappings[i]>>24)&0xff );
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->line_pulldowns[i]), (u->mappings[i]>>16)&0xff );
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->point_pulldowns[i]), (u->mappings[i]>>8)&0xff );
    _sv_slider_set_value(p1->alpha_scale[i],0,u->scale_vals[2][i]);
  }

  if(p1->dim_xb && u->x_d<p->dimensions && p1->dim_xb[u->x_d])
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p1->dim_xb[u->x_d]),TRUE);

  _sv_panel1d_update_xsel(p);

  if(u->box_active){
    p1->oldbox[0] = u->box[0];
    p1->oldbox[1] = u->box[1];
    _sv_plot_box_set(plot,u->box);
    p->private->oldbox_active = 1;
  }else{
    _sv_plot_unset_box(plot);
    p->private->oldbox_active = 0;
  }
}

void _sv_panel1d_realize(sv_panel_t *p){
  _sv_panel1d_t *p1 = p->subtype->p1;
  char buffer[160];
  int i;
  _sv_undo_suspend(p->sushi);

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sv_clean_exit), (void *)SIGINT);

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
      flags |= _SV_PLOT_NO_X_CROSS;
    else
      flags |= _SV_PLOT_NO_Y_CROSS;

    p1->graph_table = gtk_table_new(2,2,0);
    p->private->plotbox = p1->graph_table;
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p->private->plotbox, 1,1,2);

    p->private->graph = GTK_WIDGET(_sv_plot_new(_sv_panel1d_recompute_callback,p,
					    (void *)(void *)_sv_panel1d_crosshair_callback,p,
					    _sv_panel1d_box_callback,p,flags)); 
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
    sl[0] = _sv_slice_new(_sv_panel1d_map_callback,p);
    sl[1] = _sv_slice_new(_sv_panel1d_map_callback,p);

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

    p1->range_slider = _sv_slider_new((_sv_slice_t **)sl,2,
				      p1->range_scale->label_list,
				      p1->range_scale->val_list,
				      p1->range_scale->vals,
				      (p1->flip?0:_SV_SLIDER_FLAG_VERTICAL));
    
    _sv_slice_thumb_set((_sv_slice_t *)sl[0],lo);
    _sv_slice_thumb_set((_sv_slice_t *)sl[1],hi);
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
      sv_obj_t *o = p->objective_list[i].o;
      
      /* label */
      GtkWidget *label = gtk_label_new(o->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p1->obj_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* mapping pulldown */
      {
	GtkWidget *menu=_gtk_combo_box_new_markup();
	int j;
	for(j=0;j<_sv_solid_names();j++){
	  if(strcmp(_sv_solid_name(j),"inactive"))
	    snprintf(buffer,sizeof(buffer),"<span foreground=\"%s\">%s</span>",
		     _sv_solid_name(j),_sv_solid_name(j));
	  else
	    snprintf(buffer,sizeof(buffer),"%s",_sv_solid_name(j));
	  
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), buffer);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (_sv_panel1d_mapchange_callback), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p1->obj_table),menu,1,2,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	p1->map_pulldowns[i] = menu;
	_sv_solid_setup(&p1->mappings[i],0.,1.,0);
      }
      
      /* line pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_text();
	int j;
	for(j=0;j<LINETYPES;j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), line_name[j]->left);
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (_sv_panel1d_linetype_callback), p->objective_list+i);
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
			  G_CALLBACK (_sv_panel1d_pointtype_callback), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p1->obj_table),menu,3,4,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,5,0);
	p1->point_pulldowns[i] = menu;
      }
      
      /* alpha slider */
      {
	GtkWidget **sl = calloc(1, sizeof(*sl));
	sl[0] = _sv_slice_new(_sv_panel1d_alpha_callback,p->objective_list+i);
	
	gtk_table_attach(GTK_TABLE(p1->obj_table),sl[0],4,5,i,i+1,
			 GTK_EXPAND|GTK_FILL,0,0,0);
	
	p1->alpha_scale[i] = _sv_slider_new((_sv_slice_t **)sl,1,
					    (char *[]){"transparent","solid"},
					    (double []){0.,1.},
					    2,0);
	
	_sv_slider_set_gradient(p1->alpha_scale[i], &p1->mappings[i]);
	_sv_slice_thumb_set((_sv_slice_t *)sl[0],1.);
	
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
      sv_dim_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p1->dim_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* x radio buttons */
      if(!(d->flags & SV_DIM_NO_X) && !p1->link_x && !p1->link_y){
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
	_sv_dim_widget_new(p->dimension_list+i,_sv_panel1d_center_callback,_sv_panel1d_bracket_callback);
      
      gtk_table_attach(GTK_TABLE(p1->dim_table),
		       GTK_WIDGET(p->private->dim_scales[i]->t),
		       2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
    }
    
    for(i=0;i<p->dimensions;i++)
      if(p1->dim_xb[i])
	g_signal_connect (G_OBJECT (p1->dim_xb[i]), "toggled",
			  G_CALLBACK (_sv_panel1d_dimchange_callback), p);
    
    _sv_panel1d_update_xsel(p);
  }
  
  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_realize(GTK_WIDGET(p->private->spinner));
  gtk_widget_show_all(p->private->toplevel);

  _sv_undo_resume(p->sushi);
}


static int _sv_panel1d_save(sv_panel_t *p, xmlNodePtr pn){  
  _sv_panel1d_t *p1 = p->subtype->p1;
  int ret=0,i;

  xmlNodePtr n;

  xmlNewProp(pn, (xmlChar *)"type", (xmlChar *)"1d");

  // box
  if(p->private->oldbox_active){
    xmlNodePtr boxn = xmlNewChild(pn, NULL, (xmlChar *) "box", NULL);
    _xmlNewPropF(boxn, "x1", p1->oldbox[0]);
    _xmlNewPropF(boxn, "x2", p1->oldbox[1]);
  }
  
  // objective map settings
  for(i=0;i<p->objectives;i++){
    sv_obj_t *o = p->objective_list[i].o;

    xmlNodePtr on = xmlNewChild(pn, NULL, (xmlChar *) "objective", NULL);
    _xmlNewPropI(on, "position", i);
    _xmlNewPropI(on, "number", o->number);
    _xmlNewPropS(on, "name", o->name);
    _xmlNewPropS(on, "type", o->output_types);
    
    // right now Y is the only type; the below is Y-specific

    n = xmlNewChild(on, NULL, (xmlChar *) "y-map", NULL);
    _xmlNewMapProp(n, "color", _sv_solid_map(), p1->mappings[i].mapnum);
    _xmlNewMapProp(n, "line", line_name, p1->linetype[i]);    
    _xmlNewMapProp(n, "point", point_name, p1->pointtype[i]);    
    _xmlNewPropF(n, "alpha", _sv_slider_get_value(p1->alpha_scale[i],0));
  }

  // y scale
  n = xmlNewChild(pn, NULL, (xmlChar *) "range", NULL);
  _xmlNewPropF(n, "low-bracket", _sv_slider_get_value(p1->range_slider,0));
  _xmlNewPropF(n, "high-bracket", _sv_slider_get_value(p1->range_slider,1));

  // x/y dim selection
  n = xmlNewChild(pn, NULL, (xmlChar *) "axes", NULL);
  _xmlNewPropI(n, "xpos", p1->x_dnum);

  return ret;
}

int _sv_panel1d_load(sv_panel_t *p,
		     _sv_panel_undo_t *u,
		     xmlNodePtr pn,
		     int warn){
  int i;

  // check type
  _xmlCheckPropS(pn,"type","1d", "Panel %d type mismatch in save file.",p->number,&warn);
  
  // box
  u->box_active = 0;
  _xmlGetChildPropFPreserve(pn, "box", "x1", &u->box[0]);
  _xmlGetChildPropFPreserve(pn, "box", "x2", &u->box[1]);

  xmlNodePtr n = _xmlGetChildS(pn, "box", NULL, NULL);
  if(n){
    u->box_active = 1;
    xmlFree(n);
  }
  
  // objective map settings
  for(i=0;i<p->objectives;i++){
    sv_obj_t *o = p->objective_list[i].o;
    xmlNodePtr on = _xmlGetChildI(pn, "objective", "position", i);
    if(!on){
      _sv_first_load_warning(&warn);
      fprintf(stderr,"No save data found for panel %d objective \"%s\".\n",p->number, o->name);
    }else{
      // check name, type
      _xmlCheckPropS(on,"name",o->name, "Objectve position %d name mismatch in save file.",i,&warn);
      _xmlCheckPropS(on,"type",o->output_types, "Objectve position %d type mismatch in save file.",i,&warn);
      
      // right now Y is the only type; the below is Y-specific
      // load maptype, values
      int color = (u->mappings[i]>>24)&0xff;
      int line = (u->mappings[i]>>16)&0xff;
      int point = (u->mappings[i]>>8)&0xff;

      _xmlGetChildMapPreserve(on, "y-map", "color", _sv_solid_map(), &color,
		     "Panel %d objective unknown mapping setting", p->number, &warn);
      _xmlGetChildMapPreserve(on, "y-map", "line", line_name, &line,
		     "Panel %d objective unknown mapping setting", p->number, &warn);
      _xmlGetChildMapPreserve(on, "y-map", "point", point_name, &point,
		     "Panel %d objective unknown mapping setting", p->number, &warn);
      _xmlGetChildPropF(on, "y-map", "alpha", &u->scale_vals[2][i]);

      u->mappings[i] = (color<<24) | (line<<16) | (point<<8);

      xmlFreeNode(on);
    }
  }

  // y scale
  _xmlGetChildPropFPreserve(pn, "range", "low-bracket", &u->scale_vals[0][0]);
  _xmlGetChildPropF(pn, "range", "high-bracket", &u->scale_vals[1][0]);

  // x/y dim selection
  _xmlGetChildPropI(pn, "axes", "xpos", &u->x_d);

  return warn;
}

sv_panel_t *sv_panel_new_1d(sv_instance_t *s,
			    int number,
			    char *name,
			    sv_scale_t *scale,
			    sv_obj_t **objectives,
			    sv_dim_t **dimensions,	
			    unsigned flags){
  
  sv_panel_t *p = _sv_panel_new(s,number,name,objectives,dimensions,flags);
  _sv_panel1d_t *p1;

  if(!p)return p;
  p1 = calloc(1, sizeof(*p1));
  p->subtype = calloc(1, sizeof(*p->subtype));

  p->subtype->p1 = p1;
  p->type = SV_PANEL_1D;
  p1->range_scale = (sv_scale_t *)scale;
  p->private->bg_type = SV_BG_WHITE;

  if(p->flags && SV_PANEL_FLIP)
    p1->flip=1;

  p->private->realize = _sv_panel1d_realize;
  p->private->map_action = _sv_panel1d_map_redraw;
  p->private->legend_action = _sv_panel1d_legend_redraw;
  p->private->compute_action = _sv_panel1d_compute;
  p->private->request_compute = _sv_panel1d_mark_recompute;
  p->private->crosshair_action = _sv_panel1d_crosshair_callback;
  p->private->print_action = _sv_panel1d_print;
  p->private->save_action = _sv_panel1d_save;
  p->private->load_action = _sv_panel1d_load;

  p->private->undo_log = _sv_panel1d_undo_log;
  p->private->undo_restore = _sv_panel1d_undo_restore;
  p->private->def_oversample_n = p->private->oversample_n = 1;
  p->private->def_oversample_d = p->private->oversample_d = 8;
  
  return p;
}

int sv_panel_link_1d (sv_panel_t *p,
		      sv_panel_t *panel_2d,
		      unsigned flags){


  if(!p){
    fprintf(stderr,"Cannot link a NULL 1d panel\n");
    errno = -EINVAL;
    return errno;
  }

  if(!panel_2d){
    fprintf(stderr,"Panel %d (\"%s\"): Attempted to link NULL panel\n",
	    p->number,p->name);
    errno = -EINVAL;
    return errno;
  }

  if(panel_2d->type != SV_PANEL_2D){
    fprintf(stderr,"Panel %d (\"%s\"): Can only link to a 2d paenl\n",
	    p->number,p->name);
    errno = -EINVAL;
    return errno;
  }

  if(panel_2d->sushi != p->sushi){
    fprintf(stderr,"Panel %d (\"%s\"): Cannot link panel number %d (\"%s\") from a differnet instance\n",
	    p->number,p->name,panel_2d->number, panel_2d->name);
    errno = -EINVAL;
    return errno;
  }

  _sv_panel1d_t *p1 = p->subtype->p1;

  if(flags && SV_PANEL_LINK_Y)
    p1->link_y = (sv_panel_t *)panel_2d;
  else
    p1->link_x = (sv_panel_t *)panel_2d;

  return 0;
}
