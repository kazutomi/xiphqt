/*Some common macros for potential platform-specific optimization.*/
#include <math.h>
#if !defined(_ocintrin_H)
# define _ocintrin_H (1)

/*Some specific platforms may have optimized intrinsic or inline assembly
   versions of these functions which can substantially improve performance.
  We define macros for them to allow easy incorporation of these non-ANSI
   features.*/

#define OC_MAXI(_a,_b)      ((_a)<(_b)?(_b):(_a))
#define OC_MINI(_a,_b)      ((_a)>(_b)?(_b):(_a))
#define OC_CLAMPI(_a,_b,_c) (OC_MAXI(_a,OC_MINI(_b,_c)))
/*Divides an integer by a power of two, rounding towards 0.
  _dividend: The integer to divide.
  _shift:    The non-negative power of two to divide by.
  _rmask:    (1<<_shift)-1*/
#define OC_DIV_POW2(_dividend,_shift,_rmask) \
 ((_dividend)+(((_dividend)>>sizeof(_dividend)*8-1)&(_rmask))>>(_shift))
/*Divides _x by 65536, truncating towards 0.*/
#define OC_DIV2_16(_x) OC_DIV_POW2(_x,16,0xFFFF)
/*Divides _x by 2, truncating towards 0.*/
#define OC_DIV2(_x) OC_DIV_POW2(_x,1,0x1)
/*Divides _x by 8, truncating towards 0.*/
#define OC_DIV8(_x) OC_DIV_POW2(_x,3,0x7)
/*Divides _x by 16, truncating towards 0.*/
#define OC_DIV16(_x) OC_DIV_POW2(_x,4,0xF)
#define OC_DIV_ROUND_POW2(_dividend,_shift,_rval) \
 ((_dividend)+((_dividend)>>sizeof(_dividend)*8-1)+(_rval)>>(_shift))
/*Swaps two integers _a and _b if _a>_b.*/
#define OC_SORT2I(_a,_b)\
  if((_a)>(_b)){\
    int t;\
    t=(_a);\
    (_a)=(_b);\
    (_b)=t;\
  }




/*All of these macros should expect floats as arguments.*/
#define OC_MAXF(_a,_b)      ((_a)<(_b)?(_b):(_a))
#define OC_MINF(_a,_b)      ((_a)>(_b)?(_b):(_a))
#define OC_CLAMPF(_a,_b,_c) (OC_MINF(_a,OC_MAXF(_b,_c)))
#define OC_FABSF(_f)        ((float)fabs(_f))
#define OC_SQRTF(_f)        ((float)sqrt(_f))
#define OC_POWF(_b,_e)      ((float)pow(_b,_e))
#define OC_LOGF(_f)         ((float)log(_f))
#define OC_IFLOORF(_f)      ((int)floor(_f))
#define OC_ICEILF(_f)       ((int)ceil(_f))

#endif
