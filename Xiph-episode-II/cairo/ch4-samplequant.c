#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo/cairo.h>
#include <fftw3.h>
#include <squishyio/squishyio.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 200
#define SPACING 44
#define QUANT 6

#define LINE_WIDTH 6
#define WAVE_WIDTH 12

#define WAVEORIG .2,1,.2,.7
#define WAVENOISE 1,.3,.3,.8
#define WAVEQUANT 1,.7,0,.8
#define STEM .0,.0,.0,.3
#define LOLLI .0,.0,.0,1

#define QSTEM .8,.0,.0,.3
#define QLOLLI .8,.0,.0,1

#define LINE .6,.9,1.,1

#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

void write_frame(cairo_surface_t *surface, char *base){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch4_%s.png",base);
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
  int i=-SPACING;

  cairo_set_source_rgba(c,LINE);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width(c,2);

  for(;i<W+SPACING;i+=SPACING){
    cairo_move_to(c,i-8,CR-WH);
    cairo_line_to(c,i-8,CR+WH);
  }

  cairo_stroke(c);
  cairo_destroy(c);
}

void draw_quant(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  int i;

  cairo_set_source_rgba(c,LINE);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width(c,2);

  for(i=-QUANT;i<=QUANT;i++){
    float y = (float)WH*i/QUANT+CR;
    cairo_move_to(c,0,y);
    cairo_line_to(c,W,y);
  }

  cairo_stroke(c);
  cairo_destroy(c);
}

void draw_axis(cairo_surface_t *cs,int mark){
  int i;
  cairo_t *c = cairo_create(cs);
  cairo_text_extents_t extents;
  cairo_font_face_t *ff;
  double SW = 105;

  cairo_set_source_rgba(c,AXIS_COLOR);
  cairo_set_line_width(c,AXIS_LINE_WIDTH);
  cairo_move_to(c,0,CR);
  cairo_line_to(c,W,CR);
  cairo_move_to(c,W-SW/2,CR-SW/3);
  cairo_line_to(c,W,CR);
  cairo_line_to(c,W-SW/2,CR+SW/3);
  cairo_stroke(c);

  cairo_set_line_width(c,AXIS_LINE_WIDTH*.75);

  if(mark){
    for(i=1;i*SPACING<W;i++){
      double x = i*SPACING-8;
      cairo_move_to(c,x,CR-AXIS_LINE_WIDTH*2);
      cairo_line_to(c,x,CR+AXIS_LINE_WIDTH*2);
    }
    cairo_stroke(c);
  }

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

void draw_waveform(cairo_surface_t *cs, float *data, float r, float g, float b, float a){
  int i;
  cairo_t *c = cairo_create(cs);
  int first=0;
  cairo_set_source_rgba(c,r,g,b,a);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,WAVE_WIDTH);
  for(i=1;i-SPACING<W+LINE_WIDTH;i++){
    float x = i-SPACING-8;
    if(x-WAVE_WIDTH>0 && x+WAVE_WIDTH<W){
      if(!first){
        cairo_move_to(c,x,CR-data[i]*WH);
        first=1;
      }else{
        cairo_line_to(c,x,CR-data[i]*WH);
      }
    }
  }
  cairo_stroke(c);
  cairo_destroy(c);
}

void draw_samples(cairo_surface_t *cs, float *data){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=0;(i-1)*SPACING<=W+10;i++){
    int x = (i-1)*SPACING-8;
    double y = CR-WH*data[i*SPACING];

    if(x-LINE_WIDTH*2>0 && x+LINE_WIDTH*2<W){

      cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
      cairo_set_source_rgba(c,STEM);
      cairo_set_line_width(c,LINE_WIDTH*.75);
      cairo_move_to(c,x,CR);
      cairo_line_to(c,x,y);
      cairo_stroke(c);

      cairo_set_source_rgba(c,LOLLI*.5);
      cairo_set_line_width(c,LINE_WIDTH/3);
      cairo_arc(c,x,y,LINE_WIDTH*2,0,2*M_PI);
      cairo_stroke(c);

      cairo_set_source_rgba(c,LOLLI);
      cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
      cairo_set_line_width(c,LINE_WIDTH*2);
      cairo_line_to(c,x,y);
      cairo_line_to(c,x,y+.001);
      cairo_stroke(c);
    }
  }

  cairo_destroy(c);
}

void draw_qsamples(cairo_surface_t *cs, float *data, float *qdata){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=0;(i-1)*SPACING<=W+10;i++){
    int x = (i-1)*SPACING-8;
    double y1 = CR-WH*data[i*SPACING];
    double y2 = CR-WH*qdata[i*SPACING];

    if(x-LINE_WIDTH*2>0 && x+LINE_WIDTH*2<W){
      cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
      cairo_set_source_rgba(c,QSTEM);
      cairo_set_line_width(c,LINE_WIDTH*.75);
      cairo_move_to(c,x,y1);
      cairo_line_to(c,x,y2);
      cairo_stroke(c);

      cairo_set_source_rgba(c,QLOLLI*.5);
      cairo_set_line_width(c,LINE_WIDTH/3);
      cairo_arc(c,x,y2,LINE_WIDTH*2,0,2*M_PI);
      cairo_stroke(c);

      cairo_set_source_rgba(c,QLOLLI);
      cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
      cairo_set_line_width(c,LINE_WIDTH*2);
      cairo_line_to(c,x,y2);
      cairo_line_to(c,x,y2+.001);
      cairo_stroke(c);
    }
  }

  cairo_destroy(c);
}

/* cheap, dirty, circular, insufficient for high-fidelity, but this is just an illustration */
float *oversample_data(float *in,int n){
  int overn = n*SPACING*4;
  int i;

  float *data=calloc(overn+2,sizeof(*data));
  fftwf_plan plans = fftwf_plan_dft_r2c_1d(n*4,data,
                                           (fftwf_complex *)data,
                                           FFTW_ESTIMATE);
  fftwf_plan plani = fftwf_plan_dft_c2r_1d(overn,(fftwf_complex *)data,
                                           data,
                                           FFTW_ESTIMATE);

  memset(data,0,sizeof(*data)*(overn+2));
  memcpy(data,in,sizeof(*in)*n);

  fftwf_execute(plans);
  for(i=0;i<overn+2;i+=2){
    data[i]*=.25/n;
    data[i+1]*=.25/n;
  }
  fftwf_execute(plani);
  fftwf_destroy_plan(plani);
  fftwf_destroy_plan(plans);

  return data;
}

float *quantize_data(float *in,int n){
  int i;
  float *ret = calloc(n,sizeof(*ret));
  for(i=0;i<n;i++)
    ret[i] = rint(in[i]*QUANT)/QUANT;
  return ret;
}

void clear_surface(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  cairo_set_source_rgb(c,1.,1.,1.);
  cairo_paint(c);
  cairo_destroy(c);
}

int main(){
  int i;
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);

  pcm_t *wave = squishyio_load_file("../frames/ch4-short-sample.wav");
  if(wave->samples<((W+SPACING)/SPACING)){
    fprintf(stderr,"Not enough input samples\n");
    exit(1);
  }
  for(i=0;i<wave->samples;i++)
    wave->data[0][i] = wave->data[0][i] * 30.;

  /* oversample original wave into new storage */
  float *over = oversample_data(wave->data[0],wave->samples);

  /* quantize original wave into new storage */
  float *quant = quantize_data(wave->data[0],wave->samples);

  /* oversample quantized data into new storage */
  float *overquant = oversample_data(quant,wave->samples);

  /* compute noise */
  float *noise = calloc(W+SPACING*2,sizeof(float));
  for(i=0;i<W+SPACING*2;i++)
    noise[i] = overquant[i]-over[i];

  /* draw oversampled original waveform */
  clear_surface(cs);
  draw_axis(cs,0);
  draw_waveform(cs,over,WAVEORIG);
  write_frame(cs,"origcaption");

  /* draw oversampled original waveform with lines and samples */
  clear_surface(cs);
  draw_lines(cs);
  draw_axis(cs,1);
  draw_waveform(cs,over,WAVEORIG);
  draw_samples(cs,over);
  write_frame(cs,"sample");

  /* draw oversample with quantization lines */
  clear_surface(cs);
  draw_lines(cs);
  draw_quant(cs);
  draw_axis(cs,1);
  draw_waveform(cs,over,WAVEORIG);
  draw_samples(cs,over);
  write_frame(cs,"qlines");

  /* draw highlighted quantization samples */
  clear_surface(cs);
  draw_lines(cs);
  draw_quant(cs);
  draw_axis(cs,1);
  draw_waveform(cs,over,WAVEORIG);
  draw_samples(cs,over);
  draw_qsamples(cs,over,overquant);
  write_frame(cs,"hiquant");

  /* draw unhighlighted quantization samples+waveforms */
  clear_surface(cs);
  draw_lines(cs);
  draw_quant(cs);
  draw_axis(cs,1);
  draw_waveform(cs,over,WAVEORIG);
  draw_waveform(cs,overquant,WAVEQUANT);
  draw_samples(cs,overquant);
  write_frame(cs,"quant");

  /* draw all waveforms */
  clear_surface(cs);
  draw_lines(cs);
  draw_quant(cs);
  draw_axis(cs,1);
  draw_waveform(cs,over,WAVEORIG);
  draw_waveform(cs,overquant,WAVEQUANT);
  draw_samples(cs,overquant);
  draw_waveform(cs,noise,WAVENOISE);
  write_frame(cs,"noise");

  cairo_surface_destroy(cs);
  return 0;
}
