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


//#include <Files.h>
//#include <Folders.h>
//#include <LowMem.h>
//#include <string.h>
/*
folderType -- same as FindFile
send 0 for SystemFolder

path -- return value is full path to folder
*/
#ifndef getsysfolder_h
#define getsysfolder_h

#ifdef __cplusplus
extern "C" {
#endif

short GetSystemFolderPath(OSType folderType, char *path);

#ifdef __cplusplus
}
#endif

#endif