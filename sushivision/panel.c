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
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include "internal.h"

int _sv_panels=0;
sv_panel_t **_sv_panel_list=NULL;
pthread_rwlock_t panellist_m;

// panel.panel_m: rwlock, protects all panel and plane heaps
// panel.status_m: mutex, protects status variables

// Plane data in the panels is protected as follows:
//
//   Modifications of pointers, heap allocations, etc are protected by 
//     write-locking panel_m.
//
//   Both read and write access to the contents of plane.data and 
//     plane.image are protected by read-locking panel_m; reads may happen 
//     at any time, writes are checked only that they've not been superceded 
//     by a later request.  Writes are not locked against reads at all; a 
//     read from inconsistent state is temporary and cosmetic only. It will 
//     always be immediately replaced by complete/correct data when the write
//     finishes and triggers a flush.

//   comp_serialno is used to to verify 'freshness' of operations; anytime a 
//     thread needs to synchronize with panel/plane data before continuing 
//     (read or write), it compares the serialno that dispatched it to the 
//     current serialno.  A mismatch immediately aborts the task in progress 
//     with STATUS_WORKING.

//   map_serialno performs the same task with respect to remap requests
//     for a given plane.

// worker thread process order:

// > recompute setup
// > images resize
// > scale render
// > legend render
// > bg render
// > expose
// > data resize
// > image map render (throttleable)
// > computation work 
// > idle             

// proceed to later steps only if there's no work immediately
// dispatchable from earlier steps.

// UI output comes first (UI is never dead, even briefly, and that
// includes graphs), however progressive UI work is purposely
// throttled.

// wake_workers() only explicitly needed when triggering tasks via
// GDK/API; it is already called implicitly in the main loop whenever
// a task step completes.

static void payload_free(sv_dim_data_t *dd, int dims){
  for(i=0;i<dims;i++)
    _sv_dim_data_clear(dd+i);
  free(dd);
}

static int image_resize(sv_plane_t *pl, 
			sv_panel_t *p){
  return pl->c.image_resize(pl, p);
}

static int data_resize(sv_plane_t *pl, 
		       sv_panel_t *p){
  return pl->c.data_resize(pl, p);
}

static int image_work(sv_plane_t *pl, 
		      sv_panel_t *p){
  return pl->c.image_work(pl, p);
}

static int data_work(sv_plane_t *pl, 
		     sv_panel_t *p){
  return pl->c.data_work(pl, p);
}

static int plane_loop(sv_panel_t *p, int *next, 
		      int(*function)(_sv_plane_t *, 
				     sv_panel_t *)){
  int finishedflag=1;
  int last = *next;
  int serialno = p->serialno;
  do{
    int i = *next++;
    if(*next>=p->planes)*next=0;
    int status = function(p->plane_list[i],p);
    if(status == STATUS_WORKING) return STATUS_WORKING;
    if(status != STATUS_IDLE) finishedflag=0;
  }while(i!=last);

  if(finishedflag)
    return STATUS_IDLE;
  return STATUS_BUSY;
}

static int done_working(sv_panel_t *p){
  pthread_mutex_unlock(p->status_m);
  pthread_rwlock_unlock(p->panel_m);
  return STATUS_WORKING;
}

static int done_busy(sv_panel_t *p){
  pthread_mutex_unlock(p->status_m);
  pthread_rwlock_unlock(p->panel_m);
  return STATUS_BUSY;
}

static int done_idle(sv_panel_t *p){
  pthread_mutex_unlock(p->status_m);
  pthread_rwlock_unlock(p->panel_m);
  return STATUS_IDLE;
}

int _sv_panel_work(sv_panel_t *p){
  int i,serialno,status;
  pthread_rwlock_rdlock(p->panel_m);
  pthread_mutex_lock(p->status_m);

  // recomute setup

  // plane recompute calls will do nothing if recomputation is not
  // required.  Even if computation not required, may still request
  // an image resize/resample
  if(p->recompute_pending){
    p->recompute_pending=0;
    p->comp_serialno++;
    p->legend_serialno++;
    p->image_resize=1;
    p->data_resize=1;
    p->data_work=1;
    p->rescale=1;
    p->relegend=1;
    p->bgrender=0;
    p->image_next_plane=0;
    
    bg_recompute_setup(p);

    for(i=0;i<p->planes;i++)
      p->plane_list[i]->c.recompute_setup(p->plane_list[i], p);

    pthread_mutex_unlock(p->status_m);
    pthread_rwlock_unlock(p->panel_m);

    return STATUS_WORKING;
  }

  if(p->relegend_pending){
    p->relegend=1;
    p->relegend_pending=0;
  }

  serialno = p->comp_serialno;

  // bg/image resize
  // image resizes assume bg resize has completed
  if(p->bg_resize){
    status = bg_resize(p);
    if(status == STATUS_WORKING) return done_working(p);
    if(status == STATUS_BUSY) return done_busy(p);
    p->bg_resize=0;
  }

  // image resize
  if(p->image_resize){
    status = plane_loop(p,&p->image_next_plane,image_resize);
    if(status == STATUS_WORKING) return done_working(p);
    if(status == STATUS_IDLE){
      p->image_resize = 0;
      p->bg_render = 1;
    }
  }

  // legend regeneration
  if(p->relegend){
    p->relegend=0; // must be unset before relegend
    bg_legend(p);
    p->expose=1;
    return done_working(p);
  }

  // axis scale redraw
  if(p->rescale){
    p->rescale=0; // must be unset before rescale
    bg_scale(p);
    p->expose=1;
    return done_working(p);
  }

  // need to join on image resizing before proceeding to background redraw
  if(p->image_resize)
    return done_busy(p);
  
  // bg render
  if(p->bg_render){
    if(bg_render(p) == STATUS_IDLE){
      p->expose=1;
      p->bg_render=0;
    }
    return done_working(p);
  }    
  
  // expose
  if(p->expose &&
     !p->relegend &&
     !p->rescale &&
     !p->bgrender){
    // wait till all these ops are done
    p->expose = 0;
    bg_expose(p);
    return done_working(p);
  }    

  // data resize
  if(p->data_resize){
    status = plane_loop(p,&p->data_next_plane,data_resize);
    if(status == STATUS_WORKING) return done_working(p);
    if(status == STATUS_IDLE) p->data_resize = 0;
  }

  // need to join on data resizing before proceeding to map/compute work
  if(p->data_resize)
    return done_busy(p);
  
  // image map render
  if(p->map_render){
    status = plane_loop(p,&p->image_next_plane,image_work);
    if(status == STATUS_WORKING) return done_working(p);
    if(status == STATUS_IDLE){
      p->map_render = 0;
      p->bg_render = 1;
    }
  }
  
  // computation work 
  if(p->data_work){
    status = plane_loop(p,&p->data_next_plane,data_work);
    if(status == STATUS_WORKING){

      // throttled image render
      if(p->map_render==0){
	// no render currently in progress
	struct timeval now;
	gettimeofday(&now,NULL);
	
	if(p->map_throttle_last.tv_sec==0){
	  p->map_throttle_last=now;
	}else{
	  long test = (now.tv_sec - p->map_throttle_last.tv_sec)*1000 + 
	    (now.tv_usec - p->map_throttle_last.tv_usec)/1000;
	  if(test>500)
	    // first request since throttle
	    p->map_render=1;
	}
      }
      return done_working(p);
    }
    if(status == STATUS_IDLE){
      p->map_render = 1;
      p->relegend = 1;
      p->data_work = 0;
    }
  }
  return done_idle(p);
}

// looks like a cop-out but is actually the correct thing to do; the
// data *must* be WYSIWYG from panel display.
static void _sv_panel2d_print_bg(sv_panel_t *p, cairo_t *c){
  _sv_plot_t *plot = PLOT(p->private->graph);

  if(!plot) return;

  cairo_pattern_t *pattern = cairo_pattern_create_for_surface(plot->back);
  cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
  cairo_set_source(c,pattern);
  cairo_paint(c);

  cairo_pattern_destroy(pattern);
}

static void _sv_panel2d_print(sv_panel_t *p, cairo_t *c, int w, int h){
  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot = PLOT(p->private->graph);
  double pw = p->private->graph->allocation.width;
  double ph = p->private->graph->allocation.height;
  double scale;
  int i;
  double maxlabelw=0;
  double y;

  if(w/pw < h/ph)
    scale = w/pw;
  else
    scale = h/ph;

  cairo_matrix_t m;
  cairo_save(c);
  cairo_get_matrix(c,&m);
  cairo_matrix_scale(&m,scale,scale);
  cairo_set_matrix(c,&m);
  
  _sv_plot_print(plot, c, ph*scale, (void(*)(void *, cairo_t *))_sv_panel2d_print_bg, p);
  cairo_restore(c);

  // find extents widths for objective scale labels
  cairo_set_font_size(c,10);
  for(i=0;i<p->objectives;i++){
    cairo_text_extents_t ex;
    sv_obj_t *o = p->objective_list[i].o;
    cairo_text_extents(c, o->name, &ex);
    if(ex.width > maxlabelw) maxlabelw=ex.width;
  }


  y = ph * scale + 10;

  for(i=0;i<p->objectives;i++){
    sv_obj_t *o = p->objective_list[i].o;
    _sv_slider_t *s = p2->range_scales[i];
    
    // get scale height
    double labelh = _sv_slider_print_height(s);
    cairo_text_extents_t ex;
    cairo_text_extents (c, o->name, &ex);

    int lx = maxlabelw - ex.width;
    int ly = labelh/2 + ex.height/2;
    
    // print objective labels
    cairo_set_source_rgb(c,0.,0.,0.);
    cairo_move_to (c, lx,ly+y);
    cairo_show_text (c, o->name);

    // draw slider
    // set translation
    cairo_save(c);
    cairo_translate (c, maxlabelw + 10, y);
    _sv_slider_print(s, c, pw*scale - maxlabelw - 10, labelh);
    cairo_restore(c);

    y += labelh;
  }

}

// enter with lock
static void _sv_panel2d_update_legend(sv_panel_t *p){  
  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot = PLOT(p->private->graph);

  if(plot){
    int i;
    char buffer[320];
    int depth = 0;
    _sv_plot_legend_clear(plot);

    // potentially add each dimension to the legend; add axis
    // dimensions only if crosshairs are active

    // display decimal precision relative to display scales
    if(3-_sv_scalespace_decimal_exponent(&p2->x) > depth) 
      depth = 3-_sv_scalespace_decimal_exponent(&p2->x);
    if(3-_sv_scalespace_decimal_exponent(&p2->y) > depth) 
      depth = 3-_sv_scalespace_decimal_exponent(&p2->y);
    for(i=0;i<p->dimensions;i++){
      sv_dim_t *d = p->dimension_list[i].d;
      if( (d!=p->private->x_d && d!=p->private->y_d) ||
	  plot->cross_active){
	snprintf(buffer,320,"%s = %+.*f",
		 p->dimension_list[i].d->legend,
		 depth,
		 p->dimension_list[i].d->val);
	_sv_plot_legend_add(plot,buffer);
      }
    }
    
    // add each active objective plane to the legend
    // choose the value under the crosshairs 
    if(plot->cross_active){
      // one space 
      _sv_plot_legend_add(plot,NULL);

      for(i=0;i<p->objectives;i++){
	
	if(!_sv_mapping_inactive_p(p2->mappings+i)){
	  compute_result vals;
	  _sv_panel2d_compute_point(p,p->objective_list[i].o, plot->selx, plot->sely, &vals);
	  
	  if(!isnan(vals.y)){
	    
	    snprintf(buffer,320,"%s = %f",
		     p->objective_list[i].o->name,
		     vals.y);
	    _sv_plot_legend_add(plot,buffer);
	  }
	}
      }
    }
  }
}

static void _sv_panel2d_mapchange_callback(GtkWidget *w,gpointer in){
  sv_obj_list_t *optr = (sv_obj_list_t *)in;
  //sv_obj_t *o = optr->o;
  sv_panel_t *p = optr->p;
  _sv_panel2d_t *p2 = p->subtype->p2;
  int onum = optr - p->objective_list;

  _sv_undo_push();
  _sv_undo_suspend();

  _sv_mapping_set_func(&p2->mappings[onum],gtk_combo_box_get_active(GTK_COMBO_BOX(w)));
  
  //redraw the map slider
  _sv_slider_set_gradient(p2->range_scales[onum], &p2->mappings[onum]);

  // in the event the mapping active state changed
  _sv_panel_dirty_legend(p);

  //redraw the plot
  _sv_panel2d_mark_map_plane(p,onum,1,0,0);
  _sv_panel_dirty_map(p);
  _sv_undo_resume();
}

static void _sv_panel2d_map_callback(void *in,int buttonstate){
  sv_obj_list_t *optr = (sv_obj_list_t *)in;
  //sv_obj_t *o = optr->o;
  sv_panel_t *p = optr->p;
  _sv_panel2d_t *p2 = p->subtype->p2;
  int onum = optr - p->objective_list;

  if(buttonstate == 0){
    _sv_undo_push();
    _sv_undo_suspend();
  }

  // recache alpha del */
  p2->alphadel[onum] = 
    _sv_slider_val_to_del(p2->range_scales[onum],
		      _sv_slider_get_value(p2->range_scales[onum],1));

  // redraw the plot on motion
  if(buttonstate == 1){
    _sv_panel2d_mark_map_plane(p,onum,1,0,0);
    _sv_panel_dirty_map(p);
  }
  if(buttonstate == 2)
    _sv_undo_resume();
}

static void _sv_panel2d_update_xysel(sv_panel_t *p){
  _sv_panel2d_t *p2 = p->subtype->p2;
  int i;
  // update which x/y buttons are pressable */
  // enable/disable dimension slider thumbs

  for(i=0;i<p->dimensions;i++){
    if(p2->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))){
      // make the y insensitive
      if(p2->dim_yb[i])
	_gtk_widget_set_sensitive_fixup(p2->dim_yb[i],FALSE);

      // set the x dim flag
      p->private->x_d = p->dimension_list[i].d;
      p2->x_scale = p->private->dim_scales[i];
      p2->x_dnum = i;
    }else{
      // if there is a y, make it sensitive 
      if(p2->dim_yb[i])
	_gtk_widget_set_sensitive_fixup(p2->dim_yb[i],TRUE);
    }
    if(p2->dim_yb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]))){
      // make the x insensitive
      if(p2->dim_xb[i])
	_gtk_widget_set_sensitive_fixup(p2->dim_xb[i],FALSE);

      // set the y dim
      p->private->y_d = p->dimension_list[i].d;
      p2->y_scale = p->private->dim_scales[i];
      p2->y_dnum = i;
    }else{
      // if there is a x, make it sensitive 
      if(p2->dim_xb[i])
	_gtk_widget_set_sensitive_fixup(p2->dim_xb[i],TRUE);
    }
    if((p2->dim_xb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))) ||
       (p2->dim_yb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i])))){
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

static int _v_swizzle(int y, int height){
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

static void _sv_panel2d_center_callback(sv_dim_list_t *dptr){
  sv_dim_t *d = dptr->d;
  sv_panel_t *p = dptr->p;
  int axisp = (d == p->private->x_d || d == p->private->y_d);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _sv_panel2d_mark_recompute(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    _sv_panel2d_update_crosshairs(p);
  }
}

static void _sv_panel2d_bracket_callback(sv_dim_list_t *dptr){
  sv_dim_t *d = dptr->d;
  sv_panel_t *p = dptr->p;
  int axisp = (d == p->private->x_d || d == p->private->y_d);

  if(axisp)
    _sv_panel2d_mark_recompute(p);
    
}

static void _sv_panel2d_dimchange_callback(GtkWidget *button,gpointer in){
  sv_panel_t *p = (sv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sv_undo_push();
    _sv_undo_suspend();

    _sv_plot_unset_box(PLOT(p->private->graph));
    _sv_panel2d_update_xysel(p);

    _sv_panel2d_clear_pane(p);
    _sv_panel2d_mark_recompute(p);
    _sv_panel2d_update_crosshairs(p);

    _sv_undo_resume();
  }
}

static void _sv_panel2d_crosshairs_callback(sv_panel_t *p){
  double x=PLOT(p->private->graph)->selx;
  double y=PLOT(p->private->graph)->sely;
  int i;
  
  _sv_undo_push();
  _sv_undo_suspend();

  //plot_snap_crosshairs(PLOT(p->private->graph));

  for(i=0;i<p->dimensions;i++){
    sv_dim_t *d = p->dimension_list[i].d;
    if(d == p->private->x_d){
      _sv_dim_widget_set_thumb(p->private->dim_scales[i],1,x);
    }

    if(d == p->private->y_d){
      _sv_dim_widget_set_thumb(p->private->dim_scales[i],1,y);
    }
    
    p->private->oldbox_active = 0;
  }

  // dimension setting might have enforced granularity restrictions;
  // have the display reflect that
  x = p->private->x_d->val;
  y = p->private->y_d->val;

  _sv_plot_set_crosshairs(PLOT(p->private->graph),x,y);

  _sv_panel_dirty_legend(p);
  _sv_undo_resume();
}

static void _sv_panel2d_box_callback(void *in, int state){
  sv_panel_t *p = (sv_panel_t *)in;
  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sv_undo_push();
    _sv_plot_box_vals(plot,p2->oldbox);
    p->private->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sv_undo_push();
    _sv_undo_suspend();

    _sv_panel2d_crosshairs_callback(p);

    _sv_dim_widget_set_thumb(p2->x_scale,0,p2->oldbox[0]);
    _sv_dim_widget_set_thumb(p2->x_scale,2,p2->oldbox[1]);
    _sv_dim_widget_set_thumb(p2->y_scale,0,p2->oldbox[2]);
    _sv_dim_widget_set_thumb(p2->y_scale,2,p2->oldbox[3]);
    p->private->oldbox_active = 0;
    _sv_undo_resume();
    break;
  }
  _sv_panel_update_menus(p);
}

// subtype entry point for legend redraws; lock held
static int _sv_panel2d_legend_redraw(sv_panel_t *p){
  _sv_plot_t *plot = PLOT(p->private->graph);

  if(p->private->legend_progress_count)return 0;
  p->private->legend_progress_count++;
  _sv_panel2d_update_legend(p);
  _sv_panel_clean_legend(p);

  gdk_unlock();
  _sv_plot_draw_scales(plot);
  gdk_lock();

  _sv_plot_expose_request(plot);
  return 1;
}

// only called for resize events
static void _sv_panel2d_recompute_callback(void *ptr){
  sv_panel_t *p = (sv_panel_t *)ptr;
  int i;

  gdk_lock ();
  _sv_panel2d_mark_recompute(p);
  _sv_panel2d_compute(p,NULL); // initial scale setup

  // temporary: blank background to checks
  _sv_plot_t *plot = PLOT(p->private->graph);
  int pw = plot->x.pixels;
  int ph = plot->y.pixels;
  for(i=0;i<ph;i++)
    render_checks((_sv_ucolor_t *)plot->datarect+pw*i, pw, i);
  
  gdk_unlock();
}

static void _sv_panel2d_undo_log(_sv_panel_undo_t *u, sv_panel_t *p){
  _sv_panel2d_t *p2 = p->subtype->p2;
  int i;

  // alloc fields as necessary
  
  if(!u->mappings)
    u->mappings =  calloc(p->objectives,sizeof(*u->mappings));
  if(!u->scale_vals[0])
    u->scale_vals[0] =  calloc(p->objectives,sizeof(**u->scale_vals));
  if(!u->scale_vals[1])
    u->scale_vals[1] =  calloc(p->objectives,sizeof(**u->scale_vals));
  if(!u->scale_vals[2])
    u->scale_vals[2] =  calloc(p->objectives,sizeof(**u->scale_vals));

  // populate undo
  for(i=0;i<p->objectives;i++){
    u->mappings[i] = p2->mappings[i].mapnum;
    u->scale_vals[0][i] = _sv_slider_get_value(p2->range_scales[i],0);
    u->scale_vals[1][i] = _sv_slider_get_value(p2->range_scales[i],1);
    u->scale_vals[2][i] = _sv_slider_get_value(p2->range_scales[i],2);
  }
  
  u->x_d = p2->x_dnum;
  u->y_d = p2->y_dnum;
  u->box[0] = p2->oldbox[0];
  u->box[1] = p2->oldbox[1];
  u->box[2] = p2->oldbox[2];
  u->box[3] = p2->oldbox[3];
  u->box_active = p->private->oldbox_active;
}

static void _sv_panel2d_undo_restore(_sv_panel_undo_t *u, sv_panel_t *p){
  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot = PLOT(p->private->graph);
  int i;
  
  // go in through widgets
  for(i=0;i<p->objectives;i++){
    gtk_combo_box_set_active(GTK_COMBO_BOX(p2->range_pulldowns[i]),u->mappings[i]);
    _sv_slider_set_value(p2->range_scales[i],0,u->scale_vals[0][i]);
    _sv_slider_set_value(p2->range_scales[i],1,u->scale_vals[1][i]);
    _sv_slider_set_value(p2->range_scales[i],2,u->scale_vals[2][i]);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[u->x_d]),TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_yb[u->y_d]),TRUE);

  _sv_panel2d_update_xysel(p);

  if(u->box_active){
    p2->oldbox[0] = u->box[0];
    p2->oldbox[1] = u->box[1];
    p2->oldbox[2] = u->box[2];
    p2->oldbox[3] = u->box[3];
    _sv_plot_box_set(plot,u->box);
    p->private->oldbox_active = 1;
  }else{
    _sv_plot_unset_box(plot);
    p->private->oldbox_active = 0;
  }
}

static void _sv_panel2d_realize(sv_panel_t *p){
  _sv_panel2d_t *p2 = p->subtype->p2;
  int i;

  _sv_undo_suspend();

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
    p->private->graph = GTK_WIDGET(_sv_plot_new(_sv_panel2d_recompute_callback,p,
						(void *)(void *)_sv_panel2d_crosshairs_callback,p,
						_sv_panel2d_box_callback,p,0)); 
    p->private->plotbox = p->private->graph;
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p->private->plotbox, 1,1,2);
  }

  /* obj box */
  {
    p2->obj_table = gtk_table_new(p->objectives, 5, 0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p2->obj_table, 0,0,1);

    /* objective sliders */
    p2->range_scales = calloc(p->objectives,sizeof(*p2->range_scales));
    p2->range_pulldowns = calloc(p->objectives,sizeof(*p2->range_pulldowns));
    p2->alphadel = calloc(p->objectives,sizeof(*p2->alphadel));
    p2->mappings = calloc(p->objectives,sizeof(*p2->mappings));
    for(i=0;i<p->objectives;i++){
      GtkWidget **sl = calloc(3,sizeof(*sl));
      sv_obj_t *o = p->objective_list[i].o;
      int lo = o->scale->val_list[0];
      int hi = o->scale->val_list[o->scale->vals-1];
      
      /* label */
      GtkWidget *label = gtk_label_new(o->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p2->obj_table),label,0,1,i,i+1,
		       GTK_FILL,0,8,0);
      
      /* mapping pulldown */
      {
	GtkWidget *menu=_gtk_combo_box_new_markup();
	int j;
	for(j=0;j<_sv_mapping_names();j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), _sv_mapping_name(j));
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (_sv_panel2d_mapchange_callback), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p2->obj_table),menu,4,5,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,0,0);
	p2->range_pulldowns[i] = menu;
      }

      /* the range mapping slices/slider */ 
      sl[0] = _sv_slice_new(_sv_panel2d_map_callback,p->objective_list+i);
      sl[1] = _sv_slice_new(_sv_panel2d_map_callback,p->objective_list+i);
      sl[2] = _sv_slice_new(_sv_panel2d_map_callback,p->objective_list+i);
      
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[0],1,2,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[1],2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[2],3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      p2->range_scales[i] = _sv_slider_new((_sv_slice_t **)sl,3,o->scale->label_list,o->scale->val_list,
				       o->scale->vals,_SV_SLIDER_FLAG_INDEPENDENT_MIDDLE);
      gtk_table_set_col_spacing(GTK_TABLE(p2->obj_table),3,5);

      _sv_slice_thumb_set((_sv_slice_t *)sl[0],lo);
      _sv_slice_thumb_set((_sv_slice_t *)sl[1],lo);
      _sv_slice_thumb_set((_sv_slice_t *)sl[2],hi);
      _sv_mapping_setup(&p2->mappings[i],0.,1.,0);
      _sv_slider_set_gradient(p2->range_scales[i], &p2->mappings[i]);
    }
  }

  /* dims */
  {
    p2->dim_table = gtk_table_new(p->dimensions,4,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p2->dim_table, 0,0,4);
    
    GtkWidget *first_x = NULL;
    GtkWidget *first_y = NULL;
    GtkWidget *pressed_y = NULL;
    p->private->dim_scales = calloc(p->dimensions,sizeof(*p->private->dim_scales));
    p2->dim_xb = calloc(p->dimensions,sizeof(*p2->dim_xb));
    p2->dim_yb = calloc(p->dimensions,sizeof(*p2->dim_yb));
    
    for(i=0;i<p->dimensions;i++){
      sv_dim_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->legend);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p2->dim_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* x/y radio buttons */
      if(!(d->flags & SV_DIM_NO_X)){
	if(first_x)
	  p2->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
	else{
	  first_x = p2->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_xb[i],1,2,i,i+1,
			 GTK_SHRINK,0,3,0);
      }
      
      if(!(d->flags & SV_DIM_NO_Y)){
	if(first_y)
	  p2->dim_yb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_y),"Y");
	else
	  first_y = p2->dim_yb[i] = gtk_radio_button_new_with_label(NULL,"Y");
	if(!pressed_y && p2->dim_xb[i]!=first_x){
	  pressed_y = p2->dim_yb[i];
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_yb[i],2,3,i,i+1,
			 GTK_SHRINK,0,3,0);
      }
      
      p->private->dim_scales[i] = 
	_sv_dim_widget_new(p->dimension_list+i,_sv_panel2d_center_callback,_sv_panel2d_bracket_callback);
      
      gtk_table_attach(GTK_TABLE(p2->dim_table),
		       p->private->dim_scales[i]->t,
		       3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
    }
    for(i=0;i<p->dimensions;i++){
      if(p2->dim_xb[i])
	g_signal_connect (G_OBJECT (p2->dim_xb[i]), "toggled",
			  G_CALLBACK (_sv_panel2d_dimchange_callback), p);
      if(p2->dim_yb[i])
	g_signal_connect (G_OBJECT (p2->dim_yb[i]), "toggled",
			  G_CALLBACK (_sv_panel2d_dimchange_callback), p);
    }
  }

  _sv_panel2d_update_xysel(p);

  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_realize(GTK_WIDGET(p->private->spinner));
  gtk_widget_show_all(p->private->toplevel);
  _sv_panel2d_update_xysel(p); // yes, this was already done; however,
			       // gtk clobbered the event setup on the
			       // insensitive buttons when it realized
			       // them.  This call will restore them.

  _sv_undo_resume();
}

static int _sv_panel2d_save(sv_panel_t *p, xmlNodePtr pn){  
  _sv_panel2d_t *p2 = p->subtype->p2;
  int ret=0,i;

  xmlNodePtr n;

  xmlNewProp(pn, (xmlChar *)"type", (xmlChar *)"2d");

  // box
  if(p->private->oldbox_active){
    xmlNodePtr boxn = xmlNewChild(pn, NULL, (xmlChar *) "box", NULL);
    _xmlNewPropF(boxn, "x1", p2->oldbox[0]);
    _xmlNewPropF(boxn, "x2", p2->oldbox[1]);
    _xmlNewPropF(boxn, "y1", p2->oldbox[2]);
    _xmlNewPropF(boxn, "y2", p2->oldbox[3]);
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
    _xmlNewPropS(n, "color", _sv_mapping_name(p2->mappings[i].mapnum));
    _xmlNewPropF(n, "low-bracket", _sv_slider_get_value(p2->range_scales[i],0));
    _xmlNewPropF(n, "alpha", _sv_slider_get_value(p2->range_scales[i],1));
    _xmlNewPropF(n, "high-bracket", _sv_slider_get_value(p2->range_scales[i],2));
  }

  // x/y dim selection
  n = xmlNewChild(pn, NULL, (xmlChar *) "axes", NULL);
  _xmlNewPropI(n, "xpos", p2->x_dnum);
  _xmlNewPropI(n, "ypos", p2->y_dnum);

  return ret;
}

int _sv_panel2d_load(sv_panel_t *p,
		     _sv_panel_undo_t *u,
		     xmlNodePtr pn,
		     int warn){
  int i;

  // check type
  _xmlCheckPropS(pn,"type","2d", "Panel %d type mismatch in save file.",p->number,&warn);
  
  // box
  u->box_active = 0;
  _xmlGetChildPropFPreserve(pn, "box", "x1", &u->box[0]);
  _xmlGetChildPropFPreserve(pn, "box", "x2", &u->box[1]);
  _xmlGetChildPropFPreserve(pn, "box", "y1", &u->box[2]);
  _xmlGetChildPropFPreserve(pn, "box", "y2", &u->box[3]);

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
      _xmlGetChildPropFPreserve(on, "y-map", "low-bracket", &u->scale_vals[0][i]);
      _xmlGetChildPropFPreserve(on, "y-map", "alpha", &u->scale_vals[1][i]);
      _xmlGetChildPropFPreserve(on, "y-map", "high-bracket", &u->scale_vals[2][i]);
      _xmlGetChildMap(on, "y-map", "color", _sv_mapping_map(), &u->mappings[i],
		     "Panel %d objective unknown mapping setting", p->number, &warn);

      xmlFreeNode(on);
    }
  }

  // x/y dim selection
  _xmlGetChildPropIPreserve(pn, "axes", "xpos", &u->x_d);
  _xmlGetChildPropI(pn, "axes", "ypos", &u->y_d);

  return warn;
}


void _sv_panel_realize(sv_panel_t *p){
  if(p && !p->private->realized){
    p->private->realize(p);

    g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
		      G_CALLBACK (panel_keypress), p);
    gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

    p->private->realized=1;

    // generic things that happen in all panel realizations...

    // text black or white in the plot?
    decide_text_inv(p);
    p->private->popmenu = _gtk_menu_new_twocol(p->private->toplevel, menu, p);
    _sv_panel_update_menus(p);
    
  }
}

int sv_panel_background(int number,
			enum sv_background bg){

  if(number<0){
    fprintf(stderr,"sv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>_sv_panels || !_sv_panel_list[number]){
    fprintf(stderr,"sv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sv_panel_t *p = _sv_panel_list[number];
  return set_background(p,bg);
}

sv_panel_t *_sv_panel(char *name){
  int i;
  
  if(name == NULL || name == 0 || !strcmp(name,"")){
    return (sv_panel_t *)pthread_getspecific(_sv_panel_key);
  }

  for(i=0;i<_sv_panels;i++){
    sv_panel_t *p=_sv_panel_list[i];
    if(p && p->name && !strcmp(name,p->name)){
      pthread_setspecific(_sv_panel_key, (void *)p);
      return p;
    }
  }
  return NULL;
}

int sv_panel(char *name){
  sv_panel_t *p = _sv_panel(name);
  if(p)return 0;
  return -EINVAL;
}

sv_panel_t *sv_panel_new(char *name, 
			 char *objlist,
			 char *dimlist){

  sv_panel_t *p = NULL;
  int number;
  sv_token *decl = _sv_tokenize_declparam(name);
  sv_tokenlist *dim_tokens = NULL;
  sv_tokenlist *obj_tokens = NULL;  
  int i,j;

  if(!decl){
    fprintf(stderr,"sushivision: Unable to parse panel declaration \"%s\".\n",name);
    goto err;
  }

  // panel and panel list manipulation must be locked
  gdk_threads_enter();
  pthread_rwlock_wrlock(panellist_m);

  if(_sv_panels == 0){
    number=0;
    _sv_panel_list = calloc (number+1,sizeof(*_sv_panel_list));
    _sv_panels=1;
  }else{
    for(number=0;number<_sv_panels;number++)
      if(!_sv_panel_list[number])break;
    if(number==_sv_panels){
      _sv_panels=number+1;
      _sv_panel_list = realloc (_sv_panel_list,_sv_panels * sizeof(*_sv_panel_list));
    }
  }
  
  p = _sv_panel_list[number] = calloc(1, sizeof(**_sv_panel_list));
  p->name = strdup(decl->name);
  p->legend = strdup(decl->label);
  p->number = number;
  p->spinner = _sv_spinner_new();

  // parse and sanity check the maps 
  i=0;
  obj_tokens = _sv_tokenize_namelist(objectivelist);
  p->objectives = obj_tokens->n;
  p->objective_list = malloc(p->objectives*sizeof(*p->objective_list));
  for(i=0;i<p->objectives;i++){
    char *name = obj_tokens->list[i]->name;
    p->objective_list[i].o = _sv_obj(name);
    p->objective_list[i].p = p;
  }

  i=0;
  dim_tokens = _sv_tokenize_namelist(dimensionlist);
  p->dimensions = dim_tokens->n;
  p->dimension_list = malloc(p->dimensions*sizeof(*p->dimension_list));
  for(i=0;i<p->dimensions;i++){
    char *name = dim_tokens->list[i]->name;
    sv_dim_t *d = sv_dim(name);

    if(!d){
      fprintf(stderr,"Panel %d (\"%s\"): Dimension \"%s\" does not exist\n",
	      number,p->name,name);
      errno = -EINVAL;
      //XXX leak
      return NULL;
    }

    if(!d->scale){
      fprintf(stderr,"Panel %d (\"%s\"): Dimension \"%s\" has a NULL scale\n",
	      number,p->name,name);
      errno = -EINVAL;
      //XXX leak
      return NULL;
    }

    p->dimension_list[i].d = d;
    p->dimension_list[i].p = p;
  }

  _sv_tokenlist_free(obj_tokens);
  _sv_tokenlist_free(dim_tokens);
  return p;
}






  if(!p)return NULL;

  _sv_panel2d_t *p2 = calloc(1, sizeof(*p2));
  int fout_offsets[_sv_functions];
  
  p->subtype = 
    calloc(1, sizeof(*p->subtype)); /* the union is alloced not
				       embedded as its internal
				       structure must be hidden */  
  p->subtype->p2 = p2;
  p->type = SV_PANEL_2D;
  p->private->bg_type = SV_BG_CHECKS;

  // verify all the objectives have scales
  for(i=0;i<p->objectives;i++){
    if(!p->objective_list[i].o->scale){
      fprintf(stderr,"All objectives in a 2d panel must have a scale\n");
      errno = -EINVAL;
      return NULL;
    }
  }

  p->private->realize = _sv_panel2d_realize;
  p->private->map_action = _sv_panel2d_map_redraw;
  p->private->legend_action = _sv_panel2d_legend_redraw;
  p->private->compute_action = _sv_panel2d_compute;
  p->private->request_compute = _sv_panel2d_mark_recompute;
  p->private->crosshair_action = _sv_panel2d_crosshairs_callback;
  p->private->print_action = _sv_panel2d_print;
  p->private->undo_log = _sv_panel2d_undo_log;
  p->private->undo_restore = _sv_panel2d_undo_restore;
  p->private->save_action = _sv_panel2d_save;
  p->private->load_action = _sv_panel2d_load;

  /* set up helper data structures for rendering */

  /* determine which functions are actually needed; if it's referenced
     by an objective, it's used.  Precache them in dense form. */
  {
    int fn = _sv_functions;
    int used[fn],count=0,offcount=0;
    memset(used,0,sizeof(used));
    memset(fout_offsets,-1,sizeof(fout_offsets));
    
    for(i=0;i<p->objectives;i++){
      sv_obj_t *o = p->objective_list[i].o;
      for(j=0;j<o->outputs;j++)
	used[o->function_map[j]]=1;
    }

    for(i=0;i<fn;i++)
      if(used[i]){
	sv_func_t *f = _sv_function_list[i];
	fout_offsets[i] = offcount;
	offcount += f->outputs;
	count++;
      }

    p2->used_functions = count;
    p2->used_function_list = calloc(count, sizeof(*p2->used_function_list));

    for(count=0,i=0;i<fn;i++)
     if(used[i]){
        p2->used_function_list[count]=_sv_function_list[i];
	count++;
      }
  }

  /* set up computation/render helpers for Y planes */

  /* set up Y object mapping index */
  {
    int yobj_count = 0;

    for(i=0;i<p->objectives;i++){
      sv_obj_t *o = p->objective_list[i].o;
      if(o->private->y_func) yobj_count++;
    }

    p2->y_obj_num = yobj_count;
    p2->y_obj_list = calloc(yobj_count, sizeof(*p2->y_obj_list));
    p2->y_obj_to_panel = calloc(yobj_count, sizeof(*p2->y_obj_to_panel));
    p2->y_obj_from_panel = calloc(p->objectives, sizeof(*p2->y_obj_from_panel));
    
    yobj_count=0;
    for(i=0;i<p->objectives;i++){
      sv_obj_t *o = p->objective_list[i].o;
      if(o->private->y_func){
	p2->y_obj_list[yobj_count] = o;
	p2->y_obj_to_panel[yobj_count] = i;
	p2->y_obj_from_panel[i] = yobj_count;
	yobj_count++;
      }else
	p2->y_obj_from_panel[i] = -1;
      
    }
  }
  
  /* set up function Y output value demultiplex helper */
  {
    p2->y_fout_offset = calloc(p2->y_obj_num, sizeof(*p2->y_fout_offset));
    for(i=0;i<p2->y_obj_num;i++){
      sv_obj_t *o = p2->y_obj_list[i];
      int funcnum = o->private->y_func->number;
      p2->y_fout_offset[i] = fout_offsets[funcnum] + o->private->y_fout;
    }
  }

  p2->y_map = calloc(p2->y_obj_num,sizeof(*p2->y_map));
  p2->y_planetodo = calloc(p2->y_obj_num,sizeof(*p2->y_planetodo));
  p2->y_planes = calloc(p2->y_obj_num,sizeof(*p2->y_planes));

  return p;
}




void _sv_panel_undo_log(sv_panel_t *p, _sv_panel_undo_t *u){
  u->cross_mode = PLOT(p->private->graph)->cross_active;
  u->legend_mode = PLOT(p->private->graph)->legend_active;
  u->grid_mode = PLOT(p->private->graph)->grid_mode;
  u->text_mode = PLOT(p->private->graph)->bg_inv;
  u->bg_mode = p->private->bg_type;
  u->menu_cursamp = p->private->menu_cursamp;
  u->oversample_n = p->private->oversample_n;
  u->oversample_d = p->private->oversample_d;

  // panel-subtype-specific population
  p->private->undo_log(u,p);
}

void _sv_panel_undo_restore(sv_panel_t *p, _sv_panel_undo_t *u){
  // go in through setting routines
  _sv_plot_set_crossactive(PLOT(p->private->graph),u->cross_mode);
  _sv_plot_set_legendactive(PLOT(p->private->graph),u->legend_mode);
  set_background(p, u->bg_mode); // must be first; it can frob grid and test
  set_text(p, u->text_mode);
  set_grid(p, u->grid_mode);
  p->private->menu_cursamp = u->menu_cursamp;
  res_set(p, u->oversample_n, u->oversample_d);

  // panel-subtype-specific restore
  p->private->undo_restore(u,p);

  _sv_panel_dirty_legend(p); 
}

int _sv_panel_save(sv_panel_t *p, xmlNodePtr instance){  
  if(!p) return 0;
  char buffer[80];
  int ret=0;

  xmlNodePtr pn = xmlNewChild(instance, NULL, (xmlChar *) "panel", NULL);
  xmlNodePtr n;

  _xmlNewPropI(pn, "number", p->number);
  _xmlNewPropS(pn, "name", p->name);

  // let the panel subtype handler fill in type
  // we're only saving settings independent of subtype

  // background
  n = xmlNewChild(pn, NULL, (xmlChar *) "background", NULL);
  _xmlNewMapProp(n, "color", bgmap, p->private->bg_type);

  // grid
  n = xmlNewChild(pn, NULL, (xmlChar *) "grid", NULL);
  _xmlNewMapProp(n, "mode", gridmap, PLOT(p->private->graph)->grid_mode);

  // crosshairs
  n = xmlNewChild(pn, NULL, (xmlChar *) "crosshairs", NULL);
  _xmlNewMapProp(n, "active", crossmap, PLOT(p->private->graph)->cross_active);

  // legend
  n = xmlNewChild(pn, NULL, (xmlChar *) "legend", NULL);
  _xmlNewMapProp(n,"mode", legendmap, PLOT(p->private->graph)->legend_active);

  // text
  n = xmlNewChild(pn, NULL, (xmlChar *) "text", NULL);
  _xmlNewMapProp(n,"color", textmap, PLOT(p->private->graph)->bg_inv);

  // resample
  n = xmlNewChild(pn, NULL, (xmlChar *) "sampling", NULL);
  snprintf(buffer,sizeof(buffer),"%d:%d",
	   p->private->oversample_n, p->private->oversample_d);
  xmlNewProp(n, (xmlChar *)"ratio", (xmlChar *)buffer);

  // subtype 
  if(p->private->save_action)
    ret |= p->private->save_action(p, pn);

  return ret;
}

int _sv_panel_load(sv_panel_t *p,
		   _sv_panel_undo_t *u,
		   xmlNodePtr pn,
		   int warn){

  // check name 
  _xmlCheckPropS(pn,"name",p->name,"Panel %d name mismatch in save file.",p->number,&warn);

  // background
  _xmlGetChildMap(pn, "background", "color", bgmap, &u->bg_mode,
		  "Panel %d unknown background setting", p->number, &warn);
  // grid
  _xmlGetChildMap(pn, "grid", "mode", gridmap, &u->grid_mode,
		  "Panel %d unknown grid mode setting", p->number, &warn);
  // crosshairs
  _xmlGetChildMap(pn, "crosshairs", "active", crossmap, &u->cross_mode,
		  "Panel %d unknown crosshair setting", p->number, &warn);
  // legend
  _xmlGetChildMap(pn, "legend", "mode", legendmap, &u->legend_mode,
		  "Panel %d unknown legend setting", p->number, &warn);
  // text
  _xmlGetChildMap(pn, "text", "color", textmap, &u->text_mode,
		  "Panel %d unknown text color setting", p->number, &warn);
  // resample
  char *prop = NULL;
  _xmlGetChildPropS(pn, "sampling", "ratio", &prop);
  if(prop){
    int res = sscanf(prop,"%d:%d", &u->oversample_n, &u->oversample_d);
    if(res<2){
      fprintf(stderr,"Unable to parse sample setting (%s) for panel %d.\n",prop,p->number);
      u->oversample_n = p->private->def_oversample_n;
      u->oversample_d = p->private->def_oversample_d;
    }
    if(u->oversample_d == 0) u->oversample_d = 1;
    xmlFree(prop);
  }
  
  // subtype 
  if(p->private->load_action)
    warn = p->private->load_action(p, u, pn, warn);
  
  // any unparsed elements? 
  xmlNodePtr n = pn->xmlChildrenNode;
  
  while(n){
    if (n->type == XML_ELEMENT_NODE) {
      _sv_first_load_warning(&warn);
      fprintf(stderr,"Unknown option (%s) set for panel %d.\n",n->name,p->number);
    }
    n = n->next; 
  }

  return warn;
}

int sv_panel_callback_recompute (sv_panel_t *p,
				 int (*callback)(sv_panel_t *p,void *data),
				 void *data){

  p->private->callback_precompute = callback;
  p->private->callback_precompute_data = data;
  return 0;
}

