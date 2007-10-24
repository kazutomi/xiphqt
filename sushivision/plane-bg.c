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
// other planes. The bg plane doesn't bother with functions embedded
// in the plane struct.  They're simply called directly by the panel
// code.

void bg_recompute_setup(sv_panel_t *p){
  sv_planebg_t *pl = p->bg;

  pl->image_x = _sv_dim_panelscale(payload + x_dim, p->w, 0);
  pl->image_y = _sv_dim_panelscale(payload + y_dim, p->h, 1);
  pl->image_task = 0;
  pl->expose_top = -1;
  pl->expose_bottom = -1;
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
  sv_planebg_t *bg = &p->bg;
  sv_plot_t *plot = p->plot;
  int serialno = p->legend_serialno;

  if(plot){
    int i,j;
    char buffer[320];
    int depth = 0;

    pthread_mutex_unlock(p->status_m);
    _sv_plot_legend_clear(plot);
    pthread_mutex_lock(p->status_m);
    if(serialno != p->legend_serialno)return STATUS_WORKING;

    // potentially add each dimension to the legend; add axis
    // dimensions only if crosshairs are active

    // display decimal precision relative to display scales
    if(3-_sv_scalespace_decimal_exponent(&bg->image_x) > depth) 
      depth = 3-_sv_scalespace_decimal_exponent(&bg->image_x);
    if(3-_sv_scalespace_decimal_exponent(&bg->image_y) > depth) 
      depth = 3-_sv_scalespace_decimal_exponent(&bg->image_y);
    for(i=0;i<p->dimensions;i++){
      sv_dim_data_t *d = p->dim_data+i;
      int flag=0;
      for(j=0;j<p->axes;j++)
	if(p->axis_dims[j] != i &&
	   (j<=2 || !p->cross_active)){
	  flag=1;
	  break;
	}

      if(!flag){
	snprintf(buffer,320,"%s = %+.*f",
		 d->legend,
		 depth,
		 d->val);
	pthread_mutex_unlock(p->status_m);
	_sv_plot_legend_add(plot,buffer);
	pthread_mutex_lock(p->status_m);
	if(serialno != p->legend_serialno)return STATUS_WORKING;
      }
    }
    
    // add each active objective plane to the legend
    // choose the value under the crosshairs 
    if(plot->cross_active){
      // ask each plane for entries...
      int firstflag=0;
      for(i=0;i<p->planes;i++){
	int retflag=p->plane_list[i]->legend(p->plane_list[i],buffer,320);
	pthread_mutex_unlock(p->status_m);
	if(retflag && !firstflag)
	  _sv_plot_legend_add(plot,NULL);
	firstflag|=retflag;
	_sv_plot_legend_add(plot,buffer);
	pthread_mutex_lock(p->status_m);
	if(serialno != p->legend_serialno)return STATUS_WORKING;
      }
    }
  }

  pthread_mutex_unlock(p->status_m);
  _sv_plot_draw_legend(plot);
  pthread_mutex_lock(p->status_m);
  if(serialno != p->legend_serialno)return STATUS_WORKING;

  return STATUS_IDLE;
}

int bg_scale(sv_panel_t *p){
  pthread_mutex_unlock(p->status_m);
  _sv_plot_draw_scales(p->graph);
  pthread_mutex_lock(p->status_m);
}

static void render_checks(sv_ucolor_t *c, int w, int y){
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

int bg_render(sv_panel_t *p){
  sv_planebg_t *bg = &p->bg;
  sv_plot_t *plot = p->plot;
  int serialno = p->compute_serialno;
  int w = p->w,y;
  int h = p->h;
  sv_ucolor_t line[w];

  int bgtype=0;
  double r;
  double g;
  double b;

  if(bg->bg[0]=='#' && strlen(bg->bg)==7){
    bgtype=1;
    r = (toupper(bg->bg[1])-65)*.0625 + (toupper(bg->bg[2])-65)*.00390625;
    g = (toupper(bg->bg[3])-65)*.0625 + (toupper(bg->bg[4])-65)*.00390625;
    b = (toupper(bg->bg[5])-65)*.0625 + (toupper(bg->bg[6])-65)*.00390625;
  }

  // look for a line that needs love
  do{
    y = bg->image_nextline;
    bg->image_nextline++;
    if(bg->image_nextline>=h)bg->image_nextline=0;
    
    if(bg->image_flags[y]){
      sv_scalespace_t ix = bg->image_x;
      sv_scalespace_t iy = bg->image_y;
      
      // render this line 
      bg->image_flags[i]=0;
      bg->image_outstanding++;

      pthread_mutex_unlock(p->status_m);

      // bg renders a line at a time primarily as a concession to the 2d panels.  
      if(bgtype==0){
	render_checks(line,w,y);
      }else{
	u_int32_t val = 0xff000000 | (r<<16) | (g<<8) | (b);
	for(i=0;i<w;i++)line[i]=val;
      }
      
      // grab line from panels in order.  Mix using plane native mix function
      for(i=0;i<p->planes;i++){
	sv_ucolor_t *mix(sv_ucolor_t, sv_ucolor_t) = p->plane_list[i]->c.mix;
	sv_ucolor_t *pline = p->plane_list[i]->c.image[y*w]l
	for(j=0;j<w;j++)
	  line[j] = mix(pline[j],line[j]);
      }

      pthread_mutex_lock(pl->status_m);
      
      if(p->compute_serialno == serialno){
	bg->image_outstanding--;

	plot_write_line(p->plot,line);

	//invalidate rectangle for expose
	if(bg->expose_top == -1 || bg->expose_top > y)
	  bg->expose_top=y;
	if(bg->expose_bottom == -1 || bg->expose_bottom < y)
	  bg->expose_bottom=y;
      }
      return STATUS_WORKING;
    }
  }while(i!=last);

  if(bg->image_outstanding) return STATUS_BUSY;
  return STATUS_IDLE;
}

int bg_expose(sv_panel_t *p){
  sv_planebg_t *bg = &p->bg;
  if(pl->expose_top != -1){
    GdkRectangle r;
    r.y = bg->expose_top;
    r.h = bg->expose_bottom - bg->expose_top+1;
    rx = 0;
    r.w = p->w;

    pl->expose_top = -1;
    pl->expose_bottom = -1;
    pthread_mutex_unlock(p->status_m);

    // the only place worker thread should ever touch the GDK lock
    gdk_threads_enter();
    gdk_invalidate_rectangle(GTK_WIDGET(p->plot),&r,FALSE);
    gdk_threads_leave();
    
  }

  return STATUS_IDLE;
}

sv_planebg_t *sv_planebg_new(sv_panel_t *p){
  sv_planebg_t *ret = calloc(1, sizeof(*ret));
  
  ret->panel=p;
  ret->bg = strdup("checks");
  return(ret);
}


