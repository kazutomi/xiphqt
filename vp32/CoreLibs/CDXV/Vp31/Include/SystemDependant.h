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
*   Module Title :     SystemDependant.h
*
*   Description  :     Miscellaneous system dependant functions header
*
*****************************************************************************
*/

/*******************************************3*********************************
*  Header Files
*****************************************************************************
*/

/****************************************************************************
*  Constants
*****************************************************************************
*/

/****************************************************************************
*  Types
*****************************************************************************
*/        

/****************************************************************************
*   Data structures
*****************************************************************************
*/


/****************************************************************************
*  Functions
*****************************************************************************
*/

// Test machine config

// Misc.
extern void IssueWarning( char * WarningMessage );

// System dynamic memory allocation
char * SytemGlobalAlloc( unsigned int Size );   
void SystemGlobalFree( char * MemPtr );

