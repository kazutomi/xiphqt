#if !defined(_psych_H)
# define _psych_H (1)
# include "encvbr.h"

/*The assumed screen resolution vs. viewing distance.
  This is taken to be constant under the assumption that viewers will sit
   closer to higher resolution images, and farther from lower resolution ones.
  This value corresponds to roughly 50 cm from an 44 pixel/inch display, and
   is measured over a 1 degree arc around the perpendicular to the display
   surface (i.e., the maximum resolution).*/
#define OC_PIXELS_PER_DEGREE (15)

/*The weightings of each color channel for each color space.
  These are computed such that after weighting, a value of 1.0
   roughly corresponds to a Just Noticible Difference.*/
extern const float OC_YCbCr_SCALE[3][3];

#endif
