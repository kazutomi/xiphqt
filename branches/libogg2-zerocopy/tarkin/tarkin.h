// $Id: tarkin.h,v 1.1 2001/02/13 01:06:24 giles Exp $
//
// $Log: tarkin.h,v $
// Revision 1.1  2001/02/13 01:06:24  giles
// Initial revision
//

#ifndef _TARKIN_H_
#define _TARKIN_H_

#include "dwt.h"
#include "bitwise.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
  ulong addr;
  float mag;
} dwt_vector;

typedef struct {
  uint  x_dim;
  uint  y_dim;
  uint  z_dim;
  uint  x_bits;
  uint  y_bits;
  uint  z_bits;
  uint  x_workspace;
  uint  y_workspace;
  uint  z_workspace;
  ulong sz;
  ulong sz_workspace;
  ulong vectorcount;
  float *vectors;
  dwt_vector *dwtv;
} tarkindata;

#endif // _TARKIN_H_
