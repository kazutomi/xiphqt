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
#include <io.h>
#include <fcntl.h>
#include <windows.h>

#include "duck_io.h"

int duck_open(const char *name, unsigned long userData) 
{
    char filename[255];
	(void) userData;

    if(name[strlen(name)-4] != '.') {   /*no extension, try .AVI */
        sprintf(filename,"%s.AVI",name);
        //f = open(filename,O_BINARY|O_RDONLY);
        //return(f);
    }else
		strcpy(filename,name);
    //return(open(filename,O_BINARY|O_RDONLY));
	return (int)CreateFile(filename,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING,NULL);
}

void duck_close(int handle)
{
	//close(handle);
	CloseHandle((void *)handle);
}

long duck_read(int handle,unsigned char *buffer,long bytes)
{
	long bytesRead;

	if (buffer == NULL){
		duck_seek(handle,bytes,SEEK_CUR);
		return bytes;
	}

	ReadFile((void *)handle,buffer,bytes,&bytesRead,NULL);
	//return(read(handle,buffer,bytes));


	return bytesRead;
}

long duck_seek(int handle,long offset,int origin)
{
	//return(lseek(handle,offset,origin));
	return(SetFilePointer((HANDLE) handle,offset,NULL,origin));
}

int duck_readFinished(int han, int flag)
{
	return 1;
}

void set_iofuncs()
{

//	HFB_Setopen(duck_open);
//	HFB_Setclose(duck_close);
//	HFB_Setread(duck_read);
//	HFB_Setseek(duck_seek);
}
