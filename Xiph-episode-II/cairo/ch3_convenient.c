#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>

#define W 1920
#define H 1080
#define CR 540
#define WH 300

static int pstep=64;
static int xstart=317;
static int xbound=350;
static int xend=1408;
static int ystart=179;
static int ybound=186;
static int yend=910;

#define OUT         48
#define HOLD        72
#define CIRCLE      24
#define CIRCLEHOLD  72
#define IN          18
#define POST        48

cairo_surface_t *read_frame(char *filename){
  cairo_surface_t *ps=cairo_image_surface_create_from_png(filename);
  cairo_status_t status = cairo_surface_status(ps);
  if(!ps || status!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"CAIRO ERROR: Unable to load PNG file %s: %s\n\n",filename,cairo_status_to_string(status));
    exit(1);
  }
  return ps;
}

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

void copy_surface(cairo_surface_t *s,cairo_surface_t *d){
  cairo_t *c = cairo_create(d);
  cairo_set_source_surface(c,s,0,0);
  cairo_paint(c);
  cairo_destroy(c);
}

void draw_squares(cairo_surface_t *cs, float delta){
  cairo_t *c = cairo_create(cs);
  double max = pstep*.5-1;
  double this = max*delta;
  int x,y;

  cairo_set_line_width(c,this);
  cairo_set_source_rgb(c,0,0,0);

  for(x=xbound-pstep;x<xend;x+=pstep){
    int x1=(x<xstart?xstart:x);
    int x2=(x+pstep>xend?xend:x+pstep);
    for(y=ybound-pstep;y<yend;y+=pstep){
      int y1=(y<ystart?ystart:y);
      int y2=(y+pstep>yend?yend:y+pstep);

      cairo_save(c);

      /* set up clip area */
      cairo_rectangle(c,x1,y1,x2-x1,y2-y1);
      cairo_clip(c);

      /* draw */
      cairo_rectangle(c, x+this/2., y+this/2. ,pstep-this,pstep-this);
      cairo_stroke(c);

      cairo_restore(c);
    }
  }

  cairo_destroy(c);
}

void draw_circles(cairo_surface_t *cs, float delta){
  cairo_t *c = cairo_create(cs);
  int x,y;

  cairo_set_line_width(c,3);
  cairo_set_source_rgba(c,.6,.2,.2,delta);

  for(x=xbound-pstep;x<xend;x+=pstep){
    int x1=(x<xstart?xstart:x);
    int x2=(x+pstep>xend?xend:x+pstep);
    for(y=ybound-pstep;y<yend;y+=pstep){
      int y1=(y<ystart?ystart:y);
      int y2=(y+pstep>yend?yend:y+pstep);

      cairo_save(c);

      /* set up clip area */
      cairo_rectangle(c,x1,y1,x2-x1,y2-y1);
      cairo_clip(c);

      /* draw */
      cairo_arc(c,x+pstep/2.,y+pstep/2.,15,0,2*M_PI);
      cairo_stroke(c);

      cairo_restore(c);
    }
  }

  cairo_destroy(c);
}

int main(){
  cairo_surface_t *cs = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						    W,H);
  cairo_surface_t *ps = read_frame("../framegrabs/convenient-squares.png");
  int i,count=0;

  /* shrink squares */
  for(i=0;i<OUT;i++){
    copy_surface(ps,cs);
    draw_squares(cs,(double)i/OUT);
    write_frame(cs,count++);
  }

  /* hold */
  copy_surface(ps,cs);
  draw_squares(cs,1.);
  for(i=0;i<HOLD;i++)
    write_frame(cs,count++);

  /* fade in circles */
  for(i=0;i<CIRCLE;i++){
    copy_surface(ps,cs);
    draw_squares(cs,1.);
    draw_circles(cs,(double)i/CIRCLE);
    write_frame(cs,count++);
  }

  /* hold */
  copy_surface(ps,cs);
  draw_squares(cs,1.);
  draw_circles(cs,1.);
  for(i=0;i<HOLD;i++)
    write_frame(cs,count++);

  /* fade out circles */
  for(i=0;i<CIRCLE;i++){
    copy_surface(ps,cs);
    draw_squares(cs,1.);
    draw_circles(cs,1.-(double)i/CIRCLE);
    write_frame(cs,count++);
  }

  /* hold */
  copy_surface(ps,cs);
  draw_squares(cs,1.);
  for(i=0;i<HOLD;i++)
    write_frame(cs,count++);

  /* grow squares */
  for(i=0;i<IN;i++){
    copy_surface(ps,cs);
    draw_squares(cs,1-(double)i/IN);
    write_frame(cs,count++);
  }

  /* post */
  copy_surface(ps,cs);
  for(i=0;i<POST;i++)
    write_frame(cs,count++);


  cairo_surface_destroy(cs);
  cairo_surface_destroy(ps);
  return 0;
}
