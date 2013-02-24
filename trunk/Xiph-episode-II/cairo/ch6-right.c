#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo/cairo.h>
#include <fftw3.h>

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

#define TIME 8.5
#define PER 36


#define LINE_WIDTH 6
#define WAVE_WIDTH 15
#define BAND .6,0,0,1

#define WAVETOP .0,.0,.0,1
#define WAVEBOTTOM 0,.0,.0,.25
#define STEM .0,.0,.0,.3
#define LOLLI .0,.0,.0,1

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

void draw_right(cairo_surface_t *cs,float *data, float center, float shift){
  int i;
  cairo_t *c = cairo_create(cs);

  float center_sample = center*RATE+shift;
  float samples = (float)W/MULT;
  int start_sample = rint(center_sample-(samples/2));
  int end_sample = rint(start_sample+samples);
  float hold = 0;
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

  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,WAVE_WIDTH);

  for(i=start_sample;i<end_sample;i++){
    double t = (double)(i-shift)/RATE;
    double x = (double)W/2. + (t-center)*RATE*MULT;
    double y = CR2-data[i]*WH;

    if(i==start_sample)
      cairo_move_to(c,x,y);
    else
      cairo_line_to(c,x,y);
  }

  cairo_stroke(c);

  cairo_set_line_width(c,LINE_WIDTH+2);
  cairo_set_source_rgba(c,STEM);

  for(i=start_sample;i<end_sample;i++){
    double tp = (double)(i-shift-1)/RATE;
    double t = (double)(i-shift)/RATE;
    double xp = (double)W/2. + (tp-center)*RATE*MULT;
    double x = (double)W/2. + (t-center)*RATE*MULT;

    if(i==start_sample || (xp<thresh && x>=thresh)){
      hold=CR2-data[i]*WH;
      cairo_move_to(c,thresh,CR2);
      cairo_line_to(c,thresh,hold);
      thresh+=SPACING*MULT;
    }
  }
  cairo_stroke(c);

  o=SPACING*MULT;

  while(o+LINE_WIDTH<W/2.){
    thresh=W/2.-o;
    o+=SPACING*MULT;
  }

  cairo_set_line_width(c,WAVE_WIDTH/2.);

  for(i=start_sample;i<end_sample;i++){
    double tp = (double)(i-shift-1)/RATE;
    double t = (double)(i-shift)/RATE;
    double xp = (double)W/2. + (tp-center)*RATE*MULT;
    double x = (double)W/2. + (t-center)*RATE*MULT;

    if(i==start_sample || (xp<thresh && x>=thresh)){
      hold=CR2-data[i]*WH;

      cairo_set_source_rgba(c,LOLLI);
      cairo_move_to(c,thresh,hold);
      cairo_arc(c,thresh,hold,LINE_WIDTH*2,0,2*M_PI);
      cairo_fill(c);

      cairo_set_source_rgba(c,STEM);
      cairo_arc(c,thresh,hold,LINE_WIDTH*5,0,2*M_PI);
      cairo_stroke(c);
      thresh+=SPACING*MULT;
    }
  }


  cairo_stroke(c);
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);
  float *data=calloc(5*48000,sizeof(float));

  fftwf_plan planf = fftwf_plan_dft_r2c_1d(5*48000-2,data,
                                           (fftwf_complex *)data,
                                           FFTW_ESTIMATE);
  fftwf_plan plani = fftwf_plan_dft_c2r_1d(5*48000-2,(fftwf_complex *)data,
                                           data,
                                           FFTW_ESTIMATE);

  int i,count=0;

  int start=2.5*48000-IMPULSE/2;
  for(i=0;i<IMPULSE;i++){
    float v = sin((i+.5)/IMPULSE*M_PI);
    data[i+start]= v*v      * (2./(5*48000));
  }
  /* this is just an illustration, so quick hack the bandlimit; the
     below is _WRONG_ for audio use, but it will be visually OK */
  fftwf_execute(planf);
  for(i=5*48000/2/100*2;i<5*48000;i++)
    data[i]=0;
  fftwf_execute(plani);
#define WIN 3840

  for(i=0;i<(int)(2.5*48000-WIN/2);i++)
    data[i]=0;
  for(i=0;i<WIN;i++){
    float v=sin((i+.5)/WIN*M_PI);
    data[i+(int)(2.5*48000-WIN/2)]*=v*v;
  }
  for(i=(int)(2.5*48000+WIN/2);i<(int)(5*48000);i++)
    data[i]=0;

  float shift=0;
  for(i=0;i<24;i++){
    transparent_surface(cs);
    draw_lines(cs);
    draw_right(cs,data, 2.5, shift);
    write_frame(cs,"impulse-right",count++);
    shift -= SPACING/PER*sin((i+.5)/48.*M_PI)*sin((i+.5)/48.*M_PI);
  }

  for(;i<TIME*24;i++){
    transparent_surface(cs);
    draw_lines(cs);
    draw_right(cs,data, 2.5, shift);
    write_frame(cs,"impulse-right",count++);
    shift-=SPACING/PER;
  }

#define OFFSET 0

  int period = 200;
  int coeffs = period/4;
  float *square_coeffs;
  float *square_phases;
  int square_coeffs_n;
  int j;
  double phase = 0;
  float *work = fftwf_malloc((period+2)*sizeof(*work));
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(period,work,
                                          (fftwf_complex *)work,
                                          FFTW_ESTIMATE);
  for(i=0;i<period/2;i++)
    work[i]=1.;
  for(;i<period;i++)
    work[i]=-1.;
  fftwf_execute(plan);

  square_coeffs_n = coeffs;
  square_coeffs = calloc(coeffs,sizeof(*square_coeffs));
  square_phases = calloc(coeffs,sizeof(*square_phases));

  for(i=1,j=0;j<square_coeffs_n;i+=2,j++){
    square_coeffs[j] = hypotf(work[i<<1],work[(i<<1)+1]) / period;
    square_phases[j] = atan2f(work[(i<<1)+1],work[i<<1]);
  }

  for(i=0;i<5*48000;i++){
    float acc=0.;
    int k;
    for(j=0,k=1;j<square_coeffs_n;j++,k+=2)
      acc += square_coeffs[j] *
        cos( square_phases[j] + k*phase);
    data[i]=(acc+.5)*.8;
    phase += 2*M_PI/period/SPACING;
    if(phase>=2*M_PI)phase-=2*M_PI;
  }

  for(i=0;i<(int)(2.5*48000-WIN/2);i++)
    data[i]=0;
  for(i=0;i<WIN/2;i++){
    float v=sin((i+.5)/WIN*M_PI);
    data[i+(int)(2.5*48000-WIN/2)]*=v*v;
  }
  for(;i<WIN;i++){
    float v=sin((i+.5)/WIN*M_PI);
    data[i+(int)(2.5*48000-WIN/2)] *= v*v;
    data[i+(int)(2.5*48000-WIN/2)] += .8*(1.-v*v);
  }
  for(i=(int)(2.5*48000+WIN/2);i<(int)(5*48000);i++)
    data[i]=.8;

  count=0;
  shift=0;
  for(i=0;i<24;i++){
    transparent_surface(cs);
    draw_lines(cs);
    draw_right(cs,data, 2.5, shift);
    write_frame(cs,"box-right",count++);
    shift -= SPACING/PER*sin((i+.5)/48.*M_PI)*sin((i+.5)/48.*M_PI);
  }
  for(;i<TIME*24;i++){
    transparent_surface(cs);
    draw_lines(cs);
    draw_right(cs,data, 2.5, shift);
    write_frame(cs,"box-right",count++);
    shift-=SPACING/PER;
  }




  cairo_surface_destroy(cs);
  return 0;
}
