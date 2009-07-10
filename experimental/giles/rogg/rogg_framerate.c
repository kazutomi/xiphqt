/*
   Copyright (C) 2009 Xiph.org Foundation

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

/* theora aspect-ratio mod script using the rogg library */

/* compile with
   gcc -O2 -g -Wall -I. -o rogg_framerate rogg.c rogg_framerate.c
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

int verbose = 0;
int rate_set = 0;
int rate_num = 0;
int rate_den = 0;

/* big endian accessors for the theora header data */
int get16(unsigned char *data)
{
  return (data[0] << 8) | data[1];
}
int get24(unsigned char *data)
{
  return (data[0] << 16) | (data[1] << 8) | data[2];
}
int get32(unsigned char *data)
{
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}
void put16(unsigned char *data, int v)
{
  data[0] = (v >> 8) & 0xFF;
  data[1] = v & 0xFF;
}
void put24(unsigned char *data, int v)
{
  data[0] = (v >> 16) & 0xFF;
  data[1] = (v >>  8) & 0xFF;
  data[2] = v & 0xFF;
}
void put32(unsigned char *data, int v)
{
  data[0] = (v >> 24) & 0xFF;
  data[1] = (v >> 16) & 0xFF;
  data[2] = (v >>  8) & 0xFF;
  data[3] = v & 0xFF;
}

void print_header_info(FILE *out, rogg_page_header *header)
{
  fprintf(out, " Ogg page serial %08x seq %d (%5d bytes)",
	header->serialno, header->sequenceno, header->length);
  fprintf(out, (header->continued) ? " c" : "  ");
  fprintf(out, " %lld", (long long int)header->granulepos);
  fprintf(out, (header->bos) ? " bos" : "");
  fprintf(out, (header->eos) ? " eos" : "");
  fprintf(out, "\n");
}

void print_theora_info(FILE *out, unsigned char *data)
{
  fprintf(out, "  Theora info header version %d %d %d\n",
	data[7], data[8], data[9]);
  fprintf(out, "   encoded image %dx%d\n", 
	get16(data+10)<<4, get16(data+12)<<4);
  fprintf(out, "   display image %dx%d at (%d,%d)\n",
	get24(data+14), get24(data+17), data[20], data[21]);
  fprintf(out, "   frame rate %d/%d\n", get32(data+22), get32(data+26));
  fprintf(out, "   pixel aspect %d:%d\n", get24(data+30), get24(data+33));
  fprintf(out, "   colourspace %d\n", data[36]);
  fprintf(out, "   target bitrate %d\n", get24(data+37));
  fprintf(out, "   quality %d\n", data[40] >> 2);
}

void print_usage(FILE *out, char *name)
{
  fprintf(stderr, "Script for editing theora headers\n");
  fprintf(stderr, "%s [-v] [-f num:den] <file1.ogg> [<file2.ogg>...]\n",
	name);
  fprintf(stderr, "    -v          print more information\n"
		  "    -f num:den  set the frame rate\n"
		  "                common values: \n"
		  "                        25:1 PAL\n"
		  "                        30000:1001 NTSC\n"
		  "                        24:1 film\n");
}

int parse_args(int *argc, char *argv[])
{
  int arg = 1;
  int shift;

  while (arg < *argc) {
    shift = 0;
    if (argv[arg][0] == '-') {
      switch (argv[arg][1]) {
	case 'v':
	  verbose = 1;
	  shift = 1; 
	  break;
	case 'f':
	  rate_set = 1;
	  /* read frame rate from the next arg */
	  if (sscanf(argv[arg+1], "%d:%d", &rate_num, &rate_den) !=2 ) 
{
	    fprintf(stderr, "Could not parse aspect ratio '%s'.\n", argv[arg+1]);
	    fprintf(stderr, "Try something like 25:1 (PAL) or 30000:1001 (NTSC)\n\n");
	    rate_set = 0;
	  }
	  shift = 2;
	  break;
      }
    }
    if (shift) {
      memmove(&argv[arg],&argv[arg+shift],shift*sizeof(*argv));
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
	if (!header.bos) break; /* only look at the initial bos pages */
	if (verbose) {
	  print_header_info(stdout, &header);
	  int j;
	  for (j = 0; j < header.length; j++) {
	    fprintf(stdout, " %02x", header.data[j]);
	    if (!((j+1)%4)) fprintf(stdout, " ");
	    if (!((j+1)%16)) fprintf(stdout, "\n");
	  }
	  fprintf(stdout, "\n");
	}
	if (!memcmp(header.data, "\x80theora", 7)) {
	  print_theora_info(stdout, header.data);
	  if (rate_set) {
	    fprintf(stdout, "Setting frame rate to %d:%d\n",
		rate_num, rate_den);
	    put32(header.data+22, rate_num); /* numerator */
	    put32(header.data+26, rate_den); /* denominator */
	    rogg_page_update_crc(q);
	    fprintf(stdout, "New settings:\n");
	    print_theora_info(stdout, header.data); 
	  }
        }
	q += header.length;
      }
    }
    munmap(p, s.st_size);
    close(f);
  }
  return 0;
}
