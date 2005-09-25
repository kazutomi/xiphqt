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

typedef struct {
  int num;
  double alpha;
  cairo_surface_t *icon;

  int x;
  int y;
  int w;
  int h;

} onelevel;

static onelevel level_icons[5];
static int center_x;
static int level_lit;
static int reset_deployed;
static GdkRectangle text1;
static GdkRectangle text2;
static GdkRectangle text3;
static GdkRectangle text4;

static void draw_forward_arrow(onelevel *l, cairo_t *c,int fill){
  int w = l->w;
  int h = l->h;
  int cx = w/2;
  int cy = h/2;
  cairo_save(c);
  cairo_translate(c,l->x+.5,l->y+.5);

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

static void draw_backward_arrow(onelevel *l, cairo_t *c,int fill){
  int w = l->w;
  int h = l->h;
  int cx = w/2;
  int cy = h/2;
  cairo_save(c);
  cairo_translate(c,l->x-.5,l->y+.5);

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

static void invalidate_icon(Gameboard *g,onelevel *l){
  GdkRectangle r;
  
  r.x=l->x+center_x;
  r.y=l->y;
  r.width=l->w;
  r.height=l->h;

  gdk_window_invalidate_rect (GTK_WIDGET(g)->window, &r, FALSE);
}

static void onelevel_init(Gameboard *g, int num, onelevel *l){
  int current = get_level_num();
  l->num = num+current;

  if(l->icon)cairo_surface_destroy(l->icon);
  l->icon = levelstate_get_icon(num + current);
  l->alpha = 0.;

  l->w  = ICON_WIDTH;
  l->h  = ICON_HEIGHT;  
  l->x  =  (g->g.width/2) - (l->w*.5) + num*ICON_WIDTH*1.2;
  l->y  = (g->g.height - LEVELBOX_HEIGHT) / 2 + 120;
  
}

static void deploy_reset_button(Gameboard *g){
  buttonstate *states=g->b.states;

  if(!reset_deployed){
    states[10].sweepdeploy += SWEEP_DELTA;

    states[2].position = 2; //activate it
    states[2].y_target = states[2].y_active;
   
    reset_deployed=1;
  }
  
  // even if the button is already 'deployed', the animation may
  // have been interrupted.  Retrigger.

  if(g->b.buttons_ready){
    if(g->gtk_timer!=0)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
  }

}

static void undeploy_reset_button(Gameboard *g){
  buttonstate *states=g->b.states;

  if(reset_deployed){

    states[10].sweepdeploy -= SWEEP_DELTA;
    states[2].y_target = states[2].y_inactive;
    
    reset_deployed=0;
  }

  // even if the button is already 'undeployed', the animation may
  // have been interrupted.  Retrigger.
  if(g->b.buttons_ready){
    if(g->gtk_timer!=0)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_button_frame, (gpointer)g);
  } 
}

static void alpha_update(onelevel *l){
  int distance = labs(l->x - level_icons[2].x + center_x);
  double alpha = 1. - (distance/300.);
  if(alpha<0.)alpha=0.;
  //if(alpha>1.)alpha=1.;
  l->alpha = alpha;
}

void level_icons_init(Gameboard *g){
  int i;
  
  for(i=0;i<5;i++)
    onelevel_init(g,i-2,level_icons+i);

  center_x = 0;
  level_lit = 2;
  reset_deployed = 0;

  if(levelstate_in_progress())
    deploy_reset_button(g);

  memset(&text1,0,sizeof(text1));
  memset(&text2,0,sizeof(text2));
  memset(&text3,0,sizeof(text3));
  memset(&text4,0,sizeof(text4));
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
      onelevel *l=level_icons+i;
      alpha_update(l);
      if(l->num >= 0 && c){
	
	int iw = l->w;
	int ih = l->h;
	int ix = l->x+center_x;	
	int iy = l->y;
	
	if( l->alpha == 0.) continue;

	if( ix+iw < ex ) continue;
	if( ix > ex2 ) continue;
	if( iy+ih < ey ) continue;
	if( iy > ey2 ) continue;

	if(l->icon){
	  cairo_set_source_surface(c,l->icon,ix,iy);
	  cairo_paint_with_alpha(c,l->alpha);
	}

	if(center_x==0){
	  if(i==2){
	    cairo_set_source_rgba (c, B_COLOR);
	    borderbox_path(c,ix+1.5,iy+1.5,iw-3,ih-3);
	    cairo_fill (c);
	  }

	  if(i==1)
	    draw_backward_arrow(l,c,i==level_lit);
	  if(i==3)
	    draw_forward_arrow(l,c,i==level_lit);
	}
      }
    }

    // render level related text
    if(center_x == 0 && c){
      char buffer[160];
      cairo_matrix_t ma;

      // above text
      if(text1.width==0 || 
	 (ey<text1.y+text1.height && ey2>text1.y)){
	
	snprintf(buffer,160,"Level %d:",get_level_num()+1);
	cairo_select_font_face (c, "Arial",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
	cairo_matrix_init_scale (&ma, 20.,20.);
	cairo_set_font_matrix (c,&ma);
	cairo_set_source_rgba (c, TEXT_COLOR);
	text1=render_bordertext_centered(c, buffer,w/2,y+45);
      }
      
      if(text2.width==0 || 
	 (ey<text2.y+text2.height && ey2>text2.y)){
	cairo_select_font_face (c, "Arial",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_matrix_init_scale (&ma, 18.,18.);
	cairo_set_font_matrix (c,&ma);
	cairo_set_source_rgba (c, TEXT_COLOR);
	text2=render_bordertext_centered(c, get_level_desc(),w/2,y+70);
      }

      if(text3.width==0 || 
	 (ey<text3.y+text3.height && ey2>text3.y)){
	
	if(levelstate_get_hiscore()==0){
	  cairo_select_font_face (c, "Arial",
				  CAIRO_FONT_SLANT_ITALIC,
				  CAIRO_FONT_WEIGHT_NORMAL);
	  snprintf(buffer,160,"[not yet completed]");
	}else{
	  cairo_select_font_face (c, "Arial",
				  CAIRO_FONT_SLANT_NORMAL,
				  CAIRO_FONT_WEIGHT_NORMAL);
	  snprintf(buffer,160,"level high score: %ld",levelstate_get_hiscore());
	}
	
	cairo_matrix_init_scale (&ma, 18.,18.);
	cairo_set_font_matrix (c,&ma);
	cairo_set_source_rgba (c, TEXT_COLOR);
	text3=render_bordertext_centered(c, buffer,w/2,y+245);
      }

      if(text4.width==0 || 
	 (ey<text4.y+text4.height && ey2>text4.y)){

	snprintf(buffer,160,"total score all levels: %ld",levelstate_total_hiscore());
	
	cairo_select_font_face (c, "Arial",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_matrix_init_scale (&ma, 18.,18.);
	cairo_set_font_matrix (c,&ma);
	cairo_set_source_rgba (c, TEXT_COLOR);
	text4=render_bordertext_centered(c, buffer,w/2,y+265);
      }
    }else{
      memset(&text1,0,sizeof(text1));
      memset(&text2,0,sizeof(text2));
      memset(&text3,0,sizeof(text3));
      memset(&text4,0,sizeof(text4));
    }      
  }
}

static gboolean animate_level_frame(gpointer ptr){
  Gameboard *g = GAMEBOARD (ptr);
  int i;

  if(center_x == 0){
    g_source_remove(g->gtk_timer);
    g->gtk_timer=0;

    if(levelstate_in_progress())
      deploy_reset_button(g);
    else
      undeploy_reset_button(g);
    
    expose_full(g);
    return 0;
  }

  for(i=0;i<5;i++)
    if(level_icons[i].alpha)
      invalidate_icon(g, level_icons+i);

  if(center_x < 0){
    center_x += ICON_DELTA;
    if(center_x > 0) center_x = 0;
  }else{
    center_x -= ICON_DELTA;
    if(center_x < 0) center_x = 0;
  }

  // trick 'expose'; run it with a region that does nothing in order
  // to update the visibility flags
  
  render_level_icons(g, 0, 0, 0, 0, 0);

  for(i=0;i<5;i++)
    if(level_icons[i].alpha)
      invalidate_icon(g, level_icons+i);

  return 1;
}

static int find_icon(int x, int y){
  int i;
  
  for(i=1;i<4;i++){
    onelevel *l=level_icons+i;
    if(l->num>=0){
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
  onelevel_init(g,0,level_icons+2);
  invalidate_icon(g, level_icons+2);
  undeploy_reset_button(g);
}

void level_mouse_motion(Gameboard *g, int x, int y){

  int icon = find_icon(x,y);

  if(icon != level_lit){
    invalidate_icon(g, level_icons+level_lit);
    level_lit = icon;
    invalidate_icon(g, level_icons+level_lit);
  }
}

void level_mouse_press(Gameboard *g, int x, int y){
  int i;

  level_mouse_motion(g, x, y);

  if(level_lit == 1){
    if(levelstate_prev()){
      if(level_icons[4].icon)cairo_surface_destroy(level_icons[4].icon);
      for(i=4;i>=1;i--){
	level_icons[i].num = level_icons[i-1].num;
	level_icons[i].icon = level_icons[i-1].icon;
      }
      level_icons[0].icon=0;
      onelevel_init(g,-2,level_icons);
      
      if(center_x==0)expose_full(g); // only needed to 'undraw' the text

      center_x = level_icons[1].x - level_icons[2].x;
    }
  }
  
  if(level_lit == 3){
    if(levelstate_next()){
      
      if(level_icons[0].icon)cairo_surface_destroy(level_icons[0].icon);
      for(i=0;i<4;i++){
	level_icons[i].num = level_icons[i+1].num;
	level_icons[i].icon = level_icons[i+1].icon;
      }
      level_icons[4].icon=0;
      onelevel_init(g,2,level_icons+4);
      
      if(center_x==0)expose_full(g); // only needed to 'undraw' the text

      center_x = level_icons[3].x - level_icons[2].x;

    }
  }

  if(center_x){
    if(g->gtk_timer)
      g_source_remove(g->gtk_timer);
    g->gtk_timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_level_frame, (gpointer)g);
  }

}

  
  
