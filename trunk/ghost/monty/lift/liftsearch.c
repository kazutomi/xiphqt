#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "smallft.h"

int fixed_bits = 12;
int order = 14;
int num_coefficients = 14;
int p_delay, q_delay, r_delay, s_delay;
double TB = .1;

#define MAX_PZ 64
#define FS 512
drft_lookup fft;

typedef struct local_minimum{
  int *coefficients;
  double cost;
} lmin_t;

lmin_t *minimum_list = NULL;
int minimum_list_size = 0;

static inline double todB(double x){
  return ((x)==0?-400:log((x)*(x))*4.34294480f);
}

int log_minimum(int *c, double cost){
  int i,j;

  for(i=0;i<minimum_list_size;i++){
    lmin_t *min = minimum_list+i;
    for(j=0;j<num_coefficients;j++)
      if(min->coefficients[j] != c[j])break;
    if(j==num_coefficients) return i;
  }

  if(!minimum_list){
    minimum_list = malloc(sizeof(*minimum_list));
  }else{
    minimum_list = realloc(minimum_list, (minimum_list_size+1)*sizeof(*minimum_list));
  }

  int *mc = malloc(num_coefficients * sizeof(*mc));
  minimum_list[minimum_list_size].coefficients = mc;
  for(j=0;j<num_coefficients;j++) mc[j] = c[j];
  minimum_list_size++;

  fprintf(stderr,"\rfinal cost: %f                                    \n",cost);

  return minimum_list_size-1;
}

typedef struct {
  int n;
  double scale[MAX_PZ];
  double iir[MAX_PZ];
  double fir[MAX_PZ+1]; 
  double pre[MAX_PZ*2]; 
} iir_t;


//Direct form
double Az2(iir_t *f, double v){
  double iir = v;
  int i;
  int n = f->n;

  for(i=0;i<n;i++)
    iir -= f->pre[i*2+1] * f->iir[i];

  double fir = f->fir[0] * iir;
  for(i=0;i<n;i++)
    fir += f->pre[i*2+1] * f->fir[i+1];

  for(i = n*2-1; i>0; i--)
    f->pre[i] = f->pre[i-1];
  f->pre[0] = iir;

  return fir;
}

// Cascade form
double Cz2(iir_t *f, double v){
  int i;
  int n = f->n;

  v*=f->scale[n-1];
  for(i=0;i<n;i++){
    double iir = v - f->pre[i*2] * f->iir[i];
    v = iir + f->pre[i*2] * f->fir[i];
    
    f->pre[i*2] = f->pre[i*2+1];
    f->pre[i*2+1] = iir;
  }
  return v;
}

void mag_e(double *d){
  int i;

  drft_forward(&fft,d);

  d[0] = d[0]*d[0];
  for(i=2;i<FS;i+=2)
    d[i>>1] =d[i-1]*d[i-1] + d[i]*d[i];
  d[FS/2] = d[FS-1]*d[FS-1];

}

void lift_run(double *c, double *lp, double *hp, int dump){
  iir_t P,Q,R,S;
  int i,j;
  int offset = (abs(p_delay) + abs(q_delay))*2+1;

  memset(&P,0,sizeof(P));
  memset(&Q,0,sizeof(Q));
  //memset(&R,0,sizeof(R));
  //memset(&S,0,sizeof(S));

  lp[offset] = 1.;
  hp[offset+1] = 1.;
  P.n = Q.n = order;

#if 1

  j=0;
  for(i=0; i<P.n; i++)
    Q.iir[i] = P.iir[i] = c[j++];
  for(i=0; i<P.n; i++)
    Q.fir[i] = P.fir[i] = c[--j];
  Q.fir[i] = P.fir[i] = 1.;

#else

  j=0;
  for(i=0; i<P.n; i++)
    P.iir[i] = c[j++];
  for(i=0; i<P.n+1; i++)
    P.fir[i] = c[j++];
  for(i=0; i<Q.n; i++)
    Q.iir[i] = c[j++];
  for(i=0; i<Q.n+1; i++)
    Q.fir[i] = c[j++];
  //for(i=0; i<R.n; i++)
  //  R.iir[i] = c[j++];
  //for(i=0; i<R.n+1; i++)
  //  R.fir[i] = c[j++];
  //for(i=0; i<S.n; i++)
  //  S.iir[i] = c[j++];
  //for(i=0; i<S.n+1; i++)
  //  S.fir[i] = c[j++];
#endif

  /*i=0; j=0;
  if(r_delay>0) i += r_delay*2;
  if(r_delay<0) j += -r_delay*2;
  
  for(; i<FS && j<FS; i++, j++)
  hp[j] -= Az2(&R, lp[i]);*/
  
  i=0; j=0;
  if(p_delay>0) i += p_delay*2;
  if(p_delay<0) j += -p_delay*2;

  for(; i<FS && j<FS; i++, j++)
    hp[j] -= Az2(&P, lp[i]);

  i=0; j=0;
  if(q_delay>0) i += q_delay*2;
  if(q_delay<0) j += -q_delay*2;

  for(; i<FS && j<FS; i++, j++)
    lp[j] += .5*Az2(&Q, hp[i]);


  /*i=0; j=0;
  if(s_delay>0) i += s_delay*2;
  if(s_delay<0) j += -s_delay*2;

  for(; i<FS && j<FS; i++, j++)
  lp[j] += .5*Az2(&S, hp[i]);*/
 
  if(dump){
    for(i=0; i<64; i++){
      if(todB(hp[FS-i-1])>-100 ||
	 todB(lp[FS-i-1])>-100){
	fprintf(stderr,"Warning: Insufficiently stable filter!\n");
	break;
      }
      if(todB(hp[FS-i-1])>-120 ||
	 todB(lp[FS-i-1])>-120){
	fprintf(stderr,"Warning: Gibbs ringing overflow!\n");
	break;
      }
    }
  }


  // energy 
  mag_e(hp);
  mag_e(lp);

  if(dump){

    fprintf(stderr,"  a: ( ");
    for(i=0;i<Q.n;i++)
      fprintf(stderr,"%f ",Q.iir[i]);
    fprintf(stderr,")\n");
    

    for(i=0;i<=FS/2;i++)
      fprintf(stdout,"%f %f\n",(double)i/FS, todB(lp[i])/2);
    fprintf(stdout,"\n");
    for(i=0;i<=FS/2;i++)
      fprintf(stdout,"%f %f\n",(double)i/FS, todB(hp[i])/2);
    fprintf(stdout,"\n");
    
  }

}


double lift_cost(double *c){
  double *hp = calloc (FS,sizeof(*hp));
  double *lp = calloc (FS,sizeof(*lp));
  int i;

  lift_run(c, lp, hp, 0);

  // cost
  double cost = 0;
  int TBhi = (int)rint(FS/4*(1.+TB));
  int TBlo = (int)rint(FS/4*(1.-TB));

  // energy in stopband
  for(i=TBhi; i<=FS/2; i++)
    cost += lp[i];
  for(i=0; i<TBlo; i++)
    cost += hp[i]*.25;

  // overshoot energy
  /*for(i=0; i<=FS/2; i++){
    if(lp[i]>2.)
      cost += lp[i]-2.;
    if(hp[i]>8.)
      cost += (hp[i]*.25-2.);
      }*/

  // deviation from 0dB in passband
  //for(i=0; i<=TBlo; i++)
  //  cost += fabs(lp[i]-1.)/4096.;
  //for(i=TBhi; i<=FS/2; i++)
  //  cost += fabs(hp[i]-1.)/4096.;

  // overshoot from 0dB in TB
  //for(i=TBlo; i<=TBhi; i++){
  //  double over = lp[i]-1.;
  //  if(over > 0.) cost+=over;
  //  over = hp[i]-1.;
  //  if(over > 0.) cost+=over;
  //}

  free(hp);
  free(lp);
  
  return cost*cost;//(FS/2-TBhi + TBlo);
}

void lift_dump(double *c){
  double *hp = calloc (FS,sizeof(*hp));
  double *lp = calloc (FS,sizeof(*lp));

  lift_run(c, lp, hp, 1);

  free(hp);
  free(lp);
}

int walk_to_minimum_A(double *c, double ep){
  int last_change = num_coefficients-1;
  int cur_dim = 0;
  double cost = lift_cost(c);

  while(1){
    double val = c[cur_dim];
    c[cur_dim] = val+ep;
    double up_cost = lift_cost(c);
    c[cur_dim] = val-ep;
    double down_cost = lift_cost(c);
    c[cur_dim] = val;

    if(up_cost<cost && up_cost<down_cost){
      c[cur_dim]+=ep;
      last_change = cur_dim;
      cost = up_cost;
      
    }else if (down_cost<cost){
      c[cur_dim]-=ep;
      last_change = cur_dim;
      cost = down_cost;
    }else{
      if(cur_dim == last_change)break;
    }

    cur_dim++;
    if(cur_dim >= num_coefficients)
      cur_dim = 0;

    fprintf(stderr,"\rwalking A... current cost: %g    ",todB(cost)/2);
  }
  fprintf(stderr,"\n");
  return 0;
}

int walk_to_minimum_CSD(double *c, double ep){
  double prev[num_coefficients]; 
  double r[num_coefficients]; 
  double rr[num_coefficients]; 
  int have_prev = 0;
  int flag = 1;
  double ep2 = 1./(1<<24);
  double mul=1.;
  while(flag){
    int i;
    flag = 0;

    // compute gradient
    double d[num_coefficients];
    double mag = 0;
    for(i=0;i<num_coefficients;i++){
      double val = c[i];
      c[i] = val + ep2;
      d[i] = lift_cost(c);
      c[i] = val - ep2;
      d[i] -= lift_cost(c);
      c[i] = val;
      d[i] *= .5;
      mag += d[i]*d[i];
    }

    if(mag == 0) return 0;
    mag = sqrt(mag);

    // normalize gradient to |ep|
    for(i=0;i<num_coefficients;i++)
      d[i] *= ep/mag;
    
    // walk this line
    memcpy(r,c,sizeof(r));
    double cost = lift_cost(r);
    while(1){
      for(i=0;i<num_coefficients;i++)
	r[i] -= d[i]*mul;
      double test_cost = lift_cost(r);
      if(test_cost < cost){
	cost = test_cost;
	flag = 1;
	mul *= 1.5;
	if(mul>32)mul=32;
      }else{
	for(i=0;i<num_coefficients;i++)
	  r[i] += d[i]*mul;
	if(mul<1)break;
	mul *=.5;
      }
    }
    
    /*if(have_prev){
      mag = 0;
      for(i=0;i<num_coefficients;i++){
	d[i] = prev[i] - (c[i]+r[i])*.5;
	mag += d[i]*d[i];
      }
      for(i=0;i<num_coefficients;i++)
	d[i] /= mag;
      
      // walk conditioned line
      memcpy(rr,prev,sizeof(rr));
      double r_cost = lift_cost(rr);
      while(1){
	for(i=0;i<num_coefficients;i++)
	  rr[i] -= d[i]*mul;
	double test_cost = lift_cost(rr);
	if(test_cost < r_cost){
	  r_cost = test_cost;
	  mul *= 2;
	  if(mul>32)mul=32;
	}else{
	  for(i=0;i<num_coefficients;i++)
	    rr[i] += d[i]*mul;
	  mul *=.5;
	  if(mul<1./(1<<6))break;
	}
      }
      
      if(r_cost < cost){
	flag = 1;
	memcpy(r,rr,sizeof(rr));
	cost = r_cost;
	}
	}*/
    
    memcpy(prev,c,sizeof(prev));
    memcpy(c,r,sizeof(r));
    have_prev = 1;
    
    fprintf(stderr,"\rwalking CSD... current cost: %g    ",todB(cost)/2);
  }
  fprintf(stderr,"\n");
  return 0;
}

double n_choose_m(int n, int m){
  double work[n+1];
  int i,j;
  
  work[0]=1;

  for(i=0;i<n;i++){
    work[i+1] = 1;
    for(j=i;j>0;j--)
      work[j] += work[j-1];
  }

  return work[m];
}

double def_a(int n, int sub, int delay){
  double ret = n_choose_m(n,sub+1);
  int i;

  for(i=1;i<=sub+1;i++)
    ret *= (n - delay - i +.5) / (delay + i + .5);

  return ret;
}

void set_maxflat(double *fc){
  int i, j;
  for(i=0,j=0;i<order;i++,j++)
    fc[j] = def_a(order, i, order-1)+(drand48()-.5)*.01;

  //for(i=0;i<order;i++,j++)
  //  fc[j] = def_a(order, order-i-1, order-1)+(drand48()-.5)*.01;
  //fc[j++]=1.;
  //for(i=0;i<order;i++,j++)
  //  fc[j] = def_a(order, i, order-1)+(drand48()-.5)*.01;
  //for(i=0;i<order;i++,j++)
  //  fc[j] = def_a(order, order-i-1, order-1)+(drand48()-.5)*.01;
  //fc[j++]=1.;
}

int main(int argc, char *argv[]){

  drft_init(&fft,FS);

  order = 16;
  num_coefficients = order;//(order*2+1)*2;
  TB = .20;
  p_delay = 15;
  q_delay = 16;
  r_delay = 0;
  s_delay = 0;
  
  //double *fc = calloc(num_coefficients, sizeof(*fc));

  //set_maxflat(fc);
  //num_coefficients = (order*2+1)*2;

  //walk_to_minimum_A(fc,1./(1<<10));  
  //walk_to_minimum_A(fc,1./(1<<12));  
  //walk_to_minimum_A(fc,1./(1<<14));  
  //walk_to_minimum_A(fc,1./(1<<16));  
  //walk_to_minimum_A(fc,1./(1<<18));  
    
  //lift_dump(fc);
  //fflush(stdout);

  double fc[16]={ 0.494814, -0.117399, 0.053387, -0.029044, 0.016904, -0.010041, 
		  0.005928, -0.003415, 0.001890, -0.000988, 0.000478, -0.000208,
		  0.000078, -0.000022, 0.000003, 0.000001 };

  //set_maxflat(fc);
  //walk_to_minimum_CSD(fc,1./(1<<16));  
  //lift_dump(fc);
  //fflush(stdout);
  //walk_to_minimum_CSD(fc,1./(1<<17));  
  //lift_dump(fc);
  //fflush(stdout);
  //walk_to_minimum_CSD(fc,1./(1<<18));  
  //lift_dump(fc);
  //fflush(stdout);
  //walk_to_minimum_CSD(fc,1./(1<<20));  
  //walk_to_minimum_CSD(fc,1./(1<<21));  
  //lift_dump(fc);
  //fflush(stdout);
  //walk_to_minimum_CSD(fc,1./(1<<22));  
  //lift_dump(fc);
  //fflush(stdout);
  //walk_to_minimum_CSD(fc,1./(1<<24));  
  //lift_dump(fc);
  //fflush(stdout);
  walk_to_minimum_CSD(fc,1./(1<<26));  
  //lift_dump(fc);
  //fflush(stdout);
  walk_to_minimum_CSD(fc,1./(1<<28));  
  //lift_dump(fc);
  //fflush(stdout);
  walk_to_minimum_CSD(fc,1./(1<<30));  
  //lift_dump(fc);
  //fflush(stdout);


  lift_dump(fc);

  return 0;
}

#if 0
Good ones so far:

16/.21
  a: ( 0.494814 -0.117399 0.053387 -0.029044 0.016904 -0.010041 0.005928 -0.003415 0.001890 -0.000988 0.000478 -0.000208 0.000078 -0.000022 0.000003 0.000001 )
  a: ( 0.494801 -0.117383 0.053372 -0.029035 0.016902 -0.010047 0.005942 -0.003436 0.001913 -0.001012 0.000500 -0.000225 0.000090 -0.000030 0.000007 -0.000001 )
16/.19
  a: ( 0.495075 -0.117772 0.053811 -0.029476 0.017309 -0.010393 0.006214 -0.003631 0.002040 -0.001083 0.000531 -0.000232 0.000085 -0.000021 0.000000 0.000003 )
16/.16
  a: ( 0.496384 -0.119662 0.056015 -0.031800 0.019595 -0.012518 0.008089 -0.005205 0.003295 -0.002031 0.001205 -0.000679 0.000357 -0.000170 0.000070 -0.000022 )
16/.16
  a: ( 0.496302 -0.119542 0.055873 -0.031647 0.019441 -0.012371 0.007955 -0.005087 0.003196 -0.001951 0.001144 -0.000635 0.000327 -0.000152 0.000060 -0.000017 )


#endif
