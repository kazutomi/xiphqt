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
  int c;



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
  int video_x;
  int video_y;
  int frame_x;
  int frame_y;
  int frame_x_offset;  
  int frame_y_offset;
  int video_raten;
  int video_rated;
  int video_an;
  int video_ad;

  int video_r;
  int video_q;

  theora_state  td;
  ogg_packet packet_buff[4]; /* most packets we'll ever have to return */

  yuv_buffer    yuv;
  
} dryice_theora_state;

void * dryice_theora_init(dryice_params *dp) {

  theora_info          ti;
  dryice_theora_state *dts;

  /* allocate module state and initialise video parameters */
  dts = malloc(sizeof(dryice_theora_state));
  dts->frame_x = dp->frame_width;
  dts->frame_y = dp->frame_height;
  dts->video_raten = dp->framerate_numerator;
  dts->video_rated = dp->framerate_denominator;
  dts->packet_buff = malloc(sizeof(void *)*4);

  /* Theora has a divisible-by-sixteen restriction for the encoded video size */
  /* scale the frame size up to the nearest /16 and calculate offsets */
  dts->video_x = ((dts->frame_x + 15) >>4)<<4;
  dts->video_y = ((dts->frame_y + 15) >>4)<<4;
  dts->frame_x_offset = (dts->video_x - dts->frame_x)/2;
  dts->frame_y_offset = (dts->video_y - dts->frame_y)/2;
  
  dts->video_an = -1;
  dts->video_ad = -1;
  dts->video_r = -1;
  dts->video_q = 8;

  theora_info_init(&ti);
  ti.width = dts->video_x;
  ti.height = dts->video_y;
  ti.frame_width = dts->frame_x;
  ti.frame_height = dts->frame_y;
  ti.offset_x = dts->frame_x_offset;
  ti.offset_y = dts->frame_y_offset;
  ti.fps_numerator = dts->video_raten;
  ti.fps_denominator = dts->video_rated;
  ti.aspect_numerator = dts->video_an;
  ti.aspect_denominator = dts->video_ad;
  ti.colorspace = OC_CS_UNSPECIFIED;
  ti.target_bitrate = dts->video_r;
  ti.quality = dts->video_q;

  ti.dropframes_p = 0;
  ti.quick_p = 1;
  ti.keyframe_auto_p = 1;
  ti.keyframe_frequency = 64;
  ti.keyframe_frequency_force = 64;
  ti.keyframe_data_target_bitrate = dts->video_r*1.5;
  ti.keyframe_auto_threshold = 80;
  ti.keyframe_mindistance = 8;
  ti.noise_sensitivity = 1;

  theora_encode_init(&dts->td,&ti);
  theora_info_clear(&ti);

  return dts;
}

ogg_packet **
dryice_theora_encode_headers(dryice_theora_state *dts) {
  
  theora_comment tc;

  theora_encode_header(&dts->td, &dts->packet_buff[0]);
  theora_comment_init(&tc);
  theora_encode_comment(&tc, &dts->packet_buff[1]);
  theora_encode_tables(&dts->td, &dts->packet_buff[2]);
  &dts->packet_buff[3] = 0;  /* null-terminate the array */

  return dts->packet_buff;
}

ogg_packet **
dryice_theora_encode_frame(dryice_theora_state *dts,
                           char *yuvframe) {
  signed char        *line;
  int i, e;

  yuv.y_width = dts->video_x;
  yuv.y_height = dts->video_y;
  yuv.y_stride = dts->video_x;

  yuv.uv_width = dts->video_x / 2;
  yuv.uv_height = dts->video_y / 2;
  yuv.uv_stride = dts->video_x / 2;

  yuv.y = yuvframe;
  yuv.u = yuvframe + dts->video_x * dts->video_y;
  yuv.v = yuvframe + dts->video_x * dts->video_y * 5/4 ;
  
  theora_encode_YUVin(&dts->td, &yuv);
  theora_encode_packetout(&dts->td, 0, &dts->packet_buff[0]);
  &dts->packet_buff[1] = 0;
 
  return dts->packet_buff;
}

void
dryice_theora_clear(dryice_theora_state *dts) {
  free(dts->packet_buff);
  theora_clear(&dts->td);
  free(dts);
}
