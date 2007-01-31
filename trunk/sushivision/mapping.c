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

static u_int32_t scalloped_colorwheel(double val, u_int32_t mix){
  if(val<0.)val=0.;
  if(val>1.)val=1.;
  {
    int r,g,b;
    
    int category = val*12;
    int v = rint((val*12 - category)*255.);
    
    switch(category){
    case 0:
      r=0;g=v;b=0;break;
    case 1:
      r=v;g=v;b=0;break;
    case 2:
      r=v;g=(v>>1);b=0;break;
    case 3:
      r=v;g=0;b=0;break;
    case 4:
      r=v;g=0;b=v*3/4;break;
    case 5:
      r=v*3/4;g=0;b=v;break;
    case 6:
      r=(v>>1);g=0;b=v;break;
    case 7:
      r=0;g=0;b=v;break;
    case 8:
      r=0;g=v*2/3;b=v;break;
    case 9:
      r=0;g=v;b=v;break;
    case 10:
      r=v*2/3;g=v;b=v;break;
    case 11:
      r=v;g=v;b=v;break;
    case 12:
      r=255;g=255;b=255;break;
    }
    
    return (r<<16) + (g<<8) + b;
  }
}

static u_int32_t smooth_colorwheel(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r,g,b;

    if(val<= (4./7.)){
      if(val<= (2./7.)){
	if(val<= (1./7.)){
	  // 0->1, 0->g
	  r=0;
	  g= rint(7.*255.*val);
	  b=0;
	  
	}else{
	  // 1->2, g->rg
	  r= rint(7.*255.*val-255.);
	  g=255;
	  b=0;
	  
	}
      }else{
	if(val<=(3./7.)){
	  // 2->3, rg->r
	  r= 255;
	  g= rint(3.*255. - 7.*255.*val);
	  b=0;

	}else{
	  // 3->4, r->rb
	  r= 255;
	  g= 0.;
	  b= rint(7.*255.*val-3.*255.);
	}
      }
    }else{
      if(val<= (5./7.)){
	// 4->5, rb->b
	r= rint(5.*255. - 7.*255.*val);
	g= 0;
	b= 255;

      }else{
	if(val<= (6./7.)){
	  // 5->6, b->bg
	  r= 0.;
	  g= rint(7.*255.*val-5.*255.);
	  b= 255;
	  
	}else{
	  // 6->7, bg->rgb
	  r= rint(7.*255.*val-6.*255.);
	  g= 255;
	  b= 255;
	  
	}
      }
    }
    return (r<<16) + (g<<8) + b;
  }
}

static u_int32_t grayscale(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int g=rint(val*255.);
    return (g<<16)+(g<<8)+g;
  }
}

static u_int32_t grayscale_cont(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int g=rint(val*255.);
    if((g & 0xf) < 0x1) g=0;
    return (g<<16)+(g<<8)+g;
  }
}

static u_int32_t red(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int g=rint(val*255.);
    return (g<<16);
  }
}

static u_int32_t green(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int g=rint(val*255.);
    return (g<<8);
  }
}

static u_int32_t blue(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int g=rint(val*255.);
    return (g);
  }
}

static u_int32_t red_overlay(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) + (unsigned )(val*255.);
    int g = ((mix>>8) & 0xff);
    int b = ((mix) & 0xff);
    if(r>255){
      g -= (r-255);
      b -= (r-255);
      r=255;
      if(g<0)g=0;
      if(b<0)b=0;
    }
    return  (r<<16)+(g<<8)+b;
  }
}

static u_int32_t green_overlay(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff);
    int g = ((mix>>8) & 0xff) + (unsigned)(val*255.);
    int b = ((mix) & 0xff);
    if(g>255){
      r -= (g-255);
      b -= (g-255);
      g=255;
      if(r<0)r=0;
      if(b<0)b=0;
    }
    return  (r<<16)+(g<<8)+b;
  }
}

static u_int32_t blue_overlay(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff);
    int g = ((mix>>8) & 0xff);
    int b = ((mix) & 0xff) + (unsigned )(val*255.);
    if(b>255){
      r -= (b-255);
      g -= (b-255);
      b=255;
      if(r<0)r=0;
      if(g<0)g=0;
    }
    return  (r<<16)+(g<<8)+b;
  }
}

static u_int32_t inactive(double val, u_int32_t mix){
  return mix;
}

static u_int32_t (*mapfunc[])(double,u_int32_t)={
  smooth_colorwheel,
  scalloped_colorwheel,
  grayscale,
  grayscale_cont,
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
  "grayscale with contours",
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

void mapping_setup(mapping *m, double lo, double hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = mapfunc[funcnum];
}

void mapping_set_lo(mapping *m, double lo){
  m->low = lo;
  if(m->high-m->low>0.)
    m->i_range = 1./(m->high-m->low);
  else
    m->i_range=0;
}

void mapping_set_hi(mapping *m, double hi){
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

double mapping_val(mapping *m, double in){
  if(m->i_range==0){
    return NAN;
  }else{
    return (in - m->low) * m->i_range;
  }
}

u_int32_t mapping_calc(mapping *m, double in, u_int32_t mix){
  if(m->i_range==0){
    if(in<=m->low)
      return m->mapfunc(0.,mix);
    else
      return m->mapfunc(1.,mix);
  }else{
    double val = (in - m->low) * m->i_range;
    return m->mapfunc(val,mix);
  }
}

int mapping_inactive_p(mapping *m){
  if(m->mapfunc == inactive)return 1;
  return 0;
}

static u_int32_t swhite(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0xff);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0xff);
    int b = ((mix) & 0xff) *(1.-val) + (val*0xff);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t sred(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0xff);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0x60);
    int b = ((mix) & 0xff) *(1.-val) + (val*0x60);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t sgreen(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0x60);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0xff);
    int b = ((mix) & 0xff) *(1.-val) + (val*0x60);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t sblue(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0x80);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0x80);
    int b = ((mix) & 0xff) *(1.-val) + (val*0xff);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t syellow(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0xff);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0xff);
    int b = ((mix) & 0xff) *(1.-val);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t scyan(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0x60);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0xff);
    int b = ((mix) & 0xff) *(1.-val) + (val*0xff);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t spurple(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0xff);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0x60);
    int b = ((mix) & 0xff) *(1.-val) + (val*0xff);
    return  (r<<16)+(g<<8)+b;
  }
}
static u_int32_t sgray(double val, u_int32_t mix){
  if(val<0)val=0;
  if(val>1)val=1;
  {
    int r = ((mix>>16) & 0xff) *(1.-val) + (val*0xa0);
    int g = ((mix>>8) & 0xff) *(1.-val) + (val*0xa0);
    int b = ((mix) & 0xff) *(1.-val) + (val*0xa0);
    return  (r<<16)+(g<<8)+b;
  }
}

static u_int32_t (*solidfunc[])(double,u_int32_t)={
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

void solid_setup(mapping *m, double lo, double hi, int funcnum){
  m->low = lo;
  m->high = hi;
  m->i_range = 1./(hi-lo);
  m->mapfunc = solidfunc[funcnum];
}

void solid_set_func(mapping *m, int funcnum){
  m->mapnum = funcnum;
  m->mapfunc = solidfunc[funcnum];
}
