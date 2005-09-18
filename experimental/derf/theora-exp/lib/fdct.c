#include <stdlib.h>
#include <string.h>
#include "dct.h"
#include "fdct.h"



/*Performs a forward 8 point Type-II DCT transform.
  The output is scaled by a factor of 2 from the orthonormal version of the
   transform.
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      The first 8 entries are used (e.g., from a row of an 8x8 block).*/
static void fdct8(ogg_int16_t *_y,const ogg_int16_t _x[8]){
  ogg_int32_t t[9];
  ogg_int32_t r;
  /*Stage 1:*/
  /*0-7 butterfly.*/
  t[0]=_x[0]+(ogg_int32_t)_x[7];
  /*1-6 butterfly.*/
  t[1]=_x[1]+(ogg_int32_t)_x[6];
  /*2-5 butterfly.*/
  t[2]=_x[2]+(ogg_int32_t)_x[5];
  /*3-4 butterfly.*/
  t[3]=_x[3]+(ogg_int32_t)_x[4];
  t[4]=_x[3]-(ogg_int32_t)_x[4];
  t[5]=_x[2]-(ogg_int32_t)_x[5];
  t[6]=_x[1]-(ogg_int32_t)_x[6];
  t[7]=_x[0]-(ogg_int32_t)_x[7];
  /*Stage 2:*/
  /*0-3 butterfly.*/
  r=t[0]+t[3];
  t[3]=t[0]-t[3];
  t[0]=r;
  /*1-2 butterfly.*/
  r=t[1]+t[2];
  t[2]=t[1]-t[2];
  t[1]=r;
  /*6-5 butterfly.*/
  r=t[6]-t[5];
  t[6]=OC_DIV2_16(OC_C4S4*(t[6]+t[5]));
  t[5]=OC_DIV2_16(OC_C4S4*r);
  /*Stage 3:*/
  /*4-5 butterfly.*/
  r=t[4]+t[5];
  t[5]=t[4]-t[5];
  t[4]=r;
  /*7-6 butterfly.*/
  r=t[7]+t[6];
  t[6]=t[7]-t[6];
  t[7]=r;
  /*0-1 butterfly.*/
  _y[0<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C4S4*(t[0]+t[1])));
  _y[4<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C4S4*(t[0]-t[1])));
  /*3-2 rotation by 6pi/16*/
  _y[2<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C2S6*t[3])+OC_DIV2_16(OC_C6S2*t[2]));
  _y[6<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C6S2*t[3])-OC_DIV2_16(OC_C2S6*t[2]));
  /*Stage 4:*/
  /*7-4 rotation by 7pi/16*/
  _y[1<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C1S7*t[7])+OC_DIV2_16(OC_C7S1*t[4]));
  /*6-5 rotation by 3pi/16*/
  _y[5<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C5S3*t[6])+OC_DIV2_16(OC_C3S5*t[5]));
  _y[3<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C3S5*t[6])-OC_DIV2_16(OC_C5S3*t[5]));
  _y[7<<3]=(ogg_int16_t)(OC_DIV2_16(OC_C7S1*t[7])-OC_DIV2_16(OC_C1S7*t[4]));
}

/*Performs a forward 8x8 Type-II DCT transform.
  The output is scaled by a factor of 4 relative to the orthonormal version
   of the transform.
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

/*Information needed to pad boundary blocks.
  We multiply each row/column by an extension matrix that fills in the padding
   values as a linear combination of the active values, so that an equivalent
   number of coefficients are forced to zero.
  This costs at most 16 multiplies, the same as a 1-D fDCT itself, and as
   little as 7 multiplies.
  We compute the extension matrices for every possible shape in advance, as
   there are only 35.
  The coefficients for all matrices are stored in a single array to take
   advantage of the overlap and repetitiveness of many of the shapes.
  A similar technique is applied to the offsets into this array.
  This reduces the required table storage by about 48%.
  See tools/extgen.c for details.
  We could conceivably do the same for all 256 possible shapes.*/
typedef struct oc_extension_info{
  /*The mask of the active pixels in the shape.*/
  short                     mask;
  /*The number of active pixels in the shape.*/
  short                     na;
  /*The extension matrix.
    This is (8-na)xna*/
  const ogg_int32_t *const *ext;
  /*The pixel indices: na active pixels followed by 8-na padding pixels.*/
  unsigned char             pi[8];
  /*The coefficient indices: na unconstrained coefficients followed by 8-na
     coefficients to be forced to zero.*/
  unsigned char             ci[8];
}oc_extension_info;



/*The precision at which the matrix coefficients are stored.*/
#define OC_EXT_SHIFT (19)

/*The number of shapes we need.*/
#define OC_NSHAPES   (35)

static const ogg_int32_t OC_EXT_COEFFS[231]={
   0x80000,-0x1E085,-0x96FC9,-0x55871, 0x55871, 0x96FC9, 0x1E085, 0x96FC9,
   0x55871,-0x55871,-0x96FC9,-0x1E085, 0x80000, 0x00000, 0x00000, 0x00000,
   0x80000, 0x00000, 0x00000, 0x80000,-0x80000, 0x80000, 0x00000, 0x00000,
   0x80000,-0x1E086, 0x1E086,-0x4F591,-0x55E37, 0x337C7, 0x8C148, 0x43448,
   0x2266F, 0x43448, 0x8C148, 0x337C7,-0x55E37,-0x4F591,-0x75743, 0x4F591,
   0x03B45,-0x1D29D, 0x929E0, 0x2CF2A, 0x929E0,-0x1D29D, 0x03B45, 0x4F591,
  -0x75743, 0x11033, 0x7AEF5, 0x5224C,-0x209FA,-0x3D77A,-0x209FA, 0x5224C,
   0x7AEF5, 0x11033, 0x668A4,-0x29124, 0x3A15B, 0x0E6BD,-0x05F98, 0x0E6BD,
   0x3A15B,-0x29124, 0x668A4, 0x2A78F, 0x24019,-0x67F0F, 0x50F49, 0x4881E,
   0x50F49,-0x67F0F, 0x24019, 0x2A78F,-0x06898, 0x27678, 0x5F220, 0x27678,
  -0x06898, 0x1F910, 0x76C13,-0x16523, 0x76C13, 0x1F910, 0x9EB2C,-0x2E7B1,
   0x0FC86,-0x2E7B1, 0x9EB2C, 0x4F594, 0x43452,-0x129E6, 0x43452, 0x4F594,
  -0x0A8C0, 0x5D997, 0x2CF29, 0x5D997,-0x0A8C0, 0x30FBD,-0x0B7E5,-0x6AC38,
  -0x153C8, 0x8B7E5, 0x4F043, 0x8B7E5,-0x153C8,-0x6AC38,-0x0B7E5, 0x30FBD,
  -0x5A827, 0x153C8, 0x6AC38, 0x3C7A2, 0x1E085, 0x3C7A2, 0x6AC38, 0x153C8,
  -0x5A827, 0x5A827, 0x6AC38, 0x153C8,-0x3C7A2,-0x1E085,-0x3C7A2, 0x153C8,
   0x6AC38, 0x5A827, 0x30FBB, 0x4F045, 0x273D7,-0x273D7, 0x273D7, 0x1E08B,
   0x61F75, 0x1E08B, 0x273D7,-0x273D7, 0x273D7, 0x4F045, 0x30FBB,-0x273D7,
   0x273D7, 0x9E08B,-0x1E08B, 0x9E08B, 0x273D7,-0x273D7, 0x4F045, 0x30FBB,
  -0x273D7, 0x273D7,-0x273D7, 0x30FBB, 0x4F045, 0x1FC86, 0x67AC8, 0x18538,
  -0x1FC86, 0x18538, 0x67AC8, 0x1FC86, 0x45460,-0x1FC87, 0x1FC87, 0x3ABA0,
   0x1FC87,-0x1FC87, 0x45460, 0x35052, 0x5586D,-0x0A8BF, 0x5586D, 0x35052,
  -0x43EF2, 0x78F43, 0x4AFAE,-0x070BD, 0x3C10E, 0x3C10E,-0x070BD, 0x4AFAE,
   0x78F43,-0x43EF2, 0x3C10E, 0x78F43,-0x35052, 0x78F43, 0x3C10E, 0x070BD,
   0x236D6, 0x5586D, 0x236D6, 0x070BD,-0x07755, 0x3C79F, 0x4AFB6,-0x190D2,
   0x4E11C, 0x9FC86, 0x153C4,-0x3504A, 0xAC38E, 0x08CBB, 0x1E086,-0x1E086,
   0x80000, 0x08CBB, 0xAC38E,-0x3504A, 0x153C4, 0x9FC86, 0x4E11C,-0x190D2,
   0x4AFB6, 0x3C79F,-0x07755,-0x5A818, 0xDA818,-0x5A818,-0x101C31, 0x181C31,
  -0x101C31,-0xD0C67, 0x150C67,-0xD0C67,-0x7643F, 0xF643F,-0x7643F
};

static const ogg_int32_t *const OC_EXT_ROWS[96]={
  OC_EXT_COEFFS+   0,OC_EXT_COEFFS+   0,OC_EXT_COEFFS+   0,OC_EXT_COEFFS+   0,
  OC_EXT_COEFFS+   0,OC_EXT_COEFFS+   0,OC_EXT_COEFFS+   0,OC_EXT_COEFFS+   6,
  OC_EXT_COEFFS+  27,OC_EXT_COEFFS+  38,OC_EXT_COEFFS+  43,OC_EXT_COEFFS+  32,
  OC_EXT_COEFFS+  49,OC_EXT_COEFFS+  58,OC_EXT_COEFFS+  67,OC_EXT_COEFFS+  71,
  OC_EXT_COEFFS+  62,OC_EXT_COEFFS+  53,OC_EXT_COEFFS+  12,OC_EXT_COEFFS+  15,
  OC_EXT_COEFFS+  14,OC_EXT_COEFFS+  13,OC_EXT_COEFFS+  76,OC_EXT_COEFFS+  81,
  OC_EXT_COEFFS+  86,OC_EXT_COEFFS+  91,OC_EXT_COEFFS+  96,OC_EXT_COEFFS+  98,
  OC_EXT_COEFFS+  93,OC_EXT_COEFFS+  88,OC_EXT_COEFFS+  83,OC_EXT_COEFFS+  78,
  OC_EXT_COEFFS+  12,OC_EXT_COEFFS+  15,OC_EXT_COEFFS+  15,OC_EXT_COEFFS+  12,
  OC_EXT_COEFFS+  12,OC_EXT_COEFFS+  15,OC_EXT_COEFFS+  12,OC_EXT_COEFFS+  15,
  OC_EXT_COEFFS+  15,OC_EXT_COEFFS+  12,OC_EXT_COEFFS+ 101,OC_EXT_COEFFS+ 106,
  OC_EXT_COEFFS+ 112,OC_EXT_COEFFS+  16,OC_EXT_COEFFS+ 121,OC_EXT_COEFFS+ 125,
  OC_EXT_COEFFS+  20,OC_EXT_COEFFS+ 116,OC_EXT_COEFFS+ 130,OC_EXT_COEFFS+ 133,
  OC_EXT_COEFFS+ 143,OC_EXT_COEFFS+ 150,OC_EXT_COEFFS+ 157,OC_EXT_COEFFS+ 164,
  OC_EXT_COEFFS+ 167,OC_EXT_COEFFS+ 160,OC_EXT_COEFFS+ 153,OC_EXT_COEFFS+ 146,
  OC_EXT_COEFFS+ 136,OC_EXT_COEFFS+ 139,OC_EXT_COEFFS+ 171,OC_EXT_COEFFS+ 176,
  OC_EXT_COEFFS+ 181,OC_EXT_COEFFS+ 186,OC_EXT_COEFFS+ 191,OC_EXT_COEFFS+ 196,
  OC_EXT_COEFFS+ 201,OC_EXT_COEFFS+ 206,OC_EXT_COEFFS+ 209,OC_EXT_COEFFS+ 214,
  OC_EXT_COEFFS+ 198,OC_EXT_COEFFS+ 203,OC_EXT_COEFFS+  24,OC_EXT_COEFFS+ 211,
  OC_EXT_COEFFS+ 216,OC_EXT_COEFFS+ 193,OC_EXT_COEFFS+ 188,OC_EXT_COEFFS+ 178,
  OC_EXT_COEFFS+ 183,OC_EXT_COEFFS+ 173,OC_EXT_COEFFS+ 219,OC_EXT_COEFFS+ 220,
  OC_EXT_COEFFS+ 220,OC_EXT_COEFFS+  12,OC_EXT_COEFFS+  15,OC_EXT_COEFFS+ 219,
  OC_EXT_COEFFS+ 219,OC_EXT_COEFFS+ 220,OC_EXT_COEFFS+ 222,OC_EXT_COEFFS+ 225,
  OC_EXT_COEFFS+ 228,OC_EXT_COEFFS+ 229,OC_EXT_COEFFS+ 226,OC_EXT_COEFFS+ 223
};

static const oc_extension_info OC_EXTENSION_INFO[OC_NSHAPES]={
  {0x7F,7,OC_EXT_ROWS+  0,{0,1,2,3,4,5,6,7},{0,1,2,4,5,6,7,3}},
  {0xFE,7,OC_EXT_ROWS+  7,{1,2,3,4,5,6,7,0},{0,1,2,4,5,6,7,3}},
  {0x3F,6,OC_EXT_ROWS+  8,{0,1,2,3,4,5,7,6},{0,1,3,4,6,7,5,2}},
  {0xFC,6,OC_EXT_ROWS+ 10,{2,3,4,5,6,7,1,0},{0,1,3,4,6,7,5,2}},
  {0x1F,5,OC_EXT_ROWS+ 12,{0,1,2,3,4,7,6,5},{0,2,3,5,7,6,4,1}},
  {0xF8,5,OC_EXT_ROWS+ 15,{3,4,5,6,7,2,1,0},{0,2,3,5,7,6,4,1}},
  {0x0F,4,OC_EXT_ROWS+ 18,{0,1,2,3,7,6,5,4},{0,2,4,6,7,5,3,1}},
  {0xF0,4,OC_EXT_ROWS+ 18,{4,5,6,7,3,2,1,0},{0,2,4,6,7,5,3,1}},
  {0x07,3,OC_EXT_ROWS+ 22,{0,1,2,7,6,5,4,3},{0,3,6,7,5,4,2,1}},
  {0xE0,3,OC_EXT_ROWS+ 27,{5,6,7,4,3,2,1,0},{0,3,6,7,5,4,2,1}},
  {0x03,2,OC_EXT_ROWS+ 32,{0,1,7,6,5,4,3,2},{0,4,7,6,5,3,2,1}},
  {0xC0,2,OC_EXT_ROWS+ 32,{6,7,5,4,3,2,1,0},{0,4,7,6,5,3,2,1}},
  {0x01,1,OC_EXT_ROWS+  0,{0,7,6,5,4,3,2,1},{0,7,6,5,4,3,2,1}},
  {0x80,1,OC_EXT_ROWS+  0,{7,6,5,4,3,2,1,0},{0,7,6,5,4,3,2,1}},
  {0x7E,6,OC_EXT_ROWS+ 42,{1,2,3,4,5,6,7,0},{0,1,2,5,6,7,4,3}},
  {0x7C,5,OC_EXT_ROWS+ 44,{2,3,4,5,6,7,1,0},{0,1,4,5,7,6,3,2}},
  {0x3E,5,OC_EXT_ROWS+ 47,{1,2,3,4,5,7,6,0},{0,1,4,5,7,6,3,2}},
  {0x78,4,OC_EXT_ROWS+ 50,{3,4,5,6,7,2,1,0},{0,4,5,7,6,3,2,1}},
  {0x3C,4,OC_EXT_ROWS+ 54,{2,3,4,5,7,6,1,0},{0,3,4,7,6,5,2,1}},
  {0x1E,4,OC_EXT_ROWS+ 58,{1,2,3,4,7,6,5,0},{0,4,5,7,6,3,2,1}},
  {0x70,3,OC_EXT_ROWS+ 62,{4,5,6,7,3,2,1,0},{0,5,7,6,4,3,2,1}},
  {0x38,3,OC_EXT_ROWS+ 67,{3,4,5,7,6,2,1,0},{0,5,6,7,4,3,2,1}},
  {0x1C,3,OC_EXT_ROWS+ 72,{2,3,4,7,6,5,1,0},{0,5,6,7,4,3,2,1}},
  {0x0E,3,OC_EXT_ROWS+ 77,{1,2,3,7,6,5,4,0},{0,5,7,6,4,3,2,1}},
  {0x60,2,OC_EXT_ROWS+ 82,{5,6,7,4,3,2,1,0},{0,2,7,6,5,4,3,1}},
  {0x30,2,OC_EXT_ROWS+ 36,{4,5,7,6,3,2,1,0},{0,4,7,6,5,3,2,1}},
  {0x18,2,OC_EXT_ROWS+ 90,{3,4,7,6,5,2,1,0},{0,1,7,6,5,4,3,2}},
  {0x0C,2,OC_EXT_ROWS+ 34,{2,3,7,6,5,4,1,0},{0,4,7,6,5,3,2,1}},
  {0x06,2,OC_EXT_ROWS+ 84,{1,2,7,6,5,4,3,0},{0,2,7,6,5,4,3,1}},
  {0x40,1,OC_EXT_ROWS+  0,{6,7,5,4,3,2,1,0},{0,7,6,5,4,3,2,1}},
  {0x20,1,OC_EXT_ROWS+  0,{5,7,6,4,3,2,1,0},{0,7,6,5,4,3,2,1}},
  {0x10,1,OC_EXT_ROWS+  0,{4,7,6,5,3,2,1,0},{0,7,6,5,4,3,2,1}},
  {0x08,1,OC_EXT_ROWS+  0,{3,7,6,5,4,2,1,0},{0,7,6,5,4,3,2,1}},
  {0x04,1,OC_EXT_ROWS+  0,{2,7,6,5,4,3,1,0},{0,7,6,5,4,3,2,1}},
  {0x02,1,OC_EXT_ROWS+  0,{1,7,6,5,4,3,2,0},{0,7,6,5,4,3,2,1}}
};



/*Pads a single row of a partial block and then performs a forward Type-II DCT
   on the result.
  The output is scaled by a factor of 2 from the orthonormal version of the
   transform.
  _y: The buffer to store the result in.
      Data will be placed in every 8th entry (e.g., in a column of an 8x8
       block).
  _x: The input coefficients.
      The first 8 entries are used (e.g., from a row of an 8x8 block).
  _e: The extension information for the shape.*/
static void fdct8_ext(ogg_int16_t *_y,ogg_int16_t _x[8],
 const oc_extension_info *_e){
  if(_e->na==1){
    int ci;
    /*While the branch below is still correct for shapes with na==1, we can
       perform the entire transform with just 1 multiply in this case instead
       of 23.*/
    _y[0]=(ogg_int16_t)(OC_DIV2_16(OC_C4S4*(_x[_e->pi[0]]<<3)));
    for(ci=8;ci<64;ci+=8)_y[ci]=0;
  }
  else{
    int zpi;
    int api;
    int nz;
    /*First multiply by the extension matrix to compute the padding values.*/
    nz=8-_e->na;
    for(zpi=0;zpi<nz;zpi++){
      ogg_int32_t v;
      v=0;
      for(api=0;api<_e->na;api++)v+=_e->ext[zpi][api]*_x[_e->pi[api]];
      _x[_e->pi[zpi+_e->na]]=
       (ogg_int16_t)OC_DIV_ROUND_POW2(v,OC_EXT_SHIFT,1<<OC_EXT_SHIFT-1);
    }
    fdct8(_y,_x);
  }
}

/*Performs a forward 8x8 Type-II DCT transform on blocks which overlap the
   border of the picture region.
  This method ONLY works with rectangular regions.
  _border:      A description of which pixels are inside the border.
  _y:           The buffer to store the result in.
                This may be the same as _x.
  _x:           The input coefficients.
                Pixel values outside the border will be modified.*/
void oc_fdct8x8_border(const oc_border_info *_border,ogg_int16_t _y[64],
 ogg_int16_t _x[64]){
  ogg_int16_t             *in;
  ogg_int16_t             *end;
  ogg_int16_t             *out;
  ogg_int16_t              w[64];
  ogg_int64_t              mask;
  const oc_extension_info *rext;
  const oc_extension_info *cext;
  int                      rmask;
  int                      cmask;
  int                      ri;
  /*Identify the shapes of the non-zero rows and columns.*/
  rmask=cmask=0;
  mask=_border->mask;
  for(ri=0;ri<8;ri++){
    rmask|=mask&0xFF;
    cmask|=((mask&0xFF)!=0)<<ri;
    mask>>=8;
  }
  /*Find the associated extension info for these shapes.*/
  if(rmask==0xFF)rext=NULL;
  else for(rext=OC_EXTENSION_INFO;rext->mask!=rmask;){
    /*If we somehow can't find the shape, then just do an unpadded fDCT.
      It won't be efficient, but it should still be correct.*/
    if(++rext>=OC_EXTENSION_INFO+OC_NSHAPES){
      oc_fdct8x8(_y,_x);
      return;
    }
  }
  if(cmask==0xFF)cext=NULL;
  else for(cext=OC_EXTENSION_INFO;cext->mask!=cmask;){
    /*If we somehow can't find the shape, then just do an unpadded fDCT.
      It won't be efficient, but it should still be correct.*/
    if(++cext>=OC_EXTENSION_INFO+OC_NSHAPES){
      oc_fdct8x8(_y,_x);
      return;
    }
  }
  /*Transform the rows.
    We can ignore zero rows without a problem.*/
  if(rext==NULL)for(in=_x,out=w,end=out+8;out<end;in+=8,out++)fdct8(out,in);
  else for(in=_x,out=w,end=out+8,ri=cmask;out<end;in+=8,out++,ri>>=1){
    if(ri&1)fdct8_ext(out,in,rext);
  }
  /*Transform the columns.
    We transform even columns that are supposedly zero, because rounding errors
     may make them slightly non-zero, and this will give a more precise
     reconstruction with very small quantizers.*/
  if(cext==NULL)for(in=w,out=_y,end=out+8;out<end;in+=8,out++)fdct8(out,in);
  else for(in=w,out=_y,end=out+8;out<end;in+=8,out++)fdct8_ext(out,in,cext);
}

/*Performs an fDCT on a given fragment.
  _frag:     The fragment to perform the 2D DCT on.
  _dct_vals: The output buffer for the DCT coefficients.
  _ystride:  The Y stride of the plane the fragment belongs to.
  _framei:   The picture buffer index to perform the DCT on.
             Use OC_FRAME_IO for the current input frame.*/
void oc_frag_intra_fdct(const oc_fragment *_frag,ogg_int16_t _dct_vals[64],
 int _ystride,int _framei){
  ogg_int16_t    pix_buf[64];
  unsigned char *pixels;
  int            pixi;
  int            y;
  int            x;
  /*NOTE: 128 is subtracted from each pixel value to make it signed.
    The original VP3 source claimed that, "this reduces the internal precision
     requirments [sic] in the DCT transform."
    This is of course not actually true.
    The transform must still support input in the range [-255,255] to code
     predicted fragments, since the same transform is used for both.
    This actually _reduces_ the precision of the results, because larger
     (absolute) values would have fewer significant bits chopped off when
     rounding.
    We're stuck with it, however.
    At least it might reduce bias towards 0 when coding unpredicted DC
     coefficients, but that's not what VP3 justified it with.*/
  pixels=_frag->buffer[_framei];
  /*For border fragments, only copy pixels that are in the displayable
     region of the image.
    The DCT function will compute optimal padding values for the other
     pixels.*/
  if(_frag->border!=NULL){
    ogg_int64_t mask;
    mask=_frag->border->mask;
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++){
        pix_buf[pixi]=(ogg_int16_t)(((int)mask&1)?pixels[x]-128:0);
        /*This branchless code is (almost) equivalent to the previous line:
            int pmask;
            pmask=-(int)mask&1;
            pix_buf[pixi]=(ogg_int16_t)(pmask&pixels[x]);
          We don't use this code to allow the user to pass in a buffer that is
           the exact size of the displayed image, not the size padded to a
           multiple of 16.
          In the latter case, we might segfault on pixels[x] if it is not
           mapped to a valid page, even though we would discard the value
           we were attempting to read.*/
        mask>>=1;
      }
      pixels+=_ystride;
    }
    oc_fdct8x8_border(_frag->border,_dct_vals,pix_buf);
  }
  /*Otherwise, copy all the pixels in the fragment and do a normal DCT.*/
  else{
    for(pixi=y=0;y<8;y++){
      for(x=0;x<8;x++,pixi++)pix_buf[pixi]=(ogg_int16_t)(pixels[x]-128);
      pixels+=_ystride;
    }
    oc_fdct8x8(_dct_vals,pix_buf);
  }
}

/*A pipline stage for applying an fDCT to each (non-motion compensated) block
   in a frame.*/

static int oc_fdct_pipe_start(oc_enc_pipe_stage *_stage){
  int pli;
  for(pli=0;pli<3;pli++)_stage->y_procd[pli]=0;
  return _stage->next!=NULL?(*_stage->next->pipe_start)(_stage->next):0;
}

/*Does the fDCTs.
  This pipeline stage proceeds in a planar fashion.*/
static int oc_fdct_pipe_process(oc_enc_pipe_stage *_stage,int _y_avail[3]){
  int pli;
  for(pli=0;pli<3;pli++){
    int y_procd;
    int y_avail;
    /*Compute how far we can get in complete fragment rows.*/
    y_procd=_stage->y_procd[pli];
    y_avail=_y_avail[pli]&~7;
    /*If that's farther than we've already gotten, do some fDCTs.*/
    if(y_avail>y_procd){
      oc_fragment_plane    *fplane;
      oc_fragment          *frags;
      oc_fragment          *frag_end;
      oc_fragment_enc_info *efrags;
      int                   ystride;
      int                   yfrag0;
      int                   yrows;
      fplane=_stage->enc->state.fplanes+pli;
      ystride=_stage->enc->state.input[pli].ystride;
      yfrag0=fplane->froffset+(y_procd>>3)*fplane->nhfrags;
      yrows=y_avail-y_procd>>3;
      frags=_stage->enc->state.frags+yfrag0;
      efrags=_stage->enc->frinfo+yfrag0;
      do{
        for(frag_end=frags+fplane->nhfrags;frags<frag_end;frags++,efrags++){
          oc_frag_intra_fdct(frags,efrags->dct_coeffs,ystride,OC_FRAME_IO);
        }
        _stage->y_procd[pli]+=8;
        if(_stage->next!=NULL){
          int ret;
          ret=(*_stage->next->pipe_proc)(_stage->next,_stage->y_procd);
          if(ret<0)return ret;
        }
      }
      while(--yrows);
    }
  }
  return 0;
}

static int oc_fdct_pipe_end(oc_enc_pipe_stage *_stage){
  return _stage->next!=NULL?(*_stage->next->pipe_end)(_stage->next):0;
}


/*Initialize the fDCT stage of the pipeline.
  _enc: The encoding context.*/
void oc_fdct_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc){
  _stage->enc=_enc;
  _stage->next=NULL;
  _stage->pipe_start=oc_fdct_pipe_start;
  _stage->pipe_proc=oc_fdct_pipe_process;
  _stage->pipe_end=oc_fdct_pipe_end;
}
