#include <stdlib.h>
#include <float.h>
#include <string.h>
#include "dct.h"
#include "fdct.h"
#include "idct.h"

/*This is informational only, for the moment.*/
#if 0
/*The matrix representation of the inverse of Theora's iDCT.
  This is computed using the 16-bit approximations of the sines and cosines
   that Theora uses, but using floating point values and assuming that no
   truncation occurs after division (i.e., pretending that the transform is
   linear like the real DCT).*/
static float  OC_FDCT_MATRIX[8][8]={
  {
    +0.7071060184286057F, +0.7071060184286053F,
    +0.7071060184286053F, +0.707106018428605F,
    +0.7071060184286054F, +0.7071060184286055F,
    +0.7071060184286053F, +0.7071060184286052F
  },
  {
    +0.9807842388225154F ,+0.8314625429486557F,
    +0.5555743331539822F ,+0.19508263443138055F,
    -0.19508263443138069F,-0.5555743331539821F,
    -0.83146254294865560F,-0.9807842388225153F
  },
  {
    +0.9238785663979289F, +0.3826923620536123F,
    -0.38269236205361223F,-0.923878566397929F,
    -0.923878566397929F,  -0.38269236205361223F,
    +0.38269236205361207F,+0.9238785663979292F
  },
  {
    +0.8314686329814659F, -0.1950868839940573F,
    -0.980786065037542F,  -0.5555738181875018F,
    +0.555573818187502F,  +0.9807860650375418F,
    +0.19508688399405724F,-0.8314686329814658F
  },
  {
    +0.7071060184286054F, -0.7071060184286054F,
    -0.7071060184286053F, +0.7071060184286054F,
    +0.7071060184286052F, -0.7071060184286054F,
    -0.7071060184286052F, +0.7071060184286051F
  },
  {
    +0.5555738181875021F, -0.980786065037542F,
    +0.19508688399405713F,+0.8314686329814659F,
    -0.8314686329814658F, -0.19508688399405733F,
    +0.9807860650375423F, -0.5555738181875022F
  },
  {
    +0.3826923620536124F, -0.9238785663979291F,
    +0.9238785663979291F, -0.3826923620536123F,
    -0.3826923620536122F, +0.9238785663979289F,
    -0.9238785663979289F, +0.38269236205361207F
  },
  {
    +0.19508263443138085F,-0.5555743331539823F,
    +0.8314625429486557F, -0.9807842388225151F,
    +0.9807842388225151F, -0.8314625429486556F,
    +0.5555743331539822F, -0.19508263443138077F
  }
};
#endif

/*The matrix representation of the Theora's actual iDCT.
  This is computed using the 16-bit approximations of the sines and cosines
   that Theora uses, but using floating point values and assuming that no
   truncation occurs after division (i.e., pretending that the transform is
   linear like the real DCT).*/
static float OC_IDCT_MATRIX[8][8]={
  {
    +0.7071075439453125F,+0.9807891845703125F,
    +0.9238739013671875F,+0.8314666748046875F,
    +0.7071075439453125F,+0.555572509765625F,
    +0.3826904296875F,   +0.1950836181640625F
  },
  {
    +0.7071075439453125F,+0.8314685295335948F,
    +0.3826904296875F,   -0.1950868454296142F,
    -0.7071075439453125F,-0.9807858711574227F,
    -0.9238739013671875F,-0.5555783333256841F
  },
  {
    +0.7071075439453125F,+0.5555783333256841F,
    -0.3826904296875F,   -0.9807858711574227F,
    -0.7071075439453125F,+0.1950868454296142F,
     0.9238739013671875F,+0.8314685295335948F
  },
  {
    +0.7071075439453125F,+0.1950836181640625F,
    -0.9238739013671875F,-0.555572509765625F,
    +0.7071075439453125F,+0.8314666748046875F,
    -0.3826904296875F,   -0.9807891845703125F
  },
  {
    +0.7071075439453125F,-0.1950836181640625F,
    -0.9238739013671875F,+0.555572509765625F,
    +0.7071075439453125F,-0.8314666748046875F,
    -0.3826904296875F,   +0.9807891845703125F
  },
  {
    +0.7071075439453125F,-0.5555783333256841F,
    -0.3826904296875F,    0.9807858711574227F,
    -0.7071075439453125F,-0.1950868454296142F,
    +0.9238739013671875F,-0.8314685295335948F
  },
  {
    +0.7071075439453125F,-0.8314685295335948F,
    +0.3826904296875,    +0.1950868454296142F,
    -0.7071075439453125F,+0.9807858711574227F,
    -0.9238739013671875F,+0.5555783333256841F
  },
  {
    +0.7071075439453125F,-0.9807891845703125F,
    +0.9238739013671875F,-0.8314666748046875F,
    +0.7071075439453125F,-0.555572509765625F,
    +0.3826904296875F,   -0.1950836181640625F
  }
};

/*Performs a forward 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 from the orthonormal version of the
   transform.
  TODO: What is the maximum dynamic range of the input? output?
  TODO: Write a new version of this transform.
  This one uses 16 multiplies, when we could get away with as little as 10,
   and even reduce that to 8 by folding multiplies into the quantization
   stage.
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      The first 8 entries are used (e.g., from a row of an 8x8 block).*/
static void fdct8(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  int t[8];
  int u;
  int v;
  t[0]=(int)_x[0]+_x[7];
  t[1]=(int)_x[1]+_x[2];
  t[2]=(int)_x[3]+_x[4];
  t[3]=(int)_x[5]+_x[6];
  t[4]=(int)_x[5]-_x[6];
  t[5]=(int)_x[3]-_x[4];
  t[6]=(int)_x[1]-_x[2];
  t[7]=(int)_x[0]-_x[7];
  /*Butterflies for ouputs 0 and 4.*/
  u=t[0]+t[2];
  v=t[1]+t[3];
  _y[0<<3]=(ogg_int16_t)OC_DIV2_16(OC_C4S4*(u+v));
  _y[4<<3]=(ogg_int16_t)OC_DIV2_16(OC_C4S4*(u-v));
  /*Apply rotation for outputs 2 and 6.*/
  u=t[0]-t[2];
  v=t[6]-t[4];
  _y[2<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C2S6*u)+OC_DIV2_16(OC_C6S2*v));
  _y[6<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C6S2*u)-OC_DIV2_16(OC_C2S6*v));
  /*Compute some common terms.*/
  t[3]=OC_DIV2_16(OC_C4S4*(t[1]-t[3]));
  t[4]=-OC_DIV2_16(OC_C4S4*(t[6]+t[4]));
  /*Apply rotation for outputs 1 and 7.*/
  u=t[7]+t[3];
  v=t[4]-t[5];
  _y[1<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C1S7*u)-OC_DIV2_16(OC_C7S1*v));
  _y[7<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C7S1*u)+OC_DIV2_16(OC_C1S7*v));
  /*Apply rotation for outputs 3 and 5.*/
  u=t[7]-t[3];
  v=t[4]+t[5];
  _y[3<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C3S5*u)-OC_DIV2_16(OC_C5S3*v));
  _y[5<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C5S3*u)+OC_DIV2_16(OC_C3S5*v));
}

/*Performs a forward 8x8 Type-II DCT transform.
  The output is scaled by a factor of 4 relative to the orthonormal version
   of the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      This may be the same as _x.
  _x: The input coefficients. */
void oc_fdct8x8(ogg_int16_t _y[64],const ogg_int16_t _x[64]){
  const ogg_int16_t *in;
  ogg_int16_t       *end;
  ogg_int16_t       *out;
  ogg_int16_t        w[64];
  /*Transform rows of x into columns of w.*/
  for(in=_x,out=w,end=out+8;out<end;in+=8,out++)fdct8(out,in);
  /*Transform rows of w into columns of y.*/
  for(in=w,out=_y,end=out+8;out<end;in+=8,out++)fdct8(out,in);
}


/*A general low-pass extension technique:

  The idea behind low-pass signal extension is to extend a signal beyond its
   boundary in such a way that the signal energy is concentrated in the
   low-frequency coefficients (i.e., is easy to compress).

  There are several different ways to go about this.
  The obvious one is to select the coefficient at the end of the zig-zag run,
   and select padding values that force these coefficients to zero.
  This can be done in an interative manner, by filling in the padding values
   with some initial guess (e.g., the average non-padding pixel value),
   performing a DCT, zeroing the desired coefficients, inverting the DCT, and
   updating the padding values with the result of the inverse (keeping the
   non-padding values fixed).
  I remember reading about this method somewhere, but I can no longer find the
   reference for it.
  In practice, however, this method does not actually work.

  The reason for this becomes clear when examining the non-iterative
   formulation of the problem as a system of linear equations.
  One wants to solve for the padding values that zero out the desired set of
   DCT coefficients.
  This can be done by extracting the appropriate submatrix from the general
   2D DCT transformation matrix and inverting it, then multiplying this inverse
   by the actual values of these coefficients computed with some arbitrary
   initial padding value (e.g., 0).

  Unfortunately, the linear system derived in this manner is ill-conditioned.
  Due to the symmetry of the basis functions, the submatrices are often
   singular.
  One could instead use the pseudo-inverse to find a least-squares solution for
   the coefficient values, however the system is also fairly stiff, containing
   large order of magnitude differences in the eigenvalues.
  This causes very large values to appear in the pseudo-inverse, producing
   interpolated pixel values that are well outside the normal [-255,255] range,
   and creating generally large coefficient values in the non-zero
   coefficients.

  Another obvious choices of coefficients to zero is the m highest frequency
   values in each column, where m is the number of rows that have at least as
   many as 8-ci, the column index, free padding values.
  This does nothing to help the singularity or the eigenvalue problem.
  One can also attempt to extract a linearly independent set from all choices
   of coefficients to zero out, but determining when a set of vectors is
   approximately zero is both numerically challenging and still doesn't solve
   the eigenvalue problem.
  A lot of time was wasted to determine all of this.

  A better strategy is to select a linearly independent set of DCT basis
   functions to represent the pixels inside the boundary, and then extrapolate
   the remaining pixel values using the coefficients derived from these basis
   functions \cite{KA94}.
  The set can be constructed in an interative manner, by starting with only the
   DC coefficient, and adding the basis function which minimizes the
   approximation error at each step.
  Let H(i,j) be the coefficient of the ith pixel in the jth basis function in
   the 2D iDCT, c(j) the DCT coefficients in the current approxmation, A the
   set of non-padding pixels, f(i) the pixel values, and r(i)=f(i)-H(i,:) c(:)
   the reconstruction error in pixel i.
  Then the metric to minimize is

    E(j) = \frac{\[\sum_{i\in A} H(i,j)*r(i)\]^2}{\sum_{i\in A} H(i,j)^2}

  This formulation guarantees a linearly independent set of vectors is chosen.
  It avoids ill-conditioning by stopping the iteration when the residual energy
   or the change in residual energy falls below a threshold.
  Unfortunately, it now depends on the actual pixel values, unlike the
   fomulations above, which could all be reduced to finding a matrix to
   multiply the existing pixel values by which is fixed for a given shape.

  This could be avoided by choosing some sample pixel values f(n) to
   approximate.
  While this method works very well in some cases, it can still occaisionally
   select a set of basis vectors with small singular values, which means large
   coefficients in the inverse, and general instability.
  One possible choice is a single non-zero pixel value, since the DCT transform
   of this has no zeros, and each resulting coefficient has roughly the same
   order of magnitude.
  Another possible choice is the iDCT of K/((2*ci+1)*(2*ri+1)), where K is a
   large constant (e.g. 1024) and ci and ri are the row and column indices of
   the coefficient.
  This weights the importance of each DCT basis function roughly according to
   the inverse of its frequency, as one would expect in natural images.
  This is the forumla used to produce the OC_PIXEL_TEMPLATE table below.

  Initial test suggest this method works well.
  
  @INPROCEEDINGS{KA94,
    author="Andr\'{e} Kaup and Til Aach",
    title="Segment-Oriented Coding of Textured Images Based on Successive
     Approximation",
    booktitle="Proceedings of the International Symposium on Speech, Image
     Processing, and Neural Networks",
    pages="197--200",
    address="Hong Kong",
    month="Apr.",
    year=1994,
    URL="http://www.lnt.de/~kaup/paper/issipnn-94.pdf"
  }*/

/*Performs a singular value decomposition on the _m by _n matrix _a.
  This decomposes a matrix A into the product U \Sigma V^T, where U is an
   _m by _n unitary matrix, \Sigma is a diagonal matrix containing the singular
   values, and V is an _n by _n unitary matrix.

  The algorithm here is a one-sided Jacobi algorithm on the right.
  This algorithm should converge quadratically.
  ||U^T U - I|| and ||V^T V - I|| should both be zero to machine precision.
  The relative error for all singular values is uniform (i.e., the small values
   contain as many significant digits as the large ones, unlike in the
   Golub-Reinsch algorithm).
  The singular values are found in no particular order.

  For computational efficiency, a QR decomposition should be done for large
   matrices, and the SVD done on only the square, upper-triangular factor.
  This should probably be re-implemented to operate from the left, so that it
   works on rows instead of columns.

  _a:  The _m by _n matrix to decompose.
       This matrix is destructively modified.
       Returns the product U \Sigma.
       The matrices are returned pre-multiplied because that is how the
        algorithm computes them.
       U alone can be obtained by scaling the columns by 1/sqrt(s2[j]).
       However, this work doesn't always need to be done, and so it is not done
        here.
  _v:  Returns the _n by _n matrix V (not V transpose).
  _s2: Returns the diagonal of \Sigma^2.
       The singular values are returned in no particular order.
  _m:  The number of rows in _a.
  _n:  The number of columns in _a.*/
static void oc_svd(float **_a,float **_v,float *_s2,int _m,int _n){
  int slimit;
  int rot;
  int i;
  int j;
  int k;
  for(i=0;i<_n;i++){
    memset(_v[i],0,sizeof(_v[i][0])*_n);
    _v[i][i]=1;
  }
  if(_n==1)for(_s2[0]=0,i=0;i<_m;i++)_s2[0]+=_a[i][0]*_a[i][0];
  slimit=OC_MAXI(_n>>2,6);
  while(slimit-->0){
    rot=0;
    for(j=0;j<_n-1;j++){
      for(k=j+1;k<_n;k++){
        float h_jj;
        float h_kj;
        float h_kk;
        h_jj=h_kj=h_kk=0;
        for(i=0;i<_m;i++){
          h_jj+=_a[i][j]*_a[i][j];
          h_kj+=_a[i][j]*_a[i][k];
          h_kk+=_a[i][k]*_a[i][k];
        }
        _s2[j]=h_jj;
        _s2[k]=h_kk;
        if(h_kj*h_kj>FLT_EPSILON*h_jj*h_kk){
          float tau;
          float t;
          float c;
          float s;
          rot++;
          tau=(h_jj-h_kk)/(2*h_kj);
          t=tau<0?-1.F:1.F;
          tau=OC_FABSF(tau);
          t=t/(tau+OC_SQRTF(1+tau*tau));
          c=1/OC_SQRTF(1+t*t);
          s=c*t;
          for(i=0;i<_m;i++){
            t=_a[i][j];
            _a[i][j]=t*c+_a[i][k]*s;
            _a[i][k]=_a[i][k]*c-t*s;
          }
          for(i=0;i<_n;i++){
            t=_v[i][j];
            _v[i][j]=t*c+_v[i][k]*s;
            _v[i][k]=_v[i][k]*c-t*s;
          }
        }
      }
    }
    if(!rot)break;
  }
}

/*Computes the linear system used to optimally pad fragments on the border of
   the displayable region.
  This is a znp by zap matrix, where nzp is the number of pixels outside the
   region, and nap is the number of pixels inside the region.
  The matrix should be multiplied on the right by a column vector containing
   the pixels in the region, which will produce a column vector containing the
   optimal padding values for the pixels outside the region.
  The matrix entries are all signed 8.8 fixed point values.*/
void oc_border_enc_info_init(oc_border_enc_info *_border_info,
 const oc_border_info *_border){
  static float OC_PIXEL_TEMPLATE[8][8]={
    {
      130.805944460235168F, 109.374906832872852F,
       72.8771953225751012F, 55.024184039572912F,
       40.0782795394676938F, 28.7965294955646911F,
       18.3678933440416187F,  9.07566425511869923F
    },
    {
      109.374906832872831F,  91.4550962807069467F,
       60.9371117003851558F, 46.0091093544504836F,
       33.5119180457712895F, 24.07855196248319F,
       15.3585269500609484F,  7.58872187687009613F
    },
    {
       72.877195322575119F,  60.9371117003851647F,
       40.6027846823072113F, 30.6561618760094933F,
       22.329203907689914F,  16.0436921526799336F,
       10.233484085311435F,   5.05641360055671107F
    },
    {
       55.0241840395729386F, 46.0091093544505103F,
       30.6561618760094978F, 23.1462021218859703F,
       16.8591315820476461F, 12.1134062003323617F,
        7.72654750479660545F, 3.81772420447479277F
    },
    {
       40.0782795394677027F, 33.5119180457712939F,
       22.3292039076899095F, 16.8591315820476461F,
       12.2797820655009926F,  8.8231109346916412F,
       5.62782958397902355F,  2.78073760733081964F
    },
    {
      28.7965294955646946F,  24.0785519624831945F,
      16.0436921526799425F,  12.1134062003323572F,
       8.82311093469164653F,  6.33946809077178929F,
       4.04363566683221975F,  1.99797978977795906F
    },
    {
      18.3678933440416223F,  15.3585269500609511F,
      10.2334840853114394F,   7.72654750479660457F,
       5.62782958397902355F,  4.04363566683221886F,
       2.57923680219786533F,  1.27441328260909569F
    },
    {
       9.075664255118701F,    7.58872187687010324F,
       5.05641360055670929F,  3.81772420447479188F,
       2.78073760733081876F,  1.99797978977795924F,
       1.27441328260909636F, 0.629693719284134534F
    }
  };
  ogg_int16_t **x;
  float         p[64];
  float         c[64];
  float         b[64];
  float         r[64];
  float         c_norm[64];
  float         d_norm[64];
  float         h[64][64];
  float         best_de;
  float         de;
  float         w;
  int           ap[64];
  int           zp[64];
  int           ac[64];
  int           best_aci;
  int           nap;
  int           nzp;
  int           nac;
  int           api;
  int           zpi;
  int           aci;
  int           acj;
  int           ack;
  int           cri;
  int           cci;
  int           crj;
  int           ccj;
  int           pri;
  int           pci;
  int           prj;
  int           pcj;
  int           pi;
  int           ci;
  for(zpi=api=pi=0;pi<64;pi++){
    if((int)(_border->mask>>pi)&1)ap[api++]=pi;
    else zp[zpi++]=pi;
  }
  nap=api;
  nzp=zpi;
  for(api=0;api<nap;api++){
    _border_info->pi[api]=(unsigned char)ap[api];
    p[api]=OC_PIXEL_TEMPLATE[ap[api]>>3][ap[api]&7];
  }
  for(zpi=0;zpi<nzp;zpi++)_border_info->pi[nap+zpi]=(unsigned char)zp[zpi];
  for(ci=0;ci<64;ci++){
    ac[OC_IZIG_ZAG[ci]]=ci;
    c_norm[ci]=0;
    cri=ci>>3;
    cci=ci&7;
    for(api=0;api<nap;api++){
      pri=ap[api]>>3;
      pci=ap[api]&7;
      c_norm[ci]+=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri]*
       OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
    }
    d_norm[ci]=c_norm[ci];
    for(zpi=0;zpi<nzp;zpi++){
      pri=zp[zpi]>>3;
      pci=zp[zpi]&7;
      d_norm[ci]+=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri]*
       OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
    }
    d_norm[ci]/=c_norm[ci];
    c_norm[ci]=1/c_norm[ci];
  }
  /*Always include the DC coefficient.*/
  for(nac=1;;nac++){
    /*Solve for the coefficients c[]:*/
    /*Step 1: Update the system Hc=b*/
    aci=nac-1;
    cri=ac[aci]>>3;
    cci=ac[aci]&7;
    for(acj=0;acj<=aci;acj++){
      crj=ac[acj]>>3;
      ccj=ac[acj]&7;
      h[aci][acj]=0;
      for(api=0;api<nap;api++){
        pri=ap[api]>>3;
        pci=ap[api]&7;
        h[aci][acj]+=OC_IDCT_MATRIX[pci][ccj]*OC_IDCT_MATRIX[pri][crj]*
         OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
      }
    }
    b[aci]=0;
    for(api=0;api<nap;api++){
      pri=ap[api]>>3;
      pci=ap[api]&7;
      b[aci]+=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri]*p[api];
    }
    /*Step 2: Update the Cholesky factorization of H.*/
    for(acj=0;acj<aci;acj++){
      for(ack=0;ack<acj;ack++){
        h[aci][acj]-=h[aci][ack]*h[acj][ack];
      }
      h[aci][acj]/=h[acj][acj];
    }
    for(ack=0;ack<aci;ack++)h[aci][aci]-=h[aci][ack]*h[aci][ack];
    h[aci][aci]=OC_SQRTF(h[aci][aci]);
    if(nac>=nap)break;
    /*Step 3: Update the forward substitution.*/
    for(acj=0;acj<aci;acj++)b[aci]-=h[aci][acj]*b[acj];
    b[aci]/=h[aci][aci];
    /*Step 4: Do backward substitution.*/
    for(aci=nac;aci-->0;){
      c[aci]=b[aci];
      for(acj=aci+1;acj<nac;acj++)c[aci]-=h[acj][aci]*c[acj];
      c[aci]/=h[aci][aci];
    }
    /*Find the next basis function to add:*/
    /*Step 1: Compute the residuals.*/
    de=0;
    for(api=0;api<nap;api++){
      pri=ap[api]>>3;
      pci=ap[api]&7;
      r[api]=p[api];
      for(aci=0;aci<nac;aci++){
        cci=ac[aci]&7;
        cri=ac[aci]>>3;
        r[api]-=c[aci]*OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
      }
      de+=r[api]*r[api];
    }
    /*Step 2: Find the greatest reduction in residual.*/
    best_aci=nac;
    best_de=0;
    for(aci=nac;aci<64;aci++){
      cci=ac[aci]&7;
      cri=ac[aci]>>3;
      de=0;
      for(api=0;api<nap;api++){
        pri=ap[api]>>3;
        pci=ap[api]&7;
        de+=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri]*r[api];
      }
      de*=de*c_norm[ac[aci]];
      cri=ac[aci]>>3;
      cci=ac[aci]&7;
      ci=ac[nac];
      ac[nac]=ac[aci];
      ac[aci]=ci;
      for(acj=0;acj<=nac;acj++){
        crj=ac[acj]>>3;
        ccj=ac[acj]&7;
        h[nac][acj]=0;
        for(api=0;api<nap;api++){
          pri=ap[api]>>3;
          pci=ap[api]&7;
          h[nac][acj]+=OC_IDCT_MATRIX[pci][ccj]*OC_IDCT_MATRIX[pri][crj]*
           OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
        }
      }
      for(acj=0;acj<nac;acj++){
        for(ack=0;ack<acj;ack++){
          h[nac][acj]-=h[nac][ack]*h[acj][ack];
        }
        h[nac][acj]/=h[acj][acj];
      }
      for(ack=0;ack<nac;ack++)h[nac][nac]-=h[nac][ack]*h[nac][ack];
      de=h[nac][nac]=OC_SQRTF(h[nac][nac]);
      if(de>best_de){
        best_de=de;
        best_aci=aci;
      }
      ci=ac[nac];
      ac[nac]=ac[aci];
      ac[aci]=ci;
    }
    if(best_aci!=nac){
      ci=ac[nac];
      ac[nac]=ac[best_aci];
      ac[best_aci]=ci;
    }
  }
  /*Finally, compute the linear extrapolation of the padding pixels.*/
  x=(ogg_int16_t **)oc_malloc_2d(nzp,nap,sizeof(x[0][0]));
  for(api=0;api<nap;api++){
    pri=ap[api]>>3;
    pci=ap[api]&7;
    for(aci=0;aci<nac;aci++){
      cri=ac[aci]>>3;
      cci=ac[aci]&7;
      b[aci]=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
    }
    for(aci=0;aci<nac;aci++){
      for(acj=0;acj<aci;acj++)b[aci]-=h[aci][acj]*b[acj];
      b[aci]/=h[aci][aci];
    }
    for(aci=nac;aci-->0;){
      for(acj=aci+1;acj<nac;acj++)b[aci]-=h[acj][aci]*b[acj];
      b[aci]/=h[aci][aci];
    }
    for(zpi=0;zpi<nzp;zpi++){
      prj=zp[zpi]>>3;
      pcj=zp[zpi]&7;
      w=0;
      for(aci=0;aci<nac;aci++){
        cri=ac[aci]>>3;
        cci=ac[aci]&7;
        w+=OC_IDCT_MATRIX[pcj][cci]*OC_IDCT_MATRIX[prj][cri]*b[aci];
      }
      x[zpi][api]=(ogg_int16_t)OC_CLAMPI(-8192,
       (int)(256*w+(w<0?-0.5F:0.5F)),8191);
    }
  }
  {
    int scale;
    scale=0;
  for(aci=0;aci<nac;aci++){
    ci=ac[aci];
    while(d_norm[ci]>=(1<<scale))scale++;
  }
  _border_info->scale=scale;
  for(aci=0;aci<nac;aci++){
    ci=ac[aci];
    w=d_norm[ci]/(1<<scale);
    _border_info->cs[ci]=(ogg_int16_t)
     OC_CLAMPI(-32768,(int)(32768*w+(w<0?-0.5F:0.5F)),32767);
  }
  }
  for(;aci<64;aci++)_border_info->cs[ac[aci]]=0;
  _border_info->fill_matrix=x;
}

/*Performs a forward 8x8 Type-II DCT transform on blocks which overlap the
   border of the displayable region.
  This uses an iterative method to compute optimal padding values so as to
   make as many of the final coefficients (in zig-zag order) as close to 0 as
   possible.
  _border:      A description of which pixels are inside the border.
  _border_info: The matrix used to compute pixel values outside the border.
  _y:           The buffer to store the result in.
                Unlike above, this may NOT be the same as _x.
  _x:           The input coefficients.
                Coefficients outside the border should be initialized to some
                 value, but it does not really matter what.*/
void oc_fdct8x8_border(const oc_border_info *_border,
 const oc_border_enc_info *_border_info,ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t x[64];
  ogg_int16_t y[64];
  int         nap;
  int         nzp;
  int         api;
  int         zpi;
  int         ci;
  nap=_border->npixels;
  nzp=64-nap;
  for(api=0;api<64;api++)x[api]=(ogg_int16_t)(_x[api]<<_border_info->scale);
  oc_fdct8x8(y,x);
  for(ci=0;ci<64;ci++)y[ci]=(ogg_int16_t)(OC_DIV_ROUND_POW2(
   (ogg_int32_t)y[ci]*_border_info->cs[ci],15,16384));
  oc_idct8x8(x,y);
  for(api=0;api<nap;api++)x[_border_info->pi[api]]=_x[_border_info->pi[api]];
  oc_fdct8x8(_y,x);
  /*for(ci=0;ci<64;ci++)if(!_border_info->cs[ci])y[ci]=0;
  oc_idct8x8(x,y);
  for(api=0;api<nap;api++)x[_border_info->pi[api]]=_x[_border_info->pi[api]];
  oc_fdct8x8(y,x);
  for(zpi=0;zpi<nzp;zpi++){
    ogg_int32_t v;
    for(v=api=0;api<nap;api++){
      v+=_border_info->fill_matrix[zpi][api]*_x[_border_info->pi[api]];
    }
    _x[_border_info->pi[nap+zpi]]=(ogg_int16_t)(OC_DIV_ROUND_POW2(v,8,128));
  }
  oc_fdct8x8(_y,_x);*/
}
