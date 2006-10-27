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
#include "sushivision.h"
#include "mapping.h"
#include "plot.h"
#include "slice.h"
#include "slider.h"
#include "panel-2d.h"
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

/* called from idle handler only, and as such, we can be sure we're
   locked and uninterruptable */
void _sushiv_panel2d_map_redraw(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  Plot *plot = PLOT(p2->graph);

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
	    render[x] = mapping_calc(p2->mappings+i,val);

	}
      }
      
      /* store result in panel */
      memcpy(plot->datarect+y*w,render,w*sizeof(*render));
    }
    plot_expose_request(plot);
  }
}

static void mapchange_callback_2d(GtkWidget *w,gpointer in){
  sushiv_objective_t **optr = (sushiv_objective_t **)in;
  sushiv_objective_t *o = *optr;
  sushiv_panel_t *p = o->panel;
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  int onum = optr - p->objective_list;

  mapping_set_func(&p2->mappings[onum],gtk_combo_box_get_active(GTK_COMBO_BOX(w)));
  
  //redraw the map slider
  slider_draw_background(p2->range_scales[onum]);
  slider_draw(p2->range_scales[onum]);
  slider_expose(p2->range_scales[onum]);
    
  //redraw the plot
  _sushiv_panel_dirty_map(p);
}

static void map_callback_2d(void *in){
  sushiv_objective_t **optr = (sushiv_objective_t **)in;
  sushiv_objective_t *o = *optr;
  sushiv_panel_t *p = o->panel;
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  int onum = optr - p->objective_list;

  // recache alpha del */
  p2->alphadel[onum] = 
    slider_val_to_del(p2->range_scales[onum],
		      slider_get_value(p2->range_scales[onum],1));

  //redraw the plot
  _sushiv_panel_dirty_map(p);
}

static void update_xy_availability(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  int i;
  // update which x/y buttons are pressable */
  // enable/disable dimension slider thumbs
  for(i=0;i<p->dimensions;i++){
    if(p2->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))){
      // make the y insensitive
      if(p2->dim_yb[i])
	gtk_widget_set_sensitive(p2->dim_yb[i],FALSE);
      // set the dim x flag
      p->dimension_list[i]->flags |= SUSHIV_X_DIM;
    }else{
      // if there is a y, make it sensitive 
      if(p2->dim_yb[i])
	gtk_widget_set_sensitive(p2->dim_yb[i],TRUE);
      // unset dim x flag 
      p->dimension_list[i]->flags &= ~SUSHIV_X_DIM;
    }
    if(p2->dim_yb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]))){
      // make the x insensitive
      if(p2->dim_xb[i])
	gtk_widget_set_sensitive(p2->dim_xb[i],FALSE);
      // set the dim y flag
      p->dimension_list[i]->flags |= SUSHIV_Y_DIM;
    }else{
      // if there is a x, make it sensitive 
      if(p2->dim_xb[i])
	gtk_widget_set_sensitive(p2->dim_xb[i],TRUE);
      // unset dim y flag 
      p->dimension_list[i]->flags &= ~SUSHIV_Y_DIM;
    }
    if((p2->dim_xb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))) ||
       (p2->dim_yb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i])))){
      // make all thumbs visible 
      slider_set_thumb_active(p2->dim_scales[i],0,1);
      slider_set_thumb_active(p2->dim_scales[i],2,1);
    }else{
      // make bracket thumbs invisible */
      slider_set_thumb_active(p2->dim_scales[i],0,0);
      slider_set_thumb_active(p2->dim_scales[i],2,0);
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
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  double work[w];
  double inv_w = 1./w;
  int i,j;

  render_checks(w,y,render);
  gdk_threads_enter (); // misuse me as a global mutex
  
  /* by objective */
  for(i=0;i<p->objectives;i++){
    sushiv_objective_t *o = p->objective_list[i];
    double alpha = p2->alphadel[i];
    
    gdk_threads_leave (); // misuse me as a global mutex
    
    /* by x */
    for(j=0;j<w;j++){

      /* compute value for this objective for this pixel */
      dim_vals[x_d] = (x_max-x_min) * inv_w * j;
      work[j] = o->callback(dim_vals);
      
    }
    
    gdk_threads_enter (); // misuse me as a global mutex
    if(p2->serialno == serialno){
      
      /* map/render result */
      for(j=0;j<w;j++){
	double val = slider_val_to_del(p2->range_scales[i],work[j]);
	work[j] = val;
	
	if(!isnan(val) && val>=alpha)
	  render[j] = mapping_calc(p2->mappings+i,val);
	
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

// call only from main gtk thread!
void _mark_recompute_2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  Plot *plot = PLOT(p2->graph);

  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){
    if(p2->data_w != plot->w.allocation.width ||
       p2->data_h != plot->w.allocation.height){
      if(p2->data_rect){
	int i;
	for(i=0;i<p->objectives;i++)
	  free(p2->data_rect[i]);
	free(p2->data_rect);
	p2->data_rect = NULL;
      }
      
    }
    
    p2->data_w = plot->w.allocation.width;
    p2->data_h = plot->w.allocation.height;
    p2->serialno++;
    p2->last_line = 0;
    
    if(!p2->data_rect){
      int i;
      // allocate it
      p2->data_rect = calloc(p->objectives,sizeof(*p2->data_rect));
      for(i=0;i<p->objectives;i++)
	p2->data_rect[i] = malloc(p2->data_w * p2->data_h* sizeof(**p2->data_rect));
    }
    {
      int i,j;
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
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  double x=0,y=0;
  int i;
  
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i];
    sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
    if(d->flags & SUSHIV_X_DIM)
      x = slider_get_value(p2->dim_scales[i],1);
    if(d->flags & SUSHIV_Y_DIM)
      y = slider_get_value(p2->dim_scales[i],1);
    
  }
  
  plot_set_crosshairs(PLOT(p2->graph),x,y);
}

static void dim_callback_2d(void *in){
  sushiv_dimension_t **dptr = (sushiv_dimension_t **)in;
  sushiv_dimension_t *d = *dptr;
  sushiv_panel_t *p = d->panel;
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  int dnum = dptr - p->dimension_list;

  int axisp = (d->flags & SUSHIV_DIM_MASK);

  d->val = slider_get_value(p2->dim_scales[dnum],1);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _mark_recompute_2d(p);
    return;
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    update_crosshairs(p);
  }

}

static void bracket_callback_2d(void *in){
  sushiv_dimension_t **dptr = (sushiv_dimension_t **)in;
  sushiv_dimension_t *d = *dptr;
  sushiv_panel_t *p = d->panel;
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  int dnum = dptr - p->dimension_list;

  int axisp = (d->flags & SUSHIV_DIM_MASK);

  d->bracket[0] = slider_get_value(p2->dim_scales[dnum],0);
  d->bracket[1] = slider_get_value(p2->dim_scales[dnum],2);

  // if the bracketing of an axis dimension changed, rerender
  if(axisp)
    _mark_recompute_2d(p);
 
}

static void dimchange_callback_2d(GtkWidget *button,gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;

  update_xy_availability(p);
  update_crosshairs(p);
  _mark_recompute_2d(p);
}

static void crosshairs_callback(void *in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  double x=PLOT(p2->graph)->selx_val;
  double y=PLOT(p2->graph)->sely_val;
  int i;
  
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i];
    sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
    if(d->flags & SUSHIV_X_DIM)
      slider_set_value(p2->dim_scales[i],1,x);
    if(d->flags & SUSHIV_Y_DIM)
      slider_set_value(p2->dim_scales[i],1,y);
  }
}

// called from one/all of the worker threads; the idea is that several
// of the threads will all call this and they collectively interleave
// ongoing computation of the pane
int _sushiv_panel_cooperative_compute_2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  Plot *plot;

  int w,h,i,d;
  int serialno;
  double x_min, x_max;
  double y_min, y_max;
  double invh;
  int x_d=-1, y_d=-1;

  // lock during setup
  gdk_threads_enter ();
  w = p2->data_w;
  h = p2->data_h;

  if(p2->last_line>=h){
      gdk_threads_leave ();
      return 0;
  }

  plot = PLOT(p2->graph);
  serialno = p2->serialno;
  invh = 1./h;
  d = p->dimensions;

  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[d];

  /* render into temporary line; computation may be interrupted by
     events such as resizing, so we must be careful to simply assume
     the widget is unaltered between locks. This allows us to minimize
     checks. */
  u_int32_t render[w];

  /* which dim is our x?  Our y? */
  for(i=0;i<d;i++){
    sushiv_dimension_t *dim = p->dimension_list[i];
    if((dim->flags & SUSHIV_DIM_MASK) == SUSHIV_X_DIM ){
      x_d = dim->number;
      x_min = dim->bracket[0];
      x_max = dim->bracket[1];
      break;
    }
  }

  for(i=0;i<d;i++){
    sushiv_dimension_t *dim = p->dimension_list[i];
    if((dim->flags & SUSHIV_DIM_MASK) == SUSHIV_Y_DIM){
      y_d = dim->number;
      y_min = dim->bracket[0];
      y_max = dim->bracket[1];
      break;
    }
  }

  // Bulletproofing; shouldn't ever come up
  if(x_d == y_d || x_d==-1 || y_d==-1){
    gdk_threads_leave ();
    fprintf(stderr,"Invalid/missing x/y dimension setting in panel x_d=%d, y_d=%d\n",
	    x_d,y_d);
    return 0;
  }

  // Initialize local dimension value array
  for(i=0;i<d;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  // update scales if we're just starting
  if(p2->last_line==0){
    plot_set_x_scale(PLOT(p2->graph),x_min,x_max);
    plot_set_y_scale(PLOT(p2->graph),y_min,y_max); 
  }

  /* iterate */
  /* by line */
  while(!_sushiv_exiting){
    int last = p2->last_line;
    int y;
    if(plot->w.allocation.height != h)break;
    if(last>=h)break;
    p2->last_line++;

    /* unlock for computation */
    gdk_threads_leave ();
    y = v_swizzle(last,h);
    dim_vals[y_d]= (y_max - y_min) * h * y;

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

void _sushiv_realize_panel2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  int i;
  int lo = p->scale_val_list[0];
  int hi = p->scale_val_list[p->scale_vals-1];

  p2->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p2->toplevel), "delete-event",
			    G_CALLBACK (_sushiv_clean_exit), (void *)SIGINT);
 
  p2->top_table = gtk_table_new(2 + p->objectives,5,0);
  gtk_container_add (GTK_CONTAINER (p2->toplevel), p2->top_table);
  gtk_container_set_border_width (GTK_CONTAINER (p2->toplevel), 5);
  
  p2->dim_table = gtk_table_new(p->dimensions,6,0);
  gtk_table_attach(GTK_TABLE(p2->top_table),p2->dim_table,0,5,1+p->objectives,2+p->objectives,
		   GTK_EXPAND|GTK_FILL,0,0,5);
  
  /* graph */
  p2->graph = GTK_WIDGET(plot_new(recompute_callback_2d,p,
				  crosshairs_callback,p)); 
  gtk_table_attach(GTK_TABLE(p2->top_table),p2->graph,0,5,0,1,
		   GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,5);

  /* objective sliders */
  p2->range_scales = calloc(p->objectives,sizeof(*p2->range_scales));
  p2->alphadel = calloc(p->objectives,sizeof(*p2->alphadel));
  p2->mappings = calloc(p->objectives,sizeof(*p2->mappings));
  for(i=0;i<p->objectives;i++){
    GtkWidget **sl = calloc(3,sizeof(*sl));
    sushiv_objective_t *o = p->objective_list[i];

    /* label */
    GtkWidget *label = gtk_label_new(o->name);
    gtk_table_attach(GTK_TABLE(p2->top_table),label,0,1,i+1,i+2,
		     0,0,10,0);
    
    /* mapping pulldown */
    {
      GtkWidget *menu=gtk_combo_box_new_text();
      int j;
      for(j=0;j<num_mappings();j++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (menu), mapping_name(j));
      gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
      g_signal_connect (G_OBJECT (menu), "changed",
			G_CALLBACK (mapchange_callback_2d), p->objective_list+i);
      gtk_table_attach(GTK_TABLE(p2->top_table),menu,4,5,i+1,i+2,
		       GTK_SHRINK,GTK_SHRINK,5,0);
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
    p2->range_scales[i] = slider_new((Slice **)sl,3,p->scale_label_list,p->scale_val_list,
				    p->scale_vals,SLIDER_FLAG_INDEPENDENT_MIDDLE);

    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],lo);
    slice_thumb_set((Slice *)sl[2],hi);
    mapping_setup(&p2->mappings[i],0.,1.,0);
    slider_set_gradient(p2->range_scales[i], &p2->mappings[i]);
  }

  GtkWidget *first_x = NULL;
  GtkWidget *first_y = NULL;
  GtkWidget *pressed_y = NULL;
  p2->dim_scales = calloc(p->dimensions,sizeof(*p2->dim_scales));
  p2->dim_xb = calloc(p->dimensions,sizeof(*p2->dim_xb));
  p2->dim_yb = calloc(p->dimensions,sizeof(*p2->dim_yb));

  for(i=0;i<p->dimensions;i++){
    GtkWidget **sl = calloc(3,sizeof(*sl));
    sushiv_dimension_t *d = p->dimension_list[i];

    /* label */
    GtkWidget *label = gtk_label_new(d->name);
    gtk_table_attach(GTK_TABLE(p2->dim_table),label,0,1,i,i+1,
		     0,0,10,0);
    
    /* x/y radio buttons */
    if(d->flags & SUSHIV_X_RANGE){
      if(first_x)
	p2->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
      else{
	first_x = p2->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]),TRUE);
      }
      gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_xb[i],1,2,i,i+1,
		       0,0,10,0);
    }
    
    if(d->flags & SUSHIV_Y_RANGE){
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

    p2->dim_scales[i] = slider_new((Slice **)sl,3,d->scale_label_list,d->scale_val_list,
				   d->scale_vals,0);

    slice_thumb_set((Slice *)sl[0],d->scale_val_list[0]);
    slice_thumb_set((Slice *)sl[1],0);
    slice_thumb_set((Slice *)sl[2],d->scale_val_list[d->scale_vals-1]);

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


  gtk_widget_realize(p2->toplevel);
  gtk_widget_realize(p2->graph);
  gtk_widget_show_all(p2->toplevel);
}

int sushiv_new_panel_2d(sushiv_instance_t *s,
			int number,
			const char *name, 
			unsigned scalevals,
			double *scaleval_list,
			       int *objectives,
			int *dimensions,
			unsigned flags){
  
  int ret = _sushiv_new_panel(s,number,name,scalevals,scaleval_list,
			      objectives,dimensions,flags);
  sushiv_panel_t *p;
  sushiv_panel2d_t *p2;

  if(ret<0)return ret;
  p = s->panel_list[number];
  p2 = calloc(1, sizeof(*p2));
  p->internal = p2;
  p->type = SUSHIV_PANEL_2D;

  return 0;
}

