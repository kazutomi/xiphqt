#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct{
  /*The Y'CbCr values corresponding to pure black.*/
  double ycbcr_offset[3];
  /*The extent of each Y'CbCr value.*/
  double ycbcr_scale[3];
  /*The coefficients of R'G'B's contribution to Y'.*/
  double ypbpr_coeffs[3];
  /*Output device gamma.*/
  /*Note: The recommended transfer function for the output device is used
     here, not the inverse of the transfer function of the camera.
    This means we use the model L=(E'+epsilon)^gamma' instead of inverting
     E'={alpha*L,L<=epsilon; beta*L^gamma-delta, L>epsilon}.
    Note gamma' is usually intentionally not defined as 1/gamma, to produce
     an expansion of contrast on the output device which is generally more
     pleasing.
    We want to use the gamma of the output device, since that is what people
     will actually see.*/
  double disp_gamma;
  /*Low-voltage offset.*/
  /*The epsilon here is just made up.
    It is used in output models to reduce instability near the black ranges,
     since real output devices do not have a linear response segment, and
     the slope of the power function is infinite at 0.*/
  double disp_gamma_eps;
  /*CIE xyz coordinates for red, green, blue and white.
    This is how manufacturers and standards describe RGB color spaces.
    If only x and y are given z=1-x-y.*/
  double chromaticity[4][3];
  /*The RGB to CIE XYZ conversion martix.
    This is derived from the chromaticity values.*/
  double rgb2xyz[3][3];
}colorspace;

/*ITU-R BT Rec. 470-6 System M (Usually NTSC).*/
colorspace ITUREC470M={
  {16,128,128},
  {219,224,224},
  {0.299,0.587,0.114},
  2.2,0.001,
  {
    {0.67,0.33,0},
    {0.21,0.71,0.08},
    {0.14,0.08,0.78},
    /*Illuminant C.*/
    {0.31006,0.31616,0.37378}
  }
};

/*ITU-R BT Rec. 470-6 System B,G (Most PALs, and SECAM with scaled inputs).*/
colorspace ITUREC470BG={
  {16,128,128},
  {219,224,224},
  {0.299,0.587,0.114},
  /*Rec 470 claims an assumed output gamma of 2.8, but this is too high to use
     in practice.
    The document claims the overall gamma is assumed to be approximately 1.2.
    They do not specify an input gamma value, but 0.45 is typical.
    The resulting gamma value is 2.666....*/
  1.2/0.45,0.001,
  {
    {0.64,0.33,0.03},
    {0.29,0.60,0.11},
    {0.15,0.06,0.79},
    /*D65 white point.*/
    {0.312713,0.329016,0.358271}
  }
};

/*CCIR/ITU-R BT/CIE Rec 709-5 (HDTV).*/
colorspace CIEREC709={
  {16,128,128},
  {219,224,224},
  {0.2126,0.7152,0.0722},
  /*Rec 709 does not specify a gamma for the output device.
    Only the gamma of the input device (0.45) is given.
    Typical CRTs have a gamma value of 2.5, which yields an overall gamma of
     1.125, within the 1.1 to 1.2 range usually used with the dim viewing
     environment assumed for television.*/
  2.5,0.001,
  {
    {0.64,0.33,0.03},
    {0.30,0.60,0.10},
    {0.15,0.06,0.79},
    /*D65 white point.*/
    {0.312713,0.329016,0.358271}
  }
};

void matrix_invert_3x3d(double _dst[3][3],double _src[3][3]){
  double t[3][3];
  double det;
  /*TODO: look for a more numerically stable method.*/
  t[0][0]=(_src[1][1]*_src[2][2]-_src[1][2]*_src[2][1]);
  t[1][0]=(_src[1][2]*_src[2][0]-_src[1][0]*_src[2][2]);
  t[2][0]=(_src[1][0]*_src[2][1]-_src[1][1]*_src[2][0]);
  t[0][1]=(_src[2][1]*_src[0][2]-_src[2][2]*_src[0][1]);
  t[1][1]=(_src[2][2]*_src[0][0]-_src[2][0]*_src[0][2]);
  t[2][1]=(_src[2][0]*_src[0][1]-_src[2][1]*_src[0][0]);
  t[0][2]=(_src[0][1]*_src[1][2]-_src[0][2]*_src[1][1]);
  t[1][2]=(_src[0][2]*_src[1][0]-_src[0][0]*_src[1][2]);
  t[2][2]=(_src[0][0]*_src[1][1]-_src[0][1]*_src[1][0]);
  det=_src[0][0]*t[0][0]+_src[0][1]*t[1][0]+_src[0][2]*t[2][0];
  _dst[0][0]=t[0][0]/det;
  _dst[0][1]=t[0][1]/det;
  _dst[0][2]=t[0][2]/det;
  _dst[1][0]=t[1][0]/det;
  _dst[1][1]=t[1][1]/det;
  _dst[1][2]=t[1][2]/det;
  _dst[2][0]=t[2][0]/det;
  _dst[2][1]=t[2][1]/det;
  _dst[2][2]=t[2][2]/det;
}

/*Convert the chromaticity values for this color space into a transformation
   from RGB to CIE XYZ.*/
void cs_fill_rgb2xyz(colorspace *_cs){
  double frgb2xyz[3][3];
  double irgb2xyz[3][3];
  double x_white;
  double z_white;
  double scale[3];
  int    i;
  for(i=0;i<3;i++){
    frgb2xyz[0][i]=_cs->chromaticity[i][0]/_cs->chromaticity[i][1];
    frgb2xyz[1][i]=1;
    frgb2xyz[2][i]=_cs->chromaticity[i][2]/_cs->chromaticity[i][1];
  }
  matrix_invert_3x3d(irgb2xyz,frgb2xyz);
  x_white=_cs->chromaticity[3][0]/_cs->chromaticity[3][1];
  z_white=_cs->chromaticity[3][2]/_cs->chromaticity[3][1];
  for(i=0;i<3;i++){
    scale[i]=irgb2xyz[i][0]*x_white+irgb2xyz[i][1]+irgb2xyz[i][2]*z_white;
  }
  for(i=0;i<3;i++){
    int j;
    for(j=0;j<3;j++){
      _cs->rgb2xyz[i][j]=frgb2xyz[i][j]*scale[j];
    }
  }
}

/*Conversion from XYZ to L*a*b with the given X and Z values of white
   (Y of white is assumed to be 1.0).*/
void xyz2lab(double _lab[3],const double _xyz[3],double _x_white,
 double _z_white){
  double x;
  double y;
  double z;
  double y3r;
  x=_xyz[0]/_x_white;
  if(x<=0.008856)x=7.787*x+16/116;
  z=_xyz[2]/_z_white;
  if(z<=0.008856)z=7.787*z+16/116;
  y=_xyz[1];
  if(y<=0.008856){
    y=7.787*y+16/116;
    _lab[0]=903.3*y;
    y3r=pow(y,1.0/3);
  }
  else{
    y3r=pow(y,1.0/3);
    _lab[0]=116*y3r-16;
  }
  _lab[1]=500*(pow(x,1.0/3)-y3r);
  _lab[2]=200*(y3r-pow(z,1.0/3));
}

/*Converts colors from some Y'CbCr space into the perceptually uniform L*a*b.
  Unforunately, due to the way these spaces are defined in the various
   standards, this requires going through a lot of intermediate color
   spaces.*/
int ycbcr2lab(const colorspace *_cs,double _lab[3],const double _ycbcr[3]){
  double ypbpr[3];
  double rgb[3];
  double xyz[3];
  int    i;
  /*Convert Y'CbCr to Y'PbPr.*/
  for(i=0;i<3;i++){
    ypbpr[i]=(_ycbcr[i]-_cs->ycbcr_offset[i])/_cs->ycbcr_scale[i];
  }
  ypbpr[1]*=2*(1-_cs->ypbpr_coeffs[2]);
  ypbpr[2]*=2*(1-_cs->ypbpr_coeffs[0]);
  /*Convert Y'PbPr to non-linear R'G'B'.*/
  rgb[0]=ypbpr[0]+ypbpr[2];
  rgb[1]=ypbpr[0]-(ypbpr[1]*_cs->ypbpr_coeffs[2]+
   ypbpr[2]*_cs->ypbpr_coeffs[0])/_cs->ypbpr_coeffs[1];
  rgb[2]=ypbpr[0]+ypbpr[1];
  /*Convert non-linear R'G'B' to linear RGB.*/
  for(i=0;i<3;i++){
    double e;
    if(rgb[i]<0||rgb[i]>1)return -1;
    e=rgb[i]+_cs->disp_gamma_eps;
    if(e<=0)e=0;
    else rgb[i]=pow(e,_cs->disp_gamma);
  }
  /*Convert linear RGB to CIE XYZ.*/
  for(i=0;i<3;i++){
    xyz[i]=_cs->rgb2xyz[i][0]*rgb[0]+_cs->rgb2xyz[i][1]*rgb[1]+
     _cs->rgb2xyz[i][2]*rgb[2];
  }
  /*Convert CIE XYZ to L*a*b*.*/
  xyz2lab(_lab,xyz,_cs->chromaticity[3][0]/_cs->chromaticity[3][1],
   _cs->chromaticity[3][2]/_cs->chromaticity[3][1]);
  return 0;
}

double dist2(double _a[3],double _b[3]){
  return sqrt((_a[0]-_b[0])*(_a[0]-_b[0])+(_a[1]-_b[1])*(_a[1]-_b[1])+
   (_a[2]-_b[2])*(_a[2]-_b[2]));
}

#define RES1    (8)
#define NSAMPS1 (256/RES1)
#define RES2    (1)
#define RANGE   (16)
#define NSAMPS2 (2*RANGE/RES2+1)

/*This is the level of a Just Noticable Difference in L*a*b space.
  We wish to map this to contrast level 1.
  L*a*b is not truly a uniform color space, but only an approximation.
  The Euclidean distance in that space yields JND values from 1 to 3.
  Under certain test conditions, the JND can be lowered to as much as 0.5.
  Newer equations like CIE94 attempt to derive local distortions of the space,
   but we don't go for that much precision.
  1.0 is probably a safe lower bound for video.*/
#define JND     (1.0)

/*Video is normally coded in some form of YUV color space.
  What exactly the Y, U and V means vary from system to system.
  Y is usually a non-linear (gamma corrected) luminance value, denoted Y' to
   indicate the non-linearity.
  U and V are usually differences between Y' and two of the primary colors,
   usually R' and B', called chrominance values, scaled and potentially offset.
  Y'CbCr is one example.
  I don't know why Cb and Cr don't get prime symbols, despite being the
   difference of two non-linear values.

  The reason for decomposing video into a luminance channel and two chrominance
   channels is that, for natural imagery, there is very little correlation
   between the channels, which yields good compression.
  The reason for using non-linear values is due to the non-linear nature of
   the light receptors in human eyes, which makes Y'CbCr spaces nearly
   perceptually uniform in each channel.

  The goal of this program is to compute the relative importance of each
   individual channel in the various Y'CbCr spaces, as well as locate the
   Just Noticeable Difference (JND) value for each channel.
  This is accomplished by converting to the device-independent and perceptually
   uniform CIE L*a*b color space, and measuring color distances there.

  A linear regression is then performed on the distance measurements, producing
   an offset and a slope to apply to each color channel such that the JND
   value is 1.
  This is an approximation to a perceptually uniform distance metric for the
   Y'CbCr space.
  Finally, equation is inverted and the JND value for each channel is
   computed.*/
int main(int _argc,char **_argv){
  static const int NEXT[3]={1,2,0};
  static double *de[3];
  static double *dc[3];
  colorspace *cs;
  double      ycbcr0[3];
  double      ycbcra[3];
  double      ycbcrb[3];
  double      ycbcr_jnd1[3];
  double      ycbcrs[3];
  double      ycbcr_jnd2[3];
  int         nsamps[3];
  int         csamps[3];
  int         c;
  if(_argc!=2){
    fprintf(stderr,"Specify a color space: \n"
     " Rec470M\n"
     " Rec470BG\n");
    exit(-1);
  }
  if(strcmp(_argv[1],"Rec470M")==0)cs=&ITUREC470M;
  else if(strcmp(_argv[1],"Rec470BG")==0)cs=&ITUREC470BG;
  else if(strcmp(_argv[1],"Rec709")==0)cs=&CIEREC709;
  else{
    fprintf(stderr,"Invalid color space.\n"
     "Run with no options for help.\n");
    exit(-1);
  }
  cs_fill_rgb2xyz(cs);
  /*Sample the color difference in L*a*b space over the range of Y'CbCr
     values.*/
  nsamps[0]=nsamps[1]=nsamps[2]=0;
  csamps[0]=csamps[1]=csamps[2]=0;
  for(ycbcr0[0]=0;ycbcr0[0]<256;ycbcr0[0]+=RES1){
    for(ycbcr0[1]=0;ycbcr0[1]<256;ycbcr0[1]+=RES1){
      for(ycbcr0[2]=0;ycbcr0[2]<256;ycbcr0[2]+=RES1){
        double lab0[3];
        double ycbcr1[3];
        if(ycbcr2lab(cs,lab0,ycbcr0)<0)continue;
        for(c=0;c<3;c++){
          ycbcr1[NEXT[c]]=ycbcr0[NEXT[c]];
          ycbcr1[NEXT[NEXT[c]]]=ycbcr0[NEXT[NEXT[c]]];
          ycbcr1[c]=ycbcr0[c]-RANGE;
          if(ycbcr1[c]<0)ycbcr1[c]=0;
          for(;ycbcr1[c]<256&&ycbcr1[c]<=ycbcr0[c]+RANGE;ycbcr1[c]+=RES2){
            double lab1[3];
            if(ycbcr2lab(cs,lab1,ycbcr1)<0)continue;
            if(nsamps[c]>=csamps[c]){
              double *nde;
              double *ndc;
              int ncsamps;
              ncsamps=csamps[c]<<1|1;
              nde=realloc(de[c],sizeof(double)*ncsamps);
              ndc=realloc(dc[c],sizeof(double)*ncsamps);
              if(nde==NULL||ndc==NULL){
                fprintf(stderr,"Out of memory.\n");
                exit(-1);
              }
              de[c]=nde;
              dc[c]=ndc;
              csamps[c]=ncsamps;
            }
            de[c][nsamps[c]]=dist2(lab0,lab1)/JND;
            dc[c][nsamps[c]]=fabs(ycbcr0[c]-ycbcr1[c]);
            /*printf("(%lf,%lf)\n",dc[c][nsamps[c]],de[c][nsamps[c]]);*/
            nsamps[c]++;
          }
        }
      }
    }
  }
  /*Compute a linear regression from the sampled errors.*/
  for(c=0;c<3;c++){
    double ysum;
    double xsum;
    double x2sum;
    double xysum;
    double d;
    int i;
    ysum=xsum=x2sum=xysum=0;
    for(i=0;i<nsamps[c];i++){
      ysum+=de[c][i];
      xsum+=dc[c][i];
      x2sum+=dc[c][i]*dc[c][i];
      xysum+=dc[c][i]*de[c][i];
    }
    d=nsamps[c]*x2sum-xsum*xsum;
    ycbcra[c]=(x2sum*ysum-xsum*xysum)/d;
    ycbcrs[c]=xysum/x2sum;
    ycbcr_jnd2[c]=1/ycbcrs[c];
    if(ycbcra[c]<0){
      ycbcra[c]=0;
      ycbcrb[c]=ycbcrs[c];
    }
    else ycbcrb[c]=(nsamps[c]*xysum-xsum*ysum)/d;
    ycbcr_jnd1[c]=(1-ycbcra[c])/ycbcrb[c];
  }
  /*Print the non-0-intercept regressions.*/
  /*Print the offsets.*/
  printf("{%0.23lf,%0.23lf,%0.23lf}\n",ycbcra[0],ycbcra[1],ycbcra[2]);
  /*And the slopes.*/
  printf("{%0.23lf,%0.23lf,%0.23lf}\n",ycbcrb[0],ycbcrb[1],ycbcrb[2]);
  printf("\n");
  /*Print the JND values.*/
  printf("{%0.23lf,%0.23lf,%0.23lf}\n",ycbcr_jnd1[0],ycbcr_jnd1[1],
   ycbcr_jnd1[2]);
  printf("\n\n");
  /*Print the 0-intercept regressions.*/
  /*Print the slopes of the 0-intercept regressions.*/
  printf("{%0.23lf,%0.23lf,%0.23lf}\n",ycbcrs[0],ycbcrs[1],ycbcrs[2]);
  printf("\n");
  /*Print the JND values.*/
  printf("{%0.23lf,%0.23lf,%0.23lf}\n",ycbcr_jnd2[0],ycbcr_jnd2[1],
   ycbcr_jnd2[2]);
  return 0;
}
