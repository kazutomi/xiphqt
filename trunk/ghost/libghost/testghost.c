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
#include "lifting.h"
#include <stdlib.h>

const float predict[11] = {-0.00499385545085393, 0.0110380571786845, -0.018414597815401, 0.0275862067026581, -0.0393646739536688, 0.055303264488734, -0.0787612707745417, 0.118522526792966, -0.29689, 0.80484, 0.4211};
const float update[11] = {-0.000749078317628089, 0.00165570857680267, -0.00276218967231015, 0.00413793100539871, -0.00590470109305032, 0.0082954896733101, -0.0118141906161813, 0.0226, -0.07844, 0.34242, 0.221};

#define BLOCK_SIZE 192

int main(int argc, char **argv)
{
   GhostEncState *state;
   FILE *fin, *fout;
   struct LiftingBasis bas;
   float x[1200];
   int i;
   
   if (argc != 3)
   {
      fprintf (stderr, "usage: testghost input_file output_file\nWhere the input and output are raw mono files sampled at 44.1 kHz or 48 kHz\n");
      exit(1);
   }
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
#if 0
   bas.predict_delay=1;
   bas.predict_length=11;
   bas.update_delay=1;
   bas.update_length=11;
   bas.predict = predict;
   bas.update = update;
   for (i=0;i<1200;i++)
   {
      //x[i] = 2*((1.f*rand())/RAND_MAX) - 1;
      x[i] = sin(.3*i);
   }
   for (i=0;i<1024;i++)
      printf ("%f ", x[i+30]);
   printf ("\n");
   lifting_forward(x+30, &bas, 1024, 1);
   for (i=0;i<1024;i++)
      printf ("%f ", x[i+30]);
   printf ("\n");
   lifting_backward(x+30, &bas, 1024, 1);
   for (i=0;i<1024;i++)
      printf ("%f ", x[i+30]);
   printf ("\n");
   return 0;
#endif
   fin = fopen(argv[1], "r");
   fout = fopen(argv[2], "w");
   state = ghost_encoder_state_new(48000);
   while (1)
   {
      int i;
      float float_in[BLOCK_SIZE];
      short short_in[BLOCK_SIZE];
      fread(short_in, sizeof(short), BLOCK_SIZE, fin);
      //printf ("%d ", short_in[0]);

      if (feof(fin))
         break;
      for (i=0;i<BLOCK_SIZE;i++)
         float_in[i] = short_in[i];
      ghost_encode(state, float_in);
      for (i=0;i<BLOCK_SIZE;i++)
      {
         if (float_in[i] > 32767)
            short_in[i] = 32767;
         else if (float_in[i] < -32768)
            short_in[i] = -32768;
         else 
            short_in[i] = float_in[i];
      }
      fwrite(short_in, sizeof(short), BLOCK_SIZE, fout);
   }
   ghost_encoder_state_destroy(state);
   
   /*for (i=0;i<256;i++)
      printf ("%f %f\n", x[i], y[i]);*/
   return 0;
}

