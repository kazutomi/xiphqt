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


#include <unistd.h>#include <fcntl.h>#include <stdio.h>#include "duck_io.h"/*duk_file : These functions are critical to the proper functioning of HFB (and thus DXL)           They allow HFB to be cross-platform by hiding the details of            file access from HFB.*/int Duk_MessageBox(char *msg, char *title);int Duk_ExitMessageBox(char *msg);int duck_readFinished(int ksdfj, int sdjk);int duck_readFinished(int ksdfj, int sdjk){        return 1;}long duck_read(int filedes, unsigned char *buf, long  count);long duck_read(int filedes, unsigned char *buf, long  count){ int i;  i = count;return (long) read(filedes, (char *) buf,i);}long duck_seek(int fildes, long offset, int whence){return lseek(fildes,offset,whence);}void duck_close(int fildes){close(fildes);}int duck_open(const char *filename);int duck_open(const char *filename){ int r;  char msg[255];;     r = open(filename, O_RDONLY | O_BINARY);      if (r < 0) {  		sprintf(msg,"Couldn't open %s",filename);    	//Duk_MessageBox(msg,"duk_file.c");		}       return r;}int duck_open_w_mode(const char *filename, int mode);int duck_open_w_mode(const char *filename, int mode){ int r;  char msg[255];  mode = 1; /* quiet warnings about unused */     r = open(filename, O_RDONLY | O_BINARY);      if (r < 0) {  		sprintf(msg,"Couldn't open %s",filename);    	Duk_MessageBox(msg,"Duck");		}       return r;}FILE *duck_fopen(char *name, char *mode);FILE *duck_fopen(char *name, char *mode){ FILE *fp;char msg[255];fp = fopen(name,mode);if (!fp) {	sprintf(msg,"Couldn't open %s",name);    Duk_MessageBox(msg,"Duck");	}return fp;}ÿ