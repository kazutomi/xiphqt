/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#ifndef AUDIO_H
#define AUDIO_H

#include <qstring.h>
#include "arrays.h"

/*  this class is for calculating tone and noise and audio output */


class Audio
{
 public:
  Audio(QString);
  void sound(int kindoftest, int maskerfreqindex, int maskerampindex, int freqindex, float amp);
  void setzerodB(float);
  float getzerodB();

 private:
  void maketone(float, float, float *);
  void addtone(float, float, float *);
  void makenoise(float, float, float *);
  void addnoise(float, float, float *);
  void convert(float *calcbuf, short *outbuf);
  void audioout(short *);
  float zerodB;
  QString driver;
};


#endif
