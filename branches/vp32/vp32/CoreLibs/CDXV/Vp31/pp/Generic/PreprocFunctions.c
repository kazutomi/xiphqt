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
*   Module Title :     PreProcFunctions.c
*
*   Description  :     
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */

#include "Preproc.h"
#pragma warning( disable : 4799 )  // Disable no emms instruction warning!

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

/****************************************************************************
*  Imports.
*****************************************************************************
*/   
    
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


/****************************************************************************
*  Forward References
*****************************************************************************
*/  

/****************************************************************************
 * 
 *  ROUTINE       :     MachineSpecificConfig
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Checks for machine specifc features such as MMX support 
 *                      sets approipriate flags and function pointers.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
#define MMX_ENABLED 1
void MachineSpecificConfig(PP_INSTANCE *ppi)
{
    UINT32 FeatureFlags = 0;
    ppi->RowSAD = ScalarRowSAD;
    ppi->ColSAD = ScalarColSAD;
}


/****************************************************************************
 * 
 *  ROUTINE       :     ClearMmxState()
 *
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Clears down the MMX state
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void ClearMmxState(PP_INSTANCE *ppi)
{
	return;
}
