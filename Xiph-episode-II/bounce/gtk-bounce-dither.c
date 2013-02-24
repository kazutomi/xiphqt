#include "gtk-bounce.h"
#include "gtk-bounce-dither.h"

/* flat and shaped dither, adapted from gmaxwell's code in opusdec */

static unsigned int rngseed = 22222;
static inline unsigned int fast_rand() {
  /* Greg, this is one of the craziest things I've ever seen.  Bravo. */
  rngseed = (rngseed * 96314165) + 907633515;
  return rngseed;
}

const float fcoef[3][8] ={
  /* 44.1 kHz noise shaping filter sd=2.51*/
  {2.2061f, -.4706f, -.2534f, -.6214f, 1.0587f, .0676f, -.6054f, -.2738f},
  /* 48.0kHz noise shaping filter sd=2.34*/
  {2.2374f, -.7339f, -.1251f, -.6033f, 0.9030f, .0116f, -.5853f, -.2571f},
  /* lowpass noise shaping filter sd=0.65*/
  {1.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f,0.0000f, 0.0000f, 0.0000f},
};

/* This implements a n-bit quantization with full triangular dither
   and IIR noise shaping. The noise shaping filters were designed by
   Sebastian Gesemann based on the LAME ATH curves with flattening
   to limit their peak gain to 20dB.

   The 48kHz version of this filter is just a warped version of the
   44.1kHz filter and probably could be improved by shifting the
   HF shelf up in frequency a little bit since 48k has a bit more
   room and being more conservative against bat-ears is probably
   more important than more noise suppression.
   This process can increase the peak level of the signal (in theory
   by the peak error of 1.5 +20dB though this much is unobservably rare).

   We're not bothering to guard against clipping here because this is
   in a demo and the condiitons are chosen so the we know we won't run
   into it.  It also saves a little noise in the non-dithered case as
   we won't be slightly shifting the quantzation scale */

void subquant_dither_to_X(shapestate *ss, float *data, int n,
                       float quant_bits){
  int i;
  if(quant_bits==0)quant_bits=24;
  int filter_choice = request_rate==44100?0:(request_rate==48000?1:2);
  const float *dither_fcoef = fcoef[filter_choice];
  float quant_gain = pow(2,quant_bits-1);
  float quant_invgain = 1./quant_gain;
  float quant_clamp = quant_gain;
  float dither_amp = request_dither_amplitude/65536.;

  /* don't shape if we don't have a filter */
  if(request_rate != 48000 && request_rate != 44100){
    /* pop the request button back out */
    request_dither_shaped=0;
    write(eventpipe[1],"\005",1);
  }

  float *b_buf=ss->b_buf;
  float *a_buf=ss->a_buf;
  int mute=ss->mute;

  if(mute>64)
    memset(a_buf,0,sizeof(float)*4);

  for(i=0;i<n;i++){
    int silent=data[i]==0;
    int j;
    float si,r=0,err=0;
    float s = data[i] * quant_gain;

    if(request_dither && request_dither_shaped)
      for(j=0;j<4;j++)
        err += dither_fcoef[j]*b_buf[j] - dither_fcoef[j+4]*a_buf[j];

    memmove(&a_buf[1],&a_buf[0],sizeof(float)*3);
    memmove(&b_buf[1],&b_buf[0],sizeof(float)*3);
    a_buf[0]=err;
    s = s - err;

    if(request_dither && mute<16)
      r=((float)fast_rand()*(1/(float)UINT_MAX) -
         (float)fast_rand()*(1/(float)UINT_MAX)) * dither_amp;

    /* no need to clamp; this is a demo, and we won't clip */
    si = rintf(s + r);

    /* output width may be higher than the subquant */
    data[i] = si*quant_invgain;

    if(request_dither_shaped)
      b_buf[0] = (mute>16)?0:si-s;
    else
      b_buf[0] = 0;

    mute++;
    if(!silent)mute=0;
  }
  ss->mute = (mute>960?960:mute);
}

