#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "gameboard.h"
#include "gamestate.h"
#include "buttons.h"
#include "box.h"


/************************ the lower button bar *********************/

#define NUMBUTTONS 10

typedef struct {
  int target_x;
  int target_x_inactive;
  int target_x_active;

  int x;
  int x_inactive;
  int x_active;
  int y;

  cairo_surface_t *idle;
  cairo_surface_t *lit;

  char *rollovertext;
  cairo_text_extents_t ex;

  int alphad;
  double alpha;

  int rollover;
  int press;

  void (*callback)();
} buttonstate;

static buttonstate states[NUMBUTTONS];

static int width=0;
static int height=0;
static int deployed=0;
static int sweeper=0;
static gint timer = 0;
static int checkbutton_deployed=0;
static int allclear=1; // actually just a shirt-circuit
static buttonstate *grabbed=0;
static void (*undeploy_callback)(Gameboard *g);

/*********************** static housekeeping *****************/

/* initialize the rather weird little animation engine */
static void buttons_initial_placement(int w, int h){
  
  int count=BUTTON_BORDER;
  int i;

  width=w;
  height=h;
  
  for(i=0;i<BUTTON_LEFT;i++){
    buttonstate *b=states+i;
    b->x=0;
    b->target_x=count;
    b->x_active=b->target_x_active=count;
    b->x_inactive=b->target_x_inactive=count-BUTTON_EXPOSE;
    b->y = h - BUTTON_Y_FROM_BOTTOM;
    count += BUTTON_SPACING;
  }
  count -= BUTTON_SPACING;

  // assumes we will always have at least as many buttons left as right
  sweeper=count;
  
  count=width-BUTTON_BORDER;
  for(i=0;i<BUTTON_RIGHT;i++){
    buttonstate *b=states+NUMBUTTONS-i-1;
    b->x=width;
    b->target_x=count;
    b->x_active=b->target_x_active=count;
    b->x_inactive=b->target_x_inactive=count+BUTTON_EXPOSE;
    b->y = h - BUTTON_Y_FROM_BOTTOM;
    if(i>0) // special-case the checkbutton
      count -= BUTTON_SPACING;
  }
  
  // special-case the checkbutton
  states[9].target_x_inactive=states[9].target_x_active+BUTTON_SPACING;
  states[9].x_inactive=states[9].target_x_inactive;
  states[9].target_x=states[9].target_x_inactive;

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

/* cache the text extents of a rollover */
static void rollover_extents(Gameboard *g, buttonstate *b){
  
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

/* do the animation math for one button for one frame */
static int animate_one(buttonstate *b, GdkRectangle *r){
  int ret=0;
  int w2=width/2;

  if(b->target_x>w2 && width-sweeper>b->target_x)
    b->target_x=width-sweeper;
  if(b->target_x<w2 && sweeper<b->target_x)
    b->target_x=sweeper;

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

/* perform a single frame of animation for all buttons/rollovers */
static gboolean animate_buttons(gpointer ptr){
  Gameboard *g=(Gameboard *)ptr;
  GdkRectangle expose;
  int ret=0,i;

  if(!g->first_expose)return 1;
  
  memset(&expose,0,sizeof(expose));
  for(i=0;i<BUTTON_LEFT;i++)
    ret|=animate_one(states+i,&expose);
  
  /* avoid the normal expose event mechanism
     during the button animations.  This direct method is much faster as
     it won't accidentally combine exposes over long stretches of
     alpha-blended surfaces. */
  run_immediate_expose(g,
		       expose.x,
  	       expose.y,
  	       expose.width,
  	       expose.height);
  


  memset(&expose,0,sizeof(expose));
  for(i=BUTTON_LEFT;i<NUMBUTTONS;i++)
    ret|=animate_one(states+i,&expose);
    
  run_immediate_expose(g,
		       expose.x,
		       expose.y,
		       expose.width,
		       expose.height);

  if(!ret && timer!=0){
    g_source_remove(timer);
    timer=0;
  }
  return ret;
}

/* helper that wraps animate-buttons for removing the buttons at the
   end of a level */
static gboolean deanimate_buttons(gpointer ptr){
  Gameboard *g=(Gameboard *)ptr;
  int ret=0;
  sweeper-=BUTTON_DELTA;
  if(sweeper< -(BUTTON_RADIUS +1)){
    sweeper= -(BUTTON_RADIUS +1);
  }else
    ret=1;

  ret|=animate_buttons(ptr);

  if(!ret)
    // undeploy finished... call the undeploy callback
    undeploy_callback(g);

  return ret;
}

/* the normal expose method for all buttons; called from the master
   widget's expose */
void expose_buttons(Gameboard *g,cairo_t *c, int x,int y,int w,int h){
  int i;
  
  for(i=0;i<NUMBUTTONS;i++){

    buttonstate *b=states+i;
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

/* match x/y to a button if any */
static buttonstate *find_button(int x,int y){
  int i;
  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *b=states+i;
    if( (b->x-x)*(b->x-x) + (b->y-y)*(b->y-y) <= BUTTON_RADIUS*BUTTON_RADIUS+4)
      if(b->x != b->x_inactive)
	return b;
  }
  return 0;
}

/* clear all buttons to unpressed/unlit */
static void button_clear_state(Gameboard *g){
  int i;
  if(!allclear){
    for(i=0;i<NUMBUTTONS;i++){
      buttonstate *bb=states+i;
      if(bb->rollover)
	invalidate_rollover(g,bb);
      if(bb->press)
	invalidate_button(g,bb);
      
      bb->rollover=0;
      bb->press=0;
    }
  }
  allclear=1;
  grabbed=0;
}

/* set a button to pressed or lit */
static void button_set_state(Gameboard *g, buttonstate *b, int rollover, int press){
  int i;
  
  if(b->rollover == rollover && b->press == press)return;

  for(i=0;i<NUMBUTTONS;i++){
    buttonstate *bb=states+i;
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
  allclear=0;
}

/******************** toplevel abstraction calls *********************/

/* initialize the persistent caches; called once when gameboard is
   first created */
void init_buttons(Gameboard *g){
  int i;
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

  states[0].rollovertext="exit gPlanarity";
  states[1].rollovertext="back to menu";
  states[2].rollovertext="reset board";
  states[3].rollovertext="pause";
  states[4].rollovertext="help / about";
  states[5].rollovertext="expand";
  states[6].rollovertext="shrink";
  states[7].rollovertext="hide/show lines";
  states[8].rollovertext="mark intersections";
  states[9].rollovertext="click when finished!";

  states[0].callback = quit;
  states[2].callback = reset_board;
  states[5].callback = expand;
  states[6].callback = shrink;
  states[7].callback = hide_show_lines;
  states[8].callback = mark_intersections;
  states[9].callback = finish_board;

  for(i=0;i<NUMBUTTONS;i++)
    rollover_extents(g,states+i);
}

/* effects animated 'rollout' of buttons when level begins */
void deploy_buttons(Gameboard *g){
  if(!deployed ){
    buttons_initial_placement(get_board_width(),get_board_height());
    timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_buttons, (gpointer)g);
    deployed=1;
    checkbutton_deployed=0;
  }

}

/* effects animated rollout of 'check' button */
void deploy_check(Gameboard *g){
  if(deployed && !checkbutton_deployed){
    int i;
    for(i=BUTTON_LEFT;i<NUMBUTTONS-1;i++){
      states[i].target_x-=BUTTON_SPACING;
      states[i].target_x_active-=BUTTON_SPACING;
      states[i].target_x_inactive-=BUTTON_SPACING;
    }
    states[9].target_x=states[9].target_x_active;
    if(timer!=0)
      g_source_remove(timer);
    timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_buttons, (gpointer)g);
    checkbutton_deployed=1;
  }
}

/* effects animated rollback of 'check' button */
void undeploy_check(Gameboard *g){
  if(checkbutton_deployed){
    int i;
    for(i=BUTTON_LEFT;i<NUMBUTTONS-1;i++){
      states[i].target_x+=BUTTON_SPACING;
      states[i].target_x_active+=BUTTON_SPACING;
      states[i].target_x_inactive+=BUTTON_SPACING;
    }
    states[9].target_x=states[9].target_x_inactive;
    if(timer!=0)
      g_source_remove(timer);
    timer = g_timeout_add(BUTTON_ANIM_INTERVAL, animate_buttons, (gpointer)g);
    checkbutton_deployed=0;
  }
}

/* effects animated rollback of buttons at end of level */
void undeploy_buttons(Gameboard *g, void (*callback)(Gameboard *g)){
  if(timer!=0)
    g_source_remove(timer);

  button_clear_state(g);
  grabbed=0;
  deployed=0;
  undeploy_callback=callback;
  checkbutton_deployed=0;

  g_timeout_add(BUTTON_ANIM_INTERVAL, deanimate_buttons, (gpointer)g);


}

/* resize the button bar; called from master resize in gameboard */
void resize_buttons(int w,int h){
  int i;
  int dx=w-width;
  int dy=h-height;

  for(i=BUTTON_LEFT;i<NUMBUTTONS;i++){
    states[i].x+=dx;
    states[i].target_x+=dx;
    states[i].x_active+=dx;
    states[i].target_x_active+=dx;
    states[i].x_inactive+=dx;
    states[i].target_x_inactive+=dx;
    states[i].y+=dy;
  }
  width=w;
}


/* handles mouse motion events; called out of gameboard's master
   motion handler; returns nonzero if it used focus */
int button_motion_event(Gameboard *g, GdkEventMotion *event, int focusable){
  buttonstate *b = find_button((int)event->x,(int)event->y);

  if(deployed){
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

  if(deployed){
    if(focusable && deployed){
      
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
  if(focusable && deployed){
    
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
