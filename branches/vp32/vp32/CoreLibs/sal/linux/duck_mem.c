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
//#include <dos.h> 
#include <time.h>
#include <malloc.h>
#include "duck_mem.h"
#include "duck_io.h"
#include "duck_hfb.h"
#include "duck_dxl.h"

#define CHECK_FOR_MEMORY_LEAK 0

void *duck_malloc(size_t size, dmemType foo)
{    
	void *temp = malloc(size);

#if CHECK_FOR_MEMORY_LEAK
{
	FILE * out;
	
	if ((out = fopen("c:\\sjl.log","a")) != NULL) {
        fprintf(out,"duck_malloc:%x\n", temp);
        fclose(out);
    }
}
#endif

	return temp;
}

/*
void *duck_memset( void *dest, int c, size_t count )
{ 
    return((void *) memset(dest, c, count));
}                           
*/

void *duck_calloc(size_t n,size_t size, dmemType foo)
{   
	void *temp = calloc(n, size);

#if CHECK_FOR_MEMORY_LEAK
{
	FILE * out;
	
	if ((out = fopen("c:\\sjl.log","a")) != NULL) {
        fprintf(out,"duck_calloc:%x\n", temp);
        fclose(out);
    }
}
#endif

	return temp;
}

void duck_free(void *old_blk)
{  

#if CHECK_FOR_MEMORY_LEAK
{
	FILE * out;
	
	if ((out = fopen("c:\\sjl.log","a")) != NULL) {
        fprintf(out,"duck_free:%x\n", old_blk);
        fclose(out);
    }
}
#endif

	free(old_blk);
}

void *duck_memcpy(void *dest, const void *source, size_t length)
{
	return memcpy(dest, source, length);
}

void *duck_memset(void *dest, int val , size_t length)
{
	return memset(dest, val , length);
}


int duck_strcmp(const char *a, const char *b)
{
	return strcmp(a,b);
}



/*
void set_memfuncs()
{
	DXV_Setmalloc(malloc);
	DXV_Setcalloc(calloc);
	DXV_Setfree(free);

	HFB_Setmalloc(malloc);
	HFB_Setcalloc(calloc);
	HFB_Setfree(free);
}
*/
