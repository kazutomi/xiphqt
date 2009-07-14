/*
   Copyright (C) 2007 Xiph.org Foundation
   Copyright (C) 2009 Joern 'manx' Heusipp

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

/* ogg logical stream serial number mod script using the rogg library */

/* compile with
   gcc -O2 -g -Wall -I. -o rogg_changeserial rogg.c rogg_changeserial.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <rogg.h>

unsigned int old_serial = 0;
unsigned int new_serial = 0;

void print_usage(FILE *out, char *name)
{
  fprintf(stderr, "Script for editing ogg headers\n");
  fprintf(stderr, "%s [-s old:new] <file1.ogg> [<file2.ogg>...]\n",
	name);
  fprintf(stderr,
		  "    -s old:new  change the serial numer of a logical stream from old to new\n"
		  "                (use hex values, e.g. 0x89ab4567:0x0123cdef)\n"
		  "\n");
}

int parse_args(int *argc, char *argv[])
{
  int arg = 1;
  int shift;

  while (arg < *argc) {
    shift = 0;
    if (argv[arg][0] == '-') {
      switch (argv[arg][1]) {
	case 's':
	  /* read serial numbers from the next arg */
	  if (sscanf(argv[arg+1], "%x:%x", &old_serial, &new_serial) !=2 ) {
	    fprintf(stderr, "Could not parse serial numbers '%s'.\n", argv[arg+1]);
	    fprintf(stderr, "Try something like 0x89ab4567:0x0123cdef\n\n");
	    old_serial = 0;
	    new_serial = 0;
	  } else {
	    fprintf(stdout, "Changing serial 0x%08x to 0x%08x\n", old_serial, new_serial);
	  }
	  shift = 2;
	  break;
      }
    }
    if (shift) {
      int left = *argc - arg - shift;
      if (left < 0) {
	fprintf(stderr, "Interal error parsing argument '%s'.\n", argv[arg]);
	exit(1);
      }
      memmove(&argv[arg], &argv[arg+shift], left*sizeof(*argv));      
      *argc -= shift;
    } else {
      arg++;
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int f, i;
  unsigned char *p, *q, *o, *e;
  struct stat s;
  rogg_page_header header;
  uint32_t serial;

  parse_args(&argc, argv);
  if (argc < 2) {
    print_usage(stderr, argv[0]);
    exit(1);
  }

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
    if (p == MAP_FAILED) {
	fprintf(stderr, "couldn't mmap '%s'\n", argv[i]);
	close(f);
	continue;
    }
    fprintf(stdout, "Checking Ogg file '%s'\n", argv[i]);
    e = p + s.st_size; /* pointer to the end of the file */
    q = rogg_scan(p, s.st_size); /* scan for an Ogg page */
    if (q == NULL) {
	fprintf(stdout, "couldn't find ogg data!\n");
    } else {
      if (q > p) {
	fprintf(stdout, "Skipped %d garbage bytes at the start\n", (int)(q-p));
      }
      while (q < e) {
	o = rogg_scan(q, e-q); /* find the next Ogg page */
	if (o > q) {
	  fprintf(stdout, "Hole in data! skipped %d bytes\n", (int)(o-q));
	   q = o;
	} else if (o == NULL) {
	  fprintf(stdout, "Skipped %d garbage bytes as the end\n", (int)(e-q));
	  break;
	}
	rogg_page_parse(q, &header);
	rogg_read_uint32(&q[ROGG_OFFSET_SERIALNO], &serial);
	if (serial == old_serial) {
          rogg_write_uint32(&q[ROGG_OFFSET_SERIALNO], new_serial);
          rogg_page_update_crc(q);
	}
	q += header.length;
      }
    }
    munmap(p, s.st_size);
    close(f);
  }
  return 0;
}
