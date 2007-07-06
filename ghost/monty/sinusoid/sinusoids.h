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

 function: research-grade sinusoidal extraction sode
 last mod: $Id$

 ********************************************************************/

extern void level_mean(float *f,float *out,int n, float lowindow, float hiwindow, int min, int rate);
