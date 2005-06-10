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

/* simple script example for the rogg library */

/* compile with
   gcc -O2 -g -Wall -I. -o rogg_eosfix rogg.c rogg_eosfix.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <rogg.h>

typedef struct _streamref {
  uint32_t serialno;
  unsigned char *first;
  unsigned char *last;
  struct _streamref *next;
} streamref;

void print_header_info(FILE *out, rogg_page_header *header)
{
  fprintf(out, " Ogg page serial %08x seq %d (%5d bytes)",
	header->serialno, header->sequenceno, header->length);
  fprintf(out, (header->continued) ? " c" : "  ");
  fprintf(out, " %lld", header->granulepos);
  fprintf(out, (header->bos) ? " bos" : "");
  fprintf(out, (header->eos) ? " eos" : "");
  fprintf(out, "\n");
}

streamref *streamref_new(streamref *head, rogg_page_header *page)
{
  streamref *ref;

  ref = malloc(sizeof(*ref));
  if (ref != NULL) {
    ref->serialno = page->serialno;
    ref->first = page->capture;
    ref->last = NULL;
    ref->next = head;
  }
  
  return (ref != NULL) ? ref : head;
}

streamref *streamref_get(streamref *head, rogg_page_header *page)
{
  streamref *ref = head;

  while(ref != NULL) {
    if (ref->serialno == page->serialno) return ref;
  }

  return NULL;
}

streamref *streamref_update(streamref *head, rogg_page_header *page)
{
  streamref *ref;
  int newhead = 0;

  ref = streamref_get(head, page);
  if (ref == NULL) {
	ref = streamref_new(head, page);
	newhead = 1;
  }

  ref->last = page->capture;

  return newhead ? ref : head;
}

void streamref_free(streamref *head)
{
  streamref *next, *ref = head;

  while(ref != NULL) {
	next = ref->next;
	free(ref);
	ref = next;
  }
}

void streamref_seteos(streamref *head)
{
  streamref *ref = head;
  rogg_page_header header;

  while (ref != NULL) {
    rogg_parse_header(ref->last, &header);
    if (!header.eos) {
	fprintf(stderr, "setting missing eos on stream %08x\n",
	  header.serialno);
	ref->last[ROGG_OFFSET_FLAGS] |= 0x04;
	rogg_update_crc(ref->last);
    }
    ref = ref->next;
  }
}

int main(int argc, char *argv[])
{
  int f, i;
  unsigned char *p, *q, *o, *e;
  struct stat s;
  rogg_page_header header;
  streamref *refs;

  for (i = 1; i < argc; i++) {
    f = open(argv[i], O_RDWR);
    if (f < 0) {
	fprintf(stderr, "couldn't open '%s'\n", argv[i]);
	continue;
    }
    if (fstat(f, &s) < 0) {
	fprintf(stderr, "couldn't stat '%s'\n", argv[i]);
	close(f);
	continue;
    }
    p = mmap(0, s.st_size, PROT_READ|PROT_WRITE,
	MAP_SHARED, f, 0);
    if (p == NULL) {
	fprintf(stderr, "could mmap '%s'\n", argv[i]);
	close(f);
	continue;
    }
    fprintf(stdout, "Dumping Ogg file '%s'\n", argv[i]);
    e = p + s.st_size;
    q = rogg_scan(p, s.st_size);
    refs = NULL;
    if (q == NULL) {
	fprintf(stdout, "couldn't find ogg data!\n");
    } else {
      if (q > p) {
	fprintf(stdout, "Skipped %d garbage bytes at the start\n", q-p);
      } 
      while (q < e) {
	o = rogg_scan(q, e-q);
	if (o > q) {
	  fprintf(stdout, "Hole in data! skipped %d bytes\n", o - q);
	   q = o;
	} else if (o == NULL) {
	  fprintf(stdout, "Skipped %d garbage bytes as the end\n", e-q);
	  break;
	}
	rogg_parse_header(q, &header);
	print_header_info(stdout, &header);
#ifdef STRIP_EOS
	if (header.eos) {
	  /* unset any eos flags */
	  q[ROGG_OFFSET_FLAGS] &= ~0x04;
	  rogg_update_crc(q);
        }
#endif
	refs = streamref_update(refs, &header);
	q += header.length;
      }
    }
    streamref_seteos(refs);
    streamref_free(refs);
    munmap(p, s.st_size);
    close(f);
  }
  return 0;
}
