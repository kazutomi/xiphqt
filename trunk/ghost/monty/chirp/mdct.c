/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************/


#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "mdct.h"

#define cPI3_8 .38268343236508977175F
#define cPI2_8 .70710678118654752441F
#define cPI1_8 .92387953251128675613F

typedef struct {
  int n;
  int log2n;
  
  float *trig;
  int   *bitrev;
  float *win;

  float scale;
} mdct_lookup;

/* 16 through 131072 */
static mdct_lookup look_cache[18];
pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

/* build lookups for trig functions; also pre-figure scaling and
   some window function algebra. */

static int _ilog(int x){
  int ret = 0;
  while( (x>>=1) > 0)
    ret++;
  
  return ret;
}

static void mdct_init_i(mdct_lookup *lookup,int n){
  int   *bitrev=malloc(sizeof(*bitrev)*(n/4));
  float *T=malloc(sizeof(*T)*(n+n/4));
  
  int i;
  int n2=n>>1;
  int log2n=lookup->log2n=rint(log((float)n)/log(2.));
  lookup->n=n;
  lookup->trig=T;
  lookup->bitrev=bitrev;

  /* trig lookups... */
  
  for(i=0;i<n/4;i++){
    T[i*2]=(cos((M_PI/n)*(4*i)));
    T[i*2+1]=(-sin((M_PI/n)*(4*i)));
    T[n2+i*2]=(cos((M_PI/(2*n))*(2*i+1)));
    T[n2+i*2+1]=(sin((M_PI/(2*n))*(2*i+1)));
  }
  for(i=0;i<n/8;i++){
    T[n+i*2]=(cos((M_PI/n)*(4*i+2))*.5);
    T[n+i*2+1]=(-sin((M_PI/n)*(4*i+2))*.5);
  }

  /* bitreverse lookup... */

  {
    int mask=(1<<(log2n-1))-1,i,j;
    int msb=1<<(log2n-2);
    for(i=0;i<n/8;i++){
      int acc=0;
      for(j=0;msb>>j;j++)
	if((msb>>j)&i)acc|=1<<j;
      bitrev[i*2]=((~acc)&mask)-1;
      bitrev[i*2+1]=acc;

    }
  }
  lookup->scale=(4./n);

  /* The 'vorbis window' (window 0) is sin(sin(x)*sin(x)*pi*.5) */
  lookup->win=calloc(n2,sizeof(*lookup->win));

  for(i=0;i<n2;i++){
    double x=(i+.5)/n2*M_PI/2.;
    x=sin(x);
    x*=x;
    x*=M_PI/2.;
    x=sin(x);
    lookup->win[i]=x;
  }
}

void mdct_clear_i(mdct_lookup *l){
  if(l){
    if(l->trig)free(l->trig);
    if(l->bitrev)free(l->bitrev);
    if(l->win)free(l->win);
    memset(l,0,sizeof(*l));
  }
}


/* 8 point butterfly (in place, 4 register) */
static inline void mdct_butterfly_8(float *x){
  float r0   = x[6] + x[2];
  float r1   = x[6] - x[2];
  float r2   = x[4] + x[0];
  float r3   = x[4] - x[0];

	   x[6] = r0   + r2;
	   x[4] = r0   - r2;
	   
	   r0   = x[5] - x[1];
	   r2   = x[7] - x[3];
	   x[0] = r1   + r0;
	   x[2] = r1   - r0;
	   
	   r0   = x[5] + x[1];
	   r1   = x[7] + x[3];
	   x[3] = r2   + r3;
	   x[1] = r2   - r3;
	   x[7] = r1   + r0;
	   x[5] = r1   - r0;
	   
}

/* 16 point butterfly (in place, 4 register) */
static inline void mdct_butterfly_16(float *x){
  float r0     = x[1]  - x[9];
  float r1     = x[0]  - x[8];

           x[8]  += x[0];
           x[9]  += x[1];
           x[0]   = ((r0   + r1) * cPI2_8);
           x[1]   = ((r0   - r1) * cPI2_8);

           r0     = x[3]  - x[11];
           r1     = x[10] - x[2];
           x[10] += x[2];
           x[11] += x[3];
           x[2]   = r0;
           x[3]   = r1;

           r0     = x[12] - x[4];
           r1     = x[13] - x[5];
           x[12] += x[4];
           x[13] += x[5];
           x[4]   = ((r0   - r1) * cPI2_8);
           x[5]   = ((r0   + r1) * cPI2_8);

           r0     = x[14] - x[6];
           r1     = x[15] - x[7];
           x[14] += x[6];
           x[15] += x[7];
           x[6]  = r0;
           x[7]  = r1;

	   mdct_butterfly_8(x);
	   mdct_butterfly_8(x+8);
}

/* 32 point butterfly (in place, 4 register) */
static inline void mdct_butterfly_32(float *x){
  float r0     = x[30] - x[14];
  float r1     = x[31] - x[15];

           x[30] +=         x[14];           
	   x[31] +=         x[15];
           x[14]  =         r0;              
	   x[15]  =         r1;

           r0     = x[28] - x[12];   
	   r1     = x[29] - x[13];
           x[28] +=         x[12];           
	   x[29] +=         x[13];
           x[12]  = ( r0 * cPI1_8  -  r1 * cPI3_8 );
	   x[13]  = ( r0 * cPI3_8  +  r1 * cPI1_8 );

           r0     = x[26] - x[10];
	   r1     = x[27] - x[11];
	   x[26] +=         x[10];
	   x[27] +=         x[11];
	   x[10]  = (( r0  - r1 ) * cPI2_8);
	   x[11]  = (( r0  + r1 ) * cPI2_8);

	   r0     = x[24] - x[8];
	   r1     = x[25] - x[9];
	   x[24] += x[8];
	   x[25] += x[9];
	   x[8]   = ( r0 * cPI3_8  -  r1 * cPI1_8 );
	   x[9]   = ( r1 * cPI3_8  +  r0 * cPI1_8 );

	   r0     = x[22] - x[6];
	   r1     = x[7]  - x[23];
	   x[22] += x[6];
	   x[23] += x[7];
	   x[6]   = r1;
	   x[7]   = r0;

	   r0     = x[4]  - x[20];
	   r1     = x[5]  - x[21];
	   x[20] += x[4];
	   x[21] += x[5];
	   x[4]   = ( r1 * cPI1_8  +  r0 * cPI3_8 );
	   x[5]   = ( r1 * cPI3_8  -  r0 * cPI1_8 );

	   r0     = x[2]  - x[18];
	   r1     = x[3]  - x[19];
	   x[18] += x[2];
	   x[19] += x[3];
	   x[2]   = (( r1  + r0 ) * cPI2_8);
	   x[3]   = (( r1  - r0 ) * cPI2_8);

	   r0     = x[0]  - x[16];
	   r1     = x[1]  - x[17];
	   x[16] += x[0];
	   x[17] += x[1];
	   x[0]   = ( r1 * cPI3_8  +  r0 * cPI1_8 );
	   x[1]   = ( r1 * cPI1_8  -  r0 * cPI3_8 );

	   mdct_butterfly_16(x);
	   mdct_butterfly_16(x+16);

}

/* N point first stage butterfly (in place, 2 register) */
static inline void mdct_butterfly_first(float *T,
					float *x,
					int points){
  
  float *x1        = x          + points      - 8;
  float *x2        = x          + (points>>1) - 8;
  float   r0;
  float   r1;

  do{
    
               r0      = x1[6]      -  x2[6];
	       r1      = x1[7]      -  x2[7];
	       x1[6]  += x2[6];
	       x1[7]  += x2[7];
	       x2[6]   = (r1 * T[1]  +  r0 * T[0]);
	       x2[7]   = (r1 * T[0]  -  r0 * T[1]);
	       
	       r0      = x1[4]      -  x2[4];
	       r1      = x1[5]      -  x2[5];
	       x1[4]  += x2[4];
	       x1[5]  += x2[5];
	       x2[4]   = (r1 * T[5]  +  r0 * T[4]);
	       x2[5]   = (r1 * T[4]  -  r0 * T[5]);
	       
	       r0      = x1[2]      -  x2[2];
	       r1      = x1[3]      -  x2[3];
	       x1[2]  += x2[2];
	       x1[3]  += x2[3];
	       x2[2]   = (r1 * T[9]  +  r0 * T[8]);
	       x2[3]   = (r1 * T[8]  -  r0 * T[9]);
	       
	       r0      = x1[0]      -  x2[0];
	       r1      = x1[1]      -  x2[1];
	       x1[0]  += x2[0];
	       x1[1]  += x2[1];
	       x2[0]   = (r1 * T[13] +  r0 * T[12]);
	       x2[1]   = (r1 * T[12] -  r0 * T[13]);
	       
    x1-=8;
    x2-=8;
    T+=16;

  }while(x2>=x);
}

/* N/stage point generic N stage butterfly (in place, 2 register) */
static inline void mdct_butterfly_generic(float *T,
					  float *x,
					  int points,
					  int trigint){
  
  float *x1        = x          + points      - 8;
  float *x2        = x          + (points>>1) - 8;
  float   r0;
  float   r1;

  do{
    
               r0      = x1[6]      -  x2[6];
	       r1      = x1[7]      -  x2[7];
	       x1[6]  += x2[6];
	       x1[7]  += x2[7];
	       x2[6]   = (r1 * T[1]  +  r0 * T[0]);
	       x2[7]   = (r1 * T[0]  -  r0 * T[1]);
	       
	       T+=trigint;
	       
	       r0      = x1[4]      -  x2[4];
	       r1      = x1[5]      -  x2[5];
	       x1[4]  += x2[4];
	       x1[5]  += x2[5];
	       x2[4]   = (r1 * T[1]  +  r0 * T[0]);
	       x2[5]   = (r1 * T[0]  -  r0 * T[1]);
	       
	       T+=trigint;
	       
	       r0      = x1[2]      -  x2[2];
	       r1      = x1[3]      -  x2[3];
	       x1[2]  += x2[2];
	       x1[3]  += x2[3];
	       x2[2]   = (r1 * T[1]  +  r0 * T[0]);
	       x2[3]   = (r1 * T[0]  -  r0 * T[1]);
	       
	       T+=trigint;
	       
	       r0      = x1[0]      -  x2[0];
	       r1      = x1[1]      -  x2[1];
	       x1[0]  += x2[0];
	       x1[1]  += x2[1];
	       x2[0]   = (r1 * T[1]  +  r0 * T[0]);
	       x2[1]   = (r1 * T[0]  -  r0 * T[1]);

	       T+=trigint;
    x1-=8;
    x2-=8;

  }while(x2>=x);
}

static inline void mdct_butterflies(mdct_lookup *init,
			     float *x,
			     int points){
  
  float *T=init->trig;
  int stages=init->log2n-5;
  int i,j;
  
  if(--stages>0){
    mdct_butterfly_first(T,x,points);
  }

  for(i=1;--stages>0;i++){
    for(j=0;j<(1<<i);j++)
      mdct_butterfly_generic(T,x+(points>>i)*j,points>>i,4<<i);
  }

  if(points>=32){
    for(j=0;j<points;j+=32)
      mdct_butterfly_32(x+j);
  }else if (points>=16){
    for(j=0;j<points;j+=16)
      mdct_butterfly_16(x+j);
  }else if (points>=8){
    for(j=0;j<points;j+=8)
      mdct_butterfly_8(x+j);
  }
}

static inline void mdct_bitreverse(mdct_lookup *init, 
				   float *in, float *out){
  int        n       = init->n;
  int       *bit     = init->bitrev;
  float *w0      = out;
  float *w1      = w0+(n>>1);
  float *T       = init->trig+n;

  do{
    float *x0    = in+bit[0];
    float *x1    = in+bit[1];

    float  r0     = x0[1]  - x1[1];
    float  r1     = x0[0]  + x1[0];
    float  r2     = (r1     * T[0]   + r0 * T[1]);
    float  r3     = (r1     * T[1]   - r0 * T[0]);

	      w1    -= 4;

              r0     = .5*(x0[1] + x1[1]);
              r1     = .5*(x0[0] - x1[0]);
      
	      w0[0]  = r0     + r2;
	      w1[2]  = r0     - r2;
	      w0[1]  = r1     + r3;
	      w1[3]  = r3     - r1;

              x0     = in+bit[2];
              x1     = in+bit[3];

              r0     = x0[1]  - x1[1];
              r1     = x0[0]  + x1[0];
              r2     = (r1     * T[2]   + r0 * T[3]);
              r3     = (r1     * T[3]   - r0 * T[2]);

              r0     = .5*(x0[1] + x1[1]);
              r1     = .5*(x0[0] - x1[0]);
      
	      w0[2]  = r0     + r2;
	      w1[0]  = r0     - r2;
	      w0[3]  = r1     + r3;
	      w1[1]  = r3     - r1;

	      T     += 4;
	      bit   += 4;
	      w0    += 4;

  }while(w0<w1);
}

static void mdct_backward_i(mdct_lookup *init, 
			    float *in, float *outA, float *outB){
  int n=init->n;
  int n2=n>>1;
  int n4=n>>2;
  float *work=alloca(n*sizeof(*work));

  /* rotate */

  float *iX = in+n2-7;
  float *oX = outB+n4;
  float *T  = init->trig+n4;

  do{
    oX         -= 4;
    oX[0]       = (-iX[2] * T[3] - iX[0]  * T[2]);
    oX[1]       =  (iX[0] * T[3] - iX[2]  * T[2]);
    oX[2]       = (-iX[6] * T[1] - iX[4]  * T[0]);
    oX[3]       =  (iX[4] * T[1] - iX[6]  * T[0]);
    iX         -= 8;
    T          += 4;
  }while(iX>=in);

  iX            = in+n2-8;
  oX            = outB+n4;
  T             = init->trig+n4;

  do{
    T          -= 4;
    oX[0]       =   (iX[4] * T[3] + iX[6] * T[2]);
    oX[1]       =   (iX[4] * T[2] - iX[6] * T[3]);
    oX[2]       =   (iX[0] * T[1] + iX[2] * T[0]);
    oX[3]       =   (iX[0] * T[0] - iX[2] * T[1]);
    iX         -= 8;
    oX         += 4;
  }while(iX>=in);

  mdct_butterflies(init,outB,n2);
  mdct_bitreverse(init,outB,work);

  /* roatate + window */
  {
    float *oX1=outB+n4;
    float *oX2=outB+n4;
    float *iX =work;
    float *w1,*w2;
    T             =init->trig+n2;
    
    do{
      oX1-=4;

      oX1[3]  =   (iX[0] * T[1] - iX[1] * T[0]);
      oX2[0]  = - (iX[0] * T[0] + iX[1] * T[1]);

      oX1[2]  =   (iX[2] * T[3] - iX[3] * T[2]);
      oX2[1]  = - (iX[2] * T[2] + iX[3] * T[3]);

      oX1[1]  =   (iX[4] * T[5] - iX[5] * T[4]);
      oX2[2]  = - (iX[4] * T[4] + iX[5] * T[5]);

      oX1[0]  =   (iX[6] * T[7] - iX[7] * T[6]);
      oX2[3]  = - (iX[6] * T[6] + iX[7] * T[7]);

      oX2+=4;
      iX    +=   8;
      T     +=   8;
    }while(oX1>outB);

    if(outA){
      iX=outB+n4;
      oX1=outA+n4;
      oX2=oX1;
      w1=w2=init->win+n4;
      
      do{
	oX1-=4;
	iX-=4;
	w1-=4;
	
	oX1[0] += iX[0]*w1[0];
	oX1[1] += iX[1]*w1[1];
	oX1[2] += iX[2]*w1[2];
	oX1[3] += iX[3]*w1[3];
	
	oX2[0] += -iX[3]*w2[0];
	oX2[1] += -iX[2]*w2[1];
	oX2[2] += -iX[1]*w2[2];
	oX2[3] += -iX[0]*w2[3];
	
	oX2+=4;
	w2+=4;
      }while(oX1>outA);
    }

    oX2=oX1=outB+n4; 
    w1=w2=init->win+n4;
    do{
      oX1-=4;
      w1-=4;

      oX1[0]= oX2[3]*w2[3];
      oX1[1]= oX2[2]*w2[2];
      oX1[2]= oX2[1]*w2[1];
      oX1[3]= oX2[0]*w2[0];

      oX2[0]*=w1[3];
      oX2[1]*=w1[2];
      oX2[2]*=w1[1];
      oX2[3]*=w1[0];

      oX2+=4;
      w2+=4;
    }while(oX1>outB);
  }
}

static void mdct_forward_i(mdct_lookup *init, 
			   float *inA, float *inB,
			   float *out){
  int n=init->n;
  int n2=n>>1;
  int n4=n>>2;
  int n8=n>>3;
  float *w=alloca(n*sizeof(*w)); /* forward needs working space */
  float *w2=w+n2;

  /* rotate */

  /* window + rotate + step 1 */
  
  float r0;
  float r1;
  float *x0=inB+n4;
  float *x1=x0+1;
  float *T=init->trig+n2;
  float *win0=init->win+n4;
  float *win1=win0;

  int i=0;
  
  for(i=0;i<n8;i+=2){
    x0 -=4;
    win1 -=4;

    T-=2;
    r0= x0[2]*win0[1] + x1[0]*win1[2];
    r1= x0[0]*win0[3] + x1[2]*win1[0];       
    w2[i]=   (r1*T[1] + r0*T[0]);
    w2[i+1]= (r1*T[0] - r0*T[1]);

    x1 +=4;
    win0 +=4;
  }

  x0=inA+n2;
  x1=inA+1;
  win0=init->win+n2;
  win1=init->win+1;

  for(;i<n2-n8;i+=2){
    T-=2;
    x0 -=4;
    win0 -=4;

    r0= x0[2]*win0[2] - x1[0]*win1[0];
    r1= x0[0]*win0[0] - x1[2]*win1[2];       
    w2[i]=   (r1*T[1] + r0*T[0]);
    w2[i+1]= (r1*T[0] - r0*T[1]);

    x1 +=4;
    win1 +=4;
  }
    
  x0=inB+n2;
  x1=inB+1;
  win0=init->win;
  win1=init->win+n2;

  for(;i<n2;i+=2){
    T-=2;
    x0 -=4;
    win1 -=4;

    r0= -x0[2]*win0[1] - x1[0]*win1[2];
    r1= -x0[0]*win0[3] - x1[2]*win1[0];       
    w2[i]=   (r1*T[1] + r0*T[0]);
    w2[i+1]= (r1*T[0] - r0*T[1]);

    x1 +=4;
    win0 +=4;
  }


  mdct_butterflies(init,w+n2,n2);
  mdct_bitreverse(init,w+n2,w);

  /* roatate + window */

  T=init->trig+n2;
  x0=out+n2;

  for(i=0;i<n4;i++){
    x0--;
    out[i] =((w[0]*T[0]+w[1]*T[1])*init->scale);
    x0[0]  =((w[0]*T[1]-w[1]*T[0])*init->scale);
    w+=2;
    T+=2;
  }
}

int mdct_forward(float *inA, float *inB, float *out, int n){
  int num = _ilog(n);
  if(num<4 && num>17) return -1;
  
  pthread_mutex_lock(&init_lock);
  if(look_cache[num].trig==0)
    mdct_init_i(look_cache+num,n);
  pthread_mutex_unlock(&init_lock);
  
  mdct_forward_i(look_cache+num,inA,inB,out);
  return 0;
}

int mdct_backward(float *in, float *outA, float *outB, int n){
  int num = _ilog(n);
  if(num<4 && num>17) return -1;
  
  pthread_mutex_lock(&init_lock);
  if(look_cache[num].trig==0)
    mdct_init_i(look_cache+num,n);
  pthread_mutex_unlock(&init_lock);
  
  mdct_backward_i(look_cache+num,in,outA,outB);
  return 0;
}

void mdct_clear(){
  int i;
  pthread_mutex_lock(&init_lock);
  for(i=0;i<18;i++)
    if(look_cache[i].trig)
      mdct_clear_i(look_cache+i);
  pthread_mutex_unlock(&init_lock);
}
