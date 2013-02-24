#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>

#define W 1920
#define H 1080
#define CR 300
#define WH 120
#define CYCLES 3.1
#define SAMPLES (3.1*4)
#define OFFSET .2

#define TEXTY 750
#define TEXTSIZE 80

/*
-1  0 < t mod T <= .5T
  1  .5T < t mod T <= T
*/


#define AXIS_LINE_WIDTH 6
#define AXIS_COLOR 0,0,0,1
#define AXIS_FONT_SIZE 54

#define LINE_WIDTH 15
#define COLOR .4,.4,.4,1

#define ARROW_LINE_WIDTH 6
#define ARROW_COLOR 1,0,0

#define LINE_WIDTH 15

#define FIRST_COLOR .6,.6,.6,1
#define SECOND_COLOR .7,.0,.0,1

void write_frame(cairo_surface_t *surface, int frameno){
  char buffer[80];
  cairo_status_t ret;

  snprintf(buffer,80,"ch6_squaretime-%04d.png",frameno);
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

void draw_axis(cairo_surface_t *cs){
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

  for(i=-2;i<SAMPLES;i++){
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

double sign(double x){
  if(x<0)return -1;
  if(x>0)return 1;
  return 0;
}

double get_y(double x){
  float v = sin(((double)x/W*CYCLES+OFFSET)*2*M_PI);
  return CR + WH*sign(v);
}

int tosample(double x){
  double SW = (double)W/SAMPLES;
  return rint(x/SW);
}

int tox(int sample){
  double SW = (double)W/SAMPLES;
  return rint((sample-OFFSET*SAMPLES/CYCLES)*SW);
}

void draw_waveform(cairo_surface_t *cs,int p){
  int i;
  cairo_t *c = cairo_create(cs);
  double y0;

  cairo_set_source_rgba(c,FIRST_COLOR);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,LINE_WIDTH);

  for(i=-LINE_WIDTH;i<W+LINE_WIDTH;i++){
    double y = get_y(i);
    if(i==-LINE_WIDTH){
    }else{
      cairo_move_to(c,i,y0);
      if(y!=y0){
        cairo_set_source_rgba(c,FIRST_COLOR*.5);
      }else{
        if( (y<CR && p==1) || (y>CR && p==2) )
          cairo_set_source_rgba(c,SECOND_COLOR*.5);
        else
          cairo_set_source_rgba(c,FIRST_COLOR*.5);
      }
      cairo_line_to(c,i,y);
      cairo_stroke(c);
    }
    y0=y;
  }

  cairo_destroy(c);
}

#define MAX(x,y) ((x)>(y)?(x):(y))
void draw_formula(cairo_surface_t *cs,int p){
  cairo_text_extents_t extents;
  cairo_text_extents_t extentsB;
  cairo_text_extents_t extentsT;
  cairo_font_face_t *fi,*ff,*fb;
  cairo_t *c = cairo_create(cs);

  ff = cairo_toy_font_face_create ("Liberation Sans",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  fi = cairo_toy_font_face_create ("Liberation Serif",
                                   CAIRO_FONT_SLANT_OBLIQUE,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  fb = cairo_toy_font_face_create ("Adobe Garamond",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  if(!ff){
    fprintf(stderr,"Unable to create formula font");
    exit(1);
  }
  if(!fi){
    fprintf(stderr,"Unable to create formula font");
    exit(1);
  }

  int th=0,tw=0,bw=0,bh=0,w5=0,wT;

  cairo_set_font_face(c,fi);
  cairo_set_font_size(c, TEXTSIZE);

  cairo_text_extents(c, "y(t) = ", &extents);
  tw += extents.x_advance;
  cairo_text_extents(c, " t (mod T)", &extents);
  tw += extents.x_advance;

  cairo_text_extents(c, "T", &extentsT);
  wT = extentsT.x_advance;
  th = extentsT.height;

  cairo_set_font_face(c,ff);
  cairo_text_extents(c, " +1,  ", &extents);
  cairo_text_extents(c, " -1,  ", &extentsB);

  tw += MAX(extents.x_advance, extentsB.x_advance);

  cairo_set_font_size(c, TEXTSIZE/2);
  cairo_text_extents(c, "\xE2\x80\x94", &extents);
  w5 = extents.x_advance + extentsT.x_advance;
  tw += w5+w5;

  cairo_set_font_size(c, TEXTSIZE);
  cairo_text_extents(c, " \x3C", &extents);
  tw += extents.x_advance;
  cairo_text_extents(c, " \xE2\x89\xA4 ", &extents);
  tw += extents.x_advance;

  cairo_set_font_face(c,fb);
  cairo_set_font_size(c, TEXTSIZE*4);
  cairo_text_extents(c, "{", &extents);
  tw += extents.x_advance;

  cairo_set_font_face(c,fi);
  cairo_set_font_size(c, TEXTSIZE);
  cairo_set_source_rgba(c,0,0,0,1);

  int x = W/2-(tw/2)*1.05;
  int y = (th*1.5);
  char *text;

  cairo_set_source_rgba(c,0,0,0,1);
  text="y(t) = ";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c, x, TEXTY-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_set_font_face(c,fb);
  cairo_set_font_size(c, TEXTSIZE*4);
  text="{";
  cairo_text_extents(c, text, &extents);
  int d = extents.height+extents.y_bearing;

  cairo_move_to(c,x,TEXTY+extents.height/2-d);
  cairo_show_text(c,text);
  x += extents.x_advance;

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  }
  cairo_set_font_face(c,ff);
  cairo_set_font_size(c, TEXTSIZE);
  text=" +1,  ";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,TEXTY-y-extents.y_bearing/2);
  cairo_show_text(c,text);

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
   cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  }
  text=" -1,  ";
  cairo_text_extents(c, text, &extentsB);
  cairo_move_to(c,x+(extents.x_advance-extentsB.x_advance),TEXTY+y-extentsB.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;


  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  }
  text="0";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - extents.x_advance)/2,TEXTY-y-extents.y_bearing/2);
  cairo_show_text(c,text);

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
   cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  }
  cairo_set_font_size(c, TEXTSIZE/2);
  text="1";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT - extents.x_advance)/2,TEXTY+y-th*.5-extents.y_bearing/2);
  cairo_show_text(c,text);

  text="\xE2\x80\x94";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT - extents.x_advance)/2,TEXTY+y-extents.y_bearing);
  cairo_show_text(c,text);

  text="2";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT - extents.x_advance)/2,TEXTY+y+th*.5-extents.y_bearing/2);
  cairo_show_text(c,text);

  cairo_set_font_size(c, TEXTSIZE);
  cairo_set_font_face(c,fi);
  text="T";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT),TEXTY+y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += w5;

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  }
  cairo_set_font_face(c,ff);
  text=" \x3C";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,TEXTY-y-extents.y_bearing/2);
  cairo_show_text(c,text);
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,TEXTY+y-extents.y_bearing/2);
  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
   cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  }
  cairo_show_text(c,text);
  x += extents.x_advance;

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  }
  cairo_set_font_face(c,fi);
  text=" t (mod T)";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,TEXTY-y-extents.y_bearing/2);
  cairo_show_text(c,text);
  cairo_text_extents(c, text, &extents);
  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
   cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  }
  cairo_move_to(c,x,TEXTY+y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  }
  cairo_set_font_face(c,ff);
  text=" \xE2\x89\xA4 ";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,TEXTY-y-extents.y_bearing/2);
  cairo_show_text(c,text);
  cairo_text_extents(c, text, &extents);
  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
   cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  }
  cairo_move_to(c,x,TEXTY+y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;


  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
   cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  }
  cairo_set_font_face(c,fi);
  text="T";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - extents.x_advance)/2,TEXTY+y-extents.y_bearing/2);
  cairo_show_text(c,text);

  switch(p){
  case 0:
    cairo_set_source_rgba(c,0,0,0,1);
    break;
  case 1:
    cairo_set_source_rgba(c,SECOND_COLOR);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR);
    break;
  }
  cairo_set_font_face(c,ff);
  cairo_set_font_size(c, TEXTSIZE/2);
  text="1";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT - extents.x_advance)/2,TEXTY-y-th*.5-extents.y_bearing/2);
  cairo_show_text(c,text);

  text="\xE2\x80\x94";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT - extents.x_advance)/2,TEXTY-y-extents.y_bearing);
  cairo_show_text(c,text);

  text="2";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT - extents.x_advance)/2,TEXTY-y+th*.5-extents.y_bearing/2);
  cairo_show_text(c,text);

  cairo_set_font_size(c, TEXTSIZE);
  cairo_set_font_face(c,fi);
  text="T";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (w5 - wT),TEXTY-y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += w5;

  cairo_font_face_destroy(ff);
  cairo_font_face_destroy(fi);
  cairo_font_face_destroy(fb);
}


int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						    W,H);

  clear_surface(cs);
  draw_waveform(cs,0);
  draw_axis(cs);
  draw_formula(cs,0);
  write_frame(cs,0);

  clear_surface(cs);
  draw_waveform(cs,1);
  draw_axis(cs);
  draw_formula(cs,1);
  write_frame(cs,1);

  clear_surface(cs);
  draw_waveform(cs,2);
  draw_axis(cs);
  draw_formula(cs,2);
  write_frame(cs,2);

  cairo_surface_destroy(cs);
  return 0;
}
