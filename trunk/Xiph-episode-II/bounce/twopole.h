/* specific filter recipes with thanks to Alex Matulich
   http://unicorn.us.com/alex/2polefilters.html */

#ifndef _TWOPOLE_H_
#define _TWOPOLE_H_

typedef struct {
  double a;
  double b1;
  double b2;
  double x[2];
  double y[2];
} pole2;

extern void filter_set(pole2 *p, double val);
extern void filter_make_bessel2(double w, int passes, pole2 *f);
extern void filter_make_critical(double w, int passes, pole2 *f);
extern void filter_make_butterworth(double w, int passes, pole2 *f);
extern double filter_filter(double x, pole2 *p);

#endif
