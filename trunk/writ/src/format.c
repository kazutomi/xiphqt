/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Writ SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Ogg Writ SOURCE CODE IS (C) COPYRIGHT 2003                   *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 format: Writ data formatting
 last mod: $Id: format.c,v 1.2 2003/08/17 08:34:49 arc Exp $

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

/* this is undocumented in libogg, and very needed here
void oggpack_writealign(oggpack_buffer *b);
*/
