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
#define TEXT_FONT_SIZE 90

#define SAMPLES 18
#define CYCLES  1

#define PRE     96
#define FRAMES  128
#define POST    48
#define FADE          24
#define FADEMARGIN    2

#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

#define FIRST_LINE_WIDTH 15
#define FIRST_COLOR .6,.6,.6,1

#define SECOND_LINE_WIDTH 15
#define SECOND_COLOR .8,.4,.4,1
#define HALO_TAIL 60
#define HALO 15

void write_frame(cairo_surface_t *surface, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch3_stairstep-%04d.png",frameno);
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
  return CR + rint(WH*sin((double)x/W*CYCLES*2*M_PI));
}

int tox(int sample){
  double SW = (double)W/SAMPLES;
  return rint(sample*SW);
}

void draw_axis(cairo_surface_t *cs){
  int i;
  cairo_t *c = cairo_create(cs);
  double SW = 105;
  cairo_text_extents_t extents;
  cairo_font_face_t *ff;

  cairo_set_source_rgba(c,AXIS_COLOR);
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

double sadj(double x){
  if(x<.2)
    return sin(x*2.5*M_PI);
  if(x>.8)
    return sin((1-x)*2.5*M_PI);
  return 1.;
}

void draw_stairstep_trace(cairo_surface_t *cs,int frame){
int i,p,flag=0;
  double sx,y,y1;
  cairo_t *c = cairo_create(cs);
  double SW = (double)W/SAMPLES;

  /* draw trace and halo in successive passes */
  cairo_set_source_rgba(c,FIRST_COLOR);
  cairo_set_line_width(c,FIRST_LINE_WIDTH);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);

  for(i=frame * W / FRAMES;i<W;i++){
    if(i>=0){
      sx = floor((double)i / SW) * SW;
      y1 = get_y(sx);
      if(!flag){
        y=y1;
        cairo_move_to(c,i,y);
        flag=1;
      }
      cairo_line_to(c,i,y);
      if(i>=W)break;
      if(y!=y1){
        cairo_stroke(c);
        cairo_set_source_rgba(c,FIRST_COLOR*.5);
        cairo_move_to(c,i,y);
        cairo_line_to(c,i,y1);
        cairo_stroke(c);
        cairo_set_source_rgba(c,FIRST_COLOR);
      }
      y=y1;
    }
  }
  cairo_stroke(c);

  cairo_set_source_rgba(c,SECOND_COLOR);
  cairo_set_line_width(c,SECOND_LINE_WIDTH);

  for(i=0;i<frame * W / FRAMES;i++){
    sx = floor((double)i / SW) * SW;
    y1 = get_y(sx);
    if(!i){
      y=y1;
      cairo_move_to(c,i,y);
    }
    cairo_line_to(c,i,y);
    if(i>=W)break;
    if(y!=y1)
      cairo_line_to(c,i,y1);
    y=y1;
  }
  cairo_stroke(c);

  /* halos */
  cairo_save(c);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);

  for(p=HALO_TAIL;p>=0;p--){

    int start = (double)(frame-HALO_TAIL+p) * W / FRAMES;
    int end = (double)frame * W / FRAMES;

    for(i=start;i<=end;i++){
      double y0 = get_y(floor((double)(i-1) / SW) * SW);
      double y1 = get_y(floor((double)i / SW) * SW);
      double fp = 1.-(end-i)/(double)(end-start);

      if(p==0){
        cairo_set_source_rgba(c,1,.2,.1,.25*fp*fp);
      }else{
        double dp = (HALO_TAIL-p+1.)/(HALO_TAIL+1);
        cairo_set_source_rgba(c,1,.2,.1,.003*fp*fp+.01*dp*fp);
      }

      if(i>0){
        if(p==0)
          cairo_set_line_width(c,SECOND_LINE_WIDTH);
        else
          cairo_set_line_width(c,SECOND_LINE_WIDTH+2+HALO*
                               ((double)(p+3)/(HALO_TAIL+1))*
                               ((double)(p+3)/(HALO_TAIL+1)));


        cairo_move_to(c,i-1,y0);
        cairo_line_to(c,i,y0);
        if(i<W && y0 != y1){
          int j;
          if(y0>y1){
            int t = y0;
            y0 = (int)y1;
            y1 = t;
          }
          for(j=y0+1;j<y1;j++){
            double adj = sadj((j-y0)/(y1-y0));
            adj = 1.- .5*adj;

            cairo_stroke(c);

            if(p==0)
              cairo_set_line_width(c,SECOND_LINE_WIDTH*adj);
            else
              cairo_set_line_width(c,(SECOND_LINE_WIDTH+2+HALO*
                                      ((double)(p+3)/(HALO_TAIL+1))*
                                      ((double)(p+3)/(HALO_TAIL+1)))*adj);

            if(p==0){
              cairo_set_source_rgba(c,1,.2,.1,.25*fp*fp*adj);
            }else{
              double dp = (HALO_TAIL-p+1.)/(HALO_TAIL+1);
              cairo_set_source_rgba(c,1,.2,.1,(.003*fp*fp+.01*dp*fp)*adj);
            }

            cairo_move_to(c,i,j-1);
            cairo_line_to(c,i,j);
          }
        }
        cairo_stroke(c);
      }

      if(i>=W)break;
    }
  }
  cairo_restore(c);

  cairo_destroy(c);
}

void draw_cursor(cairo_surface_t *cs,int frame, double alpha){
  char buffer[80];
  double SW = (double)W/SAMPLES;
  int x = (double)frame * W / FRAMES;
  int y = get_y(floor((double)x / SW + .01) * SW);

  cairo_t *c = cairo_create(cs);
  cairo_font_face_t *ff,*ft;
  cairo_text_extents_t extents;
  cairo_text_extents_t extents2;
  cairo_text_extents_t extents3;
  cairo_text_extents_t extents4;

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
  cairo_text_extents(c, "time: ", &extents);
  cairo_text_extents(c, "value: ", &extents3);
  cairo_set_font_face(c,ff);
  cairo_text_extents(c, "00.00  ", &extents2);
  cairo_text_extents(c, "-100", &extents4);

  double TH = (TEXTY-extents.height)*2 + extents.height;

  cairo_set_line_width(c,2);

  cairo_set_source_rgba(c,.8,0,0,alpha);
  cairo_move_to(c,x,TH);
  cairo_line_to(c,x,H-TH);
  cairo_stroke(c);

#if 0
  cairo_set_source_rgba(c,.9,.9,.9,alpha);
  cairo_rectangle(c,0,0,W,TH);
  cairo_fill_preserve(c);
  cairo_set_source_rgba(c,.5,.5,.5,alpha);
  cairo_stroke(c);

  cairo_set_source_rgba(c,.9,.9,.9,alpha);
  cairo_rectangle(c,0,H-TH,W,TH);
  cairo_fill_preserve(c);
  cairo_set_source_rgba(c,.5,.5,.5,alpha);
  cairo_stroke(c);
#endif

  cairo_move_to(c, (W-extents.x_advance-extents2.x_advance-
                    extents3.x_advance-extents4.x_advance)/2, TEXTY);

  cairo_set_source_rgba(c,0,0,0,alpha);
  cairo_set_font_face(c,ft);
  cairo_show_text(c, "time: ");
  snprintf(buffer, 80,"%2.2f  ",
           (double)frame/FRAMES*SAMPLES);
  cairo_set_source_rgba(c,.8,0,0,alpha);
  cairo_set_font_face(c,ff);
  cairo_show_text(c, buffer);

  cairo_move_to(c, (W+extents.x_advance+extents2.x_advance-
                    extents3.x_advance-extents4.x_advance)/2, TEXTY);

  cairo_set_source_rgba(c,0,0,0,alpha);
  cairo_set_font_face(c,ft);
  cairo_show_text(c, "value: ");
  snprintf(buffer, 80,"%d",(int)rint((CR-(int)y)/2.4));
  cairo_set_source_rgba(c,.8,0,0,alpha);
  cairo_set_font_face(c,ff);
  cairo_show_text(c, buffer);

  cairo_font_face_destroy(ft);
  cairo_font_face_destroy(ff);
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						    W,H);

  int i;

  /* static preroll */
  for(i=-PRE;i<FADEMARGIN;i++){
    clear_surface(cs);
    draw_axis(cs);
    draw_stairstep_trace(cs,i);
    write_frame(cs,i+PRE);
  }

  /* fade in */
  for(;i<FADE+FADEMARGIN;i++){
    clear_surface(cs);
    draw_axis(cs);
    draw_cursor(cs,i,(double)(i-FADEMARGIN)/FADE);
    draw_stairstep_trace(cs,i);
    write_frame(cs,i+PRE);
  }

  /* normal */
  for(;i<FRAMES-FADE-FADEMARGIN;i++){
    clear_surface(cs);
    draw_axis(cs);
    draw_cursor(cs,i,1.);
    draw_stairstep_trace(cs,i);
    write_frame(cs,i+PRE);
  }

  /* fade out */
  for(;i<FRAMES-FADEMARGIN;i++){
    clear_surface(cs);
    draw_axis(cs);
    draw_cursor(cs,i,(double)(FRAMES-i-FADEMARGIN)/FADE);
    draw_stairstep_trace(cs,i);
    write_frame(cs,i+PRE);
  }

  /* static outro */
  for(;i<FRAMES+POST;i++){
    clear_surface(cs);
    draw_axis(cs);
    draw_stairstep_trace(cs,i);
    write_frame(cs,i+PRE);
  }

  cairo_surface_destroy(cs);
  return 0;
}
