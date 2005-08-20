#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "graph.h"
#include "gameboard.h"
#include "gamestate.h"
#include "levelstate.h"
#include "button_base.h"
#include "buttonbar.h"
#include "box.h"

static GtkWidgetClass *parent_class = NULL;

/* vertex area and draw operations ***********************************/

// invalidate the box around a single vertex
static void invalidate_region_vertex_off(GtkWidget *widget, 
					 vertex *v, int dx, int dy){
  if(v){
    GdkRectangle r;
    r.x = v->x - V_RADIUS - V_LINE + dx;
    r.y = v->y - V_RADIUS - V_LINE + dy;
    r.width =  (V_RADIUS + V_LINE)*2;
    r.height = (V_RADIUS + V_LINE)*2;
    
    gdk_window_invalidate_rect (widget->window, &r, FALSE);
  }
}

void invalidate_region_vertex(Gameboard *g, vertex *v){
  invalidate_region_vertex_off(&g->w,v,0,0);
}

// invalidate the box around a single line
static void invalidate_region_edges(GtkWidget *widget, vertex *v){
  GdkRectangle r;
  
  if(v){
    edge_list *el=v->edges;
    while (el){
      edge *e=el->edge;
      r.x = min(e->A->x,e->B->x) - E_LINE;
      r.y = min(e->A->y,e->B->y) - E_LINE;
      r.width  = labs(e->B->x - e->A->x) + 1 + E_LINE*2;
      r.height = labs(e->B->y - e->A->y) + 1 + E_LINE*2;
      gdk_window_invalidate_rect (widget->window, &r, FALSE);
      el=el->next;
    }
  }
}

// invalidate the vertex and attached verticies
static void invalidate_region_attached(GtkWidget *widget, vertex *v){
  if(v){
    edge_list *el=v->edges;
    while (el){
      edge *e=el->edge;
      if(e->A != v)invalidate_region_vertex(GAMEBOARD(widget),e->A);
      if(e->B != v)invalidate_region_vertex(GAMEBOARD(widget),e->B);
      el=el->next;
    }
    invalidate_region_vertex(GAMEBOARD(widget),v);
  }
}

// invalidate the selection box plus enough area to catch any verticies
static void invalidate_region_selection(GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  GdkRectangle r;
  r.x = g->selectionx - (V_RADIUS + V_LINE)*2;
  r.y = g->selectiony - (V_RADIUS + V_LINE)*2;
  r.width =  g->selectionw + (V_RADIUS + V_LINE)*4;
  r.height = g->selectionh + (V_RADIUS + V_LINE)*4;
  
  gdk_window_invalidate_rect (widget->window, &r, FALSE);
}

// invalidate the selection box plus enough area to catch any verticies
static void invalidate_region_verticies_selection(GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  vertex *v=g->g->verticies;
  while(v){
    if(v->selected)
      invalidate_region_vertex_off(widget,v,g->dragx,g->dragy);
    v=v->next;
  }
}

static cairo_surface_t *cache_vertex(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_IDLE_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}

static cairo_surface_t *cache_vertex_sel(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS*.5,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_IDLE_COLOR);
  cairo_fill(c);

  cairo_destroy(c);
  return ret;
}

static cairo_surface_t *cache_vertex_grabbed(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS*.5,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_ADJ_COLOR);
  cairo_fill(c);

  cairo_destroy(c);
  return ret;
}

static cairo_surface_t *cache_vertex_lit(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_LIT_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}

static cairo_surface_t *cache_vertex_attached(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgb(c,V_FILL_ADJ_COLOR);
  cairo_fill_preserve(c);
  cairo_set_source_rgb(c,V_LINE_COLOR);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}

static cairo_surface_t *cache_vertex_ghost(Gameboard *g){
  cairo_surface_t *ret=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  (V_RADIUS+V_LINE)*2,
				  (V_RADIUS+V_LINE)*2);
  cairo_t *c = cairo_create(ret);
  
  cairo_set_line_width(c,V_LINE);
  cairo_arc(c,V_RADIUS+V_LINE,V_RADIUS+V_LINE,V_RADIUS,0,2*M_PI);
  cairo_set_source_rgba(c,V_LINE_COLOR,.2);
  cairo_fill_preserve(c);
  cairo_set_source_rgba(c,V_LINE_COLOR,.4);
  cairo_stroke(c);

  cairo_destroy(c);
  return ret;
}


static void draw_vertex(cairo_t *c,vertex *v,cairo_surface_t *s){
  cairo_set_source_surface(c,
			   s,
			   v->x-V_LINE-V_RADIUS,
			   v->y-V_LINE-V_RADIUS);
  cairo_paint(c);
}      

static void setup_background_edge(cairo_t *c){
  cairo_set_line_width(c,E_LINE);
  cairo_set_source_rgba(c,E_LINE_B_COLOR);
}

static void setup_foreground_edge(cairo_t *c){
  cairo_set_line_width(c,E_LINE);
  cairo_set_source_rgba(c,E_LINE_F_COLOR);
}

static void draw_edge(cairo_t *c,edge *e){
  cairo_move_to(c,e->A->x,e->A->y);
  cairo_line_to(c,e->B->x,e->B->y);
}

static void finish_edge(cairo_t *c){
  cairo_stroke(c);
}

static void draw_selection_rectangle(Gameboard *g,cairo_t *c){
  cairo_set_source_rgba(c,SELECTBOX_COLOR);
  cairo_rectangle(c,g->selectionx,
		  g->selectiony,
		  g->selectionw,
		  g->selectionh);
  cairo_fill(c);
}

static void midground_draw(Gameboard *g,cairo_t *c,int x,int y,int w,int h){
  /* Edges attached to the grabbed vertex are drawn here; they're the
     inactive edges. */
  if(g->grabbed_vertex && !g->hide_lines){
    edge_list *el=g->grabbed_vertex->edges;
    setup_foreground_edge(c);
    while(el){
      edge *e=el->edge;
      /* no need to check rectangle; if they're to be drawn, they'll
	 always be in the rect */
      draw_edge(c,e);
      el=el->next;
    }
    finish_edge(c);
  }
  
  /* verticies are drawn in the foreground */
  {
    vertex *v = g->g->verticies;
    int clipx = x-V_RADIUS;
    int clipw = x+w+V_RADIUS;
    int clipy = y-V_RADIUS;
    int cliph = y+h+V_RADIUS;
    
    while(v){

      /* is the vertex in the expose rectangle? */
      if(v->x>=clipx && v->x<=clipw &&
	 v->y>=clipy && v->y<=cliph){
	
	if(v == g->grabbed_vertex && !g->group_drag) {
	  draw_vertex(c,v,g->vertex_grabbed);      
	} else if( v->selected ){
	  draw_vertex(c,v,g->vertex_sel);
	} else if ( v == g->lit_vertex){
	  draw_vertex(c,v,g->vertex_lit);
	} else if (v->attached_to_grabbed && !g->group_drag){
	  draw_vertex(c,v,g->vertex_attached);
	}else
	  draw_vertex(c,v,g->vertex);
      }
      
      v=v->next;
    }
  }
}

static void background_draw(Gameboard *g,cairo_t *c){
  int width=get_board_width();
  int height=get_board_height();
  edge *e=g->g->edges;

  cairo_set_source_rgb(c,1,1,1);
  cairo_paint(c);
    
  if(!g->hide_lines){
    setup_background_edge(c);
    while(e){
      if(e->active){
	draw_edge(c,e);
      }
      e=e->next;
    }
    finish_edge(c);
  }

  // if there's a a group drag in progress, midground is drawn here
  if(g->group_drag)
    midground_draw(g,c,0,0,width,height);

}

static void background_addv(Gameboard *g,cairo_t *c, vertex *v){
  edge_list *el=v->edges;

  if(!g->hide_lines){
    setup_background_edge(c);
    while(el){
      edge *e=el->edge;
      
      if(e->active)
	draw_edge(c,e);
      
      el=el->next;
    }
    finish_edge(c);
  }
}

static void foreground_draw(Gameboard *g,cairo_t *c,
			    int x,int y,int width,int height){
  /* if a group drag is in progress, draw the group ghosted in the foreground */

  if(g->group_drag){ 
    vertex *v = g->g->verticies;
    while(v){
      
      if( v->selected ){
	vertex tv;
	tv.x=v->x+g->dragx;
	tv.y=v->y+g->dragy;
	draw_vertex(c,&tv,g->vertex_ghost);
      }

      v=v->next;
    }
  }else
    midground_draw(g,c,x,y,width,height);    

  if(g->selection_grab)
    draw_selection_rectangle(g,c);
}

static void update_background(GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  GdkRectangle r;
  cairo_t *c = cairo_create(g->background);

  g->delayed_background=0;

  // render the far background plane
  background_draw(g,c);
  cairo_destroy(c);
  
  r.x=0;
  r.y=0;
  r.width=widget->allocation.width;
  r.height=widget->allocation.height;

  gdk_window_invalidate_rect (widget->window, &r, FALSE);
    
}

void update_full(Gameboard *g){
  update_background(&g->w);
}

// also updates score
static void update_background_addv(GtkWidget *widget, vertex *v){
  Gameboard *g = GAMEBOARD (widget);
  cairo_t *c = cairo_create(g->background);

  g->delayed_background=0;

  // render the far background plane
  background_addv(g,c,v);
  cairo_destroy(c);

  invalidate_region_attached(widget,v);    
}

static void update_background_delayed(GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  g->delayed_background=1;
}

static void check_lit(GtkWidget *widget,int x, int y){
  Gameboard *g = GAMEBOARD (widget);
  vertex *v = find_vertex(g->g,x,y);
  if(v!=g->lit_vertex){
    invalidate_region_vertex(g,v);
    invalidate_region_vertex(g,g->lit_vertex);
    g->lit_vertex = v;
  }
}

static void score_update(Gameboard *g){
  /* Score, level, intersections */
  char level_string[160];
  char score_string[160];
  char int_string[160];
  char obj_string[160];
  cairo_text_extents_t extentsL;
  cairo_text_extents_t extentsS;
  cairo_text_extents_t extentsO;
  cairo_text_extents_t extentsI;
  cairo_matrix_t m;

  cairo_t *c = cairo_create(g->forescore);

  if(g->g->active_intersections <= get_objective()){
    deploy_check(g);
  }else{
    undeploy_check(g);
  }


  // clear the pane
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);

  topbox(c,get_board_width(),SCOREHEIGHT);

  cairo_select_font_face (c, "Arial",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);

  cairo_matrix_init_scale (&m, 12.,15.);
  cairo_set_font_matrix (c,&m);
  cairo_set_source_rgba (c, TEXT_COLOR);

  snprintf(level_string,160,"Level %d: \"%s\"",get_level_num()+1,get_level_name());
  snprintf(score_string,160,"Score: %d",get_score());
  snprintf(int_string,160,"Intersections: %d",g->g->active_intersections);
  snprintf(obj_string,160,"Objective: %s",get_objective_string());

  cairo_text_extents (c, level_string, &extentsL);
  cairo_text_extents (c, obj_string, &extentsO);
  cairo_text_extents (c, int_string, &extentsI);
  cairo_text_extents (c, score_string, &extentsS);

  /*
  text_h = extentsL.height;
  text_h = max(text_h,extentsO.height);
  text_h = max(text_h,extentsI.height);
  text_h = max(text_h,extentsS.height);
  */

  int ty1 = 23;
  int ty2 = 38;

  cairo_move_to (c, 15, ty1);
  cairo_show_text (c, int_string);  
  cairo_move_to (c, 15, ty2);
  cairo_show_text (c, score_string);  

  cairo_move_to (c, get_board_width()-extentsL.width-15, ty1);
  cairo_show_text (c, level_string);  
  cairo_move_to (c, get_board_width()-extentsO.width-15, ty2);
  cairo_show_text (c, obj_string);  

  cairo_destroy(c);

  // slightly lazy
  {
    GdkRectangle r;
    r.x=0;
    r.y=0;
    r.width=get_board_width();
    r.height = SCOREHEIGHT;
    gdk_window_invalidate_rect (g->w.window, &r, FALSE);
  }
}

#define CW 4
static void cache_curtain(Gameboard *g){
  int x,y;
  cairo_t *c;
  g->curtains=
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  CW,CW);
  
  c = cairo_create(g->curtains);
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);
      
  cairo_set_line_width(c,1);
  cairo_set_source_rgba (c, 0,0,0,.5);
  
  for(y=0;y<CW;y++){
    for(x=y&1;x<CW;x+=2){
      cairo_move_to(c,x+.5,y);
      cairo_rel_line_to(c,0,1);
    }
  }
  cairo_stroke(c);
  cairo_destroy(c);
  
  g->curtainp=cairo_pattern_create_for_surface (g->curtains);
  cairo_pattern_set_extend (g->curtainp, CAIRO_EXTEND_REPEAT);

}

static gint mouse_motion(GtkWidget        *widget,
			 GdkEventMotion   *event){
  Gameboard *g = GAMEBOARD (widget);

  if(g->button_grabbed || paused_p()){
    g->button_grabbed = 
      button_motion_event(g,event,
			  (!g->lit_vertex && !g->grabbed_vertex && !g->selection_grab));

    if(paused_p())return TRUE;
  }

  if(!g->button_grabbed){
    if(g->grabbed_vertex){
      int x = (int)event->x;
      int y = (int)event->y;
      g->dragx = x-g->grabx;
      g->dragy = y-g->graby;
      
      invalidate_region_vertex(g,g->grabbed_vertex);
      invalidate_region_edges(widget,g->grabbed_vertex);
      move_vertex(g->g,g->grabbed_vertex,x+g->graboffx,y+g->graboffy);
      invalidate_region_vertex(g,g->grabbed_vertex);
      invalidate_region_edges(widget,g->grabbed_vertex);
    }else if (g->selection_grab){
      invalidate_region_selection(widget);
      
      g->selectionx = min(g->grabx, event->x);
      g->selectionw = labs(g->grabx - event->x);
      g->selectiony = min(g->graby, event->y);
      g->selectionh = labs(g->graby - event->y);
      select_verticies(g->g,
		       g->selectionx,g->selectiony,
		       g->selectionx+g->selectionw-1,
		       g->selectiony+g->selectionh-1);
      
      invalidate_region_selection(widget);
    }else if(g->group_drag){
      invalidate_region_verticies_selection(widget);
      g->dragx = event->x - g->grabx;
      g->dragy = event->y - g->graby;
      
      // if this puts any of the dragged offscreen adjust the drag back
      // onscreen.  It will avoid confusing/concerning users
      {
	vertex *v=g->g->verticies;
	int w=get_board_width();
	int h=get_board_height();
	
	while(v){
	  if(v->selected){
	    if(v->x + g->dragx >= w)
	      g->dragx=w - v->x -1;
	    if(v->x + g->dragx < 0 )
	      g->dragx= -v->x;
	    if(v->y + g->dragy >= h)
	      g->dragy=h - v->y -1;
	    if(v->y + g->dragy < 0 )
	    g->dragy= -v->y;
	  }
	  v=v->next;
	}
      }
      
      invalidate_region_verticies_selection(widget);
    }else{
      check_lit(widget, (int)event->x,(int)event->y);
      
      button_motion_event(g,event,
			  (!g->lit_vertex && !g->grabbed_vertex && !g->selection_grab));

    }
  }
  return TRUE;
}

static gboolean button_press (GtkWidget        *widget,
			      GdkEventButton   *event){
  Gameboard *g = GAMEBOARD (widget);

  vertex *v = find_vertex(g->g,(int)event->x,(int)event->y);
  g->button_grabbed=0;

  if(paused_p()){
    // only buttongrabs
    if(button_button_press(g,event,1))
      g->button_grabbed=1;
    return TRUE;
  }
     
  if(!g->group_drag && event->state&GDK_SHIFT_MASK){
    if(v){
      if(v->selected){
	v->selected=0;
	g->selection_active--;
      }else{
	v->selected=1;
	g->selection_active++;
      }
      invalidate_region_vertex(g,g->lit_vertex);
    }else{
      // addending group drag
      g->selection_grab=1;
      g->grabx = (int)event->x;
      g->graby = (int)event->y;
      g->selectionx = g->grabx;
      g->selectionw = 0;
      g->selectiony = g->graby;
      g->selectionh = 0;
    }
  }else{

    if(g->selection_active){
      vertex *v = find_vertex(g->g,(int)event->x,(int)event->y);
      if(v && v->selected){
	// group drag
	g->group_drag=1;
	g->grabx = (int)event->x;
	g->graby = (int)event->y;
	g->dragx = 0;
	g->dragy = 0;
	// put the verticies into the background for faster update
	update_background(widget); 
      }else{

	if(button_button_press(g,event,1)){
	  g->button_grabbed=1;
	}else{
	  deselect_verticies(g->g);
	  update_background(widget); 
	  g->selection_active=0;
	  g->group_drag=0;
	  check_lit(widget,event->x,event->y);
	  button_press(widget,event);
	}
      }

    }else if(g->lit_vertex){
      // vertex grab
      g->grabbed_vertex = g->lit_vertex;
      g->grabx = (int)event->x;
      g->graby = (int)event->y;
      g->graboffx = g->grabbed_vertex->x-g->grabx;
      g->graboffy = g->grabbed_vertex->y-g->graby;
      
      grab_vertex(g->g,g->grabbed_vertex);
      invalidate_region_attached(widget,g->grabbed_vertex);
      invalidate_region_edges(widget,g->grabbed_vertex);
      
      update_background_delayed(widget);
    }else{
      
      if(button_button_press(g,event,1)){
	g->button_grabbed=1;
      }else{
	// selection grab;
	g->selection_grab=1;
	g->grabx = (int)event->x;
	g->graby = (int)event->y;
	g->selectionx = g->grabx;
	g->selectionw = 0;
	g->selectiony = g->graby;
	g->selectionh = 0;
      }
    }
  }

  if(!g->button_grabbed)
    hide_intersections(g);
  
  return TRUE;
}

static gboolean button_release (GtkWidget        *widget,
				GdkEventButton   *event){
  Gameboard *g = GAMEBOARD (widget);

  button_button_release(g,event,1);
  if(paused_p())return TRUE;

  if(g->grabbed_vertex){
    ungrab_vertex(g->g,g->grabbed_vertex);
    update_background_addv(widget,g->grabbed_vertex);
    score_update(g);
    g->grabbed_vertex = 0;
  }

  if(g->selection_grab){
    invalidate_region_selection(widget);
    g->selection_grab=0;
    // are there selected verticies? Avoid accidentally misgrabbing one.
    if(num_selected_verticies(g->g)<=1){
      g->selection_active=0;
      deselect_verticies(g->g); // could have grabbed just one
    }else{
      commit_volatile_selection();
      g->selection_active=num_selected_verticies(g->g);
    }
  }

  if(g->group_drag){
    move_selected_verticies(g->g,g->dragx,g->dragy);
    update_background(widget); // cheating
    score_update(g);
    g->group_drag=0;
  }

  mouse_motion(widget,(GdkEventMotion *)event); // the cast is safe in
						// this case

  return TRUE;
}

static void size_request (GtkWidget *widget,GtkRequisition *requisition){

  requisition->width = get_orig_width();
  requisition->height = get_orig_height();

}

static void gameboard_init (Gameboard *g){

  // instance initialization


}

static void gameboard_destroy (GtkObject *object){
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void draw_intersection(cairo_t *c,double x, double y){
  cairo_move_to(c,x-INTERSECTION_RADIUS,y);
  cairo_rel_line_to(c,INTERSECTION_RADIUS,-INTERSECTION_RADIUS);
  cairo_rel_line_to(c,INTERSECTION_RADIUS,INTERSECTION_RADIUS);
  cairo_rel_line_to(c,-INTERSECTION_RADIUS,INTERSECTION_RADIUS);
  cairo_close_path(c);
}
static void expose_intersections(Gameboard *g,cairo_t *c,
				 int x,int y,int w,int h){

  if(g->show_intersections){
    cairo_set_source_rgba (c, INTERSECTION_COLOR);
    cairo_set_line_width(c, INTERSECTION_LINE_WIDTH);

    double xx=x-(INTERSECTION_LINE_WIDTH*.5 + INTERSECTION_RADIUS);
    double x2=w+x+(INTERSECTION_LINE_WIDTH + INTERSECTION_RADIUS*2);
    double yy=y-(INTERSECTION_LINE_WIDTH*.5 + INTERSECTION_RADIUS);
    double y2=h+y+(INTERSECTION_LINE_WIDTH + INTERSECTION_RADIUS*2);

    // walk the intersection list of all edges; draw if paired is a higher pointer
    edge *e=g->g->edges;
    while(e){
      intersection *i = e->i.next;
      while(i){
	if(i->paired > i){
	  double ix=i->x, iy=i->y;
    
	  if(ix >= xx && ix <= x2 && iy >= yy && iy <= y2)
	    draw_intersection(c,ix,iy);
	}
	i=i->next;
      }
      e=e->next;
    }
    cairo_stroke(c);
  }
}

void pop_background(Gameboard *g){
  if(g->pushed_background){
    g->pushed_background=0;
    g->pushed_curtain=0;
    update_full(g);
  }
}

void push_background(Gameboard *g, void(*redraw_callback)(Gameboard *g)){
  cairo_t *c = cairo_create(g->background);
  int w = g->w.allocation.width;
  int h = g->w.allocation.height;

  if(g->pushed_background)
    pop_background(g);

  g->redraw_callback=redraw_callback;

  foreground_draw(g,c,0,0,w,h);

  // copy in the score and button surfaces
  cairo_set_source_surface(c,g->forescore,0,0);
  cairo_rectangle(c, 0,0,w,
		  min(SCOREHEIGHT,h));
  cairo_fill(c);

  cairo_set_source_surface(c,g->forebutton,0,g->w.allocation.height-SCOREHEIGHT);
  cairo_rectangle(c, 0,0,w,h);
  cairo_fill(c);

  expose_intersections(g,c,0,0,w,h);
  cairo_destroy(c);

  if(redraw_callback)redraw_callback(g);

  g->pushed_background=1;
  {
    GdkRectangle r;
    r.x=0;
    r.y=0;
    r.width=w;
    r.height=h;
    
    gdk_window_invalidate_rect (g->w.window, &r, FALSE);
  }
}

void push_curtain(Gameboard *g,void(*redraw_callback)(Gameboard *g)){
  if(!g->pushed_background)push_background(g,0);
  if(!g->pushed_curtain){ 
    cairo_t *c = cairo_create(g->background);
    int w = g->w.allocation.width;
    int h = g->w.allocation.height;
    g->pushed_curtain=1;
    
    g->redraw_callback=redraw_callback;
    cairo_set_source (c, g->curtainp);
    cairo_paint(c);
    cairo_destroy(c);
    
    if(redraw_callback)redraw_callback(g);
    
    {
      GdkRectangle r;
      r.x=0;
      r.y=0;
      r.width=w;
      r.height=h;
      
      gdk_window_invalidate_rect (g->w.window, &r, FALSE);
    }
  }
}

void run_immediate_expose(Gameboard *g,
			  int x, int y, int w, int h){

  if (w==0 || h==0) return;

  //fprintf(stderr,"(%d) %d/%d %d/%d\n",g->first_expose,x,w,y,h);

  cairo_t *c = cairo_create(g->foreground);

  // copy background to foreground draw buffer
  cairo_set_source_surface(c,g->background,0,0);
  cairo_rectangle(c,x,y,w,h);
  cairo_fill(c);

  if(!g->pushed_background){
    foreground_draw(g,c,x,y,w,h);

    // copy in any of the score or button surfaces?
    if(y<SCOREHEIGHT){
      cairo_set_source_surface(c,g->forescore,0,0);
      cairo_rectangle(c, x,y,w,
		      min(SCOREHEIGHT-y,h));
      cairo_fill(c);
    }
    if(y+h>g->w.allocation.height-SCOREHEIGHT){
      cairo_set_source_surface(c,g->forebutton,0,g->w.allocation.height-SCOREHEIGHT);
      cairo_rectangle(c, x,y,w,h);
      cairo_fill(c);
    }

    expose_intersections(g,c,x,y,w,h);
  }
  
  expose_buttons(g,c,x,y,w,h);

  cairo_destroy(c);
  
  // blit to window
  cairo_set_source_surface(g->wc,g->foreground,0,0);
  cairo_rectangle(g->wc,x,y,w,h);
  cairo_fill(g->wc);

  if(g->delayed_background)update_background(&g->w);
  g->first_expose=1;
  //gdk_window_process_all_updates(); 
}

static gint gameboard_expose (GtkWidget      *widget,
			      GdkEventExpose *event){
  
  Gameboard *g = GAMEBOARD (widget);
  run_immediate_expose(g,event->area.x,event->area.y,
		       event->area.width,event->area.height);
  return FALSE;
}

static void paint_bottom_box (Gameboard *g){
  cairo_t *c = cairo_create(g->forebutton);
  
  cairo_save(c);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba (c, 1,1,1,1);
  cairo_paint(c);
  cairo_restore(c);

  bottombox(c,g->w.allocation.width,SCOREHEIGHT);
  cairo_destroy(c);
}

static void gameboard_realize (GtkWidget *widget){
  Gameboard *g = GAMEBOARD (widget);
  GdkWindowAttr attributes;
  gint      attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = 
    gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK|
    GDK_POINTER_MOTION_MASK|
    GDK_BUTTON_PRESS_MASK  |
    GDK_BUTTON_RELEASE_MASK|
    GDK_KEY_PRESS_MASK |
    GDK_KEY_RELEASE_MASK |
    GDK_STRUCTURE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window,
				   &attributes, attributes_mask);
  gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  gdk_window_set_user_data (widget->window, widget);
  gtk_widget_set_double_buffered (widget, FALSE);

  g->wc = gdk_cairo_create(widget->window);

  g->forescore = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  widget->allocation.width,
				  SCOREHEIGHT);

  g->forebutton = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR_ALPHA,
				  widget->allocation.width,
				  SCOREHEIGHT);

  g->background = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR,
				  widget->allocation.width,
				  widget->allocation.height);
  g->foreground = 
    cairo_surface_create_similar (cairo_get_target (g->wc),
				  CAIRO_CONTENT_COLOR,
				  widget->allocation.width,
				  widget->allocation.height);  

  // cache the vertex images that are drawn in quantity; the blit with
  // alpha is much faster than the render

  g->vertex = cache_vertex(g);
  g->vertex_lit = cache_vertex_lit(g);
  g->vertex_attached = cache_vertex_attached(g);
  g->vertex_grabbed = cache_vertex_grabbed(g);
  g->vertex_sel = cache_vertex_sel(g);
  g->vertex_ghost = cache_vertex_ghost(g);

  update_background(widget); 
  score_update(g);
  paint_bottom_box(g);
  init_buttons(g);
  cache_curtain(g);
}

static void gameboard_size_allocate (GtkWidget     *widget,
				     GtkAllocation *allocation){
  Gameboard *g = GAMEBOARD (widget);
  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget)){
    int oldw=get_board_width();
    int oldh=get_board_height();

    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
			    allocation->width, allocation->height);
    
 
     if (g->forescore)
      cairo_surface_destroy(g->forescore);
    if (g->forebutton)
      cairo_surface_destroy(g->forebutton);
    if (g->background)
      cairo_surface_destroy(g->background);
    if (g->foreground)
      cairo_surface_destroy(g->foreground);
    
    g->background = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR, // don't need alpha
				    widget->allocation.width,
				    widget->allocation.height);
    g->forescore = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR_ALPHA,
				    widget->allocation.width,
				    SCOREHEIGHT);
    g->forebutton = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR_ALPHA,
				    widget->allocation.width,
				    SCOREHEIGHT);

    g->foreground = 
      cairo_surface_create_similar (cairo_get_target (g->wc),
				    CAIRO_CONTENT_COLOR, // don't need alpha
				    widget->allocation.width,
				    widget->allocation.height);  

    // recenter all the verticies; doesn't require recomputation
    {
      vertex *v=g->g->verticies;
      int xd=(widget->allocation.width-oldw)*.5;
      int yd=(widget->allocation.height-oldh)*.5;

      while(v){
	v->x+=xd;
	v->y+=yd;
	v=v->next;
      }
    }

    // also verifies all verticies are onscreen
    resize_board(allocation->width,allocation->height);

    {
      int flag = g->pushed_background;
      if(flag)pop_background(g);

      update_background(widget);
      paint_bottom_box(g);
      score_update(g);
      resize_buttons(oldw,oldh,widget->allocation.width,widget->allocation.height);

      if(flag)push_background(g,g->redraw_callback);

    }
  }
}

static void gameboard_class_init (GameboardClass * class) {

  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (GTK_TYPE_WIDGET);

  object_class->destroy = gameboard_destroy;

  widget_class->realize = gameboard_realize;
  widget_class->expose_event = gameboard_expose;
  widget_class->size_request = size_request;
  widget_class->size_allocate = gameboard_size_allocate;
  widget_class->button_press_event = button_press;
  widget_class->button_release_event = button_release;
  widget_class->motion_notify_event = mouse_motion;

}

GType gameboard_get_type (void){

  static GType gameboard_type = 0;

  if (!gameboard_type)
    {
      static const GTypeInfo gameboard_info = {
        sizeof (GameboardClass),
        NULL,
        NULL,
        (GClassInitFunc) gameboard_class_init,
        NULL,
        NULL,
        sizeof (Gameboard),
        0,
        (GInstanceInitFunc) gameboard_init,
	0
      };

      gameboard_type = g_type_register_static (GTK_TYPE_WIDGET, "Gameboard",
                                               &gameboard_info, 0);
    }
  
  return gameboard_type;
}

Gameboard *gameboard_new (graph *gr) {
  GtkWidget *g = GTK_WIDGET (g_object_new (GAMEBOARD_TYPE, NULL));
  Gameboard *gb = GAMEBOARD (g);
  gb->g=gr;
  return gb;
}

void gameboard_reset (Gameboard *g) {
  g->lit_vertex=0;
  g->grabbed_vertex=0;
  g->delayed_background=0;

  g->group_drag=0;
  g->button_grabbed=0;
  g->selection_grab=0;
  g->selection_active=num_selected_verticies(g->g);

  update_background(&g->w);
  score_update(g);
}

void hide_lines(Gameboard *g){
  g->hide_lines=1;
  update_background(&g->w);
}

void show_lines(Gameboard *g){
  g->hide_lines=0;
  update_background(&g->w);
}

int get_hide_lines(Gameboard *g){
  return g->hide_lines;
}

void show_intersections(Gameboard *g){
  if(!g->show_intersections){
    g->show_intersections=1;
    update_background(&g->w);
  }else{
    hide_intersections(g);
  }
}

void hide_intersections(Gameboard *g){
  if(g->show_intersections){
    g->show_intersections=0;
    update_background(&g->w);
  }
}

int selected(Gameboard *g){
  return g->selection_active;
}

/***************** save/load gameboard the widget state we want to be persistent **************/

// there are only a few things; lines, intersections
int gameboard_write(FILE *f, Gameboard *g){
  
  if(g->hide_lines)
    fprintf(f,"hide_lines 1\n");
  if(g->show_intersections)
    fprintf(f,"show_intersections 1\n");
  
  return 0;
}

int gameboard_read(FILE *f, Gameboard *g){
  char *line=NULL;
  size_t n=0;
  int i;

  g->hide_lines = 0;
  g->show_intersections = 0;
  
  while(getline(&line,&n,f)>0){
    if (sscanf(line,"hide_lines %d",&i)==1)
      if(i)
	g->hide_lines = 1;
    
    if (sscanf(line,"show_intersections %d",&i)==1)
      if(i)
	g->show_intersections = 1;
  }

  free(line);

  return 0;
}
