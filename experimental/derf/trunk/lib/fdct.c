#include <stdlib.h>
#include <string.h>
#include "dct.h"
#include "fdct.h"
#include "idct.h"

/*The matrix representation of the Theora's actual iDCT.
  This is computed using the 16-bit approximations of the sines and cosines
   that Theora uses, but using floating point values and assuming that no
   truncation occurs after division (i.e., pretending that the transform is
   linear like the real DCT).*/
static const float OC_IDCT_MATRIX[8][8]={
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
  The obvious one is to select the coefficients at the end of the zig-zag run,
   and select padding values that force these coefficients to zero.
  This can be done in an iterative manner, by filling in the padding values
   with some initial guess (e.g., the average non-padding pixel value),
   performing a DCT, zeroing the desired coefficients, inverting the DCT, and
   updating the padding values with the result of the inverse (keeping the
   non-padding values fixed).
  I remember reading about this method somewhere, but I can no longer find the
   reference for it.
  In practice, however, this method does not actually work when implemented as
   just described.

  The reason for this becomes clear when examining the non-iterative
   formulation of the problem as a system of linear equations.
  One wants to solve for the padding values that zero out the desired set of
   DCT coefficients.
  This can be done by extracting the appropriate submatrix from the general
   2D DCT transformation matrix and inverting it, then multiplying this
   inverse by the actual values of these coefficients computed with some
   arbitrary initial padding value (e.g., 0).

  Unfortunately, the linear system derived in this manner is ill-conditioned.
  Due to the symmetry of the basis functions, the submatrices are often
   singular.
  One could instead use the pseudoinverse to find a least-squares solution
   for the coefficient values, however the system is also fairly stiff,
   containing large order of magnitude differences in the singular values.
  This causes very large values to appear in the pseudoinverse, producing
   interpolated pixel values that are well outside the normal [-255,255]
   range and creating generally large coefficient values in the non-zero
   coefficients.

  Another obvious choices of coefficients to zero is the m highest frequency
   values in each column, where m is the number of rows that have at least as
   many as 8-ci padding values, where ci is the column index.
  This selects the highest frequency coefficients with some non-zero support
   in the padding region, and ensures that every pixel in the padding region
   is covered by at least one such coefficient's basis function.
  However, these conditions alone are not enough to guarantee that the
   corresponding submatrix of the DCT transform is non-singular, nor does it
   even help reduce the stiffness of the least squares problem.

  A better strategy is to select a linearly independent set of DCT basis
   functions to represent the pixels inside the boundary, and then extrapolate
   the remaining pixel values using the coefficients derived from these basis
   functions.
  This is the approach taken by Kaup and Aach  \cite{KA94}.
  The coefficients corresponding to the basis functions that are not selected
   are implicitly zeroed out by this process.
  One can attempt to select the linearly independent set using Gaussian
   elimination.
  However, experiments show straightforward methods of determining when a
   pivot value is small enough to consider zero do not work reliably, and this
   still doesn't solve the stiffness problem.

  Kaup and Aach take the approach of constructing the set in an iterative
   manner by starting with only the DC coefficient and adding the basis
   function which minimizes the approximation error at each step.
  Let G(i,j) be the coefficient of the ith pixel in the jth basis function in
   the 2D iDCT, c(j) the DCT coefficients in the current approxmation, A the
   set of non-padding pixels, f(i) the pixel values, and
   r(i)=f(i)-G(i,:) c(:) the reconstruction error in pixel i.
  Then the metric to minimize is

    E(j) = \frac{\[\sum_{i\in A} r(i)*G(i,j)\]^2}{\sum_{i\in A} G(i,j)^2}

  This formulation guarantees a linearly independent set of vectors is chosen.
  It avoids ill-conditioning by stopping the iteration when the residual
   energy or the change in residual energy falls below a threshold.
  The coefficients which minimize the residual at each stage can be computed
   by formulating the least squares solution G^T G c = G^T p.
  Because G^T G is Hermitian, this can be solved by finding the Cholesky
   decomposition L such that L L^T = G^T G and L is lower-triangular.
  Then c = L^-T L^-1 G^T p, where the multiplication by L^-T and L^-1 can be
   done by forward and backward substitution.

  Unfortunately, this method now depends on the actual pixel values.
  While it has the advantage of actually working, the other methods above
   can all be reduced to finding a fixed matrix for a given shape to multiply
   the existing pixel values by.
  Thus, so long as the shape does not change, the processing can all be done
   in advance and run-time computation is reduced to a simple matrix
   multiplication.
  This dependency can be avoided by choosing some sample pixel values f(n)
   to drive approximation instead of using the actual pixel values.
  While that approach works very well in some cases, it can still
   occaisionally select a set of basis vectors with small singular values.
  There is a more robust method, however, that does not require choosing
   sample pixel values.

  One thing to note is that as each basis vector is added to G, a single row
   is added to the Cholesky decompositing L L^T of G^T G.
  This row only depends on the previous rows of G, and does not change as more
   rows are added to G.
  When computing c from p, division by a diagonal element of L is performed
   twice, once during forward substitution and once during backward
   substitution.
  These are the only divisions in this computation.
  Thus it is small values of these diagonal elements which lead to the
   numerical instability.
  So, instead of trying to minimize the residual energy at each stage, we can
   instead try to maximize the next diagonal element of L.
  This is a measure of how orthogonal the next basis function is to the
   subspace spanned by the existing basis functions.
  Because the divisions do not accumulate in the formulation of the Cholesky
   decomposition, the values on the diagonal are well-conditioned, unlike the
   pivot values in Gaussian elimination.

  This method turns out to work very well in practice, and is implemented
   below.
  The greedy optimization approach appears sufficient to avoid any of the
   numerical instabilities present in the other methods discussed.
  A threshold is no longer required for a stopping criteria, since a
   sufficiently independent set of basis functions can always be found in the
   DCT.

  Once the set of basis functions has been selected, the linear system
   p' = G' (L^-T L^-1) G^T p is solved, where p' is the vector of extrapolated
   pixel values, and G' is the portion of the 2D iDCT basis functions used in
   G that corresponds to these pixels.
  This results in a simple m by n matrix, where m is the number of padding
   pixels and n is the number of non-padding pixels.
  The entries of this matrix are scaled down by a power of two to ensure their
   magnitude is less than 1, and then they are stored as signed 1.15
   fixed-point values.
  During the encode, the non-padding pixel values are scaled by this power of
   two before multiplication.
  The total additonal cost of computing the padding is at most 16 multiplies
   per pixel.
  By comparison, Theora's fDCT implementation requires 4 multiplies per pixel.

  The complexity can be further reduced by noting that if the selected basis
   vectors are all orthogonal, then (L^-T L^-1) G^T =
   (G^-1 G^-T) G^T = G^-1 = D^-2 G^T, where D is a diagonal matrix consisting
   of the magnitude of the rows of G.
  Thus computation of padding pixel values can be avoided completely.
  The entire operation reduces to padding with 0's, performing an fDCT, and
   scaling the output.
  To reiterate, this only works when the rows of G are orthogonal, and so this
   is not done here.

  There is no longer any bias in this method towards including low-frequency
   coefficients in the set.
  However, such a bias is neither always possible, nor strictly necessary.
  If the signal statistics already tend to send high frequency coefficients to
   small values, this will not change when some of those high frequencies are
   forced to zero.
  Thus, most of the high frequency coefficients will still be quantized to
   zero, even if they do happen to be included in the basis.
  The important part is that all of the coefficients not in the basis will be
   guaranteed to be zero.
  Zeroing out low-frequency coefficients can be as beneficial to compression
   as high frequency components.
  Although they do not often form parts of long zero runs, they tend to have
   larger magnitudes that require more bits to store.

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

/*Adds the last row/column to the symmetric matrix G^T G, and then updates the
   Cholesky factorization of it, L.
  The diagonal element is left squared, since sometimes we only care about the
   magnitude of this element relative to other choices.
  Thus callers must manually take the square root when the actual Cholesky
   factorization is needed.
  _l:   The L matrix to update.
  _ac:  The set of basis functions used for each row.
  _nac: The number of rows.
  _ap:  The set of pixels that are not padding.
  _nap: The number of pixels.*/
static void oc_update_l(float _l[64][64],int _ac[64],int _nac,
 int _ap[64],int _nap){
  int aci;
  int acj;
  int ack;
  int api;
  int cri;
  int cci;
  int crj;
  int ccj;
  int pri;
  int pci;
  /*Step 1: Add the next row/column of G^T G.*/
  aci=_nac-1;
  cri=_ac[aci]>>3;
  cci=_ac[aci]&7;
  for(acj=0;acj<=aci;acj++){
    crj=_ac[acj]>>3;
    ccj=_ac[acj]&7;
    _l[aci][acj]=0;
    for(api=0;api<_nap;api++){
      pri=_ap[api]>>3;
      pci=_ap[api]&7;
      _l[aci][acj]+=OC_IDCT_MATRIX[pci][ccj]*OC_IDCT_MATRIX[pri][crj]*
       OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
    }
  }
  /*Step 2: Convert the newly added row to the corresponding row of the
     Cholesky factorization.*/
  for(acj=0;acj<aci;acj++){
    for(ack=0;ack<acj;ack++)_l[aci][acj]-=_l[aci][ack]*_l[acj][ack];
    _l[aci][acj]/=_l[acj][acj];
  }
  for(ack=0;ack<aci;ack++)_l[aci][aci]-=_l[aci][ack]*_l[aci][ack];
}

/*Computes the set of basis functions to use to synthesize a partial block.
  These basis functions are then used to derive a matrix that extends the
   signal into the missing region from the pixel values that are present.
  _border_info: The border info structure to initialize with details about the
                 selected basis set.
  _border:      A description of the partial block.*/
void oc_border_enc_info_init(oc_border_enc_info *_border_info,
 const oc_border_info *_border){
  float ext_matrix[64][64];
  float b[64];
  float l[64][64];
  float best_l_nn2;
  float scale;
  float w;
  int   ap[64];
  int   zp[64];
  int   ac[64];
  int   best_aci;
  int   shift;
  int   nap;
  int   nzp;
  int   nac;
  int   api;
  int   zpi;
  int   aci;
  int   acj;
  int   cri;
  int   cci;
  int   pri;
  int   pci;
  int   pi;
  int   ci;
  for(zpi=api=pi=0;pi<64;pi++){
    if((int)(_border->mask>>pi)&1)ap[api++]=pi;
    else zp[zpi++]=pi;
  }
  nap=api;
  nzp=zpi;
  for(api=0;api<nap;api++)_border_info->pi[api]=(unsigned char)ap[api];
  for(zpi=0;zpi<nzp;zpi++)_border_info->pi[nap+zpi]=(unsigned char)zp[zpi];
  /*Step 1: Select a linearly independent set of basis functions.*/
  for(ci=0;ci<64;ci++)ac[OC_IZIG_ZAG[ci]]=ci;
  /*Always include the DC coefficient.*/
  for(nac=1;;nac++){
    /*Update L with the most recently selected basis function.*/
    oc_update_l(l,ac,nac,ap,nap);
    l[nac-1][nac-1]=OC_SQRTF(l[nac-1][nac-1]);
    if(nac>=nap)break;
    /*Choose the next basis function to add based on its potential entry on
       the diagonal of the Cholesky factorization L.*/
    best_aci=nac;
    best_l_nn2=0;
    for(aci=nac;aci<64;aci++){
      ci=ac[nac];
      ac[nac]=ac[aci];
      ac[aci]=ci;
      oc_update_l(l,ac,nac+1,ap,nap);
      if(l[nac][nac]>best_l_nn2){
        best_l_nn2=l[nac][nac];
        best_aci=aci;
      }
      ci=ac[nac];
      ac[nac]=ac[aci];
      ac[aci]=ci;
    }
    ci=ac[nac];
    ac[nac]=ac[best_aci];
    ac[best_aci]=ci;
  }
  /*Step 2: Compute the extrapolation matrix.
    We also compute a shift value to apply to all the non-zero pixel values.
    We scale the input pixels by this amount and then divide the matrix
     entries by it, so that we're guaranteed that the entries are always less
     than one and can be approximated by a signed 1.15 fixed point value.*/
  shift=0;
  for(api=0;api<nap;api++){
    pri=ap[api]>>3;
    pci=ap[api]&7;
    for(aci=0;aci<nac;aci++){
      cri=ac[aci]>>3;
      cci=ac[aci]&7;
      b[aci]=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri];
    }
    for(aci=0;aci<nac;aci++){
      for(acj=0;acj<aci;acj++)b[aci]-=l[aci][acj]*b[acj];
      b[aci]/=l[aci][aci];
    }
    for(aci=nac;aci-->0;){
      for(acj=aci+1;acj<nac;acj++)b[aci]-=l[acj][aci]*b[acj];
      b[aci]/=l[aci][aci];
    }
    for(zpi=0;zpi<nzp;zpi++){
      pri=zp[zpi]>>3;
      pci=zp[zpi]&7;
      w=0;
      for(aci=0;aci<nac;aci++){
        cri=ac[aci]>>3;
        cci=ac[aci]&7;
        w+=OC_IDCT_MATRIX[pci][cci]*OC_IDCT_MATRIX[pri][cri]*b[aci];
      }
      ext_matrix[zpi][api]=w;
      w=OC_FABSF(w);
      while(w>=(1<<shift))shift++;
    }
  }
  _border_info->shift=shift;
  scale=1.0F/(1<<shift);
  _border_info->ext_matrix=(ogg_int16_t **)oc_malloc_2d(nzp,nap,
   sizeof(_border_info->ext_matrix[0][0]));
  for(zpi=0;zpi<nzp;zpi++){
    for(api=0;api<nap;api++){
      w=ext_matrix[zpi][api]*scale;
      _border_info->ext_matrix[zpi][api]=(ogg_int16_t)
       OC_CLAMPI(-32768,(int)(32768*w+(w<0?-0.5F:0.5F)),32767);
    }
  }
}

/*Performs a forward 8x8 Type-II DCT transform on blocks which overlap the
   border of the displayable region.
  _border:      A description of which pixels are inside the border.
  _border_info: Information needed to extrapolate pixel values outside the
                 border.
  _y:           The buffer to store the result in.
                This may be the same as _x.
  _x:           The input coefficients.
                Pixel values outside the border will be modified.*/
void oc_fdct8x8_border(const oc_border_info *_border,
 const oc_border_enc_info *_border_info,
 ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int32_t ap[64];
  int         nap;
  int         nzp;
  int         api;
  int         zpi;
  nap=_border->npixels;
  nzp=64-nap;
  for(api=0;api<nap;api++){
    ap[api]=_x[_border_info->pi[api]]<<_border_info->shift;
  }
  for(zpi=0;zpi<nzp;zpi++){
    ogg_int32_t v;
    for(v=api=0;api<nap;api++){
      v+=_border_info->ext_matrix[zpi][api]*ap[api];
    }
    _x[_border_info->pi[nap+zpi]]=(ogg_int16_t)OC_DIV_ROUND_POW2(v,15,16384);
  }
  oc_fdct8x8(_y,_x);
}
