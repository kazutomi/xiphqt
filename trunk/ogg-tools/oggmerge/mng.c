/*
	oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	mng.c
	oggmerge mng module

	Copyright 2001-2003 Ralph Giles <giles@xiph.org>

	Distributed under the GPL
	see the file COPYING for details
	or visit http://www.gnu.org/copyleft/gpl.html
*/

#include <stdio.h>
#include <stdlib.h>
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

#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#define MAX(x,y) (((x)>(y)) ? (x) : (y))

/* chunk access functions */
ogg_uint32_t mngchunk_length(const unsigned char *chunk)
{
	int length;

	length = ( ((chunk)[0]&0xFF) << 24 |
                   ((chunk)[1]&0xFF) << 16 |
                   ((chunk)[2]&0xFF) << 8 |
                   ((chunk)[3]&0xFF) );

	return length;
}
ogg_uint32_t mngchunk_type(const unsigned char *chunk)
{
	ogg_uint32_t type;

	type = ( ((chunk)[4]&0xFF) << 24 |
                 ((chunk)[5]&0xFF) << 16 |
                 ((chunk)[6]&0xFF) << 8 |
                 ((chunk)[7]&0xFF) );

	return type;
}

typedef struct {
	ogg_stream_state	*os;
	int			serialno;
	u_int64_t		packetno;
	unsigned char		*chunk;
	unsigned long		length;
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
		local->length = 0;
		
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

/* read out a chunk type from a buffer into a string */
static int mngchunk_type2string(char *name, ogg_uint32_t type)
{
	/* parse the chunk type code */
	name[0] = (char)((type >> 24) & 0xFF);
	name[1] = (char)((type >> 16) & 0xFF);
	name[2] = (char)((type >>  8) & 0xFF);
	name[3] = (char)((type      ) & 0xFF);
	name[4] = '\0';
	
	return 0;
}

/* copy a mng chunk into an ogg bitstream packet */
int mngchunk_packet(ogg_packet *op, unsigned char *chunk)
{
	ogg_int32_t length,type,crc;
	char name[5];

	length = mngchunk_length(chunk);
	type = mngchunk_type(chunk);
	mngchunk_type2string(name, type);
	fprintf(stderr, "  processing '%s' chunk (%d bytes)\n",
		name, length);

	op->packet = chunk;
	op->bytes = 12 + length;
	op->b_o_s = (type == MNG_UINT_MHDR) ? 1 : 0;
	op->e_o_s = (type == MNG_UINT_MEND) ? 1 : 0;

	return 0;
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

	if (state == NULL || buffer == NULL) return;
	mngstate = (mng_state_t*)state->private;
	
	/* are we looking for the header? */
	if (mngstate->packetno == 0) {
		int length, type;
		
		/* check the signature */
		if (size < 8) {
			fprintf(stderr, "oggmng: mng_data_in() must get at least 8 bytes on the first submission to check the signature.\n");
			exit(1);
		}
		if (memcmp(buffer, MNG_SIGNATURE, 8)) {
			fprintf(stderr, "oggmng: file signature didn't match!\n");
                	return EBADHEADER;
		}
		fprintf(stderr, "oggmng: found mng signature\n");
		
		/* check the MHDR -- TODO: should buffer instead */
		if (size < 8 + 12 + 28) {
			fprintf(stderr, "oggmng: mng_data_in() must get at least the complete MHDR chunk in the first submission.\n");
			exit(1);
		}
		length = mngchunk_length(buffer + 8);
		type = mngchunk_type(buffer + 8);
		if (type != MNG_UINT_MHDR) {
			return EBADHEADER;
		}
		if (length != 28) {
			fprintf(stderr, "oggmng: MHDR length doesn't match spec!\n");
			return EBADHEADER;
		}
		fprintf(stderr, "oggmng: found MHDR\n");
		
		/* the first packet is always the mng signature and MHDR chunk */
		op.packet = buffer;
		op.bytes = 8 + 12 + length;
		op.b_o_s = 1;
		op.e_o_s = 0;
		op.packetno = 0;
		ogg_stream_packetin(mngstate->os, &op);
		mngstate->packetno++;
		buffer += 8 + 12 + length;
		size -= 8 + 12 + length;
	}

	/* are we continuing a previous chunk? */
	if (mngstate->chunk != NULL) {
		unsigned char *chunk = mngstate->chunk;
		int length = mngstate->length;
		int needed = mngchunk_length(chunk) + 12 - length;
		int copybytes = MIN(needed,size);
		fprintf(stderr, "adding %d bytes to chunk buffer\n", copybytes);
		fprintf(stderr, "  buffer offset %d and %d bytes needed\n",
			length, needed);
		memcpy(chunk + length, buffer, copybytes);
		if (copybytes == needed) {
			mngchunk_packet(&op, chunk);
			op.packetno = mngstate->packetno++;
			ogg_stream_packetin(mngstate->os, &op);
			mngstate->chunk = NULL; /* ogg takes ownership */
			mngstate->length = 0;
		}
		buffer += copybytes;
		size -= copybytes;
	}

	/* data should be pointing to the beginning of a chunk */
	while (size > 0) {
		char    name[5];
        	int     length,type,crc;
		int	copybytes;

		if (size < 8) {	/* partial chunk header */
			fprintf(stderr, "sorry, unimplemented section\n");
			return EBADEVENT;
		}
		
		/* get the length and type */
		length = mngchunk_length(buffer);
		type = mngchunk_type(buffer);
		mngchunk_type2string(name, type);
		/* allocate storage for the next chunk */
		fprintf(stderr, "allocating storage for '%s' chunk (%d bytes)\n",
			name, length + 12);
		mngstate->chunk = malloc(length + 12);
		if (mngstate->chunk == NULL) {
			fprintf(stderr, "oggmng: failed to allocate chunk storage!\n");
			return EBADEVENT;
		}
		copybytes = MIN(size, length + 12);
		memcpy(mngstate->chunk, buffer, copybytes);
		mngstate->length = copybytes;
		size -= copybytes;
		buffer += copybytes;
		fprintf(stderr, "copied %d bytes to the chunk buffer\n", copybytes);

		if (copybytes == length + 12) {
			/* should check crc here */
			/* submit */
			fprintf(stderr, "submitting chunk for packetization (packet %d)\n", mngstate->packetno);
			mngchunk_packet(&op, mngstate->chunk);
			op.packetno = mngstate->packetno++;
			ogg_stream_packetin(mngstate->os, &op);
			mngstate->length = 0;
			free(mngstate->chunk);
			mngstate->chunk = NULL;
		} else {
			return EMOREDATA;
		}

	}

	return 0;	/* success */
}

static ogg_page *_copy_ogg_page(ogg_page *og)
{
        ogg_page *page;
 
        page = (ogg_page *)malloc(sizeof(ogg_page));
        if (page == NULL) return NULL;
 
        page->header_len = og->header_len;
        page->body_len = og->body_len;
        page->header = (unsigned char *)malloc(page->header_len);
        if (page->header == NULL) {
                free(page);
                return NULL;
        }
        memcpy(page->header, og->header, page->header_len);
        page->body = (unsigned char *)malloc(page->body_len);
        if (page->body == NULL) {
                free(page->header);
                free(page);
                return NULL;
        }
        memcpy(page->body, og->body, page->body_len);

        return page;
}

/* returns an completed oggmerge_page created from the data previously
 * passed through mng_data_in()
 *
 * returns NULL if no page is available
 */
oggmerge_page_t *mng_page_out(oggmerge_state_t *state)
{
	mng_state_t *mngstate;
	ogg_page og;
	oggmerge_page_t *om;

	if (state == NULL) return;
	mngstate = (mng_state_t*)state->private;

	om = malloc(sizeof(oggmerge_page_t));
	om->og = malloc(sizeof(ogg_page));
	if(ogg_stream_pageout(mngstate->os, &og)==0) {
		free(om);
		om = NULL;
	} else {
		om->og = _copy_ogg_page(&og);
		om->timestamp = mngstate->packetno;
		fprintf(stderr, "oggmng: returning page with timestamp %ld\n",om->timestamp);
	}
	
	return om;
}
