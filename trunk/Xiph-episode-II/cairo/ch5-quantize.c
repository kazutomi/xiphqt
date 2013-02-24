#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo/cairo.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 250
#define QUANT 4
#define CYCLES 1
#define SPACING 120

#define FRAMES 12

#define LINE_WIDTH 12

#define STEM .0,.0,.0,.3
#define LOLLI .0,.0,.0,1
#define ARROW .9,.0,.0,.1
#define LINE .6,.9,1.,1

#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

#define ARROW_LINE_WIDTH 6
#define ARROW_COLOR 1,0,0

void write_frame(cairo_surface_t *surface, char *base, int o){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch5_%s-%04d.png",base,o);
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

void draw_samples(cairo_surface_t *cs, float *data, float ldel){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=0;(i-1)*SPACING<=W+10;i++){
    int x = (i-1)*SPACING-8;
    double yf = CR-WH*data[i];
    double yi = CR-rint(data[i]*QUANT)/QUANT*WH;

    if(x-LINE_WIDTH*2>0 && x+LINE_WIDTH*2<W){
      float y;
      y = ldel<0 ? yf : ldel > 1. ? yi : ldel*(yi-yf)+yf;

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

void draw_targets(cairo_surface_t *cs, float *data, float adel){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=0;(i-1)*SPACING<=W+10;i++){
    int x = (i-1)*SPACING-8;
    //double yf = CR-WH*data[i];
    double yi = CR-rint(data[i]*QUANT)/QUANT*WH;
    //double yi2 = yi>yf ? CR-(rint(data[i]*QUANT)+1)/QUANT*WH : CR-(rint(data[i]*QUANT)-1)/QUANT*WH;

    if(x-LINE_WIDTH*2>0 && x+LINE_WIDTH*2<W){

      cairo_set_source_rgba(c,ARROW_COLOR,adel*.5);
      cairo_set_line_width(c,LINE_WIDTH/3);
      cairo_arc(c,x,yi,LINE_WIDTH*2,0,2*M_PI);
      cairo_stroke(c);

      cairo_set_source_rgba(c,ARROW_COLOR,adel);
      cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
      cairo_set_line_width(c,LINE_WIDTH*2);
      cairo_line_to(c,x,yi);
      cairo_line_to(c,x,yi+.001);
      cairo_stroke(c);

    }
  }
  cairo_destroy(c);
}

void draw_arrows(cairo_surface_t *cs, float *data, float ldel, float adel){
  int i;
  cairo_t *c = cairo_create(cs);

  /* draw lollipops */
  for(i=0;(i-1)*SPACING<=W+10;i++){
    int x = (i-1)*SPACING-8;
    double yf = CR-WH*data[i];
    double yi = CR-rint(data[i]*QUANT)/QUANT*WH;

    if(x-LINE_WIDTH*2>0 && x+LINE_WIDTH*2<W){
      float y;
      y = ldel<0 ? yf : ldel > 1. ? yi : ldel*(yi-yf)+yf;


      if(yi<yf){  /* down arrow */
        //if(yi > CR)
        //y -= LINE_WIDTH*10+ARROW_LINE_WIDTH*10;

        cairo_set_line_width(c,ARROW_LINE_WIDTH+3);
        cairo_set_source_rgba(c,1.,1.,1.,adel);
        cairo_move_to(c,x,y+LINE_WIDTH*5);
        cairo_line_to(c,x,y+LINE_WIDTH*5+ARROW_LINE_WIDTH*10+1.5);

        cairo_move_to(c,x-ARROW_LINE_WIDTH*3,
                      y+LINE_WIDTH*5+ARROW_LINE_WIDTH*4);
        cairo_line_to(c,x,y+LINE_WIDTH*5);
        cairo_line_to(c,x+ARROW_LINE_WIDTH*3,
                      y+LINE_WIDTH*5+ARROW_LINE_WIDTH*4);


        cairo_stroke(c);

        cairo_set_line_width(c,ARROW_LINE_WIDTH);
        cairo_set_source_rgba(c,ARROW_COLOR,adel);
        cairo_move_to(c,x,y+LINE_WIDTH*5);
        cairo_line_to(c,x,y+LINE_WIDTH*5+ARROW_LINE_WIDTH*10);

        cairo_move_to(c,x-ARROW_LINE_WIDTH*3,
                      y+LINE_WIDTH*5+ARROW_LINE_WIDTH*4);
        cairo_line_to(c,x,y+LINE_WIDTH*5);
        cairo_line_to(c,x+ARROW_LINE_WIDTH*3,
                      y+LINE_WIDTH*5+ARROW_LINE_WIDTH*4);

        cairo_stroke(c);


      }
      if(yi>yf){
        //if(yi < CR )
        //y += LINE_WIDTH*10+ARROW_LINE_WIDTH*10;

        cairo_set_line_width(c,ARROW_LINE_WIDTH+3);
        cairo_set_source_rgba(c,1.,1.,1.,adel);
        cairo_move_to(c,x,y-LINE_WIDTH*5);
        cairo_line_to(c,x,y-LINE_WIDTH*5-ARROW_LINE_WIDTH*10-1.5);

        cairo_move_to(c,x-ARROW_LINE_WIDTH*3,
                      y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);
        cairo_line_to(c,x,y-LINE_WIDTH*5);
        cairo_line_to(c,x+ARROW_LINE_WIDTH*3,
                      y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);


        cairo_stroke(c);

        cairo_set_line_width(c,ARROW_LINE_WIDTH);
        cairo_set_source_rgba(c,ARROW_COLOR,adel);
        cairo_move_to(c,x,y-LINE_WIDTH*5);
        cairo_line_to(c,x,y-LINE_WIDTH*5-ARROW_LINE_WIDTH*10);

        cairo_move_to(c,x-ARROW_LINE_WIDTH*3,
                      y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);
        cairo_line_to(c,x,y-LINE_WIDTH*5);
        cairo_line_to(c,x+ARROW_LINE_WIDTH*3,
                      y-LINE_WIDTH*5-ARROW_LINE_WIDTH*4);

        cairo_stroke(c);

      }
    }
  }

  cairo_destroy(c);
}

int main(){
  int i,count=0;
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);

  int s=(W+SPACING-1)/SPACING+1;
  float *data = calloc((W+SPACING-1)/SPACING,sizeof(*data));
  for(i=0;i<s;i++)
    data[i] = -sin((i-.35)/s*2*M_PI)*.9;

  clear_surface(cs);
  draw_quant(cs);
  draw_axis(cs,0);
  draw_samples(cs,data,0);
  write_frame(cs,"quant",count++);

  /* fade in lines */
  for(i=0;i<FRAMES;i++){
    clear_surface(cs);
    draw_quant(cs);
    draw_axis(cs,0);
    draw_targets(cs,data,(float)i/FRAMES);
    draw_samples(cs,data,0);
    write_frame(cs,"quant",count++);
  }

  /* fade in arrows */
  for(i=0;i<FRAMES;i++){
    clear_surface(cs);
    draw_quant(cs);
    draw_axis(cs,0);
    draw_targets(cs,data,1);
    draw_samples(cs,data,0);
    draw_arrows(cs,data,0,(float)i/FRAMES);
    write_frame(cs,"quant",count++);
  }

  /* move */
  for(i=0;i<FRAMES;i++){
    clear_surface(cs);
    draw_quant(cs);
    draw_axis(cs,0);
    draw_targets(cs,data,1);
    draw_samples(cs,data,(float)i/FRAMES);
    draw_arrows(cs,data,(float)i/FRAMES,1);
    write_frame(cs,"quant",count++);
  }

  /* fade out all */
  for(i=0;i<FRAMES;i++){
    clear_surface(cs);
    draw_quant(cs);
    draw_axis(cs,0);
    draw_targets(cs,data,1.-(float)i/FRAMES);
    draw_samples(cs,data,1);
    draw_arrows(cs,data,1,1-(float)i/FRAMES);
    write_frame(cs,"quant",count++);
  }

  clear_surface(cs);
  draw_quant(cs);
  draw_axis(cs,0);
  draw_samples(cs,data,1);
  write_frame(cs,"quant",count++);

  cairo_surface_destroy(cs);
  return 0;
}
