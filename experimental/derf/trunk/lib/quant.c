#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "quant.h"



unsigned OC_DC_QUANT_MIN[2]={4<<2,8<<2};
unsigned OC_AC_QUANT_MIN[2]={2<<2,4<<2};
