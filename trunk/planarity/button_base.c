#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"
#include "gamestate.h"
#include "buttons.h"
#include "button_base.h"


/************************ manage the buttons for buttonbar and dialogs *********************/

buttonstate states[NUMBUTTONS];

int buttons_ready=0;
static int width=0;
static int height=0;
static int allclear=1; // actually just a shirt-circuit
static buttonstate *grabbed=0;

/* determine the x/y/w/h box around the rollover text */
static GdkRectangle rollover_box(buttonstate *b){
  GdkRectangle r;

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
    GdkRectangle r=rollover_box(b);

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
  GdkRectangle r = rollover_box(b);
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
static void expand_rectangle_rollover(buttonstate *b,GdkRectangle *r){
  GdkRectangle rr = rollover_box(b);
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

/* match x/y to a button if any */
static buttonstate *find_button(int x,int y){
  int i;
  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *b=states+i;
    if(b->position)
      if( (b->x-x)*(b->x-x) + (b->y-y)*(b->y-y) <= BUTTON_RADIUS*BUTTON_RADIUS+4)
	if(b->x != b->x_inactive)
	  return b;
  }
  return 0;
}

/* set a button to pressed or lit */
static void button_set_state(Gameboard *g, buttonstate *b, int rollover, int press){
  int i;
  
  if(b->rollover == rollover && b->press == press)return;

  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *bb=states+i;
    if(bb->position){
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
  allclear=0;
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

/* do the animation math for one button for one frame */
static int animate_one(buttonstate *b, GdkRectangle *r){
  int ret=0;

  if(b->target_x_active != b->x_active){
    ret=1;
    if(b->target_x_active > b->x_active){
      b->x_active+=BUTTON_DELTA;
      if(b->x_active>b->target_x_active)
	b->x_active=b->target_x_active;
    }else{
      b->x_active-=BUTTON_DELTA;
      if(b->x_active<b->target_x_active)
	b->x_active=b->target_x_active;
    }
  }

  if(b->target_x_inactive != b->x_inactive){
    ret=1;
    if(b->target_x_inactive > b->x_inactive){
      b->x_inactive+=BUTTON_DELTA;
      if(b->x_inactive>b->target_x_inactive)
	b->x_inactive=b->target_x_inactive;
    }else{
      b->x_inactive-=BUTTON_DELTA;
      if(b->x_inactive<b->target_x_inactive)
	b->x_inactive=b->target_x_inactive;
    }
  }

  if(b->target_x != b->x){
    ret=1;
    expand_rectangle_button(b,r);
    if(b->rollover)
      expand_rectangle_rollover(b,r);

    if(b->target_x > b->x){
      b->x+=BUTTON_DELTA;
      if(b->x>b->target_x)b->x=b->target_x;
    }else{
      b->x-=BUTTON_DELTA;
      if(b->x<b->target_x)b->x=b->target_x;
    }
    expand_rectangle_button(b,r);
    if(b->rollover)
      expand_rectangle_rollover(b,r);
  }

  if(b->alphad != b->x_active - b->x){
    double alpha=0;
    if(b->x_inactive>b->x_active){
      if(b->x<=b->x_active)
	alpha=1.;
      else if (b->x>=b->x_inactive)
	alpha=0.;
      else
	alpha = (double)(b->x_inactive-b->x)/(b->x_inactive-b->x_active);
    }else if (b->x_inactive<b->x_active){
      if(b->x>=b->x_active)
	alpha=1.;
      else if (b->x<=b->x_inactive)
	alpha=0.;
      else
	alpha = (double)(b->x_inactive-b->x)/(b->x_inactive-b->x_active);
    }
    if(alpha != b->alpha){
      ret=1;
      expand_rectangle_button(b,r);
      b->alpha=alpha;
    }
    b->alphad = b->x_active - b->x;
  }
  return ret;
}

/******************** toplevel abstraction calls *********************/

/* initialize the persistent caches; called once when gameboard is
   first created */
void init_buttons(Gameboard *g){
  memset(states,0,sizeof(states));

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
  width = get_board_width();
  height = get_board_height();
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
  int ret=0,i,pos;
  
  if(!g->first_expose)return 1;
  
  /* avoid the normal expose event mechanism
     during the button animations.  This direct method is much faster as
     it won't accidentally combine exposes over long stretches of
     alpha-blended surfaces. */
  
  for(pos=1;pos<=3;pos++){
    memset(&expose,0,sizeof(expose));
    for(i=0;i<NUMBUTTONS;i++)
      if(states[i].position == pos)
	ret|=animate_one(states+i,&expose);
    if(expose.width && expose.height)
      run_immediate_expose(g,
			   expose.x,
			   expose.y,
			   expose.width,
			   expose.height);
  }
  return ret;
}

/* the normal expose method for all buttons; called from the master
   widget's expose */
void expose_buttons(Gameboard *g,cairo_t *c, int x,int y,int w,int h){
  int i;
  
  for(i=0;i<NUMBUTTONS;i++){

    buttonstate *b=states+i;

    if(b->position){
      GdkRectangle r = rollover_box(b);
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
      
      if(b->rollover || b->press)
	if(x < r.x+r.width &&
	   y < r.y+r.height &&
	   x+w > r.x &&
	   y+h > r.y)
	  
	  stroke_rollover(g,b,c);
    }
  }
}

/* resize the button bar; called from master resize in gameboard */
void resize_buttons(int w,int h){
  int i;
  int dx=w/2-width/2;
  int dy=h/2-height/2;

  for(i=0;i<NUMBUTTONS;i++){
    if(states[i].position == 2){
      states[i].x+=dx;
      states[i].target_x+=dx;
      states[i].x_active+=dx;
      states[i].target_x_active+=dx;
      states[i].x_inactive+=dx;
      states[i].target_x_inactive+=dx;
      states[i].y+=dy;
    }
  }

  dx=w-width;
  dy=h-height;

  for(i=0;i<NUMBUTTONS;i++){
    if(states[i].position == 1){
      states[i].y+=dy;
    }
  }

  for(i=0;i<NUMBUTTONS;i++){
    if(states[i].position == 3){
      states[i].x+=dx;
      states[i].target_x+=dx;
      states[i].x_active+=dx;
      states[i].target_x_active+=dx;
      states[i].x_inactive+=dx;
      states[i].target_x_inactive+=dx;
      states[i].y+=dy;
    }
  }

  width=w;
  height=h;
}

/* clear all buttons to unpressed/unlit */
void button_clear_state(Gameboard *g){
  int i;
  if(!allclear){
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
  allclear=1;
  grabbed=0;
}

/* handles mouse motion events; called out of gameboard's master
   motion handler; returns nonzero if it used focus */
int button_motion_event(Gameboard *g, GdkEventMotion *event, int focusable){
  buttonstate *b = find_button((int)event->x,(int)event->y);

  if(buttons_ready){
    if(grabbed){
      if(grabbed==b)
	button_set_state(g,grabbed,1,1);
      else
	button_set_state(g,grabbed,0,0);
      return 1;
    }
    
    if(focusable && !grabbed){
      
      if(b){
	button_set_state(g,b,1,0);
	return 1;
      }
    }
    
    button_clear_state(g);
  }
  return 0;
}

/* handles mouse button press events; called out of gameboard's master
   mouse handler.  returns nonzero if grabbing focus */
int button_button_press(Gameboard *g, GdkEventButton *event, int focusable){

  if(buttons_ready){
    if(focusable){
      
      buttonstate *b = find_button((int)event->x,(int)event->y);
      if(b){
	button_set_state(g,b,1,1);
      grabbed = b;
      return 1;
      }
    }
    
    button_clear_state(g);
  }
  return 0;  
}

/* handles mouse button press events; called out of gameboard's master
   mouse handler. returns nonzero if grabbing focus */
int button_button_release(Gameboard *g, GdkEventButton *event, int focusable){
  if(focusable && buttons_ready){
    
    buttonstate *b = find_button((int)event->x,(int)event->y);
    if(b && grabbed==b){
      button_set_state(g,b,1,0);
      
      if(b->callback)
	b->callback(g);
      
      
    }
  }
  
  grabbed=0;
  return 0;  
}
