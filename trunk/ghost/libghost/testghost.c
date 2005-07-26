/**
   @file ghost.c
   @brief Sinusoid extraction/synthesis
*/

/* Copyright (C) 2005

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <math.h>
#include <stdio.h>
#include "sinusoids.h"
#include "ghost.h"
#include "pitch.h"

int main(int argc, char **argv)
{
   GhostEncState *state;
   FILE *fin;
   /*float x[256];
   float y[256];
   float w[2] = {.05, .2};
   float ai[2], bi[2];
   int i;
   for (i=0;i<256;i++)
   {
      x[i] = cos(.05*i+2) + .2*cos(.2*i+1.1);
   }
   extract_sinusoids(x, w, ai, bi, y, 2, 256);
   printf ("%f %f\n", ai[0], bi[0]);
   printf ("%f %f\n", ai[1], bi[1]);
   */
   fin = fopen("test.sw", "r");
   state = ghost_encoder_state_new(48000);
   while (1)
   {
      int i;
      float float_in[256];
      short short_in[256];
      fread(short_in, sizeof(short), 256, fin);
      //printf ("%d ", short_in[0]);

      if (feof(fin))
         break;
      for (i=0;i<256;i++)
         float_in[i] = short_in[i];
      ghost_encode(state, float_in);
      
   }
   ghost_encoder_state_destroy(state);
   
   /*for (i=0;i<256;i++)
      printf ("%f %f\n", x[i], y[i]);*/
   return 0;
}

