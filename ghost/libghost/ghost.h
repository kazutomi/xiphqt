/**
   @file ghost.h
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

#ifndef _GHOST_H
#define _GHOST_H

#include "vorbis_psy.h"

typedef struct {
   float *pcm_buf;
   float *new_pcm;
   float *current_frame;
   
   float *analysis_window;
   float *synthesis_window;  
   float *lpc_window;
   float *big_window;
   
   float *syn_memory;
   float *noise_mem;
   float *noise_mem2;
   
   VorbisPsy *psy;
   
   float *noise_buf;
   float *new_noise;
   //float *current_noise;
   
   int length;
   int advance;
   int overlap;
   int lpc_length;
   int lpc_order;
   
   void *big_fft;
   void *lpc_fft;
} GhostEncState;

GhostEncState *ghost_encoder_state_new(int sampling_rate);

void ghost_encoder_state_destroy(GhostEncState *st);

void ghost_encode(GhostEncState *st, float *pcm);

#endif
