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
*   Module Title :     vfwcomp.c
*
*   Description  :     Video CODEC compression module
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <string.h>

#include "SystemDependant.h"
#include "CBitman.h"
#include "dct.h"
#include "CFrameW.h"
#include "Quantize.h"
#include "compdll.h"
#include "misc_common.h"
#include <stdio.h>



/****************************************************************************
*  Module constants.
*****************************************************************************
*/  

#define A_TABLE_SIZE				29

#define DF_CANDIDATE_WINDOW 5

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Module Static Variables
*****************************************************************************
*/              
UINT32 PriorKeyFrameWeight[KEY_FRAME_CONTEXT] = { 1,2,3,4,5 };

// Data structures controlling addition of residue blocks
UINT32 ResidueErrorThresh[Q_TABLE_SIZE] =  {    750, 700, 650, 600, 590, 580, 570, 560, 
                                                550, 540, 530, 520, 510, 500, 490, 480,  
                                                470, 460, 450, 440, 430, 420, 410, 400, 
                                                390, 380, 370, 360, 350, 340, 330, 320, 
                                                310, 300, 290, 280, 270, 260, 250, 245, 
                                                240, 235, 230, 225, 220, 215, 210, 205, 
                                                200, 195, 190, 185, 180, 175, 170, 165,
                                                160, 155, 150, 145, 140, 135, 130, 130 };
UINT32 ResidueBlockFactor[Q_TABLE_SIZE] =  {    3,   3,   3,   3,   3,   3,   3,   3,
                                                3,   3,   3,   3,   3,   3,   3,   3,
                                                3,   3,   3,   3,   3,   3,   3,   3,
                                                3,   3,   3,   3,   3,   3,   3,   3,
                                                2,   2,   2,   2,   2,   2,   2,   2,
                                                2,   2,   2,   2,   2,   2,   2,   2,
                                                2,   2,   2,   2,   2,   2,   2,   2,
                                                2,   2,   2,   2,   2,   2,   2,   2 };



/****************************************************************************
*  Imports
*****************************************************************************
*/              
extern void UpdateUMVBorder( PB_INSTANCE *pbi, UINT8 * DestReconPtr );

/****************************************************************************
*  Module Functions
*****************************************************************************
*/              

void CompressInit(CP_INSTANCE *cpi);
void CompressFirstFrame(CP_INSTANCE *cpi);
void CompressFrame( CP_INSTANCE *cpi, UINT32 FrameNumber );
int CompressWKeyFrame(CP_INSTANCE *cpi);


/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/

/****************************************************************************
 * 
 *  ROUTINE       :     Initialise
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Sets up the default starting cpi->pb.Configuration.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void Initialise(CP_INSTANCE *cpi)
{  
    // Call the machine / platform specific initilisation function.
    CMachineSpecificConfig(cpi);

    // DCT table initialisation
    //InitDctTables();
    cpi->pb.Configuration.HFragPixels = 8;
    cpi->pb.Configuration.VFragPixels = 8;


}               

/****************************************************************************
 * 
 *  ROUTINE       :     CompressInit
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Initialises conmpression
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CompressInit(CP_INSTANCE *cpi)
{                  
    /* We always start at frame 1 */                 
    cpi->CurrentFrame = 1;   
    
    // Reset the rate targeting correction factor.
    cpi->BpbCorrectionFactor = 1.0;

	cpi->TotalByteCount = 0;
	cpi->TotalMotionScore = 0;

    // Up regulation variables.
    cpi->FinalPassLastPos = 0;   // Used to regulate a final unrestricted pass. 
    cpi->LastEndSB = 0;	        // Where we were in the loop last time. 
    cpi->ResidueLastEndSB = 0;	// Where we were in the residue update loop last time.         

    // Select the appropriate huffman set.
	SelectHuffmanSet(&cpi->pb);

	// This makes sure encoder version specific tables are initialised
    InitQTables(&cpi->pb);  

}

/****************************************************************************
 * 
 *  ROUTINE       :     setup keyframe for compression
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Initialises keyframe conmpression
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SetupKeyFrame(CP_INSTANCE *cpi)
{
    // Make sure the "last frame" buffer contains the first frame data aswell.
    memcpy ( cpi->yuv0ptr, cpi->yuv1ptr, cpi->pb.ReconYPlaneSize + 2 * cpi->pb.ReconUVPlaneSize );

    /* Initialise the  cpi->pb.display_fragments and other fragment structures for the first frame. */
    memset( cpi->pb.display_fragments, 1, cpi->pb.UnitFragments );
    memset( cpi->extra_fragments, 1, cpi->pb.UnitFragments );

    // Set up for a BASE/KEY FRAME 
    SetFrameType( &cpi->pb,BASE_FRAME );
}

/****************************************************************************
 * 
 *  ROUTINE       :     adjust keyframe context
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     adjust's key frame context 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void AdjustKeyFrameContext(CP_INSTANCE *cpi)
{
	
	UINT32 i;
	
	// Average key frame frequency and size
	UINT32  AvKeyFrameFrequency = (UINT32) (cpi->CurrentFrame / cpi->KeyFrameCount);  
	UINT32  AvKeyFrameBytes = (UINT32) (cpi->TotKeyFrameBytes / cpi->KeyFrameCount);
	UINT32 TotalWeight=0;
	INT32 AvKeyFramesPerSecond;
	INT32 MinFrameTargetRate;

	// Update the frame carry over.
	cpi->TotKeyFrameBytes += cpi->ThisFrameSize;
	
	/* reset keyframe context and calculate weighted average of last KEY_FRAME_CONTEXT keyframes */
	for( i = 0 ; i < KEY_FRAME_CONTEXT ; i ++ )
	{
		if ( i < KEY_FRAME_CONTEXT -1)
		{
			cpi->PriorKeyFrameSize[i] = cpi->PriorKeyFrameSize[i+1];
			cpi->PriorKeyFrameDistance[i] = cpi->PriorKeyFrameDistance[i+1];
		}
		else
		{
			cpi->PriorKeyFrameSize[KEY_FRAME_CONTEXT - 1] = cpi->ThisFrameSize;
			cpi->PriorKeyFrameDistance[KEY_FRAME_CONTEXT - 1] = cpi->LastKeyFrame;
		}
		
		AvKeyFrameBytes += PriorKeyFrameWeight[i] * cpi->PriorKeyFrameSize[i];
		AvKeyFrameFrequency += PriorKeyFrameWeight[i] * cpi->PriorKeyFrameDistance[i];
		TotalWeight += PriorKeyFrameWeight[i];
	}
	AvKeyFrameBytes /= TotalWeight;
	AvKeyFrameFrequency /= TotalWeight;
	AvKeyFramesPerSecond =  100 * cpi->Configuration.OutputFrameRate / AvKeyFrameFrequency ;


	/* Calculate a new target rate per frame allowing for average key frame frequency over newest frames . */
	if ( 100 * cpi->Configuration.TargetBandwidth > AvKeyFrameBytes * AvKeyFramesPerSecond &&
		(100 * cpi->Configuration.OutputFrameRate - AvKeyFramesPerSecond ))
	{
		cpi->frame_target_rate =  
			(INT32)(100* cpi->Configuration.TargetBandwidth - AvKeyFrameBytes * AvKeyFramesPerSecond )
			/ ( (100 * cpi->Configuration.OutputFrameRate - AvKeyFramesPerSecond ) );
	}
	else // don't let this number get too small!!!
	{
		cpi->frame_target_rate = 1;
	}

	// minimum allowable frame_target_rate
	MinFrameTargetRate = (cpi->Configuration.TargetBandwidth / cpi->Configuration.OutputFrameRate) / 3;

	if(cpi->frame_target_rate < MinFrameTargetRate ) 
	{
		cpi->frame_target_rate = MinFrameTargetRate;
	}

	cpi->LastKeyFrame = 1;
	cpi->LastKeyFrameSize=cpi->ThisFrameSize;


}
/****************************************************************************
 * 
 *  ROUTINE       :     CompressFirstFrame
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Compresses the first frame
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CompressFirstFrame(CP_INSTANCE *cpi)
{                  
    UINT32  register i; 

	// if not AutoKeyframing cpi->ForceKeyFrameEvery = is frequency
	if(!cpi->AutoKeyFrameEnabled)
	{
		cpi->ForceKeyFrameEvery = cpi->KeyFrameFrequency;
	}

	/* set up context of key frame sizes and distances for more local datarate control */
	for( i = 0 ; i < KEY_FRAME_CONTEXT ; i ++ )
	{
		cpi->PriorKeyFrameSize[i] = cpi->KeyFrameDataTarget;
		
		cpi->PriorKeyFrameDistance[i] = cpi->ForceKeyFrameEvery;
	}

    // Keep track of the total number of Key Frames Coded.
    cpi->KeyFrameCount = 1;
	cpi->LastKeyFrame = 1;
    cpi->TotKeyFrameBytes = 0;

    // A key frame is not a dropped frame there for reset the count of 
    // consequative dropped frames.
    cpi->DropCount = 0;     

	SetupKeyFrame(cpi);

  	// Calculate a new target rate per frame allowing for average key frame frequency and size thus far.
    if ( cpi->Configuration.TargetBandwidth > ((cpi->KeyFrameDataTarget * cpi->Configuration.OutputFrameRate)/cpi->KeyFrameFrequency) )
    {
        cpi->frame_target_rate =  (INT32)((cpi->Configuration.TargetBandwidth - 
			((cpi->KeyFrameDataTarget * cpi->Configuration.OutputFrameRate)/cpi->KeyFrameFrequency)) / cpi->Configuration.OutputFrameRate);
    }
    else 
        cpi->frame_target_rate = 1;

    // Set baseline frame target rate.
    cpi->BaseLineFrameTargetRate = cpi->frame_target_rate;

    // A key frame is not a dropped frame there for reset the count of 
    // consequative dropped frames.
    cpi->DropCount = 0;     

    // Initialise drop frame trigger to 5 frames worth of data.
    cpi->DropFrameTriggerBytes = cpi->frame_target_rate * DF_CANDIDATE_WINDOW;

    // Set a target size for this key frame based upon the baseline target and frequency
    cpi->ThisFrameTargetBytes = cpi->KeyFrameDataTarget;

    // Get a DCT quantizer level for the key frame.
    cpi->MotionScore = cpi->pb.UnitFragments;

	// TEMP... force fixedQ.
	//cpi->FixedQ = cpi->pb.QThreshTable[cpi->Configuration.MaxQ];

	RegulateQ(cpi, cpi->pb.UnitFragments);

	cpi->pb.LastFrameQualityValue = cpi->pb.ThisFrameQualityValue;

    // Initialise quantizer. 
    UpdateQC(&cpi->pb, cpi->pb.ThisFrameQualityValue );  

    /* Initialise the cpi->pb.display_fragments and other fragment structures for the first frame. */
    for ( i = 0; i < cpi->pb.UnitFragments; i ++ )
    {
		 cpi->FragmentLastQ[i] = cpi->pb.ThisFrameQualityValue;
    } 

	/* Compress and output the frist frame. */
    PickIntra( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, cpi->pb.HFragments%4, cpi->pb.VFragments%4, cpi->pb.Configuration.VideoFrameWidth);
    UpdateFrame(cpi);  

    /* Initialise the carry over rate targeting variables. */
    cpi->CarryOver = 0;

}

/****************************************************************************
 * 
 *  ROUTINE       :     CompressKeyFrame
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Compresses a key frame
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CompressKeyFrame(CP_INSTANCE *cpi)
{                  
    UINT32  i;   

    // Average key frame frequency and size
    INT32 AvKeyFrameFrequency = (INT32) (cpi->CurrentFrame / cpi->KeyFrameCount);  
    INT32 AvKeyFrameBytes = (INT32) (cpi->TotKeyFrameBytes / cpi->KeyFrameCount);

	// Before we compress reset the carry over to the actual frame carry over 
	cpi->CarryOver = cpi->Configuration.TargetBandwidth * cpi->CurrentFrame  / cpi->Configuration.OutputFrameRate - cpi->TotalByteCount;

    // Keep track of the total number of Key Frames Coded
    cpi->KeyFrameCount += 1;

    // A key frame is not a dropped frame there for reset the count of 
    // consequative dropped frames.
    cpi->DropCount = 0;     

	SetupKeyFrame(cpi);

	// set a target size for this frame
	cpi->ThisFrameTargetBytes = (INT32) cpi->frame_target_rate + 
		( (cpi->KeyFrameDataTarget - cpi->frame_target_rate) * cpi->LastKeyFrame / cpi->ForceKeyFrameEvery );

    if ( cpi->ThisFrameTargetBytes > cpi->KeyFrameDataTarget )
        cpi->ThisFrameTargetBytes = cpi->KeyFrameDataTarget;
	
    // Get a DCT quantizer level for the key frame.
	cpi->MotionScore = cpi->pb.UnitFragments;

	RegulateQ(cpi, cpi->pb.UnitFragments);

	cpi->pb.LastFrameQualityValue = cpi->pb.ThisFrameQualityValue;

    /* Initialise DCT tables. */
    UpdateQC(&cpi->pb, cpi->pb.ThisFrameQualityValue );  

    // Initialise the cpi->pb.display_fragments and other fragment structures for the first frame. 
    for ( i = 0; i < cpi->pb.UnitFragments; i ++ )
    {
		 cpi->FragmentLastQ[i] = cpi->pb.ThisFrameQualityValue;
    } 
    
	/* Compress and output the frist frame. */
    PickIntra( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, cpi->pb.HFragments%4, cpi->pb.VFragments%4, cpi->pb.Configuration.VideoFrameWidth);
    UpdateFrame(cpi);  


}


/****************************************************************************
 * 
 *  ROUTINE       :     CompressFrame
 *
 *  INPUTS        :     UINT32 FrameNumber - The number of the frame.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Analyses and compresses a frame.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CompressFrame( CP_INSTANCE *cpi, UINT32 FrameNumber )
{       
	INT32 min_blocks_per_frame;
    UINT32	i; 
    BOOL    DropFrame = FALSE;
    UINT32  ResidueBlocksAdded=0;
	UINT32  KFIndicator = 0;

    double QModStep;
    double QModifier = 1.0;
    
    /* Clear down the macro block level mode and MV arrays. */
    for ( i = 0; i < cpi->pb.UnitFragments; i++ )
    {
        cpi->pb.FragCodingMethod[i] = CODE_INTER_NO_MV;     // Default coding mode
        cpi->pb.FragMVect[i].x = 0;
        cpi->pb.FragMVect[i].y = 0;
    }

	/* Default to normal frames. */
	SetFrameType( &cpi->pb, NORMAL_FRAME );  

	/* Clear down the difference arrays for the current frame. */                     
    memset( cpi->pb.display_fragments, 0, cpi->pb.UnitFragments );
	memset( cpi->extra_fragments, 0, cpi->pb.UnitFragments );

	// Calculate the target bytes for this frame. */
	cpi->ThisFrameTargetBytes = cpi->frame_target_rate;

	// Correct target to try and compensate for any overall rate error that is developing

    // Set the max allowed Q for this frame based upon carry over history.
	// First set baseline worst Q for this frame
	cpi->Configuration.ActiveMaxQ = cpi->Configuration.MaxQ + 10;
	if ( cpi->Configuration.ActiveMaxQ >= Q_TABLE_SIZE )
		cpi->Configuration.ActiveMaxQ = Q_TABLE_SIZE - 1;

	// Make a further adjustment based upon the carry over and recent history..
	// cpi->Configuration.ActiveMaxQ reduced by 1 for each 1/2 seconds worth of
	// -ve carry over up to a limit of 6.
	// Also cpi->Configuration.ActiveMaxQ reduced if frame is a "DropFrameCandidate".
	// Remember that if we are behind the bit target carry over is -ve.
	if ( cpi->CarryOver < 0 )
	{
		if ( cpi->DropFrameCandidate )
		{
			cpi->Configuration.ActiveMaxQ -= 4;
		}
    
		if ( cpi->CarryOver < -((INT32)cpi->Configuration.TargetBandwidth*3) )
			cpi->Configuration.ActiveMaxQ -= 6;
		else
			cpi->Configuration.ActiveMaxQ += (INT32) ((cpi->CarryOver*2) / (INT32)cpi->Configuration.TargetBandwidth);

		// Check that we have not dropped quality too far
		if ( cpi->Configuration.ActiveMaxQ < cpi->Configuration.MaxQ )
			cpi->Configuration.ActiveMaxQ = cpi->Configuration.MaxQ;
	}

    // Calculate the Q Modifier step size required to cause a step down from full 
    // target bandwidth to 40% of target between max Q and best Q
    QModStep = 0.5 / (double)((Q_TABLE_SIZE - 1) - cpi->Configuration.ActiveMaxQ); 

    // Set up the cpi->QTargetModifier[] table.
    for ( i = 0; i < cpi->Configuration.ActiveMaxQ; i++ )
    {
        cpi->QTargetModifier[i] = QModifier;
    }
    for ( i = cpi->Configuration.ActiveMaxQ; i < Q_TABLE_SIZE; i++ )
    {
        cpi->QTargetModifier[i] = QModifier;
        QModifier -= QModStep;
    }

    // if we are allowed to drop frames and are falling behind
    // (eg more than x frames worth of bandwidth)
	if ( cpi->DropFramesAllowed && 
		 ( cpi->DropCount < cpi->MaxConsDroppedFrames) && 
         ( cpi->CarryOver < -((INT32)cpi->Configuration.TargetBandwidth)) &&
         ( cpi->DropFrameCandidate) )
    {
		// (we didn't do this frame so we should have some left over for the next frame)
		cpi->CarryOver += cpi->frame_target_rate;
        DropFrame = TRUE;
        cpi->DropCount ++;

        // Adjust DropFrameTriggerBytes to account for the saving achieved.
        cpi->DropFrameTriggerBytes = (cpi->DropFrameTriggerBytes * (DF_CANDIDATE_WINDOW-1))/DF_CANDIDATE_WINDOW;        

        //  Even if we drop a frame we should account for it when considering key frame seperation.
    	cpi->LastKeyFrame++;                                    
    }
    // Reduce frame bit target by 1.75% for each 1/10th of a seconds worth of -ve carry over
    // down to a minimum of 65% of its un-modified value.
    else if ( cpi->CarryOver < -((INT32)cpi->Configuration.TargetBandwidth * 2) )
    {
    	cpi->ThisFrameTargetBytes = (UINT32)(cpi->ThisFrameTargetBytes * 0.65); 
    }
	else if ( cpi->CarryOver < 0 )
	{
        // Note that cpi->CarryOver is a -ve here hence 1.0 "+" ...
    	//cpi->ThisFrameTargetBytes = (UINT32)(cpi->ThisFrameTargetBytes * 
        //                                    (1.0 + ( ((cpi->CarryOver * 10)/8)) * 0.0175) );
    	cpi->ThisFrameTargetBytes = (UINT32)(cpi->ThisFrameTargetBytes * 
                                            (1.0 + ( ((cpi->CarryOver * 10)/((INT32)cpi->Configuration.TargetBandwidth)) * 0.0175) ));
	}

    if ( !DropFrame )
    {
        // pick all the macroblock modes and motion vectors
        UINT32 InterError;
        UINT32 IntraError;


        /* Set Baseline filter level. */
		SetScanParam( cpi->pp, SCP_CONFIGURE_PP, cpi->PreProcFilterLevel );

		/* Score / analyses the fragments. */ 
		cpi->MotionScore = YUVAnalyseFrame(cpi->pp, &KFIndicator );
        
        // Get the baseline Q value
		RegulateQ( cpi, cpi->MotionScore );

        // Recode blocks if the error score in last frame was high.
        ResidueBlocksAdded  = 0;
		for ( i = 0; i < cpi->pb.UnitFragments; i++ )
		{
            if ( !cpi->pb.display_fragments[i] )
            {
                if ( cpi->LastCodedErrorScore[i] >= ResidueErrorThresh[cpi->pb.FrameQIndex] )
                {
                    cpi->pb.display_fragments[i] = 1;           // Force block update
                    cpi->extra_fragments[i] = 1;             // Insures up to date pixel data is used.
                    ResidueBlocksAdded ++;
                }
            }
        }

        // Adjust the motion score to allow for residue blocks added. These are assumed
        // to have below average impact on bitrate (Hence ResidueBlockFactor).
        cpi->MotionScore = cpi->MotionScore + (ResidueBlocksAdded / ResidueBlockFactor[cpi->pb.FrameQIndex]);

		// Estimate the min number of blocks at best Q
   		min_blocks_per_frame = (INT32)(cpi->ThisFrameTargetBytes / GetEstimatedBpb( cpi, VERY_BEST_Q ));
		if ( min_blocks_per_frame == 0 )
			min_blocks_per_frame = 1;

        // If we have less than this number then consider adding in some extra blocks
        if ( cpi->MotionScore < min_blocks_per_frame )
        {
			min_blocks_per_frame = cpi->MotionScore + (INT32)(((min_blocks_per_frame - cpi->MotionScore) * 4) / 3 );
            UpRegulateDataStream( cpi, VERY_BEST_Q, min_blocks_per_frame );
        }
        else
        {
            // Reset control variable for best quality final pass.
            cpi->FinalPassLastPos = 0;
        }

        // Get the modified Q prediction taking into account extra blocks added.
		RegulateQ( cpi, cpi->MotionScore );

        // Unless we are already well ahead (4 seconds of data) of the projected bitrate
		if ( cpi->CarryOver < (INT32)(cpi->Configuration.TargetBandwidth * 4) )
        {
            // Look at the predicted Q (pbi->FrameQIndex).
            // Adjust the target bits for this frame based upon projected Q and re-calculate.
            // The idea is that if the Q is better than a given (good enough) level then we 
            // will try and save some bits for use in more difficult segments.
            cpi->ThisFrameTargetBytes = (INT32) (cpi->ThisFrameTargetBytes * cpi->QTargetModifier[cpi->pb.FrameQIndex]);

            // Recalculate Q again
		    RegulateQ( cpi, cpi->MotionScore );
        }


        /* Select modes and motion vectors for each of the blocks : return an error score for inter and intra */
        PickModes( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, cpi->pb.HFragments%4, cpi->pb.VFragments%4, cpi->pb.Configuration.VideoFrameWidth, &InterError, &IntraError );

        // decide whether we really should have made this frame a key frame 


		if( cpi->AutoKeyFrameEnabled )
        {
			if( (   
				( 2* IntraError < 5 * InterError ) 
                 && ( KFIndicator >= (UINT32) cpi->AutoKeyFrameThreshold) 
                 && ( cpi->LastKeyFrame > cpi->MinimumDistanceToKeyFrame) 
                ) ||
				(cpi->LastKeyFrame >= (UINT32)cpi->ForceKeyFrameEvery) 
              )
			{

				CompressKeyFrame(cpi);  // Code a key frame
				return;
			}
			
		}
        // Increment the frames since last key frame count
		cpi->LastKeyFrame++;

		if ( cpi->MotionScore > 0 )
		{
			cpi->DropCount = 0;

			/* Proceed with the frame update. */
			UpdateFrame(cpi);  

		    // Note the Quantizer used for each block coded.
		    for ( i = 0; i < cpi->pb.UnitFragments; i++ )
		    {
    		    if ( cpi->pb.display_fragments[i] )
			    {
    			    cpi->FragmentLastQ[i] = cpi->pb.ThisFrameQualityValue;
			    }
		    }

		}
    }
	else
	{
		if (cpi->NoDrops == 1)
		{
			UpdateFrame(cpi);
		}
	}

}
  

/****************************************************************************
 * 
 *  ROUTINE       :     UpdateFrame
 *
 *  INPUTS        :     None. 
 *                      
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Writes the fragment data to the output file and updates
 *                      the displayed frame.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpdateFrame(CP_INSTANCE *cpi)
{
	// Context sensitive target_rate control 
    // Average key frame frequency and size
    INT32 AvKeyFrameFrequency = (INT32) (cpi->CurrentFrame / cpi->KeyFrameCount);  
    INT32 AvKeyFrameBytes = (INT32) (cpi->TotKeyFrameBytes / cpi->KeyFrameCount);
	INT32 TotalWeight = 0;

	double CorrectionFactor;
    UINT32  fragment_count = 0;                               
    
    UINT32 diff_tokens = 0;                  
	UINT32 bits_per_token = 0;	  //TEMP

    // Reset the DC predictors.
    cpi->pb.LastIntraDC = 0;
    cpi->pb.InvLastIntraDC = 0;
    cpi->pb.LastInterDC = 0;
    cpi->pb.InvLastInterDC = 0;

  	// Initialise bit packing mechanism.
    InitAddBitsToBuffer(cpi);  

    // Write out the frame header information including size. 
    WriteFrameHeader(cpi);

	// Copy back any extra frags that are to be updated by the codec
	// as part of the background cleanup task
	CopyBackExtraFrags(cpi);

    /* Encode the data.  */
    EncodeData(cpi); 

    // Adjust drop frame trigger.
	if ( GetFrameType(&cpi->pb) != BASE_FRAME )
	{
        // Apply decay factor then add in the last frame size.
        cpi->DropFrameTriggerBytes = ((cpi->DropFrameTriggerBytes * (DF_CANDIDATE_WINDOW-1)) / DF_CANDIDATE_WINDOW) + cpi->ThisFrameSize;
    }
    else
    {
        // Increase cpi->DropFrameTriggerBytes a little. Just after a key frame
        // may actually be a good time to drop a frame.
        cpi->DropFrameTriggerBytes = (cpi->DropFrameTriggerBytes * DF_CANDIDATE_WINDOW) / (DF_CANDIDATE_WINDOW-1);
    }

    // Test for overshoot which may require a dropped frame next time around.
    // If we are already in a drop frame condition but the previous frame was not 
    // dropped then the threshold for continuing to allow dropped frames is reduced.
    if ( cpi->DropFrameCandidate )
    {
        if ( cpi->DropFrameTriggerBytes > (cpi->frame_target_rate * (DF_CANDIDATE_WINDOW+1)) )
            cpi->DropFrameCandidate = TRUE;
        else
            cpi->DropFrameCandidate = FALSE;
    }
    else
    {
        if ( cpi->DropFrameTriggerBytes > (cpi->frame_target_rate * ((DF_CANDIDATE_WINDOW*2)-2)) )
            cpi->DropFrameCandidate = TRUE;
        else
            cpi->DropFrameCandidate = FALSE;
    }

  	/* Update the BpbCorrectionFactor variable according to whether or not we were
	*  close enough with our selection of DCT quantiser. 
	*/
	if ( GetFrameType(&cpi->pb) != BASE_FRAME )
	{
        // Work out a size correction factor.
	    CorrectionFactor = (double)cpi->ThisFrameSize / (double)cpi->ThisFrameTargetBytes;
  
        if ( (CorrectionFactor > 1.05) && 
			 (cpi->pb.ThisFrameQualityValue < cpi->pb.QThreshTable[cpi->Configuration.ActiveMaxQ]) )
		{
        	CorrectionFactor = 1.0 + ((CorrectionFactor - 1.0)/2);
            if ( CorrectionFactor > 1.5 )
                cpi->BpbCorrectionFactor *= 1.5;
            else
                cpi->BpbCorrectionFactor *= CorrectionFactor;

			// Keep BpbCorrectionFactor within limits
			if ( cpi->BpbCorrectionFactor > MAX_BPB_FACTOR )
				cpi->BpbCorrectionFactor = MAX_BPB_FACTOR;
		}
		else if ( (CorrectionFactor < 0.95) &&
				  (cpi->pb.ThisFrameQualityValue > VERY_BEST_Q) )
		{
        	CorrectionFactor = 1.0 - ((1.0 - CorrectionFactor)/2);
            if ( CorrectionFactor < 0.75 )
				cpi->BpbCorrectionFactor *= 0.75;
            else
			    cpi->BpbCorrectionFactor *= CorrectionFactor;

			// Keep BpbCorrectionFactor within limits
			if ( cpi->BpbCorrectionFactor < MIN_BPB_FACTOR )
				cpi->BpbCorrectionFactor = MIN_BPB_FACTOR;
		}
	}

    // Adjust carry over and or key frame context.
    if ( GetFrameType(&cpi->pb) == BASE_FRAME )
    {
        // Adjust the key frame context unless the key frame was very small 
        {
	        AdjustKeyFrameContext(cpi);
        }
    }
    else
        // Update the frame carry over
        cpi->CarryOver += ((INT32)cpi->frame_target_rate - (INT32)cpi->ThisFrameSize);

	cpi->TotalByteCount += cpi->ThisFrameSize;
}


