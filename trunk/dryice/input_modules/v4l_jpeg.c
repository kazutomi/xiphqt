/*
 * v4l_jpeg.c, an input module for dryice
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
 *   last mod: $Id: v4l_jpeg.c,v 1.1 2004/03/02 03:32:02 arc Exp $
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
usage (char *pname)
{
  fprintf (stderr,
           "Usage: %s <options>\n"
           " -d <device>          video device (default: /dev/video)\n"
           " -o <file>            write output to file instead of stdout\n"
           "Example: %s -d /dev/video0 -o imagefile\n",
           (char*)basename(pname), (char*)basename(pname));
  exit (1);
}

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

int main(int argc, char **argv)
{
  int ifid;
  FILE *ofid;
  ssize_t count, bytes;
  int num_frames=1, frame=0;

  unsigned char input_buffer[65536]; /* 64k should be large enough */
  JSAMPROW *in_frame_buffer = 0;
  unsigned char *out_frame_buffer = 0;

  char default_dev[] = "/dev/video";
  char *device = default_dev;
  char default_filename[] = "stream.yuv";
  char *filename = default_filename;

  int a, c, pixels = 0, col, row, scalex, scaley;
 
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_source_mgr jsrc;
  
  jsrc.init_source = jinit_source;
  jsrc.fill_input_buffer = jfill_input_buffer;
  jsrc.skip_input_data = jskip_input_data;
  jsrc.resync_to_restart = jpeg_resync_to_restart;
  jsrc.term_source = jterm_source;

  while ((c = getopt (argc, argv, "d:o:s:n:")) != EOF) {
    switch (c) {
      case 'd': /* change default device */
        device = optarg;
        break;
      case 'o':
        filename = optarg;
        break;
      case 'n':
        sscanf (optarg, "%d", &num_frames);
        break;
      default:
        usage (argv[0]);
        break;
    }
  }

  if ((ifid = open(device, O_RDWR)) < 0) {
    printf("Could not open device %s\n", device);
    return -1;
  }

/*
  if ((ifid = fopen(device, "rb")) == NULL) {
    printf("Could not open device %s\n", device);
    return -1;
  }
*/

  if ((ofid = fopen(filename, "wb")) == NULL) {
    printf("Could not open file %s for writing\n", filename);
    return -1;
  } 

  /* Initialise JPEG decode */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  cinfo.src = &jsrc;

  for (frame = 0; frame < num_frames; frame++) {
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
      printf("%dX%d %d\n", cinfo.output_width, cinfo.output_height, cinfo.output_components);
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
      fprintf(ofid, "YUV4MPEG2 W%d H%d F10:1 Ip A1:1\n",
                    cinfo.output_width, cinfo.output_height);
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

    fprintf(ofid, "FRAME\n");
    fwrite(out_frame_buffer, sizeof(char), pixels * 1.5, ofid);

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
