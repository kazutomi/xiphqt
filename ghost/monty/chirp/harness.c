#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <squishyio/squishyio.h>
#include "smallft.h"
#include "scales.h"
#include "chirp.h"
#include "lpc.h"

static int BLOCKSIZE=1024;
static int MAX_CPC=200; /* maximum of 200 chirp candidates per channel */

static float MINdB=-100;
static float MAXdB=0;

#define AREAS 2
static float AREA_SIZE[AREAS]={1.,1.};
static int   AREA_HZOOM[AREAS]={1,4};
static int   AREA_VZOOM[AREAS]={4,4};
static int   AREA_FRAMEOUT_TRIGGER=1;

static int   WAVEFORM_AREA=0;
static int   SPECTROGRAM_AREA=-1;
static int   CHIRPOGRAM_AREA=-1;

static int   SPECTRUM_AREA=1;
static int   SPECTRUM_GRIDFONTSIZE=8;

static int   ENVELOPE_AREA=-1;

static int   TRACK_AREA=1;

static int   pic_w,pic_h;
static drft_lookup fft;

static void hanning(float *out, float *in, int n){
  int i;
  float scale = 2*M_PI/n;

  for(i=0;i<n;i++){
    float i5 = i+.5;
    out[i] = in[i]*(.5-.5*cos(scale*i5));
  }
}

void mag_dB(float *log,float *d, int n){
  int i;
  log[0] = todB(d[0]*d[0])*.5;
  for(i=2;i<n;i+=2)
    log[i>>1] = todB(d[i-1]*d[i-1] + d[i]*d[i])*.5;
  log[n/2] = todB(d[n-1]*d[n-1])*.5;
}

static int frameno=0;
void write_yuv(pcm_t *pcm, cairo_surface_t *s){
  int w=cairo_image_surface_get_width(s);
  int h=cairo_image_surface_get_height(s);
  unsigned char *d=cairo_image_surface_get_data(s);
  unsigned char *p=malloc(w*h*3);
  int i;
  unsigned char *p1,*p2,*p3,*dd;

  printf("FRAME S0 L%d P%.3f\n",w*h*3,
         (float)frameno*(BLOCKSIZE/AREA_VZOOM[AREA_FRAMEOUT_TRIGGER])/
         pcm->rate);
  p1=p;
  p2=p+w*h;
  p3=p2+w*h;
  dd=d;
  for(i=0;i<w*h;i++){
    *(p1++)=(65481*dd[2]+128553*dd[1]
             +24966*dd[0]+4207500)/255000;
    *(p2++)=(-8372*4*dd[2]-16436*4*dd[1]+
             +24808*4*dd[0]+29032005)/225930;
    *(p3++)=(39256*4*dd[2]-32872*4*dd[1]
             -6384*4*dd[0]+45940035)/357510;
    dd+=4;
  }
  fwrite(p,1,w*h*3,stdout);

  free(p);
  frameno++;
}

static off_t pcmno=0;
void write_pcm(pcm_t *pcm,float **d, int pos, int samples){
  int i,j;
  unsigned char *p=malloc(samples*pcm->ch*3);
  unsigned char *pp=p;
  printf("FRAME S1 L%d P%.3f\n",samples*pcm->ch*3,(float)pcmno/pcm->rate);

  for(i=pos;i<samples+pos;i++)
    for(j=0;j<pcm->ch;j++){
      int val = rint(d[j][i]*8388608.f);
      if(val<-8388608)val=-8388608;
      if(val>8388607)val=8388607;
      p[0]=(val&0xff);
      p[1]=((val>>8)&0xff);
      p[2]=((val>>16)&0xff);
      p+=3;
    }
  fwrite(pp,1,samples*pcm->ch*3,stdout);
  free(pp);
  pcmno+=samples;
}

int area_y(int area){
  int i;
  float n=0,d=0;
  for(i=0;i<area;i++)
    n+=AREA_SIZE[i];
  for(;i<AREAS;i++)
    d+=AREA_SIZE[i];
  d+=n;
  return (int)rint(n/d*pic_h);
}

int area_h(int area){
  return area_y(area+1)-area_y(area);
}

void clip_area(cairo_t *c, int area){
  cairo_rectangle(c,0,area_y(area),pic_w,area_h(area));
  cairo_clip(c);
}


void clear_area(cairo_t *c, int area){
  cairo_set_source_rgb(c,0,0,0);
  cairo_rectangle(c,0,area_y(area),pic_w,area_h(area));
  cairo_fill(c);
}


void scroll_area(cairo_t *c, int area){
  /* shift image up by a pixel */
  cairo_set_source_surface(c,cairo_get_target(c),0,-1);
  cairo_paint(c);
  cairo_set_source_rgb(c,0,0,0);
  cairo_rectangle(c,0,area_y(area)+area_h(area)-1,pic_w,1);
  cairo_fill(c);
}

void pick_a_color(int channel, float *r, float *g, float *b){

  *r=*g=*b=0.;
  switch(channel){
  case 5:
    *r=1.;
    break;
  case 1:
    *r=.7;
    *g=.7;
    *b=1;
    break;
  case 2:
    *b=1.;
    break;
  case 3:
    *r=.7;
    *g=.7;
    break;
  case 4:
    *g=.7;
    *b=.7;
    break;
  case 0:
    *r=.7;
    *b=.7;
    *g=1;
    break;
  case 6:
    *r=.45;
    *b=.45;
    *g=.45;
    break;
  case 7:
    *r=.8;
    *g=.3;
    break;
  }
}

void spectrogram(pcm_t *pcm,
                 float **current_dB,
                 float **current_spread,
                 cairo_t *c,
                 off_t pos){
  int i,j;
  float r,g,b;
  float work[BLOCKSIZE];
  int y = area_y(SPECTROGRAM_AREA)+area_h(SPECTROGRAM_AREA)-1;
  int hzoom = AREA_HZOOM[SPECTROGRAM_AREA];
  cairo_save(c);
  clip_area(c,SPECTROGRAM_AREA);
  scroll_area(c,SPECTROGRAM_AREA);

  cairo_set_operator(c,CAIRO_OPERATOR_ADD);

  for(i=0;i<pcm->ch;i++){
    for(j=0;j<BLOCKSIZE/2;j++)
    work[j] = current_dB[i][j]-current_spread[i][j]-50.;

    pick_a_color(i,&r,&g,&b);
    for(j=0;j<BLOCKSIZE/2;j++){
      float alpha = work[j]*(1./-MINdB)+1.f;
      if(alpha<0.)alpha=0.;
      if(alpha>1.)alpha=1.;
      cairo_set_source_rgba(c,r,g,b,alpha);
      cairo_rectangle(c,j*hzoom-hzoom/2,y,hzoom,1);
      cairo_fill(c);
    }
  }

  cairo_restore(c);
}

extern void slow_dcft_row(float *x, float *y, int k, int n);
void chirpogram(pcm_t *pcm,
                float *window,
                float **current_block,
                float **current_dB,
                float **current_spread,
                cairo_t *c,
                off_t pos){
  int i,j,k;
  float r,g,b;
  float work[BLOCKSIZE];
  float work2[BLOCKSIZE];
  int h = area_h(CHIRPOGRAM_AREA);
  int y = area_y(CHIRPOGRAM_AREA);
  int hzoom = AREA_HZOOM[CHIRPOGRAM_AREA];
  cairo_save(c);
  clip_area(c,CHIRPOGRAM_AREA);
  clear_area(c,CHIRPOGRAM_AREA);

  cairo_set_operator(c,CAIRO_OPERATOR_ADD);

  for(i=0;i<pcm->ch;i++){
    memcpy(work2,current_block[i],sizeof(work2));
    for(j=0;j<BLOCKSIZE;j++)work2[j]*=window[j];

    for(k=0;k<h;k++){
      slow_dcft_row(work2,work, k, BLOCKSIZE);

      for(j=0;j<BLOCKSIZE/2;j++)
        work[j] = todB(work[j]*2/BLOCKSIZE);//-current_spread[i][j]-50.;

      pick_a_color(i+1,&r,&g,&b);
      for(j=0;j<BLOCKSIZE/2;j++){
        float alpha = work[j]*(1./-MINdB)+1.f;
        if(alpha<0.)alpha=0.;
        if(alpha>1.)alpha=1.;
        cairo_set_source_rgba(c,r,g,b,alpha);
        cairo_rectangle(c,j*hzoom-hzoom/2,y+k,hzoom,1);
        cairo_fill(c);
      }
    }
  }

  cairo_restore(c);
}


void grid_lines(cairo_t *c,int area){
  int y1 = area_y(area);
  int h = area_h(area);
  int y2 = y1+h;
  int j;
  int spacing = (AREA_HZOOM[area]<4 ? 10: AREA_HZOOM[area]);
  cairo_save(c);
  clip_area(c,area);
  cairo_set_line_width(c,1.);
  cairo_set_source_rgb(c,.2,.2,.2);
  for(j=0;j<pic_w;j+=spacing){
    cairo_move_to(c,j+.5,y1);
    cairo_line_to(c,j+.5,y2);
  }
  for(j=MINdB;j<=MAXdB;j+=20){
    float y = j*(1./MINdB);
    cairo_move_to(c,.5,y1+h*y+.5);
    cairo_line_to(c,pic_w-.5,y1+h*y+.5);
  }
  cairo_stroke(c);
  for(j=MINdB;j<=MAXdB;j+=20){
    float y = j*(1./MINdB);
    cairo_text_extents_t extents;
    char b[80];
    sprintf(b,"%ddB",j);
    cairo_text_extents(c, b, &extents);
    cairo_move_to(c,.5,y1+h*y+extents.height+2.5);

    cairo_set_source_rgb(c, 0,0,0);
    cairo_text_path (c, b);
    cairo_set_line_width(c,3.);
    cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
    cairo_stroke(c);

    cairo_set_source_rgb(c,.2,.2,.2);
    cairo_move_to(c,.5,y1+h*y+extents.height+2.5);
    cairo_show_text(c, b);
  }
  cairo_restore(c);
}

void frequency_vector_dB(float *vec,int ch,int n,
                         int area, cairo_t *c,int fillp){
  float r,g,b;
  int y1 = area_y(area);
  int h = area_h(area);
  int hzoom = AREA_HZOOM[area];
  int j;

  for(j=0;j<n;j++){
    float y = vec[j]*(1./MINdB);
    if(y<0.)y=0.;
    if(y>1.)y=1.;
    if(j==0){
      cairo_move_to(c,j*hzoom+.5,y1+h*y+.5);
    }else{
      cairo_line_to(c,j*hzoom+.5,y1+h*y+.5);
    }
  }
  pick_a_color(ch,&r,&g,&b);
  if(fillp){
    cairo_line_to(c,pic_w-.5,y1+h-.5);
    cairo_line_to(c,.5,y1+h-.5);
    cairo_close_path(c);
    cairo_set_source_rgba(c,r,g,b,.2);
    cairo_fill(c);
    cairo_new_path(c);
  }else{
    cairo_set_source_rgb(c,r,g,b);
    cairo_stroke(c);
  }

}

void spectrum(pcm_t *pcm,float **current_dB,cairo_t *c,off_t pos){
  int i;
  cairo_save(c);
  clip_area(c,SPECTRUM_AREA);
  clear_area(c,SPECTRUM_AREA);
  grid_lines(c,SPECTRUM_AREA);
  cairo_set_line_width(c,.7);

  for(i=0;i<pcm->ch;i++){
    int j;
    for(j=0;j<BLOCKSIZE/2+1;j++)
      current_dB[i][j] = current_dB[i][j];
    frequency_vector_dB(current_dB[i],i,BLOCKSIZE/2+1,SPECTRUM_AREA,c,0);
  }
  cairo_restore(c);
}

void waveform_vector(float *vec,int ch,int n,
                     int area, cairo_t *c,int fillp){
  float r,g,b;
  int y1 = area_y(area);
  int h = area_h(area);
  float hzoom = AREA_HZOOM[area]*.5;
  int j;

  for(j=0;j<n;j++){
    float y = (-vec[j]+1)*h*.5;
    if(j==0){
      cairo_move_to(c,j*hzoom+.5,y1+y+.5);
    }else{
      cairo_line_to(c,j*hzoom+.5,y1+y+.5);
    }
  }
  pick_a_color(ch,&r,&g,&b);
  if(fillp){
    cairo_line_to(c,pic_w-.5,y1+h-.5);
    cairo_line_to(c,.5,y1+h-.5);
    cairo_close_path(c);
    cairo_set_source_rgba(c,r,g,b,.2);
    cairo_fill(c);
    cairo_new_path(c);
  }else{
    cairo_set_source_rgb(c,r,g,b);
    cairo_stroke(c);
  }

}

void waveform(pcm_t *pcm,float *window,float **current,cairo_t *c,off_t pos){
  int i,j;
  float work[BLOCKSIZE];
  drft_lookup fft;
  drft_init(&fft, BLOCKSIZE);

  cairo_save(c);
  clip_area(c,WAVEFORM_AREA);
  clear_area(c,WAVEFORM_AREA);
  cairo_set_line_width(c,1.);

  for(i=0;i<pcm->ch;i++){
    for(j=0;j<BLOCKSIZE;j++)
      work[j]= -current[i][j]*window[j];

    waveform_vector(work,i,BLOCKSIZE,WAVEFORM_AREA,c,0);
    //waveform_vector(window,i,BLOCKSIZE,WAVEFORM_AREA,c,0);
  }

  drft_clear(&fft);
  cairo_restore(c);
}

void spread_bark(float *in, float *out, int n, int rate,
                 float loslope, float width, float hislope){

  static float *spread_att=NULL;
  static float *barkw=NULL;
  static int spread_n=0;
  static int *transition_lo=NULL;
  static int *transition_hi=NULL;
  static float *transition_lo_att=NULL;
  static float *transition_hi_att=NULL;

  float work[n];

  int i,j;

  if(spread_n!=n){
    float b=toBark(0);

    if(spread_att){
      free(spread_att);
      free(barkw);
      free(transition_lo);
      free(transition_hi);
      free(transition_lo_att);
      free(transition_hi_att);
    }
    spread_att = calloc(n,sizeof(*spread_att));
    barkw = calloc(n,sizeof(*barkw));
    transition_lo = calloc(n,sizeof(*transition_lo));
    transition_hi = calloc(n,sizeof(*transition_hi));
    transition_lo_att = calloc(n,sizeof(*transition_lo_att));
    transition_hi_att = calloc(n,sizeof(*transition_hi_att));
    spread_n = n;

    /* used to determine rolloff per linear step */
    for(i=0;i<n;i++){
      float nb = toBark(.5*(i+1)/n*rate);
      barkw[i]=nb-b;
      b=nb;
    }

    /* compute the energy integral */
    for(i=0;i<n;i++){
      float total=1.;
      float att=0.;
      float centerbark = toBark(.5*i/n*rate);
      j=i;

      /* loop for computation that assumes we've not fallen off bottom
         edge of the spectrum */
      while(--j>=0 && att>-120.){
        float b = toBark(.5*j/n*rate);
        if(b>centerbark-width*.5){
          /* still inside the F3 point */
          att = (centerbark-b)/(width*.5)*-3.;
        }else{
          /* outside the F3 point */
          att = ((centerbark-width*.5)-b)*loslope-3.;
        }
        total += fromdB(att);
      }

      /* avoid area contraction at edges of spectrum */
      while(att>-120.){
        att += (barkw[0]*loslope);
        total += fromdB(att);
      }

      j=i;
      att=0;
      /* loop for computation that assumes we've not fallen off top
         edge of the spectrum */
      while(++j<n && att>-120.){
        float b = toBark(.5*j/n*rate);
        if(b<centerbark+width*.5){
          /* still inside the F3 point */
          att = (b-centerbark)/(width*.5)*-3.;
        }else{
          /* outside the F3 point */
          att = (b-(width*.5+centerbark))*hislope-3.;
        }
        total += fromdB(att);
      }

      /* avoid area contraction at edges of spectrum */
      while(att>-120.){
        att += (barkw[n-1]*hislope);
        total += fromdB(att);
      }

      /* numerical integration complete to well beyond limits of reason */
      spread_att[i]=1./total;
    }

    /* precompute transition bins and attenuation at transition
       point. Do it empirically in a form parallel to actual
       computation for good roundoff match (and easy
       implementation) */

    for(i=0;i<n;i++){
      float att = 1.;
      float bw=barkw[i];
      for(j=i-1;j>=0 && bw<width*.5;j--){
        bw += barkw[j];
        att *= fromdB(-6.*barkw[j+1]/width);
      }
      if(j>=0){
        transition_lo[i]=j+1;
        transition_lo_att[i]=att;
      }else{
        transition_lo[i]=0;
        transition_lo_att[i]=0.;
      }

      att = 1.;
      bw=0.;
      for(j=i+1;j<n && bw<width*.5;j++){
        bw += barkw[j];
        att *= fromdB(-6.*barkw[j-1]/width);
      }
      if(j<n){
        transition_hi[i]=j-1;
        transition_hi_att[i]=att;
      }else{
        transition_hi[i]=n-1;
        transition_hi_att[i]=0.;
      }
    }
  }

  /* seed filter transitions */
  memset(out,0,sizeof(*out)*n);
  memset(work,0,sizeof(work));
  for(i=0;i<n;i++){
    float e0 = fromdB(in[i])*fromdB(in[i])*spread_att[i];
    out[transition_lo[i]]+=e0*transition_lo_att[i];
  }

  float dB3=0.;
  float dBlo=0.;
  for(i=n-1;i>=0;i--){
    float e0 = fromdB(in[i])*fromdB(in[i])*spread_att[i];
    dB3  += e0;
    dB3  -= out[i];
    dB3   = (dB3<0?0:dB3);
    dBlo += out[i];
    out[i] = dB3+dBlo;
    dB3  *= fromdB(-6.*barkw[i]/width);
    dBlo *= fromdB(loslope*barkw[i]);
  }

  for(i=0;i<n;i++){
    float e0 = fromdB(in[i])*fromdB(in[i])*spread_att[i];
    work[transition_hi[i]]+=e0*transition_hi_att[i];
  }

  dB3=0.;
  dBlo=0.;
  for(i=0;i<n;i++){
    float e0 = fromdB(in[i])*fromdB(in[i])*spread_att[i];
    out[i] += dB3+dBlo;

    dB3  += e0;
    dB3  -= work[i];
    dB3   = (dB3<0?0:dB3);
    dBlo += work[i];
    dB3  *= fromdB(-6.*barkw[i]/width);
    dBlo *= fromdB(hislope*barkw[i]);
  }

  for(i=0;i<n;i++)
    out[i]=todB(sqrt(out[i]));

}

void envelope(pcm_t *pcm,float **current_spread,cairo_t *c,off_t pos){
  int i;
  cairo_save(c);
  clip_area(c,SPECTRUM_AREA);
  cairo_set_line_width(c,.7);

  for(i=0;i<pcm->ch;i++)
    frequency_vector_dB(current_spread[i],i+pcm->ch,BLOCKSIZE/2+1,ENVELOPE_AREA,c,0);

  cairo_restore(c);
}

static int seed_chirps(float *fft,float *dB,float *spread,
                       chirp *chirps,int *n){
  int i;
  int ret=0;

#if 0
  if(dB[0]>spread[0]+5 &&
     dB[0]>dB[1] &&
     dB[0]>-100){
    if(*n<MAX_CPC){
      chirps[*n].A=fft[0];
      chirps[*n].W=.000001;
      chirps[*n].P=0;
      chirps[*n].dA=0;
      chirps[*n].dW=0;
      chirps[*n].label=-1;
      ret=1;
      (*n)++;
    }
  }

  if(dB[BLOCKSIZE/2]>spread[BLOCKSIZE/2]+5 &&
     dB[BLOCKSIZE/2]>dB[BLOCKSIZE/2-1]&&
     dB[BLOCKSIZE/2]>-100){
    if(*n<MAX_CPC){
      chirps[*n].A=fft[BLOCKSIZE-1];
      chirps[*n].W=M_PI;
      chirps[*n].P=M_PI*.5;
      chirps[*n].dA=0;
      chirps[*n].dW=0;
      chirps[*n].label=-1;
      ret=1;
      (*n)++;
    }
  }
#endif
  for(i=3;i<BLOCKSIZE/2;i++){
    if(dB[i]>spread[i]+5 &&
       dB[i]>dB[i-1] &&
       dB[i]>dB[i+1] &&
       dB[i]>-100){
      if(*n<MAX_CPC){
        chirps[*n].A=fromdB(dB[i]);
        chirps[*n].W=(float)i/BLOCKSIZE*2.*M_PI;
        chirps[*n].P=-atan2f(fft[i*2],fft[i*2-1]);
        chirps[*n].dA=0;
        chirps[*n].dW=0;
        chirps[*n].label=-1;
        ret=1;
        (*n)++;
      }else
        break;
    }
  }

  return ret;
}

void dump_waveform(char *base,int no,float *d,int n){
  char buf[80];
  FILE *f;
  snprintf(buf,80,"%s_%d.m",base,no);
  f=fopen(buf,"w");
  if(f){
    int i;
    for(i=0;i<n;i++)
      fprintf(f,"%f\n",d[i]);
    fclose(f);
  }
}


static int fitno=0;
void track(pcm_t *pcm,
           float *window,
           float **current_block,
           float **current_fft,
           float **current_dB,
           float **current_spread,
           chirp **chirps,
           int *chirps_used,
           cairo_t *c,
           off_t off,
           float **y,
           float **r){
  int i,j,k;

  static int count=0;
  fprintf(stderr,"pos: %ld\n",(long)off);
  float rec[BLOCKSIZE];
  for(i=0;i<BLOCKSIZE;i++)rec[i]=1.;


  for(i=0;i<pcm->ch;i++){
    float lfft[BLOCKSIZE];
    float ldB[BLOCKSIZE/2+1];
    float lspread[BLOCKSIZE/2+1];
    chirp prev[MAX_CPC];
    int prev_chirps;
    int flag=1;
    int ret;
    memcpy(prev,chirps[i],chirps_used[i]*sizeof(**chirps));
    prev_chirps = chirps_used[i];

    /* advance preexisting peaks */
    advance_chirps(chirps[i], chirps_used[i], BLOCKSIZE/AREA_VZOOM[TRACK_AREA]);
    for(j=0;j<chirps_used[i];j++)chirps[i][j].label=j;

    fprintf(stderr,"FITNO: %d\n",fitno);
    fprintf(stderr,"chirps: %d\n",chirps_used[i]);
    for(j=0;j<chirps_used[i];j++)
      fprintf(stderr,"in>>%f:%f:%f::%f:%f ",chirps[i][j].A,chirps[i][j].W*BLOCKSIZE/2./M_PI,chirps[i][j].P,chirps[i][j].dA*BLOCKSIZE,chirps[i][j].dW*BLOCKSIZE*BLOCKSIZE/2/M_PI);

    //ret=estimate_chirps(current_block[i],y[i],r[i],window,BLOCKSIZE,
    //                  chirps[i],chirps_used[i],50,.01);
    for(j=0;j<chirps_used[i];j++)
      fprintf(stderr,"out(%d)>>%f:%f:%f::%f:%f   ",ret,chirps[i][j].A,chirps[i][j].W*BLOCKSIZE/2./M_PI,chirps[i][j].P,chirps[i][j].dA*BLOCKSIZE,chirps[i][j].dW*BLOCKSIZE*BLOCKSIZE/2./M_PI);
    dump_waveform("x",fitno,current_block[i],BLOCKSIZE);
    dump_waveform("y",fitno,y[i],BLOCKSIZE);
    dump_waveform("r",fitno,r[i],BLOCKSIZE);
    fitno++;
    fprintf(stderr,"\n");

    /* perform spread on residue */
    memcpy(lfft, r[i], BLOCKSIZE*sizeof(*lfft));
    drft_forward(&fft, lfft);
    for(j=0;j<BLOCKSIZE;j++) lfft[j]*=4./BLOCKSIZE;
    mag_dB(ldB,lfft,BLOCKSIZE);
    spread_bark(ldB, lspread, BLOCKSIZE/2+1,
                pcm->rate,-20., 1., -20.);

    /* 5?: scan residue for peaks exceeding theshold; add to end of
       chirps array if no advanced chirp is within a bin */
    flag=seed_chirps(lfft,ldB,current_spread[i],
                     chirps[i],chirps_used+i);

    fprintf(stderr,"FITNO: %d\n",fitno);
    fprintf(stderr,"chirps: %d\n",chirps_used[i]);
    for(j=0;j<chirps_used[i];j++)
      fprintf(stderr,"in>>%f:%f:%f::%f:%f ",chirps[i][j].A,chirps[i][j].W*BLOCKSIZE/2./M_PI,chirps[i][j].P,chirps[i][j].dA*BLOCKSIZE,chirps[i][j].dW*BLOCKSIZE*BLOCKSIZE/2/M_PI);

    //ret=estimate_chirps(current_block[i],y[i],r[i],(count>10?rec:window),BLOCKSIZE,
    //                  chirps[i],chirps_used[i],50,.01);

    for(j=0;j<chirps_used[i];j++)
      fprintf(stderr,"out(%d)>>%f:%f:%f::%f:%f   ",ret,chirps[i][j].A,chirps[i][j].W*BLOCKSIZE/2./M_PI,chirps[i][j].P,chirps[i][j].dA*BLOCKSIZE,chirps[i][j].dW*BLOCKSIZE*BLOCKSIZE/2./M_PI);
    dump_waveform("x",fitno,current_block[i],BLOCKSIZE);
    dump_waveform("y",fitno,y[i],BLOCKSIZE);
    dump_waveform("r",fitno,r[i],BLOCKSIZE);
    fitno++;
    fprintf(stderr,"\n");

#if 0
    for(k=0;k<2;k++){
      /* fit/track */
      estimate_chirps(current_block[i],y[i],r[i],window,BLOCKSIZE,
                      chirps[i],chirps_used[i],50,.01);

      /* perform spread on residue */
      memcpy(lfft, r[i], BLOCKSIZE*sizeof(*lfft));
      drft_forward(&fft, lfft);
      for(j=0;j<BLOCKSIZE;j++) lfft[j]*=4./BLOCKSIZE;
      mag_dB(ldB,lfft,BLOCKSIZE);
      //spread_bark(ldB, lspread, BLOCKSIZE/2+1,
      //          pcm->rate,-40., 1., -40.);

      /* 5?: scan residue for peaks exceeding theshold; add to end of
         chirps array if no advanced chirp is within a bin */
      flag=seed_chirps(lfft,ldB,current_spread[i],
                       chirps[i],chirps_used+i);

      fprintf(stderr,"chirps: %d\n",chirps_used[i]);
      /* 6?: if residue peaks, loop */
    }
#endif
    /* 7: compare all chirps against original spread; cull those
       below threshold, also remove them from reconstruction  */
    //cull_chirps(current_fft[i],current_dB[i],current_spread[i],
    //          chirps[i],chirps_used+i);

    /* 8: draw telemetry */
    /* The spectrum gets lollypops */
    {
      float r,g,b;
      int ytop = area_y(SPECTRUM_AREA);
      int h = area_h(SPECTRUM_AREA);
      int hzoom = AREA_HZOOM[SPECTRUM_AREA];
      int xw = (hzoom<4?4:hzoom);
      cairo_save(c);
      clip_area(c,SPECTRUM_AREA);
      cairo_set_line_width(c,.7);

      for(j=0;j<chirps_used[i];j++){
        float x = chirps[i][j].W*BLOCKSIZE*hzoom/2./M_PI;
        float y1 = todB(chirps[i][j].A - chirps[i][j].dA*.5)*(1./MINdB);
        float y2 = todB(chirps[i][j].A + chirps[i][j].dA*.5)*(1./MINdB);

        cairo_move_to(c,x+.5,ytop+y1*h+.5);
        cairo_line_to(c,x+.5,ytop+h-.5);

        cairo_move_to(c,x+.5-xw,ytop+y1*h+.5);
        cairo_line_to(c,x+.5+xw,ytop+y1*h+.5);
        cairo_move_to(c,x+.5-xw,ytop+y2*h+.5);
        cairo_line_to(c,x+.5+xw,ytop+y2*h+.5);

      }
      cairo_set_source_rgb(c,0.,1.,1.);
      //pick_a_color(i,&r,&g,&b);
      cairo_stroke(c);
      cairo_restore(c);
    }

    /* the spectrogram gets tracer lines */
    {
      float r,g,b;
      int ytop = area_y(SPECTROGRAM_AREA);
      int h = area_h(SPECTROGRAM_AREA);
      int hzoom = AREA_HZOOM[SPECTROGRAM_AREA];
      int vzoom = AREA_VZOOM[SPECTROGRAM_AREA]/AREA_VZOOM[SPECTROGRAM_AREA];
      cairo_save(c);
      clip_area(c,SPECTROGRAM_AREA);
      cairo_set_line_width(c,.7);

      for(j=0;j<chirps_used[i];j++){
        if(chirps[i][j].label>=0){
          float x1 = prev[chirps[i][j].label].W*BLOCKSIZE*hzoom/2./M_PI;
          float x2 = chirps[i][j].W*BLOCKSIZE*hzoom/2./M_PI;
          cairo_move_to(c,x1+.5,ytop+h-vzoom);
          cairo_line_to(c,x2+.5,ytop+h);
        }else{
          float x1 = (chirps[i][j].W-chirps[i][j].dW*.5)*BLOCKSIZE*hzoom/2./M_PI;
          float x2 = chirps[i][j].W*BLOCKSIZE*hzoom/2./M_PI;
          cairo_move_to(c,x1+.5,ytop+h-vzoom);
          cairo_line_to(c,x2+.5,ytop+h);
        }
      }
      cairo_set_source_rgb(c,1.,1.,1.);
      //pick_a_color(i,&r,&g,&b);
      cairo_stroke(c);
      cairo_restore(c);
    }
  }
  count++;
}


int main(int argc, char **argv){
  float **current_block;
  float **current_fft;
  float **current_dB;
  float **current_spread;
  float **current_reconstruct;
  float **current_residue;
  float window[BLOCKSIZE];

  int i,j;

  pcm_t *pcm;
  cairo_t *c;
  chirp **chirps=NULL;
  int *chirps_used;

  int areas_active[AREAS]={1,1};
  off_t areas_pos[AREAS]={0,0};

  pic_w = BLOCKSIZE/2;
  pic_h = pic_w*9/16/2;

  drft_init(&fft, BLOCKSIZE);

  /* squishyload a file. */
  pcm = squishyio_load_file(argv[1]);
  current_block = calloc(pcm->ch, sizeof(*current_block));
  current_fft = calloc(pcm->ch, sizeof(*current_fft));
  current_dB = calloc(pcm->ch, sizeof(*current_dB));
  current_spread = calloc(pcm->ch, sizeof(*current_spread));
  current_residue = calloc(pcm->ch, sizeof(*current_residue));
  current_reconstruct = calloc(pcm->ch, sizeof(*current_reconstruct));

  for(i=0;i<pcm->ch;i++){
#if 0
    pcm->data[i]=realloc(pcm->data[i],
                         (pcm->samples+BLOCKSIZE*2)*sizeof(**pcm->data));
    memmove(pcm->data[i]+BLOCKSIZE,pcm->data[i],
            sizeof(**pcm->data)*pcm->samples);

    preextrapolate(pcm->data[i]+BLOCKSIZE,BLOCKSIZE,
                   pcm->data[i],BLOCKSIZE);
    postextrapolate(pcm->data[i]+pcm->samples,BLOCKSIZE,
                    pcm->data[i]+pcm->samples+BLOCKSIZE,BLOCKSIZE);
#endif
    current_block[i] = calloc(BLOCKSIZE, sizeof(**current_block));
    current_fft[i] = calloc(BLOCKSIZE, sizeof(**current_fft));
    current_dB[i] = calloc(BLOCKSIZE/2+1, sizeof(**current_dB));
    current_spread[i] = calloc(BLOCKSIZE/2+1, sizeof(**current_spread));
    current_residue[i] = calloc(BLOCKSIZE, sizeof(**current_residue));
    current_reconstruct[i] = calloc(BLOCKSIZE, sizeof(**current_reconstruct));

  }
#if 0
  pcm->samples+=BLOCKSIZE*2-1;
#endif
  pcm->samples=(pcm->samples/BLOCKSIZE)*BLOCKSIZE;

  /* Make a cairo drawable */
  cairo_surface_t *pane=cairo_image_surface_create(CAIRO_FORMAT_RGB24,pic_w, pic_h);
  if(!pane || cairo_surface_status(pane)!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not set up Cairo surface.\n\n");
    exit(1);
  }

  c=cairo_create(pane);
  cairo_set_source_rgb(c,0,0,0);
  cairo_rectangle(c,0,0,pic_w,pic_h);
  cairo_fill(c);
  cairo_set_font_size(c, SPECTRUM_GRIDFONTSIZE);

  printf("YUV4OGG Sy\n");
  printf("VIDEO W%d H%d F%d:%d Ip A1:1 C444\n",
         pic_w,pic_h,pcm->rate,BLOCKSIZE/AREA_VZOOM[AREA_FRAMEOUT_TRIGGER]);
  printf("AUDIO R%d C%d\n",pcm->rate,pcm->ch);

  chirps=calloc(pcm->ch,sizeof(*chirps));
  chirps_used=calloc(pcm->ch,sizeof(*chirps_used));
  for(i=0;i<pcm->ch;i++)
    chirps[i]=calloc(MAX_CPC,sizeof(**chirps));

  for(j=0;j<BLOCKSIZE;j++)window[j]=1.;
  //hanning(window,window,BLOCKSIZE);

  while(1){

    /* which area is next? */
    off_t min=0;
    off_t min_area=-1;
    for(i=0;i<AREAS;i++)
      if(areas_active[i] &&
         (min_area==-1 || min>areas_pos[i])){
        min=areas_pos[i];
        min_area=i;
      }

    if(min+BLOCKSIZE>pcm->samples) break;

    /* 'pos' is the center of the current window of BLOCKSIZE */
    for(i=0;i<pcm->ch;i++){
      memcpy(current_block[i], &pcm->data[i][min],
             BLOCKSIZE*sizeof(**current_block));

      /* window and drft */
      for(j=0;j<BLOCKSIZE;j++)current_fft[i][j]=window[j]*current_block[i][j];
      drft_forward(&fft, current_fft[i]);
      for(j=0;j<BLOCKSIZE;j++) current_fft[i][j]*=4./BLOCKSIZE;
      mag_dB(current_dB[i],current_fft[i],BLOCKSIZE);
      spread_bark(current_dB[i], current_spread[i], BLOCKSIZE/2+1,
                  pcm->rate,-20., 1., -20.);
    }

    if(min_area == SPECTROGRAM_AREA)
      spectrogram(pcm,current_dB,current_spread,c,min);

    if(min_area == CHIRPOGRAM_AREA)
      chirpogram(pcm,window,current_block,current_dB,current_spread,c,min);

    if(min_area == SPECTRUM_AREA)
      spectrum(pcm,current_dB,c,min);

    if(min_area == WAVEFORM_AREA)
      waveform(pcm,window,current_block,c,min);

    if(min_area == ENVELOPE_AREA)
      envelope(pcm,current_spread,c,min);

    if(min_area == TRACK_AREA)
      track(pcm,window,current_block,current_fft,current_dB,current_spread,
            chirps,chirps_used,c,min,
            current_reconstruct,current_residue);

    /* lastly */
    if(min_area == AREA_FRAMEOUT_TRIGGER){
      write_yuv(pcm,pane);
      write_pcm(pcm,pcm->data,areas_pos[min_area],BLOCKSIZE/AREA_VZOOM[min_area]);
    }
    areas_pos[min_area]+=BLOCKSIZE/AREA_VZOOM[min_area];
  }
  return 0;
}

