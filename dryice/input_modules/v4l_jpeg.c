/*
 * v4l_jpeg.c, DryIce input module for Video4Linux JPEG video
 *
 * Based on getjpeg.c by Joerg Heckenbach <joerg@heckenbach-aw.de>
 * which is included with the ov51x USB webcam Linux kernel module.  
 *
 * Copyright (c) 2004 Arc Riley <arc@xiph.org>
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
 *   last mod: $Id: v4l_jpeg.c,v 1.2 2004/03/02 11:10:03 arc Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libgen.h>
#include <jpeglib.h>

void
jinit_source (j_decompress_ptr cinfo)
{ 
  /* could move the read frame section in main() to here */
}

boolean
jfill_input_buffer (j_decompress_ptr cinfo)
{
  return FALSE;
}

void
jskip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  if ( num_bytes > 0 ) {
    cinfo->src->next_input_byte += num_bytes;
    cinfo->src->bytes_in_buffer -= num_bytes;  
  }
}

void
jterm_source (j_decompress_ptr cinfo)
{
  /* Nothing to do here */
}

typedef struct {
  int video_in;
  char video_dev[256] = "/dev/video";

  unsigned char input_buffer[65536]; /* 64k should be large enough */
  JSAMPROW *in_frame_buffer = 0;
  unsigned char *out_frame_buffer = 0;

  /* libjpeg stuff */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_source_mgr jsrc;
  
} dryice_v4l_jpeg_state;

void *
dryice_v4l_jpeg_init(dryice_params *dp) {

  dryice_v4l_jpeg_state *dvjs;

  dvjs = malloc(sizeof(dryice_v4l_jpeg_state));

  if ((dvjs->video_in = open(device, O_RDWR)) < 0) {
    printf("Could not open device %s\n", device);
    free(dvjs); /* If we return null then we must free this first */
    return NULL;
  }

  /* Initialise JPEG decode */
  
  dvjs->jsrc.init_source = jinit_source;
  dvjs->jsrc.fill_input_buffer = jfill_input_buffer;
  dvjs->jsrc.skip_input_data = jskip_input_data;
  dvjs->jsrc.resync_to_restart = jpeg_resync_to_restart;
  dvjs->jsrc.term_source = jterm_source;

  dvjs->cinfo.err = jpeg_std_error(&dvjs->jerr);
  jpeg_create_decompress(&dvjs->cinfo);
  dvjs->cinfo.src = &dvjs->jsrc;

  /* should probobally tell camera to start sending frames */

  return dvjs;
}

/* these are from old main()
  ssize_t count, bytes;
  int num_frames=1, frame=0;



  int a, c, pixels = 0, col, row;
 */

int
dryice_v4l_jpeg_get_params(dryice_v4l_jpeg_state *dvjs,
                           dryice_video_params *dvp) {

  dvp->frame_width = dvjs->cinfo.output_width;
  dvp->frame_height = dvjs->cinfo.output_height;
  dvp->framerate_numerator = 15; /* these should come from camera */
  dvp->framerate_denominator = 1;

  return 1;
}

char *
dryice_v4l_jpeg_get_frame(dryice_v4l_jpeg_state *dvjs) {
  // First read 2 bytes of length
  bytes = 0;
  read(ifid, input_buffer, 2);
  count = (input_buffer[1] * 2048) + (input_buffer[0] * 8);
  printf("%d bytes, I think.\n", count);
  while ( bytes < count ) { 
    c = read(ifid, input_buffer+bytes, count-bytes);
    if (c < 1) {
      printf("read returned %d, sleeping", c);
      sleep(1);
    } else {
      bytes += c; 
      printf("%d bytes read\n", bytes);
    }
  }
  jsrc.next_input_byte = input_buffer;
  jsrc.bytes_in_buffer = count;
              
  jpeg_read_header(&cinfo, TRUE);
  cinfo.out_color_space = JCS_YCbCr;
  cinfo.scale_num = 1;
  cinfo.scale_denom = 2;
  jpeg_start_decompress(&cinfo);
   /* malloc an array of pointers representing rows and a frame buffer, 
     then set the row pointers to the start of each row in the buffer,
     and then malloc a second frame buffer for a converted output */
  if ( in_frame_buffer == 0 ) {
    pixels = cinfo.output_height * cinfo.output_width;
    in_frame_buffer = malloc( sizeof(JSAMPROW) * cinfo.output_height );
    *in_frame_buffer = (JSAMPLE *) malloc( sizeof(JSAMPLE) * pixels * 
                                           cinfo.output_components );
    for ( c = 1 ; c < cinfo.output_height ; c++ )
      in_frame_buffer[c] = (JSAMPLE *) *in_frame_buffer + 
                                       ( c * cinfo.output_width * 
                                         cinfo.output_components );
     /* might as well do these while we're at it */
    out_frame_buffer = malloc ( sizeof(char) * pixels * 1.5 ); 
  }

  /* read the lines into the frame buffer */
  while (cinfo.output_scanline < cinfo.output_height)
    jpeg_read_scanlines(&cinfo, (JSAMPROW *) in_frame_buffer + cinfo.output_scanline, cinfo.output_height);
  /* now copy the packed 4:4:4 frame buffer into planar 4:2:0 buffer */
  /* start by copying the Y over */
  for ( c = 0; c < pixels; c++ )
    out_frame_buffer[c] = in_frame_buffer[0][c*3];
    scalex = cinfo.output_width/2;
  scaley = cinfo.output_height/2;
  
  for ( row = 0; row < scaley; row++ ) {
    for ( col = 0; col < scalex; col++) {
      a =  in_frame_buffer[0][(((col*2)+(row*cinfo.output_width*2))*3)+1];
      a += in_frame_buffer[0][((((col*2)+(row*cinfo.output_width*2))+
                              cinfo.output_width)*3)+1];
      a += in_frame_buffer[0][(((col*2)+(row*cinfo.output_width*2))*3)+4];
      a += in_frame_buffer[0][((((col*2)+(row*cinfo.output_width*2))+
                              cinfo.output_width)*3)+4];
      (char) out_frame_buffer[col+(row * scalex)+pixels] = a/4; 
      a =  in_frame_buffer[0][(((col*2)+(row*cinfo.output_width*2))*3)+2];
      a += in_frame_buffer[0][((((col*2)+(row*cinfo.output_width*2))+
                              cinfo.output_width)*3)+2];
      a += in_frame_buffer[0][(((col*2)+(row*cinfo.output_width*2))*3)+5];
      a += in_frame_buffer[0][((((col*2)+(row*cinfo.output_width*2))+
                              cinfo.output_width)*3)+5];
      (char) out_frame_buffer[col+(row * scalex)+(pixels+(pixels/4))] = a/4;
    }
  }

  jpeg_finish_decompress(&cinfo);

}

  if ( in_frame_buffer != 0 ) {
    free(*in_frame_buffer);
    free(in_frame_buffer);
    free(out_frame_buffer);
  }
  jpeg_destroy_decompress(&cinfo);
  fclose(ofid);
  close(ifid);

  return 0;
}
