#ifndef image_ops_h
#define image_ops_h

#include <stdlib.h>

double mse(unsigned char *p1, unsigned char *p2, size_t  length);
double rmse(unsigned char *p1, unsigned char *p2, size_t length);
double psnr(unsigned char *p1, unsigned char *p2, size_t length, size_t maxval);

#endif
