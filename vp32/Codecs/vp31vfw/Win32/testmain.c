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


#include <windows.h>
#include <vfw.h>
#include "duck_dxl.h"
#include <stdio.h>
#include "cclib.h"
extern void VPEInitLibrary(void);
extern void VPEDeInitLibrary(void);

BOOL CALLBACK DllMain(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved);

HMODULE ghModule=0;
HINSTANCE hInstance=0;        
unsigned long cProcessesAttached = 0;         


// call the library initialization functions
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
	        DXL_InitVideoEx(64,64);
			VPEInitLibrary();
			InitCCLib(SpecialProc);
 			return TRUE;
		}
	}
	else if ( fdwReason == DLL_PROCESS_DETACH )
	{
        if ( !(--cProcessesAttached) )
		{
			DeInitCCLib();
	        DXL_ExitVideo(); 
			VPEDeInitLibrary();
		}
		return FALSE; 
	}
	else 
		return FALSE;
}

