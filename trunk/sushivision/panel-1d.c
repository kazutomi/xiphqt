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
#include <gdk/gdkkeysyms.h>
#include "internal.h"

#define LINETYPES 10
static char *line_name[LINETYPES+1] = {
  "line",
  "dot",
  "circle",
  "square",
  "cross",
  "triangle",
  "line and circle",
  "line and square",
  "line and cross",
  "line and triangle",
  NULL
};

static void update_context_menus(sushiv_panel_t *p);

static int ilog10(int x){
  int count=0;
  if(x<0)x=-x;
  while(x){
    count++;
    x/=10;
  }
  return count;
}

// called internally, assumes we hold lock
// redraws the data, does not compute the data
static void _sushiv_panel1d_remap(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  cairo_surface_t *cs = plot->back;
  cairo_t *c = cairo_create(cs);

  cairo_set_line_width(c,1.);

  if(plot){
    int xi,i;
    int dw = p1->data_size;
    double h = p1->panel_h;

    scalespace sx = (p1->flip?p1->y:p1->x);
    scalespace sy = (p1->flip?p1->x:p1->y);
    
    /* blank frame to black */
    cairo_set_source_rgb (c, 0,0,0);
    cairo_paint(c);

    /* do the panel and plot scales match?  If not, redraw the plot
       scales */
    if(memcmp(&sx,&plot->x,sizeof(sx)) |
       memcmp(&sy,&plot->y,sizeof(sy))){
      plot->x = sx;
      plot->y = sy;

      plot_draw_scales(plot);
    }

    /* by objective */
    if(p1->data_vec){
      for(i=0;i<p->objectives;i++){
	double *data_vec = p1->data_vec[i];
	if(data_vec){
	  double yprev=NAN,xprev=NAN;
	  
	  u_int32_t color = mapping_calc(p1->mappings+i,0,0);
	  
	  cairo_set_source_rgb(c,
			       ((color>>16)&0xff)/255.,
			       ((color>>8)&0xff)/255.,
			       ((color)&0xff)/255.);
	  
	  /* by x */
	  for(xi=0;xi<dw;xi++){
	    double val = data_vec[xi];
	    double xpixel = xi;
	    double ypixel = NAN;
	    
	    /* in linked panels, the data vector doesn't match the graph width; map */
	    if(p1->link_x || p1->link_y)
	      xpixel = scalespace_pixel(&p1->x,scalespace_value(&p1->vs,xpixel))+.5;
	    
	    /* map/render result */
	    if(!isnan(val))
	      ypixel = scalespace_pixel(&p1->y,val)+.5;
	    
	    if(!isnan(ypixel) && !isnan(yprev)){
	      if(p1->flip){
		cairo_move_to(c,yprev,h-xprev);
		cairo_line_to(c,ypixel,h-xpixel);
	      }else{
		cairo_move_to(c,xprev,h-yprev);
		cairo_line_to(c,xpixel,h-ypixel);
	      }
	      cairo_stroke(c);
	    }
	    
	    yprev=ypixel;
	    xprev=xpixel;
	  }
	}
      }
    }
  }
}

static void update_legend(sushiv_panel_t *p){  
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter ();
  int w = p1->panel_w;
  int h = p1->panel_h;
  int offset = ilog10(w>h?w:h);

  if(plot){
    int i;
    char buffer[320];
    plot_legend_clear(plot);

    // add each dimension to the legend
    for(i=0;i<p->dimensions;i++){
      // display decimal precision relative to bracket
      int depth = del_depth(p->dimension_list[i].d->bracket[0],
			    p->dimension_list[i].d->bracket[1]) + offset;
      snprintf(buffer,320,"%s = %.*f",
	       p->dimension_list[i].d->name,
	       depth,
	       p->dimension_list[i].d->val);
      plot_legend_add(plot,buffer);
    }

    // linked? add the linked dimension value to the legend
    if(p1->link_x || p1->link_y){
      sushiv_dimension_t *d;
      int depth;
      if(p1->link_x)
	d = p1->link_x->subtype->p2->x_d;
      else
	d = p1->link_y->subtype->p2->y_d;

      // display decimal precision relative to bracket
      depth = del_depth(d->bracket[0],
			d->bracket[1]) + offset;
      snprintf(buffer,320,"%s = %.*f",
	       d->name,
	       depth,
	       d->val);
      plot_legend_add(plot,buffer);
    }

    // one space 
    plot_legend_add(plot,NULL);

    // add each active objective to the legend
    // choose the value under the crosshairs 
    {
      double val = (p1->flip?plot->sely:plot->selx);
      int bin = scalespace_pixel(&p1->vs, val);
      u_int32_t color = mapping_calc(p1->mappings+i,0,0);
	  
      for(i=0;i<p->objectives;i++){

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
	
	plot_legend_add_with_color(plot,buffer,color);

      }
    }
    gdk_threads_leave ();
  }
}

void _sushiv_panel1d_map_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter (); // misuse me as a global mutex
  
  _sushiv_panel1d_remap(p);
  if(plot)
    plot_expose_request(plot);
 
  gdk_threads_leave (); // misuse me as a global mutex
}

void _sushiv_panel1d_legend_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter (); // misuse me as a global mutex
  update_legend(p);
  if(plot)
    plot_draw_scales(plot);
  gdk_threads_leave (); // misuse me as a global mutex
}

static void mapchange_callback_1d(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int onum = optr - p->objective_list;
  
  _sushiv_panel_undo_push(p);
  _sushiv_panel_undo_suspend(p);

  // update colormap
  // oh, the wasteful
  solid_set_func(&p1->mappings[onum],
		 gtk_combo_box_get_active(GTK_COMBO_BOX(w)));

  _sushiv_panel_dirty_map(p);
  _sushiv_panel_dirty_legend(p);
  _sushiv_panel_undo_resume(p);
}

static void linetype_callback_1d(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  sushiv_panel_t *p = optr->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int onum = optr - p->objective_list;
  
  _sushiv_panel_undo_push(p);
  _sushiv_panel_undo_suspend(p);

  // update colormap
  p1->linetype[onum]=gtk_combo_box_get_active(GTK_COMBO_BOX(w));

  _sushiv_panel_dirty_map(p);
  _sushiv_panel_undo_resume(p);
}

static void map_callback_1d(void *in,int buttonstate){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  
  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  // has new bracketing changed the plot range scale?
  if(p1->range_bracket[0] != slider_get_value(p1->range_slider,0) ||
     p1->range_bracket[1] != slider_get_value(p1->range_slider,1)){

    int w = plot->w.allocation.width;
    int h = plot->w.allocation.height;

    p1->range_bracket[0] = slider_get_value(p1->range_slider,0);
    p1->range_bracket[1] = slider_get_value(p1->range_slider,1);
    
    p1->y = scalespace_linear(p1->range_bracket[0],
			      p1->range_bracket[1],
			      (p1->flip?w:h),
			      PLOT(p->private->graph)->scalespacing,
			      p1->range_scale->legend);
  }

  //redraw the plot
  _sushiv_panel_dirty_map(p);
  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void update_x_sel(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int i;

  // enable/disable dimension slider thumbs
  
  for(i=0;i<p->dimensions;i++){
    
    if(p1->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p1->dim_xb[i]))){

      // set the x dim flag
      p1->x_d = p->dimension_list[i].d;
      p1->x_scale = p->private->dim_scales[i];
      p1->x_dnum = i;

      // set panel x scale to this dim
      p1->x = p1->vs = scalespace_linear(p1->x_d->bracket[0],
					 p1->x_d->bracket[1],
					 p1->data_size,
					 PLOT(p->private->graph)->scalespacing,
					 p1->x_d->name);

    }
    if(p1->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p1->dim_xb[i]))){
      // make all thumbs visible 
      slider_set_thumb_active(p->private->dim_scales[i],0,1);
      slider_set_thumb_active(p->private->dim_scales[i],2,1);
    }else{
      // make bracket thumbs invisible */
      slider_set_thumb_active(p->private->dim_scales[i],0,0);
      slider_set_thumb_active(p->private->dim_scales[i],2,0);
    }
  } 
}

static void compute_1d(sushiv_panel_t *p, 
		       int serialno,
		       int x_d, 
		       double x_min, 
		       double x_max, 
		       int w, 
		       double *dim_vals){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  double work[w];
  double inv_w = 1./w;
  int i,j;

  /* by objective */
  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = p->objective_list[i].o;
    
    /* by x */
    for(j=0;j<w;j++){
      
      /* compute value for this objective for this pixel */
      dim_vals[x_d] = (x_max-x_min) * inv_w * j + x_min;
      work[j] = o->callback(dim_vals);
      
    }
    
    gdk_threads_enter (); // misuse me as a global mutex
    if(p1->serialno == serialno){
      /* store result in panel */
      memcpy(p1->data_vec[i],work,w*sizeof(*work));
      gdk_threads_leave (); // misuse me as a global mutex 
    }else{
      gdk_threads_leave (); // misuse me as a global mutex 
      break;
    }
  }
}

// call only from main gtk thread
void _mark_recompute_1d(sushiv_panel_t *p){
  if(!p->private->realized) return;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  int w = plot->w.allocation.width;
  int h = plot->w.allocation.height;
  int dw = w;
  sushiv_panel_t *link = (p1->link_x ? p1->link_x : p1->link_y);
  sushiv_panel2d_t *p2 = (link?link->subtype->p2:NULL);
  int i,j;

  if(p1->link_x){
    dw = p2->data_w;
    p1->x_d = p2->x_d;
    p1->x_scale = p2->x_scale;
  }
  if(p1->link_y){
    dw = p2->data_h;
    p1->x_d = p2->y_d;
    p1->x_scale = p2->y_scale;
  }

  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){

    p1->x = scalespace_linear(p1->x_d->bracket[0],
			      p1->x_d->bracket[1],
			      (p1->flip?h:w),
			      PLOT(p->private->graph)->scalespacing,
			      p1->x_d->name);
    p1->y = scalespace_linear(p1->range_bracket[0],
			      p1->range_bracket[1],
			      (p1->flip?w:h),
			      PLOT(p->private->graph)->scalespacing,
			      p1->range_scale->legend);
    
    // 2d panels do not necessarily update their scales until
    // recompute time, and 1d panels may be recomputed first,
    // thus duplicate the scale computaiton here
    p1->vs = scalespace_linear(p1->x_d->bracket[0],
			       p1->x_d->bracket[1],
			       dw,
			       PLOT(p->private->graph)->scalespacing,
			       p1->x_d->name);


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

    p1->serialno++;
    p1->last_line = 0;
    _sushiv_wake_workers();
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
	x = slider_get_value(link->private->dim_scales[i],1);
    }
  }else{
    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;
      if(d == p1->x_d)
	x = slider_get_value(p->private->dim_scales[i],1);
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
	if(xflag)
	  update_crosshair(q);
	if(yflag)
	  q->private->request_compute(q);
      }else{
	if(q1->link_y == p){
	  if(yflag)
	    update_crosshair(q);
	  if(xflag)
	    q->private->request_compute(q);
	}
      }
    }
  }
}

static void dim_callback_1d(void *in, int buttonstate){
  sushiv_dimension_list_t *dptr = (sushiv_dimension_list_t *)in;
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  //Plot *plot = PLOT(p->private->graph);
  int dnum = dptr - p->dimension_list;
  int axisp = (d == p1->x_d);
  double val = slider_get_value(p->private->dim_scales[dnum],1);
  int recursep = (val != d->val);

  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  d->val = slider_get_value(p->private->dim_scales[dnum],1);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _mark_recompute_1d(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    update_crosshair(p);
  }

  /* dims can be shared amongst multiple panels; all must be updated */
  if(recursep)
    _sushiv_panel_update_shared_dimension(d,val);

  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void bracket_callback_1d(void *in, int buttonstate){
  sushiv_dimension_list_t *dptr = (sushiv_dimension_list_t *)in;
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int dnum = dptr - p->dimension_list;
  double lo = slider_get_value(p->private->dim_scales[dnum],0);
  double hi = slider_get_value(p->private->dim_scales[dnum],2);
  
  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  if(d->bracket[0] != lo || d->bracket[1] != hi){
    scalespace s = scalespace_linear(lo,hi,p1->data_size,
				     PLOT(p->private->graph)->scalespacing,
				     d->name);
    
    if(s.m == 0){
      fprintf(stderr,"X scale underflow; cannot zoom further.\n");
    }else{

      d->bracket[0] = lo;
      d->bracket[1] = hi;
      update_crosshair(p);

      _mark_recompute_1d(p);
      _sushiv_panel_update_shared_bracket(d,lo,hi);
    }
  }
 
  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void dimchange_callback_1d(GtkWidget *button,gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    update_x_sel(p);
    update_crosshair(p);
    plot_unset_box(PLOT(p->private->graph));
    _mark_recompute_1d(p);

    _sushiv_panel_undo_resume(p);
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

    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    for(i=0;i<p->dimensions;i++){
      sushiv_dimension_t *d = p->dimension_list[i].d;
      sushiv_panel1d_t *p1 = p->subtype->p1;
      if(d == p1->x_d){
	slider_set_value(p->private->dim_scales[i],1,x);
	
	// key bindings could move crosshairs out of the window; we
	// stretch in that case, which requires a recompute.
	bracket_callback_1d(p->dimension_list+i,1);
      }
            
      p1->oldbox_active = 0;
    }
    _sushiv_panel_undo_resume(p);
  }
}

static void box_callback(void *in, int state){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sushiv_panel_undo_push(p);
    plot_box_vals(plot,p1->oldbox);
    p1->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    crosshair_callback(p);

    slider_set_value(p1->x_scale,0,p1->oldbox[0]);
    slider_set_value(p1->x_scale,2,p1->oldbox[1]);
    p1->oldbox_active = 0;
    _sushiv_panel_undo_resume(p);
    break;
  }
  update_context_menus(p);
}

int _sushiv_panel_cooperative_compute_1d(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot;
  
  int dw,w,h,i,d;
  int serialno;
  double x_min, x_max;
  int x_d=-1;
  int render_scale_flag = 0;
  scalespace sx;
  scalespace sy;
  scalespace sv;

  // lock during setup
  gdk_threads_enter ();
  dw = p1->data_size;
  w = p1->panel_w;
  h = p1->panel_h;

  sx = p1->x;
  sy = p1->y;
  sv = p1->vs;
  
  if(p1->last_line){
    gdk_threads_leave ();
    return 0;
  }

  plot = PLOT(p->private->graph);
  
  serialno = p1->serialno;
  d = p->dimensions;

  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[p->sushi->dimensions];

  x_min = scalespace_value(&sv,0);
  x_max = scalespace_value(&sv,dw);
  x_d = p1->x_d->number;

  if(p1->flip){
    plot->x = sy;
    plot->y = sx;
  }else{
    plot->x = sx;
    plot->y = sy;
  }

  // Bulletproofing; shouldn't ever come up
  if(x_d==-1){
    gdk_threads_leave ();
    fprintf(stderr,"Invalid/missing x dimension setting in 1d panel x_d\n");
    return 0;
  }

  // Initialize local dimension value array
  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  // update scales if we're just starting
  if(p1->last_line==0) render_scale_flag = 1;

  if(plot->w.allocation.height == h &&
     serialno == p1->serialno){
    p1->last_line++;
    
    /* unlock for computation */
    gdk_threads_leave ();
    
    if(render_scale_flag){
      plot_draw_scales(plot);
      render_scale_flag = 0;
    }
    
    /* compute */
    compute_1d(p, serialno, x_d, x_min, x_max, dw, dim_vals);
    gdk_threads_enter ();
    _sushiv_panel_dirty_map(p);
    _sushiv_panel_dirty_legend(p);
    gdk_threads_leave ();

  }else
    gdk_threads_leave ();

  return 1;
}

static void panel1d_undo_log(sushiv_panel_undo_t *u){
  sushiv_panel_t *p = u->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int i;

  // alloc fields as necessary
  
  if(!u->mappings)
    u->mappings =  calloc(p->objectives,sizeof(*u->mappings));
  if(!u->submappings)
    u->submappings =  calloc(p->objectives,sizeof(*u->submappings));
  if(!u->scale_vals[0])
    u->scale_vals[0] =  calloc(1,sizeof(**u->scale_vals));
  if(!u->scale_vals[1])
    u->scale_vals[1] =  calloc(1,sizeof(**u->scale_vals));
  if(!u->scale_vals[2])
    u->scale_vals[2] =  calloc(1,sizeof(**u->scale_vals));
  if(!u->dim_vals[0])
    u->dim_vals[0] =  calloc(p->dimensions,sizeof(**u->dim_vals));
  if(!u->dim_vals[1])
    u->dim_vals[1] =  calloc(p->dimensions,sizeof(**u->dim_vals));
  if(!u->dim_vals[2])
    u->dim_vals[2] =  calloc(p->dimensions,sizeof(**u->dim_vals));

  // populate undo
  u->scale_vals[0][0] = slider_get_value(p1->range_slider,0);
  u->scale_vals[1][0] = slider_get_value(p1->range_slider,1);

  for(i=0;i<p->objectives;i++){
    u->mappings[i] = p1->mappings[i].mapnum;
    u->submappings[i] = p1->linetype[i];
  }

  for(i=0;i<p->dimensions;i++){
    u->dim_vals[0][i] = slider_get_value(p->private->dim_scales[i],0);
    u->dim_vals[1][i] = slider_get_value(p->private->dim_scales[i],1);
    u->dim_vals[2][i] = slider_get_value(p->private->dim_scales[i],2);
  }
  
  u->x_d = p1->x_dnum;
  u->box[0] = p1->oldbox[0];
  u->box[1] = p1->oldbox[1];
  u->box_active = p1->oldbox_active;
  
}

static void panel1d_undo_restore(sushiv_panel_undo_t *u, int *remap_flag, int *recomp_flag){
  sushiv_panel_t *p = u->p;
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  int i;
  
  *remap_flag=0;
  *recomp_flag=0;

  // go in through widgets
  if(slider_get_value(p1->range_slider,0)!=u->scale_vals[0][0] ||
     slider_get_value(p1->range_slider,1)!=u->scale_vals[1][0]){
    *remap_flag = 1;
  }
   
  slider_set_value(p1->range_slider,0,u->scale_vals[0][0]);
  slider_set_value(p1->range_slider,1,u->scale_vals[1][0]);

  for(i=0;i<p->objectives;i++){
    if(gtk_combo_box_get_active(GTK_COMBO_BOX(p1->map_pulldowns[i])) != u->mappings[i] ||
       gtk_combo_box_get_active(GTK_COMBO_BOX(p1->line_pulldowns[i])) != u->submappings[i]){
      *remap_flag = 1;
    }
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->map_pulldowns[i]),u->mappings[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(p1->line_pulldowns[i]),u->submappings[i]);
  }

  for(i=0;i<p->dimensions;i++){
    slider_set_value(p->private->dim_scales[i],0,u->dim_vals[0][i]);
    slider_set_value(p->private->dim_scales[i],1,u->dim_vals[1][i]);
    slider_set_value(p->private->dim_scales[i],2,u->dim_vals[2][i]);
  }

  if(p1->dim_xb && u->x_d<p->dimensions && p1->dim_xb[u->x_d]){
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p1->dim_xb[u->x_d]))){
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p1->dim_xb[u->x_d]),TRUE);
      *recomp_flag=1;
    }
  }

  update_x_sel(p);

  if(u->box_active){
    plot_box_set(plot,u->box);
    p1->oldbox_active = 1;
  }else{
    plot_unset_box(plot);
    p1->oldbox_active = 0;
  }
}

// called with lock
static void panel1d_find_peak(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  Plot *plot = PLOT(p->private->graph);
  int i,j;
  int dw = p1->data_size;
  int count = 0;
  
  // finds in order each peak (in the event there's more than one) of
  // each active objective
  while(1){
    
    for(i=0;i<p->objectives;i++){
      if(p1->data_vec && p1->data_vec[i] && !mapping_inactive_p(p1->mappings+i)){
	double *data=p1->data_vec[i];
	double best_val = data[0];
	double best_j = 0;
	int inner_count = count+1;
	
	for(j=1;j<dw;j++){
	  if(!isnan(data[j])){
	    if(data[j]>best_val){
	      inner_count = count+1;
	      best_val = data[j];
	      best_j = j;
	    }else if (data[j]==best_val){
	      if(inner_count <= p1->peak_count){
		inner_count++;
		best_val = data[j];
		best_j = j;
	      }
	    }
	  }
	}
	
	count = inner_count;
	if(count>p1->peak_count){
	  double xv = scalespace_value(&p1->vs,best_j);

	  if(p1->flip)
	    plot_set_crosshairs(plot,0,xv);
	  else
	    plot_set_crosshairs(plot,xv,0);
	  crosshair_callback(p);
	  
	  p1->peak_count++;
	  
	  return;
	}
      }
    }
    
    if(p1->peak_count==0)
      return; // must be all inactive
    else
      p1->peak_count=0;
  }
}
 
static gboolean panel1d_keypress(GtkWidget *widget,
				 GdkEventKey *event,
				 gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  //  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK)return FALSE;
  
  /* non-control keypresses */
  switch(event->keyval){
    
  case GDK_Q:
  case GDK_q:
    // quit
    _sushiv_clean_exit(SIGINT);
    return TRUE;

  case GDK_BackSpace:
    // undo 
    _sushiv_panel_undo_down(p);
    return TRUE;

  case GDK_r:
  case GDK_space:
    // redo/forward
    _sushiv_panel_undo_up(p);
    return TRUE;

  case GDK_p:
    // find [next] peak
    panel1d_find_peak(p);
    return TRUE;
  }

  return FALSE;
}

static void update_context_menus(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;

  // is undo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_level){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->popmenu),0),FALSE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->graphmenu),0),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->popmenu),0),TRUE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->graphmenu),0),TRUE);
  }

  // is redo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level] ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level+1]){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->popmenu),1),FALSE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->graphmenu),1),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->popmenu),1),TRUE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p1->graphmenu),1),TRUE);
  }

  // are we starting or enacting a zoom box?
  if(p1->oldbox_active){ 
    gtk_menu_alter_item_label(GTK_MENU(p1->graphmenu),3,"Zoom to selection");
  }else{
    gtk_menu_alter_item_label(GTK_MENU(p1->graphmenu),3,"Start zoom selection");
  }

}

void wrap_exit(sushiv_panel_t *dummy){
  _sushiv_clean_exit(SIGINT);
}

static char *panel_menulist[]={
  "Undo",
  "Redo",
  "",
  "Quit",
  NULL
};

static char *panel_shortlist[]={
  "Backspace",
  "Space",
  NULL,
  "q",
  NULL
};

static void (*panel_calllist[])(sushiv_panel_t *)={
  &_sushiv_panel_undo_down,
  &_sushiv_panel_undo_up,
  NULL,
  &wrap_exit,
  NULL,
};

void wrap_enter(sushiv_panel_t *p){
  plot_do_enter(PLOT(p->private->graph));
}

void wrap_escape(sushiv_panel_t *p){
  plot_do_escape(PLOT(p->private->graph));
}

static char *graph_menulist[]={
  "Undo",
  "Redo",
  "",
  "Start zoom selection",
  "Clear selection",
  "Find peaks",
  "",
  "Quit",
  NULL
};

static char *graph_shortlist[]={
  "Backspace",
  "Space",
  NULL,
  "Enter",
  "Escape",
  "p",
  NULL,
  "q",
  NULL
};

static void (*graph_calllist[])(sushiv_panel_t *)={
  &_sushiv_panel_undo_down,
  &_sushiv_panel_undo_up,
  NULL,

  &wrap_enter,
  &wrap_escape,
  &panel1d_find_peak,
  NULL,
  &wrap_exit,
  NULL,
};

void _sushiv_realize_panel1d(sushiv_panel_t *p){
  sushiv_panel1d_t *p1 = p->subtype->p1;
  int i;

  _sushiv_panel_undo_suspend(p);

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sushiv_clean_exit), (void *)SIGINT);
 
  p1->top_table = gtk_table_new(3,4,0);

  gtk_container_add (GTK_CONTAINER (p->private->toplevel), p1->top_table);
  gtk_container_set_border_width (GTK_CONTAINER (p->private->toplevel), 5);
  
  p1->obj_table = gtk_table_new(p->objectives,3,0);
  gtk_table_attach(GTK_TABLE(p1->top_table),p1->obj_table,0,4,2,3,
		   GTK_EXPAND|GTK_FILL,0,0,5);

  p1->dim_table = gtk_table_new(p->dimensions,4,0);
  gtk_table_attach(GTK_TABLE(p1->top_table),p1->dim_table,0,4,3,4,
		   GTK_EXPAND|GTK_FILL,0,0,5);
  
  /* graph */
  {
    unsigned flags = 0;
    if(p1->flip)
      flags |= PLOT_NO_X_CROSS;
    else
      flags |= PLOT_NO_Y_CROSS;
    p->private->graph = GTK_WIDGET(plot_new(recompute_callback_1d,p,
					    (void *)(void *)crosshair_callback,p,
					    box_callback,p,flags)); 
    gtk_table_attach(GTK_TABLE(p1->top_table),p->private->graph,0,4,0,1,
		     GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,5);
  }

  /* range slider */
  {
    GtkWidget **sl = calloc(2,sizeof(*sl));

    int lo = p1->range_scale->val_list[0];
    int hi = p1->range_scale->val_list[p1->range_scale->vals-1];

    /* label */
    {
      char *buf;
      asprintf(&buf,"%s range",p1->range_scale->legend);
      GtkWidget *label = gtk_label_new(buf);
      gtk_table_attach(GTK_TABLE(p1->top_table),label,0,1,1,2,
		       0,0,10,0);
      free(buf);
    }

    /* the range slices/slider */ 
    sl[0] = slice_new(map_callback_1d,p);
    sl[1] = slice_new(map_callback_1d,p);

    gtk_table_attach(GTK_TABLE(p1->top_table),sl[0],1,2,1,2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p1->top_table),sl[1],2,3,1,2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    p1->range_slider = slider_new((Slice **)sl,2,
				  p1->range_scale->label_list,
				  p1->range_scale->val_list,
				  p1->range_scale->vals,
				  SLIDER_FLAG_INDEPENDENT_MIDDLE);

    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],hi);
  }

  /* objective pulldowns */
  p1->linetype = calloc(p->objectives,sizeof(*p1->linetype));
  p1->mappings = calloc(p->objectives,sizeof(*p1->mappings));
  p1->map_pulldowns = calloc(p->objectives,sizeof(*p1->map_pulldowns));
  p1->line_pulldowns = calloc(p->objectives,sizeof(*p1->line_pulldowns));

  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = p->objective_list[i].o;

    /* label */
    GtkWidget *label = gtk_label_new(o->name);
    gtk_table_attach(GTK_TABLE(p1->obj_table),label,0,1,i,i+1,
		     0,0,10,0);
    
    /* mapping pulldown */
    {
      GtkWidget *menu=gtk_combo_box_new_markup();
      int j;
      for(j=0;j<num_solids();j++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (menu), solid_name(j));
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
	gtk_combo_box_append_text (GTK_COMBO_BOX (menu), line_name[j]);
      gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
      g_signal_connect (G_OBJECT (menu), "changed",
			G_CALLBACK (linetype_callback_1d), p->objective_list+i);
      gtk_table_attach(GTK_TABLE(p1->obj_table),menu,2,3,i,i+1,
		       GTK_SHRINK,GTK_SHRINK,5,0);
      p1->line_pulldowns[i] = menu;
    }
  }

  if(p->dimensions){
    p->private->dim_scales = calloc(p->dimensions,sizeof(*p->private->dim_scales));
    p1->dim_xb = calloc(p->dimensions,sizeof(*p1->dim_xb));
    GtkWidget *first_x = NULL;
  
    for(i=0;i<p->dimensions;i++){
      GtkWidget **sl = calloc(3,sizeof(*sl));
      sushiv_dimension_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->name);
      gtk_table_attach(GTK_TABLE(p1->dim_table),label,0,1,i,i+1,
		       0,0,10,0);
      
      /* x radio buttons */
      if(!(d->flags & SUSHIV_NO_X) && !p1->link_x && !p1->link_y){
	if(first_x)
	  p1->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
	else{
	  first_x = p1->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p1->dim_xb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p1->dim_table),p1->dim_xb[i],1,2,i,i+1,
			 0,0,10,0);
      }
    
      /* the dimension slices/slider */ 
      sl[0] = slice_new(bracket_callback_1d,p->dimension_list+i);
      sl[1] = slice_new(dim_callback_1d,p->dimension_list+i);
      sl[2] = slice_new(bracket_callback_1d,p->dimension_list+i);

      gtk_table_attach(GTK_TABLE(p1->dim_table),sl[0],2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p1->dim_table),sl[1],3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p1->dim_table),sl[2],4,5,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
      p->private->dim_scales[i] = slider_new((Slice **)sl,3,d->scale->label_list,d->scale->val_list,
					     d->scale->vals,0);
      
      slice_thumb_set((Slice *)sl[0],d->scale->val_list[0]);
      slice_thumb_set((Slice *)sl[1],0);
      slice_thumb_set((Slice *)sl[2],d->scale->val_list[d->scale->vals-1]);
    }

    for(i=0;i<p->dimensions;i++)
      if(p1->dim_xb[i])
	g_signal_connect (G_OBJECT (p1->dim_xb[i]), "toggled",
			  G_CALLBACK (dimchange_callback_1d), p);

    update_x_sel(p);
  }

  p1->popmenu = gtk_menu_new_twocol(p->private->toplevel,
				    panel_menulist,
				    panel_shortlist,
				    (void *)(void *)panel_calllist,
				    p);
  p1->graphmenu = gtk_menu_new_twocol(p->private->graph,
				      graph_menulist,
				      graph_shortlist,
				      (void *)(void *)graph_calllist,
				      p);

  update_context_menus(p);

  g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
                    G_CALLBACK (panel1d_keypress), p);
  gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_show_all(p->private->toplevel);

  _sushiv_panel_undo_resume(p);
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

  int ret = _sushiv_new_panel(s,number,name,objectives,(int []){-1},flags);
  sushiv_panel_t *p;
  sushiv_panel1d_t *p1;
  sushiv_panel_t *p2 = s->panel_list[link];

  if(ret<0)return ret;
  p = s->panel_list[number];
  p1 = calloc(1, sizeof(*p1));
  p->subtype = 
    calloc(1, sizeof(*p->subtype)); /* the union is alloced not
				       embedded as its internal
				       structure must be hidden */
  p->subtype->p1 = p1;
  p->type = SUSHIV_PANEL_1D;

  p1->range_scale = scale;

  if(flags && SUSHIV_LINK_Y)
    p1->link_y = p2;
  else
    p1->link_x = p2;

  if(p->flags && SUSHIV_FLIP)
    p1->flip=1;

  p->private->realize = _sushiv_realize_panel1d;
  p->private->map_redraw = _sushiv_panel1d_map_redraw;
  p->private->legend_redraw = _sushiv_panel1d_legend_redraw;
  p->private->compute_action = _sushiv_panel_cooperative_compute_1d;
  p->private->request_compute = _mark_recompute_1d;
  p->private->crosshair_action = crosshair_callback;

  p->private->undo_log = panel1d_undo_log;
  p->private->undo_restore = panel1d_undo_restore;
  p->private->update_menus = update_context_menus;
  
  return 0;
}

