/*
   Copyright (C) 2005 Xiph.org Foundation

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* simple library for dealing with Ogg pages in memory */

#ifndef _ROGG_H_
#define _ROGG_H_

#include <stdint.h>

/* parsed header struct */
typedef struct _rogg_page_header rogg_page_header;
struct _rogg_page_header {
  /* literal header fields, translated for byte order */
  unsigned char *capture;	/* 4 bytes */
  unsigned char version;	/* 1 byte */
  unsigned char flags;		/* 1 byte */
  uint64_t granulepos;		/* 8 bytes */
  uint32_t serialno;		/* 4 bytes */
  uint32_t sequenceno;		/* 4 bytes */
  uint32_t crc;			/* 4 bytes */
  unsigned char segments;	/* 1 byte */
  unsigned char *lacing;	/* segments bytes */
  unsigned char *data;		/* start of page data */
  /* derived flags */
  int continued;
  int bos, eos;
  int length;
};

/* Ogg header entry offsets */
#define ROGG_OFFSET_CAPTURE 0
#define ROGG_OFFSET_VERSION 4
#define ROGG_OFFSET_FLAGS 5
#define ROGG_OFFSET_GRANULEPOS 6
#define ROGG_OFFSET_SERIALNO 14
#define ROGG_OFFSET_SEQUENCENO 18
#define ROGG_OFFSET_CRC 22
#define ROGG_OFFSET_SEGMENTS 26
#define ROGG_OFFSET_LACING 27

/* little endian i/o */
void rogg_write_uint64(unsigned char *p, uint64_t v);
void rogg_write_uint32(unsigned char *p, uint32_t v);
void rogg_write_uint16(unsigned char *p, uint16_t v);
void rogg_read_uint64(unsigned char *p, uint64_t *v);
void rogg_read_uint32(unsigned char *p, uint32_t *v);
void rogg_read_uint16(unsigned char *p, uint16_t *v);

/* scan for the 'OggS' capture pattern */
unsigned char *rogg_scan(unsigned char *p, int len);

/* calculate the length of the page starting at p */
void rogg_get_length(unsigned char *p, int *length);

/* parse out the header fields of the page starting at p */
void rogg_parse_header(unsigned char *p, rogg_page_header *header);

/* recompute and store a new crc on the page starting at p */
void rogg_update_crc(unsigned char *p);

#endif /* _ROGG_H */
