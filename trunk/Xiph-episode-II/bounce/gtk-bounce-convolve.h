
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

#endif
