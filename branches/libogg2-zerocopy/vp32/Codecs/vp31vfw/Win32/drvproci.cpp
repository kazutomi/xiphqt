///////////////////////////////////////////////////////////////////////////
//
// drvproci.cpp
//
// Copyright (c) 1998 The Duck Corporation.  All Rights Reserved.
//
// Authors: James Bankoski
//
// Purpose: Main message loop for codec.  All messages come through this 
// procedure.
#include "vpvfw.h"
#include "cclib.h"
#include "regentry.h"

// I need hinstance to open the dialog box from the dlls resource fork I'm 
// not sure how else to do it. It could be namespaced or thrown into a 
// static variable of vfwcodec of course
HINSTANCE hInstance=0;        
unsigned long cProcessesAttached = 0;         

extern "C" 
{
	int vp31_Init(void);
	int vp31_Exit(void);
}


//****************************************************************************
// DriverProc: All the calls for the dll go here, its the main message handler
//****************************************************************************
// All outside calls go through here.
LRESULT PASCAL DriverProc
(
	DWORD dwDriverID, 
	HDRVR hDriver, 
	UINT uiMessage, 
	LPARAM lParam1, 
	LPARAM lParam2
) 

{
  vfwCodec &thisCodec = *(reinterpret_cast <vfwCodec *> (dwDriverID));

  switch (uiMessage) 
  {
    case DRV_LOAD:

		#ifdef DXLVFW_LOGGING
		unsigned long sizeItem;
		char filename[400];

		Registry_GetEntry(
			&filename,
			REG_CSTRING,
			&sizeItem,
			"strLogFile", 
			const_cast <char *>(vfwCodec::registryEntry));

		if (strlen(filename)>0)
			file.open(filename, ios::app);
		#endif

		// initialize all the 'c' libraries we use 
		InitCCLib(SpecialProc);
		VPEInitLibrary();
		DXL_InitVideoEx(64,64);
		vp31_Init();

		return (LRESULT)DRV_OK;

    case DRV_FREE:

		// close all the 'c' libraries that we use
		VPEDeInitLibrary();
	    DXL_ExitVideo(); 
		vp31_Exit();
		DeInitCCLib();

		#ifdef DXLVFW_LOGGING
			if (file.is_open()) file.close();
		#endif

		return (LRESULT)DRV_OK;

    case DRV_OPEN:
		return reinterpret_cast <LRESULT> (vfwCodec::Open(reinterpret_cast<ICOPEN*> (lParam2)));

	case DRV_CLOSE:  
        thisCodec.Close();
        delete &thisCodec;

        return DRV_OK;

    case DRV_QUERYCONFIGURE:    // configuration from drivers applet
		return (LRESULT)DRV_OK;

	case DRV_INSTALL:
	case DRV_REMOVE: 
    case DRV_CONFIGURE:
	case DRV_DISABLE:
	case DRV_ENABLE:
		return DRV_OK;

	default:  
        return vfwCodec::driverProc( dwDriverID, hDriver, uiMessage,lParam1,lParam2);
  }
}

//****************************************************************************
// DllMain: loading the dll in memory or removing it from memory (it may be possible
// to remove this but I definitely need hinstance stored 
//****************************************************************************
BOOL CALLBACK DllMain
(
	HANDLE hInst, 
	DWORD fdwReason, 
	LPVOID lpReserved
)
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
        hInstance = static_cast <HINSTANCE> (hInst);
		++cProcessesAttached;
	}
	else if ( fdwReason == DLL_PROCESS_DETACH )
	{
		--cProcessesAttached;
	}

	return TRUE;
}

