/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#include <math.h>
#include <ao/ao.h>
#include "audio.h"

#define SAMPRATE 44100 //samples per second
#define DURATION 1 //in seconds


Audio::Audio(QString thedriver)
{
  driver = thedriver;
}

void Audio::sound(int kindoftest, int maskerfreqindex, int maskerampindex, int freqindex, float amp)
{  
  float calcbuf[SAMPRATE * DURATION];
  short outbuf[SAMPRATE * DURATION]; 

  if(kindoftest == 0)
    {
      maketone(freqarray[freqindex], amp, calcbuf);
    }
  else if(kindoftest == 1)
    {
      makenoise(freqarray[freqindex], amp, calcbuf);
    }
  else if(kindoftest == 2)
    {
      maketone(maskerfreqarray[maskerfreqindex], maskeramparray[maskerampindex], calcbuf);
      addtone(freqarray[freqindex], amp, calcbuf);
    }
  else if(kindoftest == 3)
    {
      makenoise(maskerfreqarray[maskerfreqindex], maskeramparray[maskerampindex], calcbuf);
      addtone(freqarray[freqindex], amp, calcbuf);
    }
  else if(kindoftest == 4)
    {
      maketone(maskerfreqarray[maskerfreqindex], maskeramparray[maskerampindex], calcbuf);
      addnoise(freqarray[freqindex], amp, calcbuf);
    }
  else if(kindoftest == 5)
    {
      makenoise(maskerfreqarray[maskerfreqindex], maskeramparray[maskerampindex], calcbuf);
      addnoise(freqarray[freqindex], amp, calcbuf);
    }
  else return;

  convert(calcbuf, outbuf);
  audioout(outbuf);
}

void Audio::maketone(float freq, float amp, float *buf)
{
  int n;
  amp = pow(10., (amp / 20. + log10(zerodB)));
  for(n = 0; n < SAMPRATE * DURATION; n++)
    {
      buf[n] = 0.5 * amp * sin(2.0 * M_PI * freq * n / SAMPRATE);
    }     
}

void Audio::addtone(float freq, float amp, float *buf)
{
  int n;
  amp = pow(10., (amp / 20. + log10(zerodB)));
  for(n = 0; n < SAMPRATE * DURATION / 4; n++)
      buf[n] += 0.5 * amp * sin(2.0 * M_PI * freq * n / SAMPRATE);
  for(n = SAMPRATE * DURATION / 2; n < SAMPRATE * DURATION * 3/4; n++)
      buf[n] += 0.5 * amp * sin(2.0 * M_PI * freq * n / SAMPRATE);
  // no window function here yet, that's why it clicks sometimes
}

//  the noise methods do the same as the tone methods at the moment, 
//  there's no noise generating algorithm yet
void Audio::makenoise(float freq, float amp, float *buf)
{
  int n;
  amp = pow(10., (amp / 20. + log10(zerodB)));
  for(n = 0; n < SAMPRATE * DURATION; n++)
    {
      buf[n] = 0.5 * amp * sin(2.0 * M_PI * freq * n / SAMPRATE);
    }     
}
void Audio::addnoise(float freq, float amp, float *buf)
{
  int n;
  amp = pow(10., (amp / 20. + log10(zerodB)));
  for(n = 0; n < SAMPRATE * DURATION / 4; n++)
      buf[n] += 0.5 * amp * sin(2.0 * M_PI * freq * n / SAMPRATE);
  for(n = SAMPRATE * DURATION / 2; n < SAMPRATE * DURATION * 3/4; n++)
      buf[n] += 0.5 * amp * sin(2.0 * M_PI * freq * n / SAMPRATE);
}

void Audio::convert(float *calcbuf, short *outbuf)
{
  for(int n = 0; n < SAMPRATE * DURATION; n++)
    outbuf[n] = (signed short) (32767.0 * calcbuf[n]);
}

//  this is how I understood libao, I was happy it works,
//  but some additions might be necessary
void Audio::audioout(short *outbuf)
{
  ao_option_t *options = NULL;
  ao_device_t *device;
  int driver_id;

  if(driver == "OSS")
    driver_id = ao_get_driver_id("oss");
  else if(driver == "Alsa")
    driver_id = ao_get_driver_id("alsa");
  else return;

  device = ao_open(driver_id, 16, SAMPRATE, 1, options);
  if(device)
    { 
      ao_play(device, outbuf, SAMPRATE * DURATION * sizeof(short));
      ao_close(device);
    }
}

void Audio::setzerodB(float z)
{
  zerodB = z;
}

float Audio::getzerodB()
{
  return zerodB;
}
