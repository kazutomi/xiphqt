/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#ifndef ARRAYS_H
#define ARRAYS_H


/*  the number of tests: 0: ATH tone, 1: ATH noise, 2: tone-tone */
/*  3:... look in Audio::sound() */
#define NUMTESTS 6

/*  EHMER_MAX, P_LEVELS and P_BANDS have their analogies to the */
/*  same things in the Vorbis code, that's why I stole their names */

#define EHMER_MAX 2
static const float freqarray[] = {707., 2828.};

#define P_LEVELS 2
static const float maskeramparray[] = {10., 80.};

#define P_BANDS 2
static const float maskerfreqarray[] = {1000.0, 2000.};


/*  comment in or out the above or below defines and arrays to either  */
/*  go quickly through the test (useful for debugging) or take the whole test */


/*  the large arrays were automatically generated  */


/*  #define EHMER_MAX 65 */
/*  static const float freqarray[] = {62.5, */
/*  68.156731, 74.325439, 81.052467, 88.388344, 96.388176, 105.112053, 114.625504, 125.000000, 136.313461, 148.650879, 162.104935, 176.776688, 192.776352, 210.224106, 229.251007, 250.000000, 272.626923, 297.301758, 324.209869, 353.553375, 385.552704, 420.448212, 458.502014, 500.000000, 545.253845, 594.603516, 648.419739, 707.106750, 771.105408, 840.896423, 917.004028, 1000.000000, 1090.507690, 1189.207031, 1296.839478, 1414.213501, 1542.210815, 1681.792847, 1834.008057, 2000.000000, 2181.015381, 2378.414062, 2593.678955, 2828.427002, 3084.421631, 3363.585693, 3668.016113, 4000.000000, 4362.030762, 4756.828125, 5187.357910, 5656.854004, */
/*  				  6168.843262, 6727.171387, 7336.032227, 8000.000000, 8724.061523, 9513.656250, 10374.715820, 11313.708008, 12337.686523, 13454.342773, 14672.064453, 16000.}; */

/*  #define P_LEVELS 8 */
/*  static const float maskeramparray[] = {10., 20., 30., 40., 50., 60., 70., 80.}; */

/*  #define P_BANDS 17 */
/*  static const float maskerfreqarray[] = {62.5, 88.388351, 125.000008, 176.776703, 250.000015, 353.553406, 500.000031, 707.106812, 1000.000061, 1414.213623, 2000.000122, 2828.427246, 4000.000244, 5656.854492, 8000.000488, 11313.708984, 16000.}; */



#endif
