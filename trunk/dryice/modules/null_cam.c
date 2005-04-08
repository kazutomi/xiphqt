/*
 * null_cam.c, DryIce camera module which outputs only black frames
 *
 * Copyright (c) 2005 Arc Riley <arc@xiph.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   last mod: $Id: null_cam.c,v 1.2 2004/03/02 11:10:03 arc Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "dryice/dryice.h"

typedef struct {
  int frame_width;
  int frame_height;
  int frame_raten;
  int frame_rated;
  unsigned char *frame_buffer;
} dryice_null_cam_state;

static void *
dryice_null_cam_init(dryice_video_params *dvp) {
  int pixels;
  dryice_null_cam_state *dncs;

  dncs = malloc(sizeof(dryice_null_cam_state));
  dncs->frame_width = dvp->frame_width;
  dncs->frame_height = dvp->frame_height;
  pixels = dncs->frame_width * dncs->frame_height;
  dncs->frame_raten = dvp->framerate_numerator;
  dncs->frame_rated = dvp->framerate_denominator;  
  dncs->frame_buffer = calloc ( sizeof(unsigned char), pixels * 1.5 ); 
  return dncs;
}

int
dryice_null_cam_get_params(dryice_null_cam_state *dncs,
                           dryice_video_params *dvp) {

  dvp->frame_width = dncs->frame_width;
  dvp->frame_height = dncs->frame_height;
  dvp->framerate_numerator = dncs->frame_raten;
  dvp->framerate_denominator = dncs->frame_rated;
  return 1;
}

char *
dryice_null_cam_get_frame(dryice_null_cam_state *dncs) {

  return dncs->frame_buffer;
}

int
dryice_null_cam_destroy(dryice_null_cam_state *dncs) {

  free(dncs->frame_buffer);
  free(dncs);
  return 1;
}
