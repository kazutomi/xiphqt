/* crossfade.c
 * Crossfader plugin
 * Copyright (c) 2004 Brendan Cully
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 */

#include "definitions.h"
#include "plugin.h"

static void cf_new_track(void);
static int cf_process(int ilen, int16_t* il, int16_t* ir);

static ices_plugin_t Crossfader = {
  "crossfade",

  cf_new_track,
  cf_process,

  NULL
};

static int NewTrack = 0;
static int FadeSecs = 3;
static int FadeSamples;
static int16_t* FL;
static int16_t* FR;
static int fpos = 0;
static int flen = 0;

/* public functions */
ices_plugin_t *crossfade_plugin(int secs) {
  FadeSamples = secs * 44100;
  FL = malloc(FadeSamples * 2);
  FR = malloc(FadeSamples * 2);
  ices_log_debug("Crossfading %d seconds between tracks", secs);

  return &Crossfader;
}

/* private functions */
static void cf_new_track(void) {
  NewTrack = FadeSamples;
}

static int cf_process(int ilen, int16_t* il, int16_t* ir)
{
  int i, j;
  float weight;
  int16_t swap;

  i = 0;
  /* if the buffer is not full, don't attempt to crossfade, just fill it */
  if (flen < FadeSamples)
    NewTrack = 0;

  while (ilen && NewTrack > 0) {
    weight = (float)NewTrack / FadeSamples;
    il[i] = FL[fpos] * weight + il[i] * (1 - weight);
    ir[i] = FR[fpos] * weight + ir[i] * (1 - weight);
    i++;
    fpos = (fpos + 1) % FadeSamples;
    ilen--;
    NewTrack--;
    if (!NewTrack)
      flen = 0;
  }

  j = i;
  while (ilen && flen < FadeSamples) {
    FL[fpos] = il[j];
    FR[fpos] = ir[j];
    j++;
    fpos = (fpos + 1) % FadeSamples;
    flen++;
    ilen--;
  }

  while (ilen) {
    swap = il[j];
    il[i] = FL[fpos];
    FL[fpos] = swap;
    swap = ir[j];
    ir[i] = FR[fpos];
    FR[fpos] = swap;
    i++;
    j++;
    fpos = (fpos + 1) % FadeSamples;
    ilen--;
  }

  return i;
}
