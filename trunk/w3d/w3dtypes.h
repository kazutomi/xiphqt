#ifndef __W3D_TYPES_H
#define __W3D_TYPES_H

#if defined(_WIN32)   /* The unistd.h and stdint.h headers do not  */
                      /* exists in VC6 either in BC5               */

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

#else

	#include <unistd.h>
	#include <stdint.h>

	#ifndef O_BINARY
	#define O_BINARY        0
	#endif

#endif

#endif  /* __W3D_TYPES_H */

