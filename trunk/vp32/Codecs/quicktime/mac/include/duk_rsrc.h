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


#ifndef _duk_rsrc_h
#define _duk_rsrc_h 1

#include "dkstring.h"

#include <Resources.h>

#ifdef __cplusplus
extern "C" {
#endif

void SetDukCurResFile(short i);

short GetDukCurResFile(void);

int DukAddResourceViaPtr(char *thePtr, ResType theType,char *name);

char *DukGetStrResource(char *thePtr, ResType theType,char *name);

unsigned char *DukGetVersionResourceShortString(unsigned char *thePtr, ResType theType,char *name);
	
unsigned char *DukGetCustomResource(unsigned char *thePtr, ResType theType,char *name, unsigned long *sizeData);

int DukSetStrResource(char *thePtr, ResType theType,char *name);

int DukSetCustomResource(unsigned char *data, unsigned long sizeData, ResType theType, char *name);

int DukSetCustomResourceWithFileData(char *filename,ResType theType,char *resName);

unsigned char *DukGetPStrResource(unsigned char *thePtr, ResType theType,char *name);

#ifdef __cplusplus
} /* extern C */
#endif


#endif
