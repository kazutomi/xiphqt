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

#ifndef _OggStream_
#define _OggStream_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus*/

#include <ogg/ogg.h>
#include "vorbis/codec.h"

// "reference time" in this file is defined as 100ns units


// These are the identifiers in the header package
#define MT_Video	"video"
#define MT_Audio	"audio"
#define MT_Text		"text"

// One second in reference time
#define SEC_IN_REFTIME 10000000
	
typedef struct stream_header_video
{
	ogg_int32_t	width;
	ogg_int32_t	height;
} stream_header_video;
	
typedef struct stream_header_audio
{
	ogg_int16_t	channels;
	ogg_int16_t	blockalign;
	ogg_int32_t	avgbytespersec;
} stream_header_audio;

// This structure is used as header packet
// The library use this structure similar to
// vorbis_info for vorbis
typedef struct stream_header
{
	char	streamtype[8];
	char	subtype[4];

	ogg_int32_t	size;				// size of the structure

	ogg_int64_t	time_unit;			// in reference time
	ogg_int64_t	samples_per_unit;
	ogg_int32_t default_len;		// in media time

	ogg_int32_t buffersize;
	ogg_int16_t	bits_per_sample;

	union
	{
		// Video specific
		stream_header_video	video;
		// Audio specific
		stream_header_audio	audio;
	};
} stream_header;

typedef struct stream_state
{
	bool				initial;
	ogg_int64_t			lastpno;
	ogg_int64_t			lastpos;
	ogg_int32_t			lastsize;

	// For creating streams ...
	unsigned char*		buffer;
	int					bufferlen;
	ogg_int64_t			packetno;
	ogg_stream_state	os;
} stream_state;


extern void			stream_header_init(stream_header* sh);
extern void			stream_header_clear(stream_header* sh);
extern int			stream_header_setup_video(stream_header** sh, char* FOURCC,
										ogg_int64_t avg_time_per_frame,
										ogg_int32_t	width, 	ogg_int32_t	height,
										ogg_int16_t	bit_count,
										ogg_int32_t buffersize);
extern int			stream_header_setup_audio(stream_header** sh, ogg_int16_t audioid,
										ogg_int16_t channels,
										ogg_int16_t	blockalign,
										ogg_int32_t	avgbytespersec,
										ogg_int32_t samplespersec,
										ogg_int16_t	bitspersample,
										unsigned char* extradata,
										ogg_int32_t extralen,
										ogg_int32_t buffersize);
extern int			stream_header_setup_text(stream_header** sh);

extern int			stream_header_in(stream_header** sh, ogg_packet* op);
extern int			stream_headerout_header(stream_state* ss, stream_header* sh);
extern int			stream_headerout_comment(stream_state* ss, vorbis_comment* vc);
extern int			stream_headerout_codebook(stream_state* ss, stream_header* sh);

extern void			stream_state_init(stream_state* ss, int serialno);
extern void			stream_state_clear(stream_state* ss);
extern void			stream_state_reset(stream_state* ss);


extern int			stream_samplein_getbuffer(stream_state* ss,
											  unsigned char** buffer, int len);
extern int			stream_samplein_vorbis(stream_state* ss, vorbis_info* vi,
										bool eos,
										ogg_int64_t* mediastart, ogg_int64_t* refstart,
										int samplesize);
extern int			stream_samplein(stream_state* ss, stream_header* sh,
										bool eos, bool sync,
										ogg_int64_t* mediastart, ogg_int64_t* medialen,
										ogg_int64_t* refstart,   ogg_int64_t* refstop,
										int samplesize);
extern int			stream_sampleout_vorbis(stream_state* ss, vorbis_info* vi,
										bool* eos,
										ogg_int64_t* mediastart, ogg_int64_t* refstart,
										unsigned char** buffer, int* bufferlen);
extern int			stream_sampleout(stream_state* ss, stream_header* sh,
									   bool* eos, bool* sync,
									   ogg_int64_t* mediastart, ogg_int64_t* medialen,
									   ogg_int64_t* refstart,   ogg_int64_t* refstop,
									   unsigned char** buffer, int* bufferlen);


extern ogg_int32_t	stream_packet_len(stream_header* sh, ogg_packet* op);


ogg_int64_t reference_time_to_mediatime(ogg_int64_t time_unit,	ogg_int64_t	samples_per_unit,
										ogg_int64_t reftime);
ogg_int64_t mediatime_to_reference_time(ogg_int64_t time_unit,	ogg_int64_t	samples_per_unit,
										ogg_int64_t mediatime);
										
#define PACKET_TYPE_HEADER		0x01
#define PACKET_TYPE_COMMENT		0x03
#define PACKET_TYPE_CODEBOOK	0x05
#define PACKET_TYPE_BITS		0x07

#define	PACKET_IS_SYNCPOINT		0x08
#define PACKET_LEN_BITS01		0xc0
#define PACKET_LEN_BITS2		0x02

#define E_INVPOINTER			-2048
#define E_NOTDATA				-2049
#define E_NOTHEADER				-2050
#define E_UNKNOWNHEADER			-2051
#define E_TIMEINVAILD			-2052

#define	E_SUCCESS				0

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

