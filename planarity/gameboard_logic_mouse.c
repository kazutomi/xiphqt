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
#include "dialog_level.h"

/* does the given x/y fall inside an on-screen vertex? */
static void check_lit(Gameboard *g,int x, int y){
  vertex *v = find_vertex(&g->g,x,y);
  if(v!=g->lit_vertex){
    invalidate_vertex(g,v);
    invalidate_vertex(g,g->lit_vertex);
    g->lit_vertex = v;
  }
}

/* toplevel mouse motion handler */
gint mouse_motion(GtkWidget        *widget,
		  GdkEventMotion   *event){
  Gameboard *g = GAMEBOARD (widget);

  /* the level selection dialog implements its own icon mouse management */
  if(g->level_dialog_active)
    level_mouse_motion(g,(int)event->x,(int)event->y);

  /* similarly, if a vertex is grabbed, the only thing that can
     happen is that we're dragging that vertex */
  
  if(g->grabbed_vertex){
    int x = (int)event->x;
    int y = (int)event->y;
    g->dragx = x-g->grabx;
    g->dragy = y-g->graby;
    
    invalidate_vertex(g,g->grabbed_vertex);
    invalidate_edges(widget,g->grabbed_vertex);
    move_vertex(&g->g,g->grabbed_vertex,x+g->graboffx,y+g->graboffy);
    invalidate_vertex(g,g->grabbed_vertex);
    invalidate_edges(widget,g->grabbed_vertex);
    return TRUE;

  }

  /* if the selection is grabbed, we can only be dragging a selection box */
  if (g->selection_grab){
    invalidate_selection(widget);
    
    g->selectionx = min(g->grabx, event->x);
    g->selectionw = labs(g->grabx - event->x);
    g->selectiony = min(g->graby, event->y);
    g->selectionh = labs(g->graby - event->y);
    select_verticies(&g->g,
		     g->selectionx,g->selectiony,
		     g->selectionx+g->selectionw-1,
		     g->selectiony+g->selectionh-1);
    
    invalidate_selection(widget);
    return TRUE;
  }

  /* if a selected vertex is grabbed (group drag) we're dragging
     the ghosted selection */
  
  if(g->group_drag){
    invalidate_verticies_selection(widget);
    g->dragx = event->x - g->grabx;
    g->dragy = event->y - g->graby;
    
    /* don't allow the drag to put a vertex offscreen; bound the drag at the edge */
    {
      vertex *v=g->g.verticies;
      int w=g->g.width;
      int h=g->g.height;
      
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
    
    invalidate_verticies_selection(widget);
    return TRUE;
  }

  /* A vertex rollover has priority over a button rollover.  However,
     a button grab disallows a rollover, as does the curtain. */
  if(!g->pushed_curtain && !g->button_grabbed)
    check_lit(g, (int)event->x,(int)event->y);
  
  /* handle button rollovers and ongoing grabs; other grabs are
     already disallowed, but also verify a vertex rollover isn't
     already active (and that buttons are currently hot) */
  if(!g->lit_vertex && g->b.buttons_ready){
    buttonstate *b = find_button(g,(int)event->x,(int)event->y);

    if(g->b.grabbed){
      /* a button is grabbed; a rollover sees only the grabbed
	 button */
      if(g->b.grabbed==b)
	button_set_state(g,b,1,1);
      else
	button_set_state(g,b,0,0);
    }else{
      /* no button is grabbed; any button may see a rollover */
      if(b)
	button_set_state(g,b,1,0);
      else
	button_clear_state(g);
    }
  }
  
  return TRUE;
}

/* toplevel mouse button press handler */
gboolean mouse_press (GtkWidget        *widget,
		      GdkEventButton   *event){

  if(event->type == GDK_BUTTON_PRESS){ // filter out doubleclicks

    Gameboard *g = GAMEBOARD (widget);
    
    vertex *v = find_vertex(&g->g,(int)event->x,(int)event->y);
    buttonstate *b = find_button(g,(int)event->x,(int)event->y);
    int old_intersections = g->show_intersections;
  
    /* the level selection dialog implements its own icon mouse management */
    if(g->level_dialog_active)
      level_mouse_press(g,(int)event->x,(int)event->y);
    
    button_clear_state(g);
    g->button_grabbed=0;
    set_show_intersections(g,0);
    
    /* presses that affect board elements other than buttons can only
       happen when the curtain isn't pushed.  */
    if(!g->pushed_curtain){
      
      /* case: shift-click addition of a single new vertex to selection */
      if(v && event->state&GDK_SHIFT_MASK){
	if(v->selected){
	  v->selected=0;
	  g->selection_active--;
	}else{
	  v->selected=1;
	  g->selection_active++;
	}
	invalidate_vertex(g,g->lit_vertex);
	return TRUE;
      }
      
      /* case: shift-click drag of an additional region to add to selection */
      if(event->state&GDK_SHIFT_MASK){
	g->selection_grab=1;
	g->grabx = (int)event->x;
	g->graby = (int)event->y;
	g->selectionx = g->grabx;
	g->selectionw = 0;
	g->selectiony = g->graby;
	g->selectionh = 0;
	return TRUE;
      }
      
      /* case: drag the selected group of verticies to new location */
      if(g->selection_active && v && v->selected){
	g->group_drag=1;
	g->grabx = (int)event->x;
	g->graby = (int)event->y;
	g->dragx = 0;
	g->dragy = 0;
	// put the verticies into the background for faster update
	update_full(g); 
	return TRUE;
      }
      
      /* if selection is active, we got this far, and we're not about to
	 take action based on pointing at a button, selection should be
	 inactivated */
      if(g->selection_active && (g->lit_vertex || !b)){
	deselect_verticies(&g->g);
	g->selection_active=0;
	g->group_drag=0;
	// potentially pull verticies out of background (can happen if
	// mouse went offscreen during a grab)
	update_full(g); 
      }
      
      /* case: grab/drag a single unselected vertex */
      if(g->lit_vertex){
	g->grabbed_vertex = g->lit_vertex;
	g->grabx = (int)event->x;
	g->graby = (int)event->y;
	g->graboffx = g->grabbed_vertex->x-g->grabx;
	g->graboffy = g->grabbed_vertex->y-g->graby;
	
	grab_vertex(&g->g,g->grabbed_vertex);
	invalidate_attached(widget,g->grabbed_vertex);
	invalidate_edges(widget,g->grabbed_vertex);
	
	// highlight vertex immediately; update the background after the
	// vertex change
	update_full_delayed(g);
	return TRUE;
      }
    }
    
    /* case: button click */
    if(b){
      // if intersections were visible, a button press is the only
      // click that wouldn't auto-hide them, so restore the flag to
      // its state before entry.
      g->show_intersections=old_intersections;
      
      button_set_state(g,b,1,1);
      g->b.grabbed = b;
      return TRUE;
    }

    /* clicking anywhere else wtith no modifiers initiates a new
       selection drag (assuming curtain isn't pushed) */
    if(!g->pushed_curtain){
      g->selection_grab=1;
      g->grabx = (int)event->x;
      g->graby = (int)event->y;
      g->selectionx = g->grabx;
      g->selectionw = 0;
      g->selectiony = g->graby;
      g->selectionh = 0;
      return TRUE;
    }
  }
  
  return TRUE;
}

/* toplevel mouse button release handler */
gboolean mouse_release (GtkWidget        *widget,
			GdkEventButton   *event){
  Gameboard *g = GAMEBOARD (widget);
  
  /* case: button grabbed */
  if(g->b.grabbed){
    buttonstate *b = find_button(g,(int)event->x,(int)event->y);
    if(b && g->b.grabbed==b){
      button_set_state(g,b,1,0);
      if(b->callback)
	b->callback(g);
    }
    g->b.grabbed=0;
  }
  
  /* if the curtain is pushed, no sense checking for other grabs */
  if(!g->pushed_curtain){
  
    /* case: release a grabbed vertex */
    if(g->grabbed_vertex){
      ungrab_vertex(&g->g,g->grabbed_vertex);
      update_add_vertex(g,g->grabbed_vertex);
      update_score(g);
      g->grabbed_vertex = 0;

      if(g->g.active_intersections<=g->g.objective)
	deploy_check(g);
      else
	undeploy_check(g);

    }
    
    /* case: release a selection grab */
    if(g->selection_grab){
      invalidate_selection(widget);
      g->selection_grab=0;
      
      /* are there selected verticies? If only one was selected, that's
	 usually due to an accidental drag while clicking.  Treat a
	 single-vertex drag select as a nil selection */
      if(num_selected_verticies(&g->g)<=1){
	g->selection_active=0;
	deselect_verticies(&g->g); // could have grabbed just one
      }else{
	commit_volatile_selection();
	g->selection_active=num_selected_verticies(&g->g);
      }
    }
    
    /* case: release a group drag */
    if(g->group_drag){
      move_selected_verticies(&g->g,g->dragx,g->dragy);
      update_full(g); // cheating
      update_score(g);
      g->group_drag=0;
    }
  }

  /* a release may result in a new mouse-over; look for it */
  mouse_motion(widget,(GdkEventMotion *)event); // the cast is safe in this case

  return TRUE;
}

