/* Copyright (C) 2005 Xiph.org Foundation

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* sample RTP Vorbis client */

/* compile with: gcc -g -O2 -Wall -o vorbisrtp-client vorbisrtp-client.c -logg */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define MAX_PACKET 1500

#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

typedef struct ogg_context {
	ogg_packet op;
	ogg_stream_state os;
	ogg_page og;

	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
	int frag;
	float **pcm;
	unsigned int timestamp;
} ogg_context_t; 


int
dump_packet_raw (unsigned char *data, const int len, FILE * out)
{
  int i, j, n;

  i = 0;
  while (i < len)
    {
      fprintf (out, " %04x ", i);
      n = MIN (8, len - i);
      for (j = 0; j < n; j++)
	fprintf (out, " %02x", data[i + j]);
      fprintf (out, " ");
      n = MIN (16, len - i);
      for (j = 8; j < 16; j++)
	fprintf (out, " %02x", data[i + j]);
      fprintf (out, "   ");
      for (j = 0; j < n; j++)
	fprintf (out, "%c", isprint (data[i + j]) ? data[i + j] : '.');
      fprintf (out, "\n");
      i += 16;
    }

  return 0;
}

int
dump_packet_rtp (unsigned char *data, const int len, FILE * out)
{
  int V, P, X, CC, M, PT;
  unsigned short sequence;
  unsigned int timestamp, ssrc;
  unsigned int ident;
  int F, VDT, pkts;
  int i, offset, length;

  /* parse RTP header */
  V = (data[0] & 0xc0) >> 6;
  P = (data[0] & 0x40) >> 5;
  X = (data[0] & 0x20) >> 4;
  CC = (data[0] & 0x0f);
  M = (data[1] & 0x80) >> 7;
  PT = (data[1] & 0x7F);
  sequence = ntohs (((unsigned short *) data)[1]);
  timestamp = ntohl (((unsigned int *) data)[1]);
  ssrc = ntohl (((unsigned int *) data)[2]);

  fprintf (out, "RTP packet V:%d P:%d X:%d M:%d PT:%d", V, P, X, M, PT);
  fprintf (out, "   seq %d", sequence);
  fprintf (out, "   timestamp: %u\n", timestamp);
  fprintf (out, " ssrc: 0x%08x\n", ssrc);
  if (CC)
    for (i = 0; i < CC; i++)
      fprintf (out, " csrc: 0x%08x\n",
	       ntohl (((unsigned int *) data)[3 + i]));
  else
    fprintf (out, " no csrc\n");

  /* offset to payload header */
  offset = (3 + CC) * 4;

  /* parse Vorbis payload header */
  ident = data[offset++]<<16;
  ident += data[offset++]<<8;
  ident += data[offset++];
  F = (data[offset] & 0xc0) >> 6;
  VDT = (data[offset] & 0x30) >> 4;
  pkts = (data[offset] & 0x0F);
  offset++;
  
  fprintf(out,"ident %06x, frag type %d, data type %d, pkts %d, size %d\n",
		  ident,F,VDT,pkts,len-4*(CC+3));
 /* 
  fprintf (out, "   packets:");
  switch (F) {

  case 0:
    fprintf (out, " %d\n", pkts);
    break;
  case 1:
    fprintf (out, " frag start\n");
    break;
  case 2:
    fprintf (out, " frag cont\n");
    break;
  case 3:
    fprintf (out, " frag end\n");
    break;
  default:
    fprintf (out, " unknown!\n");
    break;
  }*/
  
  for (i = 0; i < pkts; i++)
    {
      if (offset >= len)
	{
	  fprintf (stderr, "payload length overflow. corrupt packet?\n");
	  return -1;
	}
      length = data[offset++]<<8;
      length += data[offset++];
      fprintf (out, "  data: %d bytes in block %d\n", length, i);
      offset += length;
    }
  if (pkts == 0)
    {
      length = data[offset++]<<8;
      length += data [offset++];
      fprintf (out, "  data: %d bytes in fragment\n", length);
      offset += length;
    }

  if (len - offset > 0)
    fprintf (out, "  %d unused bytes at the end of the packet!\n",
	     len - offset);

  return 0;
}

int
cfg_repack(ogg_context_t *ogg, FILE* out)
{
  ogg_packet id,co,cb;
  //FIXME bad hack
  char comment[] = 
//  {3,118,111,114,98,105,115,29,0,0,0,88,105,112,104,46,79,114,103,32,108,105,98,86,111,114,98,105,115,32,73,32,50,48,48,50,48,55,49,55,5,0,0,0,18,0,0,0,65,114,116,105,115,116,61,76,97,99,117,110,97,32,67,111,105,108,10,0,0,0,84,105,116,108,101,61,67,111,108,100,18,0,0,0,65,108,98,117,109,61,73,110,32,97,32,82,101,118,101,114,105,101,15,0,0,0,71,101,110,114,101,61,72,97,114,100,32,82,111,99,107,9,0,0,0,89,101,97,114,61,49,57,57,57,1};
  
  { 3,'v','o','r','b','i','s', 1,0,0,0, 0, 1,0,0,0, 0, 1};
  
/* get the identification packet*/
  id.packet = ogg->op.packet;
  id.bytes = 30;
  id.b_o_s = -1;
/* get the comment packet*/
  co.packet = comment;
  co.bytes = 135;
  co.granulepos = -1;
  co.packetno = -1;
/* get the setup packet*/
  cb.packet = ogg->op.packet + 30;
  cb.bytes = ogg->op.bytes - 30;
/* get the information required to decode blocksizes
 * from the info packet */
  ogg->vi.rate=id.packet[8+4+1]<<24;
  ogg->vi.rate+=id.packet[8+4+1+1]<<16;
  ogg->vi.rate+=id.packet[8+4+1+2]<<8;
  ogg->vi.rate=id.packet[8+4+1+3];
  fprintf(stderr,"parsed rate: %d\n",ogg->vi.rate);
/* start the ogg*/
  ogg_stream_init(&ogg->os,rand());

  ogg_stream_packetin(&ogg->os,&id);
  ogg_stream_packetin(&ogg->os,&co);
  ogg_stream_packetin(&ogg->os,&cb);
  ogg->op.b_o_s=1;
  do{
    int result=ogg_stream_flush(&ogg->os,&ogg->og);
    if(result==0)break;
    fwrite(ogg->og.header,1,ogg->og.header_len,out);
    fwrite(ogg->og.body,1,ogg->og.body_len,out);
    }while(!ogg_page_eos(&ogg->og));
  return 3;
}

int
pkt_repack(ogg_context_t *ogg, FILE *out){
  ogg_stream_packetin(&ogg->os,&ogg->op);
  do{
    int result=ogg_stream_pageout(&ogg->os,&ogg->og);
    if(result==0)break;
    fwrite(ogg->og.header,1,ogg->og.header_len,out);
    fwrite(ogg->og.body,1,ogg->og.body_len,out);
  }while(!ogg_page_eos(&ogg->og));
  return 1;
}

	
int
dump_packet_ogg (unsigned char *data, const int len, FILE * out, ogg_context_t *ogg)
{
  int V, P, X, CC, M, PT;
  unsigned short sequence;
  unsigned int timestamp, ssrc;
  unsigned int ident;
  int F, VDT, pkts;
  int i, offset, length, count = 0;
  ogg_packet *op = &ogg->op;

  /* parse RTP header */
  V = (data[0] & 0xc0) >> 6;
  P = (data[0] & 0x40) >> 5;
  X = (data[0] & 0x20) >> 4;
  CC = (data[0] & 0x0f);
  M = (data[1] & 0x80) >> 7;
  PT = (data[1] & 0x7F);
  sequence = ntohs (((unsigned short *) data)[1]);
  //FIXME not exactly ideal
  timestamp = ntohl (((unsigned int *) data)[1]) - ogg->timestamp;
  ogg->timestamp = ntohl (((unsigned int *) data)[1]);
  ssrc = ntohl (((unsigned int *) data)[2]);
  /* offset to payload header */
  offset = (3 + CC) * 4;

  /* parse Vorbis payload header */
  ident = data[offset++]<<16;
  ident += data[offset++]<<8;
  ident += data[offset++];
  F = (data[offset] & 0xc0) >> 6;
  VDT = (data[offset] & 0x30) >> 4;
  pkts = (data[offset] & 0x0F);
  offset++;
  
/*  printf("ident %06x, frag type %d, data type %d, pkts %d, size %d\n",
		  ident,F,VDT,pkts,len-4*(CC+3));
  
  fprintf (out, "   packets:");*/
  
  switch (F) {

  case 0:
    
    break;
  case 1:
    op->bytes = 0;
    
  case 2:
    length = data[offset++] << 8;
    length += data[offset++];
    op->packet = realloc (op->packet, length+op->bytes);
    memcpy (op->packet + op->bytes, data, length);
    op->bytes += length;
    return 0;
  case 3:
    length = data[offset++] << 8;
    length += data[offset++];
    op->packet = realloc (op->packet, length+op->bytes);
    memcpy (op->packet + op->bytes, data, length);
    op->bytes += length;
    pkts=1;
    break;
  default:
    fprintf (stderr, " unknown frament?!\n");
    break;
  }
  
  switch (VDT) {

  case 0:
  for (i = 0; i < pkts; i++)
    {
      if (offset >= len)
	{
	  fprintf (stderr, "payload length overflow. corrupt packet?\n");
	  return -1;
	}
      op->bytes = data[offset++]<<8;
      op->bytes += data[offset++];
      op->packet = &data[offset];
      op->granulepos=(pkts==1)?timestamp/1000000L*ogg->vi.rate:-1;//bad hack
      op->packetno++;
      count += pkt_repack(ogg,out);
      offset += op->bytes;
      op->b_o_s=0;
    }   
    break;
  case 1:
    count = cfg_repack(ogg, out);
    break;
  default:
    //ignore
    break;
    
  }
  if (F == 3) free(op->packet);

  return count;
}




int
main (int argc, char *argv[])
{
  int RTPSocket, ret;
  FILE *file;
  int optval = 0, decode = 0, dump = 0, opt;
  struct sockaddr_in us, them;
  struct ip_mreq group;
  unsigned char data[MAX_PACKET];
  char *hostname = "227.0.0.1", *filename = "out.ogg";
  unsigned int port = 4044;

  ogg_context_t ogg;
  memset(&ogg,0,sizeof(ogg_context_t)); 
  fprintf (stderr,
	   "||---------------------------------------------------------------------------||\n");

  fprintf (stderr, "||  Vorbis RTP Client (draft-ietf-avt-vorbis-rtp-01)\n");

  while ((opt = getopt (argc, argv, "i:p:f:")) != -1)
    {
      switch (opt)
	{

	  /*  Set IP address  */
	case 'i':
	  hostname = optarg;
	  break;

	  /*  Set port  */
	case 'p':
	  port = atoi (optarg);
	  break;

	  /*  Set TTL value  */
	case 'f':
	  filename = optarg;
	  dump = 1;
	  break;

	  /* Unknown option  */
	case '?':
	  fprintf (stderr, "\n||  Unknown option `-%c'.\n", optopt);
	  return 1;
	}
    }

  fprintf (stderr, "Opening connection to %s port %d\n", hostname, port);

  RTPSocket = socket (AF_INET, SOCK_DGRAM, 0);

  if (RTPSocket < 0)
    {
      fprintf (stderr, "Unable to create socket.\n");
      exit (1);
    }

  us.sin_family = AF_INET;
  us.sin_addr.s_addr = htonl (INADDR_ANY);
  us.sin_port = htons (port);
  ret = bind (RTPSocket, (struct sockaddr *) &us, sizeof (us));
  if (ret < 0)
    {
      fprintf (stderr, "Unable to bind socket!\n");
      exit (1);
    }

  them.sin_family = AF_INET;
  them.sin_addr.s_addr = inet_addr (hostname);
  them.sin_port = htons (port);

  if (!IN_MULTICAST (ntohl (them.sin_addr.s_addr)))
    {
      fprintf (stderr, "not a multicast address\n");
    }
  else
    {
      fprintf (stderr, "joining multicast group...\n");
      group.imr_multiaddr.s_addr = them.sin_addr.s_addr;
      group.imr_interface.s_addr = htonl (INADDR_ANY);
      ret = setsockopt (RTPSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			(void *) &group, sizeof (group));
      if (ret < 0)
	{
	  fprintf (stderr, "cannot join multicast group!\n");
	  exit (1);
	}
    }
  if (dump)
    {
      if (strcmp (filename, "-"))
	{
	  file = fopen (filename, "wb");
	  if (file == NULL)
	    {
	      fprintf (stderr, "Unable to open %s\n", filename);
	      exit (1);
	    }
	}
      else
      {
	      file = stdout;
	      filename = "Standard Output";
      }
      
      fprintf (stderr, "Dumping the stream to %s\n", filename);
    }

  while (1)
    {
      ret = recvfrom (RTPSocket, data, MAX_PACKET, 0, NULL, 0);
      fprintf (stderr, "read %d bytes of data\n", ret);
      
      dump_packet_rtp (data, ret, stderr);
      if (dump){
	dump_packet_ogg (data, ret, file, &ogg);
	fflush(file);
      }
    }

  return 0;
}
