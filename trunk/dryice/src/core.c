/*
 * core.c, "the core" of the DryIce source client for Icecast2
 *
 * Copyright (c) 2004,2005 Arc Riley <arc@xiph.org>
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
 *   last mod: $Id: core.c,v 1.2 2004/03/04 06:35:16 arc Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include <math.h>
#include <shout/shout.h>
#include <theora/theora.h>
#include "dryice/dryice.h"

/* I have no idea what this is used for */
#ifdef _WIN32
#include <fcntl.h>
#endif


int main(int argc, char **argv)
{


  /* BIG TODO

  Take the packets from the codec(s), toss them into an Ogg, ship the 
  whole thing out to libtheora with raw packets.  There is no need to 
  sleep since the stream is already running realtime, and thus, no need
  to do threading/etc as IceS2 does.  We'll basically just use libshout2
  for it's ability to initiate and pass data to Icecast2.

  */

  return 0;
}


typedef struct dryice_theora_state {
  theora_state  td;
  ogg_packet   *packet_buff[4];
  yuv_buffer    yuv;
} dryice_theora_state;

void * dryice_theora_init(dryice_video_params *dvp) {

  theora_info          ti;
  dryice_theora_state *dts;

  theora_info_init(&ti);

  /* allocate encode state */
  dts = malloc(sizeof(dryice_theora_state));

  /* Is this nessesary?  Hmm.. artifact of a late night of hacking..
  dts->packet_buff = malloc(sizeof(void *)*4); */

  /* Theora has a divisible-by-sixteen restriction for the encoded video size */
  /* scale the frame size up to the nearest /16 and calculate offsets */
  ti.frame_width = dvp->frame_width;
  ti.frame_height = dvp->frame_height;
  ti.width = ((dvp->frame_width + 15) >>4)<<4;
  ti.height = ((dvp->frame_height + 15) >>4)<<4;
  ti.offset_x = (ti.width - dvp->frame_width)/2;
  ti.offset_y = (ti.height - dvp->frame_height)/2;
  ti.fps_numerator = dvp->framerate_numerator;
  ti.fps_denominator = dvp->framerate_denominator;
  
  /* These are unspecified/undetermined */
  ti.aspect_numerator = -1;
  ti.aspect_denominator = -1;
  ti.colorspace = OC_CS_UNSPECIFIED;
  ti.target_bitrate = -1;
  ti.quality = 8;

  ti.dropframes_p = 0;
  ti.quick_p = 1;
  ti.keyframe_auto_p = 1;
  ti.keyframe_frequency = 64;
  ti.keyframe_frequency_force = 64;
  ti.keyframe_data_target_bitrate = ti.target_bitrate*1.5;
  ti.keyframe_auto_threshold = 80;
  ti.keyframe_mindistance = 8;
  ti.noise_sensitivity = 1;

  theora_encode_init(&dts->td, &ti);
  theora_info_clear(&ti);

  dts->yuv.y_width = dvp->frame_width;
  dts->yuv.y_height = dvp->frame_height;
  dts->yuv.y_stride = dvp->frame_width;
  dts->yuv.uv_width = dvp->frame_width / 2;
  dts->yuv.uv_height = dvp->frame_height / 2;
  dts->yuv.uv_stride = dvp->frame_width / 2;

  return dts;
}

void 
dryice_theora_encode_headers(dryice_theora_state *dts) {
  
  theora_comment tc;

  theora_encode_header(&dts->td, dts->packet_buff[0]);
  theora_comment_init(&tc);
  theora_encode_comment(&tc, dts->packet_buff[1]);
  theora_encode_tables(&dts->td, dts->packet_buff[2]);
  dts->packet_buff[3] = 0;  /* null-terminate the array */
}

void
dryice_theora_encode_frame(dryice_theora_state *dts,
                           char *yuvframe) {
  /* These calculations need to be checked */
  dts->yuv.y = yuvframe;
  dts->yuv.u = dts->yuv.y + dts->yuv.y_width * dts->yuv.y_height;
  dts->yuv.v = dts->yuv.u + dts->yuv.uv_width * dts->yuv.uv_height;
  
  theora_encode_YUVin(&dts->td, &dts->yuv);
  theora_encode_packetout(&dts->td, 0, dts->packet_buff[0]);
  dts->packet_buff[1] = 0;
}

void
dryice_theora_clear(dryice_theora_state *dts) {
  theora_clear(&dts->td);
  free(dts);
}
