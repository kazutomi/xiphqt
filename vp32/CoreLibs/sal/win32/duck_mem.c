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


/***********************************************\
??? duck_mem.c
\***********************************************/

#include <stdio.h> 
#include <string.h>
#include <dos.h> 
#include <time.h>
#include <malloc.h>
#include "duck_mem.h"
#include "duck_io.h"
//#include "duck_hfb.h"
#include "duck_dxl.h"


void *duck_memmove( void *dst, const void *src, size_t length )
{
    return memmove(dst, src, length);
}

void *duck_malloc(size_t size, dmemType foo)
{    
	void *temp = malloc(size);
	return temp;
}

void *duck_memset( void *dest, int c, size_t count )
{ 
    return((void *) memset(dest, c, count));
}                           

void *duck_calloc(size_t n,size_t size, dmemType foo)
{   
	void *temp = calloc(n, size);
	return temp;
}

void duck_free(void *old_blk)
{  
	free(old_blk);
}

void *duck_memcpy(void *dest, const void *source, size_t length)
{
		return memcpy(dest,source,length);
}

int duck_strcmp(const char *one, const char *two)
{
	return strcmp(one, two);
}


void set_memfuncs()
{
#if defined(DXV_DLL)
	DXV_Setmalloc(malloc);
	DXV_Setcalloc(calloc);
	DXV_Setfree(free);
#endif

#if defined(HFB_DLL)
	HFB_Setmalloc(malloc);
	HFB_Setcalloc(calloc);
	HFB_Setfree(free);
#endif
}
