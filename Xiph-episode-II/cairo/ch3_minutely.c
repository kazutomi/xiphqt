#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 300

#define TEXTY 90
#define TEXTY2 75
#define TEXT_FONT_SIZE 60

#define SAMPLES 18
#define CYCLES  1

#define PRE        108
#define DRAW       72
#define DRAWHOLD   2
#define CIRCLE     6
#define CIRCLEHOLD 12


#define POST       240


#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

#define LINE_WIDTH 15
#define COLOR .4,.4,.4,1

#define ARROW_LINE_WIDTH 6
#define ARROW_COLOR 1,0,0

#define WLINE_WIDTH 15
#define WCOLOR 1,.2,.2,.8
#define DCOLOR .2,.2,1,1
#define CCOLOR .8,0,0,1


void write_frame(cairo_surface_t *surface, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch3_minute-%04d.png",frameno);
  ret = cairo_surface_write_to_png(surface,buffer);
  if(ret != CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not write %s: %s\n",
	    buffer,cairo_status_to_string(ret));
    exit(1);
  }
}

void clear_surface(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  cairo_set_source_rgb(c,1.,1.,1.);
  cairo_paint(c);
  cairo_destroy(c);
}

void transparent_surface(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_paint(c);
  cairo_destroy(c);
}

double get_y(double x,double f){
  return CR + WH*sin((double)x/W*CYCLES*f*2*M_PI);
}

int tosample(double x){
  double SW = (double)W/SAMPLES;
  return rint(x/SW);
}

int tox(int sample){
  double SW = (double)W/SAMPLES;
  return rint(sample*SW);
}

double get_dy(double x,double f){
  double s = (double)x*SAMPLES/W;
  double y = sin((double)x/W*CYCLES*f*2*M_PI);
  if(s>10 && s<11){
    double d = sin((s-10)*M_PI);
    y += .1 * d*d;
  }
  return CR+WH*y;
}

void draw_axis(cairo_surface_t *cs, double del){
  int i;
  cairo_t *c = cairo_create(cs);
  double SW = 105;
  cairo_text_extents_t extents;
  cairo_font_face_t *ff;

  cairo_set_source_rgba(c,AXIS_COLOR*del);
  cairo_set_line_width(c,AXIS_LINE_WIDTH);
  cairo_move_to(c,0,CR);
  cairo_line_to(c,W,CR);
  cairo_move_to(c,W-SW/2,CR-SW/3);
  cairo_line_to(c,W,CR);
  cairo_line_to(c,W-SW/2,CR+SW/3);
  cairo_stroke(c);

  cairo_set_line_width(c,AXIS_LINE_WIDTH*.75);

  for(i=0;i<SAMPLES;i++){
    double x = tox(i);
    cairo_move_to(c,x,CR-AXIS_LINE_WIDTH*2);
    cairo_line_to(c,x,CR+AXIS_LINE_WIDTH*2);
  }
  cairo_stroke(c);


  ff = cairo_toy_font_face_create ("Adobe Garamond",
                                   CAIRO_FONT_SLANT_ITALIC,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  if(!ff){
    fprintf(stderr,"Unable to create axis font");
    exit(1);
  }
  cairo_set_font_face(c,ff);
  cairo_set_font_size(c, AXIS_FONT_SIZE);
  cairo_text_extents(c, "time", &extents);
  cairo_move_to(c, W-extents.width-SW*.6, CR + AXIS_FONT_SIZE);
  cairo_show_text(c, "time");

  cairo_font_face_destroy(ff);
  cairo_destroy(c);
}

void draw_waveform(cairo_surface_t *cs, double del, double f){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw sine */
  cairo_set_source_rgba(c,WCOLOR);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,WLINE_WIDTH);

  for(i=-WLINE_WIDTH;i<(W*del)+WLINE_WIDTH;i++){
    double y = get_y(i,f);
    if(!i)
      cairo_move_to(c,i,y);
    else
      cairo_line_to(c,i,y);
  }
  cairo_stroke(c);

  cairo_destroy(c);
}

void draw_different(cairo_surface_t *cs, double del, double f){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw sine */
  cairo_set_source_rgba(c,DCOLOR);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,WLINE_WIDTH);

  for(i=-WLINE_WIDTH;i<(W*del)+WLINE_WIDTH;i++){
    double y = get_dy(i,f);
    if(!i)
      cairo_move_to(c,i,y);
    else
      cairo_line_to(c,i,y);
  }
  cairo_stroke(c);

  cairo_destroy(c);
}

void draw_circle(cairo_surface_t *cs, double del, double f){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw sine */
  cairo_set_source_rgba(c,CCOLOR);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,WLINE_WIDTH/2);

  cairo_translate(c,W*10.5/SAMPLES,CR-WH*.95);
  cairo_scale(c,.8,1);
  for(i=0;i<del*1000;i++)
    cairo_arc_negative(c,0,0,WH*(.3+.05*(i/1000.)),-i/800.*2*M_PI- M_PI/3,-(i+1)/800.*2*M_PI-M_PI/3);
  cairo_stroke(c);

  cairo_destroy(c);
}

void draw_stems(cairo_surface_t *cs, double del, double f){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  cairo_set_source_rgba(c,COLOR*del*.8);

  for(i=1;i<SAMPLES;i++){
    int sx = tox(i);
    double y = get_y(sx,f);

    cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
    cairo_set_line_width(c,LINE_WIDTH*.75);
    cairo_move_to(c,sx,CR);
    cairo_line_to(c,sx,y);
    cairo_stroke(c);
  }

  cairo_destroy(c);
}

void draw_samples(cairo_surface_t *cs, double del, double f){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=1;i<SAMPLES;i++){
    int sx = tox(i);
    double y = get_y(sx,f);

    cairo_set_source_rgba(c,COLOR*del*.4);
    cairo_set_line_width(c,LINE_WIDTH/3);
    cairo_arc(c,sx,y,LINE_WIDTH*2,0,2*M_PI);
    cairo_stroke(c);

    cairo_set_source_rgba(c,COLOR*del);
    cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(c,LINE_WIDTH*2);
    cairo_line_to(c,sx,y);
    cairo_line_to(c,sx,y+.001);
    cairo_stroke(c);
  }

  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);

  int i,count=0;
  double m,base_f = 44100. / SAMPLES * CYCLES;
  double f = 20000;
  m = f/base_f;

#if 0
  /* transparent 20kHz without the waveform */

  transparent_surface(cs);
  draw_stems(cs,1.,m);
  draw_axis(cs,1.);
  draw_samples(cs,1.,m);
  draw_waveform(cs,1.,m);
  write_frame(cs,9998);

  transparent_surface(cs);
  draw_stems(cs,1.,m);
  draw_axis(cs,1.);
  draw_samples(cs,1.,m);
  write_frame(cs,9999);

#else

  /* static preroll */
  transparent_surface(cs);
  //draw_stems(cs,1.,m);
  //draw_axis(cs,1.);
  //draw_samples(cs,1.,m);
  //draw_waveform(cs,1.,m);
  for(i=0;i<PRE;i++){
    write_frame(cs,count++);
  }

  /* draw 'different' waveform */
  int circlecount=0;
  for(i=0;i<DRAW;i++){
    double s = (float)i/DRAW*SAMPLES;
    transparent_surface(cs);
    //draw_stems(cs,1.,m);
    //draw_axis(cs,1.);
    //draw_samples(cs,1.,m);
    //draw_waveform(cs,1.,m);
    draw_different(cs,(i+.5)/DRAW,m);
    if(s>=14){
      draw_circle(cs,(circlecount+.5)/CIRCLE,m);
      if(circlecount+1<CIRCLE)
        circlecount++;
    }
    write_frame(cs,count++);
  }

  for(i=0;i<POST;i++){
    write_frame(cs,count++);
  }

#endif
  cairo_surface_destroy(cs);
  return 0;
}
