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


#include "dkstring.h"


char *PtoCStr(unsigned char *s)
{
int theLen;
int t;

theLen = s[0];

for(t=0;t<theLen;t++)
    s[t] = s[t+1];
   
s[theLen] = '\0';

return (char *) s;
}

unsigned char *CtoPStr(char *s)
{
int theLen;
int t;

theLen = strlen(s);

for(t=theLen;t>=1;t--)
    s[t] = s[t-1];
   
s[0] = (char ) theLen;

return (unsigned char *) s;
}


