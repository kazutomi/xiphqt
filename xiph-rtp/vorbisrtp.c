/*
  File: Vorbis RTP Server                                                 
  Authors: Phil Kerr, Luca Barbato
  Date: 05/01/2005
  Platform: Linux

  Copyright (c) 2005, Fluendo / Xiph.Org

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.

  * Neither the name of the Xiph.Org nor the names of its contributors may 
    be used to endorse or promote products derived from this software without 
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
  THE POSSIBILITY OF SUCH DAMAGE.

*/                  

/*  System includes  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/*  Codec includes  */

#include <vorbis/codec.h>

/*  Network includes  */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Common include  */

#define HAVE_VORBIS

#include "xiph_rtp.h"

#define BUFFER_SIZE 4096


int main (int argc, char **argv) 
{
	xiph_rtp_t xr;
	
	char *buffer;
	int  bytes;

	char *filename;
	FILE *file;
    
    	ogg_sync_init (&xr.oy);

	int i = 0;
	int opt;

	char *ip = "227.0.0.1";
	unsigned int port = 4044;
	unsigned int ttl  = 1;
	long timestamp = 0, prev = 0;

    	fprintf (stderr, "Vorbis RTP Server (draft-kerr-avt-vorbis-rtp-05)\n");
	memset (&xr,0,sizeof(xiph_rtp_t));

/*  Command-line args processing  */

    if (argc < 2) {
        fprintf (stderr, "\n\tNo Vorbis file specified.\n");
        fprintf (stderr, "\tUsage: %s [-i ip address] [-p port] [-t ttl] filename\n\n",argv[0]);
        exit (1);
    }

    while ((opt = getopt (argc, argv, "i:p:t:")) != -1) {
        switch (opt)
	    {

        /*  Set IP address  */
	    case 'i':
    	        ip = optarg;
	        break;

        /*  Set port  */
	    case 'p':
	            port = atoi (optarg);
	        break;

        /*  Set TTL value  */
	    case 't':
    	        ttl = atoi (optarg);
	        break;

        /* Unknown option  */
	    case '?':
	            fprintf (stderr, "\n  Unknown option `-%c'.\n", optopt);
		    fprintf (stderr, "\tUsage: %s [-i ip address] [-p port] [-t ttl] filename\n\n",argv[0]);
	        return 1;
        }
    }

/*  Init RTP socket  */

    if ( createsocket (&xr, ip, port, ttl)<0) return 1;

/*  Print network details  */

    fprintf (stdout, "\n  Network setup\n");	  
    fprintf (stdout, "  IP Address= %s\n", ip);
    fprintf (stdout, "  Port = %d\n", port);
    fprintf (stdout, "  TTL = %d\n", ttl);
    fprintf (stdout, "\n");

/*  Open Vorbis file  */

    filename = argv [argc - 1];
    file = fopen (filename, "rb");

    if (file == NULL) {
        fprintf (stderr, "  Could not open file %s\n", filename);
        exit (1);
    }

    fprintf (stdout, "  Vorbis setup\n");
    fprintf (stdout, "  Filename: %s\n", filename);

    int eos = 0;

    buffer = ogg_sync_buffer (&xr.oy, BUFFER_SIZE);

    bytes = fread (buffer, 1, BUFFER_SIZE, file);

    ogg_sync_wrote (&xr.oy, bytes);
    
    if (ogg_sync_pageout (&xr.oy, &xr.og) != 1) {
        if (bytes < BUFFER_SIZE) {
            fprintf (stdout, "  Done\n");
            exit (0);
        }
      
        fprintf (stderr, "\n  Input does not appear to be an Ogg bitstream.\n");
        exit (1);
    }
  
    ogg_stream_init (&xr.os, ogg_page_serialno (&xr.og));
    
    vorbis_info_init (&xr.vi);
    vorbis_comment_init (&xr.vc);

    if (ogg_stream_pagein (&xr.os, &xr.og) < 0) { 
        fprintf (stderr, "  Error reading first page of Ogg bitstream data.\n");
        exit (1);
    }
    
    if (ogg_stream_packetout (&xr.os, &xr.op) != 1) { 
        fprintf (stderr, "  Error reading initial header packet.\n");
        exit (1);
    }
    
    if (vorbis_synthesis_headerin (&xr.vi, &xr.vc, &xr.op) < 0) { 
        fprintf (stderr, "  This Ogg bitstream does not contain Vorbis audio data.\n");
        exit (1);
    }

    ogg_copy_packet(&(xr.header[i]), &xr.op); //FIXME change it?

/*===========================================================================*/
/*  Process comment and codebook headers                                     */
/*===========================================================================*/

    while (i < 2) {
        while (i < 2) {
            int result = ogg_sync_pageout (&xr.oy, &xr.og);

	    if (result == 0) break; /* Need more data  */

            if (result == 1) {
		ogg_stream_pagein (&xr.os, &xr.og); 

                while(i < 2) {
            	    result = ogg_stream_packetout (&xr.os, &xr.op);

                    if (result == 0) break;
                    
                    if (result < 0) {
                        fprintf (stderr, "  Corrupt secondary header.  Exiting.\n");
                        exit (1);
    	            }

                    vorbis_synthesis_headerin (&xr.vi, &xr.vc, &xr.op);
                    i++;

                    ogg_copy_packet(&(xr.header[i]), &xr.op);
                }
            }
        }

        buffer = ogg_sync_buffer (&xr.oy, BUFFER_SIZE);
        bytes = fread (buffer, 1, BUFFER_SIZE, file);

        if (bytes == 0 && i < 2) {
            fprintf (stderr, "||  End of file before finding all Vorbis headers!\n");
            exit (1);
        }

        ogg_sync_wrote (&xr.oy, bytes);
    }

    xr.bitfield.cbident = rand ();
    
    vorbis_synthesis_init (&xr.vd, &xr.vi);
    vorbis_block_init (&xr.vd, &xr.vb);

/*===========================================================================*/
/*  Print details                                                            */
/*===========================================================================*/

    fprintf (stdout, "  Bitstream is %d channel, %ldHz\n", xr.vi.channels, xr.vi.rate);
    fprintf (stdout, "  Encoded by: %s\n", xr.vc.vendor);
    fprintf (stdout, "  Bitrates: min=%ld - nom=%ld - max=%ld\n", xr.vi.bitrate_lower, xr.vi.bitrate_nominal, xr.vi.bitrate_upper);
    fprintf (stdout, "  Decode setup ident is 0x%06x\n", xr.bitfield.cbident);
    fprintf (stdout, "\n");
    fprintf (stdout, "||  Processing\n");

/*===========================================================================*/
/*  Send the three headers inline                                            */
/*===========================================================================*/
{
int conf_bytes = xr.header[0].bytes + xr.header[2].bytes;
unsigned char *conf_packet = malloc(conf_bytes);

    memcpy (conf_packet, xr.header[0].packet, xr.header[0].bytes);
    memcpy (conf_packet + xr.header[0].bytes, 
		    xr.header[2].packet,
		    xr.header[2].bytes);
    creatertp(&xr, conf_packet, conf_bytes, 0, 1, 0);
    
    free(conf_packet);
}

/*===========================================================================*/
/*  Read raw Vorbis data and send RTP packet                                 */
/*===========================================================================*/

    while (!eos) {
        while (!eos) {
            int result = ogg_sync_pageout (&xr.oy, &xr.og);

            if (result == 0) break; /* need more data  */

            if (result < 0) {
                fprintf (stderr, "\n  Corrupt or missing data in bitstream; continuing....\n  ");
            } else {
                ogg_stream_pagein (&xr.os, &xr.og);

                while (1) {
                    result = ogg_stream_packetout (&xr.os, &xr.op);

                    if (result == 0) break; /* need more data  */

                    if (result < 0) {
                        /* no reason to complain; already complained above  */
            	    } else {
#ifdef DEBUG
			printf("  bytes %ld bos %ld eos %ld gp %lld pno %lld\n", xr.op.bytes, xr.op.b_o_s, xr.op.e_o_s, xr.op.granulepos, xr.op.packetno);
#endif
			creatertp ( &xr, xr.op.packet, xr.op.bytes, 
					timestamp, 0, xr.op.e_o_s );
			timestamp = ( vorbis_packet_blocksize(&xr.vi,&xr.op) +
					prev ) / 4*1000000L / xr.vi.rate;
			prev = vorbis_packet_blocksize(&xr.vi,&xr.op);
            	    }
                }

                if (ogg_page_eos (&xr.og)) eos = 1;
            }
        }

        if (!eos) {
            buffer = ogg_sync_buffer (&xr.oy, BUFFER_SIZE);
            bytes = fread (buffer, 1, BUFFER_SIZE, file);

            ogg_sync_wrote (&xr.oy, bytes);
    
            if (bytes == 0) eos = 1;
        }
    }
//FIXME make it a cleanup function!
    ogg_packet_clear (&(xr.header[0]));
    ogg_packet_clear (&(xr.header[1]));
    ogg_packet_clear (&(xr.header[2]));

    ogg_stream_clear (&xr.os);
  
    vorbis_block_clear (&xr.vb);
    vorbis_dsp_clear (&xr.vd);
    vorbis_comment_clear (&xr.vc);
    vorbis_info_clear (&xr.vi);

    ogg_sync_clear (&xr.oy);
    fclose (file);

    fprintf (stdout, "Done.\n");
    return (0);
}
