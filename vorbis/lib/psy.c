/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.23.4.6 2000/08/01 02:00:10 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

/* Why Bark scale for encoding but not masking computation? Because
   masking has a strong harmonic dependancy */

/* the beginnings of real psychoacoustic infrastructure.  This is
   still not tightly tuned */
void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    free(i);
  }
}

/* Set up decibel threshhold slopes on a Bark frequency scale */
/* ATH is the only bit left on a Bark scale.  No reason to change it
   right now */
static void set_curve(double *ref,double *c,int n, double crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK(i+1)*2*n/crate);
    double base=ref[i];
    if(j<endpos){
      double delta=(ref[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
	c[j]=base;
	base+=delta;
      }
    }
  }
}

static void min_curve(double *c,
		       double *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(double *c,
		       double *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(double *c,double att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static void linear_curve(double *c){
  int i;  
  for(i=0;i<EHMER_MAX;i++)
    if(c[i]<=-200.)
      c[i]=0.;
    else
      c[i]=fromdB(c[i]);
}

static void interp_curve(double *c,double *c1,double *c2,double del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.-del);
}

static void setup_curve(double **c,
			int band,
			double *curveatt_dB){
  int i,j;
  double ath[EHMER_MAX];
  double tempc[P_LEVELS][EHMER_MAX];

  memcpy(c[0],c[4],sizeof(double)*EHMER_MAX);
  memcpy(c[2],c[4],sizeof(double)*EHMER_MAX);

  /* we add back in the ATH to avoid low level curves falling off to
     -infinity and unneccessarily cutting off high level curves in the
     curve limiting (last step).  But again, remember... a half-band's
     settings must be valid over the whole band, and it's better to
     mask too little than too much, so be pessimal. */

  for(i=0;i<EHMER_MAX;i++){
    double oc_min=band*.5-1+(i-EHMER_OFFSET)*.125;
    double oc_max=band*.5-1+(i-EHMER_OFFSET+1)*.125;
    double bark=toBARK(fromOC(oc_min));
    int ibark=floor(bark);
    double del=bark-ibark;
    double ath_min,ath_max;

    if(ibark<26)
      ath_min=ATH_Bark_dB[ibark]*(1.-del)+ATH_Bark_dB[ibark+1]*del;
    else
      ath_min=200.;

    bark=toBARK(fromOC(oc_max));
    ibark=floor(bark);
    del=bark-ibark;

    if(ibark<26)
      ath_max=ATH_Bark_dB[ibark]*(1.-del)+ATH_Bark_dB[ibark+1]*del;
    else
      ath_max=200.;

    ath[i]=min(ath_min,ath_max);
  }

  /* The c array is comes in as dB curves at 20 40 60 80 100 dB.
     interpolate intermediate dB curves */
  for(i=1;i<P_LEVELS;i+=2){
    interp_curve(c[i],c[i-1],c[i+1],.5);
  }

  /* normalize curves so the driving amplitude is 0dB */
  /* make temp curves with the ATH overlayed */
  for(i=0;i<P_LEVELS;i++){
    attenuate_curve(c[i],curveatt_dB[i]);
    memcpy(tempc[i],ath,EHMER_MAX*sizeof(double));
    attenuate_curve(tempc[i],-i*10.);
    max_curve(tempc[i],c[i]);
  }

  /* Now limit the louder curves.

     the idea is this: We don't know what the playback attenuation
     will be; 0dB SL moves every time the user twiddles the volume
     knob. So that means we have to use a single 'most pessimal' curve
     for all masking amplitudes, right?  Wrong.  The *loudest* sound
     can be in (we assume) a range of ...+100dB] SL.  However, sounds
     20dB down will be in a range ...+80], 40dB down is from ...+60],
     etc... */

  for(i=P_LEVELS-1;i>0;i--){
    for(j=0;j<i;j++)
      min_curve(c[i],tempc[j]);
  }

  /* take things out of dB domain into linear amplitude */
  for(i=0;i<P_LEVELS;i++)
    linear_curve(c[i]);
      
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate){
  long i,j;
  memset(p,0,sizeof(vorbis_look_psy));
  p->ath=malloc(n*sizeof(double));
  p->octave=malloc(n*sizeof(int));
  p->bark=malloc(n*sizeof(double));
  p->vi=vi;
  p->n=n;

  /* set up the lookups for a given blocksize and sample rate */
  /* Vorbis max sample rate is limited by 26 Bark (54kHz) */
  set_curve(ATH_Bark_dB, p->ath,n,rate);
  for(i=0;i<n;i++)
    p->ath[i]=fromdB(p->ath[i]);
  for(i=0;i<n;i++)
    p->bark[i]=toBARK(rate/(2*n)*i); 

  for(i=0;i<n;i++){
    int oc=toOC((i+.5)*rate/(2*n))*2.+2; /* half octaves, actually */
    if(oc<0)oc=0;
    if(oc>=P_BANDS)oc=P_BANDS-1;
    p->octave[i]=oc;
  }  

  p->tonecurves=malloc(P_BANDS*sizeof(double **));
  p->noiseatt=malloc(P_BANDS*sizeof(double **));
  p->peakatt=malloc(P_BANDS*sizeof(double *));
  for(i=0;i<P_BANDS;i++){
    p->tonecurves[i]=malloc(P_LEVELS*sizeof(double *));
    p->noiseatt[i]=malloc(P_LEVELS*sizeof(double));
    p->peakatt[i]=malloc(P_LEVELS*sizeof(double));
  }

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++){
      p->tonecurves[i][j]=malloc(EHMER_MAX*sizeof(double));
    }

  /* OK, yeah, this was a silly way to do it */
  memcpy(p->tonecurves[0][4],tone_125_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[0][6],tone_125_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[0][8],tone_125_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[0][10],tone_125_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[2][4],tone_125_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[2][6],tone_125_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[2][8],tone_125_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[2][10],tone_125_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[4][4],tone_250_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[4][6],tone_250_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[4][8],tone_250_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[4][10],tone_250_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[6][4],tone_500_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[6][6],tone_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[6][8],tone_500_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[6][10],tone_500_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[8][4],tone_1000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[8][6],tone_1000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[8][8],tone_1000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[8][10],tone_1000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[10][4],tone_2000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[10][6],tone_2000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[10][8],tone_2000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[10][10],tone_2000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[12][4],tone_4000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[12][6],tone_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[12][8],tone_4000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[12][10],tone_4000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[14][4],tone_8000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[14][6],tone_8000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[14][8],tone_8000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[14][10],tone_8000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[16][4],tone_8000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[16][6],tone_8000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[16][8],tone_8000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[16][10],tone_8000_100dB_SL,sizeof(double)*EHMER_MAX);

  /* interpolate curves between */
  for(i=1;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2){
      /*memcpy(p->tonecurves[i][j],p->tonecurves[i-1][j],EHMER_MAX*sizeof(double));*/
      interp_curve(p->tonecurves[i][j],
		   p->tonecurves[i-1][j],
		   p->tonecurves[i+1][j],.5);
    }

  /*for(i=0;i<P_BANDS-1;i++)
    for(j=4;j<P_LEVELS;j+=2)
    min_curve(p->tonecurves[i][j],p->tonecurves[i+1][j]);*/

  /* set up the final curves */
  for(i=0;i<P_BANDS;i++)
    setup_curve(p->tonecurves[i],i,vi->toneatt[i]);

  /* set up attenuation levels */
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++){
      p->peakatt[i][j]=fromdB(p->vi->peakatt[i][j]);
      p->noiseatt[i][j]=fromdB(p->vi->noiseatt[i][j]);
    }

}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)free(p->ath);
    if(p->octave)free(p->octave);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  free(p->tonecurves[i][j]);
	}
	free(p->noiseatt[i]);
	free(p->tonecurves[i]);
	free(p->peakatt[i]);
      }
      free(p->tonecurves);
      free(p->noiseatt);
      free(p->peakatt);
    }
    memset(p,0,sizeof(vorbis_look_psy));
  }
}

static void compute_decay(vorbis_look_psy *p,double *f, double *decay, int n){
  /* handle decay */
  int i;
  double decscale=1.-pow(p->vi->decay_coeff,n); 
  double attscale=1.-pow(p->vi->attack_coeff,n); 
  for(i=0;i<n;i++){
    double del=f[i]-decay[i];
    if(del>0)
      /* add energy */
      decay[i]+=del*attscale;
    else
      /* remove energy */
      decay[i]+=del*decscale;
    if(decay[i]>f[i])f[i]=decay[i];
  }
}

static long _eights[EHMER_MAX+1]={
  981,1069,1166,1272,
  1387,1512,1649,1798,
  1961,2139,2332,2543,
  2774,3025,3298,3597,
  3922,4277,4664,5087,
  5547,6049,6597,7194,
  7845,8555,9329,10173,
  11094,12098,13193,14387,
  15689,17109,18658,20347,
  22188,24196,26386,28774,
  31379,34219,37316,40693,
  44376,48393,52772,57549,
  62757,68437,74631,81386,
  88752,96785,105545,115097,
  125515};

static int seed_curve(double *flr,
		      double **curves,
		       double amp,double specmax,
		       int x,int n,double specatt,
		       int maxEH){
  int i;
  double *curve;

  /* make this attenuation adjustable */
  int choice=(int)((todB(amp)-specmax+specatt)/10.+.5);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);

  for(i=maxEH;i>=0;i--)
    if(((x*_eights[i])>>12)<n)break;
  maxEH=i;
  curve=curves[choice];

  for(;i>=0;i--)
    if(curve[i]>0.)break;
  
  for(;i>=0;i--){
    double lin=curve[i];
    if(lin>0.){
      double *fp=flr+((x*_eights[i])>>12);
      lin*=amp;	
      if(*fp<lin)*fp=lin;
    }else break;
  }    
  return(maxEH);
}

static void seed_peak(double *flr,
		      double *att,
		      double amp,double specmax,
		      int x,int n,double specatt){
  int prevx=(x*_eights[16])>>12;

  /* make this attenuation adjustable */
  int choice=rint((todB(amp)-specmax+specatt)/10.+.5);
  if(choice<0)choice=0;
  if(choice>=P_LEVELS)choice=P_LEVELS-1;

  if(prevx<n){
    double lin=att[choice];
    if(lin){
      lin*=amp;	
      if(flr[prevx]<lin)flr[prevx]=lin;
    }
  }
}

static void seed_generic(vorbis_look_psy *p,
			 double ***curves,
			 double *f, 
			 double *flr,
			 double *seeds,
			 double specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  int maxEH=EHMER_MAX-1;

  /* prime the working vector with peak values */
  /* Use the 125 Hz curve up to 125 Hz and 8kHz curve after 8kHz. */
  for(i=0;i<n;i++)
    if(f[i]>flr[i])
      maxEH=seed_curve(seeds,curves[p->octave[i]],
		       f[i],specmax,i,n,vi->max_curve_dB,maxEH);
}

static void seed_att(vorbis_look_psy *p,
		     double **att,
		     double *f, 
		     double *flr,
		     double specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  
  for(i=0;i<n;i++)
    if(f[i]>flr[i])
      seed_peak(flr,att[p->octave[i]],f[i],
		specmax,i,n,vi->max_curve_dB);
}

static void seed_point(vorbis_look_psy *p,
		     double **att,
		     double *f, 
		     double *flr,
		     double specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  
  for(i=0;i<n;i++){
    /* make this attenuation adjustable */
    int choice=rint((todB(f[i])-specmax+vi->max_curve_dB)/10.+.5);
    double lin;
    if(choice<0)choice=0;
    if(choice>=P_LEVELS)choice=P_LEVELS-1;
    lin=att[p->octave[i]][choice]*f[i];
    if(flr[i]<lin)flr[i]=lin;
  }
}

/* bleaugh, this is more complicated than it needs to be */
static void max_seeds(vorbis_look_psy *p,double *seeds,double *flr){
  long n=p->n,i,j;
  long *posstack=alloca(n*sizeof(long));
  double *ampstack=alloca(n*sizeof(double));
  long stack=0;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
	if(seeds[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}else{
	  if(i<posstack[stack-1]*1.0905077080){
	    if(stack>1 && ampstack[stack-1]<ampstack[stack-2] &&
	       i<posstack[stack-2]*1.0905077080){
	      /* we completely overlap, making stack-1 irrelevant.  pop it */
	      stack--;
	      continue;
	    }
	  }
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;

	}
      }
    }
  }

  /* the stack now contains only the positions that are relevant. Scan
     'em straight through */
  {
    long pos=0;
    for(i=0;i<stack;i++){
      long endpos;
      if(i<stack-1 && ampstack[i+1]>ampstack[i]){
	endpos=posstack[i+1];
      }else{
	endpos=posstack[i]*1.0905077080+1; /* +1 is important, else bin 0 is
                                       discarded in short frames */
      }
      if(endpos>n)endpos=n;
      for(j=pos;j<endpos;j++)
	if(flr[j]<ampstack[i])
	  flr[j]=ampstack[i];
      pos=endpos;
    }
  }   

  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */
}

static void bark_noise(long n,double *b,double *f,double *noise){
  long i,lo=0,hi=0;
  double acc=0.;

  for(i=0;i<n;i++){

    for(;b[hi]-.5<b[i] && hi<n;hi++)
      acc+=f[hi]*f[hi];
    for(;b[lo]+.5<b[i];lo++)
      acc-=f[lo]*f[lo];
    if(hi-lo>0)
      noise[i]=sqrt(acc/(hi-lo));
    else
      noise[i]=0.;
  }
}

/* stability doesn't matter */
static int comp(const void *a,const void *b){
  if(fabs(**(double **)a)<fabs(**(double **)b))
    return(1);
  else
    return(-1);
}

static int frameno=0;
void _vp_compute_mask(vorbis_look_psy *p,double *f, 
		      double *flr, 
		      double *decay){
  double *smooth=alloca(sizeof(double)*p->n);
  double *seed=alloca(sizeof(double)*p->n);
  int i,n=p->n;
  double specmax=0.;

  memset(flr,0,n*sizeof(double));
  memset(seed,0,n*sizeof(double));

  /* noise masking */
  if(p->vi->noisemaskp){
    /* don't use the smoothed data for noise */
    bark_noise(n,p->bark,f,smooth);
    /*_analysis_output("noise",frameno,work2,n,0,1);*/
    seed_point(p,p->noiseatt,smooth,flr,specmax);
  }

  for(i=0;i<n;i++)smooth[i]=fabs(f[i]);

  if(p->vi->smoothp){
    /* compute power^.5 of three neighboring bins to smooth for peaks
       that get split twixt bins/peaks that nail the bin.  This evens
       out treatment as we're not doing additive masking any longer. */
    double acc=smooth[0]*smooth[0]+smooth[1]*smooth[1];
    double prev=smooth[0];

    smooth[0]=sqrt(acc);
    for(i=1;i<n-1;i++){
      double this=smooth[i];
      acc+=smooth[i+1]*smooth[i+1];
      smooth[i]=sqrt(acc);
      acc-=prev*prev;
      prev=this;
    }
    smooth[n-1]=sqrt(acc);
  }

  /* find the highest peak so we know the limits */
  for(i=0;i<n;i++){
    if(smooth[i]>specmax)specmax=smooth[i];
  }
  specmax=todB(specmax);

  /* set the ATH (floating below specmax by a specified att) */
  if(p->vi->athp){
    double att=specmax+p->vi->ath_adjatt;
    if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;
    att=fromdB(att);

    for(i=0;i<n;i++){
      double av=p->ath[i]*att;
      if(av>flr[i])flr[i]=av;
    }
  }

  /* peak attenuation */
  if(p->vi->peakattp){
    memset(seed,0,n*sizeof(double));
    seed_att(p,p->peakatt,smooth,seed,specmax);
    max_seeds(p,seed,flr);
  }

  /* seed the tone masking */
  if(p->vi->tonemaskp){
    memset(seed,0,n*sizeof(double));
    seed_generic(p,p->tonecurves,smooth,flr,seed,specmax);
    
    /* chase the seeds */
    max_seeds(p,seed,seed);

    /* compute, update and apply decay accumulator */
    if(p->vi->decayp)
      compute_decay(p,seed,decay,n);

    for(i=0;i<n;i++)if(flr[i]<seed[i])flr[i]=seed[i];

  }

  /* noise culling */
  if(p->vi->noisecullp){
    int j;

    /* march down from Nyquist to the first peak that exceeds the
       masking threshhold */
    for(i=n-1;i>0;i--){
      if(flr[i]<fabs(f[i])){
	break;
      }else{
	f[i]=0;
      }
    }

    /* take the edge off the steep ATH rise in treble */
    /*for(j=i+1;j<n;j++)
      flr[j]=flr[j-1];*/

    /* Smooth the falloff a bit */

    /* cull out addiitonal bands */


  }

  frameno++;
}


/* this applies the floor and (optionally) tries to preserve noise
   energy in low resolution portions of the spectrum */
/* f and flr are *linear* scale, not dB */
void _vp_apply_floor(vorbis_look_psy *p,double *f, double *flr){
  double *work=alloca(p->n*sizeof(double));
  int i,j,addcount=0;

  /* subtract the floor */
  for(j=0;j<p->n;j++){
    if(flr[j]<=0)
      work[j]=0.;
    else
      work[j]=f[j]/flr[j];
  }

  memcpy(f,work,p->n*sizeof(double));
}


