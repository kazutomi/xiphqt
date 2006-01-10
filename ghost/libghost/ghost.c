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
#include "fftwrap.h"

#define PCM_BUF_SIZE 2048

#define SINUSOIDS 15

GhostEncState *ghost_encoder_state_new(int sampling_rate)
{
   int i;
   GhostEncState *st = calloc(1,sizeof(GhostEncState));
   st->length = 256;
   st->advance = 192;
   st->overlap = 64;
   st->pcm_buf = calloc(PCM_BUF_SIZE,sizeof(float));
   st->window = calloc(st->length,sizeof(float));
   st->big_window = calloc(PCM_BUF_SIZE,sizeof(float));
   st->syn_memory = calloc(st->overlap,sizeof(float));
   st->current_pcm = st->pcm_buf + PCM_BUF_SIZE - st->length;
   for (i=0;i<st->length;i++)
      st->window[i] = 1;
   for (i=0;i<st->overlap;i++)
   {
      st->window[i] = .5-.5*cos(M_PI*i/st->overlap);
      st->window[st->length-i-1] = .5-.5*cos(M_PI*(i+1)/st->overlap);
   }
   st->big_fft = spx_fft_init(PCM_BUF_SIZE);
   for (i=0;i<PCM_BUF_SIZE;i++)
      st->big_window[i] = .5-.5*cos(2*M_PI*(i+1)/PCM_BUF_SIZE);
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
   for (i=0;i<PCM_BUF_SIZE-st->advance;i++)
      st->pcm_buf[i] = st->pcm_buf[i+st->advance];
   for (i=0;i<st->advance;i++)
      st->current_pcm[i+st->overlap]=pcm[i];
   find_pitch(st->current_pcm, &gain, &pitch, 100, 1024, st->length);
   //fprintf (stderr,"%f %f\n", pitch, gain);
   //pitch = 256;
   w = 2*M_PI/pitch;
   {
      float wi[SINUSOIDS];
      float x[st->length];
      float y[st->length];
      float ai[SINUSOIDS], bi[SINUSOIDS];
      float psd[PCM_BUF_SIZE];
      
      for (i=0;i<SINUSOIDS;i++)
         wi[i] = w*(i+1);

      spx_fft_float(st->big_fft, st->pcm_buf, psd);
      for (i=1;i<(PCM_BUF_SIZE>>1);i++)
      {
         psd[i] = 10*log10(1+psd[2*i-1]*psd[2*i-1] + psd[2*i]*psd[2*i]);
      }
      psd[0] = 10*log10(1+psd[0]*psd[0]);
      psd[(PCM_BUF_SIZE>>1)-1] = 10*log10(1+psd[PCM_BUF_SIZE-1]*psd[PCM_BUF_SIZE-1]);
      find_sinusoids(psd, wi, SINUSOIDS, (PCM_BUF_SIZE>>1)+1);
      /*for (i=0;i<SINUSOIDS;i++)
      {
         fprintf (stderr, "%f ", wi[i]);
      }
      fprintf (stderr, "\n");*/
      for (i=0;i<st->length;i++)
         x[i] = st->window[i]*st->current_pcm[i];
      extract_sinusoids(x, wi, st->window, ai, bi, y, SINUSOIDS, st->length);
      /*for (i=0;i<st->length;i++)
      y[i] = x[i];*/
      short out[st->advance];
      for (i=0;i<st->overlap;i++)
         pcm[i] = st->syn_memory[i]+y[i];
      for (i=st->overlap;i<st->advance;i++)
         pcm[i] = y[i];
      for (i=st->advance;i<st->length;i++)
         st->syn_memory[i-st->advance]=y[i];
      //fwrite(out, sizeof(short), st->advance, stdout);
      
   }
   
}
