#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "mem.h"
#include "tarkin.h"
#include "pnm.h"
#include <ogg/ogg.h>

#define CHUNK_SIZE 4096

int main (int argc, char **argv)
{
   char *fname = "stream.ogg";
   char ofname [11];
   uint32_t frame = 0;
   uint8_t *rgb;
   int fd;
   TarkinStream *tarkin_stream;
   uint32_t n_layers;
   int nread;
   int nheader = 0;
   ogg_sync_state oy;
   ogg_stream_state os;
   ogg_page og;
   ogg_packet op;
   TarkinInfo ti;
   TarkinComment tc;
   TarkinTime date;
   
   char *buffer;
   
   TarkinVideoLayerDesc *layer;

   if (argc == 2) {
      fname = argv[1];
   } else if (argc != 1) {
      printf ("\n usage: %s <tarkin_stream>\n\n", argv[0]);
      exit (-1);
   }

   if ((fd = open (fname, O_RDONLY)) < 0) {
      printf ("error opening '%s'\n", fname);
      exit (-1);
   }

   tarkin_stream = tarkin_stream_new (fd);
   ogg_sync_init(&oy);
   ogg_stream_init(&os,1);
   tarkin_info_init(&ti);
   tarkin_comment_init(&tc);
   while(1){
      buffer = ogg_sync_buffer(&oy, CHUNK_SIZE);
      if((nread = read(fd, buffer, CHUNK_SIZE))>0)
	  ogg_sync_wrote(&oy, nread);
      else{
	  ogg_sync_wrote(&oy,0);
      }	  
      if(ogg_sync_pageout(&oy,&og)){
          ogg_stream_pagein(&os,&og);
	  while(ogg_stream_packetout(&os,&op)){
             if(op.e_o_s)
                break;
             if(nheader<3){ /* 3 first packets to headerin */
                tarkin_synthesis_headerin(&ti, &tc, &op);
		if(nheader == 2){
                   tarkin_synthesis_init(tarkin_stream, &ti);
		}
	        nheader++;
             } else {
		tarkin_synthesis_packetin(tarkin_stream, &op);
		while(tarkin_synthesis_frameout(tarkin_stream, &rgb, 0, &date)==0){
		  layer = tarkin_stream->layer;
	          snprintf(ofname, 11, layer->format == TARKIN_GRAYSCALE ? "out%03d.pgm" : "out%03d.ppm", frame);
                  printf ("write '%s' %dx%d\n", ofname, layer->width, layer->height);
                  write_pnm (ofname, rgb, layer->width, layer->height);
		  tarkin_synthesis_freeframe(tarkin_stream, rgb);

                  frame ++;
		}
	     }
	  }
      }
      if(nread==0)
	  break;
   }
   tarkin_stream_destroy (tarkin_stream);
   close (fd);

   return 0;
}

