/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"
#include "gameboard_draw_button.h"


/************************ manage the buttons for buttonbar and dialogs *********************/

/* determine the x/y/w/h box around the rollover text */
static GdkRectangle rollover_box(Gameboard *g, buttonstate *b){
  GdkRectangle r;
  int width = g->g.width;
  int height = g->g.height;

  int x = b->x - b->ex.width/2 + b->ex.x_bearing -2;
  int y = b->y - BUTTON_RADIUS - BUTTON_TEXT_BORDER + b->ex.y_bearing -2;

  if(x<BUTTON_TEXT_BORDER)x=BUTTON_TEXT_BORDER;
  if(y<BUTTON_TEXT_BORDER)y=BUTTON_TEXT_BORDER;
  if(x+b->ex.width >= width - BUTTON_TEXT_BORDER)
    x = width - BUTTON_TEXT_BORDER - b->ex.width;
  if(y+b->ex.height >= height - BUTTON_TEXT_BORDER)
    y = height - BUTTON_TEXT_BORDER - b->ex.height;

  r.x=x;
  r.y=y;
  r.width=b->ex.width+5;
  r.height=b->ex.height+5;

  return r;
}

/* draw the actual rollover */
static void stroke_rollover(Gameboard *g, buttonstate *b, cairo_t *c){
  cairo_matrix_t m;

  cairo_select_font_face (c, "Arial",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);

  cairo_matrix_init_scale (&m, BUTTON_TEXT_SIZE);
  cairo_set_font_matrix (c,&m);
  
  {
    GdkRectangle r=rollover_box(g,b);

    cairo_move_to (c, r.x - b->ex.x_bearing +2, r.y -b->ex.y_bearing+2 );

    cairo_set_line_width(c,3);
    cairo_set_source_rgba(c,1,1,1,.9);
    cairo_text_path (c, b->rollovertext);  
    cairo_stroke(c);

    cairo_set_source_rgba(c,BUTTON_TEXT_COLOR);
    cairo_move_to (c, r.x - b->ex.x_bearing +2, r.y -b->ex.y_bearing+2 );
    

    cairo_show_text (c, b->rollovertext);  

  }
}

/* request an expose for a button region*/
static void invalidate_button(Gameboard *g,buttonstate *b){
  GdkRectangle r;

  r.x=b->x-BUTTON_RADIUS-1;
  r.y=b->y-BUTTON_RADIUS-1;
  r.width=BUTTON_RADIUS*2+2;
  r.height=BUTTON_RADIUS*2+2;

  gdk_window_invalidate_rect(g->w.window,&r,FALSE);
}

/* request an expose for a rollover region*/
static void invalidate_rollover(Gameboard *g,buttonstate *b){
  GdkRectangle r = rollover_box(g,b);
  invalidate_button(g,b);
  gdk_window_invalidate_rect(g->w.window,&r,FALSE);
}

/* like invalidation, but just expands a rectangular region */
static void expand_rectangle_button(buttonstate *b,GdkRectangle *r){
  int x=b->x-BUTTON_RADIUS-1;
  int y=b->y-BUTTON_RADIUS-1;
  int x2=x+BUTTON_RADIUS*2+2;
  int y2=y+BUTTON_RADIUS*2+2;

  int rx2=r->x+r->width;
  int ry2=r->y+r->height;

  if(r->width==0 || r->height==0){
    r->x=x;
    r->y=y;
    r->width=x2-x;
    r->height=y2-y;
  }

  if(x<r->x)
    r->x=x;
  if(y<r->y)
    r->y=y;
  if(rx2<x2)
    rx2=x2;
  r->width=rx2-r->x;
  if(ry2<y2)
    ry2=y2;
  r->height=ry2-r->y;
}

/* like invalidation, but just expands a rectangular region */
static void expand_rectangle_rollover(Gameboard *g,buttonstate *b,GdkRectangle *r){
  GdkRectangle rr = rollover_box(g,b);
  int x=rr.x;
  int y=rr.y;
  int x2=x+rr.width;
  int y2=y+rr.height;

  int rx2=r->x+r->width;
  int ry2=r->y+r->height;

  if(r->width==0 || r->height==0){
    r->x=x;
    r->y=y;
    r->width=x2-x;
    r->height=y2-y;
  }

  if(x<r->x)
    r->x=x;
  if(y<r->y)
    r->y=y;
  if(rx2<x2)
    rx2=x2;
  r->width=rx2-r->x;
  if(ry2<y2)
    ry2=y2;
  r->height=ry2-r->y;
}

/* cache buttons as small surfaces */
static cairo_surface_t *cache_button(Gameboard *g,
				     void (*draw)(cairo_t *c, 
						  double x, 
						  double y),
				     double pR,double pG,double pB,double pA,
				     double fR,double fG,double fB,double fA){
  cairo_surface_t *ret = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  BUTTON_RADIUS*2+1,
				  BUTTON_RADIUS*2+1);
  
  cairo_t *c = cairo_create(ret);

  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);
     
  cairo_set_source_rgba(c,fR,fG,fB,fA);
  cairo_set_line_width(c,BUTTON_LINE_WIDTH);
  draw(c,BUTTON_RADIUS+.5,BUTTON_RADIUS+.5);

  cairo_set_source_rgba(c,pR,pG,pB,pA);
  cairo_stroke(c);
  
  cairo_destroy(c);
  return ret;
}

/* determine the x/y/w/h box around a button */
static GdkRectangle button_box(buttonstate *b){
  GdkRectangle r;

  int x = b->x - BUTTON_RADIUS-1;
  int y = b->y - BUTTON_RADIUS-1;
  r.x=x;
  r.y=y;
  r.width = BUTTON_RADIUS*2+2;
  r.height= BUTTON_RADIUS*2+2;

  return r;
}

static int animate_x_axis(Gameboard *g, buttonstate *b, GdkRectangle *r){
  int ret=0;

  if(b->x_target != b->x){
    ret=1;
    expand_rectangle_button(b,r);
    if(b->rollover)
      expand_rectangle_rollover(g,b,r);

    if(b->x_target > b->x){
      b->x+=DEPLOY_DELTA;
      if(b->x>b->x_target)b->x=b->x_target;
    }else{
      b->x-=DEPLOY_DELTA;
      if(b->x<b->x_target)b->x=b->x_target;
    }
    expand_rectangle_button(b,r);
    if(b->rollover)
      expand_rectangle_rollover(g,b,r);
  }

  return ret;
}
static int animate_y_axis(Gameboard *g, buttonstate *b, GdkRectangle *r){
  int ret=0;

  if(b->y_target != b->y){
    ret=1;
    expand_rectangle_button(b,r);
    if(b->rollover)
      expand_rectangle_rollover(g,b,r);

    if(b->y_target > b->y){
      b->y+=DEPLOY_DELTA;
      if(b->y>b->y_target)b->y=b->y_target;
    }else{
      b->y-=DEPLOY_DELTA;
      if(b->y<b->y_target)b->y=b->y_target;
    }
    expand_rectangle_button(b,r);
    if(b->rollover)
      expand_rectangle_rollover(g,b,r);
  }

  if(b->alphad != b->y_active - b->y){
    double alpha=0;
    if(b->y_inactive>b->y_active){
      if(b->y<=b->y_active)
	alpha=1.;
      else if (b->y>=b->y_inactive)
	alpha=0.;
      else
	alpha = (double)(b->y_inactive-b->y)/(b->y_inactive-b->y_active);
    }else if (b->y_inactive<b->y_active){
      if(b->y>=b->y_active)
	alpha=1.;
      else if (b->y<=b->y_inactive)
	alpha=0.;
      else
	alpha = (double)(b->y_inactive-b->y)/(b->y_inactive-b->y_active);
    }
    if(alpha != b->alpha){
      ret=1;
      expand_rectangle_button(b,r);
      b->alpha=alpha;
    }
    b->alphad = b->y_active - b->y;
  }

  return ret;
}

/* do the animation math for one button for one frame */
static int animate_one(Gameboard *g,buttonstate *b, GdkRectangle *r){
  int ret=0;

  /* does this button need to be deployed? */
  if(g->b.sweeperd>0){
    if(b->y_target != b->y_active){
      if(g->b.sweeper >= b->sweepdeploy){
	b->y_target=b->y_active;
	ret=1;
      }
    }
  }
  /* does this button need to be undeployed? */
  if(g->b.sweeperd<0){
    if(b->y_target != b->y_inactive){
      if(-g->b.sweeper >= b->sweepdeploy){
	b->y_target=b->y_inactive;
	ret=1;
      }
    }
  }

  ret |= animate_x_axis(g,b,r);
  ret |= animate_y_axis(g,b,r);
  
  return ret;
}

/******************** toplevel abstraction calls *********************/

/* initialize the persistent caches; called once when gameboard is
   first created */
void init_buttons(Gameboard *g){
  buttonstate *states=g->b.states;
  memset(g->b.states,0,sizeof(g->b.states));

  states[0].idle = cache_button(g, path_button_exit, 
				BUTTON_QUIT_IDLE_PATH, 
				BUTTON_QUIT_IDLE_FILL);
  states[0].lit  = cache_button(g, path_button_exit, 
				BUTTON_QUIT_LIT_PATH, 
				BUTTON_QUIT_LIT_FILL);

  states[1].idle = cache_button(g, path_button_back, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[1].lit  = cache_button(g, path_button_back, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[2].idle = cache_button(g, path_button_reset, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[2].lit  = cache_button(g, path_button_reset, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[3].idle = cache_button(g, path_button_pause, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[3].lit  = cache_button(g, path_button_pause, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[4].idle = cache_button(g, path_button_help, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[4].lit  = cache_button(g, path_button_help, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[5].idle = cache_button(g, path_button_expand, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[5].lit  = cache_button(g, path_button_expand, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[6].idle = cache_button(g, path_button_shrink, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[6].lit  = cache_button(g, path_button_shrink, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[7].idle = cache_button(g, path_button_lines, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[7].lit  = cache_button(g, path_button_lines, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[8].idle = cache_button(g, path_button_int, 
				BUTTON_IDLE_PATH,
				BUTTON_IDLE_FILL);
  states[8].lit  = cache_button(g, path_button_int, 
				BUTTON_LIT_PATH,
				BUTTON_LIT_FILL);
  states[9].idle = cache_button(g, path_button_check, 
				BUTTON_CHECK_IDLE_PATH,
				BUTTON_CHECK_IDLE_FILL);
  states[9].lit  = cache_button(g, path_button_check, 
				BUTTON_CHECK_LIT_PATH,
				BUTTON_CHECK_LIT_FILL);
  states[10].idle= cache_button(g, path_button_play, 
				BUTTON_CHECK_IDLE_PATH,
				BUTTON_CHECK_IDLE_FILL);
  states[10].lit = cache_button(g, path_button_play, 
				BUTTON_CHECK_LIT_PATH,
				BUTTON_CHECK_LIT_FILL);
}

/* match x/y to a button if any */
buttonstate *find_button(Gameboard *g, int x,int y){
  int i;
  buttonstate *states=g->b.states;

  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *b=states+i;
    if(b->position>0)
      if( (b->x-x)*(b->x-x) + (b->y-y)*(b->y-y) <= BUTTON_RADIUS*BUTTON_RADIUS+4)
	if(b->y != b->y_inactive)
	  return b;
  }
  return 0;
}

/* set a button to pressed or lit */
void button_set_state(Gameboard *g, buttonstate *b, int rollover, int press){
  int i;
  buttonstate *states=g->b.states;
  
  if(b->rollover == rollover && b->press == press)return;

  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *bb=states+i;
    if(bb->position>0){
      if(bb!=b){
	if(bb->rollover)
	  invalidate_rollover(g,bb);
	if(bb->press)
	  invalidate_button(g,bb);
	
	bb->rollover=0;
	bb->press=0;
      }else{
	if(bb->rollover != rollover)
	  invalidate_rollover(g,bb);
	if(bb->press != press)
	  invalidate_button(g,bb);
	
	bb->rollover=rollover;
	bb->press=press;
      }
    }
  }
  g->b.allclear=0;
}

/* cache the text extents of a rollover */
void rollover_extents(Gameboard *g, buttonstate *b){
  
  cairo_matrix_t m;

  cairo_t *c = cairo_create(g->foreground);
  cairo_select_font_face (c, "Arial",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
  
  cairo_matrix_init_scale (&m, BUTTON_TEXT_SIZE);
  cairo_set_font_matrix (c,&m);
  
  cairo_text_extents (c, b->rollovertext, &b->ex);

  cairo_destroy(c);
}

/* perform a single frame of animation for all buttons/rollovers */
gboolean animate_button_frame(gpointer ptr){
  Gameboard *g=(Gameboard *)ptr;
  GdkRectangle expose;
  buttonstate *states=g->b.states;
  int ret=0,i,pos;
  
  if(!g->first_expose)return 1;

  g->b.sweeper += g->b.sweeperd;
  
  /* avoid the normal expose event mechanism
     during the button animations.  This direct method is much faster as
     it won't accidentally combine exposes over long stretches of
     alpha-blended surfaces. */
  
  for(pos=1;pos<=3;pos++){
    memset(&expose,0,sizeof(expose));
    for(i=0;i<NUMBUTTONS;i++)
      if(states[i].position == pos)
	ret|=animate_one(g,states+i,&expose);
    if(expose.width && expose.height)
      gameboard_draw(g,
		     expose.x,
		     expose.y,
		     expose.width,
		     expose.height);
  }

  if(!ret)g->b.sweeperd = 0;

  if(!ret && g->gtk_timer!=0){
    g_source_remove(g->gtk_timer);
    g->gtk_timer=0;
  }

  if(!ret && g->button_callback)
    // undeploy finished... call the undeploy callback
    g->button_callback(g);

  return ret;
}

/* the normal expose method for all buttons; called from the master
   widget's expose */
void expose_buttons(Gameboard *g,cairo_t *c, int x,int y,int w,int h){
  int i;
  buttonstate *states=g->b.states;
  
  for(i=0;i<NUMBUTTONS;i++){

    buttonstate *b=states+i;

    if(b->position>0){
      GdkRectangle r = rollover_box(g,b);
      GdkRectangle br = button_box(b);
      
      if(x < br.x+br.width &&
	 y < br.y+br.height &&
	 x+w > br.x &&
       y+h > br.y) {
	
	cairo_set_source_surface(c,
				 (b->rollover || b->press ? b->lit : b->idle),
				 br.x,
				 br.y);
	
	if(b->press)
	  cairo_paint_with_alpha(c,b->alpha);
	cairo_paint_with_alpha(c,b->alpha);
	
      }
      
      if((b->rollover || b->press) && b->y_target!=b->y_inactive)
	if(x < r.x+r.width &&
	   y < r.y+r.height &&
	   x+w > r.x &&
	   y+h > r.y)
	  
	  stroke_rollover(g,b,c);
    }
  }
}

/* resize the button bar; called from master resize in gameboard */
void resize_buttons(Gameboard *g,int oldw,int oldh,int w,int h){
  int i;
  int dx=w/2-oldw/2;
  int dy=h/2-oldh/2;
  buttonstate *states=g->b.states;

  for(i=0;i<NUMBUTTONS;i++){
    if(abs(states[i].position) == 2){

      states[i].x+=dx;
      states[i].x_target+=dx;

      states[i].y+=dy;
      states[i].y_target+=dy;
      states[i].y_active+=dy;
      states[i].y_inactive+=dy;
    }
  }

  dx=w-oldw;
  dy=h-oldh;

  for(i=0;i<NUMBUTTONS;i++){
    if(abs(states[i].position)==1 || 
       abs(states[i].position)==3){
      states[i].y+=dy;
      states[i].y_target+=dy;
      states[i].y_active+=dy;
      states[i].y_inactive+=dy;
    }
  }
  
  for(i=0;i<NUMBUTTONS;i++){
    if(abs(states[i].position) == 3){
      states[i].x+=dx;
      states[i].x_target+=dx;
    }
  }
}

/* clear all buttons to unpressed/unlit */
void button_clear_state(Gameboard *g){
  int i;
  buttonstate *states=g->b.states;

  if(!g->b.allclear){
    for(i=0;i<NUMBUTTONS;i++){
      buttonstate *bb=states+i;
      if(bb->position){
	if(bb->rollover)
	  invalidate_rollover(g,bb);
	if(bb->press)
	  invalidate_button(g,bb);
	
	bb->rollover=0;
	bb->press=0;
      }
    }
  }
  g->b.allclear=1;
  g->b.grabbed=0;
}

void deploy_buttons(Gameboard *g, void (*callback)(Gameboard *g)){

  if(!g->b.buttons_ready){
    
    // sweep across the buttons inward from the horizontal edges; when
    // the sweep passes a button it is set to deploy by making the
    // target y equal to the active y target.
    
    g->b.sweeper  = 0;
    g->b.sweeperd = 1;

    g->button_callback = callback;
    if(g->gtk_timer!=0)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
    g->b.buttons_ready=1;
  }

}

void undeploy_buttons(Gameboard *g, void (*callback)(Gameboard *ptr)){

  if(g->b.buttons_ready){
    
    button_clear_state(g);
    g->b.buttons_ready=0;

    // sweep across the buttons outward from the center; when
    // the sweep passes a button it is set to undeploy by making the
    // target y equal to the inactive y target.

    g->b.sweeper  = 0;
    g->b.sweeperd = -1;

    g->button_callback = callback;
    if(g->gtk_timer!=0)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
  }else
    callback(g);

}
