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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int dump_packet(unsigned char *data, const int len, FILE *out)
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

int main(int argc, char *argv[])
{
  int RTPSocket, ret;
  int optval = 0;
  struct sockaddr_in us, them;
  struct ip_mreq group;
  unsigned char data[MAX_PACKET];
  char *hostname;
  unsigned int port;

  if (argc < 2) hostname = "227.0.0.1";
  else hostname = argv[1];

  if (argc < 3) port = 4044;
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
    dump_packet(data, ret, stdout);
  }

  return 0;
}
