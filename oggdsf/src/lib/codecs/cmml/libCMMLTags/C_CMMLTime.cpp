#include "stdafx.h"
#include <libCMMLTags/C_CMMLTime.h>

C_CMMLTime::C_CMMLTime(void)
{
}

C_CMMLTime::~C_CMMLTime(void)
{
}

//Accessors
wstring C_CMMLTime::time() {
	return mTime;
}

//Mutators
void C_CMMLTime::setTime(wstring inTime) {
	mTime = inTime;
}