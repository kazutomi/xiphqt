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

#ifndef XIPH_RTP_H
#define XIPH_RTP_H

/**
 * Specific vorbis/theora header
 */

typedef struct header_bitfield {
    unsigned int cbident:24;
    unsigned int frag_type:2;
    unsigned int data_type:2;
    unsigned int pkts:4;
} header_bitfield_t;

/**
 * Standard rtp header
 */

typedef struct rtp_headers {
/*  v, p, x, cc flags  */
    unsigned int flags1:8;

/*  m, pt flags  */
    unsigned int flags2:8;

    unsigned int sequence:16;
    unsigned int timestamp:32;
    unsigned int ssrc:32;
} rtp_headers_t;


typedef struct framestack {
	int stacksize;
	int stackcount;
	unsigned char* framestack;
} framestack_t;


/**
 * Context structure for rtp transmission
 */

//FIXME restructure with a separate codec struct
typedef struct xiph_rtp {
	/* network related */
	struct sockaddr_in rtpsock;
	int socket;
	header_bitfield_t bitfield;
	rtp_headers_t headers;
	/* rtp related*/
	framestack_t fs;
	/* stream related */
	ogg_sync_state oy;
	ogg_stream_state os;
	ogg_page og;
	ogg_packet op;
	ogg_packet header[3];
	/* codec generic */
	int codec; // 0 = vorbis, 1 = theora, 2 =speex
#ifdef HAVE_VORBIS
	/* codec specific (vorbis)*/
	vorbis_info vi; 
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
#endif
#ifdef HAVE_THEORA
	/* codec specific (theora)*/
	theora_info      ti;
	theora_comment   tc;
	theora_state     td;
	unsigned int gp_shift;
#endif
	/* codec specific (speex) */
	//FIXME
	
} xiph_rtp_t;

int createsocket (xiph_rtp_t *xr, char *addr, unsigned int port,
		  unsigned char TTL);

void creatertp (xiph_rtp_t *xr, unsigned char* vorbdata, int length,
		long timestamp, long sleeptime, int type, int last);

int sendrtp (xiph_rtp_t *xr, const void *data, int len);

int makeheader (xiph_rtp_t *xr, unsigned char *packet, int length);

int ogg_copy_packet(ogg_packet *dst, ogg_packet *src);

#endif
