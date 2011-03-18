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

 function: second order bessel filters
 last mod: $Id$

 ********************************************************************/

#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float c;
  float igain;
} bessel1;

typedef struct {
  float c[2];
  float igain;
} bessel2;

bessel2 mkbessel2(float alpha){
  bessel2 ret={{0}};
  float w = 2.f*tan(M_PIl * alpha);  
  float r = w * -1.10160133059;
  float i = w * .636009824757;
  float mag = 1.f/(4.f + (r-4.f)*r + i*i);
  float br = (4.f - r*r - i*i)*mag;
  float bi = 4.f*i*mag;

  ret.c[1] = -br*br - bi*bi;
  ret.c[0] = 2.f*br;
  ret.igain=(1.f-ret.c[0]-ret.c[1])*.25f;
  return ret;
}

bessel1 mkbessel1(float alpha){
  bessel1 ret={0};
  float r = -2.f*tan(M_PIl * alpha);
  float mag = 1.f/(4.f + (r-4.f)*r);
  ret.c = (4.f - r*r)*mag;
  ret.igain = (1.f-ret.c)*.5f;
  return ret;
}

//static inline float compbessel2(float x2, float x1, float x0, float y2, float y1, bessel2 *b){
//  return (x0+x1*2.+x2)*b->igain + y1*b->c1+y2*b->c2;
//}

static inline float bessel1_floor(float x1, float x0, float y1, bessel1 *a, bessel1 *b){
  float ay = (x0+x1)*a->igain + y1*a->c;
  //float by = (x0+x1)*b->igain + y1*b->c;
  float by = x0;
  return (ay<by ? ay : by);
}

