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

#define SINUSOIDS 5
#define MASK_LPC_ORDER 12

void fir_mem2(const spx_sig_t *x, const spx_coef_t *num, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word32_t xi,yi;

   for (i=0;i<N;i++)
   {
      xi=SATURATE(x[i],805306368);
      yi = xi + SHL32(mem[0],2);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_32_Q15(mem[j+1], num[j],xi);
      }
      mem[ord-1] = MULT16_32_Q15(num[ord-1],xi);
      y[i] = SATURATE(yi,805306368);
   }
}
void iir_mem2(const spx_sig_t *x, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word32_t xi,yi,nyi;

   for (i=0;i<N;i++)
   {
      xi=SATURATE(x[i],805306368);
      yi = SATURATE(xi + SHL32(mem[0],2),805306368);
      nyi = NEG32(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_32_Q15(mem[j+1],den[j],nyi);
      }
      mem[ord-1] = MULT16_32_Q15(den[ord-1],nyi);
      y[i] = yi;
   }
}


GhostEncState *ghost_encoder_state_new(int sampling_rate)
{
   int i;
   GhostEncState *st = calloc(1,sizeof(GhostEncState));
   st->length = 256;
   st->advance = 192;
   st->overlap = 64;
   st->lpc_length = 384;
   st->lpc_order = MASK_LPC_ORDER;
   st->pcm_buf = calloc(PCM_BUF_SIZE,sizeof(float));
#if 1
   /* pre-analysis window centered around the current frame */
   st->current_frame = st->pcm_buf + PCM_BUF_SIZE/2 - st->length/2;
#else
   /* causal pre-analysis window (delayed) */
   st->current_frame = st->pcm_buf + PCM_BUF_SIZE - st->length;
#endif
   st->new_pcm = st->pcm_buf + PCM_BUF_SIZE - st->advance;
   
   st->noise_buf = calloc(PCM_BUF_SIZE,sizeof(float));
   st->new_noise = st->noise_buf + PCM_BUF_SIZE/2 - st->length/2;
   
   st->psy = vorbis_psy_init(44100, PCM_BUF_SIZE);
   
   st->analysis_window = calloc(st->length,sizeof(float));
   st->synthesis_window = calloc(st->length,sizeof(float));
   st->big_window = calloc(PCM_BUF_SIZE,sizeof(float));
   st->lpc_window = calloc(st->lpc_length,sizeof(float));

   st->syn_memory = calloc(st->overlap,sizeof(float));
   st->noise_mem = calloc(st->lpc_order,sizeof(float));
   st->noise_mem2 = calloc(st->lpc_order,sizeof(float));
   for (i=0;i<st->length;i++)
   {
      st->analysis_window[i] = 1;
      st->synthesis_window[i] = 1;
   }
   for (i=0;i<st->overlap;i++)
   {
      //st->analysis_window[i] = .5-.5*cos(M_PI*(i+.5)/st->overlap);
      //st->analysis_window[st->length-i-1] = .5-.5*cos(M_PI*(i+.5)/st->overlap);
      st->analysis_window[i] = ((float)i+.5)/st->overlap;
      st->analysis_window[st->length-i-1] = ((float)i+.5)/st->overlap;
   }
#if 1
   for (i=0;i<st->lpc_length;i++)
      st->lpc_window[i] = .5-.5*cos(2*M_PI*i/st->lpc_length);
#else
   for (i=0;i<st->lpc_order;i++)
      st->lpc_window[i]=0;
   for (i=st->lpc_order;i<st->lpc_length;i++)
      st->lpc_window[i] = .5-.5*cos(2*M_PI*(i-st->lpc_order)/(st->lpc_length-st->lpc_order));
#endif
   st->big_fft = spx_fft_init(PCM_BUF_SIZE);
   st->lpc_fft = spx_fft_init(st->lpc_length);
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
   float curve[PCM_BUF_SIZE>>1];
   float awk1[MASK_LPC_ORDER], awk2[MASK_LPC_ORDER];
   float mask_gain;
   
   for (i=0;i<PCM_BUF_SIZE-st->advance;i++)
      st->pcm_buf[i] = st->pcm_buf[i+st->advance];
   for (i=0;i<st->advance;i++)
      st->new_pcm[i]=pcm[i];
   
   compute_curve(st->psy, st->pcm_buf, curve);
   mask_gain = curve_to_lpc(st->psy, curve, awk1, awk2, MASK_LPC_ORDER);
   
   /*for (i=0;i<st->advance;i++)
      st->new_pcm[i]=pcm[i]+10*3.4641f*.001f*(rand()%1000-500);*/
   {
      float wi[SINUSOIDS];
      float x[st->length];
      float y[st->length];
      float ai[SINUSOIDS], bi[SINUSOIDS];
      float ci[SINUSOIDS], di[SINUSOIDS];
      float psd[PCM_BUF_SIZE];
      int nb_sinusoids;
      
      spx_fft_float(st->big_fft, st->pcm_buf, psd);
      for (i=1;i<(PCM_BUF_SIZE>>1);i++)
      {
         psd[i] = 10*log10(1+psd[2*i-1]*psd[2*i-1] + psd[2*i]*psd[2*i]);
      }
      psd[0] = 10*log10(1+psd[0]*psd[0]);
      psd[(PCM_BUF_SIZE>>1)-1] = 10*log10(1+psd[PCM_BUF_SIZE-1]*psd[PCM_BUF_SIZE-1]);
      nb_sinusoids = SINUSOIDS;
      find_sinusoids(psd, wi, &nb_sinusoids, (PCM_BUF_SIZE>>1)+1);
      //printf ("%d\n", nb_sinusoids);
      /*for (i=0;i<SINUSOIDS;i++)
      {
         fprintf (stderr, "%f ", wi[i]);
      }
      fprintf (stderr, "\n");*/
      for (i=0;i<st->length;i++)
         x[i] = st->analysis_window[i]*st->current_frame[i];
      //extract_sinusoids(x, wi, st->window, ai, bi, y, SINUSOIDS, st->length);
      extract_modulated_sinusoids(x, wi, st->analysis_window, ai, bi, ci, di, y, nb_sinusoids, st->length);
      
      /*for (i=0;i<st->length;i++)
      y[i] *= st->synthesis_window[i];*/

      for (i = 0;i < st->new_noise-st->noise_buf+st->overlap; i++)
      {
         st->noise_buf[i] = st->noise_buf[i+st->advance];
      }
      for (i=0;i<st->overlap;i++)
      {
         st->new_noise[i] = st->new_noise[i]*st->synthesis_window[i+st->length-st->overlap] + 
               (x[i]-y[i])*st->synthesis_window[i];
      }
      for (i=st->overlap;i<st->length;i++)
      {
         st->new_noise[i] = x[i]-y[i];
      }
      
      /*for (i=0;i<st->overlap;i++)
         pcm[i] = st->syn_memory[i]+y[i];
      for (i=st->overlap;i<st->advance;i++)
         pcm[i] = y[i];
      for (i=st->advance;i<st->length;i++)
      st->syn_memory[i-st->advance]=y[i];*/
      
      float noise_window[st->lpc_length];
      float noise_ac[st->lpc_length];
      float noise_psd[st->lpc_length];
      for (i=0;i<st->lpc_length;i++)
         noise_window[i] = st->lpc_window[i]*st->new_noise[i+st->length-st->lpc_length];
      /* Don't know why, but spectral version sometimes results in an unstable LPC filter */
      /*spx_fft_float(st->lpc_fft, noise_window, noise_psd);
      
      noise_psd[0] *= noise_psd[0];
      for (i=1;i<st->lpc_length-1;i+=2)
      {
         noise_psd[i] = noise_psd[i]*noise_psd[i] + noise_psd[i+1]*noise_psd[i+1];
      }
      noise_psd[st->lpc_length-1] *= noise_psd[st->lpc_length-1];
      spx_ifft_float(st->lpc_fft, noise_psd, noise_ac);
      */
      
      /*for (i=0;i<st->lpc_order+1;i++)
      {
         int j;
         double tmp = 0;
         for (j=0;j<st->lpc_length-i;j++)
            tmp += (double)noise_window[j]*(double)noise_window[i+j];
         noise_ac[i] = tmp;
      }
      for (i=0;i<st->lpc_order+1;i++)
      noise_ac[i] *= exp(-.0001*i*i);
      noise_ac[0] *= 1.0001;
      noise_ac[0] += 1;
      
      float lpc[st->lpc_order];
      _spx_lpc(lpc, noise_ac, st->lpc_order);*/
      
      /*for (i=0;i<st->lpc_order;i++)
      lpc[i] *= pow(.9,i+1);*/
      /*for (i=0;i<st->lpc_order;i++)
         printf ("%f ", lpc[i]);
      printf ("\n");*/
      //for (i=0;i<st->lpc_order;i++)
      if (0)
      {
         for (i=0;i<st->lpc_order+1;i++)
            printf ("%f ", noise_ac[i]);
         printf ("\n");
         //for (i=0;i<st->lpc_order;i++)
         //printf ("%f ", lpc[i]);
         printf ("\n");
         /*for (i=0;i<st->lpc_length;i++)
         printf ("%f ", noise_window[i]);
         printf ("\n");
         for (i=0;i<st->lpc_length;i++)
            printf ("%f ", st->lpc_window[i]);
         printf ("\n");
         for (i=0;i<st->lpc_length;i++)
            printf ("%f ", st->new_noise[i+st->length-st->lpc_length]);
         printf ("\n");*/
         exit(1);
      }
      float noise[st->advance];
      //for (i=0;i<MASK_LPC_ORDER;i++)
      //   awk1[i] = 0;
      fir_mem2(st->new_noise, awk1, noise, st->advance, MASK_LPC_ORDER, st->noise_mem);
      for (i=0;i<st->advance;i++)
         noise[i] /= mask_gain;
      
      //Replace whitened residual by white noise
      if (0) {
         float ener = 0;
         for (i=0;i<st->advance;i++)
            ener += noise[i]*noise[i];
         ener = sqrt(ener/st->advance);
         for (i=0;i<st->advance;i++)
            noise[i] = ener*sqrt(12.)*((((float)(rand()))/RAND_MAX)-.5);
      }
      
      /*for (i=0;i<st->advance;i++)
         printf ("%f\n", noise[i]);
      printf ("\n");*/
      for (i=0;i<st->advance;i++)
         noise[i] = 16*floor(.5+.0625*noise[i]);
      for (i=0;i<st->advance;i++)
         noise[i] *= mask_gain;
      iir_mem2(noise, awk1, noise, st->advance, MASK_LPC_ORDER, st->noise_mem2);
      
      /*for (i=0;i<st->advance;i++)
      pcm[i] = st->current_frame[i]-st->new_noise[i];*/
      
      for (i=0;i<st->advance;i++)
         pcm[i] = /*st->current_frame[i]-*/st->new_noise[i] /*+ noise[i]*/;
      
   }
   
}
