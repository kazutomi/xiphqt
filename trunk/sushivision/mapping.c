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
static void _sv_mapping_scalloped_colorwheel(int val, int mul, _sv_lcolor_t *r){
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

static void _sv_mapping_smooth_colorwheel(int val, int mul, _sv_lcolor_t *r){
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

static void _sv_mapping_white(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->r += val;
  r->g += val;
  r->b += val;
}

static void _sv_mapping_red(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->r += val;
}

static void _sv_mapping_green(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->g += val;
}

static void _sv_mapping_blue(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += mul;
  r->b += val;
}
 
static void _sv_mapping_black_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
}

static void _sv_mapping_white_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
  r->g += val;
  r->b += val;
}

static void _sv_mapping_red_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
}

static void _sv_mapping_green_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->g += val;
}

static void _sv_mapping_blue_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val>>4;
  r->g += val>>4;
  r->b += val;
}

static void _sv_mapping_yellow_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
  r->g += val;
}

static void _sv_mapping_cyan_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->g += val;
  r->b += val;
}

static void _sv_mapping_purple_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val;
  r->b += val;
}

static void _sv_mapping_gray_a(int val, int mul, _sv_lcolor_t *r){
  if(val<0)return;
  val = (val*mul)>>16;
  r->a += val;
  r->r += val>>1;
  r->g += val>>1;
  r->b += val>>1;
}
 
static void _sv_mapping_inactive(int val, int mul, _sv_lcolor_t *r){
  return;
}

// premultiplied alpha 
static _sv_ucolor_t _sv_mapping_normal_mix(_sv_ucolor_t in, _sv_ucolor_t bg){
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

  return (_sv_ucolor_t)(d1 | (d2 << 8));
}

static _sv_ucolor_t _sv_mapping_overlay_mix(_sv_ucolor_t in, _sv_ucolor_t bg){
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

  return (_sv_ucolor_t)(u_int32_t)((r<<16) + (g<<8) + b);
}

static _sv_ucolor_t _sv_mapping_inactive_mix(_sv_ucolor_t in, _sv_ucolor_t bg){
  return bg;
}

static void (*mapfunc[])(int, int, _sv_lcolor_t *)={
  _sv_mapping_smooth_colorwheel,
  _sv_mapping_scalloped_colorwheel,
  _sv_mapping_white,
  _sv_mapping_red,
  _sv_mapping_green,
  _sv_mapping_blue,
  _sv_mapping_red,
  _sv_mapping_green,
  _sv_mapping_blue,
  _sv_mapping_inactive
};

static _sv_ucolor_t (*mixfunc[])(_sv_ucolor_t, _sv_ucolor_t)={
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_overlay_mix,
  _sv_mapping_overlay_mix,
  _sv_mapping_overlay_mix,
  _sv_mapping_inactive_mix
};

static _sv_propmap_t *mapnames[]={
  &(_sv_propmap_t){"smooth colorwheel",0,    NULL, NULL, NULL},
  &(_sv_propmap_t){"scalloped colorwheel",1, NULL, NULL, NULL},
  &(_sv_propmap_t){"grayscale",2,            NULL, NULL, NULL},
  &(_sv_propmap_t){"red",3,                  NULL, NULL, NULL},
  &(_sv_propmap_t){"green",4,                NULL, NULL, NULL},
  &(_sv_propmap_t){"blue",5,                 NULL, NULL, NULL},
  &(_sv_propmap_t){"red overlay",6,          NULL, NULL, NULL},
  &(_sv_propmap_t){"green overlay",7,        NULL, NULL, NULL},
  &(_sv_propmap_t){"blue overlay",8,         NULL, NULL, NULL},
  &(_sv_propmap_t){"inactive",9,             NULL, NULL, NULL},
  NULL
};

int _sv_mapping_names(){
  int i=0;
  while(mapnames[i])i++;
  return i;
}

char *_sv_mapping_name(int i){
  return mapnames[i]->left;
}

_sv_propmap_t **_sv_mapping_map(){
  return mapnames;
}

void _sv_mapping_setup(_sv_mapping_t *m, float lo, float hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = mapfunc[funcnum];
  m->mixfunc = mixfunc[funcnum];
}

void _sv_mapping_set_lo(_sv_mapping_t *m, float lo){
  m->low = lo;
  if(m->high-m->low>0.)
    m->i_range = 1./(m->high-m->low);
  else
    m->i_range=0;
}

void _sv_mapping_set_hi(_sv_mapping_t *m, float hi){
  m->high=hi;
  if(m->high-m->low>0.)
    m->i_range = 1./(m->high-m->low);
  else
    m->i_range=0;
}

void _sv_mapping_set_func(_sv_mapping_t *m, int funcnum){
  m->mapnum = funcnum;
  m->mapfunc = mapfunc[funcnum];
  m->mixfunc = mixfunc[funcnum];
}

float _sv_mapping_val(_sv_mapping_t *m, float in){
  if(m->i_range==0){
    return NAN;
  }else{
    return (in - m->low) * m->i_range;
  }
}

u_int32_t _sv_mapping_calc(_sv_mapping_t *m, float in, u_int32_t mix){
  _sv_lcolor_t outc = {0,0,0,0};
  
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

  return m->mixfunc( (_sv_ucolor_t)(u_int32_t)((outc.a<<24) + (outc.r<<16) + (outc.g<<8) + outc.b),
		     (_sv_ucolor_t)mix).u | 0xff000000;
}

int _sv_mapping_inactive_p(_sv_mapping_t *m){
  if(m->mapfunc == _sv_mapping_inactive)return 1;
  return 0;
}

static void (*mapsolid[])(int, int, _sv_lcolor_t *)={
  _sv_mapping_black_a,
  _sv_mapping_red_a,
  _sv_mapping_green_a,
  _sv_mapping_blue_a,
  _sv_mapping_yellow_a,
  _sv_mapping_cyan_a,
  _sv_mapping_purple_a,
  _sv_mapping_gray_a,
  _sv_mapping_white_a,
  _sv_mapping_inactive
};

static _sv_ucolor_t (*mixsolid[])(_sv_ucolor_t, _sv_ucolor_t)={
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_normal_mix,
  _sv_mapping_inactive_mix
};

static _sv_propmap_t *solidnames[]={
  &(_sv_propmap_t){"black",0,     NULL,NULL,NULL},
  &(_sv_propmap_t){"red",1,       NULL,NULL,NULL},
  &(_sv_propmap_t){"green",2,     NULL,NULL,NULL},
  &(_sv_propmap_t){"blue",3,      NULL,NULL,NULL},
  &(_sv_propmap_t){"yellow",4,    NULL,NULL,NULL},
  &(_sv_propmap_t){"cyan",5,      NULL,NULL,NULL},
  &(_sv_propmap_t){"purple",6,    NULL,NULL,NULL},
  &(_sv_propmap_t){"gray",7,      NULL,NULL,NULL},
  &(_sv_propmap_t){"white",8,     NULL,NULL,NULL},
  &(_sv_propmap_t){"inactive",9,  NULL,NULL,NULL},
  NULL
};

int _sv_solid_names(){
  int i=0;
  while(solidnames[i])i++;
  return i;
}

char *_sv_solid_name(int i){
  return solidnames[i]->left;
}

_sv_propmap_t **_sv_solid_map(){
  return solidnames;
}

void _sv_solid_setup(_sv_mapping_t *m, float lo, float hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = mapsolid[funcnum];
  m->mixfunc = mixsolid[funcnum];
}

void _sv_solid_set_func(_sv_mapping_t *m, int funcnum){
  m->mapnum = funcnum;
  m->mapfunc = mapsolid[funcnum];
  m->mixfunc = mixsolid[funcnum];
}
