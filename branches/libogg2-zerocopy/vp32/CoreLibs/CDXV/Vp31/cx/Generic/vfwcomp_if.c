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
*   Module Title :     vfwComp_if.c 
*
*   Description  :     Compressor interface
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <string.h>

#include "BlockMapping.h"
#include "Huffman.h"
#include "Mcomp.h"
#include "compdll.h"
#include "YUVtofromRGB.h"
#include "vfw_comp_interface.h"
#include "misc_common.h"
#include "CBitman.h"
#include <stdio.h>
#include "VP31EVERSION.h"
#define CommentString "\nON2.COM VERSION VP31E " VP31EVERSION "\n"
#pragma comment(exestr,CommentString)

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        


/****************************************************************************
*  Module statics.
*****************************************************************************
*/        

static const char vp31eVersion[] = VP31EVERSION;


/****************************************************************************
*  Imports
*****************************************************************************
*/

extern void CompressInit(CP_INSTANCE *cpi);
extern void CompressFirstFrame(CP_INSTANCE *cpi);
extern void CompressKeyFrame(CP_INSTANCE *cpi);


extern void CompressFrame( CP_INSTANCE *cpi, UINT32 FrameNumber );
extern void Initialise(CP_INSTANCE *cpi);

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/


/****************************************************************************
*  Foreward references
*****************************************************************************
*/

/****************************************************************************
 * 
 *  ROUTINE       :     VP31E_GetVersionNumber
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None .
 *
 *  FUNCTION      :     Returns a pointer to the version string
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
const char * CCONV VP31E_GetVersionNumber(void)
{
    return vp31eVersion;
}

/****************************************************************************
 * 
 *  ROUTINE       :     StartEncoder
 *
 *  INPUTS        :     COMP_CONFIG		Compressor cpi->pb.Configuration
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Starts / initialises the encoder
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
BOOL CCONV StartEncoder( CP_INSTANCE **cpi , COMP_CONFIG * CompConfig )
{

	*cpi=CreateCPInstance();

    // Initialisation default config. 
	Initialise(*cpi);


	/* set the version number */
	(*cpi)->pb.Vp3VersionNo = CURRENT_ENCODE_VERSION;

    /* Set the video frame size. */
	(*cpi)->pb.YPlaneSize = 0xFFF;
	(*cpi)->pb.Configuration.VideoFrameHeight =0xFFF;
    (*cpi)->pb.Configuration.VideoFrameHeight = (CompConfig->FrameSize & 0x0000FFFF);
    (*cpi)->pb.Configuration.VideoFrameWidth = ((CompConfig->FrameSize & 0xFFFF0000) >> 16);

	/* Note the height and width in the pre-processor control structure. */
	(*cpi)->ScanConfig.VideoFrameHeight = (*cpi)->pb.Configuration.VideoFrameHeight;
	(*cpi)->ScanConfig.VideoFrameWidth = (*cpi)->pb.Configuration.VideoFrameWidth;

	// Set data rate related variables. //
	(*cpi)->Configuration.TargetBandwidth = (CompConfig->TargetBitRate * 1024) / 8;

    // Set the target minimum key frame frequency
    (*cpi)->KeyFrameFrequency = CompConfig->KeyFrameFrequency;

    // Set key frame data rate target
    (*cpi)->KeyFrameDataTarget = (CompConfig->KeyFrameDataTarget * 1024) / 8;

	/* Set the quality settings. */
	ConfigureQuality( (*cpi), CompConfig->Quality );

	// Set the frame rate variables.
    (*cpi)->Configuration.OutputFrameRate = CompConfig->FrameRate;
	if ( (*cpi)->Configuration.OutputFrameRate < 1 )
		(*cpi)->Configuration.OutputFrameRate = 1;
	else if ( (*cpi)->Configuration.OutputFrameRate > 30 )
		(*cpi)->Configuration.OutputFrameRate = 30;
	(*cpi)->frame_target_rate =  ((*cpi)->Configuration.TargetBandwidth / (*cpi)->Configuration.OutputFrameRate); 


	/* Clear the frame size changed flag. We don't need to request a resize if
	*  the grabber is not yet initialised. 
	*/
	(*cpi)->BufferedOutputBytes = 0;

    // Initialise image format details
    if(!InitFrameDetails(&(*cpi)->pb))
    {
        return FALSE;
    }
	if(!EAllocateFragmentInfo(*cpi))
    {
        DeleteFragmentInfo(&(*cpi)->pb);
        DeleteFrameInfo(&(*cpi)->pb);
        return FALSE;
    }

	if(!EAllocateFrameInfo(*cpi))
    {
        DeleteFragmentInfo(&(*cpi)->pb);
        DeleteFrameInfo(&(*cpi)->pb);
        EDeleteFragmentInfo(*cpi);
        return FALSE;
    }

    /* Set up pre-processor config pointers. */
	(*cpi)->ScanConfig.Yuv0ptr = (*cpi)->yuv0ptr;
	(*cpi)->ScanConfig.Yuv1ptr = (*cpi)->yuv1ptr;
	(*cpi)->ScanConfig.SrfWorkSpcPtr = (*cpi)->ConvDestBuffer;
	(*cpi)->ScanConfig.disp_fragments = (*cpi)->pb.display_fragments;
    (*cpi)->ScanConfig.RegionIndex = (*cpi)->pb.pixel_index_table;
	(*cpi)->ScanConfig.HFragPixels = (UINT8)(*cpi)->pb.Configuration.HFragPixels;
	(*cpi)->ScanConfig.VFragPixels = (UINT8)(*cpi)->pb.Configuration.VFragPixels;

	/* Initialise the pre-processor module. */
	if(!ScanYUVInit((*cpi)->pp, &((*cpi)->ScanConfig)))
    {
        DeleteFragmentInfo(&(*cpi)->pb);
        DeleteFrameInfo(&(*cpi)->pb);
        EDeleteFragmentInfo(*cpi);
        EDeleteFrameInfo(*cpi);
        return FALSE;
    }

	// Set encoder flags.
	(*cpi)->DropFramesAllowed = CompConfig->AllowDF;
    (*cpi)->QuickCompress = CompConfig->QuickCompress;
	(*cpi)->AutoKeyFrameEnabled = CompConfig->AutoKeyFrameEnabled;
	(*cpi)->MinimumDistanceToKeyFrame = CompConfig->MinimumDistanceToKeyFrame;
	(*cpi)->ForceKeyFrameEvery = CompConfig->ForceKeyFrameEvery;
	(*cpi)->PreProcFilterLevel = CompConfig->NoiseSensitivity;
	(*cpi)->AutoKeyFrameThreshold = CompConfig->AutoKeyFrameThreshold;
	(*cpi)->Sharpness = CompConfig->Sharpness;

	// Initialise Motion compensation
	InitMotionCompensation(*cpi);

	/* Initialise the compression process. */
    CompressInit(*cpi);

    // Indicate that the next frame to be compressed is the first in the current clip.
    (*cpi)->ThisIsFirstFrame = TRUE;

	return TRUE;
}

/****************************************************************************
 * 
 *  ROUTINE       :     ChangeCompressorSetting
 *
 *  INPUTS        :     Which setting to change and the new value
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     Setting.
 *
 *  FUNCTION      :     Sets compressor settings.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CCONV ChangeCompressorSetting ( CP_INSTANCE *cpi, C_SETTING Setting, int Value )
{
	switch ( Setting )
	{

    case C_SET_KEY_FRAME:

        // Request a new key frame (all intra). 
		cpi->ThisIsKeyFrame = TRUE; 
        break;

    case C_SET_FIXED_Q:
		if ( (Value >= 0) && (Value < 64) )
			cpi->FixedQ = cpi->pb.QThreshTable[63 - Value];
        break;

    case C_SET_FIRSTPASS_FILE:
        strcpy(cpi->FirstPassFileName, (const char *) Value);
        break;

	case C_SET_NODROPS:
		cpi->NoDrops = Value;
		break;

	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     ChangeEncoderConfig
 *
 *  INPUTS        :     Updated encoder cpi->pb.Configuration
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     Setting.
 *
 *  FUNCTION      :     Updates the encoder cpi->pb.Configuration
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CCONV ChangeEncoderConfig ( CP_INSTANCE *cpi, COMP_CONFIG * CompConfig )
{
	cpi->DropFramesAllowed = CompConfig->AllowDF;
    cpi->QuickCompress = CompConfig->QuickCompress;
	cpi->AutoKeyFrameEnabled = CompConfig->AutoKeyFrameEnabled;
	cpi->MinimumDistanceToKeyFrame = CompConfig->MinimumDistanceToKeyFrame;
	cpi->ForceKeyFrameEvery = CompConfig->ForceKeyFrameEvery;
	cpi->PreProcFilterLevel = CompConfig->NoiseSensitivity;
	cpi->AutoKeyFrameThreshold = CompConfig->AutoKeyFrameThreshold;
	cpi->Sharpness = CompConfig->Sharpness;

    // Set target data rate. 
	cpi->Configuration.TargetBandwidth = (CompConfig->TargetBitRate * 1024) / 8;

	// Set the output frame rate.
    cpi->Configuration.OutputFrameRate = CompConfig->FrameRate;
	if ( cpi->Configuration.OutputFrameRate < 1 )
		cpi->Configuration.OutputFrameRate = 1;
	else if ( cpi->Configuration.OutputFrameRate > 30 )
		cpi->Configuration.OutputFrameRate = 30;

    // Set key frame data rate target and frequency
    cpi->KeyFrameDataTarget = (CompConfig->KeyFrameDataTarget * 1024) / 8;
    cpi->KeyFrameFrequency = CompConfig->KeyFrameFrequency;

  	// Calculate a new target bytes per frame allowing for predicted key frame frequency and size.
    if ( (INT32)cpi->Configuration.TargetBandwidth > ((cpi->KeyFrameDataTarget * cpi->Configuration.OutputFrameRate)/cpi->KeyFrameFrequency) )
    {
        cpi->frame_target_rate =  (INT32)((cpi->Configuration.TargetBandwidth - ((cpi->KeyFrameDataTarget * cpi->Configuration.OutputFrameRate)/cpi->KeyFrameFrequency)) / cpi->Configuration.OutputFrameRate);
    }
    else 
        cpi->frame_target_rate = 1;

	// Set the quality settings. 
	ConfigureQuality( cpi, CompConfig->Quality );
}

/****************************************************************************
 * 
 *  ROUTINE       :     EncodeFrameYuv
 *
 *  Arguments     :     YuvInputData  - Pointer to structure that defines input data.
 *
 *                      OutPutPtr     - Poniter to buffer for compressed output data.
 *
 *  RETURNS       :     Number of bytes in output buffer .
 *
 *  FUNCTION      :     Compresses a frame supplied in YUV format.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 CCONV EncodeFrameYuv( CP_INSTANCE *cpi, YUV_INPUT_BUFFER_CONFIG *  YuvInputData, unsigned char * OutPutPtr, unsigned int *is_key )
{
    UINT32 ret_val;
    INT32  i;
    unsigned char * LocalDataPtr;
    unsigned char * InputDataPtr;
	//int iskey;
	UINT8 iskey;

    // Reset the frame size monitor variable
    cpi->ThisFrameSize = 0;
	cpi->DataOutputBuffer = OutPutPtr;
	cpi->pb.DataOutputInPtr = cpi->DataOutputBuffer;

    // If frame size has changed create new mapping from quad-tree to fragment index
	if (  YuvInputData->YHeight != (INT32)cpi->pb.Configuration.VideoFrameHeight ||
		  YuvInputData->YWidth != (INT32)cpi->pb.Configuration.VideoFrameWidth )
	{
		/* Initialise the bitmap details. */ 
		cpi->pb.Configuration.VideoFrameHeight = YuvInputData->YHeight;
		cpi->pb.Configuration.VideoFrameWidth = YuvInputData->YWidth;

		// Note the height and width in the pre-processor (SCAN) control structure. 
		cpi->ScanConfig.VideoFrameHeight = YuvInputData->YHeight;
		cpi->ScanConfig.VideoFrameWidth = YuvInputData->YWidth;

        if(!InitFrameDetails(&cpi->pb))
        {
            return 0;
        }
        if(!EAllocateFragmentInfo(cpi))
        {
            DeleteFragmentInfo(&cpi->pb);
            DeleteFrameInfo(&cpi->pb);
            return 0;
        }
        
        if(!EAllocateFrameInfo(cpi))
        {
            DeleteFragmentInfo(&cpi->pb);
            DeleteFrameInfo(&cpi->pb);
            EDeleteFragmentInfo(cpi);
            return 0;
        }

		// InitFrameDetails MUST be called before CreateBlockMapping (cpi->pb.HFragments)
		//CreateBlockMapping ( YuvInputData->YWidth, YuvInputData->YHeight );
		//CreateBlockMapping ( cpi->pb.BlockMap, cpi->pb.YSuperBlocks, cpi->pb.UVSuperBlocks, cpi->pb.HFragments, cpi->pb.VFragments );

	    // Initialise Motion compensation
	    InitMotionCompensation(cpi);
    }

    // Copy over input YUV to internal YUV buffers.
    
    // First copy over the Y data
    LocalDataPtr = cpi->yuv1ptr;
    InputDataPtr = (unsigned char *)YuvInputData->YBuffer;
    for ( i = 0; i < YuvInputData->YHeight; i++ )
    {
        memcpy( LocalDataPtr, InputDataPtr, YuvInputData->YWidth );
        LocalDataPtr += YuvInputData->YWidth;
        InputDataPtr += YuvInputData->YStride;
    }

    // Now copy over the U data
    LocalDataPtr = &cpi->yuv1ptr[(YuvInputData->YHeight * YuvInputData->YWidth)];
    InputDataPtr = (unsigned char *)YuvInputData->UBuffer;
    for ( i = 0; i < YuvInputData->UVHeight; i++ )
    {
        memcpy( LocalDataPtr, InputDataPtr, YuvInputData->UVWidth );
        LocalDataPtr += YuvInputData->UVWidth;
        InputDataPtr += YuvInputData->UVStride;
    }

    // Now copy over the V data
    LocalDataPtr = &cpi->yuv1ptr[((YuvInputData->YHeight * YuvInputData->YWidth)*5)/4];
    InputDataPtr = (unsigned char *)YuvInputData->VBuffer;
    for ( i = 0; i < YuvInputData->UVHeight; i++ )
    {
        memcpy( LocalDataPtr, InputDataPtr, YuvInputData->UVWidth );
        LocalDataPtr += YuvInputData->UVWidth;
        InputDataPtr += YuvInputData->UVStride;
    }

    // Special case for first frame
    if ( cpi->ThisIsFirstFrame )
    {

		CompressFirstFrame(cpi);
		cpi->ThisIsFirstFrame = FALSE;
		cpi->ThisIsKeyFrame = FALSE;
    }
    else if ( cpi->ThisIsKeyFrame )
    {

        CompressKeyFrame(cpi);
	    cpi->ThisIsKeyFrame = FALSE;
    }
    else 
    {
   		/* Compress the frame. */
    	CompressFrame( cpi, (unsigned int) cpi->CurrentFrame );
    }

	/* Set the output bytes buffered count and reset the  buffer input pointer. */
	cpi->BufferedOutputBytes = cpi->pb.DataOutputInPtr - cpi->DataOutputBuffer;
	cpi->pb.DataOutputInPtr = cpi->DataOutputBuffer;
	
	/* Note the number of bytes to be returned. */
	ret_val = cpi->BufferedOutputBytes;

	/* Update the buffered bytes count to indicate that there are none available. */
	cpi->BufferedOutputBytes = 0;

	/* Update stats variables. */
	cpi->LastFrameSize = (UINT32)cpi->ThisFrameSize;
	cpi->CurrentFrame++;

	/* return whether or not we are a key frame */
	iskey=GetFrameType(&cpi->pb);
	if ( iskey == 0 )
		*is_key = 1;
	else 
		*is_key = 0;

    return ret_val;

}

/****************************************************************************
 * 
 *  ROUTINE       :     StopEncoder
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None .
 *
 *  FUNCTION      :     Stops the encoder and grabber
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
BOOL CCONV StopEncoder(CP_INSTANCE **cpi)
{

	if(*cpi)
	{

		DeleteFragmentInfo(&(*cpi)->pb);
		DeleteFrameInfo(&(*cpi)->pb);
		EDeleteFragmentInfo((*cpi));
		EDeleteFrameInfo((*cpi));
		
		/* Re-set Output buffer. */
		(*cpi)->BufferedOutputBytes = 0;
		
		DeleteCPInstance(cpi);

	}

	return TRUE;
}


