#include <stdlib.h>
#include <ogg/ogg.h>
#include "dct.h"
#include "fdct.h"
#include "idct.h"

/*Divides _x by 65536, truncating towards 0.*/
#define DIV2_16(_x) OC_DIV_POW2(_x,16,0xFFFF)

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
  _y[0<<3]=(ogg_int16_t)DIV2_16(OC_C4S4*(u+v));
  _y[4<<3]=(ogg_int16_t)DIV2_16(OC_C4S4*(u-v));
  /*Apply rotation for outputs 2 and 6.*/
  u=t[0]-t[2];
  v=t[6]-t[4];
  _y[2<<3]=(ogg_int16_t)(DIV2_16(OC_C2S6*u)+DIV2_16(OC_C6S2*v));
  _y[6<<3]=(ogg_int16_t)(DIV2_16(OC_C6S2*u)-DIV2_16(OC_C2S6*v));
  /*Compute some common terms.*/
  t[3]=DIV2_16(OC_C4S4*(t[1]-t[3]));
  t[4]=-DIV2_16(OC_C4S4*(t[6]+t[4]));
  /*Apply rotation for outputs 1 and 7.*/
  u=t[7]+t[3];
  v=t[4]-t[5];
  _y[1<<3]=(ogg_int16_t)(DIV2_16(OC_C1S7*u)-DIV2_16(OC_C7S1*v));
  _y[7<<3]=(ogg_int16_t)(DIV2_16(OC_C7S1*u)+DIV2_16(OC_C1S7*v));
  /*Apply rotation for outputs 3 and 5.*/
  u=t[7]-t[3];
  v=t[4]+t[5];
  _y[3<<3]=(ogg_int16_t)(DIV2_16(OC_C3S5*u)-DIV2_16(OC_C5S3*v));
  _y[5<<3]=(ogg_int16_t)(DIV2_16(OC_C5S3*u)+DIV2_16(OC_C3S5*v));
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

/*Performs a forward 8x8 Type-II DCT transform on blocks which overlap the
   border of the displayable region.
  This uses an iterative method to compute optimal padding values so as to
   make as many of the final coefficients (in zig-zag order) as close to 0 as
   possible.
  _y: The buffer to store the result in.
      Unlike above, this may NOT be the same as _x.
  _x: The input coefficients.*/
void oc_fdct8x8_border(const oc_border_info *_border,ogg_int16_t _y[64],
 ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  ogg_int32_t old_residual;
  ogg_int32_t residual;
  int         ave;
  int         i;
  /*TODO: Figure out the dynamic range and scale the input for increased
     accuracy.*/
  /*Start padding with the average pixel value.*/
  ave=0;
  for(i=0;i<64;i++)if(_border->mask&1<<i)ave+=_x[i];
  ave/=_border->npixels;
  for(i=0;i<64;i++)if(!(_border->mask&1<<i))_x[i]=(ogg_int16_t)ave;
  /*Initialize the residual to some large value.*/
  residual=32768*64;
  do{
    old_residual=residual;
    oc_fdct8x8(_y,_x);
    residual=0;
    for(i=_border->npixels;i<64;i++){
      int z;
      z=OC_FZIG_ZAG[i];
      residual+=abs(_y[z]);
      _y[z]=0;
    }
    oc_idct8x8(w,_y);
    for(i=0;i<64;i++)if(!(_border->mask&1<<i))_x[i]=w[i];
  }
  while(residual<old_residual);
}
