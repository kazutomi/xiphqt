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
#include "timer.h"
#include "gameboard.h"
#include "gameboard_draw_button.h"
#include "dialog_level.h"
#include "levelstate.h"
#include "main.h"

static void draw_forward_arrow(graph *g,dialog_level_oneicon *l, cairo_t *c,int fill){
  int w = l->w;
  int h = l->h;
  int cx = w/2;
  int cy = h/2;
  cairo_save(c);
  cairo_translate(c,l->x+g->width/2+.5,l->y+g->height/2+.5);

  cairo_set_line_width(c,B_LINE);

  cairo_move_to(c,w-10,cy);
  cairo_line_to(c,cx,h-10);
  cairo_line_to(c,cx,(int)h*.75);
  cairo_line_to(c,10,(int)h*.75);
  cairo_line_to(c,10,(int)h*.25);
  cairo_line_to(c,cx,(int)h*.25);
  cairo_line_to(c,cx,10);
  cairo_close_path(c);

  if(fill){
    cairo_set_source_rgba (c, B_COLOR);
    cairo_fill_preserve (c);
  }
  cairo_set_source_rgba (c, B_LINE_COLOR);
  cairo_stroke (c);
  cairo_restore(c);
}

static void draw_backward_arrow(graph *g,dialog_level_oneicon *l, cairo_t *c,int fill){
  int w = l->w;
  int h = l->h;
  int cx = w/2;
  int cy = h/2;
  cairo_save(c);
  cairo_translate(c,l->x+g->width/2-.5,l->y+g->height/2+.5);

  cairo_set_line_width(c,B_LINE);

  cairo_move_to(c,10,cy);
  cairo_line_to(c,cx,h-10);
  cairo_line_to(c,cx,(int)h*.75);
  cairo_line_to(c,w-10,(int)h*.75);
  cairo_line_to(c,w-10,(int)h*.25);
  cairo_line_to(c,cx,(int)h*.25);
  cairo_line_to(c,cx,10);
  cairo_close_path(c);

  if(fill){
    cairo_set_source_rgba (c, B_COLOR);
    cairo_fill_preserve (c);
  }
  cairo_set_source_rgba (c, B_LINE_COLOR);
  cairo_stroke (c);
  cairo_restore(c);
}

static void invalidate_icon(Gameboard *g,dialog_level_oneicon *l){
  int cx = g->g.width/2;
  int cy = g->g.height/2;
  GdkRectangle r;
  
  r.x=l->x+cx+g->d.center_x;
  r.y=l->y+cy;
  r.width=l->w;
  r.height=l->h;

  gdk_window_invalidate_rect (GTK_WIDGET(g)->window, &r, FALSE);
}

static void dialog_level_oneicon_init(Gameboard *g, int num, dialog_level_oneicon *l){
  int current = get_level_num();
  l->num = num+current;

  if(l->icon)cairo_surface_destroy(l->icon);
  l->icon = levelstate_get_icon(num + current);
  l->alpha = 0.;

  l->w  = ICON_WIDTH;
  l->h  = ICON_HEIGHT;  
  l->x  = num*ICON_WIDTH*1.2 - (l->w*.5);
  l->y  = 120 - (LEVELBOX_HEIGHT) / 2;
  
}

static void deploy_reset_button(Gameboard *g){
  buttonstate *states=g->b.states;

  if(!g->d.reset_deployed){
    states[10].sweepdeploy += SWEEP_DELTA;

    states[2].position = 2; //activate it
    states[2].y_target = states[2].y_active;
   
    g->d.reset_deployed=1;
    
    if(g->b.buttons_ready){
      if(g->button_timer!=0)
	g_source_remove(g->button_timer);
      g->button_callback=0;
      g->button_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
    }
  }
}

static void undeploy_reset_button(Gameboard *g){
  buttonstate *states=g->b.states;

  if(g->d.reset_deployed){

    states[10].sweepdeploy -= SWEEP_DELTA;
    states[2].y_target = states[2].y_inactive;
    
    g->d.reset_deployed=0;

    if(g->b.buttons_ready){
      if(g->button_timer!=0)
	g_source_remove(g->button_timer);
      g->button_callback = 0;
      g->button_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
    } 
  }
}

static void alpha_update(Gameboard *g,dialog_level_oneicon *l){
  int distance = labs(l->x - g->d.level_icons[2].x + g->d.center_x);
  double alpha = 1. - (distance/300.);
  if(alpha<0.)alpha=0.;
  //if(alpha>1.)alpha=1.;
  l->alpha = alpha;
}

void level_icons_init(Gameboard *g){
  int i;
  
  for(i=0;i<5;i++)
    dialog_level_oneicon_init(g,i-2,g->d.level_icons+i);

  g->d.center_x = 0;
  g->d.center_done=1;
  g->d.level_lit = 2;
  g->d.reset_deployed = 0;

  if(levelstate_in_progress())
    deploy_reset_button(g);

  memset(&g->d.text1,0,sizeof(g->d.text1));
  memset(&g->d.text2,0,sizeof(g->d.text2));
  memset(&g->d.text3,0,sizeof(g->d.text3));
  memset(&g->d.text4,0,sizeof(g->d.text4));
}

void render_level_icons(Gameboard *g, cairo_t *c, int ex,int ey, int ew, int eh){
  if(g->level_dialog_active){
    int w= g->g.width;
    int h= g->g.height;
    int y = h/2-LEVELBOX_HEIGHT/2+SCOREHEIGHT/2;
    int i;

    int ex2 = ex+ew;
    int ey2 = ey+eh;
    
    for(i=0;i<5;i++){
      dialog_level_oneicon *l=g->d.level_icons+i;
      alpha_update(g,l);
      if(l->num >= 0 && l->num < levelstate_limit() && c){
	
	int iw = l->w;
	int ih = l->h;
	int ix = l->x+g->d.center_x+w/2;	
	int iy = l->y+h/2;
	
	if( l->alpha == 0.) continue;

	if( ix+iw < ex ) continue;
	if( ix > ex2 ) continue;
	if( iy+ih < ey ) continue;
	if( iy > ey2 ) continue;

	if(l->icon && cairo_surface_status(l->icon)==CAIRO_STATUS_SUCCESS){
	  cairo_set_source_surface(c,l->icon,ix,iy);
	  cairo_paint_with_alpha(c,l->alpha);
	}

	if(g->d.center_x==0 && g->d.center_done){
	  if(i==2){
	    cairo_set_source_rgba (c, B_COLOR);
	    borderbox_path(c,ix+1.5,iy+1.5,iw-3,ih-3);
	    cairo_fill (c);
	  }

	  if(i==1)
	    draw_backward_arrow(&g->g,l,c,i==g->d.level_lit);
	  if(i==3)
	    draw_forward_arrow(&g->g,l,c,i==g->d.level_lit);
	}
      }
    }

    // render level related text
    if(g->d.center_x == 0 && g->d.center_done && c){
      char buffer[160];

      // above text
      if(g->d.text1.width==0 || 
	 (ey<g->d.text1.y+g->d.text1.height && ey2>g->d.text1.y)){
	
	snprintf(buffer,160,_("Level %d:"),get_level_num()+1);
	set_font(c,20,20,0,1);
	cairo_set_source_rgba (c, TEXT_COLOR);
	g->d.text1=render_bordertext_centered(c, buffer,w/2,y+45);
      }
      
      if(g->d.text2.width==0 || 
	 (ey<g->d.text2.y+g->d.text2.height && ey2>g->d.text2.y)){
	set_font(c,18,18,0,0);
	cairo_set_source_rgba (c, TEXT_COLOR);
	g->d.text2=render_bordertext_centered(c, get_level_desc(),w/2,y+70);
      }

      if(g->d.text3.width==0 || 
	 (ey<g->d.text3.y+g->d.text3.height && ey2>g->d.text3.y)){
	
	if(levelstate_get_hiscore()==0){
	  set_font(c,18,18,1,0);
	  snprintf(buffer,160,_("[not yet completed]"));
	}else{
	  set_font(c,18,18,0,0);
	  snprintf(buffer,160,_("level high score: %ld"),levelstate_get_hiscore());
	}
	
	cairo_set_source_rgba (c, TEXT_COLOR);
	g->d.text3=render_bordertext_centered(c, buffer,w/2,y+245);
      }

      if(g->d.text4.width==0 || 
	 (ey<g->d.text4.y+g->d.text4.height && ey2>g->d.text4.y)){

	snprintf(buffer,160,_("total score all levels: %ld"),levelstate_total_hiscore());
	
	set_font(c,18,18,0,0);
	cairo_set_source_rgba (c, TEXT_COLOR);
	g->d.text4=render_bordertext_centered(c, buffer,w/2,y+265);
      }
    }else{
      memset(&g->d.text1,0,sizeof(g->d.text1));
      memset(&g->d.text2,0,sizeof(g->d.text2));
      memset(&g->d.text3,0,sizeof(g->d.text3));
      memset(&g->d.text4,0,sizeof(g->d.text4));
    }      
  }
}

static gboolean animate_level_frame(gpointer ptr){
  Gameboard *g = GAMEBOARD (ptr);
  int i;

  if(g->d.center_x == 0){
    g->d.center_done=1;
    g_source_remove(g->d.icon_timer);
    g->d.icon_timer=0;

    if(levelstate_in_progress())
      deploy_reset_button(g);
    
    expose_full(g);
    return 0;
  }

  g->d.center_done=0;
  for(i=0;i<5;i++)
    if(g->d.level_icons[i].alpha)
      invalidate_icon(g, g->d.level_icons+i);

  if(g->d.center_x < 0){
    g->d.center_x += ICON_DELTA;
    if(g->d.center_x > 0) g->d.center_x = 0;
  }else{
    g->d.center_x -= ICON_DELTA;
    if(g->d.center_x < 0) g->d.center_x = 0;
  }

  // trick 'expose'; run it with a region that does nothing in order
  // to update the visibility flags
  
  render_level_icons(g, 0, 0, 0, 0, 0);

  for(i=0;i<5;i++)
    if(g->d.level_icons[i].alpha)
      invalidate_icon(g, g->d.level_icons+i);

  return 1;
}

static int find_icon(Gameboard *b,graph *g, int x, int y){
  int i;
  int w= g->width;
  int h= g->height;

  x-=w/2;
  y-=h/2;

  for(i=1;i<4;i++){
    dialog_level_oneicon *l=b->d.level_icons+i;
    if(l->num>=0 && l->num<levelstate_limit()){
      if(x >= l->x && 
	 x <= l->x + l->w &&
	 y >= l->y &&
	 y <= l->y + l->h)
	return i;
    }
  }
  return 2;
}

void local_reset (Gameboard *g){
  levelstate_reset();
  dialog_level_oneicon_init(g,0,g->d.level_icons+2);
  invalidate_icon(g, g->d.level_icons+2);
  undeploy_reset_button(g);
}

void level_mouse_motion(Gameboard *g, int x, int y){

  int icon = find_icon(g,&g->g,x,y);

  if(icon != g->d.level_lit){
    invalidate_icon(g, g->d.level_icons+g->d.level_lit);
    g->d.level_lit = icon;
    invalidate_icon(g, g->d.level_icons+g->d.level_lit);
  }
}

void level_mouse_press(Gameboard *g, int x, int y){
  int i;

  level_mouse_motion(g, x, y);

  if(g->d.level_lit == 1){
    if(levelstate_prev()){

      if(!levelstate_in_progress())
	undeploy_reset_button(g);
      
      if(g->d.level_icons[4].icon)
	cairo_surface_destroy(g->d.level_icons[4].icon);
      for(i=4;i>=1;i--){
	g->d.level_icons[i].num = g->d.level_icons[i-1].num;
	g->d.level_icons[i].icon = g->d.level_icons[i-1].icon;
      }
      g->d.level_icons[0].icon=0;
      dialog_level_oneicon_init(g,-2,g->d.level_icons);
      
      if(g->d.center_x==0 && g->d.center_done)expose_full(g); // only needed to 'undraw' the text

      g->d.center_x = g->d.level_icons[1].x - g->d.level_icons[2].x;
    }
  }
  
  if(g->d.level_lit == 3){
    if(levelstate_next()){

      if(!levelstate_in_progress())
	undeploy_reset_button(g);
      
      if(g->d.level_icons[0].icon)
	cairo_surface_destroy(g->d.level_icons[0].icon);
      for(i=0;i<4;i++){
	g->d.level_icons[i].num = g->d.level_icons[i+1].num;
	g->d.level_icons[i].icon = g->d.level_icons[i+1].icon;
      }
      g->d.level_icons[4].icon=0;
      dialog_level_oneicon_init(g,2,g->d.level_icons+4);
      
      if(g->d.center_x==0 && g->d.center_done)expose_full(g); // only needed to 'undraw' the text

      g->d.center_x = g->d.level_icons[3].x - g->d.level_icons[2].x;

    }
  }

  if(g->d.center_x){
    if(g->d.icon_timer)
      g_source_remove(g->d.icon_timer);
    g->d.icon_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_level_frame, (gpointer)g);
  }

}

  
  
