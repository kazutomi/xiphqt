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
*   Module Title :     CFrameW.C
*
*   Description  :     Functions to read and write frames.
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
#include <time.h>
#include "Compdll.h"
#include "SystemDependant.h"
#include "CBitman.h"
#include "CFrameW.h"
#include "codec_common.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

#define START_SIZE  0
#define END_SIZE    1
#define DEBUFFER_DELAY 100
 
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Module Statics
*****************************************************************************
*/              

//static UINT8 FrameType;                    /* The type of frame being decoded. */

void WriteFrameSynch();

/****************************************************************************
*  Forward References.
*****************************************************************************
*/              

void WriteFrameHeader( CP_INSTANCE *cpi)
{
    UINT32 i;

    // Output the frame type (base/key frame or inter frame)
    AddBitsToBuffer( cpi, (UINT32)cpi->pb.FrameType, 1 );
    
	// usused set to 0 allways
	AddBitsToBuffer( cpi, 0, 1 );

    // Write out details of the current value of Q... variable resolution. 
    for ( i = 0; i < Q_TABLE_SIZE; i++ )
    {
        if ( cpi->pb.ThisFrameQualityValue == cpi->pb.QThreshTable[i] )
        {
            AddBitsToBuffer( cpi, (UINT32)i, 6 );
            break;
        }
    }
    if ( i == Q_TABLE_SIZE )
    {
        // An invalid DCT value was specified. 
        IssueWarning( "Invalid Q Multiplier" );
        AddBitsToBuffer( cpi, (UINT32)31, 6 );
    }
  
	// If the frame was a base frame then write out the frame dimensions. 
	if ( cpi->pb.FrameType == BASE_FRAME )
	{
        AddBitsToBuffer( cpi, (UINT32)0, 8 );
        AddBitsToBuffer( cpi, (UINT32)cpi->pb.Vp3VersionNo, 5 );

        // Key frame type / method
        AddBitsToBuffer( cpi, (UINT32)cpi->pb.KeyFrameType, 1 );

        // Spare configuration bits
        AddBitsToBuffer( cpi, (UINT32)0, 2 );        
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     Write32ToBuffer
 *
 *  INPUTS        :     UINT8 * Data
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Writes 32 bits of data to an output buffer and converts
 *                      data to big endian mode
 *                          
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void Write32ToBuffer( CP_INSTANCE *cpi, UINT8 * Data )
{ 
    UINT8   BeData[4];

    // Reverse byte order to big endian
#ifdef CPUISLITTLEENDIAN
    BeData[3] = Data[0];
    BeData[2] = Data[1];
    BeData[1] = Data[2];
    BeData[0] = Data[3];
#else
    BeData[3] = Data[3];
    BeData[2] = Data[2];
    BeData[1] = Data[1];
    BeData[0] = Data[0];
#endif


    // Update the count of bytes so far this frame.
    cpi->ThisFrameSize += 4;

	*cpi->pb.DataOutputInPtr++ = BeData[0];
	*cpi->pb.DataOutputInPtr++ = BeData[1];
	*cpi->pb.DataOutputInPtr++ = BeData[2];
	*cpi->pb.DataOutputInPtr++ = BeData[3];
    cpi->BufferedOutputBytes = cpi->BufferedOutputBytes + 4;
}




