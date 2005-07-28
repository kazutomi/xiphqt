#include <math.h>
#include <gtk/gtk.h>
#include "graph.h"
#include "box.h"

void topbox (cairo_t *c, double w, double h){
  
  double x0 = B_BORDER+B_RADIUS;
  double y0 = B_BORDER;

  double x1 = B_BORDER;
  double y1 = B_BORDER+B_RADIUS;

  double x2 = B_BORDER;
  double y2 = h-B_BORDER-B_RADIUS;

  double x3 = B_BORDER+B_RADIUS;
  double y3 = h-B_BORDER;

  double x4 = w/2 - B_HUMP - B_RADIUS;
  double y4 = h-B_BORDER;

  double x5 = x4+B_RADIUS;
  double y5 = y4;


  double x7 = w/2 + B_HUMP + B_RADIUS;
  double y7 = h-B_BORDER;

  double x8 = w - B_BORDER -B_RADIUS;
  double y8 = h-B_BORDER;

  double x9 = w - B_BORDER;
  double y9 = h-B_BORDER-B_RADIUS;

  double x10 = w - B_BORDER;
  double y10 = B_BORDER+B_RADIUS;

  double x11 = w - B_BORDER-B_RADIUS;
  double y11 = B_BORDER;

  cairo_set_line_width(c,B_LINE);

  cairo_move_to  (c, x0, y0);
  cairo_curve_to (c, x1,y0, x1,y0, x1,y1);
  cairo_line_to (c, x2,y2);
  cairo_curve_to (c, x2,y3, x2,y3, x3,y3);
  cairo_line_to (c, x4,y4);

  {
    double hh = get_board_height()*.5+50;

    double radius = sqrt( (w*.5-x5) * (w*.5-x5) + (hh-y5) * (hh-y5));
    double siderad = atan( (x5-w*.5)/(hh-y5));

    double xx = sin(siderad+.05)*radius+w*.5;
    double yy = -cos(siderad+.05)*radius+hh;
    double x = sin(siderad+.1)*radius+w*.5;
    double y = -cos(siderad+.1)*radius+hh;

    cairo_curve_to (c, x4+B_RADIUS,y4, xx,yy, x,y);
    cairo_arc(c,w*.5,hh,radius,-M_PI*.5+siderad+.1,-M_PI*.5-siderad-.1);

    xx = -sin(siderad+.05)*radius+w*.5;
    yy = -cos(siderad+.05)*radius+hh;
    xx = -sin(siderad+.1)*radius+w*.5;
    yy = -cos(siderad+.1)*radius+hh;

    cairo_curve_to (c, xx,yy, x7-B_RADIUS,y7, x7,y7);

  }

  cairo_line_to  (c, x8,y8);
  cairo_curve_to (c, x9,y8, x9,y8, x9,y9);
  cairo_line_to  (c, x10,y10);
  cairo_curve_to (c, x10,y11, x10,y11, x11,y11);
  cairo_close_path (c);

  cairo_set_source_rgba (c, B_COLOR);
  cairo_fill_preserve (c);
  cairo_set_source_rgba (c, B_LINE_COLOR);
  cairo_stroke (c);

}

void bottombox (cairo_t *c, double w, double h){
  
  double x0 = B_BORDER+B_RADIUS;
  double y0 = h-B_BORDER;

  double x1 = B_BORDER;
  double y1 = h-B_BORDER-B_RADIUS;

  double x2 = B_BORDER;
  double y2 = B_BORDER+B_RADIUS;

  double x3 = B_BORDER+B_RADIUS;
  double y3 = B_BORDER;

  double x4 = w/2 - B_HUMP - B_RADIUS;
  double y4 = B_BORDER;

  double x5 = x4+B_RADIUS;
  double y5 = y4;


  double x7 = w/2 + B_HUMP + B_RADIUS;
  double y7 = B_BORDER;

  double x8 = w - B_BORDER -B_RADIUS;
  double y8 = B_BORDER;

  double x9 = w - B_BORDER;
  double y9 = B_BORDER+B_RADIUS;

  double x10 = w - B_BORDER;
  double y10 = h- B_BORDER -B_RADIUS;

  double x11 = w - B_BORDER-B_RADIUS;
  double y11 = h-B_BORDER;

  cairo_set_line_width(c,B_LINE);

  cairo_move_to  (c, x0, y0);
  cairo_curve_to (c, x1,y0, x1,y0, x1,y1);
  cairo_line_to (c, x2,y2);
  cairo_curve_to (c, x2,y3, x2,y3, x3,y3);
  cairo_line_to (c, x4,y4);

  {
    double hh = get_board_height()*.5+50;

    double radius = sqrt( (w*.5-x5) * (w*.5-x5) + (y5+hh-h) * (y5+hh-h));
    double siderad = atan( (x5-w*.5)/(y5+hh-h));

    double xx = sin(siderad+.05)*radius+w*.5;
    double yy = cos(siderad+.05)*radius-hh+h;
    double x = sin(siderad+.1)*radius+w*.5;
    double y = cos(siderad+.1)*radius-hh+h;

    cairo_curve_to (c, x4+B_RADIUS,y4, xx,yy, x,y);
    cairo_arc_negative(c,w*.5,h-hh,radius,M_PI*.5-siderad-.1,M_PI*.5+siderad+.1);

    xx = -sin(siderad+.05)*radius+w*.5;
    yy = cos(siderad+.05)*radius-hh+h;
    xx = -sin(siderad+.1)*radius+w*.5;
    yy = cos(siderad+.1)*radius-hh+h;

    cairo_curve_to (c, xx,yy, x7-B_RADIUS,y7, x7,y7);

  }

  cairo_line_to  (c, x8,y8);
  cairo_curve_to (c, x9,y8, x9,y8, x9,y9);
  cairo_line_to  (c, x10,y10);
  cairo_curve_to (c, x10,y11, x10,y11, x11,y11);
  cairo_close_path (c);

  cairo_set_source_rgba (c, B_COLOR);
  cairo_fill_preserve (c);
  cairo_set_source_rgba (c, B_LINE_COLOR);
  cairo_stroke (c);

}

