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
*   Module Title :     Quantise
*
*   Description  :     Quantisation and dequanitsation of an 8x8 dct block. .
*
*
*****************************************************************************
*/						

/****************************************************************************
*  Header Frames
*****************************************************************************
*/
#define STRICT              /* Strict type checking. */
#include <string.h>  
#include "pbdll.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/ 
       

// MIN_LEGAL_QUANT_ENTRY = (X * the dct normalisation factor(4))
// Designed to keep quantised values within the required number of bits

#define MIN_LEGAL_QUANT_ENTRY	8  
#define MIN_DEQUANT_VAL			2

// Scale factors used to improve precision of DCT/IDCT
#define IDCT_SCALE_FACTOR       2       // Shift left bits to improve IDCT precision
//#define DCT_SCALE_FACTOR        4.0   // Not used at the moment

#define OLD_SCHEME	1

/****************************************************************************
*  Imported Functions
*****************************************************************************
*/

/****************************************************************************
*  Imported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/
#ifdef COMPDLL
#include "compdll.h"
void init_quantizer ( CP_INSTANCE *cpi, UINT32 scale_factor, UINT8 QIndex );
#endif

void init_dequantizer ( PB_INSTANCE *pbi, UINT32 scale_factor, UINT8 QIndex );


/****************************************************************************
*  Foreward References
*****************************************************************************
*/    
          

/****************************************************************************
*  Module Statics
*****************************************************************************
*/      

// Table that relates quality index values to quantizer values
#ifdef PBDLL  	
UINT32 QThreshTableV1[Q_TABLE_SIZE] = 
{ 500,  450,  400,  370,  340,  310, 285, 265,
  245,  225,  210,  195,  185,  180, 170, 160, 
  150,  145,  135,  130,  125,  115, 110, 107,
  100,   96,   93,   89,   85,   82,  75,  74,
   70,   68,   64,   60,   57,   56,  52,  50,  
   49,   45,   44,   43,   40,   38,  37,  35,  
   33,   32,   30,   29,   28,   25,  24,  22,
   21,   19,   18,   17,   15,   13,  12,  10 };


Q_LIST_ENTRY DcScaleFactorTableV1[ Q_TABLE_SIZE ] = 
{ 220, 200, 190, 180, 170, 170, 160, 160,
  150, 150, 140, 140, 130, 130, 120, 120,
  110, 110, 100, 100, 90,  90,  90,  80,
  80,  80,  70,  70,  70,  60,  60,  60,
  60,  50,  50,  50,  50,  40,  40,  40,
  40,  40,  30,  30,  30,  30,  30,  30,
  30,  20,  20,  20,  20,  20,  20,  20,
  20,  10,  10,  10,  10,  10,  10,  10 
};


Q_LIST_ENTRY Y_coeffsV1[64] =
{	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55, 
    14,  13,  16,  24,  40,  57,  69,  56, 
	14,  17,  22,  29,  51,  87,  80,  62, 
	18,  22,  37,  58,  68, 109, 103,  77, 
	24,  35,  55,  64,  81, 104, 113,  92, 
	49,  64,  78,  87, 103, 121, 120, 101, 
 	72,  92,  95,  98, 112, 100, 103,  99
};

Q_LIST_ENTRY UV_coeffsV1[64] =
{	17,	18,	24,	47,	99,	99,	99,	99,
	18,	21,	26,	66,	99,	99,	99,	99,
	24,	26,	56,	99,	99,	99,	99,	99,
	47,	66,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99
};

// Different matrices for different encoder versions
Q_LIST_ENTRY Inter_coeffsV1[64] =
{   16,  16,  16,  20,  24,  28,  32,  40,
	16,  16,  20,  24,  28,  32,  40,  48, 
    16,  20,  24,  28,  32,  40,  48,  64, 
	20,  24,  28,  32,  40,  48,  64,  64, 
	24,  28,  32,  40,  48,  64,  64,  64, 
	28,  32,  40,  48,  64,  64,  64,  96, 
 	32,  40,  48,  64,  64,  64,  96,  128,
 	40,  48,  64,  64,  64,  96,  128, 128
};

#endif

#ifdef COMPDLL
extern UINT32 QThreshTableV1[Q_TABLE_SIZE] ; 
extern Q_LIST_ENTRY DcScaleFactorTableV1[ Q_TABLE_SIZE ] ; 
extern Q_LIST_ENTRY DcScaleFactorTableUV[ Q_TABLE_SIZE ] ; 
extern Q_LIST_ENTRY Y_coeffsV1[64] ;
extern Q_LIST_ENTRY UV_coeffsV1[64] ;
extern Q_LIST_ENTRY Inter_coeffsV1[64] ;
#endif 

/*	Inverse fast DCT index											*/
/*	This contains the offsets needed to convert zigzag order into	*/
/*	x, y order for decoding. It is generated from the input zigzag	*/
/*	indexat run time.												*/

/*	For maximum speed during both quantisation and dequantisation	*/
/*	we maintain separate quantisation and zigzag tables for each	*/
/*	operation.														*/

/*	pbi->quant_index:	the zigzag index used during quantisation		*/
/*	dequant_index:	zigzag index used during dequantisation			*/
/*					the pbi->quant_index is the inverse of dequant_index	*/
/*					and is calculated during initialisation			*/
/*	pbi->quant_Y_coeffs:	quantising coefficients used, corrected for		*/
/*					compression ratio and DCT algorithm-specific	*/
/*					scaling, for the Y plane						*/
/*	pbi->quant_UV_coeffs	similar for the UV planes						*/
/*	pbi->dequant_Y_coeffs similarly adjusted Y coefficients for			*/
/*					dequantisation, also reordered for zigzag		*/
/*					indexing										*/
/*	pbi->dequant_UV_coeffs ditto for UV planes							*/

#ifdef PBDLL  	
UINT32 dequant_index[64] = 
{	0,  1,  8,  16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};
#else
extern UINT32 dequant_index[64];
#endif 

#ifdef PBDLL
/****************************************************************************
 * 
 *  ROUTINE       :     InitQTables
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Initialises Q tables based upon version number
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void InitQTables( PB_INSTANCE *pbi )
{  
	// Make version specific assignments.
	memcpy ( pbi->QThreshTable, QThreshTableV1, sizeof( pbi->QThreshTable ) );
}

/****************************************************************************
 * 
 *  ROUTINE       :     BuildQuantIndex_Generic
 *
 *  INPUTS        :     
 *                      
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Builds the quant_index table.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void BuildQuantIndex_Generic(PB_INSTANCE *pbi)
{
    INT32 i,j;

    // invert the dequant index into the quant index
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
        j = dequant_index[i];
		pbi->quant_index[j] = i;
	}
}


/****************************************************************************
 * 
 *  ROUTINE       :     UpdateQ
 *
 *  INPUTS        :     UINT32  NewQ
 *                              (A New Q value (50 - 1000))
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Updates the quantisation tables for a new Q
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpdateQ( PB_INSTANCE *pbi, UINT32 NewQ )
{  
    UINT32 qscale;

	// Do bounds checking and convert to a float. 
	qscale = NewQ; 
	if ( qscale < pbi->QThreshTable[Q_TABLE_SIZE-1] )
		qscale = pbi->QThreshTable[Q_TABLE_SIZE-1];
	else if ( qscale > pbi->QThreshTable[0] )
	   qscale = pbi->QThreshTable[0];       

	// Set the inter/intra descision control variables.
	pbi->FrameQIndex = Q_TABLE_SIZE - 1;
	while ( (INT32) pbi->FrameQIndex >= 0 )
	{
		if ( (pbi->FrameQIndex == 0) || ( pbi->QThreshTable[pbi->FrameQIndex] >= NewQ) )
			break;
		pbi->FrameQIndex --;
	}

	// Re-initialise the q tables for forward and reverse transforms.    
	init_dequantizer ( pbi, qscale, (UINT8) pbi->FrameQIndex );
}
#endif 

/********************* COMPRESSOR SPECIFIC **********************************/

#ifdef COMPDLL
/****************************************************************************
 * 
 *  ROUTINE       :     UpdateQC (compressor's update q)
 *
 *  INPUTS        :     UINT32  NewQ
 *                              (A New Q value (50 - 1000))
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Updates the quantisation tables for a new Q
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpdateQC( CP_INSTANCE *cpi, UINT32 NewQ )
{  
    UINT32 qscale;

	PB_INSTANCE *pbi = &cpi->pb;

    // Do bounds checking and convert to a float. 
    qscale = NewQ; 
    if ( qscale < pbi->QThreshTable[Q_TABLE_SIZE-1] )
        qscale = pbi->QThreshTable[Q_TABLE_SIZE-1];
    else if ( qscale > pbi->QThreshTable[0] )
       qscale = pbi->QThreshTable[0];       

    // Set the inter/intra descision control variables.
    pbi->FrameQIndex = Q_TABLE_SIZE - 1;
    while ((INT32) pbi->FrameQIndex >= 0 )
    {
        if ( (pbi->FrameQIndex == 0) || ( pbi->QThreshTable[pbi->FrameQIndex] >= NewQ) )
            break;
        pbi->FrameQIndex --;
    }

    // Re-initialise the q tables for forward and reverse transforms.    
    init_quantizer ( cpi, qscale, (UINT8) pbi->FrameQIndex );
	init_dequantizer ( pbi, qscale, (UINT8) pbi->FrameQIndex );
}

/****************************************************************************
* 
*   Routine:	init_quantizer
*
*   Purpose:    Used to initialize the encoding/decoding data structures
*				and to select DCT algorithm	
*
*   Parameters :
*       Input :
*           UINT32          scale_factor
*                           Defines the factor by which to scale QUANT_ARRAY to
*                           produce quantization_array
*
*           UINT8           QIndex          :: 
*                           Index into Q table for current quantiser value.
*   Return value :
*       None.
*
****************************************************************************
*/
#define SHIFT16 (1<<16)
void init_quantizer ( CP_INSTANCE *cpi, UINT32 scale_factor, UINT8 QIndex )
{
    int i;                   // Loop counters
    double ZBinFactor;
    double RoundingFactor;

    double temp_fp_quant_coeffs;
    double temp_fp_quant_round;
    double temp_fp_ZeroBinSize;
	PB_INSTANCE *pbi = &cpi->pb;
	// Pointers to encoder/decoder specific tables
	Q_LIST_ENTRY * Inter_coeffs;	 
	Q_LIST_ENTRY * Y_coeffs;	 
	Q_LIST_ENTRY * UV_coeffs;	 
	Q_LIST_ENTRY * DcScaleFactorTable;
	Q_LIST_ENTRY * UVDcScaleFactorTable;

    // Notes on setup of quantisers.
    // The initial multiplication  by the scale factor is done in the INT32 domain 
    // to insure that the precision in the quantiser is the same as in the inverse 
    // quantiser where all calculations are integer. 
    // The "<< 2" is a normalisation factor for the forward DCT transform.

	// New version rounding and ZB characteristics.
	Inter_coeffs = Inter_coeffsV1;
	Y_coeffs = Y_coeffsV1;
	UV_coeffs = UV_coeffsV1;
	DcScaleFactorTable = DcScaleFactorTableV1;
	UVDcScaleFactorTable = DcScaleFactorTableV1;
	ZBinFactor = 0.9;

	switch(cpi->Sharpness)
	{
	case 0:
		ZBinFactor = 0.65;
		if ( scale_factor <= 50 )
			RoundingFactor = 0.499;
		else
			RoundingFactor = 0.46;
		break;
	case 1:
		ZBinFactor = 0.75;
		if ( scale_factor <= 50 )
			RoundingFactor = 0.476;
		else
			RoundingFactor = 0.400;
		break;

	default:
		ZBinFactor = 0.9;
		if ( scale_factor <= 50 )
			RoundingFactor = 0.476;
		else
			RoundingFactor = 0.333;
		break;
	}
			
    // Use fixed multiplier for intra Y DC
	temp_fp_quant_coeffs = (double)(((UINT32)(DcScaleFactorTable[QIndex] * Y_coeffs[0])/100) << 2);
	if ( temp_fp_quant_coeffs < MIN_LEGAL_QUANT_ENTRY * 2 )
		temp_fp_quant_coeffs = MIN_LEGAL_QUANT_ENTRY * 2;
	
	temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;
	pbi->fp_quant_Y_round[0]	= (INT32) (0.5 + temp_fp_quant_round);

    temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
	pbi->fp_ZeroBinSize_Y[0]	= (INT32) (0.5 + temp_fp_ZeroBinSize);
   
	temp_fp_quant_coeffs = 1.0 / temp_fp_quant_coeffs;
	pbi->fp_quant_Y_coeffs[0]	= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);
    
	// Intra UV
    temp_fp_quant_coeffs = (double)(((UINT32)(UVDcScaleFactorTable[QIndex] * UV_coeffs[0])/100) << 2);
	if ( temp_fp_quant_coeffs < MIN_LEGAL_QUANT_ENTRY * 2)
		temp_fp_quant_coeffs = MIN_LEGAL_QUANT_ENTRY * 2;

    temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;
	pbi->fp_quant_UV_round[0]	= (INT32) (0.5 + temp_fp_quant_round);

    temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
	pbi->fp_ZeroBinSize_UV[0]	= (INT32) (0.5 + temp_fp_ZeroBinSize);
    
	temp_fp_quant_coeffs = 1.0 / temp_fp_quant_coeffs;
	pbi->fp_quant_UV_coeffs[0]= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);
	
    // Inter Y
    temp_fp_quant_coeffs = (double)(((UINT32)(DcScaleFactorTable[QIndex] * Inter_coeffs[0])/100) << 2);
	if ( temp_fp_quant_coeffs < MIN_LEGAL_QUANT_ENTRY * 4)
		temp_fp_quant_coeffs = MIN_LEGAL_QUANT_ENTRY * 4;

	temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;
	pbi->fp_quant_Inter_round[0]= (INT32) (0.5 + temp_fp_quant_round);    

    temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
	pbi->fp_ZeroBinSize_Inter[0]= (INT32) (0.5 + temp_fp_ZeroBinSize);

	temp_fp_quant_coeffs= 1.0 / temp_fp_quant_coeffs;
	pbi->fp_quant_Inter_coeffs[0]= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);

    // Inter UV
    temp_fp_quant_coeffs = (double)(((UINT32)(UVDcScaleFactorTable[QIndex] * Inter_coeffs[0])/100) << 2);
	if ( temp_fp_quant_coeffs < MIN_LEGAL_QUANT_ENTRY * 4)
		temp_fp_quant_coeffs = MIN_LEGAL_QUANT_ENTRY * 4;

	temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;
	pbi->fp_quant_InterUV_round[0]= (INT32) (0.5 + temp_fp_quant_round);    

    temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
	pbi->fp_ZeroBinSize_InterUV[0]= (INT32) (0.5 + temp_fp_ZeroBinSize);

	temp_fp_quant_coeffs= 1.0 / temp_fp_quant_coeffs;
	pbi->fp_quant_InterUV_coeffs[0]= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);

    for ( i = 1; i < 64; i++ )
    {
		// now scale coefficients by required compression factor
		// Intra Y
		temp_fp_quant_coeffs =  (double)(((UINT32)(scale_factor * Y_coeffs[i]) / 100 ) << 2 );
		if ( temp_fp_quant_coeffs < (MIN_LEGAL_QUANT_ENTRY) )
			temp_fp_quant_coeffs = (MIN_LEGAL_QUANT_ENTRY);

		temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;
		pbi->fp_quant_Y_round[i]		= (INT32) (0.5 + temp_fp_quant_round);

        temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
		pbi->fp_ZeroBinSize_Y[i]		= (INT32) (0.5 + temp_fp_ZeroBinSize);

        temp_fp_quant_coeffs = 1.0 / temp_fp_quant_coeffs;
 		pbi->fp_quant_Y_coeffs[i]		= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);

        // Intra UV
        temp_fp_quant_coeffs =  (double)(((UINT32)(scale_factor * UV_coeffs[i]) / 100 ) << 2 );
        if ( temp_fp_quant_coeffs < (MIN_LEGAL_QUANT_ENTRY))
            temp_fp_quant_coeffs = (MIN_LEGAL_QUANT_ENTRY);

		temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;
		pbi->fp_quant_UV_round[i]		= (INT32) (0.5 + temp_fp_quant_round);

        temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
		pbi->fp_ZeroBinSize_UV[i]		= (INT32) (0.5 + temp_fp_ZeroBinSize);

        temp_fp_quant_coeffs = 1.0 / temp_fp_quant_coeffs;
		pbi->fp_quant_UV_coeffs[i]		= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);	

        // Inter Y
		temp_fp_quant_coeffs =  (double)(((UINT32)(scale_factor * Inter_coeffs[i]) / 100 ) << 2 );
		if ( temp_fp_quant_coeffs < (MIN_LEGAL_QUANT_ENTRY * 2) )
			temp_fp_quant_coeffs = (MIN_LEGAL_QUANT_ENTRY * 2);

        temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;    
		pbi->fp_quant_Inter_round[i]		= (INT32) (0.5 + temp_fp_quant_round);    
        
        temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
		pbi->fp_ZeroBinSize_Inter[i]		= (INT32) (0.5 + temp_fp_ZeroBinSize);

  		temp_fp_quant_coeffs = 1.0 / temp_fp_quant_coeffs;
		pbi->fp_quant_Inter_coeffs[i]	= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);

        // Inter UV
		temp_fp_quant_coeffs =  (double)(((UINT32)(scale_factor * Inter_coeffs[i]) / 100 ) << 2 );
		if ( temp_fp_quant_coeffs < (MIN_LEGAL_QUANT_ENTRY * 2) )
			temp_fp_quant_coeffs = (MIN_LEGAL_QUANT_ENTRY * 2);

        temp_fp_quant_round = temp_fp_quant_coeffs * RoundingFactor;    
		pbi->fp_quant_InterUV_round[i]		= (INT32) (0.5 + temp_fp_quant_round);    
        
        temp_fp_ZeroBinSize = temp_fp_quant_coeffs * ZBinFactor;
		pbi->fp_ZeroBinSize_InterUV[i]		= (INT32) (0.5 + temp_fp_ZeroBinSize);

  		temp_fp_quant_coeffs = 1.0 / temp_fp_quant_coeffs;
		pbi->fp_quant_InterUV_coeffs[i]	= (INT32) (0.5 + SHIFT16 * temp_fp_quant_coeffs);
	
	}


    pbi->fquant_coeffs = pbi->fp_quant_Y_coeffs;

}

/****************************************************************************/
/*																			*/
/*		Select Quantisation Parameters										*/
/*																			*/
/*		void select_Y_quantiser ( void )									*/
/*			sets quantiser to use for Intra Y
/*																			*/
/*		void select_Inter_quantiser ( void )								*/
/*			sets quantiser to use for Inter Y
/*																			*/
/*		void select_UV_quantiser ( void )									*/
/*			sets quantiser to use UV compression constants					*/
/*																			*/
/*		void select_InterUV_quantiser ( void )								*/
/*			sets quantiser to use for Inter UV
/*																			*/
/****************************************************************************/

void select_Y_quantiser ( PB_INSTANCE *pbi )
{	
    pbi->fquant_coeffs = pbi->fp_quant_Y_coeffs;
    pbi->fquant_round = pbi->fp_quant_Y_round;
    pbi->fquant_ZbSize = pbi->fp_ZeroBinSize_Y;
}

void select_Inter_quantiser ( PB_INSTANCE *pbi )
{	
    pbi->fquant_coeffs = pbi->fp_quant_Inter_coeffs;
    pbi->fquant_round = pbi->fp_quant_Inter_round;
    pbi->fquant_ZbSize = pbi->fp_ZeroBinSize_Inter;
}

void select_UV_quantiser ( PB_INSTANCE *pbi )
{	
    pbi->fquant_coeffs = pbi->fp_quant_UV_coeffs;
    pbi->fquant_round = pbi->fp_quant_UV_round;
    pbi->fquant_ZbSize = pbi->fp_quant_UV_round;
}

void select_InterUV_quantiser ( PB_INSTANCE *pbi )
{	
	pbi->fquant_coeffs = pbi->fp_quant_InterUV_coeffs;
	pbi->fquant_round = pbi->fp_quant_InterUV_round;
	pbi->fquant_ZbSize = pbi->fp_ZeroBinSize_InterUV;
}



/***************************************************************************
* 
*   Routine:    quantize
*
*   Purpose:    Quantizes a block of pixels by dividing 
*               each element by the corresponding entry in the quantization
*               array. Output is in a list of values in the zig-zag order.
*
*   Parameters :
*       Input :
*           DCT_block        -- The block to by quantized
*       Output :
*           quantized_list   -- The quantized values in zig-zag order
*
*   Return value :
*       None.
*
*   Persistent data referenced :
*       quantization_array   Module static array read
*       zig_zag_index        Module static array read
* 
****************************************************************************
*/

#define MIN16 ((1<<16)-1)
void quantize( PB_INSTANCE *pbi, INT16 * DCT_block, Q_LIST_ENTRY * quantized_list)
{
    UINT32   		i;              /*	Row index */
    Q_LIST_ENTRY    val;            /* Quantised value. */
    
    INT32 * FquantRoundPtr = pbi->fquant_round;
    INT32 * FquantCoeffsPtr = pbi->fquant_coeffs;
    INT32 * FquantZBinSizePtr = pbi->fquant_ZbSize;
    INT16 * DCT_blockPtr = DCT_block;
    UINT32 * QIndexPtr = (UINT32 *)pbi->quant_index;
	INT32 temp;
	int x = -7 >> 1;


    // Set the quantized_list to default to 0
    memset( quantized_list, 0, 64 * sizeof(Q_LIST_ENTRY) );

    // Note that we add half divisor to effect rounding on positive number 
    for( i = 0; i < pbi->Configuration.VFragPixels; i++)
    {
        // Column 0 
        if ( DCT_blockPtr[0] >= FquantZBinSizePtr[0] )
        {
			temp = FquantCoeffsPtr[0] * ( DCT_blockPtr[0] + FquantRoundPtr[0] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[0]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[0] <= -FquantZBinSizePtr[0] )
        {
			temp = FquantCoeffsPtr[0] * ( DCT_blockPtr[0] - FquantRoundPtr[0] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[0]] = ( val < -511 ) ? -511 : val;
        }

        // Column 1 
        if ( DCT_blockPtr[1] >= FquantZBinSizePtr[1] )
        {
			temp = FquantCoeffsPtr[1] * ( DCT_blockPtr[1] + FquantRoundPtr[1] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[1]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[1] <= -FquantZBinSizePtr[1] )
        {
			temp = FquantCoeffsPtr[1] * ( DCT_blockPtr[1] - FquantRoundPtr[1] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[1]] = ( val < -511 ) ? -511 : val;
        }

        // Column 2 
        if ( DCT_blockPtr[2] >= FquantZBinSizePtr[2] )
        {
			temp = FquantCoeffsPtr[2] * ( DCT_blockPtr[2] + FquantRoundPtr[2] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[2]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[2] <= -FquantZBinSizePtr[2] )
        {
			temp = FquantCoeffsPtr[2] * ( DCT_blockPtr[2] - FquantRoundPtr[2] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[2]] = ( val < -511 ) ? -511 : val;
        }

        // Column 3 
        if ( DCT_blockPtr[3] >= FquantZBinSizePtr[3] )
        {
			temp = FquantCoeffsPtr[3] * ( DCT_blockPtr[3] + FquantRoundPtr[3] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[3]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[3] <= -FquantZBinSizePtr[3] )
        {
			temp = FquantCoeffsPtr[3] * ( DCT_blockPtr[3] - FquantRoundPtr[3] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[3]] = ( val < -511 ) ? -511 : val;
        }

        // Column 4 
        if ( DCT_blockPtr[4] >= FquantZBinSizePtr[4] )
        {
			temp = FquantCoeffsPtr[4] * ( DCT_blockPtr[4] + FquantRoundPtr[4] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[4]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[4] <= -FquantZBinSizePtr[4] )
        {
			temp = FquantCoeffsPtr[4] * ( DCT_blockPtr[4] - FquantRoundPtr[4] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[4]] = ( val < -511 ) ? -511 : val;
        }

        // Column 5 
        if ( DCT_blockPtr[5] >= FquantZBinSizePtr[5] )
        {
			temp = FquantCoeffsPtr[5] * ( DCT_blockPtr[5] + FquantRoundPtr[5] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[5]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[5] <= -FquantZBinSizePtr[5] )
        {
			temp = FquantCoeffsPtr[5] * ( DCT_blockPtr[5] - FquantRoundPtr[5] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[5]] = ( val < -511 ) ? -511 : val;
        }

        // Column 6 
        if ( DCT_blockPtr[6] >= FquantZBinSizePtr[6] )
        {
			temp = FquantCoeffsPtr[6] * ( DCT_blockPtr[6] + FquantRoundPtr[6] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[6]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[6] <= -FquantZBinSizePtr[6] )
        {
			temp = FquantCoeffsPtr[6] * ( DCT_blockPtr[6] - FquantRoundPtr[6] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[6]] = ( val < -511 ) ? -511 : val;
        }

        // Column 7 
        if ( DCT_blockPtr[7] >= FquantZBinSizePtr[7] )
        {
			temp = FquantCoeffsPtr[7] * ( DCT_blockPtr[7] + FquantRoundPtr[7] ) ;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[7]] = ( val > 511 ) ? 511 : val;
        }
        else if ( DCT_blockPtr[7] <= -FquantZBinSizePtr[7] )
        {
			temp = FquantCoeffsPtr[7] * ( DCT_blockPtr[7] - FquantRoundPtr[7] ) + MIN16;
			val = (Q_LIST_ENTRY) (temp>>16);
            quantized_list[QIndexPtr[7]] = ( val < -511 ) ? -511 : val;
        }

        FquantRoundPtr += 8;
        FquantCoeffsPtr += 8;
        FquantZBinSizePtr += 8;
        DCT_blockPtr += 8;
        QIndexPtr += 8;
    }
}

#endif
/**************************** END COMPRESSOR SPECIFIC **********************************/
/***************************************************************************************
*  Dequantiser code for decode loop
/***************************************************************************************/
#ifdef PBDLL

/****************************************************************************
* 
*   Routine:	init_pbi->dequantizer
*
*   Purpose:    Used to initialize the encoding/decoding data structures
*				and to select DCT algorithm	
*
*   Parameters :
*       Input :
*           UINT32          scale_factor
*                           Defines the factor by which to scale QUANT_ARRAY to
*                           produce quantization_array
*
*           UINT8           QIndex          :: 
*                           Index into Q table for current quantiser value.
*   Return value :
*       None.
*
****************************************************************************
*/

void init_dequantizer ( PB_INSTANCE *pbi, UINT32 scale_factor, UINT8 QIndex )
{
    int i, j;						 // Loop counter 
	
	// Used for decoder version specific tables
	Q_LIST_ENTRY * Inter_coeffs;	 
	Q_LIST_ENTRY * Y_coeffs;	 
	Q_LIST_ENTRY * UV_coeffs;	 
	Q_LIST_ENTRY * DcScaleFactorTable;
	Q_LIST_ENTRY * UVDcScaleFactorTable;

	// Decoder specific selections
	Inter_coeffs = Inter_coeffsV1;
	Y_coeffs = Y_coeffsV1;
	UV_coeffs = UV_coeffsV1;
	DcScaleFactorTable = DcScaleFactorTableV1;
	UVDcScaleFactorTable = DcScaleFactorTableV1;

	// invert the dequant index into the quant index
    // the dxer has a different order than the cxer.
    pbi->BuildQuantIndex(pbi);

	// Reorder dequantisation coefficients into dct zigzag order.
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
        j = pbi->quant_index[i];
		pbi->dequant_Y_coeffs[j] = Y_coeffs[i];
	}
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
		j = pbi->quant_index[i];
		pbi->dequant_Inter_coeffs[j] = Inter_coeffs[i];
	}
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
        j = pbi->quant_index[i];
		pbi->dequant_UV_coeffs[j] = UV_coeffs[i];
	}
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
		j = pbi->quant_index[i];
		pbi->dequant_InterUV_coeffs[j] = Inter_coeffs[i];
	}

    // Intra Y
    pbi->dequant_Y_coeffs[0] = (Q_LIST_ENTRY)((DcScaleFactorTable[QIndex] * pbi->dequant_Y_coeffs[0])/100);
    if ( pbi->dequant_Y_coeffs[0] < MIN_DEQUANT_VAL * 2 )
        pbi->dequant_Y_coeffs[0] = MIN_DEQUANT_VAL * 2;
    pbi->dequant_Y_coeffs[0] = pbi->dequant_Y_coeffs[0] << IDCT_SCALE_FACTOR;

    // Intra UV
    pbi->dequant_UV_coeffs[0] = (Q_LIST_ENTRY)((UVDcScaleFactorTable[QIndex] * pbi->dequant_UV_coeffs[0])/100);
    if ( pbi->dequant_UV_coeffs[0] < MIN_DEQUANT_VAL * 2 )
        pbi->dequant_UV_coeffs[0] = MIN_DEQUANT_VAL * 2;
    pbi->dequant_UV_coeffs[0] = pbi->dequant_UV_coeffs[0] << IDCT_SCALE_FACTOR;

    // Inter Y
    pbi->dequant_Inter_coeffs[0] = (Q_LIST_ENTRY)((DcScaleFactorTable[QIndex] * pbi->dequant_Inter_coeffs[0])/100);
    if ( pbi->dequant_Inter_coeffs[0] < MIN_DEQUANT_VAL * 4 )
        pbi->dequant_Inter_coeffs[0] = MIN_DEQUANT_VAL * 4;
    pbi->dequant_Inter_coeffs[0] = pbi->dequant_Inter_coeffs[0] << IDCT_SCALE_FACTOR;

    // Inter UV
    pbi->dequant_InterUV_coeffs[0] = (Q_LIST_ENTRY)((UVDcScaleFactorTable[QIndex] * pbi->dequant_InterUV_coeffs[0])/100);
    if ( pbi->dequant_InterUV_coeffs[0] < MIN_DEQUANT_VAL * 4 )
        pbi->dequant_InterUV_coeffs[0] = MIN_DEQUANT_VAL * 4;
    pbi->dequant_InterUV_coeffs[0] = pbi->dequant_InterUV_coeffs[0] << IDCT_SCALE_FACTOR;

	for ( i = 1; i < 64; i++ )
	{	
		// now scale coefficients by required compression factor
		pbi->dequant_Y_coeffs[i] = (Q_LIST_ENTRY)(( scale_factor * pbi->dequant_Y_coeffs[i] ) / 100);
		if ( pbi->dequant_Y_coeffs[i] < MIN_DEQUANT_VAL )
			pbi->dequant_Y_coeffs[i] = MIN_DEQUANT_VAL;
		pbi->dequant_Y_coeffs[i] = pbi->dequant_Y_coeffs[i] << IDCT_SCALE_FACTOR;

		pbi->dequant_UV_coeffs[i] = (Q_LIST_ENTRY)(( scale_factor * pbi->dequant_UV_coeffs[i] ) / 100);
		if ( pbi->dequant_UV_coeffs[i] < MIN_DEQUANT_VAL )
			pbi->dequant_UV_coeffs[i] = MIN_DEQUANT_VAL;
		pbi->dequant_UV_coeffs[i] = pbi->dequant_UV_coeffs[i] << IDCT_SCALE_FACTOR;

		pbi->dequant_Inter_coeffs[i] = (Q_LIST_ENTRY)(( scale_factor * pbi->dequant_Inter_coeffs[i] ) / 100);
		if ( pbi->dequant_Inter_coeffs[i] < (MIN_DEQUANT_VAL * 2) )
			pbi->dequant_Inter_coeffs[i] = MIN_DEQUANT_VAL * 2;
		pbi->dequant_Inter_coeffs[i] = pbi->dequant_Inter_coeffs[i] << IDCT_SCALE_FACTOR;

		pbi->dequant_InterUV_coeffs[i] = (Q_LIST_ENTRY)(( scale_factor * pbi->dequant_InterUV_coeffs[i] ) / 100);
		if ( pbi->dequant_InterUV_coeffs[i] < (MIN_DEQUANT_VAL * 2) )
			pbi->dequant_InterUV_coeffs[i] = MIN_DEQUANT_VAL * 2;
		pbi->dequant_InterUV_coeffs[i] = pbi->dequant_InterUV_coeffs[i] << IDCT_SCALE_FACTOR;

    }

	pbi->dequant_coeffs = pbi->dequant_Y_coeffs;

}


/****************************************************************************/
/*																			*/
/*		Select Quantisation Parameters										*/
/*																			*/
/*		void select_Y_dequantiser ( void )									*/
/*			sets dequantiser to use for intra Y         					*/
/*																			*/
/*		void select_Inter_dequantiser ( void )									*/
/*			sets dequantiser to use for inter Y         					*/
/*																			*/
/*		void select_UV_dequantiser ( void )									*/
/*			sets dequantiser to use UV compression constants				*/
/*																			*/
/****************************************************************************/
void select_Y_dequantiser ( PB_INSTANCE *pbi)
{	
    pbi->dequant_coeffs = pbi->dequant_Y_coeffs;
}																			  

void select_Inter_dequantiser ( PB_INSTANCE *pbi )
{	
    pbi->dequant_coeffs = pbi->dequant_Inter_coeffs;
}

void select_UV_dequantiser ( PB_INSTANCE *pbi )
{	
    pbi->dequant_coeffs = pbi->dequant_UV_coeffs;
}


/****************************************************************************
* 
*   Routine:    dequant
*
*   Purpose:    The reverse of routine quantize, this routine takes a Q_LIST
*               list of quantized values and multipies by the relevant value in
*               quantization array. 
*
*   Parameters :  
*       Input :
*           quantized_list :: Q_LIST
*                      -- The quantized values in zig-zag order
*       Output :
*           DCT_block      :: INT16 *              
*                      -- The expanded values in a 2-D block
*
*   Return value :
*       None.
*
* 
****************************************************************************
*/
void dequant( PB_INSTANCE *pbi, Q_LIST_ENTRY * quantized_list, INT32 * DCT_block)
{
    // Loop fully expanded for maximum speed
    DCT_block[dequant_index[0]] = quantized_list[0] * pbi->dequant_coeffs[0];
    DCT_block[dequant_index[1]] = quantized_list[1] * pbi->dequant_coeffs[1];
    DCT_block[dequant_index[2]] = quantized_list[2] * pbi->dequant_coeffs[2];
    DCT_block[dequant_index[3]] = quantized_list[3] * pbi->dequant_coeffs[3];
    DCT_block[dequant_index[4]] = quantized_list[4] * pbi->dequant_coeffs[4];
    DCT_block[dequant_index[5]] = quantized_list[5] * pbi->dequant_coeffs[5];
    DCT_block[dequant_index[6]] = quantized_list[6] * pbi->dequant_coeffs[6];
    DCT_block[dequant_index[7]] = quantized_list[7] * pbi->dequant_coeffs[7];
    DCT_block[dequant_index[8]] = quantized_list[8] * pbi->dequant_coeffs[8];
    DCT_block[dequant_index[9]] = quantized_list[9] * pbi->dequant_coeffs[9];
    DCT_block[dequant_index[10]] = quantized_list[10] * pbi->dequant_coeffs[10];
    DCT_block[dequant_index[11]] = quantized_list[11] * pbi->dequant_coeffs[11];
    DCT_block[dequant_index[12]] = quantized_list[12] * pbi->dequant_coeffs[12];
    DCT_block[dequant_index[13]] = quantized_list[13] * pbi->dequant_coeffs[13];
    DCT_block[dequant_index[14]] = quantized_list[14] * pbi->dequant_coeffs[14];
    DCT_block[dequant_index[15]] = quantized_list[15] * pbi->dequant_coeffs[15];
    DCT_block[dequant_index[16]] = quantized_list[16] * pbi->dequant_coeffs[16];
    DCT_block[dequant_index[17]] = quantized_list[17] * pbi->dequant_coeffs[17];
    DCT_block[dequant_index[18]] = quantized_list[18] * pbi->dequant_coeffs[18];
    DCT_block[dequant_index[19]] = quantized_list[19] * pbi->dequant_coeffs[19];
    DCT_block[dequant_index[20]] = quantized_list[20] * pbi->dequant_coeffs[20];
    DCT_block[dequant_index[21]] = quantized_list[21] * pbi->dequant_coeffs[21];
    DCT_block[dequant_index[22]] = quantized_list[22] * pbi->dequant_coeffs[22];
    DCT_block[dequant_index[23]] = quantized_list[23] * pbi->dequant_coeffs[23];
    DCT_block[dequant_index[24]] = quantized_list[24] * pbi->dequant_coeffs[24];
    DCT_block[dequant_index[25]] = quantized_list[25] * pbi->dequant_coeffs[25];
    DCT_block[dequant_index[26]] = quantized_list[26] * pbi->dequant_coeffs[26];
    DCT_block[dequant_index[27]] = quantized_list[27] * pbi->dequant_coeffs[27];
    DCT_block[dequant_index[28]] = quantized_list[28] * pbi->dequant_coeffs[28];
    DCT_block[dequant_index[29]] = quantized_list[29] * pbi->dequant_coeffs[29];
    DCT_block[dequant_index[30]] = quantized_list[30] * pbi->dequant_coeffs[30];
    DCT_block[dequant_index[31]] = quantized_list[31] * pbi->dequant_coeffs[31];
    DCT_block[dequant_index[32]] = quantized_list[32] * pbi->dequant_coeffs[32];
    DCT_block[dequant_index[33]] = quantized_list[33] * pbi->dequant_coeffs[33];
    DCT_block[dequant_index[34]] = quantized_list[34] * pbi->dequant_coeffs[34];
    DCT_block[dequant_index[35]] = quantized_list[35] * pbi->dequant_coeffs[35];
    DCT_block[dequant_index[36]] = quantized_list[36] * pbi->dequant_coeffs[36];
    DCT_block[dequant_index[37]] = quantized_list[37] * pbi->dequant_coeffs[37];
    DCT_block[dequant_index[38]] = quantized_list[38] * pbi->dequant_coeffs[38];
    DCT_block[dequant_index[39]] = quantized_list[39] * pbi->dequant_coeffs[39];
    DCT_block[dequant_index[40]] = quantized_list[40] * pbi->dequant_coeffs[40];
    DCT_block[dequant_index[41]] = quantized_list[41] * pbi->dequant_coeffs[41];
    DCT_block[dequant_index[42]] = quantized_list[42] * pbi->dequant_coeffs[42];
    DCT_block[dequant_index[43]] = quantized_list[43] * pbi->dequant_coeffs[43];
    DCT_block[dequant_index[44]] = quantized_list[44] * pbi->dequant_coeffs[44];
    DCT_block[dequant_index[45]] = quantized_list[45] * pbi->dequant_coeffs[45];
    DCT_block[dequant_index[46]] = quantized_list[46] * pbi->dequant_coeffs[46];
    DCT_block[dequant_index[47]] = quantized_list[47] * pbi->dequant_coeffs[47];
    DCT_block[dequant_index[48]] = quantized_list[48] * pbi->dequant_coeffs[48];
    DCT_block[dequant_index[49]] = quantized_list[49] * pbi->dequant_coeffs[49];
    DCT_block[dequant_index[50]] = quantized_list[50] * pbi->dequant_coeffs[50];
    DCT_block[dequant_index[51]] = quantized_list[51] * pbi->dequant_coeffs[51];
    DCT_block[dequant_index[52]] = quantized_list[52] * pbi->dequant_coeffs[52];
    DCT_block[dequant_index[53]] = quantized_list[53] * pbi->dequant_coeffs[53];
    DCT_block[dequant_index[54]] = quantized_list[54] * pbi->dequant_coeffs[54];
    DCT_block[dequant_index[55]] = quantized_list[55] * pbi->dequant_coeffs[55];
    DCT_block[dequant_index[56]] = quantized_list[56] * pbi->dequant_coeffs[56];
    DCT_block[dequant_index[57]] = quantized_list[57] * pbi->dequant_coeffs[57];
    DCT_block[dequant_index[58]] = quantized_list[58] * pbi->dequant_coeffs[58];
    DCT_block[dequant_index[59]] = quantized_list[59] * pbi->dequant_coeffs[59];
    DCT_block[dequant_index[60]] = quantized_list[60] * pbi->dequant_coeffs[60];
    DCT_block[dequant_index[61]] = quantized_list[61] * pbi->dequant_coeffs[61];
    DCT_block[dequant_index[62]] = quantized_list[62] * pbi->dequant_coeffs[62];
    DCT_block[dequant_index[63]] = quantized_list[63] * pbi->dequant_coeffs[63];
}

#endif 