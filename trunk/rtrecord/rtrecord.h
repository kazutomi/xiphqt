/*
 *
 *  rtrecord
 *    
 *      Copyright (C) 2006 Red Hat Inc.
 *
 *  rtrecord is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  rtrecord is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with rtrecord; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */


extern int channels;
extern int rate;
extern int width;
extern char *device;
extern int diskbuffer_size;
extern int diskbuffer_frames;
extern int framesize;
extern int paused;
extern int weight;
extern float smooth;

/* Producer updates into value[bank]; consumer reads value from
   value[!bank] iff bank != read.  When consumer has read a value, it
   flips read to == bank.  After producer updates a value, it will
   flip consumer_bank iff bank == read.  Thread-safe async value
   passing with no locks (altough it relies on other implicit dynamics
   to avoid starvation as it does not signal). */

typedef struct atomic_mark {
  volatile sig_atomic_t bank; // only updated by producer
  volatile sig_atomic_t read; // only updated by consumer
} atomic_mark;

extern atomic_mark dma_mark;
extern atomic_mark disk_mark;

extern long dmabuffer_frames;
typedef struct buffer_meta {
  int dmabuffer_min;
  int dmabuffer_overrun;
  int diskbuffer_min;
  int diskbuffer_overrun;
} buffer_meta;

typedef struct channel_meta {
  int peak;
  float weight_num;
  float weight_denom;
  int pcm_clip;
} channel_meta;

extern buffer_meta dma_data[2];
extern channel_meta *pcm_data[2];

extern void terminal_init_panel();
extern void terminal_remove_panel();
extern void terminal_main_loop(int quiet);

/* good only for nonnegative numbers */
#define todB(x)   ((x)==0.f?-400.f:log((x))*8.6858896f)
