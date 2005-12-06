/******************************************************************************
This is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	
******************************************************************************/
// Streamer for Elphel Camera 333 input device
// Copyright (c) 2005 Andy Latin 
// All rights reserved.
//
// http://andy.qnx.org.ru
// mail to:  
// andy@qnx.org.ru   
// andrey.latin@gmail.com
/////////////////////////////////////////////////////////////////////////////
#include <sys/time.h>
#include <sys/mman.h>           /* mmap */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "c313a.h"


/*************** RTP UCL library *******************/
#include "uclconf.h"
#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "rtp.h"

#include "defines.h"
#include "oggtheora.h"

#define D(x) 

#define HDR_INTERVAL   100   // send headers every 4 s for  25 fps
///////////////////// GLOBAL DATA ///////////////////
int			mTimeinterval = 40000;  // default for 25 fps
const char 		mmapFileName[]= "/var/tmp/mmap_strm";
strm_data_t  		*strm_data;
static strm_data_t  	strm_data_par;
int			hdr_size;
/////////////// OggTheora data///////////////////////

 int        kfgshiftm=9;
 len_data_t packets[6];
 uint32_t  header_lengths[6];
 uint8_t    *b_headers;

/////////////////////////////////////////////////////

static void
usage()
{
    printf("\n");
    printf("\tTheora RTP streamer for Elphel Camera model 333. \n");
    printf("\tInitial version writed by: Andrey Latin\n");
    printf("\nTheoraStrm 0.5 (C) 2005 Elphel Team\n\n");
    printf("\nThis is distributed in the hope that it will be useful,\n");
    printf("\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("\nGNU General Public License for more details.\n\n");

    printf("\n\tUsage: TheoraStrm [-f <fps>] [-a <address>] [-p <port>] [-w] [-h]\n\n");
    printf("\t\t[-w] - use WEB interface for manage streamer\n\n");

    printf("\t If used WEB interface for manage of streamer, options enumerated\n");
    printf("\t below can be changed from browser.\n\n");
    
    printf("\t\t[-f<fps>] - max values fps(frame per second), default 25.\n");
    printf("\t\tRange from 1 to 30 fps.\n");
    printf("\t\t[-a<address>] - Unicast or multicast address for streaming RTP\n");
    printf("\t\t\tDefault 239.0.0.1\n");

    printf("\t\t[-p<port>] - port for streaming data. Must be EVEN!\n");
    printf("\t\t\tDefault 20000.\n");

    printf("\t\t[-k<password>] - set encryption mode with key <password> (DES)\n");
    printf("\t\t[-h]  - this help\n\n");
  exit(-1);
}


/* ------------------------------------------------------------------------- */
/* RTP callback related */

static void
sdes_print(struct rtp *session, uint32_t ssrc, rtcp_sdes_type stype) {
	const char *sdes_type_names[] = {
		"end", "cname", "name", "email", "telephone", 
		"location", "tool", "note", "priv"
	};
	const uint8_t n = sizeof(sdes_type_names) / sizeof(sdes_type_names[0]);

	if (stype > n) {
		/* Theoretically impossible */
		printf("boo! invalud sdes field %d\n", stype);
		return;
	}
	
	printf("SSRC 0x%08x reported SDES type %s - ", ssrc, 
	       sdes_type_names[stype]);

	if (stype == RTCP_SDES_PRIV) {
		printf("don't know how to display.\n");
	} else {
		printf("%s\n", rtp_get_sdes(session, ssrc, stype));
	}
}

static void
packet_print(struct rtp *session, rtp_packet *p) 
{
	printf("Received data (payload %d timestamp %06d size %d) ", 
		p->rtp_pak_pt, p->rtp_pak_ts, p->rtp_data_len);

	if (p->rtp_pak_ssrc == rtp_my_ssrc(session)) {
		/* Unless filtering is enabled we are likely to see
		   out packets if sending to a multicast group. */
		printf("that I just sent.\n");
	} else {
		printf("from SSRC 0x%08x\n", p->rtp_pak_ssrc); 
	} 
}

static void
rtp_event_handler(struct rtp *session, rtp_event *e) 
{
	rtp_packet	*p;
	rtcp_sdes_item	*r;

	switch(e->type) {
	case RX_RTP: 
		p = (rtp_packet*)e->data;
		packet_print(session, p);
		xfree(p); /* xfree() is mandatory to release RTP packet data */
		break;
	case RX_SDES:
		r = (rtcp_sdes_item*)e->data;
		sdes_print(session, e->ssrc, r->type);
		break;
	case RX_BYE:
		break;
	case SOURCE_CREATED:
		printf("New source created, SSRC = 0x%08x\n", e->ssrc);
		break;
	case SOURCE_DELETED:
		printf("Source deleted, SSRC = 0x%08x\n", e->ssrc);
		break;
	case RX_SR:
	case RX_RR:
	case RX_RR_EMPTY:
	case RX_RTCP_START:
	case RX_RTCP_FINISH:
   		free(e->data);
		break;
	case RR_TIMEOUT:
		printf("RR_TIMEOUT\n");
   		free(e->data);
		break;
	case RX_APP:
		printf("RX_APP\n");
   		free(e->data);
		break;
	}
	fflush(stdout);
}


//////////////////////////////////////////////////////////////////
// make RTP packets from Theora and send it 			//
//////////////////////////////////////////////////////////////////
static void
rtp_SendTheoraFrame( struct rtp* session, uint32_t rtp_ts,
                   uint8_t *theora_data, uint32_t len, uint8_t type_frame)
{
       uint8_t rtp_packet_buf[BUF_SIZE],*ptr;

	uint32_t	lastPacket,dataOffset;
        uint32_t 	bytesToSend,payloadLength,payload_len;
	int	m_first;

        ptr=&rtp_packet_buf[2];
	rtp_packet_buf[0]=0x0;
	rtp_packet_buf[1]=type_frame;
	m_first = 1;
	lastPacket = 0;	
	bytesToSend   = len ; 
	dataOffset    = 0;
        D(printf("data len %u \n",len);)
        // first buffer for sending has 2 byets and DATA
	// we fill buffer manualy
        if(len <= BUF_SIZE-2) {
		memcpy(ptr, theora_data, len);
        	 rtp_send_data(session, rtp_ts, RTP_PT_THEORA, lastPacket,
 		   0, NULL, rtp_packet_buf , len+2 , NULL, 0, 0);
		return;
	}
	else { 
	     memcpy(ptr,theora_data,BUF_SIZE-2);
       	     rtp_send_data(session, rtp_ts, RTP_PT_THEORA, lastPacket,
	   	0, NULL, rtp_packet_buf , BUF_SIZE , NULL, 0, 0);
        }

	payloadLength = BUF_SIZE;
        ptr = theora_data+BUF_SIZE-2;
	dataOffset = dataOffset + BUF_SIZE -2;	

	do {
         payload_len  = min(payloadLength,bytesToSend - dataOffset);

	 if(  payload_len < payloadLength) 
	      lastPacket = 1;
	 else
	      lastPacket = 0;

       	 rtp_send_data(session, rtp_ts, RTP_PT_THEORA, lastPacket,
	   0, NULL, ptr , payload_len , NULL, 0, 0);
 
  	ptr += payload_len;
	dataOffset += payload_len;
	 
	} while (dataOffset < bytesToSend);

}

// SEND  headers to clients. It is Not RFC!!!!!!!!!!!!!!!
static void 
rtp_SendTheoraHdrs(struct rtp* session, uint32_t rtp_ts,
		uint8_t *header_data,uint16_t len)
{
       uint8_t *ptr;

	int	lastPacket,dataOffset;
        int 	bytesToSend,payloadLength,payload_len;

        ptr = header_data;

        header_data[0] = 0x80;// it is NOT RFC !!! First byte in packet identification Headers	
	bytesToSend   = len;
	dataOffset    = 0;
	payloadLength = BUF_SIZE;

	do {
         payload_len  = min(payloadLength,bytesToSend - dataOffset);

	 if(  payload_len < payloadLength) 
	      lastPacket = 1;
	 else
	      lastPacket = 0;

//	 if(lastPacket) hdr_ptr->F_bit=1;

	 rtp_send_data(session, rtp_ts, RTP_PT_THEORA, lastPacket,
	   0, NULL, ptr + dataOffset , payload_len , NULL, 0, 0);
	 
	  dataOffset += payload_len;
	 
	} while (dataOffset < bytesToSend);

}


int make_OggTheoraHdrs(const char *headers_filename)
{
     FILE      *headers_file;
     uint32_t  num_headers;
     int        i,r;
     struct stat     f_stat;

    if ((headers_file = fopen(headers_filename, "r"))==NULL)
       {fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",headers_filename,errno,strerror(errno)); return -1;}
    if (fread (&num_headers,   4, 1, headers_file)<1) return -4;
    if (num_headers!=6) return -5;
    if (fread (header_lengths, 4, 6, headers_file)<6) return -5;
    if (fread (&kfgshiftm,     4, 1, headers_file)<1) return -5;

    for (i=0;i<3;i++) packets[i].len=header_lengths[i]>>3; // now - in bytes
    for (i=3;i<6;i++) packets[i].len=header_lengths[i]; // these are needed to be in bits
    for (i=0;i<6;i++) {
      header_lengths[i]=(header_lengths[i]+31)>>5;
      packets[i].data=malloc(header_lengths[i]<<2);
      if (((r=fread (packets[i].data, 4, header_lengths[i], headers_file))) < (int)header_lengths[i]) {
		 fprintf(stderr, "i=%d, fread returned %d, wanted - %d\n",i, r,header_lengths[i]);
		 return -6;
	   }	 
    }
    fclose(headers_file);
    stat(headers_filename,&f_stat);
    hdr_size = f_stat.st_size;
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // draft realese - TODO for RFC future
    b_headers = (uint8_t *)malloc(hdr_size+1);
    if ((headers_file = fopen(headers_filename, "r"))==NULL)
       {fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",headers_filename,errno,strerror(errno)); return -1;}
    // put data into memory buffer
    if (fread (b_headers+1,   hdr_size, 1, headers_file)<1) return -7;
    hdr_size +=1;
    fclose(headers_file);
	
return 0;    
}

static void
main_loop(struct rtp* session) 
{
	uint64_t        time_frame = 0;
	uint32_t	rtp_ts,len=0;
	int		delta, nframe;
	char		headers_filename[]="/tmp/headers";
	char       	controlfn[]="/dev/ccam_img";

	int 		devfd, i;
	int 		acq_data, nframes_total, nframes_inter_full,
			nframes_inter_masked;
	uint8_t   	*ccam_dma, *dp = NULL;
    	uint32_t  	*ccam_dma_index;
    	int        	circindex_fd;
   	int        	circbuf_fd;
    	int        	circbuf_size;
    	int       	circindex_size;
 
	int		wp,wp0,frame_start = 0,frame_end,frame_type= 0,a_ready;
	uint32_t	frame_length;
	int		next_frame_start, frame_no =0,last_page_flags = 0;

	printf("Sending to ");
	printf("%s port %d (local SSRC = 0x%08x)\n", 
	       rtp_get_addr(session),
	       rtp_get_rx_port(session),
	       rtp_my_ssrc(session));

      a_ready = 0; 
      circindex_fd = open("/dev/circindex", O_RDWR);
      if (circindex_fd<0) { // check control OK
         fprintf(stderr,"Error opening %s\n", "/dev/circindex");
         return ;
      }
      // find total buffer length
      circindex_size=lseek(circindex_fd,0,2);
      ccam_dma_index = (uint32_t *) mmap(0, circindex_size, PROT_READ, MAP_SHARED, circindex_fd, 0);
      if(ccam_dma_index ==((uint32_t  *) -1)) {
        fprintf(stderr,"Error in mmap %s\r\n","/dev/circindex");
        close(circindex_fd);
        return ;
      }

      circbuf_fd = open("/dev/circbuf", O_RDWR);
      if (circbuf_fd<0) { // check control OK
        fprintf(stderr,"Error opening %s\n", "/dev/circbuf");
        close(circindex_fd);
        return ;
      }
      // find total buffer length
      circbuf_size=lseek(circbuf_fd,0,2);
      fprintf(stderr,"circbuf stores %d long values (%d bytes)\n",(circbuf_size>>2),circbuf_size);
      ccam_dma = (uint8_t *) mmap(0, circbuf_size, PROT_READ, MAP_SHARED, circbuf_fd, 0);
      if(ccam_dma ==((uint8_t  *) -1)) {
        fprintf(stderr,"Error in mmap %s\r\n","/dev/circbuf");
        close(circindex_fd);
        close(circbuf_fd);
        return ;
      }
 
    
	InitTimer();
	wp=wp0=0;
	
   	nframe = HDR_INTERVAL;
	nframes_total = 0;	// unlimited 
	nframes_inter_full = 128;
	nframes_inter_masked = 0;
        fprintf(stderr,"fps =%f\n",strm_data_par.fps);
        acq_data = ((nframes_total)<<16 ) |
                  (((nframes_inter_full   > 255)? 255:((nframes_inter_full   < 0)?0:nframes_inter_full  )) << 8) |
                   ((nframes_inter_masked > 255)? 255:((nframes_inter_masked < 0)?0:nframes_inter_masked));


	if(setup_tbl_FPGA( nframes_total, nframes_inter_full,
 				nframes_inter_masked)<0) return;
 
   	if(make_OggTheoraHdrs(headers_filename)<0) {
		fprintf(stderr,"Error prepare headers while parsing %s\n",headers_filename);
		free(b_headers);
		return;
	}
	frame_no = 0;

	while (true) {

	// now - time to start acquisition
    	if ((devfd = open(controlfn, O_RDONLY)) <0) {
        	fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",controlfn,errno,strerror(errno)); 
		return ;
	}
	// start compression
      	ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_JPEG_GET_N ), acq_data);

	//    now wait for done (or aborted?)
    	while (((i=ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_MONITOR_SEQ ), 0))) > CAMSEQ_JPEG) {

	     if(strm_data->status == STOP_STREAM ) {
			break;   // receive command STOP from web interface
	     }
	// data ready ?    
	if(wp == 0) {
        while(frame_no <0)
 	 frame_no = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;
         usec_sleep(mTimeinterval>>1);
	 sched_yield();
	 wp=1;
	}

dark_loop:

        last_page_flags= (ccam_dma_index[frame_no+1]==0xffffffff)?4:0;

	if (last_page_flags == 4)
	{
            usec_sleep(mTimeinterval);
	    sched_yield();
	    goto dark_loop;    
	}
//	 frame_no = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;

		// soft correction FPS    
		if(time_frame == 0) time_frame=GetTimestamp()+1; // when starting - send headers to clients
			// check number of sended data frames
			// may be we are ready to send Theora Headers?
		if(nframe >= HDR_INTERVAL) {
			rtp_SendTheoraHdrs(session,TimestampToRtp(GetTimestamp()),
				           b_headers , hdr_size);
				nframe = 0;
		}
   		delta=time_frame-GetTimestamp(); // may be waiting more ?
  
   		if( delta  > 0) {
   	   		while(delta > 0){
       				if(delta <= 10000) 
           				usec_sleep(0); // sleeps 1 clock tick (10ms)!
       				else
           				usec_sleep(delta-10000);
       				delta-=GetRelativeTime();
     	    		}
      		}
        frame_length=(ccam_dma_index[frame_no] & 0xfffffff)-frame_start;
 //       printf("FFFF %u \n",(ccam_dma_index[frame_no] & 0xfffffff)- frame_start);
        next_frame_start=((ccam_dma_index[frame_no] & 0xfffffff)+63) & (~0x1f);
        frame_type=(ccam_dma_index[frame_no]>>28) & 3;
	wp0 = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;
	if(wp0 < 0) {
		printf("frame length %d \tframe_no %d = %d \n",frame_length,frame_no,wp0);

		frame_no=wp0;
		wp = 0;
		continue;
	}

   		rtp_ts =TimestampToRtp(time_frame);

   		time_frame+= mTimeinterval; // time start for next frame INTER/INTRA
   		/* Send control packets */
   		rtp_send_ctrl(session, rtp_ts, NULL);
   		rtp_update(session);


        dp=&ccam_dma[frame_start];
        if((unsigned)frame_length <1000000)
 	     rtp_SendTheoraFrame( session, rtp_ts, dp, frame_length, frame_type);

        frame_start=next_frame_start;
	frame_no++;
	if(frame_no > wp0)
		wp = 0;

		nframe++;
		xmemchk();
	   }
     if(strm_data->status == STOP_STREAM ) {
	// stop sencor
        system("hello -z 0");
        printf("RTP Streamer stopped \n");
	break;   // receive command STOP from web interface

  //ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_JPEG ), JPEG_CMD_STOP);
	} // if STOP_STREAM	     

   } // while main	

//=========================== END MAIN LOOP ============================ 
   
}


int main(int argc, char *argv[]) 
{
	struct rtp	*session = NULL;
 	int 		devfd,c,flag,use_web;

    flag = 0;
    use_web = false;
    strcpy(strm_data_par.address,"239.0.0.1");
    strm_data_par.port = 20000;
    strm_data_par.fps  = 25.;
    strm_data_par.status   = STOP_STREAM;
//    strm_data_par.regim    = JPEG_ACCESS_DRV; 
//    strm_data_par.scanning = STOP_SCANNING;
    strcpy(strm_data_par.name,"Elphel Camera #1");
    strcpy(strm_data_par.telephone,"1-800-RTP-HOT-PHONE");
    strcpy(strm_data_par.toolname,"Elphel Theora Streamer");
    strcpy(strm_data_par.note,"Draft release ofr RTP Theora");
    strcpy(strm_data_par.location,"This camera located anywhere");
    strcpy(strm_data_par.email,"camera@elphel.com");
    strm_data_par.encrypt_key[0]='\0';

   while(true) {
     c = getopt(argc, argv, "hwa:p:f:k:");
     if (c == -1)
	       break;
     
     switch (c) {
      	case 'h':
	    flag++;	
            break;
	case 'w':
	    use_web = true; 	
            break;
      	case 'a':
	    memset(strm_data_par.address,0,21);
	    strncpy(strm_data_par.address,optarg,20);
            break;
      	case 'k':
	    memset(strm_data_par.encrypt_key,0,41);
	    strncpy(strm_data_par.encrypt_key,optarg,40);
	    strm_data_par.encryption=1;
            break;
      	case 'p':
	    strm_data_par.port = atol(optarg);	
            break;
       	case 'f':
	    strm_data_par.fps = atof(optarg);
            if(strm_data_par.fps < 0. )
		strm_data_par.fps=25.;	
            break;

     	case '?':
	     flag++;
             break;

     	default:
           printf ("?? getopt returned character code 0%o ??\n", c);
       }
    }
   if ((optind < argc)|| flag>0) {
	  usage();
	  exit(1);
   }

   if(use_web) {
        unlink(mmapFileName);
	// map shared memory for manage this with CGI 
	// where shared memory opening whith query too
	
        devfd = open(mmapFileName,O_RDWR|O_CREAT); 
        if(devfd == -1) {
  	       printf("Error opening %s!\n",mmapFileName);	  
	       
        }
	lseek(devfd,sizeof(strm_data_t),SEEK_SET);
	write(devfd,"",1);

        strm_data =(strm_data_t *) mmap(0, sizeof(strm_data_t), PROT_READ|PROT_WRITE, MAP_SHARED, devfd, 0);
        if((int)strm_data == -1) {
             fprintf(stderr,"Error in mmap %s\r\n",mmapFileName);
	     close(devfd);
	     unlink(mmapFileName);
	     exit(1);
        }
    } 
    else {
	strm_data = &strm_data_par;  // use data from local buffer   
	// not used web - start streamer immediate
	strm_data_par.status = START_STREAM;
    }   	
	
       	// OK - init data

       // General LOOP waiting command START
	while(true) {
	     if(strm_data->status == -1)
		break;
	     if(strm_data->status == STOP_STREAM ) {
		  //sleep some time and test status again
		  usec_sleep(100000);
		  continue;
	     }
	printf("\n RTP Streamer for Theora starting...\n");

	mTimeinterval = (int) TimestampTicks/strm_data->fps; // min interval between frames
        printf("Time frame %u for FPS %f, dest address %s:%d \n\n", mTimeinterval, strm_data->fps, 
			strm_data->address,strm_data->port);
	memcpy((void *)&strm_data_par,(void *)strm_data,sizeof(strm_data_t));
	//OK - receive command START streaming    
	// create session and transmit data	
	// address,port and fps(soft - not hardware) may be changed
	// from web interfase
        session = rtp_init_xmitter( strm_data->address,	/* Host/Group IP address */ 
			   strm_data->port,		/* receive port */
			   strm_data->port,		/* transmit port */
			   16,			/* time-to-live */
			   8000000,		/* B/W estimate */
			   rtp_event_handler,	/* RTP event callback */
			   NULL);		/* App. specific data */

	if (session) {

             if(strm_data->encryption) {
                if (!rtp_set_encryption_key(session,strm_data->encrypt_key))
			printf("Can't init encryption !\n ");
             }

	     uint32_t 	my_ssrc = rtp_my_ssrc(session);

		/* Set local participant info */
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_NAME,strm_data->name, strlen(strm_data->name));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_EMAIL,strm_data->email, strlen(strm_data->email));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_PHONE,strm_data->telephone, strlen(strm_data->telephone));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_LOC,strm_data->location, strlen(strm_data->location));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_TOOL,strm_data->toolname, strlen(strm_data->toolname));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_NOTE,strm_data->note, strlen(strm_data->note));

		/* Filter out local packets if requested */
		rtp_set_option(session, RTP_OPT_FILTER_MY_PACKETS, true );
		rtp_set_option(session, RTP_OPT_WEAK_VALIDATION, false);
		rtp_set_option(session, RTP_OPT_PROMISC, true);

		// Run main loop - get data from buffer and sent RTP packets
		// while this process not received command stop
		main_loop(session);

		/* Say bye-bye */
		rtp_send_bye(session);
		rtp_done(session);
		xmemchk();
	} 
	else {
	   fprintf(stderr,"Could not initialize session for %s port %d\n", strm_data->address, strm_data->port);
	}
     } // general while	
  return 0;
}
