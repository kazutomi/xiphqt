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
#include "mapping.h"

static void scalloped_colorwheel(float val, float mul, accolor *r, ccolor *mix){
  int category = val*12.f;
  val = (val*12.f - category)*mul;
  
  switch(category){
  case 0:
    r->g+=val;break;
  case 1:
    r->r+=val; r->g+=val; break;
  case 2:
    r->r+=val; r->g+=val*.5f; break;
  case 3:
    r->r+=val; break;
  case 4:
    r->r+=val; r->b+=val*.75f; break;
  case 5:
    r->r+=val*.75; r->b+=val; break;
  case 6:
    r->r+=val*.5f; r->b+=val; break;
  case 7:
    r->b+=val; break;
  case 8:
    r->g+=val*.67f; r->b+=val; break;
  case 9:
    r->g+=val; r->b+=val; break;
  case 10:
    r->r+=val*.67f; r->g+=val; r->b+=val; break;
  case 11:
    r->r+=val; r->g+=val; r->b+=val; break;
  case 12:
    r->r+=mul; r->g+=mul; r->b+=mul; break;
  }
}

static void smooth_colorwheel(float val, float mul, accolor *r, ccolor *mix){
  if(val<= (4.f/7.f)){
    if(val<= (2.f/7.f)){
      if(val<= (1.f/7.f)){
	// 0->1, 0->g
	r->g+=7.f*val*mul;
	
      }else{
	// 1->2, g->rg
	r->r+=(7.f*val-1.f)*mul;
	r->g+=mul;
	
      }
    }else{
      if(val<=(3.f/7.f)){
	// 2->3, rg->r
	r->r+=mul;
	r->g+=(3.f - 7.f*val)*mul;
	
      }else{
	// 3->4, r->rb
	r->r+=mul;
	r->b+=(7.f*val-3.f)*mul;
      }
    }
  }else{
    if(val<= (5.f/7.f)){
      // 4->5, rb->b
      r->r+=(5.f - 7.f*val)*mul;
      r->b+=mul;
      
    }else{
      if(val<= (6.f/7.f)){
	// 5->6, b->bg
	r->g+=(7.f*val-5.f)*mul;
	r->b+=mul;
	
      }else{
	// 6->7, bg->rgb
	r->r+=(7.f*val-6.f)*mul;
	r->g+=mul;
	r->b+=mul;
	
      }
    }
  }
}

static void grayscale(float val, float mul, accolor *r, ccolor *mix){
  val*=mul;
  r->r += val;
  r->g += val;
  r->b += val;
}

static void red(float val, float mul, accolor *r, ccolor *mix){
  r->r += val*mul;
}

static void green(float val, float mul, accolor *r, ccolor *mix){
  r->g += val*mul;
}

static void blue(float val, float mul, accolor *r, ccolor *mix){
  r->b += val*mul;
}

static void red_overlay(float val, float mul, accolor *o, ccolor *mix){
  float r = mix->r + val;
  float g = mix->g;
  float b = mix->b;
  if(r>1.f){
    g -= r - 1.f;
    b -= r - 1.f;
    r  = 1.f;
    if(g<0.f)g=0.f;
    if(b<0.f)b=0.f;
  }
  o->r += r*mul;
  o->g += g*mul;
  o->b += b*mul;
}

static void green_overlay(float val, float mul, accolor *o, ccolor *mix){
  float r = mix->r;
  float g = mix->g + val;
  float b = mix->b;
  if(g>1.f){
    r -= g - 1.f;
    b -= g - 1.f;
    g  = 1.f;
    if(r<0.f)r=0.f;
    if(b<0.f)b=0.f;
  }
  o->r += r*mul;
  o->g += g*mul;
  o->b += b*mul;
}

static void blue_overlay(float val, float mul, accolor *o, ccolor *mix){
  float r = mix->r;
  float g = mix->g;
  float b = mix->b + val;
  if(b>1.f){
    r -= b - 1.f;
    g -= b - 1.f;
    b  = 1.f;
    if(r<0.f)r=0.f;
    if(g<0.f)g=0.f;
  }
  o->r += r*mul;
  o->g += g*mul;
  o->b += b*mul;
}

static void inactive(float val, float mul, accolor *r, ccolor *mix){
  r->r += mix->r*mul;
  r->g += mix->g*mul;
  r->b += mix->b*mul;
}

static void (*mapfunc[])(float, float, accolor *, ccolor *)={
  smooth_colorwheel,
  scalloped_colorwheel,
  grayscale,
  red,
  green,
  blue,
  red_overlay,
  green_overlay,
  blue_overlay,
  inactive
};

static char *mapnames[]={
  "smooth colorwheel",
  "scalloped colorwheel",
  "grayscale",
  "red",
  "green",
  "blue",
  "red overlay",
  "green overlay",
  "blue overlay",
  "inactive",
0
};

int num_mappings(){
  int i=0;
  while(mapnames[i])i++;
  return i;
}

char *mapping_name(int i){
  return mapnames[i];
}

void mapping_setup(mapping *m, float lo, float hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = mapfunc[funcnum];
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
}

float mapping_val(mapping *m, float in){
  if(m->i_range==0){
    return NAN;
  }else{
    return (in - m->low) * m->i_range;
  }
}

void mapping_calcf(mapping *m, float in, float mul, accolor *out, ccolor *mix){
  if(m->i_range==0){
    if(in<=m->low)
      m->mapfunc(0.f,mul,out,mix);
    else
      m->mapfunc(1.f,mul,out,mix);
  }else{
    float val = (in - m->low) * m->i_range;
    if(val<0.f)val=0.f;
    if(val>1.f)val=1.f;
    m->mapfunc(val,mul,out,mix);
  }
}

u_int32_t mapping_calc(mapping *m, float in, u_int32_t mix){
  ccolor mixc;
  accolor outc = {0.f,0.f,0.f,0.f};
  mixc.r = ((mix>>16)&0xff) * .0039215686f;
  mixc.g = ((mix>>8)&0xff) * .0039215686f;
  mixc.b = ((mix)&0xff) * .0039215686f;
  
  mapping_calcf(m,in,1.f,&outc,&mixc);

  return 
    ((u_int32_t)rint(outc.r * 0xff0000.p0f)&0xff0000) +
    ((u_int32_t)rint(outc.g *   0xff00.p0f)&0xff00) + 
     (u_int32_t)rint(outc.b *     0xff.p0f);
}

int mapping_inactive_p(mapping *m){
  if(m->mapfunc == inactive)return 1;
  return 0;
}

static void swhite(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val)*mul;
  r->g += (mix->g * (1.f-val) + val)*mul;
  r->b += (mix->b * (1.f-val) + val)*mul;
}

static void sred(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val)*mul;
  r->g += (mix->g * (1.f-val) + val*.376f)*mul;
  r->b += (mix->b * (1.f-val) + val*.376f)*mul;
}

static void sgreen(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val*.376f)*mul;
  r->g += (mix->g * (1.f-val) + val)*mul;
  r->b += (mix->b * (1.f-val) + val*.376f)*mul;
}

static void sblue(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val*.5f)*mul;
  r->g += (mix->g * (1.f-val) + val*.5f)*mul;
  r->b += (mix->b * (1.f-val) + val)*mul;
}

static void syellow(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val)*mul;
  r->g += (mix->g * (1.f-val) + val)*mul;
  r->b += (mix->b * (1.f-val))*mul;
}

static void scyan(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val*.376f)*mul;
  r->g += (mix->g * (1.f-val) + val)*mul;
  r->b += (mix->b * (1.f-val) + val)*mul;
}

static void spurple(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val)*mul;
  r->g += (mix->g * (1.f-val) + val*.376f)*mul;
  r->b += (mix->b * (1.f-val) + val)*mul;
}

static void sgray(float val, float mul, accolor *r, ccolor *mix){
  r->r += (mix->r * (1.f-val) + val*.627f)*mul;
  r->g += (mix->g * (1.f-val) + val*.627f)*mul;
  r->b += (mix->b * (1.f-val) + val*.627f)*mul;
}

static void (*solidfunc[])(float, float, accolor *, ccolor *)={
  swhite,
  sred,
  sgreen,
  sblue,
  syellow,
  scyan,
  spurple,
  sgray,
  inactive
};

static char *solidnames[]={
  "<span foreground=\"white\">white</span>",
  "<span foreground=\"red\">red</span>",
  "<span foreground=\"green\">green</span>",
  "<span foreground=\"blue\">blue</span>",
  "<span foreground=\"yellow\">yellow</span>",
  "<span foreground=\"cyan\">cyan</span>",
  "<span foreground=\"purple\">purple</span>",
  "<span foreground=\"gray\">gray</span>",
  "inactive",
  0
};

int num_solids(){
  int i=0;
  while(solidnames[i])i++;
  return i;
}

char *solid_name(int i){
  return solidnames[i];
}

void solid_setup(mapping *m, float lo, float hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = solidfunc[funcnum];
}

void solid_set_func(mapping *m, int funcnum){
  m->mapnum = funcnum;
  m->mapfunc = solidfunc[funcnum];
}
