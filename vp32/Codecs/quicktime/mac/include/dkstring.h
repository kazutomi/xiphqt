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


#ifndef _dkstring_h
#define _dkstring_h

#include <string.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif


int stricmp(char *a, char *b);

int strnicmp(char *a, char *b, int n);

int duck_lowercase(char *a);

int strnset(char *s, int c, size_t count);

char *PtoCStr(unsigned char *s);

unsigned char *CtoPStr(char *s);


#if defined(__cplusplus)
}
#endif

#endif
