/* Copyright (C) 2005 */
/**
   @file ghost.c
   @brief Sinusoid extraction/synthesis
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

