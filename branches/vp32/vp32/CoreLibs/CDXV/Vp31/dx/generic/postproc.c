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
 *   Module Title :     PostProc.c
 *
 *   Description  :     Post Processing
 *
 *
 *****************************************************************************
 */


/****************************************************************************
 *  Header Frames
 *****************************************************************************
 */

#if defined(POSTPROCESS)

#define STRICT              /* Strict type checking. */
#include <string.h>

#include "pbdll.h"
#include "blockmapping.h"
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 *  Module constants.
 *****************************************************************************
 */        
#ifdef _MSC_VER
#define abs(x) ((x>0)?(x):(-(x)))
#endif

#define MAX(a, b) ((a>b)?a:b)
#define MIN(a, b) ((a<b)?a:b)
#define Clamp(val)  ( val<0 ? 0: ( val>255 ? 255:val ) )
#define PP_QUALITY_THRESH   49

extern Q_LIST_ENTRY DcScaleFactorTableV1[ Q_TABLE_SIZE ] ; 

static UINT32 DCQuantScaleV1[ Q_TABLE_SIZE ] ;
static UINT32 DeringModifierV1[ Q_TABLE_SIZE ] ;

INT32 SharpenModifier[ Q_TABLE_SIZE ] =
{  -12, -11, -10, -10,  -9,  -9,  -9,  -9,
    -6,  -6,  -6,  -6,  -6,  -6,  -6,  -6, 
    -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
    -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
    -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0};
/*
INT32 SharpenModifier[ Q_TABLE_SIZE ] =
{   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0};
*/


unsigned char FragDeblockingFlag[7200];


/****************************************************************************
 *  Explicit Imports
 *****************************************************************************
 */              
extern void SimpleDeblockFrame(PB_INSTANCE *pbi, UINT8* SrcBuffer, UINT8* DestBuffer);
extern void SetupLoopFilter(PB_INSTANCE *pbi);
extern void UpdateUMVBorder( PB_INSTANCE *pbi, UINT8 * DestReconPtr );
extern UINT32 LoopFilterLimitValuesV2[];
extern UINT32 LoopFilterLimitValuesV1[];
extern void UpdateFragQIndex(PB_INSTANCE *pbi);



/****************************************************************************
 *  Exported Global Variables
 *****************************************************************************
 */

/****************************************************************************
 *  Exported Functions
 *****************************************************************************
 */              

/****************************************************************************
 *  Module Statics
 *****************************************************************************
 */
void DeblockVerticalEdgesInLoopFilteredBand(
                            PB_INSTANCE *pbi, 
                            UINT8 *SrcPtr, 
                            UINT8 *DesPtr,
                            UINT32 PlaneLineStep, 
                            UINT32 FragsAcross,
                            UINT32 StartFrag,
                            UINT32 *QuantScale
                            );

/****************************************************************************
 * 
 *  ROUTINE       :     InitPostProcessing
 *
 *  INPUTS        :     FrameQValue
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Setup static initialized variables for postprocessing
 *
 *  SPECIAL NOTES :     None
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void InitPostProcessing(void)
{
    int i;
    for( i = 0 ; i < Q_TABLE_SIZE; i++)
    {
        DCQuantScaleV1[i] = (5 + DcScaleFactorTableV1[i]) / 10;
        DeringModifierV1[i] = DCQuantScaleV1[i]; //LoopFilterLimitValuesV1[i] / 2;
    }


}



/****************************************************************************
 * 
 *  ROUTINE       :     DeringBlockStrong()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void DeringBlockStrong( 
                       PB_INSTANCE *pbi, 
                       UINT8 *SrcPtr,
                       UINT8 *DstPtr,
                       INT32 Pitch,
                       UINT32 FragQIndex,
                       UINT32 *QuantScale)
{
    
    short UDMod[72];
    short LRMod[72];
    unsigned int j,k;
    const unsigned char * Src = SrcPtr;
    unsigned int QValue = QuantScale[FragQIndex];

    unsigned char p           ;
    unsigned char pl          ;
    unsigned char pr          ;
    unsigned char pu          ;
    unsigned char pd          ;

    int  al          ;
    int  ar          ;
    int  au          ;
    int  ad          ;

    int  atot        ;
    int  B           ;
    int newVal      ;

    const unsigned char *curRow = SrcPtr;
    unsigned char *dstRow = DstPtr;
    const unsigned char *lastRow = SrcPtr-Pitch;
    const unsigned char *nextRow = SrcPtr+Pitch;
    

    unsigned int rowOffset = 0;
    unsigned int round = (1<<6);
    
    int High;
    int Low;
    int TmpMod;

    int Sharpen = SharpenModifier[FragQIndex];
    //Sharpen = 0;
	(void) pbi;
    Low = 0 - QValue;
    High = 3 * QValue;
    
    if(High>32)
        High=32;
    
    //if(Low < -24)
    //    Low = -24;
    Low = 0;

    
    /* Initialize the Mod Data */
    for(k=0;k<9;k++)
    {           
        for(j=0;j<8;j++)
        {
            
            TmpMod = 32 + QValue - (abs(Src[j]-Src[j-Pitch]));

            if(TmpMod< -64)
                TmpMod = Sharpen;

            else if(TmpMod<Low)
                TmpMod = Low;
            
            else if(TmpMod>High)
                TmpMod = High;
            
            UDMod[k*8+j] = (INT16)TmpMod;
        }
        Src +=Pitch;
    }

    Src = SrcPtr;

    for(k=0;k<8;k++)
    {           
        for(j=0;j<9;j++)
        {
            TmpMod = 32 + QValue - (abs(Src[j]-Src[j-1]));
            
            if(TmpMod< -64 )
                TmpMod = Sharpen;

            else if(TmpMod<0)
                TmpMod = Low;
            
            else if(TmpMod>High)
                TmpMod = High;

            LRMod[k*9+j] = (INT16)TmpMod;
        }
        Src+=Pitch;
    }
      
    for(k=0;k<8;k++)
    {
        // In the case that this function called with
        // same buffer for source and destination, To 
        // keep the c and the mmx version to have 
        // consistant results, intermediate buffer is 
        // used to store the eight pixel value before 
        // writing them to destination(i.e. Overwriting 
        // souce for the speical case)
        
        // column 0 
        int newPixel[8];

            atot = 128;
            B = round;
            p = curRow[ rowOffset +0];
            
            pl = curRow[ rowOffset +0-1];
            al = LRMod[k*9+0];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +0];
            au = UDMod[k*8+0];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +0];
            ad = UDMod[(k+1)*8+0];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +0+1];
            ar = LRMod[k*9+0+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[0] = Clamp( newVal );

            // column 1 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +1];
            
            pl = curRow[ rowOffset +1-1];
            al = LRMod[k*9+1];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +1];
            au = UDMod[k*8+1];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +1];
            ad = UDMod[(k+1)*8+1];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +1+1];
            ar = LRMod[k*9+1+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[1] = Clamp( newVal );
            
            // column 2 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +2];
            
            pl = curRow[ rowOffset +2-1];
            al = LRMod[k*9+2];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +2];
            au = UDMod[k*8+2];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +2];
            ad = UDMod[(k+1)*8+2];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +2+1];
            ar = LRMod[k*9+2+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[2] = Clamp( newVal );

            // column 3 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +3];
            
            pl = curRow[ rowOffset +3-1];
            al = LRMod[k*9+3];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +3];
            au = UDMod[k*8+3];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +3];
            ad = UDMod[(k+1)*8+3];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +3+1];
            ar = LRMod[k*9+3+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[3] = Clamp( newVal );


            // column 4 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +4];
            
            pl = curRow[ rowOffset +4-1];
            al = LRMod[k*9+4];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +4];
            au = UDMod[k*8+4];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +4];
            ad = UDMod[(k+1)*8+4];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +4+1];
            ar = LRMod[k*9+4+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[4] = Clamp( newVal );

            // column 5 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +5];
            
            pl = curRow[ rowOffset +5-1];
            al = LRMod[k*9+5];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +5];
            au = UDMod[k*8+5];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +5];
            ad = UDMod[(k+1)*8+5];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +5+1];
            ar = LRMod[k*9+5+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[5] = Clamp( newVal );
            
            // column 6 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +6];
            
            pl = curRow[ rowOffset +6-1];
            al = LRMod[k*9+6];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +6];
            au = UDMod[k*8+6];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +6];
            ad = UDMod[(k+1)*8+6];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +6+1];
            ar = LRMod[k*9+6+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[6] = Clamp( newVal );

            // column 7 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +7];
            
            pl = curRow[ rowOffset +7-1];
            al = LRMod[k*9+7];
            atot -= al;
            B += al * pl; 
            
            pu = lastRow[ rowOffset +7];
            au = UDMod[k*8+7];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +7];
            ad = UDMod[(k+1)*8+7];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +7+1];
            ar = LRMod[k*9+7+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            newPixel[7] = Clamp( newVal );

            dstRow[ rowOffset +0]= (INT8)newPixel[0];
            dstRow[ rowOffset +1]= (INT8)newPixel[1];
            dstRow[ rowOffset +2]= (INT8)newPixel[2];
            dstRow[ rowOffset +3]= (INT8)newPixel[3];
            dstRow[ rowOffset +4]= (INT8)newPixel[4];
            dstRow[ rowOffset +5]= (INT8)newPixel[5];
            dstRow[ rowOffset +6]= (INT8)newPixel[6];
            dstRow[ rowOffset +7]= (INT8)newPixel[7];
            
            rowOffset += Pitch;
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     DeringBlockWeak()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void DeringBlockWeak( 
                     PB_INSTANCE *pbi, 
                     UINT8 *SrcPtr,
                     UINT8 *DstPtr,
                     INT32 Pitch,
                     UINT32 FragQIndex,
                     UINT32 *QuantScale)
{

    short UDMod[72];
    short LRMod[72];
    unsigned int j,k;
    const unsigned char * Src = SrcPtr;
    unsigned int QValue = QuantScale[FragQIndex];

    unsigned char p           ;
    unsigned char pl          ;
    unsigned char pr          ;
    unsigned char pu          ;
    unsigned char pd          ;

    int  al          ;
    int  ar          ;
    int  au          ;
    int  ad          ;

    int  atot        ;
    int  B           ;
    int newVal      ;

    const unsigned char *curRow = SrcPtr;
    unsigned char *dstRow = DstPtr;
    const unsigned char *lastRow = SrcPtr-Pitch;
    const unsigned char *nextRow = SrcPtr+Pitch;
    

    unsigned int rowOffset = 0;
    unsigned int round = (1<<6);

    int High;
    int Low;
    int TmpMod;
    int Sharpen = SharpenModifier[FragQIndex];
    //Sharpen = 0;
    (void) pbi;

    Low = 0 - QValue;
    High = 3 * QValue;
    
    if(High>24)
        High=24;
    
    if(Low < -16)
        Low = -16;

    Low = 0 ;

    /* Initialize the Mod Data */
    for(k=0;k<9;k++)
    {           
        for(j=0;j<8;j++)
        {
            
            TmpMod = 32 + QValue - 2*(abs(Src[j]-Src[j-Pitch]));

            if(TmpMod< -64)
                TmpMod = Sharpen;

            else if(TmpMod<Low)
                TmpMod = Low;
            
            else if(TmpMod>High)
                TmpMod = High;
            
            UDMod[k*8+j] = (INT16)TmpMod;
        }
        Src +=Pitch;
    }

    Src = SrcPtr;

    for(k=0;k<8;k++)
    {           
        for(j=0;j<9;j++)
        {
            TmpMod = 32 + QValue - 2*(abs(Src[j]-Src[j-1]));
            
            if(TmpMod< -64 )
                TmpMod = Sharpen;

            else if(TmpMod<Low)
                TmpMod = Low;
            
            else if(TmpMod>High)
                TmpMod = High;

            LRMod[k*9+j] = (INT16)TmpMod;
        }
        Src+=Pitch;
    }

    for(k=0;k<8;k++)
    {
        // loop expanded for speed
        for(j=0;j<8;j++)
        {
            // column 0 
            atot = 128;
            B = round;
            p = curRow[ rowOffset +j];
            
            pl = curRow[ rowOffset +j-1];
            al = LRMod[k*9+j];
            atot -= al;
            B += al * pl;
            
            pu = lastRow[ rowOffset +j];
            au = UDMod[k*8+j];
            atot -= au;
            B += au * pu;
            
            pd = nextRow[ rowOffset +j];
            ad = UDMod[(k+1)*8+j];
            atot -= ad;
            B += ad * pd;
            
            pr = curRow[ rowOffset +j+1];
            ar = LRMod[k*9+j+1];
            atot -= ar;
            B += ar * pr;
            
            newVal = ( atot * p + B) >> 7;
            
            dstRow[ rowOffset +j] = (INT8) Clamp( newVal );
        }
        
        rowOffset += Pitch;
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     DeringBlock()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void DeringBlock( 
                 const PB_INSTANCE *pbi, 
                 const UINT8 *SrcPtr,
                 UINT8 *DstPtr,
                 const INT32 Pitch,
                 UINT32 FragQIndex,
                 const UINT32 *QuantScale,
                 UINT32 Variance)
{

    int N[8];   // neighbors
    unsigned int j,k,l;
    unsigned int QValue = QuantScale[FragQIndex];

    int  atot        ;
    int  B           ;
    int newVal      ;

    const unsigned char *srcRow = SrcPtr;
    unsigned char *dstRow = DstPtr;
    

    unsigned int round = (1<<7);

    int High;
    int Low;
    int TmpMod;
    int Sharpen = SharpenModifier[FragQIndex];

    int Slope = 4;

    if(pbi->PostProcessingLevel > 100)
    {
        QValue = pbi->PostProcessingLevel - 100;
    }

    
    //if(Variance > 262144 )
    //{
    //    Slope = 1;
    //}
    if ( Variance > 32768)
    {
        Slope = 4;
    }
    else if (Variance > 2048)
    {
        Slope = 8;
    }
            

    Low = 0 - QValue;
    High = 3 * QValue;
    
    if(High>32)
        High=32;
    
    if(Low < -16)
        Low = -16;

    Low = 0 ;

    for(k=0;k<8;k++)
    {
        // loop expanded for speed
        for(j=0;j<8;j++)
        {
            // set up 8 neighbors of pixel srcRow[j]
            N[0] = srcRow[j  -Pitch  -1]; 
            N[1] = srcRow[j  -Pitch    ]; 
            N[2] = srcRow[j  -Pitch  +1];
            N[3] = srcRow[j          -1];
            N[4] = srcRow[j          +1];
            N[5] = srcRow[j  +Pitch  -1];
            N[6] = srcRow[j  +Pitch    ];
            N[7] = srcRow[j  +Pitch  +1];

            // column 0 
            atot = 256;
            B = round;

            for(l = 0; l<8; l++)
            {
                TmpMod = 32 + QValue - (Slope *(abs(srcRow[j]-N[l])) >> 2);
                
                if(TmpMod< -64)
                    TmpMod = Sharpen;
                
                else if(TmpMod<Low)
                    TmpMod = Low;
                
                else if(TmpMod>High)
                    TmpMod = High;

                atot -= TmpMod;
                B += TmpMod * N[l];

            }
           
            newVal = ( atot * srcRow[j] + B) >> 8;
            
            dstRow[j] = (INT8) Clamp( newVal );
        }
        
        dstRow += Pitch;
        srcRow += Pitch;
    }
}


/****************************************************************************
 * 
 *  ROUTINE       :     DeringFrame
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering the frame for deringing purpose
 *
 *  SPECIAL NOTES :     Due to the changes in the deblocking stage, the 
 *                      variances are now actually the accumulated SAD
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void DeringFrame(PB_INSTANCE *pbi, UINT8 *Src, UINT8 *Dst)
{
    UINT32  col,row;
	UINT8  *SrcPtr;	            // Pointer to line of source image data
	UINT8  *DestPtr;            // Pointer to line of destination image data
	UINT32 BlocksAcross,BlocksDown;
	UINT32 *QuantScale;
	UINT32 Block;
	UINT32 LineLength;

	INT32 Thresh1,Thresh2,Thresh3,Thresh4;

	Thresh1 = 384;                  
	Thresh2 = 4 * Thresh1;          
	Thresh3 = 5 * Thresh2/4;        
    Thresh4 = 5 * Thresh2/2;        

    QuantScale = DeringModifierV1;

	BlocksAcross = pbi->HFragments;
	BlocksDown = pbi->VFragments;

	SrcPtr = Src + pbi->ReconYDataOffset;
	DestPtr = Dst + pbi->ReconYDataOffset;
	LineLength = pbi->Configuration.YStride;

	Block = 0;
	
	// dering the y plane 
	for ( row = 0 ; row < BlocksDown; row ++)
	{
		for (col = 0; col < BlocksAcross; col ++)
		{
			UINT32 Quality = pbi->FragQIndex[Block]; 
			INT32 Variance = pbi->FragmentVariances[Block]; 
			
			if( pbi->PostProcessingLevel >5 && Variance > Thresh3 )            
			{
				DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				
				if( (col > 0                && pbi->FragmentVariances[Block-1] > Thresh4 ) ||
					(col + 1 < BlocksAcross && pbi->FragmentVariances[Block+1] > Thresh4 ) ||
					(row + 1 < BlocksDown   && pbi->FragmentVariances[Block+BlocksAcross] > Thresh4) ||
					(row > 0                && pbi->FragmentVariances[Block-BlocksAcross] > Thresh4) )
				{
					DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
					DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				}
				
			}
			else if(Variance > Thresh2 )
			{
				DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
			}
			else if(Variance > Thresh1 )
			{
				DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
			}
			else
			{
				CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
			}
			
			++Block;
			
		}
		SrcPtr += 8 * LineLength;
		DestPtr += 8 * LineLength;
    }
    // Then U
	BlocksAcross /= 2;
	BlocksDown /= 2;
	LineLength /= 2;

	SrcPtr = Src + pbi->ReconUDataOffset;
	DestPtr = Dst + pbi->ReconUDataOffset;
	for ( row = 0 ; row < BlocksDown; row ++)
	{
		for (col = 0; col < BlocksAcross; col ++)
		{
			UINT32 Quality = pbi->FragQIndex[Block]; 
			INT32 Variance = pbi->FragmentVariances[Block]; 
			
			if( pbi->PostProcessingLevel >5 && Variance > Thresh4 )            
			{
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				
			}
			else if(Variance > Thresh2 )
			{
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
			}
			else if(Variance > Thresh1 )
			{
				DeringBlockWeak(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
			}
			else
			{
				CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
			}
			
			++Block;
			
		}
		SrcPtr += 8 * LineLength;
		DestPtr += 8 * LineLength;
    }

    // Then V
	SrcPtr = Src + pbi->ReconVDataOffset;
	DestPtr = Dst + pbi->ReconVDataOffset;

	for ( row = 0 ; row < BlocksDown; row ++)
	{
		for (col = 0; col < BlocksAcross; col ++)
		{
			UINT32 Quality = pbi->FragQIndex[Block]; 
			INT32 Variance = pbi->FragmentVariances[Block]; 
			
			
			if( pbi->PostProcessingLevel >5 && Variance > Thresh4 )            
			{
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
				
			}
			else if(Variance > Thresh2 )
			{
				DeringBlockStrong(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
			}
			else if(Variance > Thresh1 )
			{
				DeringBlockWeak(pbi,SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,Quality,QuantScale);
			}
			else
			{
				CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
			}
			
			++Block;
		
		}
		SrcPtr += 8 * LineLength;
		DestPtr += 8 * LineLength;

    }
    
}

/****************************************************************************
 * 
 *  ROUTINE       :     UpdateFragQIndex
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Update the QIndex for each updated frag
 *
 *  SPECIAL NOTES :     None
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpdateFragQIndex(PB_INSTANCE *pbi)
{

	UINT32  ThisFrameQIndex;	// Corresponding Index in the QThreshTable
	UINT32	i;

	// Check this frame quality  index
    ThisFrameQIndex = pbi->FrameQIndex;
	    

	// It is not a key frame, so only reset those are coded
    for( i = 0; i < pbi->UnitFragments; i++  )
    {
        if( pbi->display_fragments[i])
        {
            pbi->FragQIndex[i] = ThisFrameQIndex;
        }
    }

}


/****************************************************************************
 * 
 *  ROUTINE       :     DeblockLoopFilteredBand
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filter both horizontal and vertical edge in a band
 *
 *  SPECIAL NOTES :     
 *
 *  REFERENCE     :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 void DeblockLoopFilteredBand(
     PB_INSTANCE *pbi, 
     UINT8 *SrcPtr, 
     UINT8 *DesPtr,
     UINT32 PlaneLineStep, 
     UINT32 FragsAcross,
     UINT32 StartFrag,
     UINT32 *QuantScale
     )
 {
    UINT32 j,k;
    UINT32 CurrentFrag=StartFrag;
    INT32 QStep;
    INT32 FLimit;
    UINT8 *Src, *Des;
    INT32  x[10];
    INT32 pitch1, pitch2, pitch3, pitch4, pitch5;
    INT32  Sum1, Sum2;

    pitch1=PlaneLineStep;
    pitch2=PlaneLineStep * 2;
    pitch3=PlaneLineStep * 3;
    pitch4=PlaneLineStep * 4;
    pitch5=PlaneLineStep * 5;


    while(CurrentFrag < StartFrag + FragsAcross)
    {

        Src=SrcPtr+8*(CurrentFrag-StartFrag);
        Des=DesPtr+8*(CurrentFrag-StartFrag);

        {
        
            QStep = QuantScale[pbi->FragQIndex[CurrentFrag+FragsAcross]];
            FLimit = ( QStep * 3 ) >> 2;
            
            for( j=0; j<8 ; j++)
            {
                x[0] = Src[-pitch5];
                x[1] = Src[-pitch4];
                x[2] = Src[-pitch3];
                x[3] = Src[-pitch2];
                x[4] = Src[-pitch1];
                x[5] = Src[      0];
                x[6] = Src[+pitch1];
                x[7] = Src[+pitch2];
                x[8] = Src[+pitch3];
                x[9] = Src[+pitch4];


                Sum1=Sum2=0;
                
                for(k=1;k<=4;k++)
                {   
                    Sum1 += abs(x[k]-x[k-1]);
                    Sum2 += abs(x[k+4]-x[k+5]);           
                }
                
                pbi->FragmentVariances[CurrentFrag] +=((Sum1>255)?255:Sum1);
                pbi->FragmentVariances[CurrentFrag + FragsAcross] += ((Sum2>255)?255:Sum2);

                if( Sum1 < FLimit &&
                    Sum2 < FLimit &&
                    (x[5] - x[4]) < QStep && 
                    (x[4] - x[5]) < QStep )
                {
                    
                    // low pass filtering (LPF7: 1 1 1 2 1 1 1) 
                    Des[-pitch4] = (UINT8)((x[0] + x[0] +x[0] + x[1] * 2 + x[2] + x[3] +x[4] + 4) >> 3);
                    Des[-pitch3] = (UINT8)((x[0] + x[0] +x[1] + x[2] * 2 + x[3] + x[4] +x[5] + 4) >> 3);
                    Des[-pitch2] = (UINT8)((x[0] + x[1] +x[2] + x[3] * 2 + x[4] + x[5] +x[6] + 4) >> 3);
                    Des[-pitch1] = (UINT8)((x[1] + x[2] +x[3] + x[4] * 2 + x[5] + x[6] +x[7] + 4) >> 3);
                    Des[      0] = (UINT8)((x[2] + x[3] +x[4] + x[5] * 2 + x[6] + x[7] +x[8] + 4) >> 3);
                    Des[ pitch1] = (UINT8)((x[3] + x[4] +x[5] + x[6] * 2 + x[7] + x[8] +x[9] + 4) >> 3);
                    Des[ pitch2] = (UINT8)((x[4] + x[5] +x[6] + x[7] * 2 + x[8] + x[9] +x[9] + 4) >> 3);
                    Des[ pitch3] = (UINT8)((x[5] + x[6] +x[7] + x[8] * 2 + x[9] + x[9] +x[9] + 4) >> 3);
                    
                }
                else 
                {
                    //copy the pixels to destination
                    Des[-pitch4]= (UINT8)x[1];
                    Des[-pitch3]= (UINT8)x[2];
                    Des[-pitch2]= (UINT8)x[3];
                    Des[-pitch1]= (UINT8)x[4];
                    Des[      0]= (UINT8)x[5];
                    Des[+pitch1]= (UINT8)x[6];
                    Des[+pitch2]= (UINT8)x[7];
                    Des[+pitch3]= (UINT8)x[8];
                }
                Src ++;
                Des ++;             
            }

        }//if

        // done with filtering the horizontal edge, 
        // now let's do the vertical one
        // skip the first one
        if(CurrentFrag==StartFrag)
            CurrentFrag++;
        else
        {
            Des=DesPtr-8*PlaneLineStep+8*(CurrentFrag-StartFrag);
            Src=Des;
            
            QStep = QuantScale[pbi->FragQIndex[CurrentFrag]];
            FLimit = ( QStep * 3 ) >> 2;
            
            for( j=0; j<8 ; j++)
            {
                
                x[0] = Src[-5];
                x[1] = Src[-4];
                x[2] = Src[-3];
                x[3] = Src[-2];
                x[4] = Src[-1];
                x[5] = Src[0];
                x[6] = Src[+1];
                x[7] = Src[+2];
                x[8] = Src[+3];
                x[9] = Src[+4];
                
                Sum1=Sum2=0;
                
                for(k=1;k<=4;k++)
                {   
                    Sum1 += abs(x[k]-x[k-1]);
                    Sum2 += abs(x[k+4]-x[k+5]);           
                }

                pbi->FragmentVariances[CurrentFrag-1] += ((Sum1>255)?255:Sum1);
                pbi->FragmentVariances[CurrentFrag] += ((Sum2>255)?255:Sum2);
                
                if( Sum1 < FLimit &&
                    Sum2 < FLimit &&
                    (x[5] - x[4]) < QStep && 
                    (x[4] - x[5]) < QStep )
                {
                        
                    // low pass filtering (LPF7: 1 1 1 2 1 1 1) 
                    Des[-4] = (UINT8)((x[0] + x[0] +x[0] + x[1] * 2 + x[2] + x[3] +x[4] + 4) >> 3);
                    Des[-3] = (UINT8)((x[0] + x[0] +x[1] + x[2] * 2 + x[3] + x[4] +x[5] + 4) >> 3);
                    Des[-2] = (UINT8)((x[0] + x[1] +x[2] + x[3] * 2 + x[4] + x[5] +x[6] + 4) >> 3);
                    Des[-1] = (UINT8)((x[1] + x[2] +x[3] + x[4] * 2 + x[5] + x[6] +x[7] + 4) >> 3);
                    Des[ 0] = (UINT8)((x[2] + x[3] +x[4] + x[5] * 2 + x[6] + x[7] +x[8] + 4) >> 3);
                    Des[ 1] = (UINT8)((x[3] + x[4] +x[5] + x[6] * 2 + x[7] + x[8] +x[9] + 4) >> 3);
                    Des[ 2] = (UINT8)((x[4] + x[5] +x[6] + x[7] * 2 + x[8] + x[9] +x[9] + 4) >> 3);
                    Des[ 3] = (UINT8)((x[5] + x[6] +x[7] + x[8] * 2 + x[9] + x[9] +x[9] + 4) >> 3);
                }

                Src += PlaneLineStep;
                Des += PlaneLineStep;               
            }//for
            CurrentFrag ++;
        }//else
    }//while

}
/****************************************************************************
 * 
 *  ROUTINE       :     DeblockVerticalEdgesInLoopFilteredBand
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filter the vertical edges in a band
 *
 *  SPECIAL NOTES :     Save the variance
 *
 *  REFERENCE     :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void DeblockVerticalEdgesInLoopFilteredBand(
                                           PB_INSTANCE *pbi, 
                                           UINT8 *SrcPtr, 
                                           UINT8 *DesPtr, 
                                           UINT32 PlaneLineStep,
                                           UINT32 FragsAcross,
                                           UINT32 StartFrag,
                                           UINT32 *QuantScale
                                           )
{
    UINT32 j,k;
    UINT32 CurrentFrag=StartFrag;
    INT32 QStep;
    INT32 FLimit;
    UINT8 *Src, *Des;
    INT32  x[10];
    INT32  Sum1, Sum2;
    
    while(CurrentFrag < StartFrag + FragsAcross-1)
    {
        
        Src=SrcPtr+8*(CurrentFrag-StartFrag+1);
        Des=DesPtr+8*(CurrentFrag-StartFrag+1);

        {
            
            QStep = QuantScale[pbi->FragQIndex[CurrentFrag+1]];
            FLimit = ( QStep * 3)>>2 ;        

            for( j=0; j<8 ; j++)
            {                
                x[0] = Src[-5];
                x[1] = Src[-4];
                x[2] = Src[-3];
                x[3] = Src[-2];
                x[4] = Src[-1];
                x[5] = Src[0];
                x[6] = Src[+1];
                x[7] = Src[+2];
                x[8] = Src[+3];
                x[9] = Src[+4];
                
                Sum1=Sum2=0;
                
                for(k=1;k<=4;k++)
                {   
                    Sum1 += abs(x[k]-x[k-1]);
                    Sum2 += abs(x[k+4]-x[k+5]);           
                }

                pbi->FragmentVariances[CurrentFrag] += ((Sum1>255)?255:Sum1);
                pbi->FragmentVariances[CurrentFrag+1] += ((Sum2>255)?255:Sum2);

                               
                if( Sum1 < FLimit &&
                    Sum2 < FLimit &&
                    (x[5] - x[4]) < QStep && 
                    (x[4] - x[5]) < QStep )
                {
                    // low pass filtering (LPF7: 1 1 1 2 1 1 1) 
                    Des[-4] = (UINT8)((x[0] + x[0] +x[0] + x[1] * 2 + x[2] + x[3] +x[4] + 4) >> 3);
                    Des[-3] = (UINT8)((x[0] + x[0] +x[1] + x[2] * 2 + x[3] + x[4] +x[5] + 4) >> 3);
                    Des[-2] = (UINT8)((x[0] + x[1] +x[2] + x[3] * 2 + x[4] + x[5] +x[6] + 4) >> 3);
                    Des[-1] = (UINT8)((x[1] + x[2] +x[3] + x[4] * 2 + x[5] + x[6] +x[7] + 4) >> 3);
                    Des[ 0] = (UINT8)((x[2] + x[3] +x[4] + x[5] * 2 + x[6] + x[7] +x[8] + 4) >> 3);
                    Des[ 1] = (UINT8)((x[3] + x[4] +x[5] + x[6] * 2 + x[7] + x[8] +x[9] + 4) >> 3);
                    Des[ 2] = (UINT8)((x[4] + x[5] +x[6] + x[7] * 2 + x[8] + x[9] +x[9] + 4) >> 3);
                    Des[ 3] = (UINT8)((x[5] + x[6] +x[7] + x[8] * 2 + x[9] + x[9] +x[9] + 4) >> 3);
                }
                Src +=PlaneLineStep;
                Des +=PlaneLineStep;                
            } //for (j) loop
        }//if
        
        CurrentFrag ++;
    }//while
    
}




/****************************************************************************
 * 
 *  ROUTINE       :     DeblockPlane
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering the channel(Y or U or V ) for deblocking 
 *
 *  NOTE:         :     This function utilized the SAD as the criteria to 
 *                      determine where to apply the new 7-tap deblocking
 *                      fitler. And at the same time it accumulates the SAD 
 *                      value for every block, which will be used in subseuuent
 *                      deringing functions.
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void DeblockPlane(
    PB_INSTANCE *pbi, 
    UINT8 *SourceBuffer, 
    UINT8 *DestinationBuffer, 
    UINT32 Channel )
{
    
    UINT32 i,k;
    UINT32 PlaneLineStep=0;
    UINT32 StartFrag =0;
    UINT32 PixelIndex=0;
    UINT8 * SrcPtr=0, * DesPtr=0;
    UINT32 FragsAcross=0;
    UINT32 FragsDown=0;
    UINT32 *QuantScale=0;

    typedef void (*ApplyFilterToBand) (xPB_INST, UINT8 *, UINT8 *, UINT32, UINT32, UINT32, UINT32 *);

    ApplyFilterToBand DeblockBand;
    ApplyFilterToBand DeblockVerticalEdgesInBand;

    DeblockBand = pbi->DeblockLoopFilteredBand;
    DeblockVerticalEdgesInBand = DeblockVerticalEdgesInLoopFilteredBand;
    

    switch( Channel )
    {    
    case 0:
        // Get the parameters
        PlaneLineStep = pbi->Configuration.YStride; 
        FragsAcross = pbi->HFragments;
        FragsDown = pbi->VFragments;
        StartFrag = 0;
        PixelIndex = pbi->ReconYDataOffset;
        SrcPtr = & SourceBuffer[PixelIndex];
        DesPtr = & DestinationBuffer[PixelIndex];
        break;
    
    case 1:
        // Get the parameters
        PlaneLineStep = pbi->Configuration.UVStride;    
        FragsAcross = pbi->HFragments / 2;
        FragsDown = pbi->VFragments / 2;
        StartFrag = pbi->YPlaneFragments;

        PixelIndex = pbi->ReconUDataOffset;
        SrcPtr = & SourceBuffer[PixelIndex];
        DesPtr = & DestinationBuffer[PixelIndex];
        break;

    default:
        // Get the parameters
        PlaneLineStep = pbi->Configuration.UVStride;    
        FragsAcross = pbi->HFragments / 2;
        FragsDown = pbi->VFragments / 2;
        StartFrag =   pbi->YPlaneFragments + pbi->UVPlaneFragments;

        PixelIndex = pbi->ReconVDataOffset;
        SrcPtr = & SourceBuffer[PixelIndex];
        DesPtr = & DestinationBuffer[PixelIndex];
        break;
    }

    QuantScale = DCQuantScaleV1;
    
    
    // In order to make use of the 
    for(i=0;i<4;i++)
    {
        memcpy(DesPtr+i*PlaneLineStep, SrcPtr+i*PlaneLineStep, PlaneLineStep);
    }

    // loop to last band
    k = 1;

    while( k < FragsDown )
    {

        SrcPtr += 8*PlaneLineStep;
        DesPtr += 8*PlaneLineStep;

        //Filter both the horizontal and vertical block edges inside the band
        DeblockBand(
                pbi, 
                SrcPtr, 
                DesPtr, 
                PlaneLineStep, 
                FragsAcross, 
                StartFrag,
                QuantScale);
        
        //Move Pointers
        StartFrag += FragsAcross;
        
        k ++;   
    }

    // The Last band
    for(i=0;i<4;i++)
    {
        memcpy(DesPtr+(i+4)*PlaneLineStep, SrcPtr+(i+4)*PlaneLineStep, PlaneLineStep);
    }
  
    DeblockVerticalEdgesInBand(
        pbi,
        SrcPtr,
        DesPtr, 
        PlaneLineStep, 
        FragsAcross, 
        StartFrag,
        QuantScale);

}



/****************************************************************************
 * 
 *  ROUTINE       :     DeblockFrame
 *
 *  FUNCTION      :     Applies a loop filter to the edge pixels of coded blocks.
 *    
 *  NOTE:         :     This function utilized the SAD as the criteria to 
 *                      determine where to apply the new 7-tap deblocking
 *                      fitler. And at the same time it accumulates the SAD 
 *                      value for every block, which will be used in subseuuent
 *                      deringing functions.
 *          
 ****************************************************************************/
void DeblockFrame(PB_INSTANCE *pbi, UINT8 *SourceBuffer, UINT8 *DestinationBuffer)
{ 

        memset(pbi->FragmentVariances, 0 , sizeof(INT32) * pbi->UnitFragments);
        
        
        UpdateFragQIndex(pbi);

        
        SetupLoopFilter(pbi);
 
        //Y
        DeblockPlane( pbi, SourceBuffer, DestinationBuffer, 0);
        
        //U
        DeblockPlane( pbi, SourceBuffer, DestinationBuffer, 1);
        
        //v
        DeblockPlane( pbi, SourceBuffer, DestinationBuffer, 2);
                
}





/****************************************************************************
 * 
 *  ROUTINE       :     PostProcess
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the edge pixels of coded blocks.
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void PostProcess(PB_INSTANCE *pbi)
{

    switch (pbi->PostProcessingLevel)
    {
    case 8:
        // on a slow machine, use a simpler and faster deblocking filter
	    DeblockFrame(pbi, pbi->LastFrameRecon,pbi->PostProcessBuffer);
        break;

    case 6:
        DeblockFrame(pbi, pbi->LastFrameRecon,pbi->PostProcessBuffer);
        UpdateUMVBorder(pbi, pbi->PostProcessBuffer );
        DeringFrame(pbi, pbi->PostProcessBuffer, pbi->PostProcessBuffer);
        break;

    case 5:
        DeblockFrame(pbi, pbi->LastFrameRecon,pbi->PostProcessBuffer);
        UpdateUMVBorder(pbi, pbi->PostProcessBuffer );
        DeringFrame(pbi, pbi->PostProcessBuffer, pbi->PostProcessBuffer);
        break;
    case 4:
        DeblockFrame(pbi, pbi->LastFrameRecon, pbi->PostProcessBuffer);
        break;
    case 1:
        UpdateFragQIndex(pbi);
        break;

    case 0:
        break;

    default:
        DeblockFrame(pbi, pbi->LastFrameRecon, pbi->PostProcessBuffer);
        UpdateUMVBorder(pbi, pbi->PostProcessBuffer );
        DeringFrame(pbi, pbi->PostProcessBuffer, pbi->PostProcessBuffer);
        break;
    }
}


#endif