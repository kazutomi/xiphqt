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


#include <stdio.h>
#include <stdlib.h>
#include "duk_rsrc.h"

#pragma global_optimizer off



char *DukGetStrResource(char *thePtr, ResType theType,char *name)
{ 
  Handle h;
  int t;
  int theLen;
  
  
  CtoPStr(name);
  h = GetNamedResource(theType,(unsigned char *) name);
  thePtr[0] = '\0';
  PtoCStr((unsigned char *) name);
  
  if (h) { 
        HLock(h);
		theLen = (unsigned char ) ((*h)[0]);
		for(t=1;t<=theLen;t++)
		   strncat(thePtr, (char *)  &((*h)[t]) , 1   );
        thePtr[theLen] = '\0';
		HUnlock(h);
		ReleaseResource(h);
		return thePtr;
		}
  else return (char *) 0L;
 
}


unsigned char *DukGetVersionResourceShortString(
	unsigned char *thePtr, 
	ResType theType,char *name)
{ 
  Handle h;
  unsigned long theLen;
  
  CtoPStr(name);
  h = GetNamedResource(theType,(unsigned char *) name);
  thePtr[0] = '\0';
  PtoCStr((unsigned char *) name);
  
  if (h) { 
        HLock(h);
		theLen = ((unsigned char *) *h)[6];
		BlockMove(&((*h)[7]),thePtr,theLen);
		HUnlock(h);
		thePtr[theLen] = '\0';
		ReleaseResource(h);
		return thePtr;
		}
  else {
 	 return (unsigned char *) 0L;
  }
  
 
}


unsigned char *DukGetCustomResource(
	unsigned char *thePtr, 
	ResType theType,char *name, unsigned long *sizeData)
{ 
  Handle h;
  unsigned long theLen;
  
  CtoPStr(name);
  h = GetNamedResource(theType,(unsigned char *) name);
  thePtr[0] = '\0';
  PtoCStr((unsigned char *) name);
  
  if (h) { 
        HLock(h);
		theLen = ((unsigned long *) *h)[0];
		BlockMove(&((*h)[4]),thePtr,theLen);
		HUnlock(h);
		*sizeData = theLen;
		ReleaseResource(h);
		return thePtr;
		}
  else {
  	 *sizeData = 0;
 	 return (unsigned char *) 0L;
  }
  
 
}



int DukSetStrResource(char *thePtr, ResType theType,char *name)
{ 
  Handle h;
  int theLen;

  
  CtoPStr(name);
  h = GetNamedResource(theType,(unsigned char *) name);
  
  if (!h)
  {
    short id;
    h = NewHandle(strlen(thePtr) + 2);
    
    HLock(h);
    (*h)[0] = strlen(thePtr);
    strcpy(&((*h)[1]),thePtr);
    HUnlock(h);
   
    id = UniqueID('STR ');
  	AddResource(h,'STR ',id,(unsigned char *) name);
  	UpdateResFile(CurResFile());
  }
	else
	{ 
        short myErr;
        theLen = strlen(thePtr);
        SetHandleSize(h,theLen + 1);
        
        HNoPurge(h);
        HLock(h);
        (*h)[0] = theLen;
        strncpy((char *)  &((*h)[1]) , thePtr, theLen);
		HUnlock(h);
		
		ChangedResource(h);
		WriteResource(h);
		myErr = ResError();
		UpdateResFile(CurResFile());
		HPurge(h);
	}

  ReleaseResource(h);  
  PtoCStr((unsigned char *) name);

  return 0;
}




int DukSetCustomResource(
	unsigned char *data, unsigned long sizeData, 
	ResType theType, char *name)
{ 
	Handle h;
  
	CtoPStr(name);
	h = GetNamedResource(theType,(unsigned char *) name);
	
	if (!h)
	{
		short id;
		h = NewHandle(sizeData + 4);
		
		HLock(h);
		((unsigned long *) *h)[0] = sizeData;
		BlockMoveData(data,&((*h)[4]),sizeData);
		HUnlock(h);
		
		id = UniqueID(theType);
		AddResource(h,theType,id,(unsigned char *) name);
		UpdateResFile(CurResFile());
	}
	else
	{
		short myErr;
        SetHandleSize(h,sizeData + 4);
       
        //HNoPurge(h);
        HLock(h);
        ((unsigned long *) *h)[0] = sizeData;
        BlockMoveData(data,&((*h)[4]),sizeData);
		HUnlock(h);
		
		ChangedResource(h);
		myErr = ResError();
		WriteResource(h);
		//HPurge(h);
	}
  
  
  ReleaseResource(h);
  PtoCStr((unsigned char *) name);

  return 0;
}



int DukSetCustomResourceWithFileData(char *filename,ResType theType,char *resName)
{
FILE *fp;
#define PROFILE_BUFFER_SIZE 1024
unsigned char buffer[PROFILE_BUFFER_SIZE];
unsigned long sizeData;

fp = fopen(filename,"rb");
if (fp) {
	sizeData = fread(buffer,1,PROFILE_BUFFER_SIZE,fp);
	if (sizeData > 0)
		DukSetCustomResource(buffer,sizeData,theType,resName);
}

return 0;
}


unsigned char *DukGetPStrResource(unsigned char *thePtr, ResType theType,char *name)
{ 
  Handle h;
  int t;
  int theLen;

  
  CtoPStr(name);
  h = GetNamedResource(theType,(unsigned char *) name);
  
  PtoCStr((unsigned char *) name);
  
  if (h) { 
        HLock(h);
		theLen = (*h)[0];
		thePtr[0] = theLen;
  		thePtr[1] = '\0';
  		thePtr++;
		for(t=1;t<=theLen;t++)
		   strncat((char *) thePtr, (char *)  &((*h)[t]) , 1   );
		HUnlock(h);
		thePtr = thePtr - 1;
		ReleaseResource(h);
		return thePtr;
		}
  else return (unsigned char *) 0L;
  
 
}


#pragma global_optimizer on
