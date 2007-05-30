/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "graph.h"
#include "graph_region.h"
#include "gettext.h"

/* Regions are 'electric fences' for mesh building; used in mesh2 to
   make non-convex shapes */

typedef struct region_segment {
  int layout; /* 0 no layout, 1 left, 2 right, 3 layout-only */
  int cont;   /* is this continuous from last line? */
  int split;  /* are we splitting the graph into interntionally
		 seperate regions here? */

  float x1;
  float y1;
  float x2;
  float y2;

  // arc computation cache (if arc)
  float cx;
  float cy;
  float radius;
  float phi0;
  float phi1;
  float phi;

  float length;
  struct region_segment *next;
} region_segment;

typedef struct region{
  int num;
  region_segment *l;

  int ox,oy,x,y;
  int layout;

  int cont;
  int split_next;
} region;

static region r;
static region layout_adj;
static region_segment *segpool=0;

#define CHUNK 64

static region_segment *new_segment(region *r, int x1,int y1,int x2, int y2){
  region_segment *ret;
  
  if(!segpool){
    int i;
    segpool = calloc(CHUNK,sizeof(*segpool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      segpool[i].next=segpool+i+1;
  }

  ret=segpool;
  segpool=ret->next;

  memset(ret,0,sizeof(*ret));
  ret->next = r->l;
  ret->layout=r->layout;
  ret->x1=x1;
  ret->y1=y1;
  ret->x2=x2;
  ret->y2=y2;
  ret->cont = r->cont;
  ret->split = r->split_next;

  r->l=ret;
  r->split_next=0;

  return ret;
}

/* angle convention: reversed y (1,-1 is first quadrant, ala X
   windows), range -PI to PI */

static int intersects_arc(edge *e, region_segment *r){
  float Ax = e->A->x - r->cx;
  float Ay = e->A->y - r->cy;
  float Bx = e->B->x - r->cx;
  float By = e->B->y - r->cy;

  float dx = Bx - Ax;
  float dy = By - Ay;
  float dr2 = dx*dx + dy*dy;
  float D = Ax*By - Bx*Ay;
  float discriminant =(r->radius*r->radius)*dr2 - D*D; 

  // does it even intersect the full circle?
  if(discriminant<=0)return 0;
  
  {
    float x1,y1,x2,y2;
    
    float sqrtd = sqrt(discriminant);
    float sign = (dy>0?1.f:-1.f);

    // finite precision required here else 0/inf slope lines will be
    // slighly off the secant
    x1 = rint((D*dy + sign*dx*sqrtd) / dr2);
    x2 = rint((D*dy - sign*dx*sqrtd) / dr2);

    y1 = rint((-D*dx + fabs(dy)*sqrtd) / dr2);
    y2 = rint((-D*dx - fabs(dy)*sqrtd) / dr2);

    Ax = rint(Ax);
    Ay = rint(Ay);
    Bx = rint(Bx);
    By = rint(By);

    // is x1,y1 actually on the segment?
    if( !(x1<Ax && x1<Bx) &&
	!(x1>Ax && x1>Bx) &&
	!(y1<Ay && y1<By) &&
	!(y1>Ay && y1>By)){
      // yes. it is in the angle range we care about?

      float ang = acos(x1 / r->radius);
      if(y1>0) ang = -ang;
      
      if(r->phi<0){
	if(r->phi0 < r->phi1){
	  if(ang <= r->phi0 || ang >= r->phi1)return 1;
	}else{
	  if(ang <= r->phi0 && ang >= r->phi1)return 1;
	}
      }else{
	if(r->phi0 < r->phi1){
	  if(ang >= r->phi0 && ang <= r->phi1)return 1;
	}else{
	  if(ang >= r->phi0 || ang <= r->phi1)return 1;
	}
      }
    }

    // is x2,y2 actually on the segment?
    // if so, it is in the arc range we care about?
    if( !(x2<Ax && x2<Bx) &&
	!(x2>Ax && x2>Bx) &&
	!(y2<Ay && y2<By) &&
	!(y2>Ay && y2>By)){
      // yes. it is in the angle range we care about?

      float ang = acos(x2 / r->radius);
      if(y2>0) ang = -ang;
      
      if(r->phi<0){
	if(r->phi0 < r->phi1){
	  if(ang <= r->phi0 || ang >= r->phi1)return 1;
	}else{
	  if(ang <= r->phi0 && ang >= r->phi1)return 1;
	}
      }else{
	if(r->phi0 < r->phi1){
	  if(ang >= r->phi0 && ang <= r->phi1)return 1;
	}else{
	  if(ang >= r->phi0 || ang <= r->phi1)return 1;
	}
      }
    }
  }
  return 0;
}

static float line_angle(float x1, float y1, float x2, float y2){
  float xd = x2-x1;
  float yd = y2-y1;
  
  if(xd == 0){
    if(yd>0)
      return -M_PI/2;
    else
      return M_PI/2;
  }else if(xd<0){
    if(yd<0)
      return atan(-yd/xd)+M_PI;
    else
      return  atan(-yd/xd)-M_PI;
  }else{
    return atan(-yd/xd);
  }
}

static float line_mag(float x1, float y1, float x2, float y2){
  float xd = x2-x1;
  float yd = y2-y1;
  return hypot(xd,yd);
}

static void compute_arc(region_segment *r,float phi){
  float x1=r->x1;
  float y1=r->y1;
  float x2=r->x2;
  float y2=r->y2;
  float ar,br,cr;
  float cx,cy,a,c,d;
  float xd = x2-x1;
  float yd = y2-y1;
  
  if(phi<-M_PI){
    ar = phi + M_PI*2;
  }else if (phi<0){
    ar = -phi;
  }else if (phi<M_PI){
    ar = phi;
  }else{
    ar = M_PI*2 - phi;
  }

  cr = line_angle(x1,y1,x2,y2);
  a = line_mag(x1,y1,x2,y2)/2.f;
  br=(M_PI/2.f)-(ar/2.f); 
  c = tan(br)*a;
  d = hypot(a,c);
  
  if(phi<-M_PI || (phi>0 && phi<M_PI)){
    cx = x1 + cos(cr+M_PI/2)*c + xd/2;
    cy = y1 - sin(cr+M_PI/2)*c + yd/2;
  }else{
    cx = x1 + cos(cr-M_PI/2)*c + xd/2;
    cy = y1 - sin(cr-M_PI/2)*c + yd/2;
  }
  
  r->cx=cx;
  r->cy=cy;
  r->radius=d;
  
  // have the center of the circle, have radius.  Determine the
  // portion of the arc we want.
  r->phi0 = acos( (x1-cx) / d);
  r->phi1 = acos( (x2-cx) / d);
  if(y1>cy) r->phi0= -r->phi0;
  if(y2>cy) r->phi1= -r->phi1;
  r->phi=phi;
}

static region_segment *region_arc(region *re, int x1, int y1, int x2, int y2, float rad){
  region_segment *n=  new_segment(re,x1,y1,x2,y2);
  compute_arc(n,rad);
  return n;
}

static region_segment *region_line(region *re,int x1, int y1, int x2, int y2){
  return new_segment(re,x1,y1,x2,y2);
}

/* The overall idea here is to massage the region segments and arcs
   slightly so that when we layout points based on a region, the
   layout is slightly inside or outside (as requested) the actual
   region. This also reverses the path when rebuilding into the new
   region, putting it in the order we actually need to evaluate it in.
*/ 

#define ADJ 2.f

static void point_adj(float x1, float y1, float x2, float y2, float *Px, float *Py, int left){
  float xd = x2-x1;
  float yd = y2-y1;
  float M = hypot(xd,yd);

  if(left){
    *Px +=  yd/M*ADJ;
    *Py += -xd/M*ADJ;
  }else{
    *Px += -yd/M*ADJ;
    *Py +=  xd/M*ADJ;
  }
}

static void line_adj(float *x1, float *y1, float *x2, float *y2, int left){
  float xd = *x2-*x1;
  float yd = *y2-*y1;
  float M = hypot(xd,yd);

  if(left){
    *x1 +=  yd/M*ADJ;
    *x2 +=  yd/M*ADJ;
    *y1 += -xd/M*ADJ;
    *y2 += -xd/M*ADJ;
  }else{
    *x1 += -yd/M*ADJ;
    *x2 += -yd/M*ADJ;
    *y1 +=  xd/M*ADJ;
    *y2 +=  xd/M*ADJ;
  }

  // make sure there's an overlap!
  *x1-=xd/M*4.;
  *x2+=xd/M*4.;
  *y1-=yd/M*4.;
  *y2+=yd/M*4.;
}


static float tangent_distance_from_center(float x1, float y1, float x2, float y2, 
					  float cx, float cy){
  float xd = x2 - x1;
  float yd = y2 - y1;
  return ((x2-x1)*(cy-y1) - (y2-y1)*(cx-x1)) / hypot(xd,yd);
}

static float radius_adjust(float r, float arc_phi, int left){
  if(arc_phi<0){
    if(left){
      r+=ADJ;
    }else{
      r-=ADJ;
    }
  }else{
    if(left){
      r-=ADJ;
    }else{
      r+=ADJ;
    }
  }
  return r;
}

static void line_line_adj(region_segment *A, region_segment *B,
			  float *new_x, float *new_y, int left){
  double newd_x;
  double newd_y;

  float Ax1=A->x1;
  float Ay1=A->y1;
  float Ax2=A->x2;
  float Ay2=A->y2;

  float Bx1=B->x1;
  float By1=B->y1;
  float Bx2=B->x2;
  float By2=B->y2;

  line_adj(&Ax1, &Ay1, &Ax2, &Ay2, left);
  line_adj(&Bx1, &By1, &Bx2, &By2, left);

  // compute new intersection
  if(!intersects(Ax1,Ay1,Ax2,Ay2, Bx1,By1,Bx2,By2, &newd_x, &newd_y)){
    // odd; do nothing rather than fail unpredictably
    *new_x=Ax2;
    *new_y=Ay2;
  }else{
    *new_x=newd_x;
    *new_y=newd_y;
  }
}

static void line_arc_adj(float x1, float y1, float x2, float y2, 
			 float cx, float cy, float r, float arc_phi,
			 float *new_x2, float *new_y2, int lleft, int aleft){
  float xd = x2 - x1;
  float yd = y2 - y1;
  float c = tangent_distance_from_center(x1,y1,x2,y2,cx,cy);
  float a = sqrt(r*r - c*c),a2;
  float M = hypot(xd,yd);
  float ax = x2 + xd/M*a;
  float ay = y2 + yd/M*a;

  float ax1 = x2 - xd/M*a;
  float ay1 = y2 - yd/M*a;
  if(hypot(ax1-cx,ay1-cy) < hypot(ax-cx,ay-cy)){
    ax=ax1;
    ay=ay1;
  }
  
  xd = ax-x2;
  yd = ay-y2;

  point_adj(x1, y1, x2, y2, &ax, &ay, lleft);

  r = radius_adjust(r,arc_phi,aleft);
  c = hypot(cx-ax,cy-ay);
  a2 = sqrt(r*r-c*c);

  *new_x2 = ax - xd/a*a2; 
  *new_y2 = ay - yd/a*a2; 
}

static void arc_arc_adj(region_segment *arc, region_segment *next,
			 float *new_x2, float *new_y2, int left){
  float x2 =arc->x2;
  float y2 =arc->y2;

  float cx1=arc->cx;
  float cy1=arc->cy;
  float r1 =arc->radius;

  float cx2=next->cx;
  float cy2=next->cy;
  float r2 =next->radius;

  float c;
  float xd = cx2-cx1;
  float yd = cy2-cy1;
  float d = hypot(xd,yd);
  float x = (d*d - r1*r1 + r2*r2) / (2*d);

  // is the old x2/y2 to the left or right of the line connecting the
  // circle centers?
  float angle_x2y2 = line_angle(cx1,cy1,x2,y2);
  float angle_c1c2 = line_angle(cx1,cy1,cx2,cy2);
  float angle = angle_x2y2 - angle_c1c2;
  if(angle < -M_PI)angle += M_PI*2.f;
  if(angle >  M_PI)angle -= M_PI*2.f;

  r1=radius_adjust(r1,arc->phi,left);
  r2=radius_adjust(r2,arc->phi,left);
  
  if(r1+r2>=d){
    // still have a valid solution
    x = (d*d - r1*r1 + r2*r2) / (2*d);
    c = sqrt(r2*r2 - x*x);

    if(angle>0){
      // left of c1,c2 segment
      *new_x2 = cx1+xd/d*x + yd/d*c;
      *new_y2 = cy1+yd/d*x - xd/d*c;
    }else{
      // right
      *new_x2 = cx1+xd/d*x - yd/d*c;
      *new_y2 = cy1+yd/d*x + xd/d*c;
    }
  }else{
    // circles shrunk and no longer overlap.  
    fprintf(stderr,_("region overlap adjustment failed in arc_arc_adj; \n"
	    "  This is an internal error that should never happen.\n"));
  }
}

static float phisub(float phi0, float phi1, float arcphi){
  float phid = phi1-phi0;
  if(arcphi<0){
    if(phid>0) phid -= M_PI*2.f;
  }else{
    if(phid<0) phid += M_PI*2.f;
  }
  return phid;
}

static void radius_adj_xy(region_segment *s,float *x1,float *y1, int left){
  float xd = *x1 - s->cx;
  float yd = *y1 - s->cy;

  float r = s->radius;
  float new_r = radius_adjust(r,s->phi,left);
  float delta = new_r/r;

  *x1 = s->cx + xd*delta;
  *y1 = s->cy + yd*delta;
}

static void adjust_layout(){
  /* build adjustments from intersection region into layout region */
  region_segment *s = r.l;
  region_segment *endpath = 0;
  region_segment *endpath_adj = 0;
  float x2=-1,y2=-1;

  // first, release previous layout
  region_segment *l=layout_adj.l;
  region_segment *le=l;

  while(le && le->next)le=le->next;
  if(le){
    le->next=segpool;
    segpool=l;
  }
  memset(&layout_adj,0,sizeof(layout_adj));

  while(s){
    float x1=0,y1=0,phi=0,radius=0;

    if(s->cont){
      if(!endpath){
	endpath=s;
	endpath_adj=0;
      }
    }
    
    if(s->layout){
      if(s->layout<3){
	
	// the flags mark beginning and end of the path, but don't say
	// if it's closed.
	if(!s->cont && endpath_adj)
	  if(endpath->x2 != s->x1 ||
	     endpath->y2 != s->y1)
	    endpath_adj=0;
	
	/* first handle the lone-circle special case */
	if(s->x1==s->x2 && s->y1==s->y2){
	  if(s->radius>0){
	    if(s->layout == 1) radius= s->radius+2;
	    if(s->layout == 2) radius= s->radius-2;
	    x1=x2=s->x1;y1=y2=s->y1;
	    phi=s->phi;
	  }
	}else{
	  region_segment *p = 0;

	  if(s->cont)
	    p = s->next;
	  else if(endpath_adj)
	    p = endpath;

	  if(p){
	    if(s->radius){
	      if(x2==-1){
		x2=s->x2;
		y2=s->y2;
		radius_adj_xy(s,&x2,&y2,s->layout==1);
	      }
	      if(p->radius){
		// arc - arc case
		float phi0,phi1;
		arc_arc_adj(p,s,&x1,&y1,s->layout==1);
		phi0=line_angle(s->cx,s->cy,x1,y1);
		phi1=line_angle(s->cx,s->cy,x2,y2);
		phi=phisub(phi0,phi1,s->phi);
	      }else{
		// arc-line case
		float phi0,phi1;
		line_arc_adj(p->x1, p->y1, p->x2, p->y2, 
			     s->cx, s->cy, s->radius, s->phi,
			     &x1, &y1, s->layout==1, s->layout==1);
		phi0=line_angle(s->cx,s->cy,x1,y1);
		phi1=line_angle(s->cx,s->cy,x2,y2);
		phi=phisub(phi0,phi1,s->phi);
	      }
	    }else{
	      if(x2==-1){
		x2=s->x2;
		y2=s->y2;
		point_adj(s->x1, s->y1, s->x2, s->y2, &x2, &y2, s->layout==1);
	      }
	      if(p->radius){
		// line-arc case
		line_arc_adj(s->x2, s->y2, s->x1, s->y1, 
			     p->cx, p->cy, p->radius, p->phi,
			     &x1, &y1, s->layout==2, s->layout==1);
	      }else{
		// line-line case
		line_line_adj(p, s, &x1, &y1, s->layout==1);
	      }
	    }
	  }else{
	    x1=s->x1;
	    y1=s->y1;
	    x2=s->x2;
	    y2=s->y2;
	    if(s->radius){
	      // lone arc case; alter radius
	      radius_adj_xy(s,&x1,&y1,s->layout==1);
	      radius_adj_xy(s,&x2,&y2,s->layout==1);
	      phi=s->phi;
	    }else{
	      // lone line segment case; offset
	      point_adj(s->x1, s->y1, s->x2, s->y2, &x1, &y1, s->layout==1);
	      point_adj(s->x1, s->y1, s->x2, s->y2, &x2, &y2, s->layout==1);
	      
	    }
	  }
	}
      }else{
	x1=s->x1;
	x2=s->x2;
	y1=s->y1;
	y2=s->y2;
	phi=s->phi;
	if(x1==x2 && y1==y2)
	  radius = s->radius;
      }

      // push the region segment
      {
	region_segment *n=new_segment(&layout_adj,rint(x1),rint(y1),rint(x2),rint(y2));
	n->layout=3;
	n->cont=(s->cont || endpath_adj);
	n->split = s->split;

	if(radius){
	  // circle; radius variable is treated as a flag
	  n->cx=x1;
	  n->cy=y1;
	  n->radius=radius;
	  n->phi0=-M_PI;
	  n->phi1= M_PI;
	  n->cont=1;
	}else if(s->radius){
	  // arc
	  compute_arc(n,phi);
	}
	if(s->cont && !endpath_adj)endpath_adj=n;	

      }

      if(endpath_adj && !s->cont){
	// go back and clean up the endpath path member
	endpath_adj->x2 = rint(x1);
	endpath_adj->y2 = rint(y1);
	
	if(endpath->radius>0){
	  endpath_adj->phi1=line_angle(endpath->cx,endpath->cy,endpath_adj->x2,endpath_adj->y2);
	  endpath_adj->phi=phisub(endpath_adj->phi0,endpath_adj->phi1,endpath_adj->phi);
	}
      }
    }
    
    if(!s->cont){
      endpath_adj=0;
      endpath=0;
      x2=-1;
      y2=-1;
    }else{
      x2=x1;
      y2=y1;
    }

    s=s->next;
  }
}

void region_init(){
  // release any lines and arcs
  region_segment *l=r.l;
  region_segment *le=r.l;
  region_segment *a=layout_adj.l;
  region_segment *ae=layout_adj.l;

  while(le && le->next)le=le->next;
  while(ae && ae->next)ae=ae->next;
    
  if(le){
    le->next=segpool;
    segpool=l;
  }
  if(ae){
    ae->next=segpool;
    segpool=a;
  }

  memset(&r,0,sizeof(r));
  memset(&layout_adj,0,sizeof(layout_adj));
}

int region_layout(graph *g){
  // count up the total length of the region segments used in layout
  float length=0,acc=0,ldel;
  int num_adj=g->vertex_num;
  int activenum=0;
  region_segment *l;
  vertex *v = g->verticies;

  adjust_layout();

  l = layout_adj.l;

  while(l){
    if(l->radius==0){
      float xd=l->x2 - l->x1;
      float yd=l->y2 - l->y1;
      length += l->length = hypot(xd,yd);
    }else{
      float diam = l->radius*2.f*M_PI;
      float del=phisub(l->phi0,l->phi1,l->phi);
      if(l->phi<0)
	del = -del;
      
      length += l->length = diam*del*(1.f/(M_PI*2.f));
    }
    l=l->next;
  }

  // non-contiguous beginnings sink a single point segment per
  l = layout_adj.l;
  while(l){
    if(!l->cont)
      num_adj--;
    l=l->next;
  }

  /* perform layout segment by segment */
  l = layout_adj.l;
  ldel = (float)length/num_adj;
  while(l && v){
    int i;
    int num_placed = l->cont ? rint((l->length-acc)/ldel) :  rint((l->length-acc)/ldel)+1;
    float snap_del = l->cont ? l->length/num_placed : l->length/(num_placed-1);
    float snap_acc=l->cont?snap_del:0;
    
    if(l->split)activenum++;

    if(l->radius==0){
      float x1 = l->x1;
      float y1 = l->y1;
      float x2 = l->x2;
      float y2 = l->y2;
      float xd=(x2-x1)/l->length;
      float yd=(y2-y1)/l->length;
      
      for(i=0;v && i<num_placed;i++){
	v->x = rint(x1+xd*snap_acc);
	v->y = rint(y1+yd*snap_acc);
	
	if(snap_acc)
	  acc+=ldel;
	snap_acc+=snap_del;
	v->active=activenum;
	v=v->next;
      }
    }else{
      /* next is an arc */
      float x = l->cx;
      float y = l->cy;
      float phid = phisub(l->phi0,l->phi1,l->phi);
      
      phid /= l->length;
      
      for(i=0;v && i<num_placed;i++){
	v->x = rint( cos(l->phi0+phid*snap_acc)*(l->radius)+x);
	v->y = rint( -sin(l->phi0+phid*snap_acc)*(l->radius)+y);
	
	if(snap_acc)
	  acc+=ldel;
	snap_acc+=snap_del;
	v->active=activenum;
	v=v->next;
      }
    }
    
    acc-=l->length;  
    l=l->next;
  }
  return activenum;
}

void region_circle(int x,int y, float rad, int layout){
  region_segment *a=new_segment(&r,0,0,0,0);
  a->cx=a->x1=a->x2=x;
  a->cy=a->y1=a->y2=y;
  a->radius=rad;
  a->phi0=-M_PI;
  a->phi1=M_PI;
  a->phi=M_PI*2.f;
  a->layout=layout;
  a->cont=0; // not really necessary, just consistent
  r.cont=0;
}

void region_new_area(int x, int y, int layout){
  r.x=r.ox=x;
  r.y=r.oy=y;
  r.layout=layout;
  r.cont=0;
}

void region_line_to(int x,int y){
  region_line(&r,r.x,r.y,x,y);
  r.x=x;
  r.y=y;
  r.cont=1;
}

void region_arc_to(int x,int y, float rad){
  region_arc(&r,r.x,r.y,x,y,rad);
  r.x=x;
  r.y=y;
  r.cont=1;
}

void region_close_line(){
  region_line(&r,r.x,r.y,r.ox,r.oy);
  r.x=r.ox;
  r.y=r.oy;  
  r.cont=0;
}

void region_close_arc(float rad){
  region_arc(&r,r.x,r.y,r.ox,r.oy,rad);
  r.x=r.ox;
  r.y=r.oy;
  r.cont=0;
}

void region_split_here(){
  r.split_next=1;
}

int region_intersects(edge *e){

  region_segment *s=r.l;
  while(s){
    if(s->layout<3){
      if(s->radius!=0){
	if(intersects_arc(e,s))return 1;
      }else{
	double xdummy,ydummy;
	
	if(intersects(e->A->x,e->A->y,e->B->x,e->B->y,
		      s->x1,s->y1,s->x2,s->y2,
		      &xdummy,&ydummy))return 1;

      }
    }
    s=s->next;
  }
  return 0;
}

int have_region(){
  if(r.l)return 1;
  return 0;
}
