#include <string.h>
#include <ogg/ogg.h>
#include "dct.h"
#include "idct.h"

/*Performs an inverse 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 relative to the orthonormal version of
   the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      The first 8 entries are used (e.g., from a row of an 8x8 block).*/
static void idct8(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  int t[8];
  int r;
  /*6-1 rotation by 7pi/16.*/
  t[6]=(int)(OC_C7S1*_x[1]>>16)-(int)(OC_C1S7*_x[7]>>16);
  t[1]=(int)(OC_C1S7*_x[1]>>16)+(int)(OC_C7S1*_x[7]>>16);
  /*4-7 rotation by 3pi/16.*/
  t[4]=(int)(OC_C3S5*_x[5]>>16)-(int)(OC_C5S3*_x[3]>>16);
  t[7]=(int)(OC_C5S3*_x[5]>>16)+(int)(OC_C3S5*_x[3]>>16);
  /*7-1 reverse butterfly.*/
  r=t[1]-t[7];
  t[7]+=t[1];
  t[1]=r*OC_C4S4>>16;
  /*4-6 reverse butterfly.*/
  r=t[6]-t[4];
  t[4]+=t[6];
  t[6]=r*OC_C4S4>>16;
  /*5-3 rotation by pi/4.*/
  t[5]=(int)(OC_C4S4*(_x[0]-_x[4])>>16);
  t[3]=(int)(OC_C4S4*(_x[0]+_x[4])>>16);
  /*2-0 rotation by 3pi/8.*/
  t[2]=(int)(OC_C6S2*_x[2]>>16)-(int)(OC_C2S6*_x[6]>>16);
  t[0]=(int)(OC_C2S6*_x[2]>>16)+(int)(OC_C6S2*_x[6]>>16);
  /*1-5 reverse butterfly.*/
  r=t[5]+t[1];
  t[5]-=t[1];
  t[1]=r;
  /*0-3 reverse butterfly.*/
  r=t[3]-t[0];
  t[0]+=t[3];
  t[3]=r;
  /*2-6 reverse butterfly.*/
  r=t[6]-t[2];
  t[2]+=t[6];
  t[6]=r;
  /*0-7 butterfly (first half).*/
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  /*1-2 butterfly.*/
  _y[1<<3]=(ogg_int16_t)(t[1]+t[2]);
  _y[2<<3]=(ogg_int16_t)(t[1]-t[2]);
  /*3-4 butterfly.*/
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  /*5-6 butterfly.*/
  _y[5<<3]=(ogg_int16_t)(t[5]+t[6]);
  _y[6<<3]=(ogg_int16_t)(t[5]-t[6]);
  /*0-7 butterfly (second half).*/
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}

/*Performs an inverse 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 relative to the orthonormal version of
   the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      Only the first 4 entries are used.
      The other 4 are assumed to be 0.*/
static void idct8_4(ogg_int16_t *_y,const ogg_int16_t _x[4]){
  int t[8];
  int r;
  t[6]=(int)(OC_C7S1*_x[1]>>16);
  t[1]=(int)(OC_C1S7*_x[1]>>16);
  t[4]=-(int)(OC_C5S3*_x[3]>>16);
  t[7]=(int)(OC_C3S5*_x[3]>>16);
  r=t[1]-t[7];
  t[7]+=t[1];
  t[1]=r*OC_C4S4>>16;
  r=t[6]-t[4];
  t[4]+=t[6];
  t[6]=r*OC_C4S4>>16;
  t[3]=t[5]=(int)(OC_C4S4*_x[0]>>16);
  t[2]=(int)(OC_C6S2*_x[2]>>16);
  t[0]=(int)(OC_C2S6*_x[2]>>16);
  t[5]-=t[1];
  t[1]+=t[3];
  r=t[3]-t[0];
  t[0]+=t[3];
  t[3]=r;
  r=t[6]-t[2];
  t[2]+=t[6];
  t[6]=r;
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  _y[1<<3]=(ogg_int16_t)(t[1]+t[2]);
  _y[2<<3]=(ogg_int16_t)(t[1]-t[2]);
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[5]+t[6]);
  _y[6<<3]=(ogg_int16_t)(t[5]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}

/*Performs an inverse 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 relative to the orthonormal version of
   the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      Only the first 3 entries are used.
      The other 5 are assumed to be 0.*/
static void idct8_3(ogg_int16_t *_y,const ogg_int16_t _x[3]){
  int t[8];
  int r;
  t[4]=t[6]=(int)(OC_C7S1*_x[1]>>16);
  t[7]=t[1]=(int)(OC_C1S7*_x[1]>>16);
  t[1]=t[1]*OC_C4S4>>16;
  t[6]=t[6]*OC_C4S4>>16;
  t[3]=t[5]=(int)(OC_C4S4*_x[0]>>16);
  t[2]=(int)(OC_C6S2*_x[2]>>16);
  t[0]=(int)(OC_C2S6*_x[2]>>16);
  t[5]-=t[1];
  t[1]+=t[3];
  r=t[3]-t[0];
  t[0]+=t[3];
  t[3]=r;
  r=t[6]-t[2];
  t[2]+=t[6];
  t[6]=r;
  _y[0<<3]=(ogg_int16_t)(t[0]+t[7]);
  _y[1<<3]=(ogg_int16_t)(t[1]+t[2]);
  _y[2<<3]=(ogg_int16_t)(t[1]-t[2]);
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[5]+t[6]);
  _y[6<<3]=(ogg_int16_t)(t[5]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[0]-t[7]);
}

/*Performs an inverse 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 relative to the orthonormal version of
   the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      Only the first 2 entries are used.
      The other 6 are assumed to be 0.*/
static void idct8_2(ogg_int16_t *_y,const ogg_int16_t _x[3]){
  int t[8];
  t[4]=t[6]=(int)(OC_C7S1*_x[1]>>16);
  t[7]=t[1]=(int)(OC_C1S7*_x[1]>>16);
  t[1]=t[1]*OC_C4S4>>16;
  t[6]=t[6]*OC_C4S4>>16;
  t[3]=t[5]=(int)(OC_C4S4*_x[0]>>16);
  t[5]-=t[1];
  t[1]+=t[3];
  _y[0<<3]=(ogg_int16_t)(t[3]+t[7]);
  _y[1<<3]=(ogg_int16_t)(t[1]+t[6]);
  _y[2<<3]=(ogg_int16_t)(t[1]-t[6]);
  _y[3<<3]=(ogg_int16_t)(t[3]+t[4]);
  _y[4<<3]=(ogg_int16_t)(t[3]-t[4]);
  _y[5<<3]=(ogg_int16_t)(t[5]+t[6]);
  _y[6<<3]=(ogg_int16_t)(t[5]-t[6]);
  _y[7<<3]=(ogg_int16_t)(t[3]-t[7]);
}

/*Performs an inverse 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 relative to the orthonormal version of
   the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      Only the first entry is used.
      The other 7 are assumed to be 0.*/
static void idct8_1(ogg_int16_t *_y,const ogg_int16_t _x[1]){
  _y[0<<3]=_y[1<<3]=_y[2<<3]=_y[3<<3]=
   _y[4<<3]=_y[5<<3]=_y[6<<3]=_y[7<<3]=(ogg_int16_t)(OC_C4S4*_x[0]>>16);
}

/*Performs an inverse 8x8 Type-II DCT transform.
  The input is assumed to be scaled by a factor of 4 relative to orthonormal
   version of the transform.
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      This may be the same as _x.
  _x: The input coefficients. */
void oc_idct8x8(ogg_int16_t _y[64],const ogg_int16_t _x[64]){
  const ogg_int16_t *in;
  ogg_int16_t       *end;
  ogg_int16_t       *out;
  ogg_int16_t        w[64];
  /*Transform rows of x into columns of w.*/
  for(in=_x,out=w,end=out+8;out<end;in+=8,out++)idct8(out,in);
  /*Transform rows of w into columns of y.*/
  for(in=w,out=_y,end=out+8;out<end;in+=8,out++)idct8(out,in);
  /*Adjust for scale factor.*/
  for(out=_y,end=out+64;out<end;out++)*out=(ogg_int16_t)(*out+8>>4);
}

/*Performs an inverse 8x8 Type-II DCT transform.
  The input is assumed to be scaled by a factor of 4 relative to orthonormal
   version of the transform.
  All coefficients but the first 10 in zig-zag scan order are assumed to be 0:
   x  x  x  x  0  0  0  0
   x  x  x  0  0  0  0  0
   x  x  0  0  0  0  0  0
   x  0  0  0  0  0  0  0
   0  0  0  0  0  0  0  0
   0  0  0  0  0  0  0  0
   0  0  0  0  0  0  0  0
   0  0  0  0  0  0  0  0
  TODO: What is the maximum dynamic range of the input? output?
  _y: The buffer to store the result in.
      This may be the same as _x.
  _x: The input coefficients. */
void oc_idct8x8_10(ogg_int16_t _y[64],const ogg_int16_t _x[64]){
  const ogg_int16_t *in;
  ogg_int16_t       *end;
  ogg_int16_t       *out;
  ogg_int16_t        w[64];
  /*Transform rows of x into columns of w.*/
  idct8_4(w,_x);
  idct8_3(w+1,_x+8);
  idct8_2(w+2,_x+16);
  idct8_1(w+3,_x+24);
  /*Transform rows of w into columns of y.*/
  for(in=w,out=_y,end=out+8;out<end;in+=8,out++)idct8_4(out,in);
  /*Adjust for scale factor.*/
  for(out=_y,end=out+64;out<end;out++)*out=(ogg_int16_t)(*out+8>>4);
}
