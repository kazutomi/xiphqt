/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia
   Also (C) 2005 Michael Smith <msmith@xiph.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <oggz/oggz.h>
#include <ogg/ogg.h>

static int
process_page (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  ogg_int64_t gpos = ogg_page_granulepos((ogg_page *)og);
  int granuleshift = oggz_get_granuleshift(oggz, serialno);

  fprintf (stdout, "|%010ld| ", serialno);
	fprintf(stdout, "page %ld, ", ogg_page_pageno((ogg_page *)og));
  if(granuleshift < 1) {
    fprintf(stdout, "GP %lld, ", gpos);
	}
	else {
	  ogg_int64_t iframe, pframe;
    iframe = gpos >> granuleshift;
    pframe = gpos - (iframe << granuleshift);

    fprintf (stdout, "granulepos %lld|%lld, ", iframe, pframe);
	}

	fprintf(stdout, "Flags: ");
	if(ogg_page_continued((ogg_page *)og))
		fprintf(stdout, "C");
	if(ogg_page_bos((ogg_page *)og))
		fprintf(stdout, "B");
	if(ogg_page_eos((ogg_page *)og))
		fprintf(stdout, "E");

	fprintf(stdout, " (%d)", ogg_page_packets((ogg_page *)og));
	
	fprintf(stdout, "\n");
	

  return 0;
}

int process_packet(OGGZ *oggz, ogg_packet *op, long serialno, void *user_data)
{
  int granuleshift = oggz_get_granuleshift (oggz, serialno);
  fprintf (stdout, "|%010ld|   ", serialno);
  if(granuleshift < 1) {
    fprintf(stdout, "GP %lld, ", op->granulepos);
	}
	else {
	  ogg_int64_t iframe, pframe;
    iframe = op->granulepos >> granuleshift;
    pframe = op->granulepos - (iframe << granuleshift);

    fprintf (stdout, "GP %lld|%lld, ", iframe, pframe);
	}
  fprintf (stdout, " packetno %lld", op->packetno);

  fprintf(stdout, ": %ld bytes\n", op->bytes);

	return 0;
}

int
main (int argc, char ** argv)
{
  OGGZ * oggz;
  long n;

  char *progname = argv[0];

  if (argc < 2) {
    printf ("usage: %s file.ogg\n", progname);
  }

  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ | OGGZ_AUTO)) == NULL) {
    printf ("%s: unable to open file %s\n", progname, argv[1]);
    exit (1);
  }

  oggz_set_read_page (oggz, -1, process_page, NULL);
	oggz_set_read_callback(oggz, -1, process_packet, NULL);

  while ((n = oggz_read (oggz, 1024)) > 0);

  oggz_close (oggz);

  exit (0);
}
