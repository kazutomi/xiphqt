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

#define _GNU_SOURCE
#include <math.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include "sushivision.h"
#include "scale.h"

/* slider scales */
void sv_scale_free(sv_scale_t *in){
  sv_scale_t *s = (sv_scale_t *)in;
  int i;
  
  if(s){
    if(s->val_list)free(s->val_list);
    if(s->label_list){
      for(i=0;i<s->vals;i++)
	free(s->label_list[i]);
      free(s->label_list);
    }
    if(s->legend)free(s->legend);
    free(s);
  }
}

sv_scale_t *sv_scale_new(char *arg){
  int i=0;
  _sv_tokenlist *t= _sv_tokenlistize(format);

  // sanity check the tokenization
  if(!t){
    fprintf(stderr,"sushivision: Unable to parse scale argument \"%s\".\n",arg);
    return NULL;
  }

  if(t->n != 1)
    fprintf(stderr,"sushivision: Ignoring excess arguments to scale.\n");

  if(
  
  sv_scale_t *s = calloc(1, sizeof(*s));
  s->vals = count;
  s->val_list = calloc(s->vals,sizeof(*s->val_list));
  s->label_list = calloc(s->vals,sizeof(*s->label_list));

  arg=format;
  while(arg && *arg){
    char *buf = strdup(arg);
    char *pos = strchr(buf,';');
    while(pos && *(pos-1)=='\\')
      pos = strchr(pos+1,';');
    if(!pos){
      arg=NULL;
    }else{
      arg+=pos-buf+1;
      pos[0]=0;
    }

    // an argument is a number and potentially a colon followed by an auxiliary label.
    pos = strchr(buf,':');
    if(pos){
      // we have a colon (label)
      s->label_list[i] = strdup(trim(pos+1));
      *pos='\0';
    }else{
      // we have only a number
      s->label_list[i] = strdup(trim(buf));
    }
    s->val_list[i]=atof_portable(buf);
    free(buf);
    
    i++;
  }

  if(legend)
    s->legend=strdup(legend);
  else
    s->legend=strdup("");

  return s;
}

sv_scale_t *sv_scale_copy(sv_scale_t *in){
  sv_scale_t *s = calloc(1, sizeof(*s));
  int i;

  s->vals = in->vals;
  s->val_list = calloc(s->vals,sizeof(*s->val_list));
  s->label_list = calloc(s->vals,sizeof(*s->label_list));
  s->legend = strdup(in->legend);

  for(i=0;i<s->vals;i++){
    s->val_list[i] = in->val_list[i];
    s->label_list[i] = strdup(in->label_list[i]);
  }
  return s;
}

/************************* plot and graph scalespaces *********************/

double _sv_scalespace_value(_sv_scalespace_t *s, double pixel){
  double val = (double)(pixel-s->first_pixel)*s->neg/s->step_pixel + s->first_val;
  return val * s->expm;
}

void _sv_scalespace_double(_sv_scalespace_t *s){
  s->two_exponent--;
  s->expm = pow(5,s->five_exponent) * pow(2,s->two_exponent);
  s->first_val *= 2;
  s->first_pixel *= 2;
  s->pixels *= 2;
}

double _sv_scalespace_pixel(_sv_scalespace_t *s, double val){
  val /= s->expm;
  val -= s->first_val;
  val *= s->step_pixel;
  val *= s->neg;
  val += s->first_pixel;

  return val;
}

double _sv_scalespace_scaledel(_sv_scalespace_t *from, _sv_scalespace_t *to){
  return from->expm * to->step_pixel * from->neg * to->neg / (from->step_pixel * to->expm);
}

long _sv_scalespace_scalenum(_sv_scalespace_t *from, _sv_scalespace_t *to){
  int five = from->five_exponent - to->five_exponent;
  int two = from->two_exponent - to->two_exponent;
  long ret = to->step_pixel;
  while(two-->0)
    ret *= 2;
  while(five-->0)
    ret *= 5;
  return ret*2;
}

long _sv_scalespace_scaleden(_sv_scalespace_t *from, _sv_scalespace_t *to){
  int five = to->five_exponent - from->five_exponent;
  int two = to->two_exponent - from->two_exponent;
  long ret = from->step_pixel;
  while(two-->0)
    ret *= 2;
  while(five-->0)
    ret *= 5;
  return ret*2;
}

long _sv_scalespace_scaleoff(_sv_scalespace_t *from, _sv_scalespace_t *to){
  int fiveF = from->five_exponent - to->five_exponent;
  int twoF = from->two_exponent - to->two_exponent;
  int fiveT = to->five_exponent - from->five_exponent;
  int twoT = to->two_exponent - from->two_exponent;
  long expF = 1;
  long expT = 1;

  while(twoF-->0) expF *= 2;
  while(fiveF-->0) expF *= 5;
  while(twoT-->0) expT *= 2;
  while(fiveT-->0) expT *= 5;
  
  return (2 * from->first_val * from->step_pixel * from->neg - (2 * from->first_pixel + 1)) 
    * expF * to->step_pixel

    - (2 * to->first_val * to->step_pixel * to->neg - (2 * to->first_pixel + 1)) 
    * expT * from->step_pixel;
}

long _sv_scalespace_scalebin(_sv_scalespace_t *from, _sv_scalespace_t *to){
  int fiveF = from->five_exponent - to->five_exponent;
  int twoF = from->two_exponent - to->two_exponent;
  int fiveT = to->five_exponent - from->five_exponent;
  int twoT = to->two_exponent - from->two_exponent;
  long expF = 1;
  long expT = 1;

  while(twoF-->0) expF *= 2;
  while(fiveF-->0) expF *= 5;
  while(twoT-->0) expT *= 2;
  while(fiveT-->0) expT *= 5;
  
  return (2 * from->first_val * from->step_pixel * from->neg - (2 * from->first_pixel)) 
    * expF * to->step_pixel

    - (2 * to->first_val * to->step_pixel * to->neg - (2 * to->first_pixel)) 
    * expT * from->step_pixel;
}

int _sv_scalespace_mark(_sv_scalespace_t *s, int num){
  return s->first_pixel + s->step_pixel*num;
}

int _sv_scalespace_decimal_exponent(_sv_scalespace_t *s){
  double val = s->two_exponent*.3 + s->five_exponent*.7;
  if(val<0){
    return (int)floor(val);
  }else{
    return (int)ceil(val);
  }
}

double _sv_scalespace_label(_sv_scalespace_t *s, int num, char *buffer){
  int pixel = _sv_scalespace_mark(s,num);
  double val = _sv_scalespace_value(s,pixel);
  int decimal_exponent = _sv_scalespace_decimal_exponent(s);
  if(decimal_exponent<0){
    sprintf(buffer,"%.*f",-decimal_exponent,val);
  }else{
    sprintf(buffer,"%.0f",val);
  }
  return val;
}

// name is *not* copied
_sv_scalespace_t _sv_scalespace_linear (double lowpoint, double highpoint, int pixels, int max_spacing, char *name){
  double orange = fabs(highpoint - lowpoint), range;
  _sv_scalespace_t ret;

  int five_place;
  int two_place;
  long long first;
  int neg = (lowpoint>highpoint?-1:1);

  if(pixels<1)pixels=1;

  memset(&ret,0,sizeof(ret)); // otherwise packing may do us in!

  ret.lo = lowpoint;
  ret.hi = highpoint;
  ret.init = 1;
  ret.pixels = pixels;
  ret.legend = name;
  ret.spacing = max_spacing;

  if(orange < 1e-30*pixels){
    // insufficient to safeguard the int64 first var below all by
    // itself, but it will keep things on track until later checks
    orange = 1e-30 * pixels;
    highpoint = lowpoint + orange;
    ret.massaged = 1;
  }

  while(1){
    range = orange;
    five_place = 0;
    two_place = 0;

    while(rint(pixels / range) < max_spacing){
      five_place++;
      two_place++;
      range *= .1;
    }
    while(rint(pixels / range) > max_spacing){
      five_place--;
      two_place--;
      range *= 10;
    }
    
    if (rint(pixels / (range*.2)) <= max_spacing){
      five_place++;
      range *= .2;
    }
    if (rint(pixels / (range*.5)) <= max_spacing){
      two_place++;
      range *= .5;
    }

    ret.two_exponent = two_place;
    ret.five_exponent = five_place;
    
    if(pixels == 0. || range == 0.)
      ret.step_pixel = max_spacing;
    else
      ret.step_pixel = rint(pixels / range);
    ret.expm = pow(2,two_place) * pow(5,five_place);
  
    first = (long long)(lowpoint/ret.expm);

    if(neg<0){
      /* overflow check */
      if(LLONG_MAX * ret.expm < highpoint){
	lowpoint += orange/2;
	orange *= 2;
	highpoint = lowpoint - orange;
	ret.massaged = 1;
	continue;
      }
    }else{
      /* overflow check */
      if(LLONG_MAX * ret.expm < lowpoint){
	lowpoint -= orange/2;
	orange *= 2;
	highpoint = lowpoint + orange;
	ret.massaged = 1;
	continue;
      }
    }

    while((first+neg) * ret.expm * neg <= lowpoint*neg)
      first += neg;
    
    if(neg<0){
      /* overflow check */
      if(LLONG_MIN * ret.expm > lowpoint){
	lowpoint += orange/2;
	orange *= 2;
	highpoint = lowpoint - orange;
	ret.massaged = 1;
	continue;
      }
    }else{
      /* overflow check */
      if(LLONG_MIN * ret.expm > highpoint){
	lowpoint -= orange/2;
	orange *= 2;
	highpoint = lowpoint + orange;
	ret.massaged = 1;
	continue;
      }
    }
    
    while((first-neg) * ret.expm * neg > highpoint*neg)
      first -= neg;
    
    // make sure the scale display has meaningful sig figs to work with */
    if( first*128/128 != first ||
	first*16*ret.expm == (first*16+neg)*ret.expm ||
	(first*16+neg)*ret.expm == (first*16+neg*2)*ret.expm){
      lowpoint -= orange/(neg*2);
      orange *= 2;
      highpoint = lowpoint + neg*orange;
      ret.massaged = 1;
      continue;
    }

    ret.first_val = first;
    
    ret.first_pixel = rint((first - (lowpoint / ret.expm)) * ret.step_pixel);

    ret.neg = neg;
    break;
  }

  return ret;
}
