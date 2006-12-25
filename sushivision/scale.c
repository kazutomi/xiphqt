/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
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
#include "sushivision.h"
#include "scale.h"

/* slider scales */
static int trailing_zeroes(double A){
  int count=0;
  int64_t iA = (A<0 ? floor(-A) : floor(A));;
  if(iA==0)return 0;
  while(!(iA%10)){
    count++;
    iA/=10;
  }
  return count;
}

/* depth, at a minimum, must capture the difference between consecutive scale values */
int del_depth(double A, double B){
  int depth = 0;

  double del = B-A;
  if(del<0)del=-del;
  if(del != 0){
    if(del<1){
      float testdel=del;
      while(testdel<1){
	depth++;
	testdel = del * pow(10,depth);
      }
    }else{
      float testdel=del;
      while(testdel>10){
	depth--;
	testdel = del / pow(10,-depth);
      }
    }
  }
  return depth;
}

static int abs_depth(double A){
  int depth = 0;

  if(A<0)A=-A;
  if(A != 0){
    if(A<1){
      /* first ratchet down to find the beginning */
      while(A * pow(10.,depth) < 1)
	depth++;

      /* look for trailing zeroes; assume a reasonable max depth */
      depth+=5;

      A*=pow(10.,depth);
      int zero0 = trailing_zeroes(A);
      int zeroN = trailing_zeroes(A-1);
      int zeroP = trailing_zeroes(A+1);

      if(zero0 >= zeroN && zero0 >= zeroP)
	depth -= zero0;
      else if(zeroN >= zero0 && zeroN >= zeroP)
	depth -= zeroN;
      else 
	depth -=zeroP;

    }else
      // Max at five sig figs, less if trailing zeroes... */
      depth= 5-trailing_zeroes(A*100000.);
  }

  return depth;
}

static char *generate_label(double val,int depth){
  char *ret;
  
  if(depth>0){
    asprintf(&ret,"%.*f",depth,val);
  }else{
    asprintf(&ret,"%ld",(long)val);
  }
  return ret;
}

/* default scale label generation is hard, but fill it in in an ad-hoc
   fashion.  Add flags to help out later */
char **scale_generate_labels(unsigned scalevals, double *scaleval_list){
  unsigned i;
  int depth;
  char **ret;

  // default behavior; display each scale label at a uniform decimal
  // depth.  Begin by finding the smallest significant figure in any
  // label.  Since they're being passed in explicitly, they'll be
  // pre-hinted to the app's desires. Deal with rounding. */

  if(scalevals<2){
    fprintf(stderr,"Scale requires at least two scale values.");
    return NULL;
  }

  depth = del_depth(scaleval_list[0],scaleval_list[1]);

  for(i=2;i<scalevals;i++){
    int val = del_depth(scaleval_list[i-1],scaleval_list[i]);
    if(val>depth)depth=val;
  }
  
  for(i=0;i<scalevals;i++){
    int val = abs_depth(scaleval_list[i]);
    if(val>depth)depth=val;
  }
  
  ret = calloc(scalevals,sizeof(*ret));
  for(i=0;i<scalevals;i++)
    ret[i] = generate_label(scaleval_list[i],depth);
  
  return ret;
}

void scale_free(sushiv_scale_t *s){
  int i;
  
  if(s){
    if(s->val_list)free(s->val_list);
    if(s->label_list){
      for(i=0;i<s->vals;i++)
	free(s->label_list[i]);
      free(s->label_list);
    }
    free(s);
  }
}

sushiv_scale_t *scale_new(unsigned scalevals, double *scaleval_list){
  int i;

  sushiv_scale_t *s = NULL;

  if(scalevals<2){
    fprintf(stderr,"Scale requires at least two scale values.");
    return NULL;
  }

  s = calloc(1, sizeof(*s));
  
  // copy values
  s->vals = scalevals;
  s->val_list = calloc(scalevals,sizeof(*s->val_list));
  for(i=0;i<(int)scalevals;i++)
    s->val_list[i] = scaleval_list[i];

  // generate labels
  s->label_list = scale_generate_labels(scalevals,scaleval_list);

  return s;
}

int scale_set_scalelabels(sushiv_scale_t *s, char **scalelabel_list){
  int i;
  for(i=0;i<s->vals;i++){
    if(s->label_list[i])
      free(s->label_list[i]);
    s->label_list[i] = strdup(scalelabel_list[i]);
  }
  return 0;
}

/* plot and graph scales */

double scalespace_value(scalespace *s, double pixel){
  double val = (pixel-s->first_pixel)*s->step_val/s->step_pixel + s->first_val;
  return val * s->m;
}

double scalespace_pixel(scalespace *s, double val){
  val /= s->m;
  val -= s->first_val;
  val *= s->step_pixel;
  val /= s->step_val;
  val += s->first_pixel;

  return val;
}

int scalespace_mark(scalespace *s, int num){
  return s->first_pixel + s->step_pixel*num;
}

double scalespace_label(scalespace *s, int num, char *buffer){
  int pixel = scalespace_mark(s,num);
  double val = scalespace_value(s,pixel);

  if(s->decimal_exponent<0){
    sprintf(buffer,"%.*f",-s->decimal_exponent,val);
  }else{
    sprintf(buffer,"%.0f",val);
  }
  return val;
}

// name is *not* copied
scalespace scalespace_linear (double lowpoint, double highpoint, int pixels, int max_spacing, char *name){
  double range = fabs(highpoint - lowpoint);
  scalespace ret;

  int place = 0;
  int step = 1;
  long long first;
  int neg = (lowpoint>highpoint?-1:1);

  ret.lo = lowpoint;
  ret.hi = highpoint;
  ret.init = 1;
  ret.pixels=pixels;
  ret.legend=name;

  if(range == 0){
    ret.m = 0;
    return ret;
  }

  if(range!=0.){
    while(pixels / range < max_spacing){
      place++;
      range *= .1;
    }
    while(pixels / range > max_spacing){
      place--;
      range *= 10;
    }
  }

  ret.decimal_exponent = place;

  if (pixels / (range*.2) <= max_spacing){
    step *= 5;
    range *= .2;
  }
  if (pixels / (range*.5) <= max_spacing){
    step *= 2;
    range *= .5;
  }

  step *= neg;
  ret.step_val = step;

  if(pixels == 0. || range == 0.)
    ret.step_pixel = max_spacing;
  else
    ret.step_pixel = rint(pixels / range);
  ret.m = pow(10,place);
  first = (long long)(lowpoint/ret.m)/step*step;

  if(neg){
    if(LLONG_MAX * ret.m < highpoint){
      ret.m = 0;
      return ret;
    }
  }else{
    if(LLONG_MAX * ret.m < lowpoint){
      ret.m = 0;
      return ret;
    }
  }

  while(first * ret.m * neg < lowpoint*neg)
    first += step;

  if(neg){
    if(LLONG_MIN * ret.m > lowpoint){
      ret.m = 0;
      return ret;
    }
  }else{
    if(LLONG_MIN * ret.m > highpoint){
      ret.m = 0;
      return ret;
    }
  }

  while(first * ret.m * neg > highpoint*neg)
    first -= step;

  ret.first_val = first;

  ret.first_pixel = rint((first - (lowpoint / ret.m)) * ret.step_pixel / ret.step_val);

  return ret;
}
