/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include "random.h"

// Portable 32 bit random number generator.  It's not crypto-grade,
// but we don't need crypto-grade.  We need complete control over the
// result sequence and reproducability, so we can't use any local
// generator.  As long as it's 100% consistent across
// platforms/OSes/compilers and fairly uniform (doesn't always return
// 17), we're all set.

// This is a C derivative of the PASCAL "Integer Version 2" minimal
// standard number generator thich appears in the article:
//     Park, Steven K. and Miller, Keith W., "Random Number
//     Generators: Good Ones are Hard to Find", Communications of the
//     ACM, October, 1988.


static int32_t next = 123456789;

void random_seed(int32_t seed){
  next = seed;
}

#define MPLIER     16807
#define MOBYMP     127773
#define MOMDMP     2836

int32_t random_number(){
  int32_t hival, loval, testval;

  hival = next / MOBYMP;
  loval = next % MOBYMP;
  testval = MPLIER*loval - MOMDMP*hival;
  if (testval > 0)
    next = testval;
  else
    next = testval + MAX_G_RAND;

  return next;
}

int random_yes(int per128_yes){
  u_int32_t num = (u_int32_t)random_number();
  return (num < (unsigned int)per128_yes << 24U);
}
