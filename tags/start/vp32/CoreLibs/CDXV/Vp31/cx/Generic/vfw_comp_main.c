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
*   Module Title :     VFW_COMP_MAIN.c
*
*   Description  :     Main for video codec demo compression dll
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#define INC_WIN_HEADER      1
#include <windows.h>

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
 
/****************************************************************************
*  Module statics.
*****************************************************************************
*/        

unsigned long cProcessesAttached = 0;         

HINSTANCE hInstance;        /* Application instance handle. */

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Imports
*****************************************************************************
*/
extern void VPEInitLibrary(void);
extern void VPEDeInitLibrary(void);


BOOL WINAPI DllMain(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
        hInstance = hInst;
		if ( cProcessesAttached++ )
		{	
			return(TRUE);         // Not the first initialization.
    	}
		else
		{
			// initialize all the global variables in the dll
			VPEInitLibrary();

			return TRUE;
		}
	}

	else if ( fdwReason == DLL_PROCESS_DETACH )
	{
		if (--cProcessesAttached)
		{
			return TRUE;
		}
		else
		{
			VPEDeInitLibrary();
			return TRUE;
		}
	}
	else
		return FALSE;
}


