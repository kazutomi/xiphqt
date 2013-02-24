#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo/cairo.h>

#define W 3840
#define H 2160
#define CR1 907.5
#define CR2 1237.5
#define DE  .2
#define WH 225
#define SAMPLES 3840
#define SPACING 100
#define IMPULSE 80

#define MULT 1.
#define RATE 48000

#define TIME 25
#define PER 36


#define LINE_WIDTH 6
#define WAVE_WIDTH 15
#define BAND .6,0,0,1
#define WAVETOP .0,.0,.0,1
#define WAVEBOTTOM .0,.0,.0,.25
#define LINE .6,.9,1.,1

void write_frame(cairo_surface_t *surface, char *base, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch6_%s-%04d.png",base,frameno);
  ret = cairo_surface_write_to_png(surface,buffer);
  if(ret != CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not write %s: %s\n",
	    buffer,cairo_status_to_string(ret));
    exit(1);
  }
}

void transparent_surface(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  cairo_set_source_rgba(c,1.,1.,1.,1.);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
  cairo_paint(c);
  cairo_destroy(c);
}

void draw_lines(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  double o;

  cairo_set_source_rgba(c,LINE);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width(c,LINE_WIDTH);

  cairo_move_to(c,W/2.,CR2-WH);
  cairo_line_to(c,W/2.,CR2+WH*DE);
  o=SPACING*MULT;

  while(o<W+LINE_WIDTH/2.){
    double x0=W/2.-o;
    double x1=W/2.+o;

    cairo_move_to(c,x0,CR2-WH);
    cairo_line_to(c,x0,CR2+WH*DE);
    cairo_move_to(c,x1,CR2-WH);
    cairo_line_to(c,x1,CR2+WH*DE);

    o+=SPACING*MULT;
  }

  cairo_stroke(c);
  cairo_destroy(c);
}

void draw_wrong(cairo_surface_t *cs,float *data, float center, float shift){
  int i;
  cairo_t *c = cairo_create(cs);

  float center_sample = center*RATE+shift;
  float samples = (float)W/MULT;
  int start_sample = rint(center_sample-(samples/2));
  int end_sample = rint(start_sample+samples);
  float hold = CR2;
  float thresh=0;
  double o=SPACING*MULT;

  while(o+LINE_WIDTH<W/2.){
    thresh=W/2.-o;
    o+=SPACING*MULT;
  }

  fprintf(stderr,"%d %d %f\n",start_sample,end_sample,shift);

  cairo_set_source_rgba(c,WAVETOP);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,WAVE_WIDTH);

  for(i=start_sample;i<end_sample;i++){
    double t = (double)(i-shift)/RATE;
    double x = (double)W/2. + (t-center)*RATE*MULT;
    double y = CR1-data[i]*WH;

    if(i==start_sample)
      cairo_move_to(c,x,y);
    else
      cairo_line_to(c,x,y);
  }

  cairo_stroke(c);

  cairo_set_source_rgba(c,WAVEBOTTOM);

  cairo_move_to(c,thresh,CR2);
  for(i=start_sample;i<end_sample;i++){
    double tp = (double)(i-shift-1)/RATE;
    double t = (double)(i-shift)/RATE;
    double xp = (double)W/2. + (tp-center)*RATE*MULT;
    double x = (double)W/2. + (t-center)*RATE*MULT;

    if(xp<thresh && x>=thresh){
      cairo_line_to(c,thresh,hold);

      hold=CR2-data[i]*WH;

      cairo_line_to(c,thresh,hold);
      thresh += SPACING*MULT;
    }

    cairo_line_to(c,x,hold);
  }

  cairo_stroke(c);
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);
  float *data=calloc(5*48000,sizeof(float));
  int i,count=0;

  int start=2.5*48000-IMPULSE/2;
  for(i=0;i<IMPULSE;i++){
    float v = sin((i+.5)/IMPULSE*M_PI);
    data[i+start]= v*v*.8;
  }

  for(i=0;i<360;i++){
    transparent_surface(cs);
    draw_lines(cs);
    draw_wrong(cs,data, 2.5, (TIME*24/2.+2.2*24-i)*SPACING/PER);
    write_frame(cs,"impulse-wrong",count++);
  }

  memset(data,0,sizeof(*data)*48000*5);

  for(i=0;i<2.5*48000;i++)
    data[i]=0;
  for(;i<5*48000;i++)
    data[i]=.8;

  count=0;

  for(i=0;i<360;i++){
    transparent_surface(cs);
    draw_lines(cs);
    draw_wrong(cs,data, 2.5, (TIME*24/2.+2.2*24-i)*SPACING/PER);
    write_frame(cs,"box-wrong",count++);
  }

  cairo_surface_destroy(cs);
  return 0;
}
