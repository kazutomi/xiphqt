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

static FILE *out;

static char * progname;

static int
process_page (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  ogg_int64_t gpos = ogg_page_granulepos((ogg_page *)og);

  if(gpos != -1 && ogg_page_packets((ogg_page *)og) == 0) {
    int i;

    fprintf(stderr, "Fixing granulepos on page with no completed packets\n");
    for(i=6;i<14;i++){
      gpos = -1;
      og->header[i]=(unsigned char)(gpos&0xff);
      gpos>>=8;
    }
    ogg_page_checksum_set((ogg_page *)og);
  }

  fwrite (og->header, 1, og->header_len, out);
  fwrite (og->body, 1, og->body_len, out);

  return 0;
}

int
main (int argc, char ** argv)
{
  OGGZ * oggz;
  long n;

  progname = argv[0];

  if (argc < 3) {
    printf ("usage: %s in.ogg out.ogg\n", progname);
  }

  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ | OGGZ_AUTO)) == NULL) {
    printf ("%s: unable to open file %s\n", progname, argv[1]);
    exit (1);
  }

  out = fopen(argv[2], "wb");
  if(!out) {
    fprintf(stderr, "%s: Failed to open output file \"%s\"\n", progname, argv[2]);
    exit(1);
  }

  oggz_set_read_page (oggz, -1, process_page, NULL);

  while ((n = oggz_read (oggz, 1024)) > 0);

  oggz_close (oggz);

  fclose(out);

  exit (0);
}
