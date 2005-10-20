/*****************************************************************************
||  File: Vorbis RTP Server                                                 
||  Authors: Phil Kerr, Luca Barbato
||  Date: 05/01/2005
||  Platform: Linux
||
||  Copyright (c) 2005, Fluendo / Xiph.Org
||
||  Redistribution and use in source and binary forms, with or without 
||  modification, are permitted provided that the following conditions are met:
||
||  * Redistributions of source code must retain the above copyright notice, 
||    this list of conditions and the following disclaimer.
||
||  * Redistributions in binary form must reproduce the above copyright notice, 
||    this list of conditions and the following disclaimer in the documentation 
||    and/or other materials provided with the distribution.
||
||  * Neither the name of the Xiph.Org nor the names of its contributors may 
||    be used to endorse or promote products derived from this software without 
||    specific prior written permission.
||
||  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
||  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
||  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
||  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
||  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
||  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
||  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
||  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
||  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
||  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
||  THE POSSIBILITY OF SUCH DAMAGE.
||
||*****************************************************************************
||
||  Compile:  gcc vorbisrtp.c -o vorbisrtp -lvorbis -logg -fpack-struct -Wall
||
||  $Id:$                                                       
||                  
||****************************************************************************/
/*  General includes                                                         */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/*****************************************************************************/
/*  Vorbis includes                                                          */
/*****************************************************************************/

#include <vorbis/codec.h>

/*****************************************************************************/
/*  Network includes                                                         */
/*****************************************************************************/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/*****************************************************************************/
/*  Data structs and variables                                               */
/*****************************************************************************/

ogg_int16_t convbuffer [4096];

struct sockaddr_in rtpsock;
int rtpsocket;

/*****************************************************************************/
/*  Vorbis packet header                                                     */
/*                                                                           */
/*  fragmentation flags work like so:                                        */
/*                                                                           */
/*  C F                                                                      */
/*  0 0  Unfragmented packet (aka multi-packet..packet)                      */
/*  0 1  First fragmented packet                                             */
/*  1 0  Middle fragment                                                     */
/*  1 1  Last fragment.                                                      */
/*                                                                           */
/*****************************************************************************/

struct VorbisBitfields {
    unsigned int cbident:24;
    unsigned int frag_type:2;
    unsigned int data_type:2;
    unsigned int pkts:4;
} VorbisBitfields;

/*****************************************************************************/
/*  RTP header                                                               */
/*****************************************************************************/

struct RTPHeaders {
/*  v, p, x, cc flags  */
    unsigned int flags1:8;

/*  m, pt flags  */
    unsigned int flags2:8;

    unsigned int sequence:16;
    unsigned int timestamp:32;
    unsigned int ssrc:32;
} RTPHeaders;

/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/

void progressmarker (int type);


int createsocket (struct RTPHeaders *RTPHeaders, struct sockaddr_in *sockAddr, char *addr, unsigned int port, unsigned char TTL);
void creatertp (unsigned char* vorbdata, int length, long timestamp, struct VorbisBitfields *vorbheader, int type);
int sendrtp (struct RTPHeaders *RTPHeaders, int fd, struct sockaddr_in *sockAddr, const void *data, int len);

int makevorbisheader (unsigned char *packet, int length, struct VorbisBitfields *vorbheader);
int ogg_copy_packet(ogg_packet *dst, ogg_packet *src);


/*****************************************************************************/
/*  Print progress marker                                                    */
/*****************************************************************************/

void progressmarker (int type)
{

#if 0	
/*===========================================================================*/
/*  Prints output markers. 0 = normal, 1 = frag, 2 = last frag,              */
/*  3 = comment, 4 = codebook, 5 = packed, 6 = config                        */
/*===========================================================================*/

/*  Output marker position  */
    static int outmarkpos = 0;
    switch (type) {
        case 0:
            fprintf (stdout, "."); /* normal */
        break;

        case 1:
            fprintf (stdout, "+"); /* fragment */
        break;

        case 2:
            fprintf (stdout, "|"); /* final fragment */
        break;

        case 3:
            fprintf (stdout, "m"); /* metadata packet */
        break;

        case 4:
            fprintf (stdout, "b"); /* setup header */
        break;

        case 5:
            fprintf (stdout, "p"); /* packed */
        break;

        case 6:
            fprintf (stdout, "c"); /* info header */
        break;
    };

    fflush (NULL);

    if (outmarkpos == 70) {
        fprintf (stdout, "\n||  ");
        outmarkpos = 0;
    } else 
        outmarkpos++;
#endif
    return;
}

/*****************************************************************************/
/*  Creates RTP socket                                                       */
/*****************************************************************************/

int createsocket (struct RTPHeaders *RTPHeaders, struct sockaddr_in *sockAddr, char *addr, unsigned int port, unsigned char TTL)
{
    int RTPSocket, ret;
    int optval = 0;
    struct sockaddr_in sin;

/*===========================================================================*/
/*  Init RTP headers                                                         */
/*===========================================================================*/

/*  Sets v=2, p=0, x=0, cc=0  */
    RTPHeaders -> flags1 = 0x80;

/*  Sets m=0, pt=96  */
    RTPHeaders -> flags2 = 0x60;

    RTPHeaders -> sequence = htonl (rand () & 65535);
    RTPHeaders -> timestamp = htonl (rand ());
    RTPHeaders -> ssrc = htonl (rand ());

/*===========================================================================*/
/*  Create socket                                                            */
/*===========================================================================*/

    RTPSocket = socket (AF_INET, SOCK_DGRAM, 0);

    if (RTPSocket < 0) {
        fprintf (stderr, "Socket creation failed.\n");
        exit (1);
    }

    sockAddr -> sin_family = sin.sin_family = AF_INET;
    sockAddr -> sin_port = sin.sin_port = htons (port);
    sockAddr -> sin_addr.s_addr = inet_addr (addr);

    ret = setsockopt (RTPSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int));

    if (ret < 0) {
        fprintf (stderr, "setsockopt SO_REUSEADDR error\n");
        exit (1);
    }

/*===========================================================================*/
/*  Set multicast parameters                                                 */
/*===========================================================================*/

    if (IN_MULTICAST (ntohl (sockAddr -> sin_addr.s_addr))) {

        ret = setsockopt (RTPSocket, IPPROTO_IP, IP_MULTICAST_TTL, &TTL, sizeof (char));

        if (ret < 0) {
            fprintf (stderr, "Multicast setsockopt TTL failed.\n");
            exit (1);
        }

        optval = 1; 

        ret = setsockopt (RTPSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof (char));

        if (ret < 0) {
            fprintf (stderr, "Multicast setsockopt LOOP failed.\n");
            exit (1);
        }
    }

    return RTPSocket;
}

/*****************************************************************************/
/*  Fills in the Vorbis RTP header in a packet.                              */
/*****************************************************************************/

int makevorbisheader (unsigned char *packet, int length, struct VorbisBitfields *vorbheader)
{
    if (length < 6) return -1;
    printf("||  ident %06x, frag type %d, data type %d, pkts %02d, size %d\n",
		    vorbheader->cbident, vorbheader -> frag_type,
		    vorbheader -> data_type, vorbheader -> pkts,
		    length);

    packet[0] = (vorbheader->cbident&0xff0000)>>16;
    packet[1] = (vorbheader->cbident&0xff00)>>8;
    packet[2] = vorbheader->cbident&0xff;
    
    packet[3] = (vorbheader -> frag_type) << 6;
    packet[3] |= (vorbheader -> data_type) << 4;
    packet[3] |= (vorbheader -> pkts) & 0xf;
    
    return 0;
}

/*****************************************************************************/
/*  Creates RTP packet from Vorbis frame.                                    */
/*  Deals with fragmentation and multiple Vorbis frame RTP packets           */
/*****************************************************************************/

void creatertp (unsigned char* vorbdata, int length, long timestamp, struct VorbisBitfields *vorbheader, int type)
{
    int sleeptime, frag, position = 0;
    unsigned short framesize;
    unsigned char *packet;

    const unsigned int max_payload = 1000;

    /* we accumulate short frames between calls */
    static int stacksize = 0;
    static int stackcount = 0;
    static unsigned char* framestack = NULL;

/*===========================================================================*/
/*  Set sleeptime value based on timestamp                                   */
/*===========================================================================*/


if (type)
    sleeptime = 300; //  ((1 / (float) bitrate) * 1000000);
else
	sleeptime = timestamp;

/*===========================================================================*/
/*  Frame fragmentation                                                      */
/*===========================================================================*/

    if (length > max_payload) {
        frag = 1;
        while (length > max_payload) {        
            /*  Set Vorbis header flags  */
            vorbheader -> frag_type = frag;
            vorbheader -> data_type = type;
            vorbheader -> pkts = 0;

            framesize = max_payload;

            packet = malloc (framesize + 4 + 2);

            makevorbisheader (packet, framesize + 2 + 4, vorbheader);
 //           memcpy (packet + 4, &framesize, 2);
            packet[4]=(framesize&0xff00)>>8;
	    packet[5]=framesize&0xff;
            memcpy (packet + 4 + 2, vorbdata + position, framesize);

            /*  Swap RTP headers from host to network order  */
            RTPHeaders.sequence = htons (RTPHeaders.sequence);

            sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, framesize + 4 + 2);

            /*  Swap headers back to host order  */
            RTPHeaders.sequence = ntohs (RTPHeaders.sequence);

            length -= max_payload;
            position += max_payload;
            frag = 2;

            RTPHeaders.sequence = RTPHeaders.sequence + 1;

            progressmarker (1);

            free (packet);
        }

        /*  Set Vorbis header flags  */
        vorbheader -> frag_type = 3;
        vorbheader -> data_type = type;
        vorbheader -> pkts = 0;

        framesize = length;

        packet = malloc (length + 4 + 2);

        makevorbisheader (packet, length + 4 + 2, vorbheader);
        packet[4]=(framesize&0xff00)>>8;
	packet[5]=framesize&0xff;
        memcpy (packet + 4 + 2, vorbdata + position, length);

        /*  Swap RTP headers from host to network order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = htonl (RTPHeaders.timestamp);

        sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, length + 4 + 2);

        /*  Swap headers back to host order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = ntohl (RTPHeaders.timestamp);

        /*  Increment RTP headers  */
        RTPHeaders.sequence++;
        RTPHeaders.timestamp += sleeptime;

        progressmarker (2);

        free (packet);
        return;
    }

/*===========================================================================*/
/*  Frame packing.  Used only for type 0 packets (raw Vorbis data)           */
/*===========================================================================*/

    if (length < max_payload && type == 0) {
        if (length + stacksize < max_payload && stackcount < 15) {

            framestack = realloc (framestack, (stacksize + (length + 2)));
            
	    framestack[stacksize++]= (length&0xff00)>>8;
	    framestack[stacksize++]= length&0xff;
	    
            memcpy (framestack + (stacksize), vorbdata, length);
            stackcount++;
            stacksize += (length);
        }

        /* todo: we also need to be able to flush this at end-of-stream */

        if (length + stacksize > max_payload || stackcount >= 15) {

            /*  Set Vorbis header flags  */
            vorbheader -> frag_type = 0;
            vorbheader -> data_type = 0;
            vorbheader -> pkts = stackcount;

            packet = malloc (stacksize + 4);

            makevorbisheader (packet, stacksize + 4, vorbheader);
            memcpy (packet + 4, framestack, stacksize);

            /*  Swap RTP headers from host to network order  */
            RTPHeaders.sequence = htons (RTPHeaders.sequence);
            RTPHeaders.timestamp = htonl (RTPHeaders.timestamp);

            sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, stacksize + 4);

            /*  Swap headers back to host order  */
            RTPHeaders.sequence = htons (RTPHeaders.sequence);
            RTPHeaders.timestamp = ntohl (RTPHeaders.timestamp);

            if (stackcount == 1) 
                progressmarker (0);
            else
                progressmarker (5);

            usleep (sleeptime);

            RTPHeaders.sequence++;
            RTPHeaders.timestamp += sleeptime;

            stacksize = 0;
            stackcount = 0;

            free (packet);
        }   
        return;
    } 

/*===========================================================================*/
/*  Send header packets (under max_payload octets) - No Packing              */
/*===========================================================================*/

    else if (length < max_payload) {

        /*  Set Vorbis header flags  */
        vorbheader -> frag_type = 0;
        vorbheader -> data_type = type;
        vorbheader -> pkts = 1;

        framesize = (unsigned char) length;

        packet = malloc (length + 4 + 2);

        makevorbisheader (packet, length + 4 + 2, vorbheader);

	packet[4]=(length&0xff00)>>8;
	packet[5]=length&0xff;

        memcpy (packet + 4 + 2, vorbdata, length);

        /*  Swap RTP headers from host to network order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = htonl (RTPHeaders.timestamp);

        sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, length + 4 + 2);

        /*  Swap headers back to host order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = ntohl (RTPHeaders.timestamp);

        sleeptime = timestamp;
        usleep (sleeptime);

        RTPHeaders.sequence++;
        RTPHeaders.timestamp += sleeptime;

        free (packet);
    }            
}

/*****************************************************************************/
/*  Sends RTP packet                                                         */
/*****************************************************************************/

int sendrtp (struct RTPHeaders *RTPHeaders, int rtpsocket, struct sockaddr_in *sockAddr, const void *packet, int length)
{
    int ret;

    char *outbuffer;

    outbuffer = malloc (sizeof (struct RTPHeaders) + length);

    memcpy (outbuffer,  RTPHeaders, sizeof (struct RTPHeaders));
    memcpy (outbuffer + sizeof (struct RTPHeaders), packet, length);

    ret = sendto (rtpsocket, outbuffer, sizeof (struct RTPHeaders) + length, 0, (struct sockaddr *) sockAddr, sizeof (*sockAddr));

    free (outbuffer);
    return (ret);
}

/*****************************************************************************/
/*  Duplicates an Ogg packet                                                 */
/*****************************************************************************/
int ogg_copy_packet(ogg_packet *dst, ogg_packet *src)
{
  dst->packet = malloc(src->bytes);
  memcpy(dst->packet, src->packet, src->bytes);
  dst->bytes = src->bytes;
  dst->b_o_s = src->b_o_s;
  dst->e_o_s = src->e_o_s;

  dst->granulepos = src->granulepos;
  dst->packetno = src->packetno;
#ifdef DEBUG
  printf("||  bytes %ld bos %ld eos %ld gp %lld pno %lld\n",
  dst->bytes, dst->b_o_s, dst->e_o_s, dst->granulepos, dst->packetno);
#endif
  return 0;
}

/*****************************************************************************/

int main (int argc, char **argv) 
{
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    ogg_packet header[3];
  
    vorbis_info vi; 

    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;

    char *buffer;
    int  bytes;

    char *filename;
    FILE *file;
    ogg_sync_init (&oy);

    int i = 0;
    int opt;

    char *ip = "227.0.0.1";
    unsigned int port = 4044;
    unsigned int ttl  = 1;
    long timestamp = 0, prev = 0;

    fprintf (stderr, "||---------------------------------------------------------------------------||\n");
    fprintf (stderr, "||  Vorbis RTP Server (draft-kerr-avt-vorbis-rtp-05)\n");	  

/*===========================================================================*/
/*  Command-line args processing                                             */
/*===========================================================================*/

    if (argc < 2) {
        fprintf (stderr, "||\n||  No Vorbis file specified.\n");
        fprintf (stderr, "||  Usage: vorbisrtp [-i ip address] [-p port] [-t ttl] filename\n\n");
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
	            fprintf (stderr, "\n||  Unknown option `-%c'.\n", optopt);
	        return 1;
        }
    }

/*===========================================================================*/
/*  Init RTP socket                                                          */
/*===========================================================================*/

    rtpsocket = createsocket (&RTPHeaders, &rtpsock, ip, port, ttl);

/*===========================================================================*/
/*  Print network details                                                    */
/*===========================================================================*/

    fprintf (stdout, "||---------------------------------------------------------------------------||\n");
    fprintf (stdout, "||  Network setup\n");	  
    fprintf (stdout, "||  IP Address= %s\n", ip);
    fprintf (stdout, "||  Port = %d\n", port);
    fprintf (stdout, "||  TTL = %d\n", ttl);
    fprintf (stdout, "||\n");
    fprintf (stdout, "||---------------------------------------------------------------------------||\n");

/*===========================================================================*/
/*  Open Vorbis file                                                         */
/*===========================================================================*/

    filename = argv [argc - 1];
    file = fopen (filename, "rb");

    if (file == NULL) {
        fprintf (stderr, "||  Could not open file %s\n", filename);
        exit (1);
    }

    fprintf (stdout, "||  Vorbis setup\n");
    fprintf (stdout, "||  Filename: %s\n", filename);

    int eos = 0;

    buffer = ogg_sync_buffer (&oy, 4096);

    bytes = fread (buffer, 1, 4096, file);

    ogg_sync_wrote (&oy, bytes);
    
    if (ogg_sync_pageout (&oy, &og) != 1) {
        if (bytes < 4096) {
            fprintf (stdout, "||  Done\n");
            exit (0);
        }
      
        fprintf (stderr, "\n||  Input does not appear to be an Ogg bitstream.\n");
        exit (1);
    }
  
    ogg_stream_init (&os, ogg_page_serialno (&og));
    
    vorbis_info_init (&vi);
    vorbis_comment_init (&vc);

    if (ogg_stream_pagein (&os, &og) < 0) { 
        fprintf (stderr, "||  Error reading first page of Ogg bitstream data.\n");
        exit (1);
    }
    
    if (ogg_stream_packetout (&os, &op) != 1) { 
        fprintf (stderr, "||  Error reading initial header packet.\n");
        exit (1);
    }
    
    if (vorbis_synthesis_headerin (&vi, &vc, &op) < 0) { 
        fprintf (stderr, "||  This Ogg bitstream does not contain Vorbis audio data.\n");
        exit (1);
    }

    ogg_copy_packet(&(header[i]), &op);

/*===========================================================================*/
/*  Process comment and codebook headers                                     */
/*===========================================================================*/

    while (i < 2) {
        while (i < 2) {
            int result = ogg_sync_pageout (&oy, &og);

        	if (result == 0) break; /* Need more data  */

        	if (result == 1) {
        	    ogg_stream_pagein (&os, &og); 

                while(i < 2) {
            	    result = ogg_stream_packetout (&os, &op);

                    if (result == 0) break;
                    
                    if (result < 0) {
                        fprintf (stderr, "||  Corrupt secondary header.  Exiting.\n");
                        exit (1);
    	            }


                    vorbis_synthesis_headerin (&vi, &vc, &op);
                    i++;

                    ogg_copy_packet(&(header[i]), &op);
                }
            }
        }

        buffer = ogg_sync_buffer (&oy, 4096);
        bytes = fread (buffer, 1, 4096, file);

        if (bytes == 0 && i < 2) {
            fprintf (stderr, "||  End of file before finding all Vorbis headers!\n");
            exit (1);
        }

        ogg_sync_wrote (&oy, bytes);
    }

    VorbisBitfields.cbident = rand ();
    
    vorbis_synthesis_init (&vd, &vi);
    vorbis_block_init (&vd, &vb);

/*===========================================================================*/
/*  Print details                                                            */
/*===========================================================================*/

    fprintf (stdout, "||  Bitstream is %d channel, %ldHz\n", vi.channels, vi.rate);
    fprintf (stdout, "||  Encoded by: %s\n", vc.vendor);
    fprintf (stdout, "||  Bitrates: min=%ld - nom=%ld - max=%ld\n", vi.bitrate_lower, vi.bitrate_nominal, vi.bitrate_upper);
    fprintf (stdout, "||  Decode setup ident is 0x%06x\n", VorbisBitfields.cbident);
    fprintf (stdout, "||\n");
    fprintf (stdout, "||---------------------------------------------------------------------------||\n");
    fprintf (stdout, "||  Processing\n");

/*===========================================================================*/
/*  Send the three headers inline                                            */
/*===========================================================================*/
{
int conf_bytes = header[0].bytes+header[2].bytes;
unsigned char *conf_packet=malloc(conf_bytes);

    memcpy (conf_packet,header[0].packet,header[0].bytes);
    memcpy (conf_packet+header[0].bytes,header[2].packet,header[2].bytes);
    creatertp(conf_packet, conf_bytes, 0, &VorbisBitfields, 1);
    progressmarker (6);

free(conf_packet);
}

/*===========================================================================*/
/*  Read raw Vorbis data and send RTP packet                                 */
/*===========================================================================*/

    while (!eos) {
        while (!eos) {
            int result = ogg_sync_pageout (&oy, &og);

            if (result == 0) break; /* need more data  */

            if (result < 0) {
                fprintf (stderr, "\n||  Corrupt or missing data in bitstream; continuing....\n||  ");
            } else {
                ogg_stream_pagein (&os, &og); 

                while (1) {
                    result = ogg_stream_packetout (&os, &op);

                    if (result == 0) break; /* need more data  */

                    if (result < 0) {
                        /* no reason to complain; already complained above  */
            	    } else {
#ifdef DEBUG
			printf("||  bytes %ld bos %ld eos %ld gp %lld pno %lld\n", op.bytes, op.b_o_s, op.e_o_s, op.granulepos, op.packetno);
#endif
			creatertp ( op.packet, op.bytes, 
					timestamp, &VorbisBitfields, 0);
			timestamp = (vorbis_packet_blocksize(&vi,&op)+prev)/4*1000000L/vi.rate;
			prev = vorbis_packet_blocksize(&vi,&op);
            	    }
                }

                if (ogg_page_eos (&og)) eos = 1;
            }
        }

        if (!eos) {
            buffer = ogg_sync_buffer (&oy, 4096);
            bytes = fread (buffer, 1, 4096, file);

            ogg_sync_wrote (&oy, bytes);
    
            if (bytes == 0) eos = 1;
        }
    }

    ogg_packet_clear (&(header[0]));
    ogg_packet_clear (&(header[1]));
    ogg_packet_clear (&(header[2]));

    ogg_stream_clear (&os);
  
    vorbis_block_clear (&vb);
    vorbis_dsp_clear (&vd);
    vorbis_comment_clear (&vc);
    vorbis_info_clear (&vi);

    ogg_sync_clear (&oy);
    fclose (file);

    fprintf (stdout, "\n||---------------------------------------------------------------------------||\n");
    fprintf (stdout, "||  Done.\n");
    return (0);
}

/*****************************************************************************/
/*  End                                                                      */
/*****************************************************************************/
