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


#ifndef _regentry_h
#define _regentry_h


#if TARGET_OS_MAC
#include <Components.h>
	typedef struct {
		ComponentInstance theCodec;
		short resFileID;
		short oldID;
	} RegistryAccessStruct, *RegistryAccess;
#else 
	typedef char RegistryAccessStruct[1024],*RegistryAccess;
#endif


typedef enum REG_TYPE_TEMP { 
	REG_CSTRING=0, 
	REG_UNSIGNED_LONG=1, 
	REG_INTEGER=2, 
	REG_PROFILE=3,
	REG_DOUBLE=4
} REGISTRY_TYPE;
 

#ifdef __cplusplus
extern "C" {
#endif  
 
int Registry_GetEntry(void *data, REGISTRY_TYPE r, 
unsigned long *sizeItem, char *nameItem, RegistryAccess regAccess);

int Registry_SetEntry(void *data, REGISTRY_TYPE r, 
unsigned long sizeItem, char *nameItem, RegistryAccess regAccess);

int Registry_Open(RegistryAccess regAccess, char *mode);

int Registry_Close(RegistryAccess regAccess);

#ifdef __cplusplus
}
#endif


#endif /* include guard */