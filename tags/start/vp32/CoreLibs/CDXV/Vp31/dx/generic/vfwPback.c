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
*   Module Title :     VFWPBack.C
*
*   Description  :     Video CODEC playback module
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */

//#include "Timer.h"
#include "Quantize.h"

#include "pbdll.h"
#include "blockmapping.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
                
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/  

int LoadAndDecode(PB_INSTANCE *pbi);

/****************************************************************************
*  Imports 
*****************************************************************************
*/  
extern void UpdateUMVBorder( PB_INSTANCE *pbi, UINT8 * DestReconPtr );
extern void IssueWarning( char * WarningMessage );

/****************************************************************************
*  Module Static Variables
*****************************************************************************
*/  


/****************************************************************************
*  Forward References
*****************************************************************************
*/  

/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/


/* Last Inter frame DC values */
//extern Q_LIST_ENTRY InvLastInterDC;
//extern Q_LIST_ENTRY InvLastIntraDC;

/****************************************************************************
 * 
 *  ROUTINE       :     LoadAndDecode
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Loads and decodes a frame.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
int LoadAndDecode(PB_INSTANCE *pbi)
{    
    UINT32  i;
    BOOL    LoadFrameOK;

    // Reset the DC predictors.
    pbi->InvLastIntraDC = 0;
    pbi->InvLastInterDC = 0;

    /* Load the next frame. */
    LoadFrameOK = LoadFrame(pbi); 
            
    if ( LoadFrameOK )
    {
        if ( (pbi->ThisFrameQualityValue != pbi->LastFrameQualityValue) )
        {
            /* Initialise DCT tables. */
            UpdateQ( pbi, pbi->ThisFrameQualityValue );  

            pbi->LastFrameQualityValue = pbi->ThisFrameQualityValue;    
        }   
        
       
        /* Decode the data into the fragment buffer. */
        DecodeData(pbi);                    

    }

    // If there have been any changes and we want RGB output then convert.
    if ( LoadFrameOK )
    {   
        if ( pbi->OutputRGB )
        {
            if ( !pbi->SkipYUVtoRGB )
            {
                if ( pbi->FramesHaveBeenSkipped )
                {
                    // Copy additional fragments for YUV conversion into the display fragments list.
                    for ( i = 0; i < pbi->UnitFragments; i++ )
                    {
                        if ( pbi->skipped_display_fragments[i] )
                            pbi->display_fragments[i] = 1;
                    }

                    // The YUV conversion is now up to date so clear the skipped 
                    // display fragments list.
                    memset( pbi->skipped_display_fragments, 0, pbi->UnitFragments );

                    // Clear the skipped flag... we are up to date
                    pbi->FramesHaveBeenSkipped = FALSE;
                }            

                // Update the display bitmap from the YUV411 structure. 
                CopyYUVtoBmp(pbi,pbi->LastFrameRecon, pbi->bmp_dptr0, FALSE, TRUE  );
            }
            else
            {
                pbi->FramesHaveBeenSkipped = TRUE;

                // Note fragments that need YUV conversion at a later date.
                for ( i = 0; i < pbi->UnitFragments; i++ )
                {
                    if ( pbi->display_fragments[i] )
                        pbi->skipped_display_fragments[i] = 1;
                }

            }
        }
        return 0;
    }
    else
    {
        // Test this condition. .. ??? is it an error
        return -1;
		IssueWarning( "Frame Load Error" );
    }

}                          



