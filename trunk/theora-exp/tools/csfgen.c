#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if !defined(M_PI)
# define M_PI (3.1415926535897932384626433832795)
#endif

/*Nadenau computes the transfer function of the HVS with respect to a
   particular wavelet coefficient by mapping a small region of the frequency
   domain to that region.
  This, of course, assumes that the wavelet function has a uniform impulse
   response to frequencies within that range, and 0 response to any frequencies
   outside of it, which is of course hogwash.
  He then uses Fourier series of this transfer function to develop a set of FIR
   filters which approximate it.

  We, of course, are not using wavelet functions, but there is a nice
   partitioning of the frequency domain that is normally assumed with the DCT
   that would allow us to do the same thing.
  However, in the future, we may wish to use transforms other than the DCT, in
   which case Nadenau's idealized frequency responses would no longer be a
   reasonable approximation.
  The process we actually want to approximate is:

    Image -> FFT -> Scale by CSF -> iFFT -> Block transform (DCT)

  We would like to do this by manipulating the DCT coefficients directly,
   without the transformation into the frequency domain and back.

  Therefore we posit that a set of symmetric FIR filters exist which
   approximate the above process, and proceed to set up a numerical
   optimization problem to solve for their coefficients.
  This is done by applying the above process to a set of simple cosine curves
   with varying frequencies and phases, and evaluating what the proper output
   of the filters should be.
  A least-squares problem is then set up to find the filter coefficients which
   yield the closest results.
  There is, of course, no need to translate a cosine curve into the frequency
   domain and back, so the process is relatively straightforward, and would
   work for any block transform.*/

/*The number of phases to sample at each frequency.*/
#define NPHASES   (16)
/*The number of frequencies to sample (in the interval [0,pi)).*/
#define NFREQS    (1024)
/*The maximum half-length of a filter, rounded up.
  The actual filter length is 2*NTAPS-1, but there are only NTAPS unique filter
   coefficients, since the filters are symmetric.
  The value 4 here is sufficient to guarantee that any chopped-off coefficients
   account for less than 1/255th of the magnitude of coefficient 0.*/
#define NTAPS     (4)
/*The actual maximum size of the filters.*/
#define FILT_SIZE ((NTAPS<<1)-1)

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

/*Solves a linear system Ax=b for x by Gaussian elimination with partial
   pivoting.
  _x:   Will contain the solution vector.
  _a:   The _n by _n matrix of coefficients on the left-hand side.
  _b:   The right hand side.
  _ips: Scratch space in which to store pivot points.
  _n:   The size of the system.
  Return: 0 if the system has a solution, or a negative value on error.*/
static int lin_solve(double _x[],double **_a,double _b[],int _ips[],int _n){
  double sum;
  int    nm1;
  int    ip;
  int    i;
  int    j;
  int    k;
  if(_n==0)return 0;
  /*Initialize pivot list.*/
  for(i=0;i<_n;i++){
    double row_norm;
    _ips[i]=i;
    row_norm=0;
    for(j=0;j<_n;j++){
      double q;
      q=fabs(_a[i][j]);
      if(row_norm<q)row_norm=q;
    }
    if(row_norm==0)return -1;
    _x[i]=1/row_norm;
  }
  /*Gaussian elimination.*/
  nm1=_n-1;
  for(k=0;k<nm1;k++){
    double big;
    double pivot;
    int    pivi;
    int    kp;
    int    kp1;
    big=0;
    for(i=k;i<_n;i++){
      double size;
      ip=_ips[i];
      size=fabs(_a[ip][k])*_x[ip];
      if(size>big){
        big=size;
        pivi=i;
      }
    }
    if(big==0)return -2;
    if(pivi!=k){
      j=_ips[k];
      _ips[k]=_ips[pivi];
      _ips[pivi]=j;
    }
    kp=_ips[k];
    pivot=_a[kp][k];
    kp1=k+1;
    for(i=kp1;i<_n;i++){
      double em;
      ip=_ips[i];
      em=-_a[ip][k]/pivot;
      _a[ip][k]=-em;
      for(j=kp1;j<_n;j++)_a[ip][j]+=em*_a[kp][j];
    }
  }
  /*Last element of _ips[n]th row.*/
  if(_a[_ips[nm1]][nm1]==0)return -3;
  /*Back substitution.*/
  ip=_ips[0];
  _x[0]=_b[ip];
  for(i=1;i<_n;i++){
    double sum;
    ip=_ips[i];
    sum=0;
    for(j=0;j<i;j++)sum+=_a[ip][j]*_x[j];
    _x[i]=_b[ip]-sum;
  }
  _x[nm1]=_x[nm1]/_a[_ips[nm1]][nm1];
  for(i=nm1;i-->0;){
    ip=_ips[i];
    sum=0;
    for(j=i+1;j<_n;j++)sum+=_a[ip][j]*_x[j];
    _x[i]=(_x[i]-sum)/_a[ip][i];
  }
  return 0;
}

/*A Contrast Sensitivity Function.*/
typedef double (*csf_func)(const void *_ctx,double _f);

/*A single CSF filter.*/
typedef double     csf_filter[NTAPS];
/*A set of CSF filters, one for each DCT coefficient.*/
typedef csf_filter csf_filter_bank[8];

/*Finds a filter bank for a given CSF function.
  _filters: Will contain the optimal filter bank.
  _csf:     The CSF function.
  _csf_ctx: The parameters of the CSF function.
  _scratch: An NFREQS*NPHASES*8 by NTAPS array of scratch space.*/
static void find_csf_filters(csf_filter_bank _filters,csf_func _csf,
 const void *_csf_ctx,double **_scratch){
  double  ata[NTAPS][NTAPS];
  double  ata_scratch[NTAPS][NTAPS];
  double  atb[NTAPS];
  double *atap[NTAPS];
  double *ata_scratchp[NTAPS];
  double  x[8*FILT_SIZE];
  double  y[8*FILT_SIZE];
  int     aoff[8];
  int     ips[NTAPS];
  int     row;
  int     f;
  int     p;
  int     c;
  int     i;
  int     j;
  int     k;
  /*Set up offsets into _scratch, and the local 2D arrays.*/
  aoff[0]=0;
  for(i=1;i<8;i++)aoff[i]=aoff[i-1]+NFREQS*NPHASES;
  for(i=0;i<NTAPS;i++){
    atap[i]=ata[i];
    ata_scratchp[i]=ata_scratch[i];
  }
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
      theta=p*(2*M_PI)/NPHASES-8*(NTAPS-1)*dtheta;
      /*Fill in a perfect cosine curve at this frequency/phase.*/
      for(i=0;i<8*FILT_SIZE;i++){
        x[i]=cos(theta);
        theta+=dtheta;
      }
      /*Find the transformed coefficient values of this curve.*/
      for(i=0;i<FILT_SIZE;i++){
        fdct8d(y+(i<<3),x+(i<<3));
      }
      /*Add the equation for each coefficient to our linear systems.*/
      for(c=0;c<8;c++){
        _scratch[aoff[c]+row][0]=y[(NTAPS-1<<3)+c];
        for(j=1;j<NTAPS;j++){
          _scratch[aoff[c]+row][j]=y[(NTAPS-1-j<<3)+c]+y[(NTAPS-1+j<<3)+c];
        }
        _scratch[aoff[c]+row][NTAPS]=y[(NTAPS-1<<3)+c]*cs;
      }
    }
  }
  /*We now have the system Ax=b, where x is the filter coefficients we want to
     solve for, and each row of A is a set of actual coefficients, and b is the
     CSF-scaled coefficient we actually want as output.
    There is a row of A for each frequency and phase, but we only have NTAPS
     filter coefficients, so we find the solution which minimizes (Ax-b)^2.
    This is done by solving (A^T A)x = A^T b, which conveniently is a 5x5
     system, and thus easy to solve.*/
  for(c=0;c<8;c++){
    for(i=0;i<NTAPS;i++){
      for(j=0;j<NTAPS;j++)ata[i][j]=0;
      atb[i]=0;
    }
    for(k=0;k<NFREQS*NPHASES;k++){
      for(i=0;i<NTAPS;i++){
        for(j=0;j<NTAPS;j++){
          ata[i][j]+=_scratch[aoff[c]+k][i]*_scratch[aoff[c]+k][j];
        }
        atb[i]+=_scratch[aoff[c]+k][i]*_scratch[aoff[c]+k][NTAPS];
      }
    }
    /*We're paranoid: On the off chance the system has no solution, we reduce
       the dimensionality of the filter and pad with 0's.
      That, of course, never actually happens.*/
    for(k=NTAPS;k>0;k--){
      memcpy(ata_scratch,ata,sizeof(ata_scratch));
      if(!lin_solve(_filters[c],ata_scratchp,atb,ips,k)){
        for(;k<NTAPS;k++)_filters[c][k]=0;
        break;
      }
    }
  }
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

/*The various contrast levels for which we will compute filters.*/
static const double CONTRASTS[8]={0,1,2,4,8,16,32,64};

/*Builds the CSF filter banks for each channel and contrast level.*/
static void build_csf_tables(csf_filter_bank _filters[5][8]){
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
  double  **scratch;
  int       chi;
  int       ci;
  /*Allocate the scratch space needed for our least-squares problem.*/
  scratch=(double **)alloc_2d(NFREQS*NPHASES*8,NTAPS+1,sizeof(double));
  for(chi=0;chi<5;chi++){
    for(ci=0;ci<8;ci++){
      csf_ctxs[chi].contrast=pow(CONTRASTS[ci]*CONTRAST_SCALE,1.0/CONTRAST_EXP);
      /*Find the normalization constant for this contrast level.*/
      csf_ctxs[chi].scale=csf_ctxs[chi].contrast+
       1/csf_ctxs[chi].c0csf(csf_ctxs[chi].c0csf_ctx,csf_ctxs[chi].peakf);
      find_csf_filters(_filters[chi][ci],(csf_func)csf,csf_ctxs+chi,scratch);
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
