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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>


#include "c313a.h"

#include "defines.h"
#include "oggtheora.h"

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
u_int32_t           header_lengths[6];
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
     u_int32_t  num_headers;
     int        i,r;

     //rtp foo, has to go
    //struct stat     f_stat;

    if ((headers_file = fopen(headers_filename, "r"))==NULL) {
        fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",
                        headers_filename,errno,strerror(errno)); 
        return -1;
    }
    if (fread (&num_headers,   4, 1, headers_file)<1) 
        return -4;
    if (num_headers!=6) 
        return -5;
    if (fread (header_lengths, 4, 6, headers_file)<6) 
        return -5;
    if (fread (&kfgshiftm,     4, 1, headers_file)<1) 
        return -5;

    for (i=0;i<3;i++) 
        packets[i].len=header_lengths[i]>>3; // now - in bytes
    for (i=3;i<6;i++) 
        packets[i].len=header_lengths[i]; // these are needed to be in bits
    for (i=0;i<6;i++) {
        header_lengths[i]=(header_lengths[i]+31)>>5;
        packets[i].data=malloc(header_lengths[i]<<2);
        if (((r=fread (packets[i].data, 4, header_lengths[i], headers_file))) < (int)header_lengths[i]) {
           fprintf(stderr, "i=%d, fread returned %d, wanted - %d\n",
                           i, r,header_lengths[i]);
           return -6;
        }     
    }
    fclose(headers_file);
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
    u_int8_t    *ccam_dma, *dp = NULL;
    u_int32_t   *ccam_dma_index;
    int         circindex_fd;
    int         circbuf_fd;
    int         circbuf_size;
    int         circindex_size;
 
    int         wp,wp0,frame_start = 0,frame_end,frame_type= 0,a_ready;
    uint32_t    frame_length;
    int         next_frame_start, frame_no =0,last_page_flags = 0;

    //FIXME, use some random number here.
    u_int32_t   stream_serial=0x114a36b7;
    int         page = 0;
    
    //FIXME where does this have to go?                    
    u_int64_t  granule=0;
    int header_length;
    ogg_header_t header;
    int page_size=8192;


    a_ready = 0; 
    circindex_fd = open("/dev/circindex", O_RDWR);
    if (circindex_fd<0) { // check control OK
        fprintf(stderr,"Error opening %s\n", "/dev/circindex");
        return ;
    }
    // find total buffer length
    circindex_size=lseek(circindex_fd,0,2);
    ccam_dma_index = (u_int32_t *) mmap(0, circindex_size, PROT_READ, MAP_SHARED, circindex_fd, 0);
    if(ccam_dma_index ==((u_int32_t  *) -1)) {
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
        (u_int8_t *) mmap(0, circbuf_size, PROT_READ, MAP_SHARED, circbuf_fd, 0);
    if(ccam_dma ==((u_int8_t  *) -1)) {
        fprintf(stderr,"Error in mmap %s\r\n","/dev/circbuf");
        close(circindex_fd);
        close(circbuf_fd);
        return ;
    }
    
    InitTimer();
    wp=wp0=0;
    
    nframe = HDR_INTERVAL;
    nframes_total = 0;    // unlimited 
    nframes_inter_full = 128;
    nframes_inter_masked = 0;
    time_slow = (int)(100./strm_data_par.fps);
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
    
    if(make_OggTheoraHdrs(headers_filename)<0) {
        fprintf(stderr,"Error prepare headers while parsing %s\n",headers_filename);
        free(b_headers);
        return;
    }
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
        fprintf(stderr,"wait for encoding\n");
        while ((i=ioctl(devfd, _IO(CMOSCAM_IOCTYPE, IO_CCAM_MONITOR_SEQ ), 0)) > CAMSEQ_JPEG) {
            if(strm_data->status == STOP_STREAM ) {
                break;   // receive command STOP from web interface
            }
            // has to break here if http connection is closed
            
            // data ready ?    
            if(wp == 0) {
                while(frame_no <0)
                    frame_no = ioctl(devfd, _CCCMD( CCAM_RPARS , P_JPEG_WP), 0)-1;
                usec_sleep(mTimeinterval>>1);
                sched_yield();
                wp=1;
            }

            // dark_loop
            fprintf(stderr,"dark loop\n");
            do {
                // what does this do??? 
                last_page_flags= (ccam_dma_index[frame_no+1]==0xffffffff)?4:0;
                
                if (last_page_flags == 4) {
                   usec_sleep(mTimeinterval);
                   sched_yield();
                }
            } while(last_page_flags == 4);
            fprintf(stderr,"dark loop done\n");
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

            time_frame+= mTimeinterval; // time start for next frame INTER/INTRA

            //pointer to next theora frame in camera dma
            dp=&ccam_dma[frame_start];

            //why this check for 1000000?
            if((unsigned)frame_length <1000000) {
                fprintf(stderr,"ogg_frame_page\n");
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
                
                //FIXME now actually print the ogg page here.
                fprintf(stderr,"FIXME, now print ogg page, header_length %d = frame_length %d \n",
                                                     header_length,frame_length);
                
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
                fprintf (stderr,"?? getopt returned character code 0%o ??\n", c);
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
        strm_data = (strm_data_t *) mmap(0, sizeof(strm_data_t), 
                                     PROT_READ|PROT_WRITE, MAP_SHARED, devfd, 0);
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

        mTimeinterval = (int) TimestampTicks/strm_data->fps; // min interval between frames
        fprintf(stderr,"Time frame %u for FPS %f, dest address %s:%d \n\n", mTimeinterval, strm_data->fps, 
        strm_data->address,strm_data->port);
        memcpy((void *)&strm_data_par,(void *)strm_data,sizeof(strm_data_t));
         
        fprintf(stderr,"\n HTTP Streamer for Theora starting...\n");
        main_loop();
  } // general while    
  return 0;
}
