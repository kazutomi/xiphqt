/*
	oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	mngread.c

	Copyright 2000 Ralph Giles <giles@ashlu.bc.ca>

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
#include "config.h"

/* file signatures */
#define PNG_SIGNATURE	"\211PNG\r\n\032\n"
#define MNG_SIGNATURE	"\212MNG\r\n\032\n"
#define JNG_SIGNATURE	"\213JNG\r\n\032\n"

/* chunk types in hex from libmng.h */
#define MNG_UINT_MHDR 0x4d484452L
#define MNG_UINT_MEND 0x4d454e44L

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

/* convert mng to ogg */
int mngconvert(param_t *param)
{
	char	sig[8];
	char	lenb[4],typeb[4],crcb[4];
	char	*chunkb;
	char	chunkname[5];
	int	length,type,crc;
	int	count = 0;
	int	err = 0;

	/* check the signature */
	err = fread(sig, sizeof(*sig), 8, param->in);
	sig[8] = '\0';
	if (strncmp((const char *)sig, MNG_SIGNATURE, 8) != 0) {
		fprintf(stderr, "not a mng file!");
		return ferror(param->in);
	} else {
		fprintf(stderr, "found mng signature\n");
	}

	ogg_stream_reset(param->os);
	ogg_sync_reset(param->oy);

	/* write out the mng signature as its own packet */
	{
		ogg_packet op;

		op.packet = sig;
		op.bytes = 8;
		op.b_o_s = 1;
		op.e_o_s = 0;
		op.granulepos = count++;

		ogg_stream_packetin(param->os, &op);
	}

	/* iterate over the chunks, converting each to an ogg_packet */
	fprintf(stderr, "processing chunks...\n");
	while(1) {
		/* read the chunk length */
		/* numbers in the file are in network byte order
		   so we read as a chars and unpack in an
		   endian-independent way. */
		fread(lenb, 1, 4, param->in);
		length = (lenb[0]&0xFF) << 24 | 
			(lenb[1]&0xFF) << 16 |
			(lenb[2]&0xFF) << 8 |
			(lenb[3]&0xFF);

		/* read the chunk type */
		fread(typeb, 1, 4, param->in);
		memcpy(chunkname, typeb, 4);
		chunkname[4] = '\0';
		fprintf(stderr, "  %s\t%5d bytes", chunkname, length);

		/* allocate a buffer for the data and read it in */
		chunkb = (char *)malloc((size_t)12 + length);
		if (chunkb == NULL) {
			fprintf(stderr,
				"aiigh! couldn't allocate buffer for chunk!\n");
  			return ENOMEM;
		}
		memcpy(chunkb, lenb, 4);
		memcpy(chunkb+4, typeb, 4);
		fread(chunkb+8, 1, length, param->in);

		/* read the crc */
		fread(crcb, 1, 4, param->in);
		crc = (crcb[0]&&0xFF) << 24 | 
                        (crcb[1]&0xFF) << 16 |
                        (crcb[2]&0xFF) << 8 |
                        (crcb[3]&0xFF);
		/* FIXME: should actually check the crc */
		fprintf(stderr, "\tcrc (%x)\n", crc);
		memcpy(chunkb+8+length, crcb, 4);

		/* we now have a complete copy of the chunk in chunkb */

		/* submit it to the ogg packet engine */
		{
			ogg_packet op;

			op.packet = chunkb;
			op.bytes = length+12;
			op.b_o_s = (strncmp((const char *)chunkname,
					"MHDR", 4)) ? 0 : 1;
			op.b_o_s = 0;
			op.e_o_s = (strncmp((const char *)chunkname,
					"MEND", 4)) ? 0 : 1;
			op.granulepos = count++;

			ogg_stream_packetin(param->os, &op);
		}

		/* write out any completed pages */
		{
			ogg_page og;

			while (ogg_stream_pageout(param->os, &og)) {
				fwrite(og.header, 1,
					og.header_len, param->out);
				fwrite(og.body, 1,
					og.body_len, param->out);
			}
		}

		/* free our storage */
		free (chunkb);

		/* are we done? */
		if (!strncmp((const char *)chunkname, "MEND", 4)) break;
	}
		
	fprintf(stderr, "done\n");
}
