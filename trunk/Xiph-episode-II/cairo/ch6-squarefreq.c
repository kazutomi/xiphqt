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

#define TEXT0 750
#define TEXTY 600
#define TEXTH 500
#define XMARGINL 90
#define XMARGINR 60
#define TEXTSIZE 60
#define REDUCA .8
#define REDUCB .78

#define OVER 20
#define YUP (24*4)

#define PRE 240
#define FADE 12
#define FUNDAMENTAL 33
#define PLUS (1./12)
#define ACCEL 1.00
#define ACCELB .0005
#define APLUS 120
#define APLUSPLUS 128
#define POST 32

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

  snprintf(buffer,80,"ch6_squarefreq-%04d.png",frameno);
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


double memo[(W+LINE_WIDTH*2)*OVER];
static int lastmemo=-1;
void compute_ys(int p){
  int i,j;
  if(lastmemo==-1 || p<lastmemo){
    for(j=-LINE_WIDTH*OVER;j<(W+LINE_WIDTH)*OVER;j++){
      double v=0.;
      for(i=0;i<p;i++)
        v += (4/((i*2+1)*M_PI))*sin(((j*(1./OVER)/W*CYCLES+OFFSET)*(i*2+1))*2*M_PI);
      memo[j+LINE_WIDTH*OVER]=v;
    }
  }else if(p>lastmemo){
    for(j=-LINE_WIDTH*OVER;j<(W+LINE_WIDTH)*OVER;j++){
      double v=memo[j+LINE_WIDTH*OVER];
      for(i=lastmemo;i<p;i++)
        v += (4/((i*2+1)*M_PI))*sin(((j*(1./OVER)/W*CYCLES+OFFSET)*(i*2+1))*2*M_PI);
      memo[j+LINE_WIDTH*OVER]=v;
    }
  }
  lastmemo=p;
}

double get_ys(int x){
  int i;
  float v=0;
  x+=LINE_WIDTH;
  x*=OVER;
  for(i=x-OVER+1;i<x+OVER;i++){
    float d = 1.-fabs(x-i)/OVER;
    v+=d*memo[i];
  }
  return CR+WH*v/OVER;
}

int tosample(double x){
  double SW = (double)W/SAMPLES;
  return rint(x/SW);
}

int tox(int sample){
  double SW = (double)W/SAMPLES;
  return rint((sample-OFFSET*SAMPLES/CYCLES)*SW);
}

void draw_square(cairo_surface_t *cs){
  int i;
  cairo_t *c = cairo_create(cs);
  double y0;

  cairo_set_source_rgba(c,FIRST_COLOR);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,LINE_WIDTH);

  for(i=-LINE_WIDTH;i<W+LINE_WIDTH;i++){
    double y = get_y(i);
    cairo_move_to(c,i,y0);
    cairo_set_source_rgba(c,FIRST_COLOR*.5);
    cairo_line_to(c,i,y);
    cairo_stroke(c);
    y0=y;
  }

  cairo_destroy(c);
}

void draw_waveform(cairo_surface_t *cs,int p, double alpha){
  int i;
  cairo_t *c = cairo_create(cs);
  double ym=0,yM=0;
  int x=-LINE_WIDTH;
  if(p<1)return;

  cairo_set_source_rgba(c,SECOND_COLOR*alpha);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,LINE_WIDTH);

  compute_ys(p);
  for(i=-LINE_WIDTH;i<W+LINE_WIDTH;i++){
    double y = get_ys(i);
    cairo_line_to(c,i,y);
  }


  cairo_stroke(c);
  cairo_destroy(c);
}

#define MAX(x,y) ((x)>(y)?(x):(y))

double draw_formula_Y(cairo_t *c,int size,double *xx, double y, double alpha){
  cairo_text_extents_t extents;
  cairo_text_extents_t extentsB;
  cairo_font_face_t *fi,*ff;
  double x=*xx;
  char *text;

  fi = cairo_toy_font_face_create ("Liberation Serif",
                                   CAIRO_FONT_SLANT_OBLIQUE,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  ff = cairo_toy_font_face_create ("Liberation Sans",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  if(!fi){
    fprintf(stderr,"Unable to create formula font");
    exit(1);
  }

  cairo_set_source_rgba(c,0,0,0,alpha);
  cairo_set_font_size(c, size);

  cairo_set_font_face(c,fi);
  text = "y";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_set_font_face(c,ff);
  text = "(";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;


  cairo_set_font_face(c,fi);
  text = "t";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_set_font_face(c,ff);
  text = ") = ";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_font_face_destroy(fi);
  cairo_font_face_destroy(ff);
  *xx=x;
  return extents.height;
}

double draw_formula_T(cairo_t *c,int n,int size,double *xx,double y, int color, double alpha){
  cairo_text_extents_t extents;
  cairo_text_extents_t extentsB;
  cairo_font_face_t *fi,*ff,*fb;
  double x=*xx;
  char *text;
  char buf[80];
  double th;

  fi = cairo_toy_font_face_create ("Liberation Serif",
                                   CAIRO_FONT_SLANT_OBLIQUE,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  ff = cairo_toy_font_face_create ("Liberation Sans",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
  fb = cairo_toy_font_face_create ("Liberation Sans",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
  if(!ff){
    fprintf(stderr,"Unable to create formula font");
    exit(1);
  }
  if(!fi){
    fprintf(stderr,"Unable to create formula font");
    exit(1);
  }


  switch(color){
  case 0:
    cairo_set_source_rgba(c,0,0,0,0);
    break;
  case 1:
  case 2:
    cairo_set_source_rgba(c,SECOND_COLOR*alpha);
    break;
  }

  /* fraction */

  cairo_set_font_face(c,fb);
  cairo_set_font_size(c, size*.5);
  text = "4";
  cairo_text_extents(c, text, &extents);
  th = extents.height;

  text = "\xE2\x80\x94";
  cairo_text_extents(c, text, &extentsB);
  if(extents.x_advance>extentsB.x_advance)extentsB.x_advance=extents.x_advance;
  if(n>1){
    snprintf(buf,80,"%d\xCF\x80",(n*2-1));
  }else{
    snprintf(buf,80,"\xCF\x80");
  }
  cairo_text_extents(c, buf, &extents);
  if(extents.x_advance>extentsB.x_advance)extentsB.x_advance=extents.x_advance;
  cairo_move_to(c,x + (extentsB.x_advance - extents.x_advance)/2,y+th-extents.y_bearing/2);
  cairo_show_text(c,buf);

  text = "\xE2\x80\x94";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (extentsB.x_advance - extents.x_advance)/2,y-extents.y_bearing);
  cairo_show_text(c,text);

  text="4";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (extentsB.x_advance - extents.x_advance)/2,y-th-extents.y_bearing/2);
  cairo_show_text(c,text);
  x+= extentsB.x_advance+size/10;

  cairo_set_font_size(c, size);
  cairo_set_font_face(c,ff);

  if(n>1)
    snprintf(buf,80,"sin(%d",n*2-1);
  else
    snprintf(buf,80,"sin(");
  text=buf;
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x+=extents.x_advance;

  cairo_set_font_face(c,fi);
  text="\xCF\x89t";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x+=extents.x_advance;

  cairo_set_font_face(c,ff);
  text=")";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  cairo_show_text(c,text);
  x+=extents.x_advance;

  switch(color){
  case 0:
  case 1:
    cairo_set_source_rgba(c,0,0,0,0);
    break;
  case 2:
    cairo_set_source_rgba(c,FIRST_COLOR*alpha);
    break;
  }

  cairo_set_font_face(c,ff);
  text=" + ";
  cairo_text_extents(c, text, &extentsB);
  cairo_move_to(c,x,y-extentsB.y_bearing/2);
  cairo_show_text(c,text);
  x+=extentsB.x_advance;

  cairo_font_face_destroy(ff);
  cairo_font_face_destroy(fi);
  *xx=x;
  return extents.height;
}

static int startframe=-1;

void draw_formula(cairo_surface_t *cs, double pp, int frame, double alpha){
  cairo_t *c = cairo_create(cs);
  int p = pp;
  double y = TEXTY;
  double size = TEXTSIZE;
  int i = 1;
  int alignw=0;
  float reduce = REDUCA;
  double lw = W-(XMARGINL+XMARGINR);

  while(y<TEXTY+TEXTH){

    double w = alignw;
    double ww = alignw;
    int ii=i;
    double x=XMARGINL;
    while(w < lw){
      ww=w;
      if(ii==1){
        draw_formula_Y(c,size,&w, 0, 0);
        alignw=w;
      }
      draw_formula_T(c,ii,size,&w, 0, 0,0);
      if(i==1 && ii>p && w<lw){
        ii++;
        x = (W-XMARGINL-XMARGINR-ww)/2+XMARGINL;
        y = TEXT0;
        break;
      }
      ii++;
    }

    if(i==1 && ii-1>p && w>=lw && startframe==-1){
      startframe=frame;
    }
    if(i==1 && startframe>-1){
      float del = (float)(frame-startframe)/YUP;
      if(del<=1){
        del = sin(del*.5*M_PI);
        del *= del;
        y = del*TEXTY + (1-del)*TEXT0;
      }else{
        y = TEXTY;
      }
    }

    lw=ww-8;
    ii--;


    double yh=0;

    if(i>1)
      x+=alignw;

    for(;i<ii;i++){
      if(i==1)
        draw_formula_Y(c,size,&x, y, alpha);

      double hh=draw_formula_T(c,i,size,&x, y, i<p?2:(p&&i<=p?1:0), alpha);
      if(hh>yh)yh=hh;
    }

    size *= reduce;
    reduce = REDUCB;
    yh*=1.5;
    y += yh+10;
    if(size<1)size=1;
  }
  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						    W,H);
  int i;
  int count=0;

  clear_surface(cs);
  draw_square(cs);
  draw_axis(cs);
  for(i=0;i<PRE;i++){
    write_frame(cs,count++);
  }

  /* fade to fundamental */
  for(i=0;i<FADE;i++){
    clear_surface(cs);
    draw_square(cs);
    draw_axis(cs);
    draw_waveform(cs,1,(float)i/FADE);
    draw_formula(cs,1,0,(float)i/FADE);
    write_frame(cs,count++);
  }

  /* hold fundamental */
  clear_surface(cs);
  draw_square(cs);
  draw_axis(cs);
  draw_waveform(cs,1,1);
  draw_formula(cs,1,0,1);
  for(i=0;i<FUNDAMENTAL;i++){
    write_frame(cs,count++);
  }

  /* count up */
  float p = 2;
  float a = ACCEL;
  float a2 = PLUS;
  int lastinc=0;
  int cont=0;
  for(i=1;i<=APLUS;i++){
    clear_surface(cs);
    draw_square(cs);
    draw_axis(cs);
    draw_waveform(cs,(int)p,1);
    draw_formula(cs,p,i,1);
    write_frame(cs,count++);
    a+=(i<APLUS/2)?ACCELB:ACCELB*(1+20*((float)(i-APLUS/2)/(APLUS/2)));
    a2*=a;
    int inc = (int)(p+a2) - (int)(p);
    if(inc>0 && lastinc>0)cont=1;
    if(cont && inc<lastinc){
      p+=inc=lastinc;
    }else{
      p+=a2;
    }
    lastinc=inc;
    fprintf(stderr,"p=%f, togo=%d\n",p,APLUSPLUS-i);
  }

 for(;i<=APLUSPLUS;i++){
    clear_surface(cs);
    draw_square(cs);
    draw_axis(cs);
    draw_waveform(cs,(int)p,1);
    draw_formula(cs,p,i,1);
    write_frame(cs,count++);
    fprintf(stderr,"p=%f, togo=%d\n",p,APLUSPLUS-i);
  }

  for(i=0;i<POST;i++)
    write_frame(cs,count++);

  cairo_surface_destroy(cs);
  return 0;
}
