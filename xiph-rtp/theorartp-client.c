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

/* compile with: gcc -g -O2 -Wall -o vorbisrtp-client vorbisrtp-client.c -logg 
 * append -DCHECK -lvorbis to enable header checks*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ogg/ogg.h>
#include <theora/theora.h>

#define MAX_PACKET 1500

#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

typedef struct ogg_context {
	ogg_packet op;
	ogg_stream_state os;
	ogg_page og;
	long long int prev_gp;
	long int prev_keyframe;
	long int curr_frame;

	theora_info ti;
	theora_comment tc;
	theora_state td;


	int frag;
	unsigned int timestamp;

	int gp_shift;

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
  fprintf (out, " ssrc: 0x%08x", ssrc);
  if (CC)
    for (i = 0; i < CC; i++)
      fprintf (out, " csrc: 0x%08x",
	       ntohl (((unsigned int *) data)[3 + i]));
  else
    fprintf (out, " no csrc");
  fprintf (out, "\n");
  /* offset to payload header */
  offset = (3 + CC) * 4;

  /* parse payload header */
  ident = data[offset++]<<16;
  ident += data[offset++]<<8;
  ident += data[offset++];
  F = (data[offset] & 0xc0) >> 6;
  VDT = (data[offset] & 0x30) >> 4;
  pkts = (data[offset] & 0x0F);
  offset++;
  
  fprintf(out,"ident %06x, frag type %d, data type %d, pkts %d, size %d\n",
		 ident,F,VDT,pkts,len-4*(CC+3));
 
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

int cfg_parse( ogg_context_t *ogg )
{
	oggpack_buffer opb;
	
	oggpackB_readinit(&opb,ogg->op.packet, ogg->op.bytes);
	
	oggpackB_read(&opb,8*7);
	oggpackB_read(&opb,8*3);
	oggpackB_read(&opb,16);
	oggpackB_read(&opb,16);

	oggpackB_read(&opb,64);
	
//	ogg->time_den =
	oggpackB_read(&opb,32);
//	ogg->time_num = 
	oggpackB_read(&opb,32);

	oggpackB_read(&opb,24);
	oggpackB_read(&opb,24);
	
	oggpackB_read(&opb,38);
	
	ogg->gp_shift = oggpackB_read(&opb,5);

	return 0; //FIXME add some checks and return -1 on failure
}

int
cfg_repack(ogg_context_t *ogg, FILE* out)
{
  ogg_packet id,co,cb;
  unsigned char comment[] = 
/*quite minimal comment*/
   { 0x81,'t','h','e','o','r','a',
   	10,0,0,0, 
   		't','h','e','o','r','a','-','r','t','p',
   		0,0,0,0};
  
/* get the identification packet*/
  id.packet = ogg->op.packet;
  id.bytes = 42;
  id.b_o_s = 1;
  id.e_o_s = 0;
  id.granulepos = 0;


/* get the comment packet*/
  co.packet = comment;
  co.bytes = sizeof(comment);
  co.b_o_s = 0;
  co.e_o_s = 0;
  co.granulepos = 0;

/* get the setup packet*/
  cb.packet = ogg->op.packet + 42;
  cb.bytes = ogg->op.bytes - 42;
  cb.b_o_s = 0;
  cb.e_o_s = 0;
  cb.granulepos = 0;
  
  cfg_parse(ogg);

#if CHECK
  //FIXME add some checks
 
#endif
/* start the ogg*/
  ogg_stream_init(&ogg->os,rand());

  ogg_stream_packetin(&ogg->os,&id);
  ogg_stream_packetin(&ogg->os,&co);
  ogg_stream_packetin(&ogg->os,&cb);

  do{
    int result=ogg_stream_flush(&ogg->os,&ogg->og);
    if(result==0)break;
    fwrite(ogg->og.header,1,ogg->og.header_len,out);
    fwrite(ogg->og.body,1,ogg->og.body_len,out);
    }while(!ogg_page_eos(&ogg->og));
  return 3;
}

int
pkt_repack(ogg_context_t *ogg, unsigned int timestamp, FILE *out){

  int frame_type;
  ogg_packet *op = &ogg->op;
  
  oggpack_buffer opb;
  oggpack_readinit(&opb,op->packet,op->bytes);
  oggpack_read(&opb,1); //video marker
  
  frame_type = oggpackB_read(&opb,1);

 
  if(timestamp)
  {
	  if (frame_type == 0) //KEY_FRAME
		  ogg->prev_keyframe = timestamp;
	  
	  ogg->curr_frame = timestamp;
	  
	  op->granulepos = (ogg->curr_frame - ogg->prev_keyframe)
		   | ogg->prev_keyframe << ogg->gp_shift;
  }
  else
  {
      if(ogg->prev_gp==-1)
      {
        op->granulepos = 0;
      }
      else
      {
        if(frame_type == 0) //KEY_FRAME
	{
          ogg->prev_keyframe = op->granulepos &
		  ((1<<ogg->gp_shift)-1);
          op->granulepos >>= ogg->gp_shift;
          op->granulepos += ++ogg->prev_keyframe;
          op->granulepos <<= ogg->gp_shift;
        }
	else
          op->granulepos++;
      }
  }

  ogg->prev_gp = op->granulepos;

#ifdef DEBUG
  fprintf(stderr," type %d, timestamp %d, bytes %ld bos %ld eos %ld gp %lld pno %lld\n",frame_type, timestamp, op->bytes, op->b_o_s, op->e_o_s, op->granulepos, op->packetno );
#endif
 

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
  
  switch (F) {

  case 0:
    op->bytes = data[offset++]<<8;
    op->bytes += data[offset++];
    op->packet = &data[offset];
    op->packetno++;
    offset += op->bytes;
    break;
  case 1:
    op->bytes = 0;
    //FIXME malloc checks
    op->packet = NULL;
  case 2:
    length = data[offset++] << 8;
    length += data[offset++];
    op->packet = realloc (op->packet, length+op->bytes);
    memcpy (op->packet + op->bytes, data + offset, length);
    op->bytes += length;
    return 0;
  case 3:
    length = data[offset++] << 8;
    length += data[offset++];
    op->packet = realloc (op->packet, length+op->bytes);
    memcpy (op->packet + op->bytes, data + offset, length);
    op->bytes += length;
    break;
  default:
    fprintf (stderr, " unknown frament?!\n");
    return 0;
  }
  
  switch (VDT) {

  case 0:
  
  count += pkt_repack(ogg, timestamp, out);
  
  for (i = 1; i < pkts; i++)
    {
      if (offset >= len)
	{
	  fprintf (stderr, "payload length overflow. corrupt packet?\n");
	  return -1;
	}
      op->bytes = data[offset++]<<8;
      op->bytes += data[offset++];
      op->packet = &data[offset];
      op->packetno++;
      count += pkt_repack(ogg, 0, out);
      offset += op->bytes;
    }
  break;
  case 1:
    count = cfg_repack(ogg, out);
    op->packetno+=3;
    break;
  default:
    //ignore
    break;
    
  }
  if ( F == 3 ) 
  {
	free(op->packet);
	op->packet = NULL;
  }
  return count;
}

int
main (int argc, char *argv[])
{
  int RTPSocket, ret;
  FILE *file=stdout;
  int verbose = 0, dump = 0, opt;
  struct sockaddr_in us, them;
  struct ip_mreq group;
  unsigned char data[MAX_PACKET];
  char *hostname = "227.0.0.1", *filename = "out.ogg";
  unsigned int port = 4044;

  ogg_context_t ogg;
  memset(&ogg,0,sizeof(ogg_context_t));
  ogg.prev_gp=-1;


  fprintf (stderr, "  Vorbis RTP Client (draft-barbato-avt-rtp-theora-05)\n");

  while ((opt = getopt (argc, argv, "i:p:f:v")) != -1)
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
	case 'v':
	  verbose=1;
	  break;

	  /* Unknown option  */
	case '?':
	  fprintf (stderr, "\n||  Unknown option `-%c'.\n", optopt);
	  fprintf (stderr, "||  Usage: vorbisrtp-client [-i ip address] [-p port] [-f filename]\n");
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
      
      if (verbose) dump_packet_rtp (data, ret, stderr);
      if (dump){
	dump_packet_ogg (data, ret, file, &ogg);
	fflush(NULL);
      }
    }

  return 0;
}
