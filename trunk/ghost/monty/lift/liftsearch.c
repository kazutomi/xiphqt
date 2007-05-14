#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "smallft.h"

int fixed_bits = 12;
int num_coefficients = 14;
int p_delay = 2;
int q_delay = 3;
double TB = .1;

#define MAX_PZ 32
#define FS 256
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

  return minimum_list_size-1;
}

double fixed_to_double(int f){
  return f*pow(2.,(double)-fixed_bits);
}

typedef struct {
  int n;
  double a[MAX_PZ]; // normalized filter; a[0] is actually a_1
  double b[MAX_PZ+1]; 
  double pre[MAX_PZ*2]; 
} iir_t;

double Az2(iir_t *f, double v){
  double iir = v;
  int i;
  int n = f->n;

  for(i=0;i<n;i++)
    iir -= f->pre[i*2+1] * f->a[i];

  double fir = f->b[0] * iir;
  for(i=0;i<n;i++)
    fir += f->pre[i*2+1] * f->b[i+1];

  for(i = n*2-1; i>0; i--)
    f->pre[i] = f->pre[i-1];
  f->pre[0] = iir;

  return fir;
}

void mag_e(double *d){
  int i;

  drft_forward(&fft,d);

  d[0] = d[0]*d[0];
  for(i=2;i<FS;i+=2)
    d[i>>1] =d[i-1]*d[i-1] + d[i]*d[i];
  d[FS/2] = d[FS-1]*d[FS-1];

}

void lift_run(int *c, double *lp, double *hp, int dump){
  iir_t P;
  iir_t Q;
  int i,j;
  int offset = (abs(p_delay) + abs(q_delay))*2+1;

  memset(&P,0,sizeof(P));
  memset(&Q,0,sizeof(Q));
  lp[offset] = 1.;
  hp[offset+1] = 1.;

  P.n = Q.n = ((num_coefficients/2) -1)/2;

  for(i=0,j=0; i<P.n; i++, j++)
    P.a[i] = fixed_to_double(c[j]);
  for(i=0; i<P.n+1; i++, j++)
    P.b[i] = fixed_to_double(c[j]);

  for(i=0; i<Q.n; i++, j++)
    Q.a[i] = fixed_to_double(c[j]);
  for(i=0; i<Q.n+1; i++, j++)
    Q.b[i] = fixed_to_double(c[j]);

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

  // energy 
  mag_e(hp);
  mag_e(lp);

  if(dump){
    fprintf(stdout,"Filter result: \n");

    fprintf(stdout,"  P: (");
    for(i=0;i<P.n+1;i++)
      fprintf(stdout,"%s%f",(i==0?"":", "),P.b[i]);
    fprintf(stdout,") / (1");
    for(i=0;i<P.n;i++)
      fprintf(stdout,", %f",P.a[i]);
    fprintf(stdout,")\n");

    fprintf(stdout,"  Q: (");
    for(i=0;i<Q.n+1;i++)
      fprintf(stdout,"%s%f",(i==0?"":", "),Q.b[i]);
    fprintf(stdout,") / (1");
    for(i=0;i<Q.n;i++)
      fprintf(stdout,", %f",Q.a[i]);
    fprintf(stdout,")\n");

    fprintf(stdout,"  response: {\n");
    for(i=0;i<=FS/2;i++)
      fprintf(stdout,"%f %f\n",(double)i/FS, todB(lp[i])/2);
    fprintf(stdout,"\n");
    for(i=0;i<=FS/2;i++)
      fprintf(stdout,"%f %f\n",(double)i/FS, todB(hp[i])/2);
    fprintf(stdout,"}\n\n");
  }

}


double lift_cost(int *c){
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
  for(i=0; i<=TBlo; i++)
    cost += hp[i];

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
  
  return cost;
}

void lift_dump(int *c){
  double *hp = calloc (FS,sizeof(*hp));
  double *lp = calloc (FS,sizeof(*lp));
  int i;

  lift_run(c, lp, hp, 1);

  free(hp);
  free(lp);
}

int walk_to_minimum(int *c){
  int last_change = 0;
  int cur_dim = 0;
  double cost = lift_cost(c);

  while(1){
    c[cur_dim]++;
    double up_cost = lift_cost(c);
    c[cur_dim]-=2;
    double down_cost = lift_cost(c);
    c[cur_dim]++;

    if(up_cost<cost && up_cost<down_cost){
      c[cur_dim]++;
      last_change = cur_dim;
      cost = up_cost;
      
      // keep walking until we hit the minimum in this dim
      /*while(1){
	c[cur_dim]++;
	up_cost = lift_cost(c);
	if(up_cost<cost){
	  cost = up_cost;
	}else{
	  c[cur_dim]--;
	  break;
	}
	}*/

    }else if (down_cost<cost){
      c[cur_dim]--;
      last_change = cur_dim;
      cost = down_cost;

      // keep walking until we hit the minimum in this dim
      /*
      while(1){
	c[cur_dim]--;
	down_cost = lift_cost(c);
	if(down_cost<cost){
	  cost = down_cost;
	}else{
	  c[cur_dim]++;
	  break;
	}
	}*/

    }else{
      if(cur_dim == last_change)break;
    }

    cur_dim++;
    if(cur_dim >= num_coefficients)
      cur_dim = 0;

    fprintf(stderr,"\rwalking... current cost: %g    ",cost);
  }
  fprintf(stderr,"\r                                                    ",cost);

  log_minimum(c,cost);
}


int walk_to_minimum2(int *c){
  double cost = lift_cost(c);
  int i;
  
  while(1){
    double up_cost[num_coefficients];
    double down_cost[num_coefficients];
    double min_cost = cost;
    int min_cost_i = -1;
    int min_cost_d = -1;

    for(i=0;i<num_coefficients;i++){
      c[i]++;
      up_cost[i] = lift_cost(c);
      c[i]-=2;
      down_cost[i] = lift_cost(c);
      c[i]++;
    }

    for(i=0;i<num_coefficients;i++){
      if(up_cost[i] < cost){
	cost = up_cost[i];
	min_cost_i = i;
	min_cost_d = 1;
      }
      if(down_cost[i] < cost){
	cost = down_cost[i];
	min_cost_i = i;
	min_cost_d = -1;
      }
    }

    if(min_cost_i == -1) break;
    c[min_cost_i] += min_cost_d;

    fprintf(stderr,"\rwalking... current cost: %f    ",cost);
  }
  fprintf(stderr,"\r                                                    ",cost);

  log_minimum(c,cost);
}


void bisect_Ncube_recurse(int *lowrange, int *highrange, int min_num){





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

int main(int argc, char *argv[]){
  int *c;
  double *fc;
  int i,j;

  drft_init(&fft,FS);

  int order = 8;
  num_coefficients = (order*2+1)*2;
  TB = .1;
  p_delay = order-1;
  q_delay = order;

  c = alloca(num_coefficients * sizeof(*c));
  fc = alloca(num_coefficients * sizeof(*fc));

  fixed_bits = 8;
  for(i=0,j=0;i<order;i++,j++)
    fc[j] = def_a(order, i, p_delay);
  for(i=0;i<order;i++,j++)
    fc[j] = def_a(order, order-i-1, p_delay);
  fc[j++]=1.;
  for(i=0;i<order;i++,j++)
    fc[j] = def_a(order, i, p_delay);
  for(i=0;i<order;i++,j++)
    fc[j] = def_a(order, order-i-1, p_delay);
  fc[j++]=1.;
  
  for(i=0;i<num_coefficients;i++)
    c[i] = rint(fc[i] * pow(2, fixed_bits));

  walk_to_minimum(c);  

  for(j=9;j<=20;j++){
    fixed_bits = j;
    for(i=0;i<num_coefficients;i++)
      c[i] <<=1;
    walk_to_minimum(c);
  }

  lift_dump(c);

  return 0;
}
