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


#include <stdlib.h>#include "duck_mem.h"#include <stddef.h>#include <string.h>#include <MacMemory.h>void *duck_memmove(void *dst, void *src, size_t length){	memmove(dst,src,length);}void *duck_malloc(size_t theSize, enum tmemtype fred){   	void *temp;	temp = NewPtr(theSize);	return temp;}void *duck_calloc(size_t n,size_t theSize, enum tmemtype  fred){   	void *temp;		temp = NewPtrClear(n * theSize);	return temp;}void duck_free(void *old_blk){  	DisposePtr( (char *)old_blk );}void *duck_memcpy(void *dest, const void *source, size_t length){		BlockMoveData(source, dest, length);}void *duck_memset(void *dest, int val , size_t length){	return memset(dest, val, length);}int  duck_strcmp(const char *one, const char *two){	return strcmp(one, two);}