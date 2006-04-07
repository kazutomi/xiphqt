#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if !defined(M_PI)
# define M_PI (3.1415926535897932384626433832795)
#endif

/*This program computes a set of quantization matrices using the same CSF
   functions that were used to generate the CSF filters in the psychovisual
   model.
  The main difference is that instead of a full FIR filter, only a single
   scale factor for each coefficient is derived.

  The same quantization matrices are used for both inter and intra modes.
  The reason is that human sensitivity to errors is the same regardless of
   what mode happened to be used to code a macro block.
  This assumption ignores statistical correlation between the use of intra
   mode and scene changes or other high frequency temporal effects which could
   actually have an effect on perception, because our current psychovisual
   model does not take these into account.*/

/*The number of phases to sample at each frequency.*/
#define NPHASES   (16)
/*The number of frequencies to sample (in the interval [0,pi)).*/
#define NFREQS    (1024)

/*The magnitude of each DCT basis vector.
  For a non-orthogonal transform, one per coefficient will be needed.
  The Theora DCT function scales its ouput by 2 in each dimension.*/
#define DCT_SCALE (2.0)

/*The relative sensitivity to changes in each color channel.
  This is derived by making small changes in each color channel individually
   and measuring the distance to the new color in L*a*b space.
  A linear regression with an intercept of 0 was then fit to the measured
   distances.
  These are the slopes of the fit lines.
  See colorreg.c for details.

  These values depend on the particular color space one is using, but we'd like
   to derive generic quantization matrices.
  Therefore we use the geometric average over all color spaces (currently only
   two).
  Minor mismatches in the fit of the matrices to actual CSF curves will not
   harm the encoder, since it uses adaptive thresholding and amplitude
   reduction based on per-coefficient sensitivity measurements.*/
const float OC_YCbCr_SCALE[3]={
  0.62660577006129702908501F,
  1.33082085846485050021660F,1.08759046924374083944404F
  /*JND values:
  1.59589976310332425729864F,
  0.75141593523980064237395F,0.91946373959616699828994F
  */
};



/*Allocates a 2-dimesional array of the given size.
  _height: The number of columns.
  _row:    The number of rows.
  _sz:     The size of each element.*/
static void **alloc_2d(size_t _height,size_t _width,size_t _sz){
  size_t  colsz;
  size_t  datsz;
  char   *ret;
  colsz=_height*sizeof(void *);
  datsz=_sz*_width*_height;
  ret=(char *)malloc(datsz+colsz);
  if(ret!=NULL){
    size_t   rowsz;
    size_t   i;
    void   **p;
    char    *datptr;
    p=(void **)ret;
    i=_height;
    rowsz=_sz*_width;
    for(datptr=ret+colsz;i-->0;p++,datptr+=rowsz)*p=(void *)datptr;
  }
  return (void **)ret;
}

/*cos(n*pi/2) (resp. sin(m*pi/2)) accurate to 16 bits.*/
#define OC_C1S7D (64277.0/65536)
#define OC_C2S6D (60547.0/65536)
#define OC_C3S5D (54491.0/65536)
#define OC_C4S4D (46341.0/65536)
#define OC_C5S3D (36410.0/65536)
#define OC_C6S2D (25080.0/65536)
#define OC_C7S1D (12785.0/65536)

/*A floating-point implementation of the Theora DCT.
  Here we pretend that the transform is linear, and ignore the non-linearities
   introduced by rounding and truncation in the integer version.*/
static void fdct8d(double _y[8],double _x[8]){
  double t[8];
  double u;
  double v;
  t[0]=_x[0]+_x[7];
  t[1]=_x[1]+_x[2];
  t[2]=_x[3]+_x[4];
  t[3]=_x[5]+_x[6];
  t[4]=_x[5]-_x[6];
  t[5]=_x[3]-_x[4];
  t[6]=_x[1]-_x[2];
  t[7]=_x[0]-_x[7];
  /*Butterflies for ouputs 0 and 4.*/
  u=t[0]+t[2];
  v=t[1]+t[3];
  _y[0]=OC_C4S4D*(u+v);
  _y[4]=OC_C4S4D*(u-v);
  /*Apply rotation for outputs 2 and 6.*/
  u=t[0]-t[2];
  v=t[6]-t[4];
  _y[2]=OC_C2S6D*u+OC_C6S2D*v;
  _y[6]=OC_C6S2D*u-OC_C2S6D*v;
  /*Compute some common terms.*/
  t[3]=OC_C4S4D*(t[1]-t[3]);
  t[4]=-OC_C4S4D*(t[6]+t[4]);
  /*Apply rotation for outputs 1 and 7.*/
  u=t[7]+t[3];
  v=t[4]-t[5];
  _y[1]=OC_C1S7D*u-OC_C7S1D*v;
  _y[7]=OC_C7S1D*u+OC_C1S7D*v;
  /*Apply rotation for outputs 3 and 5.*/
  u=t[7]-t[3];
  v=t[4]+t[5];
  _y[3]=OC_C3S5D*u-OC_C5S3D*v;
  _y[5]=OC_C5S3D*u+OC_C3S5D*v;
}

/*A Contrast Sensitivity Function.*/
typedef double (*csf_func)(const void *_ctx,double _f);

/*A set of CSF filters, one for each DCT coefficient.*/
typedef double csf_filter_bank[8];

/*Finds a filter bank for a given CSF function.
  _filters: Will contain the optimal filter bank.
  _csf:     The CSF function.
  _csf_ctx: The parameters of the CSF function.
  _scratch: An NFREQS*NPHASES*8 array of scratch space.*/
static void find_csf_filters(csf_filter_bank _filters,csf_func _csf,
 const void *_csf_ctx){
  double  ata[8];
  double  atb[8];
  double  x[8];
  double  y[8];
  int     row;
  int     f;
  int     p;
  int     c;
  int     i;
  memset(ata,0,sizeof(ata));
  memset(atb,0,sizeof(atb));
  /*Find the response of each coefficient for each frequency and phase, and
     the expected response of CSF-sensitive coefficients.*/
  for(row=f=0;f<NFREQS;f++){
    double cs;
    /*Compute the CSF weight at this frequency.*/
    cs=_csf(_csf_ctx,f/(double)NFREQS)/DCT_SCALE;
    for(p=0;p<NPHASES;p++,row++){
      double theta;
      double dtheta;
      dtheta=f*M_PI/NFREQS;
      theta=p*(2*M_PI)/NPHASES;
      /*Fill in a perfect cosine curve at this frequency/phase.*/
      for(i=0;i<8;i++){
        x[i]=cos(theta);
        theta+=dtheta;
      }
      /*Find the transformed coefficient values of this curve.*/
      fdct8d(y,x);
      /*Add the equation for each coefficient to our linear systems.*/
      for(c=0;c<8;c++){
        ata[c]+=y[c]*y[c];
        atb[c]+=y[c]*y[c]*cs;
      }
    }
  }
  /*We now have the system Ax=b, where x is the filter coefficients we want to
     solve for, and each row of A is a set of actual coefficients, and b is the
     CSF-scaled coefficient we actually want as output.
    There is a row of A for each frequency and phase, but we only have 1
     filter coefficient, so we find the solution which minimizes (Ax-b)^2.
    This is done by solving (A^T A)x = A^T b, which conveniently is a 1x1
     system, and thus easy to solve.*/
  for(c=0;c<8;c++)_filters[c]=atb[c]/ata[c];
}

/*CSF function parameters.
  All values are derived from the measurements in Chapter 5 of Nadenau's Ph.D.
   thesis \cite{Nad00}.
  We use his measurements because they were actually performed in a Y'CbCr
   space, though he does not specify which one.

  The extensions to supra-threshold levels use the model proposed by Mark
   Cannon Jr. \cite{Can85}.
  There is no clear theory for how supra-threshold contrast should be handled.
  The law of contrast constancy \cite{GS75} suggests that quantization should
   be uniform at high levels of contrast.
  However, recent experiments on wavelet compression suggest that, although the
   effect of the CSF is reduced at supra-threshold levels, it still provides
   better perceptual quality than uniform quantization \cite{CH03}.
  Purported reasons for this are the better preservation of edge structure when
   details are removed from the fine scales (highest frequencies) first.

  When Cannon's results are adapted to our framework, correcting for the change
   from Michelson contrast to our model, the result is that the shape of the
   CSF curve is preserved, though to a lesser amplitude, throughout the entire
   useful contrast range.
  This corresponds very well with the findings of Chandler and Hemami.
  The advantage of using Cannon's scheme over theirs is that Cannon's is very
   simple, and contains no unexplained magic constants.
  Current results from Chandler and Hemami give only a small benefit over
   JPEG200.

  @ARTICLE{Can85,
    author="Mark W. Cannon, Jr.",
    title="Perceived contrast in the fovea and periphery",
    journal="Journal of the Optical Society of America A",
    volume=2,
    number=10,
    pages="1760--1758",
    month="Oct.",
    year=1985
  }

  @ARTICLE{GS75,
    author="M. A. Georgeson and G. D. Sullivan",
    title="Contrast constancy: Deblurring in human vision by spatial frequency
     channels",
    journal="The Journal of Physiology",
    volume=252,
    number=3,
    pages="627--656",
    month="Nov.",
    year=1975
  }

  @INPROCEEDINGS{CH03,
    author="Damon M. Chandler and Sheila S. Hemami",
    title="Suprathreshold image compression based on contrast allocation and
     global precedence",
    booktitle="Proc. Human Vision and Electronic Imaging 2003",
    address="Santa Clara, CA",
    month="Jan.",
    year=2003
  }

  @PHDTHESIS{Nad00,
    author="Marcus Nadenau",
    title="Integration of Human Color Visual Models into High
     Quality Image Compression",
    school="\'{E}cole Polytechnique F\'{e}d\'{e}rale de Lausanne",
    year=2000,
    URL="http://ltswww.epfl.ch/pub_files/nadenau/Thesis_Marcus_LQ.pdf"
  }*/

typedef struct csfl_ctx csfl_ctx;
typedef struct csfc_ctx csfc_ctx;
typedef struct csf_ctx  csf_ctx;

/*The luma CSF function context.*/
struct csfl_ctx{
  /*The frequency where the CSF attains its maximum.*/
  double peakf;
  /*The global scale factor.
    This accounts for small rounding errors in the parameters Nadenau reports
     so that the function maximum is actually 1.0.*/
  double s;
  /*The actual parameters from Table 5.3 on page 83 of Nadenau's thesis.
    There is an error in this table: the roles of a1 and a2 are reversed.
    Simply swapping the data for these two parameters yields correct results.*/
  double a1;
  double a2;
  double b1;
  double b2;
  double c1;
  double c2;
};

/*The luma CSF function.*/
static double csfl(csfl_ctx *_ctx,double _f){
  return _f<_ctx->peakf?1:
   _ctx->s*(_ctx->a1*(_f*_f)*exp(_ctx->b1*pow(_f,_ctx->c1))+
   _ctx->a2*exp(_ctx->b2*pow(_f,_ctx->c2)));
}

/*The actual luma CSF parameters.*/
static const csfl_ctx Y_CTX={
  3.74695975597140362,
  0.999941996555815748,
  0.9973,0.221,
  -0.9699,-0.800,
  0.7578,1.999
};

/*The chroma CSF function context.*/
struct csfc_ctx{
  /*The actual parameters from Table 5.4 on page 83 of Nadenau's thesis.*/
  double a1;
  double b1;
  double c1;
};

/*The chroma CSF function.*/
static double csfc(csfc_ctx *_ctx,double _f){
  return _ctx->a1*exp(_ctx->b1*pow(_f,_ctx->c1));
}

/*The actual Cr CSF parameters.*/
static const csfc_ctx CR_CTX={
  1.000,-0.1521,0.893
};

/*The actual Cb CSF parameters.*/
static const csfc_ctx CB_CTX={
  1.000,-0.2041,0.900
};

/*A general contrast-dependent CSF function context.*/
struct csf_ctx{
  /*The threshold-level CSF function.*/
  csf_func    c0csf;
  /*The threshold-level CSF function parameters.*/
  const void *c0csf_ctx;
  /*The Nyquist frequency of this channel.*/
  double      fmax;
  /*The frequency where the threshold-level CSF funtion attains its maximum.*/
  double      peakf;
  /*A scale parameter used to normalize the curve to have a maximum of 1.0.*/
  double      scale;
  /*The contrast level.*/
  double      contrast;
};

/*The exponent parameter in the supra-threshold model.
  Values between 0.4 and 0.6 are reasonable.*/
#define CONTRAST_EXP (0.5)
/*The scale parameter of the contrast level.
  Cannon does not report values for this parameter, and it would differ for us
   anyway since we do not use Michelson contrast
   ((L_max-L_min)/(L_max+L_min)).
  It must be sufficiently small so that physical contrast does not actually
   decrease as perceptual contrast increases.*/
#define CONTRAST_SCALE (1.0/64)

/*The main contrast-dependent CSF function.*/
static double csf(csf_ctx *_ctx,double _f){
  double f;
  double s0;
  f=_f*_ctx->fmax;
  s0=_ctx->c0csf(_ctx->c0csf_ctx,f);
  return _ctx->scale/(_ctx->contrast+1/s0);
}

/*This is the Nyquist frequency for a 45 dpi screen viewed from 50 cm out.
  This is assumed to be the closest reasonable viewing distance.*/
#define FREQUENCY_MAX (7.558704385696589)

/*Builds the CSF filter banks for each channel and contrast level.*/
static void build_csf_tables(csf_filter_bank _filters[5][64],
 double _qscale[64]){
  csf_ctx   csf_ctxs[5]={
    {
      (csf_func)csfl,
      &Y_CTX,
      FREQUENCY_MAX,
      3.74695975597140362,
    },
    {
      (csf_func)csfc,
      &CR_CTX,
      FREQUENCY_MAX,
      0,
    },
    {
      (csf_func)csfc,
      &CR_CTX,
      0.5*FREQUENCY_MAX,
      0,
    },
    {
      (csf_func)csfc,
      &CB_CTX,
      FREQUENCY_MAX,
      0,
    },
    {
      (csf_func)csfc,
      &CB_CTX,
      0.5*FREQUENCY_MAX,
      0,
    }
  };
  int chi;
  int qi;
  for(chi=0;chi<5;chi++){
    for(qi=0;qi<64;qi++){
      csf_ctxs[chi].contrast=pow(_qscale[qi]*CONTRAST_SCALE,1.0/CONTRAST_EXP);
      /*Find the normalization constant for this contrast level.*/
      csf_ctxs[chi].scale=csf_ctxs[chi].contrast+
       1/csf_ctxs[chi].c0csf(csf_ctxs[chi].c0csf_ctx,csf_ctxs[chi].peakf);
      find_csf_filters(_filters[chi][qi],(csf_func)csf,csf_ctxs+chi);
    }
  }
}

/*The resolution at which the base matrices are sampled over the range of qi
   values.*/
#define NQRANGES (3)

int main(void){
  static const int   QSIZES[NQRANGES]={15,16,32};
  static const int   QOFFS[NQRANGES+1]={0,15,31,63};
  /*The names of each pixel format.*/
  static const char *FORMAT_NAMES[4]={
    "4:4:4 subsampling",
    "4:2:2 subsampling",
    "subsampling in the Y direction",
    "4:2:0 subsampling"
  };
  /*The names of each color plane.*/
  static const char *CHANNEL_NAMES[3]={
    "Y'",
    "Cb",
    "Cr"
  };
  csf_filter_bank filters[5][64];
  double          fbase_matrices[9][64][64];
  unsigned char   base_matrices[9][64][64];
  int             ac_scale[64];
  int             dc_scale[64];
  double          qscales[64];
  double          bscale[NQRANGES+1][2];
  double          bmax[NQRANGES+1][2];
  double          bmin[NQRANGES+1][2];
  int             pci;
  int             pfi;
  int             qti;
  int             pli;
  int             chx;
  int             chy;
  int             qri;
  int             qi;
  int             ci;
  for(qi=0;qi<64;qi++){
    qscales[qi]=qi>=63?0.5F:1.5*pow(2,0.0625*(64-qi));
    ac_scale[qi]=65535;
    dc_scale[qi]=65535;
  }
  /*Compute all the CSF filters.*/
  build_csf_tables(filters,qscales);
  /*Build the base matrices.*/
  for(pci=0;pci<9;pci++){
    if(pci==0)chx=chy=pli=pfi=0;
    else{
       pli=(pci-1>>2)+1;
       pfi=pci-1&3;
       chx=(pli<<1)-1+(pfi&1);
       chy=(pli<<1)-1+(pfi>>1);
    }
    for(qi=0;qi<64;qi++)for(ci=0;ci<64;ci++){
      fbase_matrices[pci][qi][ci]=
       1/(OC_YCbCr_SCALE[pli]*filters[chx][qi][ci&7]*filters[chy][qi][ci>>3]);
    }
  }
  /*Scale the base matrices to be in the range of a single byte.*/
  for(qri=0;qri<=NQRANGES;qri++){
    qi=QOFFS[qri];
    bmax[qri][0]=bmax[qri][1]=1;
    bmin[qri][0]=bmin[qri][1]=65536*256;
    for(pci=0;pci<9;pci++)for(ci=0;ci<64;ci++){
      if(fbase_matrices[pci][qi][ci]>bmax[qri][ci!=0]){
        bmax[qri][ci!=0]=fbase_matrices[pci][qi][ci];
      }
      if(fbase_matrices[pci][qi][ci]<bmin[qri][ci!=0]){
        bmin[qri][ci!=0]=fbase_matrices[pci][qi][ci];
      }
    }
    for(ci=0;ci<2;ci++){
      double scale;
      scale=bmin[qri][ci]*qscales[qi]*100;
      if(scale>255/bmax[qri][ci])scale=255/bmax[qri][ci];
      bscale[qri][ci]=scale;
    }
  }
  /*Quantize the base matrices to integers at our sample points.*/
  for(pci=0;pci<9;pci++){
    for(qri=0;qri<=NQRANGES;qri++){
      qi=QOFFS[qri];
      for(ci=0;ci<64;ci++){
        base_matrices[pci][qi][ci]=(unsigned char)(
         fbase_matrices[pci][qi][ci]*bscale[qri][ci!=0]+0.5);
      }
    }
  }
  /*Interpolate the remaining base matrices.*/
  for(pci=0;pci<9;pci++){
    for(qri=0;qri<NQRANGES;qri++){
      for(qi=QOFFS[qri]+1;qi<QOFFS[qri+1];qi++)for(ci=0;ci<64;ci++){
        base_matrices[pci][qi][ci]=(unsigned char)(
         (2*((QOFFS[qri+1]-qi)*base_matrices[pci][QOFFS[qri]][ci]+
         (qi-QOFFS[qri])*base_matrices[pci][QOFFS[qri+1]][ci])+QSIZES[qri])/
         (2*QSIZES[qri]));
      }
    }
  }
  /*Select scale factors for every matrix.*/
  for(qi=64;qi-->0;){
    /*In general, we want to slightly underestimate the scale factor, so we
       take the minimum scale factor over all color channels and coefficients.*/
    for(pci=0;pci<9;pci++){
      int scale;
      scale=(int)(fbase_matrices[pci][qi][0]*qscales[qi]*
       (100/(DCT_SCALE*DCT_SCALE))/base_matrices[pci][qi][0]);
      if(scale<1)scale=1;
      if(scale<dc_scale[qi])dc_scale[qi]=scale;
      for(ci=1;ci<64;ci++){
        scale=(int)(fbase_matrices[pci][qi][ci]*qscales[qi]*
         (100/(DCT_SCALE*DCT_SCALE))/base_matrices[pci][qi][ci]);
        if(scale<1)scale=1;
        if(scale<ac_scale[qi])ac_scale[qi]=scale;
      }
    }
    /*But we must keep the quantizer monotonic.*/
    if(qi<63){
      for(pci=0;pci<9;pci++){
        while(base_matrices[pci][qi][0]<base_matrices[pci][qi+1][0]||
         dc_scale[qi]<dc_scale[qi+1]){
          /*dc_scale[qi]++;*/
          if(base_matrices[pci][qi][0]<255&&dc_scale[qi]>=dc_scale[qi+1]){
            base_matrices[pci][qi][0]++;
          }
          else dc_scale[qi]++;
        }
        for(ci=1;ci<64;ci++){
          while(base_matrices[pci][qi][ci]<base_matrices[pci][qi+1][ci]||
           ac_scale[qi]<ac_scale[qi+1]){
            /*ac_scale[qi]++;*/
            if(base_matrices[pci][qi][ci]<255&&ac_scale[qi]>=ac_scale[qi+1]){
              base_matrices[pci][qi][ci]++;
            }
            else ac_scale[qi]++;
          }
        }
      }
    }
  }
  /*Print the quantization matrices and their integer, interpolated
     approximations.*/
#if 0
  for(pci=0;pci<9;pci++)for(ci=0;ci<64;ci++){
    fprintf(stderr,"[%i][%i]=",pci,ci);
    for(qi=0;qi<64;qi++){
      if(!(qi&3))fprintf(stderr,"\n  ");
      fprintf(stderr,"%3i-%5.2lf",
       (base_matrices[pci][qi][ci]*(ci==0?dc_scale[qi]:ac_scale[qi])/100)<<2,
       fbase_matrices[pci][qi][ci]*qscales[qi]);
      if(qi<63)fprintf(stderr,",");
    }
    fprintf(stderr,"\n\n");
  }
#endif
  /*Check to make sure that the coefficients always decrease as the qi rises.
    This is assumed by the current encoder.*/
#if 0
  /*First check to make sure it is not a flaw in the computed matrices
     themselves.*/
  for(pci=0;pci<9;pci++)for(qi=0;qi<63;qi++)for(ci=0;ci<64;ci++){
    if(fbase_matrices[pci][qi][ci]*qscales[qi]<
     fbase_matrices[pci][qi+1][ci]*qscales[qi+1]){
      fprintf(stderr,"Monotonicity failure: [%i][%i][%i]-[%i][%i][%i]: %lf\n",
       pci,qi,ci,pci,qi+1,ci,
       fbase_matrices[pci][qi+1][ci]*qscales[qi+1]-
       fbase_matrices[pci][qi][ci]*qscales[qi]);
    }
  }
  /*Then check the approximations.*/
  for(pci=0;pci<9;pci++)for(qi=0;qi<63;qi++){
    if(base_matrices[pci][qi][0]*dc_scale[qi]<
     base_matrices[pci][qi+1][0]*dc_scale[qi+1]){
      fprintf(stderr,"Monotonicity failure: [%i][%i][0]-[%i][%i][0]\n",
       pci,qi,pci,qi+1);
    }
    for(ci=1;ci<64;ci++){
      if(base_matrices[pci][qi][ci]*ac_scale[qi]<
       base_matrices[pci][qi+1][ci]*ac_scale[qi+1]){
        fprintf(stderr,"Monotonicity failure: [%i][%i][%i]-[%i][%i][%i]\n",
         pci,qi,ci,pci,qi+1,ci);
      }
    }
  }
#endif
  /*Dump the generated tables as C code.*/
  printf("static const int OC_DEF_QRANGE_SIZES[%i]={",NQRANGES);
  for(qri=0;qri<NQRANGES;qri++){
    printf("%i",QSIZES[qri]);
    if(qri+1<NQRANGES)printf(",");
  }
  printf("};\n");
  printf("\nstatic const th_quant_base OC_DEF_BASE_MATRICES[9][%i]={",
   NQRANGES+1);
  for(pci=0;pci<9;pci++){
    if(pci==0)chx=chy=pli=pfi=0;
    else{
       pli=(pci-1>>2)+1;
       pfi=pci-1&3;
       chx=(pli<<1)-1+(pfi&1);
       chy=(pli<<1)-1+(pfi>>1);
    }
    printf("\n  /*%s matrices, %s.*/",CHANNEL_NAMES[pli],FORMAT_NAMES[pfi]);
    printf("\n  {");
    for(qri=0;qri<=NQRANGES;qri++){
      qi=QOFFS[qri];
      printf("\n    /*qi=%i.*/",qi);
      printf("\n    {");
      for(ci=0;ci<64;ci++){
         if(!(ci&7))printf("\n      ");
         printf("%3i",base_matrices[pci][qi][ci]);
         if(ci<63)printf(",");
      }
      printf("\n    }");
      if(qri<NQRANGES)printf(",");
    }
    printf("\n  }");
    if(pci<8)printf(",");
  }
  printf("\n};\n");
  printf("\nconst th_quant_info OC_DEF_QUANT_INFO[4]={");
  for(pfi=0;pfi<4;pfi++){
    printf("\n  {");
    printf("\n    {");
    for(qi=0;qi<64;qi++){
      if(!(qi&7))printf("\n      ");
      printf("%4i",dc_scale[qi]);
      if(qi<63)printf(",");
    }
    printf("\n    },");
    printf("\n    {");
    for(qi=0;qi<64;qi++){
      if(!(qi&7))printf("\n      ");
      printf("%4i",ac_scale[qi]);
      if(qi<63)printf(",");
    }
    printf("\n    },");
    printf("\n    {");
    /*There's really no obvious guidance on how to set the loop filter limits
       from the VP3 source code.
      Setting them to half the luma DC quantizer value is at least the obvious
       choice, and that's what we do here.
      This doesn't quite match what VP3 did: their values were larger for
       larger quantizers, and smaller for smaller quantizers, but it matches
       pretty closely for all intermediate quantizers.
      This also doesn't take into account the minimum quantizer limits, because
       those change depending on whether blocks are coded in intra or inter
       mode, while the loop filter must span the boundaries of all different
       block types.
      Perhaps using the smaller of the limits is still appropriate, though.*/
    for(qi=0;qi<64;qi++){
      if(!(qi&7))printf("\n      ");
      printf("%4i",(int)(qscales[qi]*fbase_matrices[0][qi][0]*0.5+0.5));
      if(qi<63)printf(",");
    }
    printf("\n    },");
    printf("\n    {");
    for(qti=0;qti<2;qti++){
       printf("\n      {");
       for(pli=0;pli<3;pli++){
         printf("\n        {%i,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[%i]}",
          NQRANGES,pli==0?0:(pli-1<<2)+1+(~pfi&3));
         if(pli<2)printf(",");
       }
       printf("\n      }");
       if(qti<1)printf(",");
    }
    printf("\n    }");
    printf("\n  }");
    if(pfi<3)printf(",");
  }
  printf("\n};\n");
  return 0;
}
