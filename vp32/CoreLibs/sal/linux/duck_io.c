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
??? duck_io.c
\***********************************************/

#include <stdio.h> 
#include <string.h>
/* #include <io.h> */
#include <fcntl.h>
#include "duck_io.h"
#include "duck_hfb.h"

int duck_readFinished(int han, int flag)
{
	return 1;
}

int duck_open(const char *name, unsigned long userData)
{
    int f;                                       
    char filename[80];
#if 0
    if(name[strlen(name)-4] != '.') {   /*no extension, try .AVI */
        sprintf(filename,"%s.AVI",name);
        f = open(filename,O_RDONLY);
        return(f);
    }
    strcpy(filename,name);
    return(open(filename,O_RDONLY));
#else
{
	f = open(name,O_RDONLY);
	return f;
}
#endif
}

void duck_close(int handle)
{
	close(handle);
}

long duck_read(int handle,unsigned char *buffer,long bytes)
{
	if (buffer == NULL){
		duck_seek(handle,bytes,SEEK_CUR);
		return bytes;
	}
		
	return(read(handle,buffer,bytes));
}

long duck_seek(int handle,long offset,int origin)
{
	return(lseek(handle,offset,origin));
}

/*
void set_iofuncs()
{
	HFB_Setopen(open);
	HFB_Setclose(close);
	HFB_Setread(read);
	HFB_Setseek(lseek);
}
*/

