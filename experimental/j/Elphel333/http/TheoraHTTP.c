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
// HTTP Ogg Streamer for Elphel Camera 333 input device
// Copyright (c) 2005 Jan Gerber
/////////////////////////////////////////////////////////////////////////////
#include <sys/time.h>
#include <sys/mman.h>           /* mmap */
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>


#include "c313a.h"

#include "defines.h"
#include "oggtheora.h"
#include "cgi.h"

#define D(x) 

#define HDR_INTERVAL   100   // send headers every 4 s for  25 fps

///////////////////// GLOBAL DATA ///////////////////
int                 mTimeinterval = 40000;  // default for 25 fps
const char          mmapFileName[]= "/var/tmp/mmap_strm";
strm_data_t         *strm_data;
static strm_data_t  strm_data_par;
int                 hdr_size;
/////////////// OggTheora data///////////////////////
int                 kfgshiftm=9;
len_data_t          packets[6];
uint32_t           header_lengths[6];
uint8_t             *b_headers;
/////////////////////////////////////////////////////


static void
usage()
{
    printf("\n");
    printf("\tTheora HTTP streamer for Elphel Camera model 333. \n");
    printf("\tBased on TheoraStrm 0.5 (RTP streamer)\n");
  
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

int make_OggTheoraHdrs(const char *headers_filename)
{
     FILE      *headers_file;
     uint32_t  num_headers;
     int        i,r;

     //rtp foo, has to go
    //struct stat     f_stat;

    //load header from file
    if((i=load_headers(headers_filename, packets, &kfgshiftm)) < 0)
        return i;

// now some RTP foo that is in no way RFC
/*
    stat(headers_filename,&f_stat);
    hdr_size = f_stat.st_size;
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // draft realese - TODO for RFC future
    b_headers = (uint8_t *)malloc(hdr_size+1);
    if ((headers_file = fopen(headers_filename, "r"))==NULL) {
        fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",
                       headers_filename,errno,strerror(errno)); 
        return -1;
    }
    // put data into memory buffer
    if (fread (b_headers+1,   hdr_size, 1, headers_file)<1) 
        return -7;
    hdr_size +=1;
    fclose(headers_file);
*/
    return 0;    
}

static void main_loop() {
    uint64_t    time_frame = 0;
    uint32_t    len=0;
    int         delta, nframe;
    char        headers_filename[]="/tmp/headers";
    char        controlfn[]="/dev/ccam_img";

    int         devfd, i;
    int         acq_data;
    int         nframes_total, nframes_inter_full, nframes_inter_masked;
    int         time_slow;
    uint8_t    *ccam_dma, *dp = NULL;
    uint8_t    *dp_prev = NULL;
    uint32_t   *ccam_dma_index;
    int         circindex_fd;
    int         circbuf_fd;
    int         circbuf_size;
    int         circindex_size;
 
    int         wp,wp0,frame_start = 0,frame_end,frame_type= 0,a_ready;
    uint32_t    frame_length;
    int         next_frame_start, frame_no =0,last_page_flags = 0;

    //FIXME, use some random number here.
    uint32_t   stream_serial=0x114a36b7;
    int         page = 0;
    
    //FIXME where does this have to go?                    
    uint64_t  granule=0;
    uint64_t    granule_mask=(1<<kfgshiftm)-1;
    int header_length;
    ogg_header_t header;
    int page_size=8192;

      int          frame_header_length;   // frame header lengh truncated to 32-bit words (the rest is combined with frame data)
      int          overlapped_header; // 0/4 - if "4" then bits from the first word of data are already copied to the header
      int          first_frame=1;

      int fw=0;
      FILE *output_fd=stdout;


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
    fprintf(stderr,"circbuf stores %d long values (%d bytes)\n",
                                                (circbuf_size>>2),circbuf_size);
    ccam_dma = 
        (uint8_t *) mmap(0, circbuf_size, PROT_READ, MAP_SHARED, circbuf_fd, 0);
    if(ccam_dma ==((uint8_t  *) -1)) {
        fprintf(stderr,"Error in mmap %s\r\n","/dev/circbuf");
        close(circindex_fd);
        close(circbuf_fd);
        return ;
    }
    
    wp=wp0=0;
    
    nframe = HDR_INTERVAL;
    nframes_total = 0;    // unlimited 
    nframes_inter_full = 128;
    nframes_inter_masked = 0;
    time_slow = (int)(100./strm_data_par.fps);
    time_slow = 1;
    fprintf(stderr,"fps =%f time_slow %d\n",strm_data_par.fps,time_slow);
    acq_data = ((nframes_total)<<16 ) 
              | (((nframes_inter_full   > 255)? 
                255:
                ((nframes_inter_full   < 0)?0:nframes_inter_full  )) << 8) 
              | ((nframes_inter_masked > 255)? 
                255:
                ((nframes_inter_masked < 0)?0:nframes_inter_masked));

    if(setup_tbl_FPGA( nframes_total, nframes_inter_full,
                                      nframes_inter_masked, time_slow)<0)
        return;
   
    // Ogg Header this should go in its own function: ogg_send_header 
    // first make and send first page
    i= load_headers(headers_filename, packets, &kfgshiftm);
    header_length = ogg_page (&header, (int)2, (int)0, stream_serial, page++, (int)1, &packets[0]);

    if (header_length<0) 
        return; // header_length-1000;
    if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
        fprintf(stderr,"error writing output data1 - error = %d (%s)\n",
                                                         errno,strerror(errno)); 
    }
    if ((fw= fwrite_all(output_fd, packets[0].data, packets[0].len))<0) {
        fprintf(stderr,"error writing output data2 - error = %d (%s)\n",
                                                         errno,strerror(errno)); 
    }


    //make and send second page with 2 packets
    header_length= ogg_page (&header, (int)0, (int)0, stream_serial, page++, (int)2, &packets[1]);

    if (header_length<0) 
        return; // header_length-2000;    
    if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
        fprintf(stderr,"error writing output data3 - error = %d (%s)\n",
                                                         errno,strerror(errno)); 
    }
    if ((fw= fwrite_all(output_fd, packets[1].data, packets[1].len))<0) {
        fprintf(stderr,"error writing output data4 - error = %d (%s)\n",
                        errno, strerror(errno)); 
    }
    if ((fw= fwrite_all(output_fd, packets[2].data, packets[2].len))<0) {
        fprintf(stderr,"error writing output data5 - error = %d (%s)\n",
                        errno,strerror(errno)); 
    }
    // Ogg Header End

    frame_no = 0;

    while (true) {
        // start acquisition
        fprintf(stderr,"start acquisition\n");
        if ((devfd = open(controlfn, O_RDONLY)) <0) {
            fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",
                    controlfn,errno,strerror(errno));
            return ;
        }
        // start compression
        fprintf(stderr,"start compressoin\n");
        ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_JPEG_GET_N ), acq_data);

        //wait for encoding to be done (or aborted?)
        fprintf(stderr,"encoding...\n");
        while ((i=ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_MONITOR_SEQ ), 0)) > CAMSEQ_JPEG) {
            
            // data ready ?    
            if(wp == 0) {
                while(frame_no <0)
                    frame_no = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;
                usec_sleep(mTimeinterval>>1);
                sched_yield();
                wp=1;
            }

            // dark_loop
            do {
                last_page_flags= (ccam_dma_index[frame_no+1]==0xffffffff)?4:0;
                
                if (last_page_flags == 4) {
                   usec_sleep(mTimeinterval);
                   sched_yield();
                }
            } while(last_page_flags == 4);
            //frame_no = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;
            // soft correction FPS    

            frame_length=(ccam_dma_index[frame_no] & 0xfffffff)-frame_start;
  
            //index of next frame in dma
            next_frame_start=((ccam_dma_index[frame_no] & 0xfffffff)+63) & (~0x1f);

            frame_type=(ccam_dma_index[frame_no]>>28) & 3;

            // figure out what this does?
            wp0 = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;
            if(wp0 < 0) {
                fprintf (stderr,"frame length %d \tframe_no %d = %d \n",
                                                     frame_length,frame_no,wp0);
                frame_no=wp0;
                wp = 0;
                continue;
            }

            frame_header_length =  ((packets[3+frame_type].len+31)>>3)& (~3);
            overlapped_header   =  (packets[3+frame_type].len & 31)?4:0;

            /* calculated granulepos */
            if (first_frame) {
                first_frame=0;
            } else if (frame_type==INTRA){
                granule=(granule+ (((granule&granule_mask)+1)<<kfgshiftm)) & (~granule_mask);
            } else {
                granule=(granule & (~granule_mask)) |  ((granule+1) &granule_mask);
            }

            //pointer to next theora frame in camera dma

            dp=&ccam_dma[frame_start];
            dp_prev = &ccam_dma[frame_start];

            //why this check for 1000000?
            if((unsigned)frame_length <1000000) {
                header_length= ogg_frame_page (&header,       // [1]
                                       last_page_flags,       // [2]
                                       page_size,             // [3]
                                       granule,
                                       stream_serial,
                                       page++,
                                       packets[3+frame_type], // [4]
                                       &frame_length,         // [5]
                                       &dp                    // [6]
                                      );
                
                /*
                 [1] pointer to the header data
                     (returns - header length in bytes)
                 [2] +1 - continuation, +4 - last (+2 - first)
                     "last" will be masked out if needed
                 [3]  maximal; page size to use
                 [4]  not used if (flags & 1). 
                      frame_header.len is measured in bits, not bytes!
                 [5]  frame data length
                 [6]  pointer to array with frame data(bytes)
                */

                if (header_length<0) 
                    fprintf(stderr, "ups\n");

                // write to output stream first (or only) page of a frame
                if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
                    fprintf(stderr,"error writing output data6 - error = %d (%s)\n",
                            errno,strerror(errno)); 
                }
                if ((fw= fwrite_all(output_fd, packets[3+frame_type].data, frame_header_length))<0) {
                    fprintf(stderr,"error writing output data7 - error = %d (%s)\n",
                            errno,strerror(errno)); 
                }
                if ((fw= fwrite_all(output_fd, dp_prev+overlapped_header, (int) (dp-dp_prev-overlapped_header)))<0) {
                    fprintf(stderr,"error writing output data8 - error = %d (%s)\n",
                            errno,strerror(errno)); 
                }

                while (frame_length>0) {
                    dp_prev=dp;
                    header_length= ogg_frame_page (&header,             // [1]
                                        last_page_flags,        // [2]
                                        page_size,              // [3]
                                        granule,
                                        stream_serial,
                                        page++,
                                        packets[3+frame_type],  // [4]
                                        &frame_length,          // [5]
                                        &dp);                   // [6]
        
                    // write to output stream other pages of a frame
                    if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
                       fprintf(stderr,"error writing output data9 - error = %d (%s)\n",
                                errno,strerror(errno)); 
                    }
                    if ((fw= fwrite_all(output_fd, dp_prev, (int) (dp-dp_prev)))<0){
                        fprintf(stderr,"error writing output data10 - error = %d (%s)\n",
                                errno,strerror(errno)); 
                    } 
                }
            }
            frame_start=next_frame_start;
            frame_no++;
            if(frame_no > wp0)
                wp = 0;

            nframe++;
            //???
            //xmemchk();
        }
        if(strm_data->status == STOP_STREAM ) {
        // stop sencor
            // system("hello -z 0");
            fprintf (stderr,"RTP Streamer stopped \n");
            break;   // receive command STOP from web interface
            //ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_JPEG ), JPEG_CMD_STOP);
        } // if STOP_STREAM         
    } // while main    
//=========================== END MAIN LOOP ============================ 
}


int main(int argc, char *argv[]) 
{
    int         devfd,c,flag,use_web;

    flag = 0;
    use_web = false;
    strcpy(strm_data_par.address,"239.0.0.1");
    strm_data_par.port = 20000;
    strm_data_par.fps  = 25.;
    strm_data_par.status   = START_STREAM;
    //strm_data_par.regim    = JPEG_ACCESS_DRV; 
    //strm_data_par.scanning = STOP_SCANNING;
    strcpy(strm_data_par.name,"Elphel Camera #1");
    strcpy(strm_data_par.telephone,"1-800-RTP-HOT-PHONE");
    strcpy(strm_data_par.toolname,"Elphel Theora Streamer");
    strcpy(strm_data_par.note,"Draft release ofr RTP Theora");
    strcpy(strm_data_par.location,"This camera located anywhere");
    strcpy(strm_data_par.email,"camera@elphel.com");
    strm_data_par.encrypt_key[0]='\0';


  /* these are from cgitest.cgi */
  int i;
  const char *method;
  const char *query;
  const char *uri;
  char  copyQuery [4096];

  const char mmapFileName[]= "/var/tmp/mmap_strm";
  strm_data_t    *strm_data;

  method = getenv("REQUEST_METHOD");
  query  = getenv("QUERY_STRING");
  uri    = getenv("REQUEST_URI");
  i= (query)? strlen (query):0;
     fprintf(stderr,"querry length=%d\n", i);

  if (i>(int)(sizeof(copyQuery)-1)) i= sizeof(copyQuery)-1;
  if (i>0) strncpy(copyQuery,query, i);
  copyQuery[i]=0;
  unescape(copyQuery,sizeof(copyQuery));

  if (method == NULL) {
    /* Not a CGI! */
    printf("This program should be run as a CGI from the web server!\n");
   // exit(1);
  }

  parseQuery(gparams, copyQuery);


    strm_data = &strm_data_par;  // use data from local buffer   
    // not used web - start streamer immediate
    strm_data_par.status = START_STREAM;    
    
    // OK - init data

    // General LOOP waiting command START
    printf("HTTP/1.0 200 OK\r\n");
    printf("Content-Type: application/ogg\r\n");
    printf("Pragma: no-cache\r\n\r\n");


    while(true) {
         if(strm_data->status == -1)
            break;
         if(strm_data->status == STOP_STREAM ) {
            //sleep some time and test status again
            usec_sleep(100000);
            continue;
         }

        mTimeinterval = (int) TimestampTicks/strm_data->fps; // min interval between frames
        fprintf(stderr,"Time frame %u for FPS %f, dest address %s:%d \n\n", mTimeinterval, strm_data->fps, 
        strm_data->address,strm_data->port);
        memcpy((void *)&strm_data_par,(void *)strm_data,sizeof(strm_data_t));
         
        fprintf(stderr,"\n HTTP Streamer for Theora starting...\n");
        main_loop();
  } // general while    
  return 0;
}
