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

 format.c: Writ formatting functions
 last mod: $Id: format.c,v 1.3 2003/12/09 06:38:44 arc Exp $

 ********************************************************************/

#include <writ/writ.h>

static int ilog(unsigned int v) {
  int ret = 0;
  if (v) --v;
  while (v) {
    ret++;
    v >>= 1;
  }
  return(ret);
}

