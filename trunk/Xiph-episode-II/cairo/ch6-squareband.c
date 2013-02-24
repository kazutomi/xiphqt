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

#define TEXTY 600
#define TEXTH 500
#define XMARGINL 90
#define XMARGINR 60
#define TEXTSIZE 60
#define REDUCA .8
#define REDUCB .78

#define OVER 20
#define YUP (24*4)

#define PRE 48
#define FHOLD 16
#define FFADE 24
#define BANDHOLD 72
#define BANDFADE 18
#define POST 48

#define PLUS 1
#define ACCEL .01

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

  snprintf(buffer,80,"ch6_squareband-%04d.png",frameno);
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

void transparent_surface(cairo_surface_t *cs){
  cairo_t *c = cairo_create(cs);
  cairo_set_source_rgba(c,1.,1.,1.,1.);
  cairo_set_operator(c,CAIRO_OPERATOR_CLEAR);
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

void draw_square(cairo_surface_t *cs, float alpha){
  int i;
  cairo_t *c = cairo_create(cs);
  double y0;

  cairo_set_source_rgba(c,FIRST_COLOR*.5*alpha);
  cairo_set_line_cap(c,CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(c,LINE_WIDTH);

  for(i=-LINE_WIDTH;i<W+LINE_WIDTH;i++){
    double y = get_y(i);
    cairo_move_to(c,i,y0);
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

double draw_formula_Y(cairo_t *c,int size,double *xx, double y, int show){
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

  cairo_set_font_size(c, size);

  cairo_set_font_face(c,fi);
  text = "y";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show)
    cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_set_font_face(c,ff);
  text = "(";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show)
    cairo_show_text(c,text);
  x += extents.x_advance;


  cairo_set_font_face(c,fi);
  text = "t";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show)
    cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_set_font_face(c,ff);
  text = ") = ";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show)
    cairo_show_text(c,text);
  x += extents.x_advance;

  cairo_font_face_destroy(fi);
  cairo_font_face_destroy(ff);
  *xx=x;
  return extents.height;
}

double draw_formula_T(cairo_t *c,int n,int size,double *xx,double y, int plus, int show){
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
  if(show)
    cairo_show_text(c,buf);

  text = "\xE2\x80\x94";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (extentsB.x_advance - extents.x_advance)/2,y-extents.y_bearing);
  if(show)
    cairo_show_text(c,text);

  text="4";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x + (extentsB.x_advance - extents.x_advance)/2,y-th-extents.y_bearing/2);
  if(show)
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
  if(show)
    cairo_show_text(c,text);
  x+=extents.x_advance;

  cairo_set_font_face(c,fi);
  text="\xCF\x89t";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show)
    cairo_show_text(c,text);
  x+=extents.x_advance;

  cairo_set_font_face(c,ff);
  text=")";
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show)
    cairo_show_text(c,text);
  x+=extents.x_advance;

  cairo_set_font_face(c,ff);
  text=" + ";
  cairo_text_extents(c, text, &extentsB);
  cairo_move_to(c,x,y-extentsB.y_bearing/2);
  if(plus)
    cairo_show_text(c,text);
  x+=extentsB.x_advance;

  cairo_font_face_destroy(ff);
  cairo_font_face_destroy(fi);
  *xx=x;
  return extents.height;
}

double draw_formula_K(cairo_t *c,int n,int size,double *xx,double y, int show){
  cairo_text_extents_t extents;
  cairo_font_face_t *ff;
  double x=*xx;
  char *text;
  char buf[80];
  double th;

  ff = cairo_toy_font_face_create ("Liberation Sans",
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
  if(!ff){
    fprintf(stderr,"Unable to create formula font");
    exit(1);
  }

  cairo_set_font_size(c, size*1.2);
  cairo_set_font_face(c,ff);

  snprintf(buf,80,"%dkHz + ",n*2-1);
  text=buf;
  cairo_text_extents(c, text, &extents);
  cairo_move_to(c,x,y-extents.y_bearing/2);
  if(show){
    snprintf(buf,80,"%dkHz",n*2-1);
    cairo_show_text(c,text);
  }
  x+=extents.x_advance;

  cairo_font_face_destroy(ff);
  *xx=x;
  return extents.height;
}

void draw_formula(cairo_t *c, int from, int to, int plusp, int fracp, int kHzp){
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
      draw_formula_T(c,ii,size,&w, 0, 0, 0);
      ii++;
    }

    lw=ww-8;
    ii--;

    double yh=0;
    if(i>1)
      x+=alignw;

    for(;i<ii;i++){
      if(i==1){
        draw_formula_Y(c,size,&x, y, from==0);
        if(to==0)return;
      }

      double wT=x;
      double wK=0;
      double hh = draw_formula_T(c,i,size, &wT, y, plusp && i>=from && i<to, fracp && i>=from && i<to);
      wT-=x;
      if(hh>yh)yh=hh;
      draw_formula_K(c,i,size,&wK, y, 0);
      if(i>=from && i<to && kHzp){
        wK = x + (wT-wK)/2;
        draw_formula_K(c,i,size,&wK, y, 1);
      }

      x+=wT;
      if(hh>yh)yh=hh;
      if(i>=to)return;
    }

    size *= reduce;
    reduce = REDUCB;
    yh*=1.5;
    y += yh+10;
    if(size<1)size=1;
  }
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    W,H);
  int i,j;
  int count=0;

  transparent_surface(cs);
  draw_waveform(cs,10,1);
  write_frame(cs,count++);

  clear_surface(cs);
  draw_square(cs,1);
  draw_axis(cs);

  cairo_t *c=cairo_create(cs);
  cairo_set_source_rgba(c,0,0,0,1);
  draw_formula(c,0,0,0,0,0);
  cairo_set_source_rgba(c,FIRST_COLOR);
  draw_formula(c,1,1000,1,1,0);
  cairo_destroy(c);

  for(i=0;i<PRE;i++){
    write_frame(cs,count++);
  }

  /* kHz markers */
  int hold = FHOLD;
  for(j=1;j<20;j+=2){
    int jj = (j+1)/2;

    for(i=1;i<=hold;i++){

      clear_surface(cs);
      draw_square(cs,1);
      draw_axis(cs);

      /* new waveform */
      draw_waveform(cs,jj,1);

      cairo_t *c=cairo_create(cs);
      /* draw Y and all pluses */
      cairo_set_source_rgba(c,0,0,0,1);
      draw_formula(c,0,0,0,0,0);
      cairo_set_source_rgba(c,FIRST_COLOR);
      draw_formula(c,1,1000,1,0,0);

      /* draw fractions to date */
      cairo_set_source_rgba(c,0,0,0,1);
      draw_formula(c,1,jj,0,1,0);

      /* draw fractions past */
      cairo_set_source_rgba(c,FIRST_COLOR);
      draw_formula(c,jj+1,1000,0,1,0);

      cairo_set_source_rgba(c,SECOND_COLOR);
      draw_formula(c,jj,jj+1,0,0,1);

      write_frame(cs,count++);
    }
    hold--;
  }

  for(i=1;i<=FHOLD-hold;i++)
    write_frame(cs,count++);

  for(i=1;i<=FFADE;i++){
    clear_surface(cs);
    draw_square(cs,(1-(float)i/FFADE));
    draw_axis(cs);

    /* waveform */
    draw_waveform(cs,10,1);

    cairo_t *c=cairo_create(cs);
    /* draw Y */
    cairo_set_source_rgba(c,0,0,0,1);
    draw_formula(c,0,0,0,0,0);

    /* draw fractions to date */
    cairo_set_source_rgba(c,FIRST_COLOR);
    draw_formula(c,1,10,1,0,0);
    cairo_set_source_rgba(c,0,0,0,1);
    draw_formula(c,1,10,0,1,0);

    /* fade in last fraction */
    cairo_set_source_rgba(c,0,0,0,(float)i/FFADE);
    draw_formula(c,10,11,0,1,0);

    /* fade out last marker */
    cairo_set_source_rgba(c,SECOND_COLOR*(1-(float)i/FFADE));
    draw_formula(c,10,11,0,0,1);

    /* fade pluses past */
    cairo_set_source_rgba(c,FIRST_COLOR*(1-(float)i/FFADE));
    draw_formula(c,9,1000,1,0,0);

    /* fade fractions past */
    draw_formula(c,11,1000,0,1,0);

    write_frame(cs,count++);
  }

  for(i=1;i<=BANDHOLD;i++)
    write_frame(cs,count++);




  cairo_surface_destroy(cs);
  return 0;
}
