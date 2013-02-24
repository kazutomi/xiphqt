#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>
#include <fftw3.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 330
#define SAMPLES 44
#define CYCLES 1.1
#define OFFSET -.05

#define LINE_WIDTH 14
#define COLOR .6,0,0,1

void write_frame(cairo_surface_t *surface, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch6_overlay-%04d.png",frameno);
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

void clear_surface(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  cairo_set_source_rgb(c,1.,1.,1.);
  cairo_paint(c);
  cairo_destroy(c);
}

void draw_overlay(cairo_surface_t *cs,float *data){
  int i;
  cairo_t *c = cairo_create(cs);

  cairo_set_source_rgba(c,COLOR);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,LINE_WIDTH);

  for(i=0;i<W;i++){
    double y = data[i]*WH+CR;
    if(i==0)
      cairo_move_to(c,i,y);
    else
      cairo_line_to(c,i,y);
  }

  cairo_stroke(c);
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);
  int period=SAMPLES;
  int coeffs = period/4;
  float *square_coeffs;
  float *square_phases;
  int square_coeffs_n;
  int i,j;
  double phase=OFFSET*2*M_PI - (M_PI*CYCLES/period);
  float waveform[W];
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

  for(i=0;i<W;i++){
    float acc=0.;
    int k;
    for(j=0,k=1;j<square_coeffs_n;j++,k+=2)
      acc += square_coeffs[j] *
        cos( square_phases[j] + k*phase);
    waveform[i]=acc;
    phase += 2*M_PI*CYCLES/W;
    if(phase>=2*M_PI)phase-=2*M_PI;
  }

  transparent_surface(cs);
  draw_overlay(cs,waveform);
  write_frame(cs,0);
  cairo_surface_destroy(cs);
  return 0;
}
