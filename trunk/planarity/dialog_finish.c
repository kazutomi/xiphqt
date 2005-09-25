#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "graph.h"
#include "timer.h"
#include "gameboard.h"
#include "gameboard_draw_button.h"
#include "levelstate.h"
#include "dialog_finish.h"
#include "dialog_level.h"

static void finish_post (Gameboard *g){
  // back to buttonbar activity!
  g->finish_dialog_active=0;
  pop_curtain(g);
  levelstate_next();
  levelstate_go();
  enter_game(g);
} 

static void finish_level (Gameboard *g){
  // back to buttonbar activity!
  g->finish_dialog_active=0;
  pop_curtain(g);
  level_dialog(g,1);
} 

static void finish_quit (Gameboard *g){
  gtk_main_quit();
} 

static void local_go (Gameboard *g){
  undeploy_buttons(g,finish_post);
}

static void local_level (Gameboard *g){
  undeploy_buttons(g,finish_level);
}

static void local_quit (Gameboard *g){
  undeploy_buttons(g,finish_quit);
}

/* initialize the rather weird little animation engine */
static void setup_finish_buttons(Gameboard *g,int bw, int bh){
  int i;
  int w=g->g.width;
  int h=g->g.height;
  buttonstate *states=g->b.states;

  states[0].rollovertext="exit gPlanarity";
  states[1].rollovertext="level selection menu";
  states[10].rollovertext="play next level!";

  states[0].callback = local_quit;
  states[1].callback = local_level;
  states[10].callback = local_go;

  for(i=0;i<NUMBUTTONS;i++)
    states[i].position=0;

  states[0].position = 2; //center;
  states[1].position = 2; //center;
  states[10].position = 2; //center;

  {
    buttonstate *b=states;
    b->x = b->x_target = w/2 - bw/2 + FINISH_BUTTON_BORDER;
    b->y_active = h/2 + bh/2 - FINISH_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = 0;
  }

  {
    buttonstate *b=states+1;
    b->x = b->x_target = w/2;
    b->y_active = h/2 + bh/2 - FINISH_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = SWEEP_DELTA;
  }

  {
    buttonstate *b=states+10;
    b->x = b->x_target = w/2 + bw/2 - FINISH_BUTTON_BORDER;
    b->y_active = h/2 + bh/2 - FINISH_BUTTON_Y;
    b->y = b->y_target = b->y_inactive = b->y_active + BUTTON_EXPOSE;
    b->sweepdeploy = SWEEP_DELTA*2;
  }

  for(i=0;i<NUMBUTTONS;i++)
    if(states[i].position)
      rollover_extents(g,states+i);  
}

static void draw_finishbox(Gameboard *g){
  int w= g->g.width;
  int h= g->g.height;

  cairo_t *c = cairo_create(g->background);
  borderbox_path(c,
		 w/2 - FINISHBOX_WIDTH/2,
		 h/2 - FINISHBOX_HEIGHT/2,
		 FINISHBOX_WIDTH,
		 FINISHBOX_HEIGHT);
  cairo_set_source_rgb(c,1,1,1);
  cairo_fill(c);

  centerbox(c,
	    w/2 - FINISHBOX_WIDTH/2,
	    h/2 - FINISHBOX_HEIGHT/2,
	    FINISHBOX_WIDTH,
	    SCOREHEIGHT);

  centerbox(c,
	    w/2 - FINISHBOX_WIDTH/2 ,
	    h/2 + FINISHBOX_HEIGHT/2 - SCOREHEIGHT,
	    FINISHBOX_WIDTH,
	    SCOREHEIGHT);

  {
    cairo_matrix_t ma;
    char *time = get_timer_string();
    char buffer[160];
    int time_bonus=graphscore_get_bonus(&g->g);

    int y;

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);

    cairo_matrix_init_scale (&ma, 18.,18.);
    cairo_set_font_matrix (c,&ma);
    cairo_set_source_rgba (c, TEXT_COLOR);

    y=h/2-FINISHBOX_HEIGHT/2+SCOREHEIGHT/2;
    render_text_centered(c,"Level Complete!", w/2,y);y+=45;

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_matrix_init_scale (&ma, 16.,16.);
    cairo_set_font_matrix (c,&ma);

    snprintf(buffer,160,"Elapsed: %s",time);
    render_bordertext_centered(c,buffer, w/2,y);y+=24;


    snprintf(buffer,160,"Objective: %d points",graphscore_get_score(&g->g));
    render_bordertext_centered(c,buffer, w/2,y);y+=24;

    snprintf(buffer,160,"Time bonus: %d points",time_bonus);
    render_bordertext_centered(c,buffer, w/2,y);y+=35;

    cairo_select_font_face (c, "Arial",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);

    snprintf(buffer,160,"Final score: %d points",graphscore_get_score(&g->g)+time_bonus);
    render_bordertext_centered(c,buffer, w/2,y);y+=24;

    if(graphscore_get_score(&g->g)+time_bonus >= levelstate_get_hiscore()){
      cairo_set_source_rgba (c, HIGH_COLOR);
      render_bordertext_centered(c,"A high score!", w/2,y);y+=45;
      cairo_set_source_rgba (c, TEXT_COLOR);
    }else{
      snprintf(buffer,160,"Previous best: %ld points",levelstate_get_hiscore());
      render_bordertext_centered(c,buffer, w/2,y);y+=45;
    }


    render_bordertext_centered(c,"Total score to date:", w/2,y);
    y+=21;
    snprintf(buffer,160,"%ld points",levelstate_total_hiscore());
    render_bordertext_centered(c,buffer, w/2,y);

  }

  cairo_destroy(c);
}

void finish_level_dialog(Gameboard *g){
  g->finish_dialog_active=1;

  // set up new buttons
  setup_finish_buttons(g,FINISHBOX_WIDTH, FINISHBOX_HEIGHT);

  // draw pausebox
  push_curtain(g,draw_finishbox);

  // deploy new buttons
  deploy_buttons(g,0);
}

