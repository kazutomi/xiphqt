/*
   Copyright (C) 2005-2006 Xiph.org Foundation

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
   gcc -O2 -g -Wall -I. -o rogg_pagedump rogg.c rogg_pagedump.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <rogg.h>

void print_header_info(FILE *out, rogg_page_header *header)
{
  fprintf(out, " Ogg page serial %08x seq %d (%5d bytes)",
	header->serialno, header->sequenceno, header->length);
  fprintf(out, (header->continued) ? " c" : "  ");
  fprintf(out, " granule %lld", header->granulepos);
  fprintf(out, (header->bos) ? " bos" : "");
  fprintf(out, (header->eos) ? " eos" : "");
  fprintf(out, "\n");
}

int main(int argc, char *argv[])
{
  int f, i;
  unsigned char *p, *q, *o, *e;
  struct stat s;
  rogg_page_header header;

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
	fprintf(stderr, "couldn't mmap '%s'\n", argv[i]);
	close(f);
	continue;
    }
    fprintf(stdout, "Dumping Ogg file '%s'\n", argv[i]);
    e = p + s.st_size;
    q = rogg_scan(p, s.st_size);
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
	rogg_page_parse(q, &header);
	print_header_info(stdout, &header);
	q += header.length;
      }
    }
    munmap(p, s.st_size);
    close(f);
  }
  return 0;
}
