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


/****************************************************************************
*
*   Module Title :     FDct.c
*
*   Description  :     Video CODEC: 8x8 DCT Implementation.
*
*
*****************************************************************************
*/

#include "dct.h"

// Data pointers


/****************************************************************************
 * 
 *  ROUTINE       :     fdct_slow
 *
 *  INPUTS        :     INT32 *   Interger input data
 *
 *  OUTPUTS       :     double *  Floating point outputs    
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Performs an fdct.
 *
 *                      The algorithm used is derived from the 
 *                      flowgraph for the Vetterli and Lightenberg
 *                      fast 1-D dct given in the JPEG reference book by
 *                      Pennebaker and Mitchell.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

#define MULTS
#ifdef MULTS
#define C4S4MULT(X) (INT32)( (X) * C4S4 )
#define C6S2MULT(X) (INT32)( (X) * C6S2 )
#define C2S6MULT(X) (INT32)( (X) * C2S6 )
#define C1S7MULT(X) (INT32)( (X) * C1S7 )
#define C7S1MULT(X) (INT32)( (X) * C7S1 )
#define C3S5MULT(X) (INT32)( (X) * C3S5 )
#define C5S3MULT(X) (INT32)( (X) * C5S3 )
#else
#define C4S4MULT(X) C4S4_TablePtr[X]
#define C6S2MULT(X) C6S2_TablePtr[X]
#define C2S6MULT(X) C2S6_TablePtr[X]
#define C1S7MULT(X) C1S7_TablePtr[X]
#define C7S1MULT(X) C7S1_TablePtr[X]
#define C3S5MULT(X) C3S5_TablePtr[X]
#define C5S3MULT(X) C5S3_TablePtr[X]
#endif

#define PI 3.1415926535897
static INT32 xC1S7 = 64277;
static INT32 xC2S6 = 60547;
static INT32 xC3S5 = 54491;
static INT32 xC4S4 = 46341;
static INT32 xC5S3 = 36410;
static INT32 xC6S2 = 25080;
static INT32 xC7S1 = 12785;

static INT32 xSUM17=77062;
static INT32 xDIF17=51492;
static INT32 xSUM35=90901;
static INT32 xDIF35=18081;
static INT32 xSUM26=85637;
static INT32 xDIF26=35467;



#define SIGNBITDUPPED(X) ((signed )((X & 0x80000000)) >> 31)
#define DOROUND(X) X = ( (SIGNBITDUPPED(X) & (0xffff)) + X ); 

///////////////////////////////////////////////////////////////////
// A short version that  matches the floating point version above//
///////////////////////////////////////////////////////////////////
/* 
   In the floating point version above, there are lots of implicit 
   type casting from double to integer, resulting _ftol calls take
   up 36 percent of the compress time. To address this problem, we
   try to match the above function here with integer operations.
*/
void fdct_short ( INT16 * InputData, INT16 * OutputData )
{
	int loop;
	
	INT32  is07, is12, is34, is56;
	INT32  is0734, is1256;
	INT32  id07, id12, id34, id56; 

	INT32  irot_input_x, irot_input_y;
	INT32  icommon_product1;   // Re-used product  (c4s4 * (s12 - s56)). 
	INT32  icommon_product2;   // Re-used product  (c4s4 * (d12 + d56)).
	
	INT32  temp1, temp2;	   // intermediate variable for computation

	INT32  InterData[64];
	INT32 * ip = InterData;
	INT16 * op = OutputData;
	for (loop = 0; loop < 8; loop++)
	{
		// Pre calculate some common sums and differences.
		is07 = InputData[0] + InputData[7];
		is12 = InputData[1] + InputData[2];
		is34 = InputData[3] + InputData[4];
		is56 = InputData[5] + InputData[6];

		id07 = InputData[0] - InputData[7];
		id12 = InputData[1] - InputData[2];
		id34 = InputData[3] - InputData[4];
		id56 = InputData[5] - InputData[6];
	
		is0734 = is07 + is34;
		is1256 = is12 + is56;
		
		// Pre-Calculate some common product terms.
		icommon_product1 = xC4S4*(is12 - is56); 
		DOROUND(icommon_product1)
		icommon_product1>>=16;
		
		icommon_product2 = xC4S4*(id12 + id56);
		DOROUND(icommon_product2)
		icommon_product2>>=16;

	
		ip[0] = (xC4S4*(is0734 + is1256));
		DOROUND(ip[0]);
		ip[0] >>= 16;

		ip[4] = (xC4S4*(is0734 - is1256));
		DOROUND(ip[4]);
		ip[4] >>= 16;

		// Define inputs to rotation for outputs 2 and 6 
		irot_input_x = id12 - id56;
		irot_input_y = is07 - is34;

		// Apply rotation for outputs 2 and 6. 
		temp1=xC6S2*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC2S6*irot_input_y;
		DOROUND(temp2);
		temp2>>=16;
		ip[2] = temp1 + temp2;

		temp1=xC6S2*irot_input_y;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC2S6*irot_input_x ;
		DOROUND(temp2);
		temp2>>=16;
		ip[6] = temp1 -temp2 ;

		// Define inputs to rotation for outputs 1 and 7 
		irot_input_x = icommon_product1 + id07;
		irot_input_y = -( id34 + icommon_product2 );

		// Apply rotation for outputs 1 and 7. 

		temp1=xC1S7*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC7S1*irot_input_y;
		DOROUND(temp2);
		temp2>>=16;
		ip[1] = temp1 - temp2;

		temp1=xC7S1*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC1S7*irot_input_y ;
		DOROUND(temp2);
		temp2>>=16;
		ip[7] = temp1 + temp2 ;
		
		// Define inputs to rotation for outputs 3 and 5 
		irot_input_x = id07 - icommon_product1;
		irot_input_y = id34 - icommon_product2;

		// Apply rotation for outputs 3 and 5. 
		temp1=xC3S5*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC5S3*irot_input_y ;
		DOROUND(temp2);
		temp2>>=16;
		ip[3] = temp1 - temp2 ;


		temp1=xC5S3*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC3S5*irot_input_y;
		DOROUND(temp2);
		temp2>>=16;
		ip[5] = temp1 + temp2;
		
		// Increment data pointer for next row. 
		InputData += 8 ;
		ip += 8;		// advance pointer to next row 
		
	}


	//	Performed DCT on rows, now transform the columns	
	ip = InterData;
	for (loop = 0; loop < 8; loop++)
	{
		// Pre calculate some common sums and differences. 
		is07 = ip[0 * 8] + ip[7 * 8];
		is12 = ip[1 * 8] + ip[2 * 8];
		is34 = ip[3 * 8] + ip[4 * 8];
		is56 = ip[5 * 8] + ip[6 * 8];

		id07 = ip[0 * 8] - ip[7 * 8];
		id12 = ip[1 * 8] - ip[2 * 8];
		id34 = ip[3 * 8] - ip[4 * 8];
		id56 = ip[5 * 8] - ip[6 * 8];
	
		is0734 = is07 + is34;
		is1256 = is12 + is56;
		
		// Pre-Calculate some common product terms.
		icommon_product1 = xC4S4*(is12 - is56) ; 
		icommon_product2 = xC4S4*(id12 + id56) ;
		DOROUND(icommon_product1)
		DOROUND(icommon_product2)
		icommon_product1>>=16;
		icommon_product2>>=16;

		
		temp1 = xC4S4*(is0734 + is1256) ;
		temp2 = xC4S4*(is0734 - is1256) ;
		DOROUND(temp1);
		DOROUND(temp2);
		temp1>>=16;
		temp2>>=16;
		op[0*8] = (INT16) temp1;
		op[4*8] = (INT16) temp2;

		// Define inputs to rotation for outputs 2 and 6 
		irot_input_x = id12 - id56;
		irot_input_y = is07 - is34;

		// Apply rotation for outputs 2 and 6. 
		temp1=xC6S2*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC2S6*irot_input_y;
		DOROUND(temp2);
		temp2>>=16;
		op[2*8] = (INT16) (temp1 + temp2);

		temp1=xC6S2*irot_input_y;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC2S6*irot_input_x ;
		DOROUND(temp2);
		temp2>>=16;
		op[6*8] = (INT16) (temp1 -temp2) ;

		// Define inputs to rotation for outputs 1 and 7 
		irot_input_x = icommon_product1 + id07;
		irot_input_y = -( id34 + icommon_product2 );

		// Apply rotation for outputs 1 and 7. 
		temp1=xC1S7*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC7S1*irot_input_y;
		DOROUND(temp2);
		temp2>>=16;
		op[1*8] = (INT16) (temp1 - temp2);

		temp1=xC7S1*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC1S7*irot_input_y ;
		DOROUND(temp2);
		temp2>>=16;
		op[7*8] = (INT16) (temp1 + temp2);

		// Define inputs to rotation for outputs 3 and 5 
		irot_input_x = id07 - icommon_product1;
		irot_input_y = id34 - icommon_product2;

		// Apply rotation for outputs 3 and 5. 
		temp1=xC3S5*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC5S3*irot_input_y ;
		DOROUND(temp2);
		temp2>>=16;
		op[3*8] = (INT16) (temp1 - temp2) ;


		temp1=xC5S3*irot_input_x;
		DOROUND(temp1);
		temp1>>=16;
		temp2=xC3S5*irot_input_y;
		DOROUND(temp2);
		temp2>>=16;
		op[5*8] = (INT16) (temp1 + temp2);


		// Increment data pointer for next column. 
		ip ++;
		op ++;
	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     FDct1d
 *
 *  INPUTS        :     INT16 *  InputData
 *
 *  OUTPUTS       :     INT16 *  OoutputData    
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Performs an 1-d fdct.
 *
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void FDct1d ( INT16 * InputData, INT16 * OutputData )
{
	
	INT32  is07, is12, is34, is56;
	INT32  is0734, is1256;
	INT32  id07, id12, id34, id56; 

	INT32  irot_input_x, irot_input_y;
	INT32  icommon_product1;   // Re-used product  (c4s4 * (s12 - s56)). 
	INT32  icommon_product2;   // Re-used product  (c4s4 * (d12 + d56)).
	INT32  temp1, temp2;	   // intermediate variable for computation

	INT16 * op = OutputData;
	
	// Pre calculate some common sums and differences.
	is07 = InputData[0] + InputData[7];
	is12 = InputData[1] + InputData[2];
	is34 = InputData[3] + InputData[4];
	is56 = InputData[5] + InputData[6];
	
	id07 = InputData[0] - InputData[7];
	id12 = InputData[1] - InputData[2];
	id34 = InputData[3] - InputData[4];
	id56 = InputData[5] - InputData[6];
	
	is0734 = is07 + is34;
	is1256 = is12 + is56;
	
	// Pre-Calculate some common product terms.
	icommon_product1 = xC4S4*(is12 - is56); 
	DOROUND(icommon_product1)
		icommon_product1>>=16;
	
	icommon_product2 = xC4S4*(id12 + id56);
	DOROUND(icommon_product2)
		icommon_product2>>=16;
	
	
	temp1= (xC4S4*(is0734 + is1256));
	DOROUND(temp1);
	op[0] =(INT16) temp1>>16;
	
	temp2 = (xC4S4*(is0734 - is1256));
	DOROUND(temp2);
	op[4] =(INT16)temp2>>16;
	
	// Define inputs to rotation for outputs 2 and 6 
	irot_input_x = id12 - id56;
	irot_input_y = is07 - is34;
	
	// Apply rotation for outputs 2 and 6. 
	temp1=xC6S2*irot_input_x;
	DOROUND(temp1);
	temp1>>=16;
	temp2=xC2S6*irot_input_y;
	DOROUND(temp2);
	temp2>>=16;
	op[2] =(INT16)( temp1 + temp2);
	
	temp1=xC6S2*irot_input_y;
	DOROUND(temp1);
	temp1>>=16;
	temp2=xC2S6*irot_input_x ;
	DOROUND(temp2);
	temp2>>=16;
	op[6] =(INT16)( temp1 -temp2 );
	
	// Define inputs to rotation for outputs 1 and 7 
	irot_input_x = icommon_product1 + id07;
	irot_input_y = -( id34 + icommon_product2 );
	
	// Apply rotation for outputs 1 and 7. 
	
	temp1=xC1S7*irot_input_x;
	DOROUND(temp1);
	temp1>>=16;
	temp2=xC7S1*irot_input_y;
	DOROUND(temp2);
	temp2>>=16;
	op[1] = (INT16)(temp1 - temp2);
	
	temp1=xC7S1*irot_input_x;
	DOROUND(temp1);
	temp1>>=16;
	temp2=xC1S7*irot_input_y ;
	DOROUND(temp2);
	temp2>>=16;
	op[7] = (INT16)(temp1 + temp2 );
	
	// Define inputs to rotation for outputs 3 and 5 
	irot_input_x = id07 - icommon_product1;
	irot_input_y = id34 - icommon_product2;
	
	// Apply rotation for outputs 3 and 5. 
	temp1=xC3S5*irot_input_x;
	DOROUND(temp1);
	temp1>>=16;
	temp2=xC5S3*irot_input_y ;
	DOROUND(temp2);
	temp2>>=16;
	op[3] = (INT16)(temp1 - temp2 );
	
	
	temp1=xC5S3*irot_input_x;
	DOROUND(temp1);
	temp1>>=16;
	temp2=xC3S5*irot_input_y;
	DOROUND(temp2);
	temp2>>=16;
	op[5] = (INT16)(temp1 + temp2);
	
}


/****************************************************************************
 * 
 *  ROUTINE       :     FDct1d4
 *
 *  INPUTS        :     INT16 *  InputData
 *
 *  OUTPUTS       :     INT16 *  OoutputData    
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Performs 1-d fdct on four rows
 *
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void FDct1d4 ( INT16 * InputData, INT16 * OutputData )
{
	INT16 Input[8];
	INT16 Output[8];
	UINT32 i;

	for ( i = 0; i < 4 ; i++ )
	{
		Input[0] = InputData[ 0*4 + i];
		Input[1] = InputData[ 1*4 + i];
		Input[2] = InputData[ 2*4 + i];
		Input[3] = InputData[ 3*4 + i];
		Input[4] = InputData[ 4*4 + i];
		Input[5] = InputData[ 5*4 + i];
		Input[6] = InputData[ 6*4 + i];	
		Input[7] = InputData[ 7*4 + i];	
	
		FDct1d ( Input, Output );

		OutputData[ 0*4 + i] = Output[0];
		OutputData[ 1*4 + i] = Output[1];
		OutputData[ 2*4 + i] = Output[2];
		OutputData[ 3*4 + i] = Output[3];	
		OutputData[ 4*4 + i] = Output[4];	
		OutputData[ 5*4 + i] = Output[5];	
		OutputData[ 6*4 + i] = Output[6];	
		OutputData[ 7*4 + i] = Output[7];	
		
	}


}


