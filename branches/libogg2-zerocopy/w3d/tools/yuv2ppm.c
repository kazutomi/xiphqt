/**
 *  Conversion utility from the VQEG .yuv format to .ppm's
 *  A readme on http://ftp.crc.ca/test/pub/crc/vqeg/TestSequences/
 *  describes the file format like this:
 *
 *
 *  10 Frames(Not used) + 8seconds Video + 10 Frames(Not used)
 *
 * The 10 frames of unused video allow enough frames for an MPEG2
 * codec to stabilize. Objective models will skip these frames.
 *
 * There are two video formats 525@60Hz and 625@50Hz. The format may be
 * identified by the sequence file name. The 525@60Hz sequence file names end
 * with _525.yuv and the 625@50Hz file names end with _625.yuv. Both formats
 * contain 720 pixels (1440 bytes) per horizontal line. The 525@60 sequences 
 * have 486 active lines per frame and the 625@50 sequences have 576 active 
 * lines per frame. 
 *
 * Each line is in pixel multiplexed 4:2:2 component video format as follows:
 *
 *  Cb Y Cr Y ...
 *  720  Y bytes per line
 *  360 Cb bytes per line
 *  360 Cr bytes per line
 *
 * Lines are concatenated into frames and frames are concatenated to form the
 * sequence files.
 *
 * All sequences are from interlaced video source except src13 which was 
 * converted from 24Hz film by the 3/2 pulldown method. The lines of the two 
 * fields are interlaced into the frames. The top field of the 525@60Hz 
 * material is temporally LATER than the bottom field (Bottom field first) and 
 * the top field of the 625@50Hz material is temporally EARLIER than the bottom
 * field (Top field first).  
 *
 * The frame sizes are:
 * 
 *  525@60Hz Frame size = 1440 x 486 = 699840 bytes/frame
 *  625@50Hz Frame size = 1440 x 576 = 829440 bytes/frame
 *
 * [...snip...]
 */

#include <stdint.h>
#include <stdio.h>
#include "../pnm.h"
#include "../mem.h"


static
void usage (char *progname)
{
   printf ("\n"
           "usage: %s <in.yuv> <ppmnames>\n"
           "\n"
           "        where <in.yuv> is the input .yuv sequence and\n"
           "        and <ppmnames> is a template like image%%03d.ppm\n"
           "\n", progname);
   exit (-1);
}


static inline
uint8_t CLAMP(int16_t x)
{
   return  ((x > 255) ? 255 : (x < 0) ? 0 : x);
}


static
int yuv2ppm (const char *yuvname, const char *fmt, int lines)
{
   char fname[256];
   unsigned char *yuv;
   unsigned char *y;
   unsigned char *u, *v;
   unsigned char *buf, *rgb;
   int i, j, frame;
   FILE *f;

   if (!(f = fopen (yuvname, "r")))
      return -1;

   buf = (char*) MALLOC (3 * 720 * lines * sizeof(char));
   yuv = (char*) MALLOC (1440 * lines);

   for (frame=0; frame<28; frame++) { 
      snprintf (fname, 256, fmt, frame);
      printf ("write '");
      printf (fname, frame);
      printf ("'");
      if (fread (yuv, 1440, lines, f) != lines) {
         printf ("\n\n unexpected end of file !!\n\n");
         FREE(buf);
         FREE (yuv);
         fclose (f);
         return -1;
      }
      for (j=0; j<lines; j++) {
         rgb = buf + 3 * j * 720;
         y = yuv + j * 1440 + 1;
         u = yuv + j * 1440;
         v = yuv + j * 1440 + 2;
         for (i=0; i<720; i++, rgb+=3) {
            int k = i/2;
            int _y = y [2*i];
            int _u = u [4*k] - 128;
            int _v = v [4*k] - 128;
            rgb [0] = CLAMP(_y + 1.371 * _v);
            rgb [1] = CLAMP(_y - 0.698 * _v - 0.336 * _u);
            rgb [2] = CLAMP(_y + 1.732 * _u);
         }
      }
      write_pnm (fname, buf, 720, lines);
      printf ("\n");
   }
   FREE (buf);
   FREE (yuv);
   fclose (f);
   return 0;
}


int main (int argc, char **argv)
{
   int lines_per_frame;

   if (argc != 3)
      usage (argv[0]);

   if (strstr(argv[1], "625"))
      lines_per_frame = 576;
   else if (strstr(argv[1], "525"))
      lines_per_frame = 486;
   else {
      printf ("\n"
              "could not determine number of lines per frame, .yuv filename \n"
              "must contain either the string \"525\" or \"625\"\n"
              "\n");
      return -1;
   }

   printf ("use %i lines per frame.\n", lines_per_frame);

   yuv2ppm (argv[1], argv[2], lines_per_frame);

   return 0;
}

