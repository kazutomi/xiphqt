#if !defined(_psych_H)
# define _psych_H (1)

/*The weightings of each color channel for each color space.
  These are computed such that after weighting, a value of 1.0
   roughly corresponds to a Just Noticible Difference.*/
extern const float OC_YUV_SCALE[3][3];

#endif