//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//--------------------------------------------------------------------------


#include <assert.h>
#include "ColorConversions.h"
#include "lutbl.h"
#include "ccstr.h"
#include <stdio.h>
#include "cpuidlib.h"
#include "asmcolorconversions.h"
#include "cclib.h"
#include "stdlib.h"

#ifdef _DEBUG
#include <windows.h>
#else
#define OutputDebugString
#endif

/*
 * constants needed for color conversion
 */
#define ScaleFactor      0x8000
#define ShiftFactor      15

#define PixelsPerBlock      4
#define PixelsPerBlockShift 2


/*
 * function prototypes
 */
void DefaultFunction( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
                      unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer );


/*
 * Global Function pointers
 * Once InitCCLib is called they should point to the fastest functions that are able
 * to run on the current machine
 */
void (*RGB32toYV12FuncPtr)( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight, 
                            unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer ) = DefaultFunction;

void (*RGB24toYV12FuncPtr)( unsigned char *RGBBuffer, int ImageWidth, int ImageHeight,
                            unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )= DefaultFunction;

void (*YVYUtoYV12FuncPtr)( unsigned char *YVYUBuffer, int ImageWidth, int ImageHeight,
                           unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer ) = DefaultFunction;

#ifdef _DEBUG
char DebugString[132];  // for outputtting debug information
#else
#endif _DEBUG

/*
 * **-DefaultFunction
 *
 * Our function pointers will by default be initilized to point to this function.  The purpose
 * of this function is to prevent us from going off into the weeds if the init function is not
 * called.
 *
 * Assumptions:
 *  None
 *
 * Input:
 *  Does not matter
 *
 * Output:
 *  If init function not called will force an error
 */
void DefaultFunction( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
                      unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
{
   char *CharPtr = 0;
   
#ifdef _DEBUG
   assert( 0 ); // InitCCLib MUST be called before using this function
#else
   // force error
   *CharPtr = 1;
#endif
}

/*
 * **-InitCCLib
 *
 * See cclib.h for a more detailed description of this function
 */
int InitCCLib( PROCTYPE CpuType )
{
   PROCTYPE DetectedCpuType;
   int ReturnValue = 0;  // assume will work

   if( CpuType == SpecialProc )
   {
      DetectedCpuType = findCPUId();
   }
   else
   {
      DetectedCpuType = CpuType;
   }

   switch( DetectedCpuType )
   {
      /*
       * The following processors supports the MMX instructions
       * point function pointers to MMX version of the functions
       */
      case PMMX:
      case PII:
      case AMDK63D:
      case AMDK6:
         RGB32toYV12FuncPtr = CC_RGB32toYV12_MMX;
         RGB24toYV12FuncPtr = CC_RGB24toYV12_MMX;
         YVYUtoYV12FuncPtr  = CC_YVYUtoYV12_MMX;
         break;

         
      /*
       * The following processors supports the XMM instructions
       * point function pointers to XMM version of the functions
       */
      case XMM:
	  case WMT:
         RGB32toYV12FuncPtr = CC_RGB32toYV12_XMM;
         RGB24toYV12FuncPtr = CC_RGB24toYV12_XMM;
         YVYUtoYV12FuncPtr  = CC_YVYUtoYV12_MMX;         
         break;

      /*
       * No know optimizations are avaliable for the following processors.
       * use the generic C version of the functions
       */
      case X86:
      case PPRO:
      case C6X86:
      case C6X86MX:
      case AMDK5:
      case MACG3:
      case MAC68K:
      default:
         RGB32toYV12FuncPtr = CC_RGB32toYV12_C;
         RGB24toYV12FuncPtr = CC_RGB24toYV12_C;
         YVYUtoYV12FuncPtr  = CC_YVYUtoYV12_C;
         break;
   }

   return( ReturnValue );
}

/*
 * **-DeInitCCLib
 *
 * See cclib.h for a more detailed description of this function
 */
void DeInitCCLib( void )
{
}

/*
 * **-CC_RGB32toYV12_C
 *
 * This function will convert a RGB32 buffer to planer YV12 output.
 * Alpha is ignored.
 *
 * See CCLIB.h for a more detailed description of this function
 *
 */
void CC_RGB32toYV12_C( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
                       unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
{
   unsigned char *YPtr1, *YPtr2;                        /* Local pointers to buffers */
   unsigned char *UPtr, *VPtr;
   RGB32Pixel    *RGBAPtr1, *RGBAPtr2;
   int           WidthCtr, HeightCtr;
   int           RSum, GSum, BSum;
   int           YScaled, UScaled, VScaled;             /* scaled Y, U, V values */
   int          *YRMultPtr;
   int          *YGMultPtr;
   int          *YBMultPtr;
   int          *URMultPtr;
   int          *UGMultPtr;
   int          *UBVRMultPtr;
   int          *VGMultPtr;
   int          *VBMultPtr;

   assert( !(ImageHeight & 0x1) ); /* Height must be even otherwise we will behave incorrectly */
   assert( !(ImageWidth & 0x1) );  /* Width must be even otherwise we will behave incorrectly */

   YRMultPtr   = YRMult;   
   YGMultPtr   = YGMult;   
   YBMultPtr   = YBMult;   
   URMultPtr   = URMult;   
   UGMultPtr   = UGMult;   
   UBVRMultPtr = UBVRMult; 
   VGMultPtr   = VGMult;   
   VBMultPtr   = VBMult;   

   /* setup pointers to first image line that we are processing */
   RGBAPtr1 = (RGB32Pixel *)RGBABuffer;
   YPtr1 = YBuffer;
   UPtr  = UBuffer;
   VPtr  = VBuffer;

   /* setup pointers to second image line that we are processing */
   RGBAPtr2 = ((RGB32Pixel *)RGBABuffer) + ImageWidth;
   YPtr2 = YBuffer + ImageWidth;

   for( HeightCtr = ImageHeight; HeightCtr != 0; HeightCtr-=2 )
   {
      for( WidthCtr = 0; WidthCtr < ImageWidth; WidthCtr += 2 )
      {
         /* process pixel (0,0) in our 2x2 block */
         YScaled = YRMultPtr[RGBAPtr1->Red];
         RSum = RGBAPtr1->Red;

         YScaled += YGMultPtr[RGBAPtr1->Green];
         GSum = RGBAPtr1->Green;

         YScaled += YBMultPtr[RGBAPtr1->Blue];
         BSum = RGBAPtr1->Blue;

         *YPtr1 = (unsigned char)(YScaled >> ShiftFactor); /* Y value valid write to output array */
         ++RGBAPtr1;
         ++YPtr1;
 
         /* process pixel (1,0) in our 2x2 block */
         YScaled = YRMultPtr[RGBAPtr2->Red];
         RSum += RGBAPtr2->Red;
         
         YScaled += YGMultPtr[RGBAPtr2->Green];
         GSum += RGBAPtr2->Green;

         YScaled += YBMultPtr[RGBAPtr2->Blue];
         BSum += RGBAPtr2->Blue;

         *YPtr2 = (unsigned char)(YScaled >> ShiftFactor); /* Y value valid write to output array */
         ++RGBAPtr2;
         ++YPtr2;

         /* process pixel (0,1) in our 2x2 block */
         YScaled = YRMultPtr[RGBAPtr1->Red];
         RSum += RGBAPtr1->Red;

         YScaled += YGMultPtr[RGBAPtr1->Green];
         GSum += RGBAPtr1->Green;

         YScaled += YBMultPtr[RGBAPtr1->Blue];
         BSum += RGBAPtr1->Blue;

         *YPtr1 = (unsigned char)(YScaled >> ShiftFactor); /* Y value valid write to output array */
         ++RGBAPtr1;
         ++YPtr1;

         /* process pixel (1,1) in our 2x2 block */
         YScaled = YRMultPtr[RGBAPtr2->Red];
         RSum += RGBAPtr2->Red;

         YScaled += YGMultPtr[RGBAPtr2->Green];
         GSum += RGBAPtr2->Green;

         YScaled += YBMultPtr[RGBAPtr2->Blue];
         BSum += RGBAPtr2->Blue;

         RSum += 2;												/* add in round factors */
         GSum += 2;
         BSum += 2;

         RSum = RSum >> 2;
         GSum = GSum >> 2;
         BSum = BSum >> 2;

         UScaled = URMultPtr[RSum];
         VScaled = UBVRMultPtr[RSum];

         UScaled += UGMultPtr[GSum];
         VScaled += VGMultPtr[GSum];

         UScaled += UBVRMultPtr[BSum];
         VScaled += VBMultPtr[BSum];

         UScaled = UScaled >> ShiftFactor;
         VScaled = VScaled >> ShiftFactor;

         *YPtr2 = (unsigned char)(YScaled >> ShiftFactor);       /* Y value valid write to output array */
         *UPtr  = (unsigned char)UScaled;
         *VPtr  = (unsigned char)VScaled;

         ++RGBAPtr2;
         ++YPtr2;
         ++UPtr;
         ++VPtr;
      }

      /* Increment our pointers */
      YPtr1    += ImageWidth;
      YPtr2    += ImageWidth;
      RGBAPtr1 += ImageWidth;
      RGBAPtr2 += ImageWidth;
   }
}

/*
 * **-CC_RGB24toYV12_C
 *
 * See cclib.h for a more detailed desciption of the function
 *
 */
void CC_RGB24toYV12_C( unsigned char *RGBBuffer, int ImageWidth, int ImageHeight,
                       unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
{
   unsigned char *YPtr1, *YPtr2;                        /* Local pointers to buffers */
   unsigned char *UPtr, *VPtr;
   RGB24Pixel    *RGBPtr1, *RGBPtr2;
   int           WidthCtr, HeightCtr;
   int           RSum, GSum, BSum;
   int           YScaled, UScaled, VScaled;             /* scaled Y, U, V values */
   int          *YRMultPtr;
   int          *YGMultPtr;
   int          *YBMultPtr;
   int          *URMultPtr;
   int          *UGMultPtr;
   int          *UBVRMultPtr;
   int          *VGMultPtr;
   int          *VBMultPtr;

   assert( !(ImageHeight & 0x1) ); /* Height must be even otherwise we will behave incorrectly */
   assert( !(ImageWidth & 0x1) );  /* Width must be odd otherwise we will behave incorrectly */

   YRMultPtr   = YRMult;   
   YGMultPtr   = YGMult;   
   YBMultPtr   = YBMult;   
   URMultPtr   = URMult;   
   UGMultPtr   = UGMult;   
   UBVRMultPtr = UBVRMult; 
   VGMultPtr   = VGMult;   
   VBMultPtr   = VBMult;   

   /* setup pointers to first image line that we are processing */
   RGBPtr1 = (RGB24Pixel *)RGBBuffer;
   YPtr1 = YBuffer;
   UPtr  = UBuffer;
   VPtr  = VBuffer;

   /* setup pointers to second image line that we are processing */
   RGBPtr2 = ((RGB24Pixel *)RGBBuffer) + ImageWidth;
   YPtr2 = YBuffer + ImageWidth;

   for( HeightCtr = ImageHeight; HeightCtr != 0; HeightCtr-=2 )
   {
      for( WidthCtr = 0; WidthCtr < ImageWidth; WidthCtr += 2 )
      {
         /* process pixel (0,0) in our 2x2 block */
         YScaled = YRMultPtr[RGBPtr1->Red];
         RSum = RGBPtr1->Red;

         YScaled += YGMultPtr[RGBPtr1->Green];
         GSum = RGBPtr1->Green;

         YScaled += YBMultPtr[RGBPtr1->Blue];
         BSum = RGBPtr1->Blue;

         *YPtr1 = (unsigned char)(YScaled >> ShiftFactor); /* Y value valid write to output array */
         ++RGBPtr1;
         ++YPtr1;
 
         /* process pixel (1,0) in our 2x2 block */
         YScaled = YRMultPtr[RGBPtr2->Red];
         RSum += RGBPtr2->Red;
         
         YScaled += YGMultPtr[RGBPtr2->Green];
         GSum += RGBPtr2->Green;

         YScaled += YBMultPtr[RGBPtr2->Blue];
         BSum += RGBPtr2->Blue;

         *YPtr2 = (unsigned char)(YScaled >> ShiftFactor); /* Y value valid write to output array */
         ++RGBPtr2;
         ++YPtr2;

         /* process pixel (0,1) in our 2x2 block */
         YScaled = YRMultPtr[RGBPtr1->Red];
         RSum += RGBPtr1->Red;

         YScaled += YGMultPtr[RGBPtr1->Green];
         GSum += RGBPtr1->Green;

         YScaled += YBMultPtr[RGBPtr1->Blue];
         BSum += RGBPtr1->Blue;

         *YPtr1 = (unsigned char)(YScaled >> ShiftFactor); /* Y value valid write to output array */
         ++RGBPtr1;
         ++YPtr1;

         /* process pixel (1,1) in our 2x2 block */
         YScaled = YRMultPtr[RGBPtr2->Red];
         RSum += RGBPtr2->Red;

         YScaled += YGMultPtr[RGBPtr2->Green];
         GSum += RGBPtr2->Green;

         YScaled += YBMultPtr[RGBPtr2->Blue];
         BSum += RGBPtr2->Blue;

         RSum += 2;                                               /* add in round factor */
         GSum += 2;
         BSum += 2;

         RSum = RSum >> 2;
         GSum = GSum >> 2;
         BSum = BSum >> 2;

         UScaled = URMultPtr[RSum];
         VScaled = UBVRMultPtr[RSum];

         UScaled += UGMultPtr[GSum];
         VScaled += VGMultPtr[GSum];

         UScaled += UBVRMultPtr[BSum];
         VScaled += VBMultPtr[BSum];

         UScaled = UScaled >> ShiftFactor;
         VScaled = VScaled >> ShiftFactor;

         *YPtr2 = (unsigned char)(YScaled >> ShiftFactor);       /* Y value valid write to output array */
         *UPtr  = (unsigned char)UScaled;
         *VPtr  = (unsigned char)VScaled;

         ++RGBPtr2;
         ++YPtr2;
         ++UPtr;
         ++VPtr;
      }

      /* Increment our pointers */
      YPtr1    += ImageWidth;
      YPtr2    += ImageWidth;
      RGBPtr1 += ImageWidth;
      RGBPtr2 += ImageWidth;
   }
}



/*
 * **-CC_YVYUtoYV12_C
 *
 * See CCLIB.H for a more detailed description of this function
 */
void CC_YVYUtoYV12_C( unsigned char *YVYUBuffer, int ImageWidth, int ImageHeight,
                      unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
{
   YVYUPixel *InputBuffer1, *InputBuffer2;
   unsigned char *YOutput1, *YOutput2;
   int HeightCtr, WidthCtr;
   int UAvg, VAvg;
   int HalfImageWidth = ImageWidth/2;

   assert( !(ImageHeight & 0x1) ); /* Height must be even otherwise we will behave incorrectly */
   assert( !(ImageWidth % 8) );    /* Width must be multiple of 8 */

   InputBuffer1 = (YVYUPixel *)YVYUBuffer;                   /* Point to first image line that we are processing */

   /*
    * Point to 2nd image line that we are processing by adding it HalfImageWidth.
    * need to add half and not whole image width because The YVYU structure actually
    * points to two pixels.
    */
   InputBuffer2 = ((YVYUPixel *)YVYUBuffer) + HalfImageWidth;
   YOutput1     = YBuffer;                                   /* Point to first Y output line */
   YOutput2     = YBuffer + ImageWidth;                      /* Point to 2nd Y output line */
   
   for( HeightCtr = ImageHeight; HeightCtr != 0; HeightCtr -= 2 )
   {
      for( WidthCtr = ImageWidth; WidthCtr != 0; WidthCtr -= 2 )
      {
         /* process pixel on first image line */
         *YOutput1 = InputBuffer1->Y0;                       /* copy first Y to output buffer */
         ++YOutput1;                                         /* Index to 2nd output Y */
         *YOutput1 = InputBuffer1->Y1;                       /* copy 2nd Y to output buffer */
         ++YOutput1;                                         /* Index to next output Y */

         UAvg = InputBuffer1->U;                             /* setup to average */
         VAvg = InputBuffer1->V;                             /* setup to average */
         
         ++InputBuffer1;                                     /* step to next input pixel */

         /* process pixel on 2nd image line */
         *YOutput2 = InputBuffer2->Y0;                       /* copy first Y to output buffer */
         ++YOutput2;                                         /* Index to 2nd output Y */
         *YOutput2 = InputBuffer2->Y1;                       /* copy 2nd Y to output buffer */
         ++YOutput2;                                         /* Index to next output Y */

         UAvg += InputBuffer2->U;                            /* setup to average */
         VAvg += InputBuffer2->V;                            /* setup to average */

         ++InputBuffer2;                                     /* set to next input pixel */

         /* average U and V and write to output */
         *UBuffer = UAvg >> 1;
         *VBuffer = VAvg >> 1;

         /* setp to next U, V output locations */
         ++UBuffer;
         ++VBuffer;
      }

      /* since we are processing two lines at a time need to step over a line already processed */
      InputBuffer1 += HalfImageWidth;
      InputBuffer2 += HalfImageWidth;
      YOutput1     += ImageWidth;
      YOutput2     += ImageWidth;
   }
}

void ConvertRGBtoYUV(
	unsigned char *r_src,unsigned char *g_src,unsigned char *b_src, 
	int width, int height, int rgb_step, int rgb_pitch,
	unsigned char *y_src, unsigned char *u_src, unsigned char *v_src,  
	int uv_width_shift, int uv_height_shift,
	int y_step, int y_pitch,int uv_step,int uv_pitch
	)
{

	int i,j,k,l,m,n,o,p;
	int y_scaled,u_scaled,v_scaled;

	// remember r,g,b
	unsigned char *r_ptr = r_src;
	unsigned char *g_ptr = g_src;
	unsigned char *b_ptr = b_src;
	unsigned char *r_ptr2 = r_src;
	unsigned char *g_ptr2 = g_src;
	unsigned char *b_ptr2 = b_src;

	int *YRMultPtr = YRMult;  
	int *YGMultPtr = YGMult;  
	int *YBMultPtr = YBMult;  
	int *URMultPtr = URMult;  
	int *UGMultPtr = UGMult;  
	int *UBVRMultPtr = UBVRMult;
	int *VGMultPtr = VGMult;
	int *VBMultPtr = VBMult;

	int uv_width_mask = (0xFFFFFFFF>>(32-uv_width_shift));
	int uv_height_mask = (0xFFFFFFFF>>(32-uv_height_shift));

	int uv_this_row,uv_this_col;
	int uv_width_scale = 1 << uv_width_shift, uv_height_scale = 1 << uv_height_shift;
	int uv_sum_shift = uv_width_shift + uv_height_shift;
	

	int avg_round = 1 << ( uv_sum_shift - 1 );
	int r_sum,g_sum,b_sum;

	j=height+1;

	
	// do y rows
	while(--j)
	{
		// we have a uv on this row if all of the bits lower than shift are set
		uv_this_row = (uv_height_mask & j)==uv_height_mask;

		// do y columns
		i = width+1;
		k = 0;
		l = 0;
		m = 0;
		while(--i)
		{

			// calculate 3 multiply parts of rgb -> to y 
			y_scaled = YRMultPtr[r_ptr[k]];	// includes round value for shift factor
			y_scaled += YGMultPtr[g_ptr[k]];
			y_scaled += YBMultPtr[b_ptr[k]];
	        y_src[l] = (unsigned char)(y_scaled >> ShiftFactor); 

			// we have a uv on this col if all of the bits lower than shift are set
			uv_this_col = (uv_width_mask & i)==uv_width_mask;

			// we now have a row and column on which we should calculate u and v values
			if(uv_this_row && uv_this_col)
			{
				r_sum = g_sum = b_sum = avg_round;

				r_ptr2 = r_ptr + k;
				g_ptr2 = g_ptr + k;
				b_ptr2 = b_ptr + k;

				// calculate r_sum, g_sum and b_sum for given u,v
				n = uv_height_scale + 1;
				while(--n)
				{
					o = uv_width_scale+1;
					p = 0;
					while(--o)
					{
						r_sum += r_ptr2[p];
						g_sum += g_ptr2[p];
						b_sum += b_ptr2[p];

						// step back one column
						p -= rgb_step;
					}

					// step back one line
					r_ptr2 -= rgb_pitch;
					g_ptr2 -= rgb_pitch;
					b_ptr2 -= rgb_pitch;

				}

				// calculate avg instead of just sum
				r_sum >>= uv_sum_shift;
				g_sum >>= uv_sum_shift;
				b_sum >>= uv_sum_shift;

				// convert rgb -> u,v
				u_scaled = URMultPtr[r_sum];
				v_scaled = UBVRMultPtr[r_sum];

				u_scaled += UGMultPtr[g_sum];
				v_scaled += VGMultPtr[g_sum];

				u_scaled += UBVRMultPtr[b_sum];
				v_scaled += VBMultPtr[b_sum];

				// convert from fixed point integer
				u_scaled >>= ShiftFactor;
				v_scaled >>= ShiftFactor;
				
				// save in result
				u_src[m] = (unsigned char) u_scaled;
				v_src[m] = (unsigned char) v_scaled;

				m+=uv_step;

			}

			k+=rgb_step;	// next rgb pixel
			l+=y_step;		// next y pixel

		}

		// we had uv's move to the next one
		if(uv_this_row)
		{
			u_src+=uv_pitch;
			v_src+=uv_pitch;
		}
		// next row
		r_ptr+=rgb_pitch;
		g_ptr+=rgb_pitch;
		b_ptr+=rgb_pitch;
		y_src+=y_pitch;
	}



}
