#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 300

#define TEXTY 140
#define TEXTY2 140
#define TEXT_FONT_SIZE 90

#define SAMPLES 18
#define CYCLES  1

#define PRE        128
#define FADE       24
#define OFFSET     1
#define ARROW      64
#define ZERO_FADE  24
#define ZERO       256


#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

#define FIRST_LINE_WIDTH 15
#define FIRST_COLOR .6,.6,.6,1

#define SECOND_LINE_WIDTH 15
#define SECOND_COLOR .8,.4,.4,1
#define HALO_TAIL 10
#define HALO 15


#define LINE_WIDTH 15
#define COLOR .4,.4,.4,1

#define ARROW_LINE_WIDTH 6
#define ARROW_COLOR 1,0,0


void write_frame(cairo_surface_t *surface, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch3_zero-%04d.png",frameno);
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

double get_y(double x){
  return CR + WH*sin((double)x/W*CYCLES*2*M_PI);
}

int tosample(double x){
  double SW = (double)W/SAMPLES;
  return floor(x/SW+.001);
}

int tox(int sample){
  double SW = (double)W/SAMPLES;
  return ceil(sample*SW);
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

void draw_stems(cairo_surface_t *cs, double del, int frame){
  int i;
  cairo_t *c = cairo_create(cs);
  /* draw lollipops */

  cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
  for(i=1;i<SAMPLES;i++){
    int sx = tox(i);
    double y = get_y(sx);
    double y0 = get_y(tox(i-1));

    double frameoff = frame - (i*OFFSET) - ((ARROW-SAMPLES*OFFSET)/2);
    double dp = 1.-frameoff/((ARROW-SAMPLES*OFFSET)/2);
    if(dp<0)dp=0;
    if(dp>1)dp=1;

    cairo_set_line_cap(c,CAIRO_LINE_CAP_BUTT);
    cairo_set_source_rgba(c,COLOR*del*(.5+dp*.5)*.8);
    cairo_set_line_width(c,LINE_WIDTH*.75);
    cairo_move_to(c,sx,CR);
    cairo_line_to(c,sx,y);
    cairo_stroke(c);


    cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(c,FIRST_COLOR*(1-dp)*del);
    cairo_set_line_width(c,FIRST_LINE_WIDTH);
    cairo_move_to(c,sx,y0);
    cairo_line_to(c,sx,y);
    cairo_stroke(c);



  }

  cairo_destroy(c);
}

void draw_samples(cairo_surface_t *cs, double del){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=1;i<SAMPLES;i++){
    int sx = tox(i);
    double y = get_y(sx);

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

void draw_stairstep_full(cairo_surface_t *cs,double del){
  int i,flag=0;
  double y;
  cairo_t *c = cairo_create(cs);

  cairo_set_source_rgba(c,FIRST_COLOR*del);
  cairo_set_line_width(c,FIRST_LINE_WIDTH);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);

  for(i=0;i<W;i++){
    double s = tosample(i);
    double sx = tox(s);
    double y1 = get_y(sx);
    if(!flag){
      y=y1;
      cairo_move_to(c,i,y);
      flag=1;
    }
    cairo_line_to(c,i,y);
    if(i>=W)break;
    if(y!=y1)
      cairo_line_to(c,i,y1);
    y=y1;
  }
  cairo_stroke(c);
  cairo_destroy(c);
}

void draw_arrows(cairo_surface_t *cs,double del,int frame){
  int i,p,s;
  cairo_t *c = cairo_create(cs);

  /* draw trace and halo in successive passes */
  cairo_set_line_join(c,CAIRO_LINE_JOIN_MITER);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);


  for(s=0;s<SAMPLES;s++){
    int sx = tox(s);
    double end = tox(s+1);
    int y = get_y(sx);

    if(end>sx){
      /* sample highlight */
      cairo_set_source_rgba(c,.8,0,0,del);
      cairo_set_line_width(c,LINE_WIDTH*2);
      cairo_line_to(c,sx,y);
      cairo_line_to(c,sx,y+.001);
      cairo_stroke(c);
    }
  }

  /* trace */
  for(s=SAMPLES-1;s>=0;s--){
    int sx = tox(s);
    double end = tox(s+1);
    int y = get_y(sx);
    double frameoff = frame - (s*OFFSET);
    double dp = frameoff/(ARROW-(SAMPLES*OFFSET));
    if(dp<0)dp=0;
    if(dp>1)dp=1;
    dp = sin(dp*M_PI/2);
    dp = dp*dp;
    dp = sin(dp*M_PI/2);
    dp = dp*dp;
    end = (end-sx)*dp+sx;

    if(end>sx){

#if 0
      /* trace background */
      cairo_set_line_width(c,LINE_WIDTH);
      cairo_set_source_rgba(c,SECOND_COLOR*del*dp);
      cairo_move_to(c,sx,y);
      cairo_line_to(c,end,y);
      cairo_stroke(c);

      /* sample highlight */
      cairo_set_source_rgba(c,.8,0,0,del);
      cairo_set_line_width(c,LINE_WIDTH*2);
      cairo_line_to(c,sx,y);
      cairo_line_to(c,sx,y+.001);
      cairo_stroke(c);

      for(p=HALO_TAIL;p>=0;p--){
        double sdp = (frameoff-HALO_TAIL+p)/(ARROW-(SAMPLES*OFFSET));
        if(sdp>=0 && sdp<=1){
          sdp = sin(sdp*M_PI/2);
          sdp = sdp*sdp;
          sdp = sin(sdp*M_PI/2);
          sdp = sdp*sdp;
          double start = (tox(s+1)-sx)*sdp+sx;

          for(i=start;i<=end;i++){
            double fp = 1.-(end-i)/(double)(end-start+1);

            if(p==0){
              cairo_set_source_rgba(c,1,.2,.1,.25*fp*fp*del);
              cairo_set_line_width(c,SECOND_LINE_WIDTH);
            }else{
              double dp = (HALO_TAIL-p+1.)/(HALO_TAIL+1);
              cairo_set_source_rgba(c,1,.2,.1,(.003*fp*fp+.01*dp*fp)*del);
              cairo_set_line_width(c,SECOND_LINE_WIDTH+2+HALO*
                                   ((double)(p+3)/(HALO_TAIL+1))*
                                   ((double)(p+3)/(HALO_TAIL+1)));
            }

            /* squarewave halo */
            cairo_move_to(c,i-1,y);
            cairo_line_to(c,i,y);
            cairo_stroke(c);
            if(i>=W)break;
          }
        }
      }
#endif

      /* arrow_clear */
      double ddp = (double)(end-sx)/(ARROW_LINE_WIDTH*5);
      if(ddp<0)ddp=0;
      if(ddp>1.)ddp=1.;
      cairo_set_line_width(c,SECOND_LINE_WIDTH+4);
      cairo_set_source_rgba(c,1,1,1,1*del*ddp);

      cairo_move_to(c,sx,y);
      cairo_line_to(c,end-SECOND_LINE_WIDTH*.75,y);
      cairo_stroke(c);
      cairo_set_line_width(c,ARROW_LINE_WIDTH+4);
      cairo_move_to(c,end-ARROW_LINE_WIDTH*5,y + ARROW_LINE_WIDTH*3);
      cairo_line_to(c,end,y);
      cairo_line_to(c,end-ARROW_LINE_WIDTH*5,y - ARROW_LINE_WIDTH*3);
      cairo_stroke(c);

      /* arrow */
      cairo_set_line_width(c,SECOND_LINE_WIDTH);
      cairo_set_source_rgba(c,.8,0,0,1*del*ddp);

      cairo_move_to(c,sx,y);
      cairo_line_to(c,end-SECOND_LINE_WIDTH*.75,y);
      cairo_stroke(c);
      cairo_set_line_width(c,ARROW_LINE_WIDTH);
      cairo_move_to(c,end-ARROW_LINE_WIDTH*5,y + ARROW_LINE_WIDTH*3);
      cairo_line_to(c,end,y);
      cairo_line_to(c,end-ARROW_LINE_WIDTH*5,y - ARROW_LINE_WIDTH*3);
      cairo_stroke(c);
    }
  }

  cairo_destroy(c);
}

void draw_readout(cairo_surface_t *cs, double alpha){
  cairo_t *c = cairo_create(cs);
  cairo_font_face_t *ff;
  cairo_text_extents_t extents;
  ff = cairo_toy_font_face_create ("Adobe Garamond",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  if(!ff){
    fprintf(stderr,"Unable to create cursor font");
    exit(1);
  }
  cairo_set_font_face(c,ff);
  cairo_set_font_size(c, TEXT_FONT_SIZE);
  cairo_text_extents(c, "\xE2\x80\x9CZero-order hold\xE2\x80\x9D", &extents);
  double TH = (TEXTY-extents.height)*2 + extents.height;

  cairo_move_to(c, (W-extents.width)/2, TEXTY);

  cairo_set_source_rgba(c,0,0,0,alpha);
  cairo_show_text(c, "\xE2\x80\x9CZero-order hold\xE2\x80\x9D");

  cairo_font_face_destroy(ff);

  cairo_destroy(c);
}

void draw_alignment(cairo_surface_t *cs,double del){
  int i;
  cairo_t *c = cairo_create(cs);
  cairo_font_face_t *ff;
  cairo_text_extents_t extents;

  ff = cairo_toy_font_face_create ("Adobe Garamond",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  if(!ff){
    fprintf(stderr,"Unable to create cursor font");
    exit(1);
  }
  cairo_set_font_face(c,ff);
  cairo_set_font_size(c, TEXT_FONT_SIZE);
  cairo_text_extents(c, "\xE2\x80\x9CZero-order hold\xE2\x80\x9D", &extents);
  double TH = (TEXTY-extents.height)*2 + extents.height;

  cairo_set_source_rgba(c,.6,.8,1,del);
  cairo_set_line_width(c,2);

  for(i=0;i<=SAMPLES;i++){
    double x = tox(i);
    cairo_move_to(c,x,TH);
    cairo_line_to(c,x,H-TH);
  }
  cairo_stroke(c);

  cairo_font_face_destroy(ff);
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						    W,H);
  int i,count=0;

  /* static preroll */
  for(i=0;i<PRE;i++){
    clear_surface(cs);
    draw_stems(cs,1.,0);
    draw_axis(cs,1.);
    draw_samples(cs,1.);
    write_frame(cs,count++);
  }

  /* fade in alignment lines */
  for(i=0;i<FADE;i++){
    clear_surface(cs);
    draw_alignment(cs,(double)i/FADE);
    draw_stems(cs,1.,0);
    draw_axis(cs,1.);
    draw_samples(cs,1.);
    //draw_readout(cs,(double)i/FADE);
    write_frame(cs,count++);
  }

  /* arrows */
  for(i=0;i<ARROW;i++){
    clear_surface(cs);
    draw_alignment(cs,1.);
    draw_stems(cs,1.,i);
    draw_axis(cs,1.);
    draw_samples(cs,1.);
    draw_arrows(cs,1.,i);
    //draw_readout(cs,1.);
    write_frame(cs,count++);
  }

  /* fade in zero-hold, */
  for(i=0;i<ZERO_FADE;i++){
    clear_surface(cs);
    draw_alignment(cs,1.);
    draw_stems(cs,1.,i+ARROW);
    draw_axis(cs,1.);
    draw_samples(cs,1.);
    draw_stairstep_full(cs,(double)i/ZERO_FADE);
    draw_arrows(cs,1.,ARROW+i);
    draw_readout(cs,(double)i/ZERO_FADE);
    write_frame(cs,count++);
  }

 /* zero-hold */
  for(i=0;i<ZERO;i++){
    clear_surface(cs);
    draw_alignment(cs,1.);
    draw_stems(cs,1.,i+ARROW);
    draw_axis(cs,1.);
    draw_samples(cs,1.);
    draw_stairstep_full(cs,1.);
    draw_arrows(cs,1.,ARROW+i);
    draw_readout(cs,1.);
    write_frame(cs,count++);
  }

  cairo_surface_destroy(cs);
  return 0;
}
