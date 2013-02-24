#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 300

#define TEXTY 140
#define TEXTY2 140
#define TEXT_FONT_SIZE 90

#define SAMPLES 18
#define CYCLES  1

#define PRE       108
#define FADE      12
#define ARROW     76
#define BRACKETS  72
#define MID       160
#define WAVEFADE  48
#define WAVEFORM  48
#define KHZ_UP    72
#define KHZ_20    160

#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

#define LINE_WIDTH 15
#define COLOR .4,.4,.4,1

#define ARROW_LINE_WIDTH 6
#define ARROW_COLOR 1,0,0

#define WLINE_WIDTH 15
#define WCOLOR 1,.2,.2,.8


void write_frame(cairo_surface_t *surface, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch3_samples-%04d.png",frameno);
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

void draw_arrows(cairo_surface_t *cs,double del){
  int i;
  cairo_t *c = cairo_create(cs);

  cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_MITER);
  cairo_set_source_rgba(c,ARROW_COLOR,del);

  for(i=1;i<SAMPLES;i++){
    int x = tox(i);
    double y = get_y(x,1.);

    //if(y>CR)y=CR;


    cairo_set_line_width(c,ARROW_LINE_WIDTH+3);
    cairo_set_source_rgba(c,1.,1.,1.,del);
    cairo_move_to(c,x,y-LINE_WIDTH*5);
    cairo_line_to(c,x,y-LINE_WIDTH*5-ARROW_LINE_WIDTH*10-1.5);

    cairo_move_to(c,x-ARROW_LINE_WIDTH*3,
                  y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);
    cairo_line_to(c,x,y-LINE_WIDTH*5);
    cairo_line_to(c,x+ARROW_LINE_WIDTH*3,
                  y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);


    cairo_stroke(c);

    cairo_set_line_width(c,ARROW_LINE_WIDTH);
    cairo_set_source_rgba(c,ARROW_COLOR,del);
    cairo_move_to(c,x,y-LINE_WIDTH*5);
    cairo_line_to(c,x,y-LINE_WIDTH*5-ARROW_LINE_WIDTH*10);

    cairo_move_to(c,x-ARROW_LINE_WIDTH*3,
                  y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);
    cairo_line_to(c,x,y-LINE_WIDTH*5);
    cairo_line_to(c,x+ARROW_LINE_WIDTH*3,
                  y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);

    cairo_stroke(c);
  }

  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width(c,LINE_WIDTH*2);

  for(i=1;i<SAMPLES;i++){
    int x = tox(i);
    double y = get_y(x,1.);

    cairo_move_to(c,x,y);
    cairo_line_to(c,x,y+.001);
  }
  cairo_stroke(c);

  cairo_destroy(c);
}

void draw_brackets(cairo_surface_t *cs, double del){
  int i;
  cairo_t *c = cairo_create(cs);
  double r = 10;
  cairo_font_face_t *ff;
  cairo_text_extents_t extents;

  cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_MITER);
  cairo_set_line_width(c,ARROW_LINE_WIDTH);
  cairo_set_source_rgba(c,ARROW_COLOR,del);

  for(i=0;i<SAMPLES;i++){
    double x0 = tox(i)+LINE_WIDTH;
    double x1 = tox(i+1)-LINE_WIDTH;
    double y = CR+WH+ARROW_LINE_WIDTH*10;

    cairo_move_to(c,x0,y);
    cairo_line_to(c,x1,y);

    cairo_move_to(c,x0+ARROW_LINE_WIDTH*4,y-ARROW_LINE_WIDTH*2.5);
    cairo_line_to(c,x0,y);
    cairo_line_to(c,x0+ARROW_LINE_WIDTH*4,y+ARROW_LINE_WIDTH*2.5);

    cairo_move_to(c,x1-ARROW_LINE_WIDTH*4,y-ARROW_LINE_WIDTH*2.5);
    cairo_line_to(c,x1,y);
    cairo_line_to(c,x1-ARROW_LINE_WIDTH*4,y+ARROW_LINE_WIDTH*2.5);
    cairo_stroke(c);
  }

  ff = cairo_toy_font_face_create ("Adobe Garamond",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
  if(!ff){
    fprintf(stderr,"Unable to create cursor font");
    exit(1);
  }
  cairo_set_font_face(c,ff);
  cairo_set_font_size(c, TEXT_FONT_SIZE);
  cairo_text_extents(c, "undefined", &extents);
  double TH = (TEXTY-extents.height)*2 + extents.height;
  cairo_move_to(c, (W-extents.x_advance)/2, H-TH+TEXTY2+ARROW_LINE_WIDTH*5);

  cairo_show_text(c, "unde\xEF\xAC\x81ned");

  cairo_destroy(c);
}

void draw_regions(cairo_surface_t *cs, double del){
  int i;
  cairo_t *c = cairo_create(cs);

  cairo_set_line_width(c,2);

  for(i=0;i<SAMPLES;i++){
    double x0 = tox(i)+LINE_WIDTH/2;
    double x1 = tox(i+1)-LINE_WIDTH/2;
    double y0 = CR-WH-ARROW_LINE_WIDTH*15;
    double y1 = CR+WH+ARROW_LINE_WIDTH*15;

    cairo_set_source_rgba(c,ARROW_COLOR,del*.1);
    cairo_move_to(c,x0,y0);
    cairo_line_to(c,x0,y1);
    cairo_line_to(c,x1,y1);
    cairo_line_to(c,x1,y0);
    cairo_close_path(c);
    cairo_fill(c);

    cairo_set_source_rgba(c,ARROW_COLOR,del);
    cairo_move_to(c,x0,y0);
    cairo_line_to(c,x0,y1);
    cairo_move_to(c,x1,y0);
    cairo_line_to(c,x1,y1);
    cairo_stroke(c);
  }

  cairo_destroy(c);

}

void draw_readout(cairo_surface_t *cs, double alpha, double f){
  char buffer[80];

  cairo_t *c = cairo_create(cs);
  cairo_font_face_t *ff;
  cairo_font_face_t *ft;
  cairo_text_extents_t extents;
  cairo_text_extents_t extents2;
  cairo_text_extents_t extents3;

  ff = cairo_toy_font_face_create ("Liberation Sans",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
  ft = cairo_toy_font_face_create ("Adobe Garamond",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  if(!ff){
    fprintf(stderr,"Unable to create cursor font");
    exit(1);
  }
  cairo_set_font_face(c,ft);
  cairo_set_font_size(c, TEXT_FONT_SIZE);

  cairo_text_extents(c, "frequency: ", &extents);
  cairo_text_extents(c, "kHz", &extents3);

  cairo_set_font_face(c,ff);
  cairo_text_extents(c, "10", &extents2);

  double TH = (TEXTY-extents2.height)*2 + extents2.height;

  cairo_set_line_width(c,2);

  cairo_set_font_face(c,ft);
  cairo_move_to(c, (W-extents.x_advance-extents2.x_advance-extents3.x_advance)/2, TEXTY);
  cairo_set_source_rgba(c,0,0,0,alpha);
  cairo_show_text(c, "frequency: ");

  cairo_set_font_face(c,ff);
  snprintf(buffer, 80,"%d",(int)(f+.001));
  cairo_set_source_rgba(c,.8,0,0,alpha);
  cairo_show_text(c, buffer);
  cairo_set_source_rgba(c,0,0,0,alpha);
  cairo_set_font_face(c,ft);
  cairo_show_text(c, "kHz");

  cairo_text_extents(c, "sampling rate: 44.1kHz", &extents);

  cairo_move_to(c, (W-extents.x_advance)/2, H-TH+TEXTY2);
  cairo_show_text(c, "sampling rate: 44.1kHz");

  cairo_font_face_destroy(ff);
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);

  int i,count=0;

  /* static preroll */
  for(i=0;i<PRE;i++){
    clear_surface(cs);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    write_frame(cs,count++);
  }

  /* fade in arrows */
  for(i=0;i<FADE;i++){
    clear_surface(cs);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    draw_arrows(cs,(double)i/FADE);
    write_frame(cs,count++);
  }

  /* hold arrows */
  for(i=0;i<ARROW;i++){
    clear_surface(cs);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    draw_arrows(cs,1.);
    write_frame(cs,count++);
  }

  /* fade out arrows, fade in brackets */
  for(i=0;i<FADE;i++){
    clear_surface(cs);
    draw_brackets(cs,(double)i/FADE);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    draw_arrows(cs,1-(double)i/FADE);
    write_frame(cs,count++);
  }

  /* fade in regions */
  for(i=0;i<FADE;i++){
    clear_surface(cs);
    draw_regions(cs,(double)i/FADE);
    draw_brackets(cs,1.);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    write_frame(cs,count++);
  }

  /* hold */
  for(i=0;i<BRACKETS;i++){
    clear_surface(cs);
    draw_regions(cs,1.);
    draw_brackets(cs,1.);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    write_frame(cs,count++);
  }

  /* fade out regions, brackets */
  for(i=0;i<FADE;i++){
    clear_surface(cs);
    draw_regions(cs,1.-(double)i/FADE);
    draw_brackets(cs,1.-(double)i/FADE);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    write_frame(cs,count++);
  }

  /* mid */
  for(i=0;i<MID;i++){
    clear_surface(cs);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    write_frame(cs,count++);
  }

  /* fade in waveform */
  for(i=0;i<WAVEFADE;i++){
    clear_surface(cs);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    draw_waveform(cs,(double)i/WAVEFADE,1.);
    write_frame(cs,count++);
  }

  /* hold waveform */
  for(i=0;i<WAVEFORM;i++){
    clear_surface(cs);
    draw_stems(cs,1.,1.);
    draw_axis(cs,1.);
    draw_samples(cs,1.,1.);
    draw_waveform(cs,1.,1.);
    write_frame(cs,count++);
  }

  /* fade in and advance to 20kHz */
  for(i=0;i<KHZ_UP;i++){
    double alpha = (double)i/FADE;
    double m,base_f = 44100. / SAMPLES * CYCLES;
    double f = sin((i+.5)/KHZ_UP*M_PI/2.);
    f = sin(f*M_PI/2.);
    f = base_f + (20000.-base_f)*f*f;
    m = f/base_f;
    if(alpha>1.)alpha=1.;

    clear_surface(cs);
    draw_stems(cs,1,m);
    draw_axis(cs,1.0);
    draw_readout(cs,alpha,rint(f/1000.));
    draw_samples(cs,1,m);
    draw_waveform(cs,1.0,m);
    write_frame(cs,count++);
  }

  /* hold 20kHz */
  for(i=0;i<KHZ_20;i++){
    double m,base_f = 44100. / SAMPLES * CYCLES;
    double f = 20000;
    m = f/base_f;

    clear_surface(cs);
    draw_stems(cs,1.,m);
    draw_axis(cs,1.);
    draw_readout(cs,1.,rint(f/1000.));
    draw_samples(cs,1.,m);
    draw_waveform(cs,1.,m);
    write_frame(cs,count++);
  }

  cairo_surface_destroy(cs);
  return 0;
}
