#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gtk-bounce-filter.h"

static inline float todB(float x){
  return log((double)(x)*(x)+1e-50)*4.34294480;
}

#define A0 .35875f
#define A1 .48829f
#define A2 .14128f
#define A3 .01168f

static float beta(int n, float alpha){
  return cosh(acosh(pow(10,alpha))/(n-1));
}

static double T(double n, double x){
  if(fabs(x)<=1){
    return cos(n*acos(x));
  }else{
    return cosh(n*acosh(x));
  }
}

void filter_notch_reset(notchfilter *f){
  f->preroll = 0;
  f->mix_now = 0.;
  f->z1p = f->delayz1;
  f->z2p = f->delayz2;
  memset(f->delayz1,0,sizeof(f->delayz1));
  memset(f->delayz2,0,sizeof(f->delayz2));
}

void filter_notch_destroy(notchfilter *f){
  if(f)free(f);
}

/* Hardwired / specialized notch filter implementation **************/

/* Intended for very deep notches; we want greater than single
   precision mantissa depth (~200db).  Although it is possible to make
   that go in single precision, there's no reason to be clever.  Just
   run the filter in double precision and be done with it. */

/* Normally, we'd probably want to use a fast convolution FIR filter
   for this, but the additional latency of a sharp enough FIR filter
   will impact the responsiveness of the demo.  The IIR notch filter
   is inferior in several ways, but its reaction outside the stop band
   will be much faster than a big convolution filter and we really
   don't care about phase behavior or precision requirements. */

int run_notch_filter(notchfilter *f,
                     float *buf,
                     int n,
                     int run,
                     int active,
                     float gain){
  int i,j;
  float mix_target = active ? 1. : 0.;
  float mix = f->mix_now;
  double *z1 = f->z1p;
  double *z2 = f->z2p;
  double *a1 = f->a1;
  double *a2 = f->a2;
  double *b1 = f->b1;

  for(i=0;i<n;i++){

    double x = buf[i];
    for(j=0;j<f->stages;j++){
      double z0 = x - a1[j]*z1[j] - a2[j]*z2[j];
      x = z0 + b1[j]*z1[j] + z2[j];
      z2[j] = z0;
    }

    double *temp = z1;
    z1 = z2;
    z2 = temp;

    if(f->preroll > f->rate){
      if(fabs(mix-mix_target)>.0000001){
        mix = filter_filter(mix_target, &f->mix_filter);
        buf[i] *= (1.-mix);
        buf[i] += x*mix*gain;
      }else{
        if(!active && !run){
          /* reset state and return; leave rest of buf untouched */
          filter_notch_reset(f);
          return 0;
        }else{
          f->mix_now = mix_target;
          if(mix_target>.0000001){
            buf[i] = x*gain;
          }
        }
      }
    }else{
      f->preroll++;
    }
  }

  f->mix_now = mix;
  f->z1p = z1;
  f->z2p = z2;
  return 1;
}

/* Analog filter gurus, please don't laugh too hard */
/* Hand drawn in the z-plane, hardwired to our specific use. */
/* For a more general-purpose filter, see the convolution based
   bandstop with a deep Dolph-Tschebyshev window later below. */
#define DUMPH 0
notchfilter *filter_notch_new(float f, int rate){
  int i,j;

#if DUMPH
  float Hmag[rate*8];
  float Hang[rate*8];
  for(i=0;i<rate*8;i++)Hmag[i]=1.;
  for(i=0;i<rate*8;i++)Hang[i]=0.;
#endif

  notchfilter *ret = calloc(1,sizeof(*ret));
  int stages = ret->stages = 30;
  float side = (stages-1.)/2;
  float tw = 30;
  float fscale = tw/2./side;

  for(j=0;j<stages;j++){
    float bw = 15 + 6.2*cos(M_PI*(j-side)/side);
    double r = 1. - M_PI*(bw)/rate;

    float jfz = sin(M_PI*(j-side)/(side+1)/2)*side;
    float jfp = sin(M_PI*(j-side)/(side)/2)*side;
    float wz = 2*M_PI*(f+jfz*fscale)/rate;
    float wp = 2*M_PI*(f+jfp*fscale*1.206)/rate;

    double b1 = ret->b1[j] = -2. * cos(wz);
    double a1 = ret->a1[j] = r * -2 * cos(wp);
    double a2 = ret->a2[j] = r*r;

#if DUMPH
    for(i=0;i<rate*8;i++){
      float w = 2*M_PI*i/rate/16;
      double a = 1 + b1*cos(w) + cos(2*w);
      float b = -b1*sin(w) - sin(2*w);
      double c = 1. + a1*cos(w) + a2*cos(2*w);
      double d = -a1*sin(w) - a2*sin(2*w);
      double re = (a*c + b*d)/(c*c+d*d);
      double im = (b*c - a*d)/(c*c+d*d);
      Hmag[i] *= hypot(im,re);
      Hang[i] += atan2(im,re);
    }
#endif
  }

#if DUMPH
  {
    FILE *file=fopen("iirnotchM.m","w");
    for(i=0;i<rate*8;i++){
      fprintf(file,"%f %lf\n",(i/16.), todB(Hmag[i])-90.6);
    }
    fclose(file);
  }
  {
    FILE *file=fopen("iirnotchA.m","w");
    for(i=0;i<rate*8;i++){
      float a = Hang[i]*180/M_PI;
      fprintf(file,"%f %lf\n",(i/16.), a);
    }
    fclose(file);
  }
#endif

  ret->rate = rate;
  filter_make_critical(.0001,1,&ret->mix_filter);
  filter_notch_reset(ret);
  return (ret);
}

/* FIR bandstop filter constructed out of a frequency domain box
   response convolved with a deep Dolph-Tschebyshev window */

/* 'n' is equivalent to complex blocksize; the returned array is a
   real-only array of frequency-domain amplitudes of size n/2+1 */
convofilter *filter_bandstop_new(float w0,
                                 float w1,
                                 int rate,
                                 int n){
  int i;
  convofilter *ret = calloc(1,sizeof(*ret));
  float *f = malloc((n/2+1)*sizeof(*f));

  double *freqbuffer = fftw_malloc((n*2+1)*sizeof(*freqbuffer));
  double *winbuffer = fftw_malloc((n/4+1)*sizeof(*winbuffer));

  fftw_plan freqplan_i =
    fftw_plan_r2r_1d(n*2+1,freqbuffer,freqbuffer,
                     FFTW_REDFT00,FFTW_ESTIMATE);
  fftw_plan winplan_i =
    fftw_plan_r2r_1d(n/4+1,winbuffer,winbuffer,
                     FFTW_REDFT00,FFTW_ESTIMATE);
  fftw_plan freqplan_f =
    fftw_plan_r2r_1d(n/2+1,freqbuffer,freqbuffer,
                     FFTW_REDFT00,FFTW_ESTIMATE);

  /* construct box response */
  int a = rint(w0/rate*n*4);
  int b = rint(w1/rate*n*4);
  for(i=0;i<a;i++)
    freqbuffer[i]=1.;
  for(;i<=b;i++)
    freqbuffer[i]=0.;
  for(;i<n*2+1;i++)
    freqbuffer[i]=1.;

  /* to time domain for windowing/padding */
  fftw_execute(freqplan_i);

  /* ~160dB deep Dolph-Tschebyshev window; exceeds precision of fftwf */
  /* be aware that the 'Net is rife with incorrect definitions of this
      window (and many that don't even make sense).  This version is
      adapted from Dolph's 1946 paper. */
  int N=n/2;
  float alpha = 8.;
  double B = beta(N,alpha);

  for(i=0;i<n/4+1;i++)
    winbuffer[i] = T(N,B*cos( M_PI*i/N ));

  fftw_execute(winplan_i);

  double D = 2./(winbuffer[0]*n*n*8);

  for(i=0;i<n/4+1;i++)
    freqbuffer[i]*=winbuffer[i];

  /* pad */
  for(;i<n/2+1;i++)
    freqbuffer[i]=0;

  /* back to frequency */
  fftw_execute(freqplan_f);

  for(i=0;i<n/2+1;i++)
    f[i]=freqbuffer[i]*D;

#if 0
  {
    FILE *file=fopen("notch.m","w");
    for(i=0;i<n/2+1;i++)
      fprintf(file,"%d %lf\n",(int)(rate/2.*i/(n/2)),todB(f[i]));
    fclose(file);
  }
#endif

  fftw_destroy_plan(winplan_i);
  fftw_destroy_plan(freqplan_i);
  fftw_destroy_plan(freqplan_f);
  fftw_free(winbuffer);
  fftw_free(freqbuffer);

  ret->blocksize=n;
  ret->filter=f;

  return ret;
}

convofilter *filter_lowpass_new(float w,
                                int rate,
                                int n){
  int i;
  convofilter *ret=calloc(1,sizeof(*ret));

  float *f = malloc((n/2+1)*sizeof(*f));
  double *freqbuffer = fftw_malloc((n/2+1)*sizeof(*freqbuffer));

  fftw_plan freqplan_i =
    fftw_plan_r2r_1d(n/4+1,freqbuffer,freqbuffer,
                      FFTW_REDFT00,FFTW_ESTIMATE);
  fftw_plan freqplan_f =
    fftw_plan_r2r_1d(n/2+1,freqbuffer,freqbuffer,
                      FFTW_REDFT00,FFTW_ESTIMATE);

  /* construct box response */
  int a = rint(w/rate*n*.5);
  for(i=0;i<a;i++)
    freqbuffer[i]=1.;
  for(;i<n/4+1;i++)
    freqbuffer[i]=0.;

  /* to time domain for windowing/padding */
  fftw_execute(freqplan_i);

  /* Blackmann-Harris */
  float scale = 4*M_PI/n;
  for(i=0;i<n/4+1;i++){
    float w = A0 + A1*cos(scale*i) + A2*cos(scale*i*2) + A3*cos(scale*i*3);
    freqbuffer[i] *= w;
  }

  /* pad */
  for(;i<n/2+1;i++)
    freqbuffer[i]=0;

  /* back to frequency */
  fftw_execute(freqplan_f);

  for(i=0;i<n/2+1;i++)
    f[i]=2.*freqbuffer[i]/(n*n); /* normalize */

#if 0
  {
    FILE *file=fopen("low.m","w");
    for(i=0;i<n/2+1;i++)
      fprintf(file,"%d %f\n",(int)(rate/2.*i/(n/2)),todB(f[i]));
    fclose(file);
  }
#endif

  fftw_destroy_plan(freqplan_i);
  fftw_destroy_plan(freqplan_f);
  fftw_free(freqbuffer);

  ret->blocksize=n;
  ret->filter=f;
  return ret;
}

convostate *convostate_new(convofilter *f){
  convostate *ret = calloc(1,sizeof(*ret));
  int i;

  for(i=0;i<3;i++){
    ret->fbuffer[i]=fftwf_malloc((f->blocksize+2)*sizeof(*ret->fbuffer));
    ret->forward[i]=fftwf_plan_dft_r2c_1d(f->blocksize,ret->fbuffer[i],
                                          (fftwf_complex *)ret->fbuffer[i],
                                          FFTW_ESTIMATE);
    ret->inverse[i]=fftwf_plan_dft_c2r_1d(f->blocksize,
                                          (fftwf_complex *)ret->fbuffer[i],
                                          ret->fbuffer[i],
                                          FFTW_ESTIMATE);
  }
  filter_make_critical(.0001,1,&ret->mix_filter);
  ret->mix_now=0;
  return ret;
}

void convofilter_destroy(convofilter *f){
  if(f){
    if(f->filter)free(f->filter);
    free(f);
  }
}

void convostate_destroy(convostate *f){
  if(f){
    int i;
    for(i=0;i<3;i++){
      if(f->fbuffer[i])fftwf_free(f->fbuffer[i]);
      if(f->forward[i])fftwf_destroy_plan(f->forward[i]);
      if(f->inverse[i])fftwf_destroy_plan(f->inverse[i]);
    }
    free(f);
  }
}

/* modifies buf in-place */
int run_convolution_filter(convofilter *f,
                           convostate *s,
                           float *buf,
                           int n,
                           int run,
                           int active,
                           float gain){
  int i;
  int bs = f->blocksize;
  int bs2 = f->blocksize/2;
  int bs4 = f->blocksize/4;

  while(n){
    float *head = s->fbuffer[s->head];
    float *head4 = head+bs4;
    float *head2 = head+bs2;
    int copysamples = n;
    if(copysamples+s->headfill>bs2) copysamples = bs2-s->headfill;

    /* accumulate new samples into the head */
    memcpy(head4+s->headfill,buf,sizeof(*head)*copysamples);

    /* overlap-add samples out of the lap buffer if it's deep enough */
    if(s->storefill==2){
      float *tail = s->fbuffer[(s->head+1)%3]+bs2+s->headfill;
      float *mid  = s->fbuffer[(s->head+2)%3]+s->headfill;
      float mix_target = active ? 1. : 0.;

      if(fabs(s->mix_now-mix_target)>.0000001){
        float mix;
        for(i=0;i<copysamples;i++){
          float val = tail[i]+mid[i];
          mix = filter_filter(mix_target, &s->mix_filter);

          buf[i] *= (1.-mix);
          buf[i] += val*mix*gain;
        }
        s->mix_now = mix;
      }else{
        if(!active && !run){
          /* reset state and return; leave rest of buf untouched */
          for(i=0;i<3;i++)
            memset(s->fbuffer[i],0,(f->blocksize+2)*sizeof(**s->fbuffer));
          s->head=0;
          s->headfill=0;
          s->storefill=0;
          return 0;
        }else{
          s->mix_now = mix_target;
          if(mix_target>.0000001){
            for(i=0;i<copysamples;i++){
              float val = tail[i]+mid[i];
              buf[i] = val*gain;
            }
          }
        }
      }
    } //else nothing; leave buf untouched

    s->headfill+=copysamples;
    buf+=copysamples;
    n-=copysamples;

    /* full accumulation? */
    if(s->headfill==bs2){

      /* pad fftw3f buffer */
      memset(head,0,bs4*sizeof(*head2));
      memset(head+bs2+bs4,0,bs4*sizeof(*head2));

      /* transform */
      fftwf_execute(s->forward[s->head]);

      /* filter */
      float *mag = f->filter;
      for(i=0;i<bs+2;i+=2){
        head[i] *= mag[i>>1];
        head[i+1] *= mag[i>>1];
      }

      /* inverse transform */
      fftwf_execute(s->inverse[s->head]);

      /* cycle */
      s->head = (s->head+1)%3;
      s->headfill=0;
      s->storefill++;
      if(s->storefill>2)s->storefill=2;
    }
  }
  return 1;
}

#if 0
#define fromdB(x) (exp((x)*.11512925f))
int main(int argc, char **argv){
  int i;
  float data[131072];

  for(i=0;i<131072;i++){
    data[i]=sin(1000*2.*M_PI/44100*i)*.95;
    data[i]+=(drand48()-drand48())*fromdB(-93.);
  }
  notchfilter *notch=filter_notch_new(1000,44100);
  //convofilter *notch=filter_bandstop_new(970,1030,44100,32768);
  //convofilter *lowpass=filter_lowpass_new(20500,44100,512);

  //convostate *nt = convostate_new(notch);
  //convostate *lt = convostate_new(lowpass);

  for(i=0;i<131072;i+=4096){
    //run_convolution_filter(notch,nt,data+i,4096,1,1,256.);
    run_notch_filter(notch,data+i,4096,1,1,6309);
  }

  FILE *f=fopen("test.m","w");
  for(i=0;i<131072;i++){
    fputc(((int)rint(data[i]*32768))&0xff,stdout);
    fputc((((int)rint(data[i]*32768))>>8)&0xff,stdout);
    fprintf(f,"%d %f\n",i,data[i]);
  }
  fclose(f);

  return 0;
}

#endif
