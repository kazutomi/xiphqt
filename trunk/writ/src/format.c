/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggWrit SOFTWARE CODEC SOURCE CODE.     *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggWrit SOURCE CODE IS (C) COPYRIGHT 2003                    *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 format: Writ packet formatting
 last mod: $Id: format.c,v 1.1 2003/08/17 04:13:03 arc Exp $

 ********************************************************************/

/* This came directly from libvorbis mapping0.c, thanks Monty */
static int ilog(unsigned int v) {
  int ret = 0;
  if (v) --v;
  while (v) {
    ret++;
    v >>= 1;
  }
  return(ret);
}

/* Returns the number of bits needed to pad the current byte */
static int bitp(unsigned int t) {
  int ret = 0;

/* This is the formula in Python: ((((t-1)/8)+1)*8)-t 
   it works because everything is done on the int level */

  return(ret);
}
