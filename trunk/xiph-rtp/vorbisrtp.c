/*****************************************************************************
||  File: Vorbis RTP Server                                                 
||  Author: Phil Kerr                                                        
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
int convsize = 4096;

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
    unsigned int cbident:32;
    int continuation:1;
    int fragment:1;
    int reserved:1;
    int pkts:5;
} VorbisBitfields;

/*****************************************************************************/
/*  Vorbis configuration packet                                              */
/*****************************************************************************/

struct VorbisConfig {
    unsigned int bsz0:4;
    unsigned int bsz1:4;
    unsigned char channels;
    unsigned long version:32;
    unsigned long samplerate:32;
    unsigned long bitratemax:32;
    unsigned long bitratenom:32;
    unsigned long bitratemin:32;
} VorbisConfig;

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

unsigned int crc32 (int length, unsigned char *crcdata);
unsigned int crc32_init (void);
unsigned int crc32_hash (unsigned int state, int length, unsigned char *crcdata);
unsigned int crc32_fine (unsigned int state);

int createsocket (struct RTPHeaders *RTPHeaders, struct sockaddr_in *sockAddr, char *addr, unsigned int port, unsigned char TTL);
void creatertp (unsigned char* vorbdata, int length, int bitrate, struct VorbisBitfields *vorbheader, int type);
int sendrtp (struct RTPHeaders *RTPHeaders, int fd, struct sockaddr_in *sockAddr, const void *data, int len);
void configpacket (struct VorbisConfig *Config, int bsz0, int bsz1, vorbis_info vi);

/*****************************************************************************/
/*  Calculate CRC32                                                          */
/*****************************************************************************/

unsigned int crc32 (int length, unsigned char *crcdata) 
{
    unsigned int crc, state;
    
    state = crc32_init();
    state = crc32_hash(state, length, crcdata);
    crc = crc32_fine(state);
    
    return crc;
}

unsigned int crc32_init (void)
{
    unsigned int state = 0xFFFFFFFF;
    return state;
}

unsigned int crc32_hash (unsigned int state, int length, unsigned char *crcdata)
{
    unsigned int byte, mask;
    int index, loop;
    
    index = 0;
   
    while (index < length) {
        byte = crcdata [index];
        state = state ^ byte;

        for (loop = 7; loop >= 0; loop--) {
            mask = -(state & 1);
            state = (state >> 1) ^ (0xEDB88320 & mask);
        }
        index++;
    }
    
    return state;
}

unsigned int crc32_fine (unsigned int state)
{
    unsigned int crc = ~state;
    return crc;
}

/*****************************************************************************/
/*  Print progress marker                                                    */
/*****************************************************************************/

void progressmarker (int type)
{
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
}

/*****************************************************************************/
/*  Creates and send configuration packet                                    */
/*****************************************************************************/

void configpacket (struct VorbisConfig *Config, int bsz0, int bsz1, vorbis_info vi) 
{
    Config -> bsz0 = bsz0;
    Config -> bsz1 = bsz1;
    Config -> channels = htons (vi.channels);
    Config -> version = htonl (vi.version);
    Config -> samplerate = htonl (vi.rate);
    Config -> bitratemin = htonl (vi.bitrate_lower);
    Config -> bitratenom = htonl (vi.bitrate_nominal);
    Config -> bitratemax = htonl (vi.bitrate_upper);

    creatertp ((unsigned char*) Config, 22, vi.rate, &VorbisBitfields, 1);
    progressmarker (6);
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
    if (length < 5) return -1;

    ((unsigned int *)packet)[0] = htonl(vorbheader->cbident);
    packet[4] = (vorbheader -> continuation) << 7;
    packet[4] |= (vorbheader -> fragment) << 6;
    packet[4] |= (vorbheader -> reserved) << 5;
    packet[4] |= (vorbheader -> pkts) & 0x1f;          
    
    return 0;
}

/*****************************************************************************/
/*  Creates RTP packet from Vorbis frame.                                    */
/*  Deals with fragmentation and multiple Vorbis frame RTP packets           */
/*****************************************************************************/

void creatertp (unsigned char* vorbdata, int length, int bitrate, struct VorbisBitfields *vorbheader, int type)
{
    int sleeptime, frag, cont, position = 0;
    unsigned char framesize;
    unsigned char *packet;

    static int stacksize;
    static int stackcount;
    static unsigned char* framestack;

/*===========================================================================*/
/*  Test Codebook Ident (used for debug)                                     */
/*===========================================================================*/

/*    vorbheader -> cbident = htonl (0xc0deb00c); */

/*===========================================================================*/
/*  Set sleeptime value (todo: this should use the granulepos)                                                     */
/*===========================================================================*/

    sleeptime = ((1 / (float) bitrate) * 1000000);

/*===========================================================================*/
/*  Frame fragmentation                                                      */
/*===========================================================================*/

    if (length > 256) {
        cont = 0;
        frag = 1;
        while (length > 256) {        
            /*  Set Vorbis header flags  */
            vorbheader -> continuation = cont;
            vorbheader -> fragment = frag;
            vorbheader -> reserved = 0;
            vorbheader -> pkts = 0;

            packet = malloc (262);

            makevorbisheader (packet, 262, vorbheader);
            memcpy (packet + 5, &framesize, 1);
            memmove (packet + 6, vorbdata, 256);

            /*  Swap RTP headers from host to network order  */
            RTPHeaders.sequence = htons (RTPHeaders.sequence);

            sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, 262);

            /*  Swap headers back to host order  */
            RTPHeaders.sequence = ntohs (RTPHeaders.sequence);

            length -= 256;
            position += 256;
            cont = 1;
            frag = 0;

            RTPHeaders.sequence = RTPHeaders.sequence + 1;

            progressmarker (1);

            free (packet);
        }

        /*  Set Vorbis header flags  */
        vorbheader -> continuation = 1;
        vorbheader -> fragment = 1;
        vorbheader -> reserved = 0;
        vorbheader -> pkts = 0;

        framesize = (unsigned char) length;

        packet = malloc (length + 6);

        makevorbisheader (packet, length + 6, vorbheader);
        memcpy (packet + 5, &framesize, 1);
        memcpy (packet + 6, vorbdata + position, length);

        /*  Swap RTP headers from host to network order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = htonl (RTPHeaders.timestamp);

        sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, length + 6);

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

    if (length < 256 && type == 0) {
        if (length + stacksize < 256 && stackcount < 15) {

            framestack = realloc (framestack, (stacksize + (length + 1)));

            framestack [stacksize] = length;
            memcpy (framestack + (stacksize + 1), vorbdata, length);
            stackcount++;
            stacksize += (length + 1);
        }  

        if (length + stacksize > 256 || stackcount > 15) {

            /*  Set Vorbis header flags  */
            vorbheader -> continuation = 0;
            vorbheader -> fragment = 0;
            vorbheader -> reserved = 0;
            vorbheader -> pkts = stackcount;

            packet = malloc (stacksize + 6);

            makevorbisheader (packet, stacksize + 6, vorbheader);
            memmove (packet + 6, framestack, stacksize);

            /*  Swap RTP headers from host to network order  */
            RTPHeaders.sequence = htons (RTPHeaders.sequence);
            RTPHeaders.timestamp = htonl (RTPHeaders.timestamp);

            sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, stacksize + 6);

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
/*  Send header packets (under 256 octets) - No Packing                      */
/*===========================================================================*/

    else if (length < 256) {

        /*  Set Vorbis header flags  */
        vorbheader -> continuation = 0;
        vorbheader -> fragment = 0;
        vorbheader -> reserved = 0;
        vorbheader -> pkts = 1;

        framesize = (unsigned char) length;

        packet = malloc (length + 6);

        makevorbisheader (packet, length + 6, vorbheader);
        memcpy (packet + 5, &framesize, 1);
        memcpy (packet + 6, vorbdata, length);

        /*  Swap RTP headers from host to network order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = htonl (RTPHeaders.timestamp);

        sendrtp (&RTPHeaders, rtpsocket, &rtpsock, packet, length + 6);

        /*  Swap headers back to host order  */
        RTPHeaders.sequence = htons (RTPHeaders.sequence);
        RTPHeaders.timestamp = ntohl (RTPHeaders.timestamp);

        sleeptime = ((1 / (float) bitrate) * 1000000);
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

int main (int argc, char **argv) 
{
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
  
    vorbis_info vi; 

    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;

    unsigned int crc_state;
    
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

    fprintf (stderr, "||---------------------------------------------------------------------------||\n");
    fprintf (stderr, "||  Vorbis RTP Server (draft-kerr-avt-vorbis-rtp-04)\n");	  

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
    
    crc_state = crc32_init ();
    crc_state = crc32_hash (crc_state, op.bytes, op.packet);
    
    if (vorbis_synthesis_headerin (&vi, &vc, &op) < 0) { 
        fprintf (stderr, "||  This Ogg bitstream does not contain Vorbis audio data.\n");
        exit (1);
    }

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

                    if (i == 1) crc_state = crc32_hash (crc_state, op.bytes, op.packet);

                    vorbis_synthesis_headerin (&vi, &vc, &op);
                    i++;
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

    VorbisBitfields.cbident = crc32_fine (crc_state);
    
    convsize = 4096 / vi.channels;
    vorbis_synthesis_init (&vd, &vi);
    vorbis_block_init (&vd, &vb);

/*===========================================================================*/
/*  Codebook Ident                                                           */
/*===========================================================================*/

    VorbisBitfields.cbident = crc32 (sizeof (vd), (char*) &vd);

/*===========================================================================*/
/*  Print details                                                            */
/*===========================================================================*/

    fprintf (stdout, "||  Bitstream is %d channel, %ldHz\n", vi.channels, vi.rate);
    fprintf (stdout, "||  Encoded by: %s\n", vc.vendor);
    fprintf (stdout, "||  Bitrates: min=%ld - nom=%ld - max=%ld\n", vi.bitrate_lower, vi.bitrate_nominal, vi.bitrate_upper);
    fprintf (stdout, "||  Decode setup ident is 0x%08x\n", VorbisBitfields.cbident);
    fprintf (stdout, "||\n");
    fprintf (stdout, "||---------------------------------------------------------------------------||\n");
    fprintf (stdout, "||  Processing\n||  ");

/*===========================================================================*/
/*  Create configuration header                                              */
/*===========================================================================*/

    configpacket (&VorbisConfig, vorbis_info_blocksize (&vi, 0), vorbis_info_blocksize (&vi, 1), vi);

/*===========================================================================*/
/*  Create and send codebook                                                 */
/*===========================================================================*/

    creatertp ((char*) &vd, sizeof (vd), vi.rate, &VorbisBitfields, 2);
    progressmarker (4);

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
                        creatertp (op.packet, op.bytes, vi.rate, &VorbisBitfields, 0);
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
