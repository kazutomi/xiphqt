/*
   Copyright (C) 2004 Xiph.org Foundation

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

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

/* quick program to filter an ogg file
   correcting (for example) header errors */

/* compile with:
   gcc -o oggfilter oggfilter.c `pkg-config --cflags oggz` \
     `pkg-config --libs oggz`

   you will need libogg 1.1(.x) from http://xiph.org/
   and liboggz (tested with 0.8.3) from http://annodex.net/
 */

#include <stdio.h>
#include <stdlib.h>
#include <oggz/oggz.h>

#define BUFSIZE 4096

/* write out a big-endian 32 bit integer */
static void write_uint32(unsigned char *p, unsigned int v)
{
  p[0] = (v >> 24) & 0xFF;
  p[1] = (v >> 16) & 0xFF;
  p[2] = (v >> 8) & 0xFF;
  p[3] = (v) & 0xFF;
}

/* write out a big-endian 32 bit integer */
static void write_uint24(unsigned char *p, unsigned int v)
{
  p[0] = (v >> 16) & 0xFF;
  p[1] = (v >> 8) & 0xFF;
  p[2] = (v) & 0xFF;
}

/* write out a big-endian 16 bit integer */
static void write_uint16(unsigned char *p, unsigned int v)
{
  p[0] = (v >> 8) & 0xFF;
  p[1] = (v) & 0xFF;
}

/* copy an ogg packet from reader to writer */
static int
packet_copy (OGGZ *oggz, ogg_packet *op, long serialno, void *user_data)
{
  OGGZ *writer = (OGGZ *)user_data;
  int flush;
  int ret;

  if (op->granulepos == -1) {
    flush = 0;
  } else {
    flush = OGGZ_FLUSH_AFTER;
  }

  if ((ret = oggz_write_feed(writer, op, serialno, flush, NULL)) != 0) {
    printf ("oggz_write_feed: %d\n", ret);
  }

  return 0;
}

/* do our special processing on a packet */
static int
packet_fix(OGGZ *oggz, ogg_packet *op, long serialno, void *user_data)
{
  int aspect_numerator = 1;
  int aspect_denominator = 1;

  /* fix the aspect ratio */
  write_uint24(op->packet+30, aspect_numerator);
  write_uint24(op->packet+33, aspect_denominator);

  /* pass the packet on and replace ourselves so we're 
     only called for the first packet in the stream */
  if (!packet_copy(oggz, op, serialno, user_data))
    oggz_set_read_callback (oggz, -1, packet_copy, user_data);

  return 0;
}

/* packet dispatch; called for new streams */
static int
packet_dispatch(OGGZ *oggz, ogg_packet *op, long serialno, void *user_data)
{
  OGGZ *writer = (OGGZ *)user_data;
  const unsigned char theora_magic[] = "\x80theora";
  const unsigned char vorbis_magic[] = "\x01vorbis";

  if (!memcmp(op->packet, theora_magic, 7)) {
    /* filter theora streams */
    packet_fix(oggz, op, serialno, user_data);
  } else {
    /* just copy anything else */
    if (!packet_copy(oggz, op, serialno, user_data))
      oggz_set_read_callback(oggz, serialno, packet_copy, user_data);
  }

  return 0;
}


int
main (int argc, char ** argv)
{
  OGGZ *reader, *writer;
  int n;

  if (argc < 3) {
    printf("usage: %s <in.ogg> <out.ogg>\n", argv[0]);
    printf("  filters in.ogg into out.ogg, doing\n");
    printf("  whatever it was compiled to do.\n");
  }

  reader = oggz_open(argv[1], OGGZ_READ);
  if (reader == NULL) {
    printf ("unable to open file %s\n", argv[1]);
    exit (1);
  }

  writer = oggz_open(argv[2], OGGZ_WRITE);
  if (writer == NULL) {
    printf ("unable to open file %s\n", argv[2]);
    oggz_close(reader);
    exit (1);
  }

  /* set the callback for stream dispatch */
  oggz_set_read_callback(reader, -1, packet_dispatch, writer);

  while ((n = oggz_read(reader, BUFSIZE)) > 0) {
    while (oggz_write(writer, n) > 0);
  }

  oggz_close(writer);
  oggz_close(reader);

  return 0;
}
