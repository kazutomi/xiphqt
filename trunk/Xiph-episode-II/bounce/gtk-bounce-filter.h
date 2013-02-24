
#ifndef _BOUNCE_CONVO_H_
#define _BOUNCE_CONVO_H_

#include "twopole.h"
#include <fftw3.h>

typedef struct {
  int blocksize;
  float *filter;
} convofilter;

typedef struct {
  fftwf_plan forward[3];
  fftwf_plan inverse[3];
  float *fbuffer[3];
  int head;

  int headfill;
  int storefill;

  pole2 mix_filter;
  float mix_now;
} convostate;

#define MAXSTAGES 41

typedef struct {
  int stages;
  int rate;
  double a1[MAXSTAGES];
  double a2[MAXSTAGES];
  double b1[MAXSTAGES];
  double *z1p;
  double *z2p;
  double delayz1[MAXSTAGES];
  double delayz2[MAXSTAGES];
  pole2 mix_filter;
  float mix_now;
  int preroll;
} notchfilter;

extern convofilter *filter_bandstop_new(float w0,
                                        float w1,
                                        int rate,
                                        int n);

extern convofilter *filter_lowpass_new(float w,
                                       int rate,
                                       int n);

extern void convofilter_destroy(convofilter *f);

extern convostate *convostate_new(convofilter *f);

extern void convostate_destroy(convostate *f);

extern int run_convolution_filter(convofilter *f,
                                  convostate *s,
                                  float *buf,
                                  int n,
                                  int run,
                                  int active,
                                  float gain);

extern notchfilter *filter_notch_new(float f,int rate);
extern void filter_notch_destroy(notchfilter *f);
extern int run_notch_filter(notchfilter *f,
                            float *buf,
                            int n,
                            int run,
                            int active,
                            float gain);

#endif
