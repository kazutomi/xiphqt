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
  int     ips;
  int     row;
  int     f;
  int     p;
  int     c;
  int     i;
  int     j;
  int     k;
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
  See doc/theory/psych.nb for more details.

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

/*The main contrast-dependent CSF function.*/
static double csf(csf_ctx *_ctx,double _f){
  double f;
  f=_f*_ctx->fmax;
  return _ctx->scale/(pow(_ctx->contrast,1.0/CONTRAST_EXP)+
   1.0/_ctx->c0csf(_ctx->c0csf_ctx,f));
}

/*This is the Nyquist frequency for a 90 dpi screen viewed from 50 cm out.
  This is assumed to be the closest reasonable viewing distance.*/
#define FREQUENCY_MAX (15.4609862434702964)

/*Builds the CSF filter banks for each channel and contrast level.*/
static void build_csf_tables(csf_filter_bank _filters[5][64],
 double _qscale[64]){
  csf_ctx   csf_ctxs[5]={
    {
      (csf_func)csfl,
      &Y_CTX,
      FREQUENCY_MAX,
      3.74695975597140362
    },
    {
      (csf_func)csfc,
      &CR_CTX,
      FREQUENCY_MAX,
      0
    },
    {
      (csf_func)csfc,
      &CR_CTX,
      0.5*FREQUENCY_MAX,
      0
    },
    {
      (csf_func)csfc,
      &CB_CTX,
      FREQUENCY_MAX,
      0
    },
    {
      (csf_func)csfc,
      &CB_CTX,
      0.5*FREQUENCY_MAX,
      0
    }
  };
  int chi;
  int qi;
  for(chi=0;chi<5;chi++){
    for(qi=0;qi<64;qi++){
      csf_ctxs[chi].contrast=_qscale[64];
      /*Find the normalization constant for this contrast level.*/
      csf_ctxs[chi].scale=pow(_qscale[qi],1.0/CONTRAST_EXP)+
       1/csf_ctxs[chi].c0csf(csf_ctxs[chi].c0csf_ctx,csf_ctxs[chi].peakf);
      find_csf_filters(_filters[chi][qi],(csf_func)csf,csf_ctxs+chi);
    }
  }
}

int main(void){
  /*The names of each channel: these get inserted as comments in the generated
     code.*/
  static const char *CHANNEL_NAMES[5]={
    "Y channel filters.",
    "Cr channel filters at full resolution.",
    "Cr channel filters at half resolution.",
    "Cb channel filters at full resolution.",
    "Cb channel filters at half resolution."
  };
  int i;
  int j;
  int k;
  csf_filter_bank filters[5][8];
  /*Compute all the CSF filters.*/
  build_csf_tables(filters);
  /*Dump the generated tables as C code for inclusion in lib/psych.c.*/
  printf("static const oc_csf_filter_bank OC_CSF_FILTERS[5][8]={");
  for(i=0;i<5;i++){
    printf("\n  /*%s*/",CHANNEL_NAMES[i]);
    printf("\n  {");
    for(j=0;j<8;j++){
      printf("\n    /*Contrast level %g filters.*/",CONTRASTS[j]);
      printf("\n    {");
      for(k=0;k<8;k++){
        printf("\n      {");
        printf("\n        %+28.25fF,%+28.25fF,",filters[i][j][k][0],
         filters[i][j][k][1]);
        printf("\n        %+28.25fF,%+28.25fF",filters[i][j][k][2],
         filters[i][j][k][3]);
        printf("\n      }");
        if(k+1<8)printf(",");
      }
      printf("\n    }");
      if(j+1<8)printf(",");
    }
    printf("\n  }");
    if(i+1<5)printf(",");
  }
  printf("\n};");
  return 0;
}
