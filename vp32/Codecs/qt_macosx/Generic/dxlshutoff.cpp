//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////
//
// dxlshutoff.cpp
//
// Purpose: Shuts off certain dxer's. 
//
/////////////////////////////////////////////////////////////////////////



// shut off DXVs ability to do tm1,tmrt
extern "C" 
{
	int tm1_Init( int i)
	{
	    #pragma unused(i) 
		return 0;
	}
	
	int tmrt_Init( int i)
	{ 
		#pragma unused(i)
		return 0;
	}
	
	int tm1_Exit()
	{
		return 0;
	}
	
	int tmrt_Exit()
	{ 
		return 0;
	}
	
	int torq_Init( int i)
	{ 
		#pragma unused(i)
		return 0;
	}
	
	int torq_Exit()
	{ 
		return 0;
	}

}


