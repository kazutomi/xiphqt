;//==========================================================================
;//
;//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;//  PURPOSE.
;//
;//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
;//
;//--------------------------------------------------------------------------



#include <new.h>	// Does new.h always supply placement new?
 
#include <MacMemory.h>

inline void * operator new(size_t size)
{
	void *np=NewPtrSys(size);
	if (!np) throw -1;
	return np;
}
inline void operator delete(void *x)
{
	DisposePtr((char *)x);
}

inline void * operator new[](size_t size)
{
	void *np=NewPtrSys(size);
	if (!np) throw -1;
	return np;
}
inline void operator delete[](void *x)
{
	DisposePtr((char *)x);
}


// inline void *operator new( size_t, void *p) { return p;}
