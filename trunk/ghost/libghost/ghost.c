/**
   @file ghost.c
   @brief Main codec file
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


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ghost.h"
#include "pitch.h"
#include "sinusoids.h"
#define PCM_BUF_SIZE 2048

GhostEncState *ghost_encoder_state_new(int sampling_rate)
{
   GhostEncState *st = calloc(1,sizeof(GhostEncState));
   st->frame_size = 256;
   st->pcm_buf = calloc(PCM_BUF_SIZE,sizeof(float));
   st->current_pcm = st->pcm_buf + PCM_BUF_SIZE - st->frame_size;
   return st;
}

void ghost_encoder_state_destroy(GhostEncState *st)
{
   free(st);
}

void ghost_encode(GhostEncState *st, float *pcm)
{
   int i;
   float gain;
   float pitch;
   float w;
   for (i=0;i<PCM_BUF_SIZE-st->frame_size;i++)
      st->pcm_buf[i] = st->pcm_buf[i+st->frame_size];
   for (i=0;i<st->frame_size;i++)
      st->current_pcm[i]=pcm[i];
   find_pitch(st->current_pcm, &gain, &pitch, 100, 768, st->frame_size);
   //pitch = 256;
   //printf ("%d %f\n", pitch, gain);
   w = 2*M_PI/pitch;
   {
      float wi[45];
      float y[256];
      float ai[45], bi[45];
      for (i=0;i<45;i++)
         wi[i] = w*(i+1);
      extract_sinusoids(st->current_pcm, wi, ai, bi, y, 20, 256);
      short out[256];
      for (i=0;i<256;i++)
         out[i] = y[i];
      fwrite(out, sizeof(short), 256, stdout);
   }
   
}
