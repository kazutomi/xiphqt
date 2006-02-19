/*
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

  Author: Luca Barbato <lu_zero@gentoo.org>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <vorbis/codec.h>
#include <theora/theora.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "xiph_rtp.h"


void progressmarker (int type)
{
	static int outmarkpos=0;
	FILE *out=stderr;
	
	switch (type) {
		case 0:
			fprintf (out, "."); /* normal */
		break;
		case 1:
			fprintf (out, "<"); /* start fragment */
		break;

		case 2:
			fprintf (out, "+"); /* middle fragment */
		break;

		case 3:
			fprintf (out, ">"); /* end fragment */
		break;
#if 0
		case 4:
			fprintf (stdout, "b"); /* setup header */
		break;

		case 5:
			fprintf (stdout, "p"); /* packed */
		break;

		case 6:
			fprintf (stdout, "c"); /* info header */
		break;
	
#endif
		default:
		break;
	}

	fflush (NULL);
	
	if (outmarkpos == 70) {
		fprintf (out, "\n");
		outmarkpos = 0;
	} else 
		outmarkpos++;
}



int createsocket (xiph_rtp_t *xr, char *addr, unsigned int port, 
		  unsigned char TTL)
{
	int ret;
	int optval = 0;
	struct sockaddr_in sin;
	rtp_headers_t *headers= &xr->headers;

/*  Init RTP headers  */

/*  Sets v=2, p=0, x=0, cc=0  */
	headers -> flags1 = 0x80;

/*  Sets m=0, pt=96  */
	headers -> flags2 = 0x60;

	headers -> sequence = htonl (rand () & 65535);
	headers -> timestamp = htonl (rand ());
	headers -> ssrc = htonl (rand ());

/*  Create socket  */

	xr->socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (xr->socket < 0) {
		fprintf (stderr, "Socket creation failed.\n");
		return -1;
	}

	xr->rtpsock.sin_family = sin.sin_family = AF_INET;
	xr->rtpsock.sin_port = sin.sin_port = htons (port);
	xr->rtpsock.sin_addr.s_addr = inet_addr (addr);

	ret = setsockopt (xr->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int));

	if (ret < 0) {
		fprintf (stderr, "setsockopt SO_REUSEADDR error\n");
		return -1;
	}

/*  Set multicast parameters */

	if (IN_MULTICAST (ntohl (xr->rtpsock.sin_addr.s_addr))) {

		ret = setsockopt (xr->socket, IPPROTO_IP, IP_MULTICAST_TTL, &TTL, sizeof (char));

		if (ret < 0) {
			fprintf (stderr, "Multicast setsockopt TTL failed.\n");
			return -1;
		}

		optval = 1; 

		ret = setsockopt (xr->socket, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof (char));

		if (ret < 0) {
			fprintf (stderr, "Multicast setsockopt LOOP failed.\n");
			return -1;
		}
	} 

	return 0;
}

int sendrtp (xiph_rtp_t *xr, const void *data, int len)
{
	int ret;
	char *outbuffer;

#if 0
	fprintf(stderr,"sent %d bytes\n",len);
#endif
	outbuffer = malloc (sizeof (rtp_headers_t) + len);
	
	memcpy (outbuffer,  &xr->headers, sizeof (rtp_headers_t));
	memcpy (outbuffer + sizeof (rtp_headers_t), data, len);
	
	ret = sendto (xr->socket, outbuffer, sizeof (rtp_headers_t) + len,
			0, (struct sockaddr *) &xr->rtpsock,
			sizeof (struct sockaddr_in));
	
	free (outbuffer);
	return (ret);
}

//FIXME timestamp and sleeptime could be just one parameter
static void flush_stack (xiph_rtp_t *xr, long sleeptime, long timestamp)
{
	framestack_t *fs = &xr->fs;
	unsigned char *packet;
	
	if ( fs->stackcount )
	{ 
			/*  Set Vorbis header flags  */
		xr->bitfield.frag_type = 0;
		xr->bitfield.data_type = 0;
		xr->bitfield.pkts = fs->stackcount;

		packet = malloc (fs->stacksize + 4);

		makeheader (xr, packet, fs->stacksize + 4);
		memcpy (packet + 4, fs->framestack, fs->stacksize);

		/*  Swap RTP headers from host to network order  */
		xr->headers.sequence = htons (xr->headers.sequence);
		xr->headers.timestamp = htonl (xr->headers.timestamp);

		sendrtp (xr, packet, fs->stacksize + 4);

		/*  Swap headers back to host order  */
		xr->headers.sequence = htons (xr->headers.sequence);
		xr->headers.timestamp = ntohl (xr->headers.timestamp);

		if (fs->stackcount == 1) 
			progressmarker (0);
		else
			progressmarker (5);

		usleep (sleeptime);

		xr->headers.sequence++;
		xr->headers.timestamp += timestamp;

		fs->stacksize = 0;
		fs->stackcount = 0;

		free (packet);
	}
}

static void stack_packet(xiph_rtp_t *xr, unsigned char* vorbdata, int length)
{
	framestack_t *fs = &xr->fs;
	
	fs->framestack = 
		realloc (fs->framestack, (fs->stacksize + (length + 2)));
	fs->framestack[fs->stacksize++]= (length&0xff00)>>8;
	fs->framestack[fs->stacksize++]= length&0xff;
		
	memcpy (fs->framestack + (fs->stacksize), vorbdata, length);
	
	fs->stackcount++;
	fs->stacksize += length;

}

//FIXME max_payload should stay somewhere else
void creatertp (xiph_rtp_t *xr, unsigned char* vorbdata, int length,
		long timestamp, int type, int last)
{
	int frag, position = 0;
	unsigned short framesize;
	unsigned char *packet;
	long sleeptime = timestamp;
	framestack_t *fs = &xr->fs;
	const unsigned int max_payload = 1000;

/*  Set sleeptime value based on timestamp */

	if (type)
	{
		sleeptime = 300; //  ((1 / (float) bitrate) * 1000000);
		// flush any other packet in queue (chained ogg!)
		flush_stack(xr, timestamp, timestamp);
	}

/*  Frame packing.  Used only for type 0 packets (raw Vorbis data) */

	if ((length <= max_payload && type == 0 ) || fs->stacksize ) {
		
		if (length + fs->stacksize <= max_payload
				&& fs->stackcount < 15) 
		{
			stack_packet(xr, vorbdata, length);
		}

		else if (length + fs->stacksize > max_payload
				|| fs->stackcount >= 15)
		{
			flush_stack(xr, sleeptime, timestamp);
		
			if (length <= max_payload)
				stack_packet(xr,vorbdata,length);
		}
		if (last)
			flush_stack(xr, sleeptime, timestamp);

	} 

/*  Send header packets (under max_payload octets) - No Packing		*/

	else if (length < max_payload) {

		/*  Set Vorbis header flags  */
		xr->bitfield.frag_type = 0;
		xr->bitfield.data_type = type;
		xr->bitfield.pkts = 1;

		framesize = (unsigned char) length;

		packet = malloc (length + 4 + 2);

		makeheader (xr, packet, length + 4 + 2);

		packet[4]=(length&0xff00)>>8;
		packet[5]=length&0xff;

		memcpy (packet + 4 + 2, vorbdata, length);

		/*  Swap RTP headers from host to network order  */
		xr->headers.sequence = htons (xr->headers.sequence);
		xr->headers.timestamp = htonl (xr->headers.timestamp);

		sendrtp (xr, packet, length + 4 + 2);

		/*  Swap headers back to host order  */
		xr->headers.sequence = htons (xr->headers.sequence);
		xr->headers.timestamp = ntohl (xr->headers.timestamp);

	/* We need to sleep something like enough time to not overflow
	 * the playout buffer nor starve it */
		sleeptime = timestamp;
		usleep (sleeptime);

		xr->headers.sequence++;
		xr->headers.timestamp += timestamp;

		free (packet);
	}			
	
/*  Frame fragmentation */

	if (length > max_payload) {
		frag = 1;
		while (length > max_payload) {		
		/*  Set Vorbis header flags  */
			xr->bitfield.frag_type = frag;
			xr->bitfield.data_type = type;
			xr->bitfield.pkts = 0;

			framesize = max_payload;

			packet = malloc (framesize + 4 + 2);

			makeheader (xr, packet, framesize + 2 + 4);
		/* write 16-bit network order fragment length */
			packet[4]=(framesize&0xff00)>>8;
			packet[5]=framesize&0xff;
			memcpy (packet + 4 + 2, vorbdata + position, framesize);

			/*  Swap RTP headers from host to network order  */
			xr->headers.sequence = htons (xr->headers.sequence);

			sendrtp (xr, packet, framesize + 4 + 2);

			/*  Swap headers back to host order  */
			xr->headers.sequence = ntohs (xr->headers.sequence);

			length -= max_payload;
			position += max_payload;
			frag = 2;

			xr->headers.sequence = xr->headers.sequence + 1;

			progressmarker (1);

			free (packet);
		}

		/*  Set Vorbis header flags  */
		xr->bitfield.frag_type = 3;
		xr->bitfield.data_type = type;
		xr->bitfield.pkts = 0;

		framesize = length;

		packet = malloc (length + 4 + 2);

		makeheader (xr, packet, length + 4 + 2);
		packet[4]=(framesize&0xff00)>>8;
		packet[5]=framesize&0xff;
		memcpy (packet + 4 + 2, vorbdata + position, length);

		/*  Swap RTP headers from host to network order  */
		xr->headers.sequence = htons (xr->headers.sequence);
		xr->headers.timestamp = htonl (xr->headers.timestamp);

		sendrtp (xr, packet, length + 4 + 2);

		/*  Swap headers back to host order  */
		xr->headers.sequence = htons (xr->headers.sequence);
		xr->headers.timestamp = ntohl (xr->headers.timestamp);

		/*  Increment RTP headers  */
		xr->headers.sequence++;
		xr->headers.timestamp += timestamp;

		progressmarker (2);

		free (packet);
		return;
	}
}
	
int makeheader (xiph_rtp_t *xr, unsigned char *packet, int length)
{
	if (xr->codec == 3) return 0; //Speex has no header
	
	/* Vorbis and Theora header */
	if (length < sizeof(header_bitfield_t)) return -1;
	
	printf( "ident %06x, frag type %d,"
		" data type %d, pkts %02d, size %d\n", xr->bitfield.cbident,
		xr->bitfield.frag_type, xr->bitfield.data_type,
		xr->bitfield.pkts, length);

	packet[0] = (xr->bitfield.cbident&0xff0000)>>16;
	packet[1] = (xr->bitfield.cbident&0xff00)>>8;
	packet[2] = xr->bitfield.cbident&0xff;
	
	packet[3] = (xr->bitfield.frag_type) << 6;
	packet[3] |= (xr->bitfield.data_type) << 4;
	packet[3] |= (xr->bitfield.pkts) & 0xf;
	
	return 0;

}

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
  	printf("- bytes %ld bos %ld eos %ld gp %lld pno %lld\n",
  	dst->bytes, dst->b_o_s, dst->e_o_s, dst->granulepos, dst->packetno);
#endif

	return 0;
}

