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


#include <Files.h>
#include <Folders.h>
#include <LowMem.h>
#include <string.h>
#include "GetSysFolder.h"
/*
folderType -- same as FindFile
send 0 for SystemFolder

path -- return value is full path to folder
*/
short GetSystemFolderPath(OSType folderType, char *path)
{
	CInfoPBRec	myPB;
	Str255		dirName;
	OSErr		myErr;
	FSSpec		fileSpec;
	Str255		fullPath;
	Str255		tempPath;
	
	if(!folderType)
		folderType = kSystemFolderType;
	
	myErr = FindFolder(kOnSystemDisk, folderType, false,&fileSpec.vRefNum,&fileSpec.parID);
	if(myErr)
		return myErr;
	
	dirName[0] = 0;
	fullPath[0] = 0;
	tempPath[0] = 0;
	myPB.dirInfo.ioFDirIndex = -1;
	myPB.dirInfo.ioCompletion = nil;
	myPB.dirInfo.ioNamePtr = dirName;
	myPB.dirInfo.ioVRefNum = fileSpec.vRefNum;
	myPB.dirInfo.ioDrParID = fileSpec.parID;
	
	do
	{
		myPB.dirInfo.ioDrDirID = myPB.dirInfo.ioDrParID;
		myErr = PBGetCatInfoSync(&myPB);
		dirName[0]++;
		dirName[dirName[0]] = ':';
		
		BlockMoveData(&fullPath[1], &tempPath[dirName[0]+1], fullPath[0]);
		BlockMoveData(&dirName[1], &tempPath[1], dirName[0]);
		tempPath[0] += dirName[0];
		
		BlockMoveData(&tempPath[0], &fullPath[0], tempPath[0]+1);
	}
	while( (myPB.dirInfo.ioDrDirID != fsRtDirID) && (myErr == noErr) );
	
	fullPath[0] -= 1; /* get rid of that dumb-as trailing colon cancer */
	fullPath[fullPath[0]+1] = '\0';
	BlockMoveData(&fullPath[1], path, fullPath[0] + 1);
	
	return myErr;
}


short GetSystemStartVolumeName(char *name);
short GetSystemStartVolumeName(char *name)
{
char *c;

GetSystemFolderPath(0,name);

c = strstr(name,":");
if (c) {
    *c = '\0';
    return 0;
}
return -1;

} /* GetSystemStartVolumeName */