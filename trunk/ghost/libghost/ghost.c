/* Copyright (C) 2005 */
/**
   @file ghost.c
   @brief Main codec file
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

#include <stdlib.h>
#include "ghost.h"
#include "pitch.h"
#include "sinusoids.h"

#define PCM_BUF_SIZE 2048

GhostEncState *ghost_encoder_state_new(int sampling_rate)
{
   GhostEncState *st = calloc(1,sizeof(GhostEncState));
   st->pcm_buf = calloc(PCM_BUF_SIZE,sizeof(float));
   st->current_pcm = st->pcm_buf + PCM_BUF_SIZE - 256;
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
   int pitch;
   for (i=0;i<PCM_BUF_SIZE-st->frame_size;i++)
      st->pcm_buf[i] = st->current_pcm[i+st->frame_size];
   for (i=0;i<st->frame_size;i++)
      st->current_pcm[i]=pcm[i];
   find_pitch(st->current_pcm, &gain, &pitch, 100, 768, st->frame_size);
   printf ("%d %f\n", pitch, gain);
}
