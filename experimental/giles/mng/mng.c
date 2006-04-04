/* Copyright (C) 2006 Xiph.org Foundation

   MNG encode and playback utility

   Distributed under the terms of the GPL v2 or later.
*/

/* $Id: $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#if 0
/* note: we require libmng version 1.0.8 or later for the Ogg MNG stuff */
#include <libmng.h>

/* SDL playback support is optional */
#ifdef HAVE_SDL
#include <SDL.h>
#endif
#endif

#include <ogg/ogg.h>

#define PNG_SIGNATURE "\211PNG\r\n\032\n"
#define MNG_SIGNATURE "\212MNG\r\n\032\n"
#define JNG_SIGNATURE "\213JNG\r\n\032\n"

#define BUFFER_SIZE 4096

/* for now, just convert mng to oggmng */

int read_uint32(FILE *in, uint32_t *value)
{
    unsigned char data[4];
    int bytes;

    bytes = fread(data, 1, 4, in);
    if (bytes != 4) return -1;
    
    *value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    return 0;    
}

int put_uint32(unsigned char *buffer, uint32_t value)
{
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >>  8) & 0xFF;
    buffer[3] = (value      ) & 0xFF;

    return 0;
}

int main(int argc, char *argv[])
{
    FILE *in, *out;
    char *infilename, *outfilename;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    unsigned char magic[8];
    unsigned char *buffer;
    int bytes, ret;
    uint32_t type, length, crc;
    long packetno;

    if (argc < 2) {
	in = stdin;
    } else {
	infilename = argv[1];
	in = fopen(infilename, "rb");
	if (in == NULL) {
	    fprintf(stderr, "unable to open input file '%s'\n", 
		infilename);
	    exit(2);
	}
    }

    out = stdout;

    srand(clock());
    ogg_stream_init(&os, rand());

    /* first packet is special */
    bytes = fread(magic, 1, 8, in);
    if (bytes != 8) {
	fprintf(stderr, "couldn't read file magic!\n");
	exit(3);
    }
      /* verify file magic */
    if (!memcmp(magic, MNG_SIGNATURE, 8)) {
	fprintf(stderr, "Encapsulating MNG file\n");
    } else if (!memcmp(magic, PNG_SIGNATURE, 8)) {
	fprintf(stderr, "Encapsulating PNG file\n");
    } else if (!memcmp(magic, JNG_SIGNATURE, 8)) {
	fprintf(stderr, "Encapsulating JNG file\n");
    } else {
	fprintf(stderr, "Unrecognized file type!\n");
    }

      /* get the MHDR */
    ret = read_uint32(in, &length);
    if (length != 28) {
	fprintf(stderr, "warning: MHDR is %d bytes instead of 28\n", 
		bytes - 4);
    }
    /* be slightly permissive on spec breakage */
    if (length > 10240) {
	fprintf(stderr, "error: MHDR unreasonably large, aborting.\n");
	return 4;
    }
    buffer = malloc(length + 8 + 4);
    memcpy(buffer, magic, 8);
    bytes = fread(buffer+8, 1, length + 4, in);
    if (bytes != length + 4) {
	fprintf(stderr, "i/o error: unexpected end of MHDR\n");
    }
    ret = read_uint32(in, &crc);
    /* todo; verify crc */
    fprintf(stderr, "chunk %c%c%c%c %d bytes\tcrc %08x\n",
	buffer[8], buffer[9], buffer[10], buffer[11],
	bytes - 4, crc);
    op.packet = buffer;
    op.bytes = 8 + bytes;
    op.b_o_s = 1;
    op.e_o_s = 0;
    op.granulepos = -1;
    op.packetno = packetno = 0;
    ogg_stream_packetin(&os, &op);
    free(buffer); /* libogg copies the data */
    /* we'll take care of writing this out in the main loop below */

    while (!feof(in)) {
	ret = read_uint32(in, &length);
	if (ret && feof(in)) break; /* no more data */
	buffer = malloc(length + 4);
	bytes = fread(buffer, 1, length+4, in);
	if (bytes != length+4) {
	    fprintf(stderr, "i/o error: unexpected end of data"
		", read %d of %d bytes.\n", bytes, length+4);
	    break;
	}
	ret = read_uint32(in, &crc);
	/* todo: verify crc */
	fprintf(stderr, "chunk %c%c%c%c %d bytes\tcrc %08x\n",
		buffer[0], buffer[1], buffer[2], buffer[3],
		bytes - 4, crc);
	op.packet = buffer;
	op.bytes = bytes;
	op.b_o_s = 0;
	op.e_o_s = feof(in); /* set eos flag on the last packet */
	op.granulepos = -1;
	op.packetno = packetno++;
	ogg_stream_packetin(&os, &op);
	free(buffer);

	/* write out any completed Ogg pages */
	while (ogg_stream_pageout(&os, &og) > 0) {
	    fwrite(og.header, 1, og.header_len, out);
	    fwrite(og.body, 1, og.body_len, out);
	}
    }

    /* flush any remaining pages */
    while (ogg_stream_flush(&os, &og) > 0) {
	fwrite(og.header, 1, og.header_len, out);
	fwrite(og.body, 1, og.body_len, out);
    }
   
    ogg_stream_clear(&os);

    if (in != stdin) fclose(in);

    return 0;
}
