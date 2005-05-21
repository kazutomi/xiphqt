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

/* sample RTP Theora client */

/* compile with: gcc -g -O2 -Wall -o theorartp-client theorartp-client.c */


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

#define MAX_PACKET 1500

#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

int dump_packet_raw(unsigned char *data, const int len, FILE *out)
{
  int i, j, n;

  i = 0;
  while (i < len) {
    fprintf(out, " %04x ", i);
    n = MIN(8, len - i);
    for (j = 0; j < n; j++)
      fprintf(out, " %02x", data[i+j]);
    fprintf(out, " ");
    n = MIN(16, len - i);
    for (j = 8; j < 16; j++)
      fprintf(out, " %02x", data[i+j]);
    fprintf(out, "   ");
    for (j = 0; j < n; j++)
      fprintf(out, "%c", isprint(data[i+j]) ? data[i+j] : '.');
    fprintf(out, "\n");
    i += 16;
  }

  return 0;
}

typedef struct _rtp_header {
  int V,P,X,CC,M,PT;
  unsigned short sequence;
  unsigned int timestamp, ssrc;
  unsigned int ident;
  unsigned int csrc[16];
  int offset;
  const unsigned char *data, *payload;
  int len, plen;
} rtp_header;

int parse_packet_rtp_header(const unsigned char *data,
				 const int len, rtp_header *rtp)
{
  int i;

  /* check for bad arguments */
  if ((data == NULL) || (rtp == NULL)|| (len < 12))
    return -1;

  /* parse RTP header */
  rtp->V = (data[0] & 0xc0) >> 6; /* version */
  rtp->P = (data[0] & 0x40) >> 5;
  rtp->X = (data[0] & 0x20) >> 4;
  rtp->CC = (data[0] & 0x0f);	  /* CSRC count */
  rtp->M = (data[1] & 0x80) >> 7;
  rtp->PT = (data[1] & 0x7F);     /* packet type */
  rtp->sequence = ntohs(((unsigned short *)data)[1]);
  rtp->timestamp = ntohl(((unsigned int *)data)[1]);
  rtp->ssrc = ntohl(((unsigned int *)data)[2]);
  rtp->offset = (3 + rtp->CC) * 4; /* offset to payload header */

  /* check space for the CSRC fields */
  if (rtp->offset >= len) return -2;

  /* read the CSRC list, and null empty entries */
  for (i = 0; i < rtp->CC; i++) 
    rtp->csrc[i] = ntohl(((unsigned int *)data)[3+i]);
  for (i = rtp->CC; i < 16; i++)
    rtp->csrc[i] = 0;
  
  /* index into the packet */
  rtp->data = data;
  rtp->len = len;
  rtp->payload = data + rtp->offset;
  rtp->plen = len - rtp->offset;

  return 0; /* success */
}

int dump_packet_rtp_header(rtp_header *rtp, FILE *out)
{
  int i;
  
  fprintf(out, "RTP packet V:%d P:%d X:%d M:%d PT:%d",
    rtp->V, rtp->P, rtp->X, rtp->M, rtp->PT);
  fprintf(out, "   seq %d", rtp->sequence);
  fprintf(out, "   timestamp: %u\n", rtp->timestamp);
  fprintf(out, " ssrc: 0x%08x\n", rtp->ssrc);
  if (rtp->CC) 
    for (i = 0; i < rtp->CC; i++)
      fprintf(out, " csrc: 0x%08x\n", rtp->csrc[i]);
  else
    fprintf(out, " no csrc\n");

  return 0;
}

int dump_packet_rtp_vorbis(rtp_header *rtp, FILE *out)
{
  const unsigned char *payload = rtp->payload;
  unsigned int ident;
  int C,F,R,pkts;
  int offset, length;
  int i;
  
  /* parse Vorbis payload header */
  offset = rtp->offset;
  ident = ntohl(*(unsigned int *)payload);
  offset += 4;
  C = (payload[offset] & 0x80) >> 7;
  F = (payload[offset] & 0x40) >> 6;
  R = (payload[offset] & 0x20) >> 5;
  pkts = (payload[offset] & 0x1F);
  offset++;

  fprintf(out, " Vorbis payload ident 0x%08x  C:%d F:%d R:%d",
    ident, C, F, R);
  fprintf(out, "   packets:");
  if (C == 0 && F == 0)
    fprintf(out, " %d\n", pkts);
  else if (C == 0 && F == 1)
    fprintf(out, " frag start\n");
  else if (C == 1 && F == 0) 
    fprintf(out, " frag cont.\n");
  else /* C == 1 && F == 1 */
    fprintf(out, " frag end\n");

  for (i = 0; i < pkts; i++) {
    if (offset >= rtp->plen) {
      fprintf(stderr, "payload length overflow. corrupt packet?\n");
      return -1;
    }
    length = payload[offset++];
    fprintf(out, "  data: %d bytes in block %d\n", length, i);
    offset += length;
  }
  if (pkts == 0) {
    length = payload[offset++];
    fprintf(out, "  data: %d bytes in fragment\n", length);
    offset += length;
  }

  if (rtp->plen - offset > 0)
    fprintf(out, "  %d unused bytes at the end of the packet!\n", 
      rtp->plen - offset);

  return 0;
}

int dump_packet_rtp_theora(rtp_header *rtp, FILE *out)
{
  const unsigned char *payload = rtp->payload;
  unsigned int ident;
  unsigned char reserved;
  int C,F,R,pkts;
  int offset, length;
  int i;

  /* parse 4 byte Theora payload header */
  /* || setup ident || reserved || C | F | R | #pkts || */
  /*   C F flag interpretation 
   *   0 0  Unfragmented packet (aka multi-packet..packet)
   *   0 1  First fragmented packet
   *   1 0  Middle fragment
   *   1 1  Last fragment.
   */
  offset = rtp->offset;
  ident = ntohs(*(unsigned short *)payload);
  reserved = payload[2];
  C = (payload[3] & 0x80) >> 7;
  F = (payload[3] & 0x40) >> 6;
  R = (payload[3] & 0x20) >> 5;
  pkts = (payload[3] & 0x1F);
  offset += 4;


  fprintf(out, " Theora payload ident 0x%08x  C:%d F:%d R:%d",
    ident, C, F, R);
  fprintf(out, "   packets:");
  if (C == 0 && F == 0)
    fprintf(out, " %d\n", pkts);
  else if (C == 0 && F == 1)
    fprintf(out, " frag start\n");
  else if (C == 1 && F == 0) 
    fprintf(out, " frag cont.\n");
  else /* C == 1 && F == 1 */
    fprintf(out, " frag end\n");

  for (i = 0; i < pkts; i++) {
    if (offset >= rtp->plen) {
      fprintf(stderr, "payload length overflow. corrupt packet?\n");
      return -1;
    }
    length = payload[offset++];
    fprintf(out, "  data: %d bytes in block %d\n", length, i);
    offset += length;
  }
  if (pkts == 0) {
    length = payload[offset++];
    fprintf(out, "  data: %d bytes in fragment\n", length);
    offset += length;
  }

  if (rtp->plen - offset > 0)
    fprintf(out, "  %d unused bytes at the end of the packet!\n", 
      rtp->plen - offset);

  return 0;
}

int dump_packet_rtp_unknown(rtp_header *rtp, FILE *out)
{
  fprintf(out, " Unknown packet type\n");

  return 0;
}

int dump_packet_rtp(const unsigned char *data, const int len, FILE *out)
{
  rtp_header rtp;

  parse_packet_rtp_header(data, len, &rtp);
  dump_packet_rtp_header(&rtp, out);

  switch (rtp.PT) {
    case 96: /* vorbis */
      dump_packet_rtp_vorbis(&rtp, out);
      break;
    case 98: /* theora */
      dump_packet_rtp_theora(&rtp, out);
      break;
    default:
      dump_packet_rtp_unknown(&rtp, out);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int RTPSocket, ret;
  struct sockaddr_in us, them;
  struct ip_mreq group;
  unsigned char data[MAX_PACKET];
  char *hostname;
  unsigned int port;

  if (argc < 2) hostname = "227.0.0.1";
  else hostname = argv[1];

  if (argc < 3) port = 4045;
  else port = atoi(argv[2]);

  fprintf(stderr, "Opening connection to %s port %d\n", hostname, port);

  RTPSocket = socket(AF_INET, SOCK_DGRAM, 0);

  if (RTPSocket < 0) {
    fprintf(stderr, "Unable to create socket.\n");
    exit(1);
  }

  us.sin_family = AF_INET;
  us.sin_addr.s_addr = htonl(INADDR_ANY);
  us.sin_port = htons(port);
  ret = bind(RTPSocket, (struct sockaddr *)&us, sizeof(us));
  if (ret < 0) {
    fprintf(stderr, "Unable to bind socket!\n");
    exit(1);
  }

  them.sin_family = AF_INET;
  them.sin_addr.s_addr = inet_addr(hostname);
  them.sin_port = htons(port);
  
  if (!IN_MULTICAST(ntohl(them.sin_addr.s_addr))) {
    fprintf(stderr, "not a multicast address\n");
  } else {
    fprintf(stderr, "joining multicast group...\n");
    group.imr_multiaddr.s_addr = them.sin_addr.s_addr;
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    ret = setsockopt(RTPSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
	(void *)&group, sizeof(group));
    if (ret < 0) {
      fprintf(stderr, "cannot join multicast group!\n");
      exit(1);
    }
  }

  while (1) {
    ret = recvfrom(RTPSocket, data, MAX_PACKET, 0, NULL, 0);
    fprintf(stderr, "read %d bytes of data\n", ret);
    dump_packet_rtp(data, ret, stdout);
    dump_packet_raw(data, ret, stdout);
  }

  return 0;
}
