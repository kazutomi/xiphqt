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


#ifdef VP3_COMPRESS

#include "vfw_comp_interface.h"
#include <windows.h>

//BOOL CALLBACK DllMain(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
HMODULE ghModule;
HINSTANCE hInstance;        // Application instance handle. 
unsigned long cProcessesAttached = 0;         

BOOL CALLBACK DllMain(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
        hInstance = hInst;
        ghModule = hInst;

        if ( cProcessesAttached++ )
		{	
			return(TRUE);         // Not the first initialization.
    	}
		else
		{
			VPEInitLibrary();
			return TRUE;
		}
	}
	else if ( fdwReason == DLL_PROCESS_DETACH )
	{
        if ( !(--cProcessesAttached) )
			VPEDeInitLibrary();
		return FALSE; 
	}
	else 
		return FALSE;
}

#endif
