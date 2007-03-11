/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "internal.h"

// val is .16; lcolor has pigment added in the range [0,mul]
static void scalloped_colorwheel(int val, int mul, lcolor *r){
  int category = val*12;
  if(category<0)return;
  
  val = ((category & 0xffff)*mul)>>16;
  category >>= 16;
  r->a += mul; 

  switch(category){
  case 0:
    r->g += val; break;
  case 1:
    r->r += val; r->g += val; break;
  case 2:
    r->r += val; r->g += val>>1; break;
  case 3:
    r->r += val; break;
  case 4:
    r->r += val; r->b += (val*3+2)>>2; break;
  case 5:
    r->r += (val*3+2)>>2; r->b += val; break;
  case 6:
    r->r += val>>1; r->b += val; break;
  case 7:
    r->b += val; break;
  case 8:
    r->g += (val*3+2)>>2; r->b += val; break;
  case 9:
    r->g += val; r->b+=val; break;
  case 10:
    r->r += val>>1; r->g += val; r->b += val; break;
  case 11:
    r->r += val; r->g += val; r->b += val; break;
  default:
    r->r += mul; r->g += mul; r->b += mul; break;
  }
}

static void smooth_colorwheel(int val, int mul, lcolor *r){
  if(val<0)return;
  r->a += mul;
  if(val < 37450){ // 4/7
    if(val < 18725){ // 2/7
      if(val < 9363){ // 1/7

	// 0->1, 0->g
	r->g+=(7L*val*mul)>>16;
	
      }else{

	// 1->2, g->rg
	r->r+=((7L*val - 65536)*mul)>>16;
	r->g+=mul;
	
      }
    }else{
      if(val < 28087){ // 3/7

	// 2->3, rg->r
	r->r+=mul;
	r->g+=((196608 - 7L*val)*mul)>>16;
	
      }else{

	// 3->4, r->rb
	r->r+=mul;
	r->b+=((7L*val - 196608)*mul)>>16;

      }
    }
  }else{
    if(val < 46812){ // 5/7

      // 4->5, rb->b
      r->r+=((327680 - 7L*val)*mul)>>16;
      r->b+=mul;
      
    }else{
      if(val < 56174){ // 6/7

	// 5->6, b->bg
	r->g+=((7L*val - 327680)*mul)>>16;
	r->b+=mul;
	
      }else{
	// 6->7, bg->rgb
	r->r+=((7L*val - 393216)*mul)>>16;
	r->g+=mul;
	r->b+=mul;
	
      }
    }
  }
}

static void white(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->r += val;
  r->g += val;
  r->b += val;
}

static void red(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->r += val;
}

static void green(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->g += val;
}

static void blue(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->b += val;
}
 
static void black_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
}

static void white_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
  r->g += val;
  r->b += val;
}

static void red_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
}

static void green_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->g += val;
}

static void blue_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val>>4;
  r->g += val>>4;
  r->b += val;
}

static void yellow_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
  r->g += val;
}

static void cyan_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->g += val;
  r->b += val;
}

static void purple_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
  r->b += val;
}

static void gray_a(int val, int mul, lcolor *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val>>1;
  r->g += val>>1;
  r->b += val>>1;
}
 
static void inactive(int val, int mul, lcolor *r){
  return;
}

// premultiplied alpha 
static ucolor normal_mix(ucolor in, ucolor bg){
  u_int32_t sa, s1, s2, d1, d2;
  if(in.c.a==255) return in;
  
  // taken from liboil
  sa = ~(in.u) >> 24;
  s1 = (in.u) & 0x00ff00ff;
  d1 = (bg.u) & 0x00ff00ff;
  d1 *= sa;
  d1 += 0x00800080;
  d1 += (d1 >> 8) & 0x00ff00ff;
  d1 >>= 8;
  d1 &= 0x00ff00ff;
  d1 += s1;
  d1 |= 0x01000100 - ((d1 >> 8) & 0x00ff00ff);
  d1 &= 0x00ff00ff;

  s2 = ((in.u) >> 8) & 0x00ff00ff;
  d2 = ((bg.u) >> 8) & 0x00ff00ff;
  d2 *= sa;
  d2 += 0x00800080;
  d2 += (d2 >> 8) & 0x00ff00ff;
  d2 >>= 8;
  d2 &= 0x00ff00ff;
  d2 += s2;
  d2 |= 0x01000100 - ((d2 >> 8) & 0x00ff00ff);
  d2 &= 0x00ff00ff;

  return (ucolor)(d1 | (d2 << 8));
}

static ucolor overlay_mix(ucolor in, ucolor bg){
  int r = bg.c.r + in.c.r;
  int g = bg.c.g + in.c.g;
  int b = bg.c.b + in.c.b;
  int ro = (r>255 ? r-255 : 0);
  int go = (g>255 ? g-255 : 0);
  int bo = (b>255 ? b-255 : 0);

  r-=(go+bo);
  g-=(ro+bo);
  b-=(ro+go);

  if(r>255) r = 255;
  if(g>255) g = 255;
  if(b>255) b = 255;
  if(r<0) r = 0;
  if(g<0) g = 0;
  if(b<0) b = 0;

  return (ucolor)(u_int32_t)((r<<16) + (g<<8) + b);
}

static ucolor inactive_mix(ucolor in, ucolor bg){
  return bg;
}

static void (*mapfunc[])(int, int, lcolor *)={
  smooth_colorwheel,
  scalloped_colorwheel,
  white,
  red,
  green,
  blue,
  red,
  green,
  blue,
  inactive
};

static ucolor (*mixfunc[])(ucolor, ucolor)={
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  overlay_mix,
  overlay_mix,
  overlay_mix,
  inactive_mix
};

static propmap *mapnames[]={
  &(propmap){"smooth colorwheel",0,    NULL, NULL, NULL},
  &(propmap){"scalloped colorwheel",1, NULL, NULL, NULL},
  &(propmap){"grayscale",2,            NULL, NULL, NULL},
  &(propmap){"red",3,                  NULL, NULL, NULL},
  &(propmap){"green",4,                NULL, NULL, NULL},
  &(propmap){"blue",5,                 NULL, NULL, NULL},
  &(propmap){"red overlay",6,          NULL, NULL, NULL},
  &(propmap){"green overlay",7,        NULL, NULL, NULL},
  &(propmap){"blue overlay",8,         NULL, NULL, NULL},
  &(propmap){"inactive",9,             NULL, NULL, NULL},
  NULL
};

int num_mappings(){
  int i=0;
  while(mapnames[i])i++;
  return i;
}

char *mapping_name(int i){
  return mapnames[i]->left;
}

propmap **mapping_map(){
  return mapnames;
}

void mapping_setup(mapping *m, float lo, float hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = mapfunc[funcnum];
  m->mixfunc = mixfunc[funcnum];
}

void mapping_set_lo(mapping *m, float lo){
  m->low = lo;
  if(m->high-m->low>0.)
    m->i_range = 1./(m->high-m->low);
  else
    m->i_range=0;
}

void mapping_set_hi(mapping *m, float hi){
  m->high=hi;
  if(m->high-m->low>0.)
    m->i_range = 1./(m->high-m->low);
  else
    m->i_range=0;
}

void mapping_set_func(mapping *m, int funcnum){
  m->mapnum = funcnum;
  m->mapfunc = mapfunc[funcnum];
  m->mixfunc = mixfunc[funcnum];
}

float mapping_val(mapping *m, float in){
  if(m->i_range==0){
    return NAN;
  }else{
    return (in - m->low) * m->i_range;
  }
}

u_int32_t mapping_calc(mapping *m, float in, u_int32_t mix){
  lcolor outc = {0,0,0,0};
  
  if(m->i_range==0){
    if(in<=m->low)
      m->mapfunc(0,255,&outc);
    else
      m->mapfunc(256*256,255,&outc);
  }else{
    float val = (in - m->low) * m->i_range;
    if(val<0.f)val=0.f;
    if(val>1.f)val=1.f;
    m->mapfunc(rint(val*65536.f),255,&outc);
  }

  return m->mixfunc( (ucolor)(u_int32_t)((outc.a<<24) + (outc.r<<16) + (outc.g<<8) + outc.b),
		     (ucolor)mix).u | 0xff000000;
}

int mapping_inactive_p(mapping *m){
  if(m->mapfunc == inactive)return 1;
  return 0;
}

static void (*mapsolid[])(int, int, lcolor *)={
  black_a,
  red_a,
  green_a,
  blue_a,
  yellow_a,
  cyan_a,
  purple_a,
  gray_a,
  white_a,
  inactive
};

static ucolor (*mixsolid[])(ucolor, ucolor)={
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  normal_mix,
  inactive_mix
};

static propmap *solidnames[]={
  &(propmap){"black",0,     NULL,NULL,NULL},
  &(propmap){"red",1,       NULL,NULL,NULL},
  &(propmap){"green",2,     NULL,NULL,NULL},
  &(propmap){"blue",3,      NULL,NULL,NULL},
  &(propmap){"yellow",4,    NULL,NULL,NULL},
  &(propmap){"cyan",5,      NULL,NULL,NULL},
  &(propmap){"purple",6,    NULL,NULL,NULL},
  &(propmap){"gray",7,      NULL,NULL,NULL},
  &(propmap){"white",8,     NULL,NULL,NULL},
  &(propmap){"inactive",9,  NULL,NULL,NULL},
  NULL
};

int num_solids(){
  int i=0;
  while(solidnames[i])i++;
  return i;
}

char *solid_name(int i){
  return solidnames[i]->left;
}

propmap **solid_map(){
  return solidnames;
}

void solid_setup(mapping *m, float lo, float hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = mapsolid[funcnum];
  m->mixfunc = mixsolid[funcnum];
}

void solid_set_func(mapping *m, int funcnum){
  m->mapnum = funcnum;
  m->mapfunc = mapsolid[funcnum];
  m->mixfunc = mixsolid[funcnum];
}
