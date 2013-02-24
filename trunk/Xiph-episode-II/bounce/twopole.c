/* specific filter recipes with thanks to Alex Matulich
   http://unicorn.us.com/alex/2polefilters.html */

#include <math.h>
#include "twopole.h"

void filter_make_bessel2(double w, int passes, pole2 *f){
  double c = pow(pow(pow(2,(1./passes))-.75,-.5)-.5,-.5)/sqrt(3);
  double wc = c*w;
  double w0 = tan(M_PI*wc);
  double g = 3;
  double p = 3;
  double K1 = p*w0;
  double K2 = g*w0*w0;
  double A0 = K2/(1+K1+K2);
  double A1 = 2*A0;
  double A2 = A0;
  double B1 = 2*A0*(1/K2-1);
  double B2 = 1-(A0+A1+A2+B1);

  f->a = A0;
  f->b1 = B1;
  f->b2 = B2;
  filter_set(f,0);
}

void filter_make_critical(double w, int passes, pole2 *f){
  double c = pow(pow(2,(1./2*passes))-1,-.5);
  double wc = c*w;
  double w0 = tan(M_PI*wc);
  double g = 1;
  double p = 2;
  double K1 = p*w0;
  double K2 = g*w0*w0;
  double A0 = K2/(1+K1+K2);
  double A1 = 2*A0;
  double A2 = A0;
  double B1 = 2*A0*(1/K2-1);
  double B2 = 1-(A0+A1+A2+B1);

  f->a = A0;
  f->b1 = B1;
  f->b2 = B2;
  filter_set(f,0);
}

void filter_make_butterworth(double w, int passes, pole2 *f){
  double c = pow(pow(2,(1./passes))-1,-.25);
  double wc = c*w;
  double w0 = tan(M_PI*wc);
  double g = 1;
  double p = sqrt(2);
  double K1 = p*w0;
  double K2 = g*w0*w0;
  double A0 = K2/(1+K1+K2);
  double A1 = 2*A0;
  double A2 = A0;
  double B1 = 2*A0*(1/K2-1);
  double B2 = 1-(A0+A1+A2+B1);

  f->a = A0;
  f->b1 = B1;
  f->b2 = B2;
  filter_set(f,0);
}

void filter_set(pole2 *p, double val){
  p->x[0]=p->x[1]=val;
  p->y[0]=p->y[1]=val;
}


double filter_filter(double x, pole2 *p){
  double a = p->a;
  double  y = a*x + 2*a*p->x[0] + a*p->x[1] + p->b1*p->y[0] + p->b2*p->y[1];
  p->y[1] = p->y[0]; p->y[0] = y;
  p->x[1] = p->x[0]; p->x[0] = x;

  return y;
}
