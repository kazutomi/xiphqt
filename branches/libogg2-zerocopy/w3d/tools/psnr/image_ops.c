#include <math.h>
#include "image_ops.h"

inline double
mse(unsigned char *p1, unsigned char *p2, size_t length)
{
	int i;
	double tmp, err = 0;

	for (i = 0; i < length; ++i,++p1,++p2) {
		tmp = *p1 - *p2;
		err += tmp*tmp;		
	}

	return err/length;
}

inline double
rmse(unsigned char *p1, unsigned char *p2, size_t length)
{
	return sqrt(mse(p1,p2,length));
}

inline double
psnr(unsigned char *p1, unsigned char *p2, size_t length, size_t maxval)
{	
	return 10*log10(maxval*maxval/mse(p1,p2,length));
}







