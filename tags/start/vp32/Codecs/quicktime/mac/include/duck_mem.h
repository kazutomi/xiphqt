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


#ifndef _duck_mem_h
#define _duck_mem_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tmemtype {
	DMEM_GENERAL = 0,
	DMEM_TEMP,
	DMEM_CDBUFF,
	DMEM_FRAMEBUFF
} dmemType;

/* 
	size_t should be found in stddef.h on most compilers
	if necessary you can define this to be an unsigned int
*/

void *duck_malloc(size_t, dmemType);
void *duck_calloc(size_t, size_t, dmemType);
void duck_free(void *);

void *duck_memcpy(void *dest, const void *source, size_t length);
void *duck_memset(void *dest, int val , size_t length);
int duck_strcmp(const char *one, const char *two);

#ifdef __cplusplus
}
#endif

#endif
