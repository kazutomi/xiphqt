/*******************************************************************************
*                                                                              *
* Copyright (c) 2001, Tobias Waldvogel                                         *
* All rights reserved.                                                         *
*                                                                              *
* Redistribution and use in source and binary forms, with or without           *
* modification, are permitted provided that the following conditions are met:  *
*                                                                              *
*  - Redistributions of source code must retain the above copyright notice,    *
*    this list of conditions and the following disclaimer.                     *
*                                                                              *
*  - Redistributions in binary form must reproduce the above copyright notice, *
*    this list of conditions and the following disclaimer in the documentation *
*    and/or other materials provided with the distribution.                    *
*                                                                              *
*  - The names of the contributors may not be used to endorse or promote       *
*    products derived from this software without specific prior written        *
*    permission.                                                               *
*                                                                              *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"  *
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE    *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE   *
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE     *
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR          *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF         *
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS     *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN      *
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)      *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE   *
* POSSIBILITY OF SUCH DAMAGE.                                                  *
*                                                                              *
*******************************************************************************/

// This development is based on ogg vorbis.
// Therfore you need the ogg vorbis SDK to use
// this library. (Available on www.xiph.org)


/************************************************

Introduction:

The goal of this library is to embed other media
than vorbis into ogg streams.

Well, what do we need for the playback ?
- A sample (ptr to buffer and the length)
- The time when to render this sample

One of the biggest problems was the time code
handling. Unfortunately the author of Ogg has
decided to use media time as time stampes.
I.e. for audio the time unit is samples/sec,
for video frames/sec. I've chosen to use for
milliseconds for text streams. Anyway we need
a common timecode to get all streams in sync.
Thefore I'm using later on "reference time",
which is 100ns units.
  
On the other hand, as you might know, Ogg streams
are organized in pages and packets. To save
space an Ogg page has only one time stamp,
which is the time stamp of the last packet in
this page. For all the other packet we just get
"-1" as time stamp and we have to calculate the
next time base on the last known time stamp.

To get valid time stamps on every packet I'm
using a new structure which contains
ogg_stream_state and some data from the last
sample for the time code calculation

************************************************

Design of the packets:

I'm using three header packets like vorbis:

- 1. Packet (header)

  This packet contains information about the
  stream like type, time base a.s.o.
  
  0x0000 0x01 indicates "Header packet"
  0x0001 stream_info structure,
         the size is indicated in the
		 size member

- 2. Packet (comment)

  This is identical to the vorbis comment packet
  0x0000 0x03 indicated "Comment packet"
  0x0001 data .... (see vorbis doc)

- 3. Packet ("codebook")

  This is just to have the same number of header
  packets as vorbis. It's just one byte

  0x0000 0x05 indicates "Codebook packet"

Data packets

0x0000	Bit0	0 = Data packet 1
		Bit1	Bit 2 of len bytes
		Bit3	1 = This sample is a syncpoint
					("keyframe")
		Bit4	Currently not used
		Bit5	Currently not used
		Bit6&7	Bit 0 and 1 of len bytes

		len bytes is the number of following bytes
		with sample length in media time
		
		0 =	default len (from header packet)

 0x0001	Len bytes specified with bits  1 7 6,
		LowByte ... HighByte
 ..
 0x000x Data

*************************************************


How doest it work ?

I assume that you are familiar with the vorbis
playback examples when reading this documentation

Instead of creating an ogg_stream_state you
must create an stream_state structure for every
stream and initialize it with stream_state_init
Additionally you need for every stream a
vorbis_info, a vorbis_comment and a stream_info

Now you need an ogg_sync_state as in the vorbis
playback example and distribute the pages to
the corresponding stream (you get stream number
with ogg_page_serialno). First you must identify
the streams. The best way is passing the first
packet first to stream_header_in. If it likes
this stream it returns E_SUCCESS (= 0). If it
failes you should use the vorbis function
vorbis_header_in to ask vorbis if it is a vorbis
stream (returns also 0 to indicate success).

...
************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include "OggStream.h"

//
// Converts reference time in media time
//
ogg_int64_t reference_time_to_mediatime(ogg_int64_t time_unit,	ogg_int64_t	samples_per_unit,
									    ogg_int64_t reftime)
{
	ogg_int64_t mediatime;
	
	ogg_int64_t	x = reftime*  samples_per_unit;
	mediatime = x / time_unit;
	if ((x % time_unit) > (time_unit)>>1) mediatime++;
	return mediatime;
}

//
// Converts media time to reference time
//
ogg_int64_t mediatime_to_reference_time(ogg_int64_t time_unit,	ogg_int64_t	samples_per_unit,
										ogg_int64_t mediatime)
{
	ogg_int64_t reftime;

	ogg_int64_t	x = mediatime*  time_unit;
	reftime = x / samples_per_unit;
	if ((x % samples_per_unit) > (samples_per_unit)>>1) reftime++;
	return reftime;
}

//
// Initializes the stream_header struct
//
extern void	stream_header_init(stream_header* sh)
{
	memset(sh, 0, sizeof(stream_header));
	sh->size = sizeof(stream_header);
}

//
// Clears the stream_header struct
//
extern void	stream_header_clear(stream_header* sh)
{
	stream_header_init(sh);
}

//
// Initializes the stream_header struct for use with video
// FOURCC is the traditional four character code
// buffersize is the suggested playback buffer
//
extern int stream_header_setup_video(stream_header** sh, char* FOURCC,
										ogg_int64_t avg_time_per_frame,
										ogg_int32_t	width, 	ogg_int32_t	height,
										ogg_int16_t	bit_count,
										ogg_int32_t buffersize)
{
	*sh = (stream_header*) malloc(sizeof(stream_header));
	stream_header_init(*sh);

	strcpy((*sh)->streamtype, MT_Video);
	(*sh)->subtype[0]				= FOURCC[0];
	(*sh)->subtype[1]				= FOURCC[1];
	(*sh)->subtype[2]				= FOURCC[2];
	(*sh)->subtype[3]				= FOURCC[3];

	(*sh)->time_unit				= avg_time_per_frame;
	(*sh)->samples_per_unit			= 1;
	(*sh)->default_len				= 1; // we expect 1 sample per packet

	(*sh)->buffersize				= buffersize;
	(*sh)->bits_per_sample			= bit_count;
	(*sh)->video.width				= width;
	(*sh)->video.height				= height;

	return E_SUCCESS;
}

//
// Initializes the stream_header struct for use with audio
// other than vorbis (not recommended :-)
// AudioId is the the format ID from the Windows platform
// extradata and extralen is additional setup data
// required by the code. This depends on the codec
// (For windows programmers: this is the extra part of the
// WAVEFORMATEX strucure)
//
extern int stream_header_setup_audio(stream_header** sh, ogg_int16_t audioid,
										ogg_int16_t channels,
										ogg_int16_t	blockalign,
										ogg_int32_t	avgbytespersec,
										ogg_int32_t samplespersec,
										ogg_int16_t	bitspersample,
										unsigned char* extradata,
										ogg_int32_t extralen,
										ogg_int32_t buffersize)
{
	*sh = (stream_header*) malloc(sizeof(stream_header) + extralen);
	stream_header_init(*sh);
	(*sh)->size = sizeof(stream_header) + extralen;

	strcpy((*sh)->streamtype, MT_Audio);
	char	buffer[5];
	sprintf(buffer, "%04x", audioid);
	(*sh)->subtype[0]				= buffer[0];
	(*sh)->subtype[1]				= buffer[1];
	(*sh)->subtype[2]				= buffer[2];
	(*sh)->subtype[3]				= buffer[3];
	
	(*sh)->time_unit				= SEC_IN_REFTIME;
	(*sh)->samples_per_unit			= samplespersec;
	(*sh)->default_len				= 1;

	(*sh)->bits_per_sample			= bitspersample;
	(*sh)->audio.channels			= channels;
	(*sh)->audio.blockalign			= blockalign;
	(*sh)->audio.avgbytespersec		= avgbytespersec;
	(*sh)->buffersize				= buffersize;

	memcpy((*sh)+1, extradata, extralen);

	return E_SUCCESS;
}

// Initializes the stream_header struct for use with text streams
//
extern int stream_header_setup_text(stream_header** sh)
{
	*sh = (stream_header*) malloc(sizeof(**sh));
	stream_header_init(*sh);

	strcpy((*sh)->streamtype, MT_Text);

	(*sh)->time_unit				= 10000; // 1 ms
	(*sh)->samples_per_unit			= 1;
	(*sh)->default_len				= 1;

	(*sh)->buffersize				= 16384;
	return E_SUCCESS;
}

// Initializes the stream_state struct structure and
// the ogg_stream_state structure inside
//
extern void stream_state_init(stream_state* ss, int serialno)
{
	ss->buffer  = NULL;
	ss->initial = true;
	ss->packetno = 0;
	ogg_stream_init(&ss->os, serialno);
	return;
}

// Clears the stream_state struct structure and
// the ogg_stream_state structure inside
//
extern void stream_state_clear(stream_state* ss)
{
	if (ss->buffer) free(ss->buffer);
	ss->buffer = NULL;
	ss->initial = true;
	ss->packetno = 0;
	ogg_stream_clear(&ss->os);
}

// Resets the stream_state struct structure and
// the ogg_stream_state structure inside
//
extern void stream_state_reset(stream_state* ss)
{
	if (ss->buffer) free(ss->buffer);
	ss->buffer = NULL;
	ss->initial = true;
	ss->packetno = 0;
	ogg_stream_reset(&ss->os);
}

// Allocates buffer to add a sample to a stream
// We allocate 8 bytes more to add the header byte
// and the lenght later on
//
extern int stream_samplein_getbuffer(stream_state* ss, unsigned char** buffer, int len)
{
	if (ss->buffer && (ss->bufferlen < (len+8)))
	{
		free(ss->buffer);
		ss->buffer = NULL;
	}
	if (!ss->buffer)
	{
		ss->bufferlen = len+8;
		ss->buffer = (unsigned char*) malloc(ss->bufferlen);
	}
	*buffer = ss->buffer+8;
	
	return E_SUCCESS;
}

// Adds a vorbis sample to the stream with time code
// You may use media time or reference time
extern int stream_samplein_vorbis(stream_state* ss, vorbis_info* vi,
									bool eos,
									ogg_int64_t* mediastart, ogg_int64_t* refstart,
									int samplesize)
{
	ogg_packet	op;

	if (ss->buffer[8] & PACKET_TYPE_HEADER)
		op.granulepos = 0;
	else if (mediastart)
		op.granulepos = *mediastart;
	else if (refstart)
		op.granulepos = reference_time_to_mediatime(SEC_IN_REFTIME, vi->rate, *refstart);
	else
		op.granulepos = -1;

	op.b_o_s		= 0;
	op.e_o_s		= eos ? 1 : 0;
	op.packetno		= ss->packetno;
	op.packet		= ss->buffer+8;
	op.bytes		= samplesize;

	ogg_stream_packetin(&ss->os, &op);
	ss->packetno++;

	return E_SUCCESS;
}

// Adds a to the stream with time code
// You may use media time or reference time
extern int stream_samplein(stream_state* ss, stream_header* sh,
							bool eos, bool sync,
							ogg_int64_t* mediastart, ogg_int64_t* medialen,
							ogg_int64_t* refstart,   ogg_int64_t* refstop,
							int samplesize)
{
	int				result;
	ogg_packet		op;
	ogg_int32_t		samplelen;
	int				lenbytes = 0;
	unsigned char	flags = 0;

	if (mediastart)
		op.granulepos = *mediastart;
	else if (refstart)
		op.granulepos = reference_time_to_mediatime(sh->time_unit, sh->samples_per_unit, *refstart);
	else
		op.granulepos = -1;

	if (medialen)
		samplelen = (ogg_int32_t)*medialen;
	else if (refstop)
		samplelen = (ogg_int32_t)(reference_time_to_mediatime(sh->time_unit, sh->samples_per_unit,
												*refstop) - op.granulepos);
	else
		samplelen = 1;

	if (sync) flags |= PACKET_IS_SYNCPOINT;
	
	// To save space we define that 0 Length Bits means
	// a length of 1 in MediaTime (Typical for video frames);
	if (samplelen != sh->default_len)
	{
		int bytesleft = 4;

		while ((samplelen & 0xff000000) == 0 && (bytesleft > 1))
		{
			samplelen<<=8;
			bytesleft--;
		}

		while (bytesleft > 0)
		{
			ss->buffer[7-lenbytes] = (unsigned char)(samplelen>>24);
			lenbytes++;
			samplelen<<=8;
			bytesleft--;
		}		

		flags |= (lenbytes<<6) & PACKET_LEN_BITS01;
		flags |= (lenbytes>>1) & PACKET_LEN_BITS2;	// At the beginning I used only two bits
	}

	ss->buffer[7-lenbytes] = flags;

	op.b_o_s		= 0;
	op.e_o_s		= eos ? 1 : 0;
	op.packetno		= ss->packetno;
	op.packet		= ss->buffer + 7 - lenbytes;
	op.bytes		= samplesize + 1 + lenbytes;

	result = ogg_stream_packetin(&ss->os, &op);
	if (result > 0) ss->packetno++;

	return E_SUCCESS;
}

// Extracts a vorbis sample from the stream and reconstructs the time information
// You might not need this function because vorbis_sysnthesis deals with
// packets rather than samples but in the DS environment this is done in
// a seperate filter and the packet is passed along as sample
extern int stream_sampleout_vorbis(stream_state* ss, vorbis_info* vi,
									bool* eos,
									ogg_int64_t* mediastart, ogg_int64_t* refstart,
									unsigned char** buffer, int* bufferlen)
{
	if (!buffer || !bufferlen) return E_INVPOINTER;

	ogg_packet	op;
	
	int result = ogg_stream_packetout(&ss->os, &op);
	if (result <= 0) return result;							// Didn't got a packet

	if (*op.packet & PACKET_TYPE_HEADER)
	{
		ss->initial  = false;
		ss->lastpno  = op.packetno;
		ss->lastpos  = 0;
		ss->lastsize = 0;
		return E_NOTDATA;
	}
	if (eos)* eos = (op.e_o_s != 0);
	*buffer = op.packet; *bufferlen = op.bytes;

	ogg_int32_t	blocksize = vorbis_packet_blocksize(vi, &op)>>1;

	if (op.granulepos == -1 && op.packetno == ss->lastpno+1 && !ss->initial)
	{
		op.granulepos = ss->lastpos;
		if (ss->lastsize) op.granulepos += (ogg_int64_t)((ss->lastsize + blocksize)>>1);
	}

	if (op.granulepos != -1) ss->initial  = false;
	ss->lastpno  = op.packetno;
	ss->lastpos  = op.granulepos;
	ss->lastsize = blocksize;

	if (mediastart) *mediastart = op.granulepos;
	if (refstart)   *refstart = mediatime_to_reference_time(SEC_IN_REFTIME, vi->rate, op.granulepos);

	return (op.granulepos != -1) ? result : E_TIMEINVAILD;
}

// Extracts a sample from the stream and reconstructs the time information
extern int stream_sampleout(stream_state* ss, stream_header* sh,
								   bool* eos, bool* sync,
								   ogg_int64_t* mediastart, ogg_int64_t* medialen,
								   ogg_int64_t* refstart,   ogg_int64_t* refstop,
								   unsigned char** buffer, int* bufferlen)
{
	ogg_packet	op;
	
	int result = ogg_stream_packetout(&ss->os, &op);
	if (result <= 0) return result;							// Didn't got a packet

	if (*op.packet & PACKET_TYPE_HEADER)
	{
		ss->initial  = false;
		ss->lastpno  = op.packetno;
		ss->lastpos  = 0;
		ss->lastsize = 0;

		return E_NOTDATA;
	}

	ogg_int16_t	lenbytes;
	
	lenbytes  = (*op.packet & PACKET_LEN_BITS01)>>6;
	lenbytes |= (*op.packet & PACKET_LEN_BITS2) <<1;

	if (eos)		*eos = (op.e_o_s != 0);
	if (buffer)		*buffer = op.packet + 1 + lenbytes;
	if (bufferlen)	*bufferlen = op.bytes - 1 - lenbytes;

	ogg_int32_t	blocksize = stream_packet_len(sh, &op);

	if (op.granulepos == -1 && op.packetno == ss->lastpno+1 && !ss->initial)
		op.granulepos = ss->lastpos + (ogg_int64_t)ss->lastsize;

	if (op.granulepos != -1) ss->initial  = false;
	ss->lastpno  = op.packetno;
	ss->lastpos  = op.granulepos;
	ss->lastsize = blocksize;

	if (sync)		*sync = (op.granulepos != -1 && (*op.packet & PACKET_IS_SYNCPOINT));

	if (mediastart) *mediastart = op.granulepos;
	if (medialen)	*medialen = blocksize;
	if (refstart)   *refstart = mediatime_to_reference_time(sh->time_unit, sh->samples_per_unit,
															op.granulepos);
	if (refstop)	*refstop  = mediatime_to_reference_time(sh->time_unit, sh->samples_per_unit,
															op.granulepos + blocksize);
	return (op.granulepos != -1) ? result : E_TIMEINVAILD;
}

// Returns the length of a packet in media time (not for vorbis packets)
extern ogg_int32_t stream_packet_len(stream_header* sh, ogg_packet* op)
{
	if (op->packet[0] & PACKET_TYPE_HEADER) return E_NOTDATA;

	ogg_int16_t	lenbytes;
	
	lenbytes  = (*op->packet & PACKET_LEN_BITS01)>>6;
	lenbytes |= (*op->packet & PACKET_LEN_BITS2) <<1;

	if (lenbytes == 0) return sh->default_len;

	ogg_int32_t len = 0;
	while (lenbytes)
	{
		(len) <<= 8;
		(len) |= op->packet[lenbytes];
		lenbytes--;
	}
	return len;
}

// Add the header packet to the stream
extern int stream_headerout_header(stream_state* ss, stream_header* sh)
{
	ogg_packet		op;

	ss->packetno	= 0;

	op.b_o_s		= 1;
	op.e_o_s		= 0;
	op.granulepos	= 0;
	op.packetno		= ss->packetno;

	op.packet		= (unsigned char*)malloc(sh->size + 1);
	op.packet[0]	= PACKET_TYPE_HEADER;
	memcpy(op.packet+1, sh, sh->size);
	op.bytes		= sh->size + 1;
	ogg_stream_packetin(&ss->os, &op);
	free(op.packet);
	ss->packetno++;

	return E_SUCCESS;
}

// Adds the comment to the stream
extern int stream_headerout_comment(stream_state* ss, vorbis_comment* vc)
{
	ogg_packet		op;

	vorbis_commentheader_out(vc, &op);
	op.b_o_s		= 0;
	op.e_o_s		= 0;
	op.granulepos	= 0;
	op.packetno		= ss->packetno;
	ogg_stream_packetin(&ss->os, &op);
	ss->packetno++;

	return E_SUCCESS;
}


// This is for compatibility with the first header packet format
int stream_header_in_oldheader(stream_header** sh, ogg_packet* op)
{
	if ((*op->packet & PACKET_TYPE_BITS) != PACKET_TYPE_HEADER)
		return E_NOTHEADER;

	unsigned char*		p = op->packet;
	
	if (*(ogg_int32_t*)(p+96) == 0x05589f80)  // first part of FORMAT_VideoInfo
	{
		stream_header_setup_video(sh, (char*)(p+68),
								  *(ogg_int64_t*)(p+164),  // avg_time_per_frame,
  								  *(ogg_int32_t*)(p+176),  // width
								  *(ogg_int32_t*)(p+180),  // height
								  *(ogg_int16_t*)(p+182),  // bit_count
								  *(ogg_int32_t*)(p+40));  // buffersize
		return E_SUCCESS;
	}

	if (*(ogg_int32_t*)(p+96) == 0x05589F81)  // first part of FORMAT_WaveFormatEx
	{
		stream_header_setup_audio(sh,
								  *(ogg_int16_t*)(p+124),  // formattag
								  *(ogg_int16_t*)(p+126),  // channels
								  *(ogg_int16_t*)(p+136),  // blockalign
								  *(ogg_int32_t*)(p+132),  // avgbytespersec
								  *(ogg_int32_t*)(p+128),  // samplespersec
								  *(ogg_int16_t*)(p+138),  // bitspersample
								  p+142,				   // extradata
								  *(ogg_int16_t*)(p+140),  // extrasize
								  *(ogg_int32_t*)(p+40));  // buffersize
		return E_SUCCESS;
	}

	*sh = NULL;
	return E_UNKNOWNHEADER;
}

// Creates a stream_info structure from a header packet
extern int stream_header_in(stream_header** sh, ogg_packet* op)
{

	if ((*op->packet & PACKET_TYPE_HEADER) == 0)
		return E_NOTHEADER;

	if (strncmp((char*)op->packet+1, "Direct Show Samples embedded in Ogg", 35) == 0)
		return stream_header_in_oldheader(sh, op);

	if ((*op->packet & PACKET_TYPE_BITS) == PACKET_TYPE_HEADER)
	{
		if (strncmp((char*)op->packet+1, MT_Video, strlen(MT_Video)) == 0 ||
			strncmp((char*)op->packet+1, MT_Audio, strlen(MT_Audio)) == 0 ||
			strncmp((char*)op->packet+1, MT_Text,  strlen(MT_Text))  == 0)
		{
			stream_header* fsh = (stream_header*) (op->packet + 1);
			*sh = (stream_header*) malloc(fsh->size);

			memcpy(*sh, op->packet+1, fsh->size);
			return E_SUCCESS;
		}
	}

	if ((*op->packet & PACKET_TYPE_BITS) == PACKET_TYPE_CODEBOOK)
		return E_SUCCESS;

	*sh = NULL;
	return E_UNKNOWNHEADER;
}
