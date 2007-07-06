/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007                   *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: scale inlines
 last mod: $Id$

 ********************************************************************/

#include <math.h>

#define MIN(a,b)  ((a)<(b) ? (a):(b))
#define MAX(a,b)  ((a)>(b) ? (a):(b))

static inline float todB(float x){
  return log(x*x+1e-80f)*4.34294480f;
}

static inline float fromdB(float x){
  return exp((x)*.11512925f);
}

static inline float toBark(float x){
  return 13.1f*atan(.00074f*x) + 2.24f*atan(x*x*1.85e-8f) + 1e-4f*x;
}

static inline float fromBark(float x){
  return 102.f*x - 2.f*pow(x,2.f) + .4f*pow(x,3.f) + pow(1.46f,x) - 1.f;
}

