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

// the background 'plane' is handled a little differently from the
// other planes, as alot of non-generalizable panel rendering also
// happens in this module.  For this reason, the bg plane doesn't
// bother with functions embedded in the plane struct.  They're simply
// called directly by the panel code.

void bg_recompute_setup(sv_panel_t *p){
  sv_planebg_t *pl = p->bg;

  pl->image_x = _sv_dim_panelscale(payload + x_dim, p->w, 0);
  pl->image_y = _sv_dim_panelscale(payload + y_dim, p->h, 1);
  pl->image_task = 0;

}

void bg_resize(sv_panel_t *p){
  sv_planebg_t *pl = p->bg;

  if(pl->image_status_size != p->h){
    unsigned char *n=NULL;
    unsigned char *o=NULL;
    int serialno = p->comp_serialno;

    pthread_mutex_unlock(pl->status_m);    
    n = calloc(p->h,sizeof(*n));
    pthread_mutex_lock(pl->status_m);    
    if(serialno == p->comp_serialno){
      o = pl->image_status;
      pl->image_status = n;
      n = NULL;
    }
    pthread_mutex_unlock(pl->status_m);    
    if(n)free(n);
    if(o)free(o);
    pthread_mutex_lock(pl->status_m);    
    return STATUS_WORKING;
  }

  return STATUS_IDLE;
}

int bg_legend(sv_panel_t *p){


}

int bg_scale(sv_panel_t *p){
      gdk_unlock ();      
      _sv_plot_draw_scales(plot); // this should happen outside lock
      gdk_lock ();      


}

int bg_render(sv_panel_t *p){


}

int bg_expose(sv_panel_t *p){


}

sv_planebg_t *sv_planebg_new(sv_panel_t *p){


}

static void render_checks(_sv_ucolor_t *c, int w, int y){
  /* default checked background */
  /* 16x16 'mid-checks' */ 
  int x,j;
  
  int phase = (y>>4)&1;
  for(x=0;x<w;){
    u_int32_t phaseval = 0xff505050UL;
    if(phase) phaseval = 0xff808080UL;
    for(j=0;j<16 && x<w;j++,x++)
      c[x].u = phaseval;
    phase=!phase;
  }
}

// enter with lock
static int _sv_panel2d_render_bg_line(sv_panel_t *p, int plot_serialno, int map_serialno){
  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot = PLOT(p->private->graph);
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno) return -1;
  
  int ph = p2->y.pixels;
  int pw = p2->x.pixels;
  unsigned char *todo = p2->bg_todo;
  int i = p2->bg_next_line,j;
  _sv_ucolor_t work_bg[pw];
  _sv_ucolor_t work_pl[pw];
  int bgmode = p->private->bg_type;

  /* find a row that needs to be updated */
  while(i<ph && !todo[i]){
    p->private->map_complete_count--;
    p2->bg_next_line++;
    i++;
  }

  if(i == ph)
    goto done;

  if(i < p2->bg_first_line) p2->bg_first_line = i;
  if(i+1 > p2->bg_last_line) p2->bg_last_line = i+1;
  p2->bg_next_line++;

  /* gray background checks */
  gdk_unlock();

  switch(bgmode){
  case SV_BG_WHITE:
    for(j=0;j<pw;j++)
      work_bg[j].u = 0xffffffffU;
    break;
  case SV_BG_BLACK:
    for(j=0;j<pw;j++)
      work_bg[j].u = 0xff000000U;
    break;
  default:
    render_checks(work_bg,pw,i);
    break;
  }

  /* by objective */
  for(j=0;j<p->objectives;j++){
    int o_ynum = p2->y_obj_from_panel[j];
    
    gdk_lock();
    if(plot_serialno != p->private->plot_serialno ||
       map_serialno != p->private->map_serialno) return -1;

    /**** mix Y plane */
    
    if(p2->y_planes[o_ynum]){
      int x;
      _sv_ucolor_t (*mixfunc)(_sv_ucolor_t,_sv_ucolor_t) = p2->mappings[j].mixfunc;
      _sv_ucolor_t *rect = p2->y_planes[o_ynum] + i*pw;
      memcpy(work_pl,rect,sizeof(work_pl));
      
      gdk_unlock();
      for(x=0;x<pw;x++)
	work_bg[x] = mixfunc(work_pl[x],work_bg[x]);
    }else
      gdk_unlock();

    /**** mix Z plane */
    
    /**** mix vector plane */

  }

  gdk_lock();
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno) return -1;

    // rendered a line, get it on the screen */
  
  memcpy(plot->datarect+pw*i, work_bg, sizeof(work_bg));

  p->private->map_complete_count--;

 done:
  if(p->private->map_complete_count)
    return 1; // not done yet

  return 0;
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

