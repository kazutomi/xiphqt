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
#include "duck_hfb.h"

int duck_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2); 
}


