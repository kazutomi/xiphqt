/*
	oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	mng.c
	oggmerge mng module

	Copyright 2001 Ralph Giles <Ralph_Giles@telus.net>

	Distributed under the GPL
	see the file COPYING for details
	or visit http://www.gnu.org/copyleft/gpl.html
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <ogg/ogg.h>

#include "oggmerge.h"
//#include "config.h"

/* file signatures */
#define PNG_SIGNATURE	"\211PNG\r\n\032\n"
#define MNG_SIGNATURE	"\212MNG\r\n\032\n"
#define JNG_SIGNATURE	"\213JNG\r\n\032\n"

/* chunk types in hex from libmng.h */
#define MNG_UINT_MHDR 0x4d484452L
#define MNG_UINT_MEND 0x4d454e44L

typedef struct {
	ogg_stream_state	*os;
	int			serialno;
	u_int64_t		packetno;
	unsigned char		*chunk;
} mng_state_t;

/* allocates and initializes local storage for a particular
 * substream conversion.
 *
 * returns 1 on success, 0 on failure
 */
int mng_state_init(oggmerge_state_t *state, int serialno)
{
	mng_state_t	*local;
	ogg_stream_state *os;
	ogg_packet	op;

	if (state == NULL) return 0;

        local = (mng_state_t *)malloc(sizeof(*local));

        if (local != NULL) {
		os = (ogg_stream_state*)malloc(sizeof(*os));
		ogg_stream_init(os, serialno);
		local->os = os;
		local->serialno = serialno;
		local->packetno = 0;	/* number of 'next' packet */
		local->chunk = NULL;

		/* save our local data inside the oggmerge state */
		state->private = local;

		return 1;	/* success */
	}

	return 0;

}

/* closes and deallocates any resources associated
 * with a substream conversion
 *
 * returns 0 on success
 */
int mng_state_close(oggmerge_state_t *state)
{
	mng_state_t     *local;

        if (state == NULL) return 0;	/* nothing to do */

        local = (mng_state_t *)state->private;
                                                      
        if (local != NULL) {                          
                ogg_stream_destroy(local->os);
		state->private = NULL;
		free(local);
	}

	return 0;	/* success */
}

/* accepts a buffer of input data (raw mng bytestream)
 * the buffer must be completely consumed before returning
 *
 * returns 0 on success, a negative error from oggmerge.h on failure
 */
int mng_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	mng_state_t	*mngstate;
	ogg_packet	op;

	if (state == NULL || buffer == NULL || size <= 0) return;

	mngstate = (mng_state_t*)state->private;
	
	/* are we looking for the signature? */
	if (mngstate->packetno == 0) {
		char	sig[9];

		if (size < 8) {
			fprintf(stderr, "oggmng: mng_data_in() must get at least 8 bytes on the first submission to check the signature.\n");
			exit(1);
		}
		memcpy(sig, buffer, 8);
		sig[8] = '\0';
		if (strncmp((const char *)sig, MNG_SIGNATURE, 8) != 0) {
                	return EBADHEADER;
		}
		
		fprintf(stderr, "oggmng: found mng signature\n");
		
		/* the first packet is always just the mng signature */
		op.packet = buffer;
		op.bytes = 8;
		op.b_o_s = 1;
		op.e_o_s = 0;
		op.packetno = 0;
		/* submit it */
		ogg_stream_packetin(mngstate->os, &op);
		mngstate->packetno++;
		buffer += 8;
		size -= 8;
	}

	/* are we continuing a previous chunk? */
	if (mngstate->chunk != NULL) {
		fprintf(stderr, "sorry, unimplemented section\n");
		return EBADEVENT;
	}

	/* data should be pointing to the beginning of a chunk */
	while (size > 0) {
		char    lenb[4],typeb[4],crcb[4];
		char    name[5];
        	int     length,type,crc;
		int	copybytes;

		/* read the get the length and type */
		length = (buffer[0]&0xFF) << 24 | 
                        (buffer[1]&0xFF) << 16 |
                        (buffer[2]&0xFF) << 8 |
                        (buffer[3]&0xFF);
		memcpy(name, buffer+4, 4);
		name[4] = '\0';

		/* should check crc here */

		fprintf(stderr, "oggmng: found '%s' chunk (%d bytes)\n", name, length);

		/* copy the chunk into an ogg packet */
		op.bytes = length + 12;
		op.packet = (unsigned char *)malloc(op.bytes);
		if (op.packet == NULL) return EMALLOC;
		op.b_o_s = 0;	/* should be the second packet in the header page */
		op.e_o_s = ( (unsigned int)*(buffer+4) == MNG_UINT_MEND) ? 1 : 0;
		copybytes = (length + 12 < size) ? length + 12 : size;
		memcpy(op.packet, buffer, copybytes);
		buffer += copybytes;
		if (size < length + 12) {
			/* save state here */
			return EMOREDATA;
		}
		size -= copybytes;
		/* submit */
		op.packetno = mngstate->packetno++;
		ogg_stream_packetin(mngstate->os, &op);
	}

	return 0;	/* success */
}

/* returns an completed oggmerge_page created from the data previously
 * passed through mng_data_in()
 *
 * returns NULL if no page is available
 */
oggmerge_page_t *mng_page_out(oggmerge_state_t *state)
{
	return NULL;
}

/* copy a mng chunk into an ogg bitstream packet */
ogg_packet *mngchunk_ogg(char *chunk)
{
	ogg_packet *packet = NULL;
	int32_t chunktype;
	int32_t length,crc;
	char chunkname[5];

	/* parse the chunk type code */
	chunkname[0] = (char)((chunktype >> 24) & 0xFF);
	chunkname[1] = (char)((chunktype >> 16) & 0xFF);
	chunkname[2] = (char)((chunktype >>  8) & 0xFF);
	chunkname[3] = (char)((chunktype      ) & 0xFF);
	chunkname[4] = '\0';

	fprintf(stderr, "  processing '%s' chunk\n", chunkname);

	packet = (ogg_packet*)malloc(sizeof(ogg_packet));
	packet->packet = chunk;

	packet->b_o_s = (chunktype == MNG_UINT_MHDR) ? 1 : 0;
	packet->e_o_s = (chunktype == MNG_UINT_MEND) ? 1 : 0;

	return packet;
}

