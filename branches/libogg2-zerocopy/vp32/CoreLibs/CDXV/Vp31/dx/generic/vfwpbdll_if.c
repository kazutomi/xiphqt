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
*   Module Title :     vfwpbdll_if.c
*
*   Description  :     Video codec demo playback dll interface
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <stdio.h> 

#if defined(MACPPC)

#define __try  

#endif

#include "Huffman.h"
#include "pbdll.h"
#include "blockmapping.h"
#include <math.h>
#include "vfw_pb_interface.h"
#include "VP31DVERSION.h"
#define CommentString "\nON2.COM VERSION VP31D " VP31DVERSION "\n"
#pragma comment(exestr,CommentString)

/****************************************************************************
*  Implicit Imports
*****************************************************************************
*/        

extern unsigned int CPUFrequency;

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
 
/****************************************************************************
*  Exported data structures.
*****************************************************************************
*/        


/****************************************************************************
*  Module statics.
*****************************************************************************
*/        


static const char vp31dVersion[] = VP31DVERSION;

/****************************************************************************
*  Imports
*****************************************************************************
*/
extern void CCONV ScaleOrCenter( PB_INSTANCE (*pbi), YUV_BUFFER_CONFIG * YuvConfig );

#if !defined(MACPPC)   
#if defined(POSTPROCESS)
extern void PostProcess(PB_INSTANCE *pbi);
const unsigned long PP_MACHINE_LOWLIMIT = 350; //Lowest CPU (MHz) to enable PostProcess
const unsigned long PP_MACHINE_MIDLIMIT = 400; //Lowest CPU (MHz) to enable PostProcess
const unsigned long PP_MACHINE_TOPLIMIT = 590; //Lowest CPU (MHz) to enable PostProcess
/*
const unsigned long PP_MACHINE_LOWLIMIT = 340; //Lowest CPU (MHz) to enable PostProcess
const unsigned long PP_MACHINE_MIDLIMIT = 440; //Lowest CPU (MHz) to enable PostProcess
*/
#endif
#endif

extern void InitialiseConfiguration(PB_INSTANCE *pbi);
//extern UINT32 FrameSize;  /* The number of bytes in the frame (read from the frame header). */


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
 *  ROUTINE       :     VP31D_GetVersionNumber
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
const char * CCONV VP31D_GetVersionNumber(void)
{
    return vp31dVersion;
}

/****************************************************************************
 * 
 *  ROUTINE       :     StartDecoder
 *
 *  INPUTS        :     The handle of the display window.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     TRUE if succeeds else FALSE.
 *
 *  FUNCTION      :     Starts the compressor grabber
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
BOOL CCONV StartDecoder( PB_INSTANCE **pbi, UINT32 ImageWidth, UINT32 ImageHeight )
{
    __try
    {
		// set up our structure holding all formerly global information about a playback instance
		*pbi = CreatePBInstance();

        // Validate the combination of height and width.
		(*pbi)->Configuration.VideoFrameWidth = ImageWidth;
		(*pbi)->Configuration.VideoFrameHeight = ImageHeight;
        
        (*pbi)->ProcessorFrequency = CPUFrequency;
		// Fills in fragment counts as well
		if(!PbBuildBitmapHeader( (*pbi), (*pbi)->Configuration.VideoFrameWidth, (*pbi)->Configuration.VideoFrameHeight ))
        {
            DeletePBInstance(pbi);
            return FALSE;
        }

        /* Initialise the input buffer pointers. */

	    /* Set last_dct_thresh to an illegal value to make sure the
	    *  Q tables are initialised for the new video sequence. 
	    */
	    (*pbi)->LastFrameQualityValue = 0;

	    /* Clear various flags. */
	    (*pbi)->DecoderErrorCode = NO_DECODER_ERROR;

	    // Set up various configuration parameters.
	    InitialiseConfiguration(*pbi);

        // Clear down the YUVtoRGB conversion skipped list.
        memset( (*pbi)->skipped_display_fragments, 0, (*pbi)->UnitFragments );



	    return TRUE;
    }

#if !defined(MACPPC)
    __except( TRUE )
    {
        ErrorTrap( *pbi, GEN_EXCEPTIONS );
	    return FALSE;
    }
#else
//    catch(BOOL X)
//    {
//        ErrorTrap( *pbi, GEN_EXCEPTIONS );
//	    return FALSE;
//    }
#endif

}
/****************************************************************************
 * 
 *  ROUTINE       :     GetPbParam
 *
 *  INPUTS        :     PB_COMMAND_TYPE Command
 *                      char *          Parameter
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *	
 *  FUNCTION      :     Generalised command interface to decoder.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CCONV GetPbParam( PB_INSTANCE *pbi, PB_COMMAND_TYPE Command, UINT32 *Parameter )
{
	switch ( Command )
	{
#if defined(POSTPROCESS)
	case PBC_SET_POSTPROC:
		*Parameter =pbi->PostProcessingLevel;
#endif

	default:
        break;
	}
}

#if !defined(MACPPC)    
/****************************************************************************
 * 
 *  ROUTINE       :     SetPbParam
 *
 *  INPUTS        :     PB_COMMAND_TYPE Command
 *                      char *          Parameter
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *	
 *  FUNCTION      :     Generalised command interface to decoder.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CCONV SetPbParam( PB_INSTANCE *pbi, PB_COMMAND_TYPE Command, UINT32 Parameter )
{
#if defined(POSTPROCESS)
	switch ( Command )
	{
    case PBC_SET_CPUFREE:
    {
        double Pixels = pbi->Configuration.VideoFrameWidth * pbi->Configuration.VideoFrameHeight;
        double FreeMhz = pbi->ProcessorFrequency * Parameter / 100;
        double PixelsPerMhz = 100 * sqrt(1.0*Pixels) / FreeMhz;

        pbi->CPUFree = Parameter;
        
        if( PixelsPerMhz > 100 )
            pbi->PostProcessingLevel = 0;
        else if( PixelsPerMhz > 90 )
            pbi->PostProcessingLevel = 4;
        else if( PixelsPerMhz > 80 )
            pbi->PostProcessingLevel = 5;
        else
            pbi->PostProcessingLevel = 6;
        break;
    }
	case PBC_SET_POSTPROC:
		if( Parameter == 9 )				
		{
            SetPbParam( pbi, PBC_SET_CPUFREE, 70);
		}
		else
			pbi->PostProcessingLevel = Parameter;

		break;

	default:
        break;
	}
#endif
}
#endif

/****************************************************************************
 * 
 *  ROUTINE       :     GetYUVConfig
 *
 *  INPUTS        :     YUV_BUFFER_CONFIG  * YuvConfig
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *	
 *  FUNCTION      :     Gets details of the reconstruction buffer
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CCONV GetYUVConfig( PB_INSTANCE (*pbi), YUV_BUFFER_CONFIG * YuvConfig )
{
    __try
    {
		
		YuvConfig->YWidth = pbi->Configuration.VideoFrameWidth;
		YuvConfig->YHeight = pbi->Configuration.VideoFrameHeight;
		YuvConfig->YStride = pbi->Configuration.YStride;
		
		YuvConfig->UVWidth = pbi->Configuration.VideoFrameWidth / 2;
		YuvConfig->UVHeight = pbi->Configuration.VideoFrameHeight / 2;
		YuvConfig->UVStride = pbi->Configuration.UVStride;
		
#if defined(POSTPROCESS)
		if(pbi->PostProcessingLevel)
		{
			YuvConfig->YBuffer = &pbi->PostProcessBuffer[pbi->ReconYDataOffset];
			YuvConfig->UBuffer = &pbi->PostProcessBuffer[pbi->ReconUDataOffset];
			YuvConfig->VBuffer = &pbi->PostProcessBuffer[pbi->ReconVDataOffset];
		}
		else
#endif
		{
			YuvConfig->YBuffer = &pbi->LastFrameRecon[pbi->ReconYDataOffset];
			YuvConfig->UBuffer = &pbi->LastFrameRecon[pbi->ReconUDataOffset];
			YuvConfig->VBuffer = &pbi->LastFrameRecon[pbi->ReconVDataOffset];
		}
        
    }
#if !defined(MACPPC)    
    __except ( TRUE )
    {
        ErrorTrap( pbi, GEN_EXCEPTIONS );
    }    
#else
//    catch(BOOL X)
//    {
//        ErrorTrap( pbi, GEN_EXCEPTIONS );
//    }    
#endif
}

/****************************************************************************
 Debugging Aid Only
*/
void writeframe(PB_INSTANCE *pbi, char * address,int x)
{
	// write the frame
	FILE *yframe;
	char filename[255];
	sprintf(filename,"d:\\y%d.raw",x);
	yframe=fopen(filename,"wb");
	fwrite(address,pbi->ReconYPlaneSize,1,yframe);
	fclose(yframe);
}

/****************************************************************************
 * 
 *  ROUTINE       :     DecodeFrameToYUV
 *
 *  INPUTS        :     UINT8 * VideoBufferPtr
 *                              Compressed input video data
 *
 *                      UINT32  ByteCount 
 *                              Number of bytes compressed data in buffer. *  
 *
 *                      UINT32  Height and width of image to be decoded
 *
 *  OUTPUTS       :     None
 *                      None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Decodes a frame into the internal YUV reconstruction buffer.
 *                      Details of this buffer can be obtained by calling GetYUVConfig().
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
int CCONV DecodeFrameToYUV( PB_INSTANCE (*pbi), char * VideoBufferPtr, unsigned int ByteCount,
                             UINT32 ImageWidth,     UINT32 ImageHeight )
{
    __try
    {
        // Sanity check to make sure that the decoder has been initialised
        // If it has not then we must initialise it before we proceed.

        // Clear any outstanding error code .... start with a clean slate.
        pbi->DecoderErrorCode = NO_DECODER_ERROR;

        // Indicate that no YUV to RGB required.
        pbi->OutputRGB = FALSE;

        //	Copy the compressed video data into the internal. 
        //memcpy( pbi->DataInputBuffer, VideoBufferPtr, ByteCount );
		pbi->br.bitsinremainder = 0;
		pbi->br.position = (cuchar *)VideoBufferPtr;

   	    // Decode the frame. 
	    if(LoadAndDecode(pbi)==-1)
            return -1;

#if defined(POSTPROCESS)
		if(pbi->PostProcessingLevel)
		{
			PostProcess(pbi);
		}
#endif

    }
#if !defined(MACPPC)    
    __except ( TRUE )
    {
        ErrorTrap( pbi, GEN_EXCEPTIONS );
        return -2;
    }
#else
//    catch(BOOL X)
//    {
//        ErrorTrap( pbi, GEN_EXCEPTIONS );
//    }
#endif    
    return 0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     StopDecoder
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
BOOL CCONV StopDecoder(PB_INSTANCE **pbi)
{
    __try
    {
		if(*pbi)
		{
			DeleteFragmentInfo(* pbi);
			DeleteFrameInfo(* pbi);
			DeletePBInstance(pbi);
		
	        return TRUE;
		}
    }
#if !defined(MACPPC)        
    __except ( TRUE )
    {
        ErrorTrap( *pbi, GEN_EXCEPTIONS );
		return FALSE;
    }
#else
//    catch (BOOL X)
//    {
//        ErrorTrap( *pbi, GEN_EXCEPTIONS );
//		return FALSE;
//    }
#endif    
	return TRUE;
}

/****************************************************************************
 * 
 *  ROUTINE       :     ErrorTrap
 *
 *  INPUTS        :     Nonex.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Called when a fatal error is detected.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 void ErrorTrap( PB_INSTANCE *pbi, int ErrorCode )
 {
     // Flag the error unless there is already one pending
     if ( pbi->DecoderErrorCode == NO_DECODER_ERROR )
     {
	     pbi->DecoderErrorCode = ErrorCode;
     }

 }


