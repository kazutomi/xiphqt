#ifndef __TYPES_HEADER_INLUDE__
#define __TYPES_HEADER_INLUDE__

#if !defined(_WIN32)  // These headers do not exists in VC6 either in BC5
	#include <unistd.h>
	#include <stdint.h>
	#define O_BINARY        0
#else
	#define TYPE_BITS       10
	#define __FUNCTION__    ""    // in Win this is not supported.

	typedef char  int8_t;
	typedef short int16_t;
	typedef int   int32_t;

	typedef unsigned char  uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int   uint32_t;

	typedef int16_t TYPE;

	#define snprintf       _snprintf    // name confusion...
	#define inline         __inline
#endif

#endif  // __TYPES_HEADER_INLUDE__
