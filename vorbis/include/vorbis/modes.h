/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: predefined encoding modes
 last mod: $Id: modes.h,v 1.10.2.1 2000/05/24 21:16:58 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp16_0.vqh"
#include "vorbis/book/lsp32_0.vqh"
#include "vorbis/book/resaux0_short.vqh"
#include "vorbis/book/resaux0_long.vqh"

#include "vorbis/book/res0a_0.vqh"
#include "vorbis/book/res0a_1.vqh"
#include "vorbis/book/res0a_2.vqh"
#include "vorbis/book/res0a_3.vqh"
#include "vorbis/book/res0a_4.vqh"

/* A farily high quality setting mix */
static vorbis_info_psy _psy_set0={
  1,/*athp*/
  1,/*decayp*/
  1,/*smoothp*/
  0,8,0.,

  -130.,

  1,/* tonemaskp */
  {-80.,-80.,-80.,-80.,-100.}, /* remember that el 0,2 is a 80 dB curve */
  {-35.,-40.,-60.,-80.,-80.}, /* remember that el 4 is an 80 dB curve, not 100 */
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},  

  1,/* peakattp */
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-8.,-10.,-12.,-16.,-18.},
  {-6.,-8.,-10.,-12.,-12.},

  1,/*noisemaskp */
  {-100.,-100.,-100.,-200.,-200.},
  {-100.,-100.,-100.,-200.,-200.},
  {-100.,-100.,-100.,-200.,-200.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-55.,-55.,-60.,-80.,-80.},

  100.,

  .9998, .9999  /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0={0};
static vorbis_info_floor0 _floor_set0={16, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1={32, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0={0,128, 16,8,2,
					   {0,2,3,4,6,6,16},
					   {1.5,1.5,2.5,3.5,3.5,5,11.5},
					   {16,4,4,4,4,2,2},
					   {0,1,1,1,1,1,1,1},
					   {4,5,6,7,8}};
static vorbis_info_residue0 _residue_set1={0,1024, 16,8,3,
					   {0,2,3,4,6,6,16},
					   {1.5,1.5,2.5,3.5,3.5,5,11.5},
					   {16,4,4,4,4,2,2},
					   {0,1,1,1,1,1,1,1},
					   {4,5,6,7,8}};
static vorbis_info_mapping0 _mapping_set0={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1={1, {0,0}, {0}, {1}, {1}, {0}};
static vorbis_info_mode _mode_set0={0,0,0,0};
static vorbis_info_mode _mode_set1={1,0,0,1};

/* CD quality stereo, no channel coupling */
vorbis_info info_A={
  /* channels, sample rate, upperkbps, nominalkbps, lowerkbps */
  0, 2, 44100, 0,0,0,
  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   9,   1,
  /* modes */
  {&_mode_set0,&_mode_set1},
  /* maps */
  {0,0},{&_mapping_set0,&_mapping_set1},
  /* times */
  {0,0},{&_time_set0},
  /* floors */
  {0,0},{&_floor_set0,&_floor_set1},
  /* residue */
  {0,0},{&_residue_set0,&_residue_set1},
  /* books */
  {&_vq_book_lsp16_0,      /* 0 */
   &_vq_book_lsp32_0,      /* 1 */

   &_huff_book_resaux0_short,
   &_huff_book_resaux0_long,

   &_vq_book_res0a_0,
   &_vq_book_res0a_1,
   &_vq_book_res0a_2,
   &_vq_book_res0a_3,
   &_vq_book_res0a_4,
  },
  /* psy */
  {&_psy_set0},
  /* thresh sample period, preecho clamp trigger threshhold, range */
  64, 4, 2 
};

#define PREDEF_INFO_MAX 0

#endif
