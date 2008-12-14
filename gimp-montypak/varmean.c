/*
 *   Variance/mean collection helper for GIMP - The GNU Image Manipulation Program
 *
 *   Copyright 2008 Monty
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

static int collect_add_row(guchar *b, int *s, int *ss, int w, int h, int row, int x1, int x2){
  int i,j;
  if(row<0||row>=h)return 0;
  if(x1<0)x1=0;
  if(x2>w)x2=w;
  i=row*w+x1;
  j=row*w+x2;
  while(i<j){
    *s+=b[i];
    *ss+=b[i]*b[i];
    i++;
  }
  return(x2-x1);
}

static int collect_sub_row(guchar *b, int *s, int *ss, int w, int h, int row, int x1, int x2){
  int i,j;
  if(row<0||row>=h)return 0;
  if(x1<0)x1=0;
  if(x2>w)x2=w;
  i=row*w+x1;
  j=row*w+x2;
  while(i<j){
    *s-=b[i];
    *ss-=b[i]*b[i];
    i++;
  }
  return(x1-x2);
}

static int collect_add_col(guchar *b, int *s, int *ss, int w, int h, int col, int y1, int y2){
  int i,j;
  if(col<0||col>=w)return 0;
  if(y1<0)y1=0;
  if(y2>h)y2=h;
  i=y1*w+col;
  j=y2*w+col;
  while(i<j){
    *s+=b[i];
    *ss+=b[i]*b[i];
    i+=w;
  }
  return(y2-y1);
}

static int collect_sub_col(guchar *b, int *s, int *ss, int w, int h, int col, int y1, int y2){
  int i,j;
  if(col<0||col>=w)return 0;
  if(y1<0)y1=0;
  if(y2>h)y2=h;
  i=y1*w+col;
  j=y2*w+col;
  while(i<j){
    *s-=b[i];
    *ss-=b[i]*b[i];
    i+=w;
  }
  return(y1-y2);
}

static inline void collect_var(guchar *b, double *v, guchar *m, int w, int h, int n){
  int x,y;
  int sum=0;
  int ssum=0;
  int acc=0;
  double d = 1;

  /* prime */
  for(y=0;y<=n;y++)
    acc+=collect_add_row(b,&sum,&ssum,w,h,y,0,n+1);
  d=1./acc;

  for(x=0;x<w;){
    /* even x == increasing y */

    int mean = sum*d;
    if(m) m[x] = mean;
    v[x] = ssum*d - mean*mean;

    for(y=1;y<h;y++){
      int nn=collect_add_row(b,&sum,&ssum,w,h,y+n,x-n,x+n+1) +
	collect_sub_row(b,&sum,&ssum,w,h,y-n-1,x-n,x+n+1);
      if(nn){
	acc += nn;
	d = 1./acc;
      }
      
      mean = sum*d;
      if(m) m[y*w+x] = mean;
      v[y*w+x] = ssum*d - mean*mean;

    }

    x++;
    if(x>=w)break;

    acc+=collect_add_col(b,&sum,&ssum,w,h,x+n,h-n-1,h);
    acc+=collect_sub_col(b,&sum,&ssum,w,h,x-n-1,h-n-1,h);
    d=1./acc;

    /* odd x == decreasing y */
    mean = sum*d;
    if(m) m[(h-1)*w+x] = mean;
    v[(h-1)*w+x] = ssum*d - mean*mean;

    for(y=h-2;y>=0;y--){
      int nn=collect_add_row(b,&sum,&ssum,w,h,y-n,x-n,x+n+1) +
	collect_sub_row(b,&sum,&ssum,w,h,y+n+1,x-n,x+n+1);
      if(nn){
	acc += nn;
	d = 1./acc;
      }
      
      mean = sum*d;
      if(m) m[y*w+x] = mean;
      v[y*w+x] = ssum*d - mean*mean;
    }
    x++;
    acc+=collect_add_col(b,&sum,&ssum,w,h,x+n,0,n+1);
    acc+=collect_sub_col(b,&sum,&ssum,w,h,x-n-1,0,n+1);
    d=1./acc;

  }

}
