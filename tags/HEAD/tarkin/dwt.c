// $Id: dwt.c,v 1.1 2001/02/13 01:06:24 giles Exp $
//
// $Log: dwt.c,v $
// Revision 1.1  2001/02/13 01:06:24  giles
// Initial revision
//

#include <math.h>
#include "tarkin.h"

void daub4(float a[], unsigned long n, int isign)
{
  float *wksp;
  unsigned long nh, nh1, i, j;

  if(n < 4) return;
  wksp = (float *)malloc(sizeof(float) * n);
  nh1 = (nh=n>>1)+1;
  if(isign>=0) {
    for(i=1,j=1;j<=n-3;j+=2,i++) {
      wksp[i]=C0*a[j]+C1*a[j+1]+C2*a[j+2]+C3*a[j+3];
      wksp[i+nh]=C3*a[j]-C2*a[j+1]+C1*a[j+2]-C0*a[j+3];
    }
    wksp[i]=C0*a[n-1]+C1*a[n]+C2*a[1]+C3*a[2];
    wksp[i+nh]=C3*a[n-1]-C2*a[n]+C1*a[1]-C0*a[2];
  } else {
    wksp[1]=C2*a[nh]+C1*a[n]+C0*a[1]+C3*a[nh1];
    wksp[2]=C3*a[nh]-C0*a[n]+C1*a[1]-C2*a[nh1];
    for(i=1,j=3;i<nh;i++) {
      wksp[j++]=C2*a[i]+C1*a[i+nh]+C0*a[i+1]+C3*a[i+nh1];
      wksp[j++]=C3*a[i]-C0*a[i+nh]+C1*a[i+1]-C2*a[i+nh1];
    }
  }
  for(i=1;i<=n;i++) a[i]=wksp[i];
  free(wksp);
}

void wtn(float a[], unsigned long nn[], int ndim, int isign)
{
  unsigned long i1,i2,i3,k,n,nnew,nprev=1,nt,ntot=1;
  int idim;
  float *wksp;

  for(idim=1;idim<=ndim;idim++) ntot *= nn[idim];
  wksp=(float *)malloc(sizeof(float)*ntot);

  for(idim=1;idim<=ndim;idim++) {
    n = nn[idim];
    nnew = n * nprev;
    if(n > 4) {
      for(i2=0;i2<ntot;i2+=nnew) for(i1=1;i1<=nprev;i1++) {
        for(i3=i1+i2,k=1;k<=n;k++,i3+=nprev) wksp[k] = a[i3];
        if(isign >= 0) for(nt=n;nt>=4;nt>>=1) daub4(wksp,nt,isign);
        else for(nt=4;nt<=n;nt<<=1) daub4(wksp,nt,isign);
        for(i3=i1+i2,k=1;k<=n;k++,i3+=nprev) a[i3]=wksp[k];
      }
    }
    nprev = nnew;
  }
  free(wksp);
}

// Monodimensional DWT/iDWT transform
// if isign is positive - Forward DWT
// if isign is negative - Inverse DWT
//
// Originally snagged from Numeric Recipes in C (2nd Ed)
// Modified to get around braindeadedness with arrays starting at [1].

void zdaub4(float a[], unsigned long n, int isign)
{
  float *wksp;
  unsigned long nh, nh1, i, j;

  if(n < 4) return;
  wksp = (float *)malloc(sizeof(float) * n);

  nh  = n>>1;
  nh1 = nh+1;

  if(isign>=0) {
    for(i=0,j=0;j<n-3;j+=2,i++) {
      wksp[i]=C0*a[j]+C1*a[j+1]+C2*a[j+2]+C3*a[j+3];
      wksp[i+nh]=C3*a[j]-C2*a[j+1]+C1*a[j+2]-C0*a[j+3];
    }
    wksp[i]=C0*a[n-2]+C1*a[n-1]+C2*a[0]+C3*a[1];
    wksp[i+nh]=C3*a[n-2]-C2*a[n-1]+C1*a[0]-C0*a[1];
  } else {
    wksp[0]=C2*a[nh-1]+C1*a[n-1]+C0*a[0]+C3*a[nh];
    wksp[1]=C3*a[nh-1]-C0*a[n-1]+C1*a[0]-C2*a[nh];
    for(i=0,j=2;i<nh-1;i++,j+=2) {
      wksp[j]   = C2*a[i]+C1*a[i+nh]+C0*a[i+1]+C3*a[i+nh1];
      wksp[j+1] = C3*a[i]-C0*a[i+nh]+C1*a[i+1]-C2*a[i+nh1];
    }
  }
  for(i=0;i<n;i++) a[i]=wksp[i];
  free(wksp);
}

// Multidimensional DWT/iDWT transform
// a[]   - array of floating point values to be converted
// dim[] - dimensions of the a[] array
// ndim  - number of dimensions
// isign - if <0, perform iDWT, otherwise perform DWT
// 
// Dimensions *must* be powers of 2. Undefined but usually
// bad things happen if any of the dimensions are not.
//
// This routine will also work with more than 3 dimensions
// should the need arise to compress 3d motion images.

void dwt(float a[], unsigned long dim[], int ndim, int isign)
{
  unsigned long i1,i2,i3,k,n,nnew,nprev=1,nt,ntot=1;
  int idim;
  float *wksp;

  // Determine array extents and allocate workspace memory
  for(idim=0;idim<ndim;idim++) ntot *= dim[idim];
  wksp=(float *)malloc(sizeof(float)*ntot);

  // Perform monodimensional DWT/iDWT across float array,
  // One dimension at a time...
  for(idim=0;idim<ndim;idim++) {
    n = dim[idim];

    // 'nnew'  - length for 'line' progression
    // 'nprev' - length for individual element progression
    nnew = n * nprev;

    // No point in transforming if there's 4 or less samples...
    if(n > 4) {

      // Cycle through the entire array
      for(i2=0;i2<ntot;i2+=nnew) {
        for(i1=0;i1<nprev;i1++) {
          // Copy the appropriate elements into a monodimensional workspace...
          for(i3=i1+i2,k=0;k<n;k++,i3+=nprev) wksp[k] = a[i3];

          // And perform the DWT/iDWT in pyramid fashion
          if(isign >= 0) {
            for(nt=n;nt>=4;nt>>=1) zdaub4(wksp,nt,isign);
          } else {
            for(nt=4;nt<=n;nt<<=1) zdaub4(wksp,nt,isign);
          }

          // Copy the results back from the conversion
          for(i3=i1+i2,k=0;k<n;k++,i3+=nprev) a[i3]=wksp[k];
        }
      }
    }
    nprev = nnew;
  }
  free(wksp);
}


#ifdef DWTMAIN
int main() {
  float tf[64*64*16];
  int x, y, z, dim[3];
  ulong l;

  for(l=0;l<64*64*16;l++) tf[l] = l;

  dim[0] = 64;  
  dim[1] = 64;  
  dim[2] = 16;  

  dwt(tf, dim, 3, 1);
  for(l=0;l<64*64*16;l++) tf[l] = floor(tf[l]/2)*2;
  dwt(tf, dim, 3, -1);

  for(z=0;z<16;z++) for(y=0;y<64;y++) for(x=0;x<64;x++) {
    l = x+y*64+z*64*64;
    printf("%ld %f\n", l, tf[l]);
  }
}

#endif
