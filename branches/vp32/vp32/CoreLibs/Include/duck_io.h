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


#ifndef _duck_io_h
#define _duck_io_h

#if defined(__cplusplus)
extern "C" {
#endif

int duck_open(const char *fname, unsigned long userData); 
void duck_close(int ghndl);

long duck_read(int ghndl,unsigned char *buf, long nbytes);
long duck_seek(int gHndl,long offs, int origin);

int duck_readFinished(int han, int flag); /* FWG 7-9-99 */

#if defined(__cplusplus)
}
#endif

#endif
