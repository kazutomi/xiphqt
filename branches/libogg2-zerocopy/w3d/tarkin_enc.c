#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mem.h"
#include "tarkin.h"
#include "pnm.h"
#include "w3dtypes.h"


static
void usage (const char *program_name)
{
   printf ("\n"
     " usage: %s <input filename format string> <bitrate> <a_m> <s_m>\n"
     "\n"
     "   input ppm filename format:  optional, \"%%i.ppm\" by default\n"
     "   bitrate:                    cut Y/U/V bitstream after limit bytes/frame\n"
     "                               (something like 3000 makes sense here)\n" 
     "   a_m, s_m:                   number of vanishing moments of the\n"
     "                               analysis/synthesis filter, (2,2) by default\n"
     "\n" 
     " The resulting stream.tarkin will have bitrate*frame+sizeof(header) bytes.\n" 
     "\n", program_name);
   exit (-1);
}

TarkinError free_frame(void *s, void *ptr) {
    FREE (ptr);
    return(TARKIN_OK);
}

struct tarkin_enc {
    ogg_stream_state os;
    int fd;
};

TarkinError packet_out(void *stream, ogg_packet *op) {
    ogg_page og;
    TarkinStream *s = stream;
    struct tarkin_enc *te = s->user_ptr;
    ogg_stream_packetin(&te->os,op);
    if(op->e_o_s){
       ogg_stream_flush(&te->os, &og);
       write(te->fd, og.header, og.header_len);
       write(te->fd, og.body , og.body_len);
    } else {   
       while(ogg_stream_pageout(&te->os,&og)){
          write(te->fd, og.header, og.header_len);
          write(te->fd, og.body , og.body_len);
       }
    }   
    return (TARKIN_OK);
}

int main (int argc, char **argv)
{
   char *fmt = "%i.ppm";
   char fname[256];
   uint32_t frame = 0;
   uint8_t *rgb;
   struct tarkin_enc te;
   TarkinStream *tarkin_stream;
   TarkinVideoLayerDesc layer [] = { { 0, 0, 1, 5000, TARKIN_RGB24 } };
   int type,i;
   TarkinComment tc;
   TarkinInfo ti;
   ogg_page         og;
   ogg_packet       op[3];
   TarkinTime date;
   

   if (argc == 1) {
      layer[0].bitstream_len = 1000;
      layer[0].a_moments = 2;
      layer[0].s_moments = 2;
   } else if (argc == 5) {
      fmt = argv[1];
      layer[0].bitstream_len = strtol (argv[2], 0, 0);
      layer[0].a_moments = strtol (argv[3], 0, 0);
      layer[0].s_moments = strtol (argv[4], 0, 0);
   } else {
      usage (argv[0]);
   }

   snprintf (fname, 256, fmt, 0);
   type = read_pnm_header (fname, &layer[0].width, &layer[0].height);

   if (type < 0) {
      printf (" failed opening '%s' !!\n", fname);
      exit (-1);
   }

   layer[0].format = (type == 3) ? TARKIN_RGB24 : TARKIN_GRAYSCALE;


   if ((te.fd = open ("stream.ogg", O_CREAT | O_RDWR | O_TRUNC | O_BINARY, 0644)) < 0) {
      printf ("error opening '%s' for writing !\n", "stream.ogg");
      usage (argv[0]);
   }

   ogg_stream_init(&te.os,1);
   tarkin_info_init(&ti);

   ti.inter.numerator = 1;
   ti.inter.denominator = 1;

   tarkin_comment_init(&tc);
   tarkin_comment_add_tag(&tc, "TITLE", "tarkin_enc produced file");
   tarkin_comment_add_tag(&tc, "ARTIST", "C coders ;)");
   tarkin_stream = tarkin_stream_new ();
   tarkin_analysis_init(tarkin_stream, &ti, free_frame, packet_out,(void*)&te);
   tarkin_analysis_add_layer(tarkin_stream, &layer[0]);
   printf("n_layers: %d\n", tarkin_stream->n_layers);
   tarkin_analysis_headerout(tarkin_stream, &tc, op, &op[1], &op[2]);
   for(i=0;i<3;i++){
      ogg_stream_packetin(&te.os, &op[i]);
   }
   ogg_stream_flush(&te.os,&og);
   write(te.fd, og.header, og.header_len);
   write(te.fd, og.body, og.body_len);
	 

   do {
      rgb  = (uint8_t*) MALLOC (layer[0].width * layer[0].height * type);
      snprintf (fname, 256, fmt, frame);
      printf ("read '");
      printf (fname, frame);
      printf ("'");

      if (read_pnm (fname, rgb) < 0)
      {
         printf (" failed.\n");
         break;
      }
      printf ("\n");
      date.numerator = frame;
      date.denominator = 1;
      tarkin_analysis_framein (tarkin_stream, rgb, 0, &date);
      frame++;
   } while (1);

   FREE (rgb);
   tarkin_analysis_framein (tarkin_stream, NULL, 0, NULL); /* EOS */
   tarkin_comment_clear (&tc);
   tarkin_stream_destroy (tarkin_stream);
   close (te.fd);

   return 0;
}

