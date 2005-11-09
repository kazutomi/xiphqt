/*
  Copyright (c) 2005, Fluendo / Xiph.Org

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.

  * Neither the name of the Xiph.Org nor the names of its contributors may 
    be used to endorse or promote products derived from this software without 
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
  THE POSSIBILITY OF SUCH DAMAGE.

  Author: Luca Barbato <lu_zero@gentoo.org>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <vorbis/codec.h>
#include <theora/theora.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <xiph_rtp.h>


//FIXME remove the exit
int createsocket (xiph_rtp_t *xr, char *addr, unsigned int port, 
		  unsigned char TTL){
    int socket, ret;
    int optval = 0;
    struct sockaddr_in sin;
    rtp_headers_t *headers= &xr->headers;
    
    

/*===========================================================================*/
/*  Init RTP headers                                                         */
/*===========================================================================*/

/*  Sets v=2, p=0, x=0, cc=0  */
    headers -> flags1 = 0x80;

/*  Sets m=0, pt=96  */
    headers -> flags2 = 0x60;

    headers -> sequence = htonl (rand () & 65535);
    headers -> timestamp = htonl (rand ());
    headers -> ssrc = htonl (rand ());

/*===========================================================================*/
/*  Create socket                                                            */
/*===========================================================================*/

    socket = socket (AF_INET, SOCK_DGRAM, 0);

    if (socket < 0) {
        fprintf (stderr, "Socket creation failed.\n");
        exit (1);
    }

    xr->rtpsock.sin_family = sin.sin_family = AF_INET;
    xr->rtpsock.sin_port = sin.sin_port = htons (port);
    xr->rtpsock.sin_addr.s_addr = inet_addr (addr);

    ret = setsockopt (socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int));

    if (ret < 0) {
        fprintf (stderr, "setsockopt SO_REUSEADDR error\n");
        exit (1);
    }

/*===========================================================================*/
/*  Set multicast parameters                                                 */
/*===========================================================================*/

    if (IN_MULTICAST (ntohl (xr->rtpsock.sin_addr.s_addr))) {

        ret = setsockopt (socket, IPPROTO_IP, IP_MULTICAST_TTL, &TTL, sizeof (char));

        if (ret < 0) {
            fprintf (stderr, "Multicast setsockopt TTL failed.\n");
            exit (1);
        }

        optval = 1; 

        ret = setsockopt (socket, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof (char));

        if (ret < 0) {
            fprintf (stderr, "Multicast setsockopt LOOP failed.\n");
            exit (1);
        }
    }

    //FIXME
    //rx->socket = socket; 

    return socket;
}


