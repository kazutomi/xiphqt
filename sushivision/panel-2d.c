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

static void render_checks(int w, int y, u_int32_t *render){
  int x,j;
  /* default checked background */
  /* 16x16 'mid-checks' */
  int phase = (y>>4)&1;
  for(x=0;x<w;){
    u_int32_t phaseval = 0x505050;
    if(phase) phaseval = 0x808080;
    for(j=0;j<16 && x<w;j++,x++)
      render[x] = phaseval;
    phase=!phase;
  }
}

static void _sushiv_panel2d_remap(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);

  int w,h,x,y,i;
  w = p2->data_w;
  h = p2->data_h;

  if(plot){
    u_int32_t render[w];

    /* iterate */
    /* by line */
    for(y = 0; y<h; y++){
   
      render_checks(w,y,render);

      /* by objective */
      for(i=0;i<p->objectives;i++){
	double *data_rect = p2->data_rect[i] + y*w;
	double alpha = p2->alphadel[i];

	/* by x */
	for(x=0;x<w;x++){
	  double val = data_rect[x];
	  
	  /* map/render result */
	  if(!isnan(val) && val>=alpha)
	    render[x] = mapping_calc(p2->mappings+i,val,render[x]);
	}
      }
      
      /* store result in panel */
      memcpy(plot->datarect+y*w,render,w*sizeof(*render));
    }
  }
}

static void _sushiv_panel2d_map_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter (); // misuse me as a global mutex
  
  _sushiv_panel2d_remap(p);
  if(plot)
    plot_expose_request(plot);
 
  gdk_threads_leave (); // misuse me as a global mutex
}

static void _sushiv_panel2d_legend_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  if(plot)
    plot_draw_scales(plot);
}

static int ilog10(int x){
  int count=0;
  if(x<0)x=-x;
  while(x){
    count++;
    x/=10;
  }
  return count;
}

static void update_legend(sushiv_panel_t *p){  
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter ();
  int w = p2->data_w;
  int h = p2->data_h;
  int x = plot_get_crosshair_xpixel(plot);
  int y = plot_get_crosshair_ypixel(plot);
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

    // one space 
    plot_legend_add(plot,NULL);

    // add each active objective to the legend
    // choose the value under the crosshairs 
    for(i=0;i<p->objectives;i++){
      float val=NAN;

      if(p2->data_rect && p2->data_rect[i] &&
	 x<w && x>0 &&
	 y<h && y>0 )
	val = p2->data_rect[i][y*w+x];

      if(!isnan(val) && val >= p2->alphadel[i]){
	
	val = slider_del_to_val(p2->range_scales[i],val);
	
	if(!isnan(val) && !mapping_inactive_p(p2->mappings+i)){
	  snprintf(buffer,320,"%s = %f",
		   p->objective_list[i].o->name,
		   val);
	  plot_legend_add(plot,buffer);
	}
      }
    }
    gdk_threads_leave ();

    _sushiv_panel_dirty_legend(p);

  }
}

static void mapchange_callback_2d(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  //sushiv_objective_t *o = optr->o;
  sushiv_panel_t *p = optr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int onum = optr - p->objective_list;

  _sushiv_panel_undo_push(p);
  _sushiv_panel_undo_suspend(p);

  mapping_set_func(&p2->mappings[onum],gtk_combo_box_get_active(GTK_COMBO_BOX(w)));
  
  //redraw the map slider
  slider_draw_background(p2->range_scales[onum]);
  slider_draw(p2->range_scales[onum]);
  slider_expose(p2->range_scales[onum]);
    
  update_legend(p);

  //redraw the plot
  _sushiv_panel_dirty_map(p);
  _sushiv_panel_undo_resume(p);
}

static void map_callback_2d(void *in,int buttonstate){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  //sushiv_objective_t *o = optr->o;
  sushiv_panel_t *p = optr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int onum = optr - p->objective_list;

  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  // recache alpha del */
  p2->alphadel[onum] = 
    slider_val_to_del(p2->range_scales[onum],
		      slider_get_value(p2->range_scales[onum],1));

  //redraw the plot
  _sushiv_panel_dirty_map(p);
  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void update_xy_availability(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;
  // update which x/y buttons are pressable */
  // enable/disable dimension slider thumbs

  for(i=0;i<p->dimensions;i++){
    if(p2->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))){
      // make the y insensitive
      if(p2->dim_yb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_yb[i],FALSE);

      // set the x dim flag
      p2->x_d = p->dimension_list[i].d;
      p2->x_scale = p->private->dim_scales[i];
      p2->x_dnum = i;
      // set panel x scale to this dim
      p2->x = scalespace_linear(p2->x_d->bracket[0],
				p2->x_d->bracket[1],
				p2->data_w,
				PLOT(p->private->graph)->scalespacing,
				p2->x_d->name);
    }else{
      // if there is a y, make it sensitive 
      if(p2->dim_yb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_yb[i],TRUE);
    }
    if(p2->dim_yb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]))){
      // make the x insensitive
      if(p2->dim_xb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_xb[i],FALSE);

      // set the y dim
      p2->y_d = p->dimension_list[i].d;
      p2->y_scale = p->private->dim_scales[i];
      p2->y_dnum = i;
      // set panel y scale to this dim
      p2->y = scalespace_linear(p2->y_d->bracket[0],
				p2->y_d->bracket[1],
				p2->data_h,
				PLOT(p->private->graph)->scalespacing,
				p2->y_d->name);
    }else{
      // if there is a x, make it sensitive 
      if(p2->dim_xb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_xb[i],TRUE);
    }
    if((p2->dim_xb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))) ||
       (p2->dim_yb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i])))){
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

static void compute_one_line_2d(sushiv_panel_t *p, 
				int serialno,
				int y, 
				int x_d, 
				double x_min, 
				double x_max, 
				int w, 
				double *dim_vals, 
				u_int32_t *render){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  double work[w];
  double inv_w = 1./w;
  int i,j;

  render_checks(w,y,render);
  gdk_threads_enter (); // misuse me as a global mutex
  
  /* by objective */
  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = p->objective_list[i].o;
    double alpha = p2->alphadel[i];
    
    gdk_threads_leave (); // misuse me as a global mutex
    
    /* by x */
    for(j=0;j<w;j++){

      /* compute value for this objective for this pixel */
      dim_vals[x_d] = (x_max-x_min) * inv_w * j + x_min;
      work[j] = o->callback(dim_vals);
      
    }
    
    gdk_threads_enter (); // misuse me as a global mutex
    if(p2->serialno == serialno){
      
      /* map/render result */
      for(j=0;j<w;j++){
	double val = slider_val_to_del(p2->range_scales[i],work[j]);
	work[j] = val;
	
	if(!isnan(val) && val>=alpha)
	  render[j] = mapping_calc(p2->mappings+i,val,render[j]);
	
      }
      
      /* store result in panel */
      memcpy(p2->data_rect[i]+y*w,work,w*sizeof(*work));
    }else
      break;
  }
  gdk_threads_leave (); // misuse me as a global mutex 
}

static int v_swizzle(int y, int height){
  int yy = height >> 5;
  if(y < yy)
    return (y<<5)+31;

  y -= yy;
  yy = (height+16) >> 5;
  if(y < yy)
    return (y<<5)+15;

  y -= yy;
  yy = (height+8) >> 4;
  if(y < yy)
    return (y<<4)+7;

  y -= yy;
  yy = (height+4) >> 3;
  if(y < yy)
    return (y<<3)+3;

  y -= yy;
  yy = (height+2) >> 2;
  if(y < yy)
    return (y<<2)+1;

  y -= yy;
  return y<<1;
}

// assumes data is locked
static void fast_scale_x(double *data, 
			 int w,
			 int h,
			 scalespace new,
			 scalespace old){
  int x,y;
  double work[w];
  int mapbase[w];
  double mapdel[w];

  double old_w = old.pixels;
  double new_w = new.pixels;
  double old_lo = scalespace_value(&old,0);
  double old_hi = scalespace_value(&old,old_w);
  double new_lo = scalespace_value(&new,0);
  double new_hi = scalespace_value(&new,new_w);
  double newscale = (new_hi-new_lo)/new_w;
  double oldscale = old_w/(old_hi-old_lo);
  for(x=0;x<w;x++){
    double xval = x*newscale+new_lo;
    double map = ((xval-old_lo)*oldscale);
    mapbase[x]=(int)floor(map);
    mapdel[x]=map-floor(map);
  }

  for(y=0;y<h;y++){
    double *data_line = data+y*w;
    for(x=0;x<w;x++){
      if(mapbase[x]<0 || mapbase[x]>=(w-1)){
	work[x]=NAN;
      }else{
	int base = mapbase[x];
	double del = mapdel[x];
	double A = data_line[base];
	double B = data_line[base+1];
	if(isnan(A) || isnan(B)) // damn you SIGFPE
	  work[x]=NAN;
	else
	  work[x]= A - A*del + B*del;
	
      }
    }
    memcpy(data_line,work,w*(sizeof(*work)));
  }   
}

static void fast_scale_y(double *data, 
			 int w,
			 int h,
			 scalespace new,
			 scalespace old){
  int x,y;
  double work[h];
  int mapbase[h];
  double mapdel[h];

  double old_h = old.pixels;
  double new_h = new.pixels;
  double old_lo = scalespace_value(&old,old_h);
  double old_hi = scalespace_value(&old,0);
  double new_lo = scalespace_value(&new,new_h);
  double new_hi = scalespace_value(&new,0);
  double newscale = (new_hi-new_lo)/new_h;
  double oldscale = old_h/(old_hi-old_lo);
  
  for(y=0;y<h;y++){
    double yval = y*newscale+new_lo;
    double map = ((yval-old_lo)*oldscale);
    mapbase[y]=(int)floor(map);
    mapdel[y]=map-floor(map);
  }
  
  for(x=0;x<w;x++){
    double *data_column = data+x;
    int stride = w;
    for(y=0;y<h;y++){
      if(mapbase[y]<0 || mapbase[y]>=(h-1)){
	work[y]=NAN;
      }else{
	int base = mapbase[y]*stride;
	double del = mapdel[y];
	double A = data_column[base];
	double B = data_column[base+stride];
	
	if(isnan(A) || isnan(B)) // damn you SIGFPE
	  work[y]=NAN;
	else
	  work[y]= A - A*del + B*del;
	
      }
    }
    for(y=0;y<h;y++){
      *data_column = work[y];
      data_column+=stride;
    }
  }   
}

static void fast_scale(double *newdata, 
		       scalespace xnew,
		       scalespace ynew,
		       double *olddata,
		       scalespace xold,
		       scalespace yold){
  int y;
  
  int new_w = xnew.pixels;
  int new_h = ynew.pixels;
  int old_w = xold.pixels;
  int old_h = yold.pixels;

  if(new_w > old_w){
    if(new_h > old_h){
      // copy image to new, scale there
      for(y=0;y<old_h;y++){
	double *new_line = newdata+y*new_w;
	double *old_line = olddata+y*old_w;
	memcpy(new_line,old_line,old_w*(sizeof*new_line));
      }
      fast_scale_x(newdata,new_w,new_h,xnew,xold);
      fast_scale_y(newdata,new_w,new_h,ynew,yold);
    }else{
      // scale y in old pane, copy to new, scale x 
      fast_scale_y(olddata,old_w,old_h,ynew,yold);
      for(y=0;y<new_h;y++){
	double *new_line = newdata+y*new_w;
	double *old_line = olddata+y*old_w;
	memcpy(new_line,old_line,old_w*(sizeof*new_line));
      }
      fast_scale_x(newdata,new_w,new_h,xnew,xold);
    }
  }else{
    if(new_h > old_h){
      // scale x in old pane, o=copy to new, scale y
      fast_scale_x(olddata,old_w,old_h,xnew,xold);
      for(y=0;y<old_h;y++){
	double *new_line = newdata+y*new_w;
	double *old_line = olddata+y*old_w;
	memcpy(new_line,old_line,new_w*(sizeof*new_line));
      }
      fast_scale_y(newdata,new_w,new_h,ynew,yold);
    }else{
      // scale in old pane, copy to new 
      // also the case where newdata == olddata and the size is unchanged
      fast_scale_x(olddata,old_w,old_h,xnew,xold);
      fast_scale_y(olddata,old_w,old_h,ynew,yold);
      if(olddata != newdata){
	for(y=0;y<new_h;y++){
	  double *new_line = newdata+y*new_w;
	  double *old_line = olddata+y*old_w;
	  memcpy(new_line,old_line,new_w*(sizeof*new_line));
	}
      }
    }
  }
}

// call only from main gtk thread!
static void _mark_recompute_2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  int w = plot->w.allocation.width;
  int h = plot->w.allocation.height;

  //_sushiv_panel1d_mark_recompute_linked(p);    XXXX

  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){
    if(p2->data_w != plot->w.allocation.width ||
       p2->data_h != plot->w.allocation.height){
      if(p2->data_rect){
	
	// make new rects, do a fast/dirty scaling job from old to new
	int i;
	for(i=0;i<p->objectives;i++){
	  double *new_rect = malloc(w * h* sizeof(**p2->data_rect));

	  fast_scale(new_rect,plot->x,plot->y,p2->data_rect[i],p2->x,p2->y);

	  free(p2->data_rect[i]);
	  p2->data_rect[i] = new_rect;
	}
	p2->x = plot->x;
	p2->y = plot->y;
	p2->data_w = w;
	p2->data_h = h;
	_sushiv_panel2d_map_redraw(p);
      }
    }
    
    p2->serialno++;
    p2->last_line = 0;
    
    if(!p2->data_rect){
      int i,j;
      // allocate it
      p2->data_w = w;
      p2->data_h = h;
      p2->x = scalespace_linear(p2->x_d->bracket[0],
				p2->x_d->bracket[1],
				w,
				PLOT(p->private->graph)->scalespacing,
				p2->x_d->name);
      p2->y = scalespace_linear(p2->y_d->bracket[0],
				p2->y_d->bracket[1],
				h,
				PLOT(p->private->graph)->scalespacing,
				p2->y_d->name);
      p2->data_rect = calloc(p->objectives,sizeof(*p2->data_rect));
      for(i=0;i<p->objectives;i++)
	p2->data_rect[i] = malloc(p2->data_w * p2->data_h* sizeof(**p2->data_rect));
      
      // blank it 
      for(i=0;i<p->objectives;i++)
	for(j=0;j<p2->data_w*p2->data_h;j++)
	  p2->data_rect[i][j]=NAN;
      _sushiv_panel2d_map_redraw(p);
    }

    _sushiv_wake_workers();
  }
}

static void recompute_callback_2d(void *ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  _mark_recompute_2d(p);
}

static void update_crosshairs(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  double x=0,y=0;
  int i;
  
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    if(d == p2->x_d)
      x = slider_get_value(p->private->dim_scales[i],1);
    if(d == p2->y_d)
      y = slider_get_value(p->private->dim_scales[i],1);
    
  }
  
  plot_set_crosshairs(PLOT(p->private->graph),x,y);

  // crosshairs snap to a pixel position; the cached dimension value
  // should be accurate with respect to the crosshairs
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    if(d == p2->x_d)
      d->val = scalespace_value(&plot->x,plot_get_crosshair_xpixel(plot));
    if(d == p2->y_d)
      d->val = scalespace_value(&plot->y,p2->data_h - plot_get_crosshair_ypixel(plot));
  }

  // _sushiv_panel1d_update_linked_crosshairs(p); XXXX

  update_legend(p);
}

static void dim_callback_2d(void *in, int buttonstate){
  sushiv_dimension_list_t *dptr = (sushiv_dimension_list_t *)in;
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  //Plot *plot = PLOT(p->private->graph);
  int dnum = dptr - p->dimension_list;
  int axisp = (d == p2->x_d || d == p2->y_d);
  double val = slider_get_value(p->private->dim_scales[dnum],1);
  int recursep = (val != d->val);

  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  d->val = slider_get_value(p->private->dim_scales[dnum],1);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _mark_recompute_2d(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    update_crosshairs(p);
  }

  /* dims can be shared amongst multiple panels; all must be updated */
  if(recursep)
    _sushiv_panel_update_shared_dimension(d,val);
  

  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void bracket_callback_2d(void *in, int buttonstate){
  sushiv_dimension_list_t *dptr = (sushiv_dimension_list_t *)in;
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int dnum = dptr - p->dimension_list;
  double lo = slider_get_value(p->private->dim_scales[dnum],0);
  double hi = slider_get_value(p->private->dim_scales[dnum],2);
  
  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  if(d->bracket[0] != lo || d->bracket[1] != hi){
    double xy_p = d == p2->x_d;
    scalespace s = scalespace_linear(lo,hi,(xy_p?p2->data_w:p2->data_h),
				     PLOT(p->private->graph)->scalespacing,
				     d->name);
    
    if(s.m == 0){
      if(xy_p)
	fprintf(stderr,"X scale underflow; cannot zoom further.\n");
      else
	fprintf(stderr,"Y scale underflow; cannot zoom further.\n");
    }else{
      xy_p?(p2->x=s):(p2->y=s);
      
      d->bracket[0] = lo;
      d->bracket[1] = hi;
      update_crosshairs(p);

      _mark_recompute_2d(p);
      _sushiv_panel_update_shared_bracket(d,lo,hi);
    }
  }
 
  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void dimchange_callback_2d(GtkWidget *button,gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    update_xy_availability(p);
    update_crosshairs(p);
    plot_unset_box(PLOT(p->private->graph));
    _mark_recompute_2d(p);

    _sushiv_panel_undo_resume(p);
  }
}

static void _sushiv_panel2d_crosshairs_callback(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  double x=PLOT(p->private->graph)->selx;
  double y=PLOT(p->private->graph)->sely;
  int i;
  
  _sushiv_panel_undo_push(p);
  _sushiv_panel_undo_suspend(p);

  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    if(d == p2->x_d){
      slider_set_value(p->private->dim_scales[i],1,x);

      // key bindings could move crosshairs out of the window; we
      // stretch in that case, which requires a recompute.
      bracket_callback_2d(p->dimension_list+i,1);
    }

    if(d == p2->y_d){
      slider_set_value(p->private->dim_scales[i],1,y);

      // key bindings could move crosshairs out of the window; we
      // stretch in that case, which requires a recompute.
      bracket_callback_2d(p->dimension_list+i,1);
    }

    p2->oldbox_active = 0;
  }
  _sushiv_panel_undo_resume(p);
}

static void box_callback(void *in, int state){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sushiv_panel_undo_push(p);
    plot_box_vals(plot,p2->oldbox);
    p2->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    _sushiv_panel2d_crosshairs_callback(p);

    slider_set_value(p2->x_scale,0,p2->oldbox[0]);
    slider_set_value(p2->x_scale,2,p2->oldbox[1]);
    slider_set_value(p2->y_scale,0,p2->oldbox[2]);
    slider_set_value(p2->y_scale,2,p2->oldbox[3]);
    p2->oldbox_active = 0;
    _sushiv_panel_undo_resume(p);
    break;
  }
  p->private->update_menus(p);
}

// called from one/all of the worker threads; the idea is that several
// of the threads will all call this and they collectively interleave
// ongoing computation of the pane
static int _sushiv_panel_cooperative_compute_2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot;
  
  int w,h,i,d;
  int serialno;
  double x_min, x_max;
  double y_min, y_max;
  double invh;
  int x_d=-1, y_d=-1;
  int render_scale_flag = 0;
  scalespace sx;
  scalespace sy;

  // lock during setup
  gdk_threads_enter ();
  w = p2->data_w;
  h = p2->data_h;
  sx = p2->x;
  sy = p2->y;

  if(p2->last_line>h){
    gdk_threads_leave ();
    return 0;
  }

  plot = PLOT(p->private->graph);

  if(p2->last_line==h){
    p2->last_line++;
    gdk_threads_leave ();
    plot_expose_request(plot);
    update_legend(p); 
    //_sushiv_panel1d_mark_recompute_linked(p);    
    return 0;
  }
  
  serialno = p2->serialno;
  invh = 1./h;
  d = p->dimensions;

  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[p->sushi->dimensions];

  /* render into temporary line; computation may be interrupted by
     events such as resizing, so we must be careful to simply assume
     the widget is unaltered between locks. This allows us to minimize
     checks. */
  u_int32_t render[w];

  x_min = scalespace_value(&sx,0);
  x_max = scalespace_value(&sx,w);
  x_d = p2->x_d->number;

  y_min = scalespace_value(&sy,h);
  y_max = scalespace_value(&sy,0);
  y_d = p2->y_d->number;

  // if the scale bound has changed, fast scale our background data to fill
  // the pane while new, more precise data renders.
  if(memcmp(&sx,&plot->x,sizeof(sx))){
    for(i=0;i<p->objectives;i++)
      fast_scale_x(p2->data_rect[i],w,h,
		   sx,plot->x);
    plot->x = sx;
    _sushiv_panel2d_remap(p);
  }
  if(memcmp(&sy,&plot->y,sizeof(sy))){
    for(i=0;i<p->objectives;i++)
      fast_scale_y(p2->data_rect[i],w,h,
		   sy,plot->y);
    plot->y = sy;
    _sushiv_panel2d_remap(p);
  }

  // Bulletproofing; shouldn't ever come up
  if(x_d == y_d || x_d==-1 || y_d==-1){
    gdk_threads_leave ();
    fprintf(stderr,"Invalid/missing x/y dimension setting in panel x_d=%d, y_d=%d\n",
	    x_d,y_d);
    return 0;
  }

  // Initialize local dimension value array
  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  // update scales if we're just starting
  if(p2->last_line==0) render_scale_flag = 1;

  /* iterate */
  /* by line */
  while(!_sushiv_exiting){
    int last = p2->last_line;
    int y;
    if(plot->w.allocation.height != h)break;
    if(last>=h)break;
    if(serialno != p2->serialno)break;
    p2->last_line++;

    /* unlock for computation */
    gdk_threads_leave ();

    if(render_scale_flag){
      plot_draw_scales(plot);
      render_scale_flag = 0;
    }

    y = v_swizzle(last,h);
    dim_vals[y_d]= (y_max - y_min) / h * y + y_min;

    /* compute line */
    compute_one_line_2d(p, serialno, y, x_d, x_min, x_max, w, dim_vals, render);

    /* move rendered line back into widget */
    gdk_threads_enter ();
    if(p2->serialno == serialno){
      u_int32_t *line = plot_get_background_line(plot, y);
      memcpy(line,render,w*sizeof(*render));
      plot_expose_request_line(plot,y);
    }else
      break;
  }
   
  gdk_threads_leave ();
  return 1;
}

static void panel2d_undo_log(sushiv_panel_undo_t *u){
  sushiv_panel_t *p = u->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;

  // alloc fields as necessary
  
  if(!u->mappings)
    u->mappings =  calloc(p->objectives,sizeof(*u->mappings));
  if(!u->obj_vals[0])
    u->obj_vals[0] =  calloc(p->objectives,sizeof(**u->obj_vals));
  if(!u->obj_vals[1])
    u->obj_vals[1] =  calloc(p->objectives,sizeof(**u->obj_vals));
  if(!u->obj_vals[2])
    u->obj_vals[2] =  calloc(p->objectives,sizeof(**u->obj_vals));
  if(!u->dim_vals[0])
    u->dim_vals[0] =  calloc(p->dimensions,sizeof(**u->dim_vals));
  if(!u->dim_vals[1])
    u->dim_vals[1] =  calloc(p->dimensions,sizeof(**u->dim_vals));
  if(!u->dim_vals[2])
    u->dim_vals[2] =  calloc(p->dimensions,sizeof(**u->dim_vals));

  // populate undo
  for(i=0;i<p->objectives;i++){
    u->mappings[i] = p2->mappings[i].mapnum;
    u->obj_vals[0][i] = slider_get_value(p2->range_scales[i],0);
    u->obj_vals[1][i] = slider_get_value(p2->range_scales[i],1);
    u->obj_vals[2][i] = slider_get_value(p2->range_scales[i],2);
  }

  for(i=0;i<p->dimensions;i++){
    u->dim_vals[0][i] = slider_get_value(p->private->dim_scales[i],0);
    u->dim_vals[1][i] = slider_get_value(p->private->dim_scales[i],1);
    u->dim_vals[2][i] = slider_get_value(p->private->dim_scales[i],2);
  }
  
  u->x_d = p2->x_dnum;
  u->y_d = p2->y_dnum;
  u->box[0] = p2->oldbox[0];
  u->box[1] = p2->oldbox[1];
  u->box[2] = p2->oldbox[2];
  u->box[3] = p2->oldbox[3];
  u->box_active = p2->oldbox_active;
}

static void panel2d_undo_restore(sushiv_panel_undo_t *u, int *remap_flag, int *recomp_flag){
  sushiv_panel_t *p = u->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  int i;
  
  *remap_flag=0;
  *recomp_flag=0;

  // go in through widgets
  for(i=0;i<p->objectives;i++){
    if(gtk_combo_box_get_active(GTK_COMBO_BOX(p2->range_pulldowns[i])) != u->mappings[i] ||
       slider_get_value(p2->range_scales[i],0)!=u->obj_vals[0][i] ||
       slider_get_value(p2->range_scales[i],1)!=u->obj_vals[1][i] ||
       slider_get_value(p2->range_scales[i],2)!=u->obj_vals[2][i]){ 
      *remap_flag = 1;
    }
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(p2->range_pulldowns[i]),u->mappings[i]);
    slider_set_value(p2->range_scales[i],0,u->obj_vals[0][i]);
    slider_set_value(p2->range_scales[i],1,u->obj_vals[1][i]);
    slider_set_value(p2->range_scales[i],2,u->obj_vals[2][i]);
  }

  for(i=0;i<p->dimensions;i++){
    /*if(slider_get_value(p->private->dim_scales[i],0)!=u->dim_vals[0][i] ||
       slider_get_value(p->private->dim_scales[i],2)!=u->dim_vals[2][i]){
      if(i==u->x_d || i==u->y_d)
	recomp_flag=1;
    }
    if(slider_get_value(p->private->dim_scales[i],0)!=u->dim_vals[1][i]){
      if(i!=u->x_d && i!=u->y_d)
	recomp_flag=1;
	}*/
    slider_set_value(p->private->dim_scales[i],0,u->dim_vals[0][i]);
    slider_set_value(p->private->dim_scales[i],1,u->dim_vals[1][i]);
    slider_set_value(p->private->dim_scales[i],2,u->dim_vals[2][i]);
  }

  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[u->x_d]))){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[u->x_d]),TRUE);
    *recomp_flag=1;
  }
  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[u->y_d]))){
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_yb[u->y_d]),TRUE);
    *recomp_flag=1;
  }

  update_xy_availability(p);

  if(u->box_active){
    plot_box_set(plot,u->box);
    p2->oldbox_active = 1;
  }else{
    plot_unset_box(plot);
    p2->oldbox_active = 0;
  }
}

// called with lock
static void panel2d_find_peak(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  int i,j;
  int w = p2->data_w;
  int h = p2->data_h;
  int n = w*h;
  int count = 0;
  
  // finds in order each peak (in the event there's more than one) of
  // each active objective
  while(1){

    for(i=0;i<p->objectives;i++){
      if(p2->data_rect && p2->data_rect[i] && !mapping_inactive_p(p2->mappings+i)){
	double *data=p2->data_rect[i];
	double best_val = data[0];
	double best_j = 0;
	int inner_count = count+1;
	
	for(j=1;j<n;j++){
	  if(!isnan(data[j])){
	    if(data[j]>best_val){
	      inner_count = count+1;
	      best_val = data[j];
	      best_j = j;
	    }else if (data[j]==best_val){
	      if(inner_count <= p2->peak_count){
		inner_count++;
		best_val = data[j];
		best_j = j;
	      }
	    }
	  }
	}
	 
	count = inner_count;
	if(count>p2->peak_count){
	  int y = best_j/w;
	  int x = best_j - y*w;
	  double xv = scalespace_value(&p2->x,x);
	  double yv = scalespace_value(&p2->y,h-y);
	  
	  plot_set_crosshairs(plot,xv,yv);
	  _sushiv_panel2d_crosshairs_callback(p);
	  
	  p2->peak_count++;
	  
	  return;
	}
      }
    }
    
    if(p2->peak_count==0)
      return; // must be all inactive
    else
      p2->peak_count=0;
  }
}

static gboolean panel2d_keypress(GtkWidget *widget,
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
    panel2d_find_peak(p);
    return TRUE;
  }

  return FALSE;
}

static void update_context_menus(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;

  // is undo active?
  if(!p->private->undo_stack ||
     !p->private->undo_level){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),0),FALSE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),0),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),0),TRUE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),0),TRUE);
  }

  // is redo active?
  if(!p->private->undo_stack ||
     !p->private->undo_stack[p->private->undo_level] ||
     !p->private->undo_stack[p->private->undo_level+1]){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),1),FALSE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),1),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),1),TRUE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),1),TRUE);
  }

  // are we starting or enacting a zoom box?
  if(p2->oldbox_active){ 
    gtk_menu_alter_item_label(GTK_MENU(p2->popmenu),3,"Zoom to box");
  }else{
    gtk_menu_alter_item_label(GTK_MENU(p2->popmenu),3,"Start zoom box");
  }

}

static void wrap_exit(sushiv_panel_t *dummy){
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

static void wrap_enter(sushiv_panel_t *p){
  plot_do_enter(PLOT(p->private->graph));
}

static void wrap_escape(sushiv_panel_t *p){
  plot_do_escape(PLOT(p->private->graph));
}

static char *graph_menulist[]={
  "Undo",
  "Redo",
  "",
  "Start zoom box",
  "Clear box and crosshairs",
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
  &panel2d_find_peak,
  NULL,
  &wrap_exit,
  NULL,
};


static void _sushiv_realize_panel2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;

  _sushiv_panel_undo_suspend(p);

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sushiv_clean_exit), (void *)SIGINT);
 
  p2->top_table = gtk_table_new(2 + p->objectives,5,0);
  gtk_container_add (GTK_CONTAINER (p->private->toplevel), p2->top_table);
  gtk_container_set_border_width (GTK_CONTAINER (p->private->toplevel), 5);
  
  p2->dim_table = gtk_table_new(p->dimensions,6,0);
  gtk_table_attach(GTK_TABLE(p2->top_table),p2->dim_table,0,5,1+p->objectives,2+p->objectives,
		   GTK_EXPAND|GTK_FILL,0,0,5);
  
  /* graph */
  p->private->graph = GTK_WIDGET(plot_new(recompute_callback_2d,p,
				  (void *)(void *)_sushiv_panel2d_crosshairs_callback,p,
				  box_callback,p)); 
  gtk_table_attach(GTK_TABLE(p2->top_table),p->private->graph,0,5,0,1,
		   GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,5);

  /* objective sliders */
  p2->range_scales = calloc(p->objectives,sizeof(*p2->range_scales));
  p2->range_pulldowns = calloc(p->objectives,sizeof(*p2->range_pulldowns));
  p2->alphadel = calloc(p->objectives,sizeof(*p2->alphadel));
  p2->mappings = calloc(p->objectives,sizeof(*p2->mappings));
  for(i=0;i<p->objectives;i++){
    GtkWidget **sl = calloc(3,sizeof(*sl));
    sushiv_objective_t *o = p->objective_list[i].o;
    int lo = o->scale->val_list[0];
    int hi = o->scale->val_list[o->scale->vals-1];

    /* label */
    GtkWidget *label = gtk_label_new(o->name);
    gtk_table_attach(GTK_TABLE(p2->top_table),label,0,1,i+1,i+2,
		     0,0,10,0);
    
    /* mapping pulldown */
    {
      GtkWidget *menu=gtk_combo_box_new_markup();
      int j;
      for(j=0;j<num_mappings();j++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (menu), "<i>testing</i>");//mapping_name(j));
      gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
      g_signal_connect (G_OBJECT (menu), "changed",
			G_CALLBACK (mapchange_callback_2d), p->objective_list+i);
      gtk_table_attach(GTK_TABLE(p2->top_table),menu,4,5,i+1,i+2,
		       GTK_SHRINK,GTK_SHRINK,5,0);
      p2->range_pulldowns[i] = menu;
    }

    /* the range mapping slices/slider */ 
    sl[0] = slice_new(map_callback_2d,p->objective_list+i);
    sl[1] = slice_new(map_callback_2d,p->objective_list+i);
    sl[2] = slice_new(map_callback_2d,p->objective_list+i);

    gtk_table_attach(GTK_TABLE(p2->top_table),sl[0],1,2,i+1,i+2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p2->top_table),sl[1],2,3,i+1,i+2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p2->top_table),sl[2],3,4,i+1,i+2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    p2->range_scales[i] = slider_new((Slice **)sl,3,o->scale->label_list,o->scale->val_list,
				    o->scale->vals,SLIDER_FLAG_INDEPENDENT_MIDDLE);

    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],lo);
    slice_thumb_set((Slice *)sl[2],hi);
    mapping_setup(&p2->mappings[i],0.,1.,0);
    slider_set_gradient(p2->range_scales[i], &p2->mappings[i]);
  }

  GtkWidget *first_x = NULL;
  GtkWidget *first_y = NULL;
  GtkWidget *pressed_y = NULL;
  p->private->dim_scales = calloc(p->dimensions,sizeof(*p->private->dim_scales));
  p2->dim_xb = calloc(p->dimensions,sizeof(*p2->dim_xb));
  p2->dim_yb = calloc(p->dimensions,sizeof(*p2->dim_yb));

  for(i=0;i<p->dimensions;i++){
    GtkWidget **sl = calloc(3,sizeof(*sl));
    sushiv_dimension_t *d = p->dimension_list[i].d;

    /* label */
    GtkWidget *label = gtk_label_new(d->name);
    gtk_table_attach(GTK_TABLE(p2->dim_table),label,0,1,i,i+1,
		     0,0,10,0);
    
    /* x/y radio buttons */
    if(!(d->flags & SUSHIV_NO_X)){
      if(first_x)
	p2->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
      else{
	first_x = p2->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]),TRUE);
      }
      gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_xb[i],1,2,i,i+1,
		       0,0,10,0);
    }
    
    if(!(d->flags & SUSHIV_NO_Y)){
      if(first_y)
	p2->dim_yb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_y),"Y");
      else
	first_y = p2->dim_yb[i] = gtk_radio_button_new_with_label(NULL,"Y");
      if(!pressed_y && p2->dim_xb[i]!=first_x){
	pressed_y = p2->dim_yb[i];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]),TRUE);
      }
      gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_yb[i],2,3,i,i+1,
		       0,0,10,0);
    }

    /* the dimension slices/slider */ 
    sl[0] = slice_new(bracket_callback_2d,p->dimension_list+i);
    sl[1] = slice_new(dim_callback_2d,p->dimension_list+i);
    sl[2] = slice_new(bracket_callback_2d,p->dimension_list+i);

    gtk_table_attach(GTK_TABLE(p2->dim_table),sl[0],3,4,i,i+1,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p2->dim_table),sl[1],4,5,i,i+1,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p2->dim_table),sl[2],5,6,i,i+1,
		     GTK_EXPAND|GTK_FILL,0,0,0);

    p->private->dim_scales[i] = slider_new((Slice **)sl,3,d->scale->label_list,d->scale->val_list,
					   d->scale->vals,0);

    slice_thumb_set((Slice *)sl[0],d->scale->val_list[0]);
    slice_thumb_set((Slice *)sl[1],0);
    slice_thumb_set((Slice *)sl[2],d->scale->val_list[d->scale->vals-1]);

  }
  for(i=0;i<p->dimensions;i++){
    if(p2->dim_xb[i])
      g_signal_connect (G_OBJECT (p2->dim_xb[i]), "toggled",
			  G_CALLBACK (dimchange_callback_2d), p);
    if(p2->dim_yb[i])
      g_signal_connect (G_OBJECT (p2->dim_yb[i]), "toggled",
			G_CALLBACK (dimchange_callback_2d), p);
  }
  update_xy_availability(p);

  p2->popmenu = gtk_menu_new_twocol(p->private->toplevel,
				    panel_menulist,
				    panel_shortlist,
				    (void *)(void *)panel_calllist,
				    p);
  p2->graphmenu = gtk_menu_new_twocol(p->private->graph,
				      graph_menulist,
				      graph_shortlist,
				      (void *)(void *)graph_calllist,
				      p);

  update_context_menus(p);

  g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
                    G_CALLBACK (panel2d_keypress), p);
  gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_show_all(p->private->toplevel);
  update_xy_availability(p); // yes, this was already done; however,
			     // gtk clobbered the event setup on the
			     // insensitive buttons when it realized
			     // them.  This call will restore them.

  _sushiv_panel_undo_resume(p);
}

int sushiv_new_panel_2d(sushiv_instance_t *s,
			int number,
			const char *name, 
			int *objectives,
			int *dimensions,
			unsigned flags){
  
  int i;
  int ret = _sushiv_new_panel(s,number,name,objectives,dimensions,flags);
  sushiv_panel_t *p;
  sushiv_panel2d_t *p2;

  if(ret<0)return ret;
  p = s->panel_list[number];
  p2 = calloc(1, sizeof(*p2));
  p->subtype = 
    calloc(1, sizeof(*p->subtype)); /* the union is alloced not
				       embedded as its internal
				       structure must be hidden */
  
  p->subtype->p2 = p2;
  p->type = SUSHIV_PANEL_2D;

  // verify all the objectives have scales
  for(i=0;i<p->objectives;i++){
    if(!p->objective_list[i].o->scale){
      fprintf(stderr,"All objectives in a 2d panel must have a scale\n");
      return -EINVAL;
    }
  }

  p->private->realize = _sushiv_realize_panel2d;
  p->private->map_redraw = _sushiv_panel2d_map_redraw;
  p->private->legend_redraw = _sushiv_panel2d_legend_redraw;
  p->private->compute_action = _sushiv_panel_cooperative_compute_2d;
  p->private->request_compute = _mark_recompute_2d;
  p->private->crosshair_action = _sushiv_panel2d_crosshairs_callback;

  p->private->undo_log = panel2d_undo_log;
  p->private->undo_restore = panel2d_undo_restore;
  p->private->update_menus = update_context_menus;
  return 0;
}

