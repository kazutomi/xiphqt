// Implementation of Theora RTP streaming
// Copyright 2005 Elphel, Inc
// Andrey Latin andrey.latin@gmail.com


#ifndef __OGGTHEORA_STRM_H_DEF_
#define __OGGTHEORA_STRM_H_DEF_

typedef enum {
    INTRA = 0,
    INTER_FULL = 1,
    INTER_PARTIAL = 2
} FrameType;


typedef struct {
  int     len;
  char *  data;
} len_data_t;


typedef struct {
  uint8_t            OggS[4];
  uint8_t            version;
  uint8_t            flags;
//  uint64_t           granule;
//  uint32_t           serial;
//  uint32_t           page;
//  uint32_t           crc;
  uint8_t           granule[8];
  uint8_t           serial[4];
  uint8_t           page[4];
  uint8_t           crc[4];
  uint8_t           num_segm;
  uint8_t           lacing_values [255];

} ogg_header_t;

#define C333_MAXWIDTH (4096)
#define C333_MAXHEIGHT (2048)

typedef struct {
     int        nstx; // width  in superblocks
     int        nsty; // height in superblocks
     int        frn; // frame rate nominator ( in 1/1000000)
     int        nobr; // nominal bit rate (-1 - undefined?)
     int        qual; // relative quality (was 0x10)
     int        kfgshiftm; // will calculate
     int        nuser;
     char      *vendor;
     char     **user;

     int        qual_left; // all in pixels, high quality will be inside and including the spacified coordinates
     int        qual_bottom;
     int        qual_right;
     int        qual_top;
     int        skip_left;
     int        skip_bottom;
     int        skip_right;
     int        skip_top;
     int        nqis;
     int        qis[2];
     int  sharpness;       // 0-2 - obsolete now in Theora
     int  pre_ht[12];
     int  pre_ht30[6];

     uint16_t   fpga_mask[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8]; // [64][16]
     uint32_t   fpga_zerobin[6];
     uint32_t   fpga_qtables[512];
     uint16_t   fpga_hti    [12];
     uint16_t   fpga_hti30  [16];
     uint32_t   fpga_htable [1024];
     uint16_t   fpga_firstbit[4];
} fpga_params;



int oggtheora_prepare (const char *headers_filename, // file to save headers
                       uint16_t   fpga_mask[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8], // [64][16]
                       uint32_t   fpga_zerobin[6],
                       uint32_t   fpga_qtables[512],
                       uint16_t   fpga_hti    [12],
                       uint16_t   fpga_hti30  [16],
                       uint32_t   fpga_htable [1024],
                       uint16_t   fpga_firstbit[4],
                       int         nstx, //number of supertiles (128x64) horizontal
                       int         nsty, //number of supertiles (128x64) vertical
                       int         frn,  // frame rate nominator (1000000=1fps)
                       int         nobr, // (-1 - "undfined")
                       int         qual, // realtive quality =16
                       int         kfgshiftm,
                       char       *vendor, // "Xiph.Org libTheora I 20040317 3 2 0"
                       int         nuser,
                       char      **user,
// selective qi and skipping blocks is not tested yet
                       int         qual_left,
                       int         qual_bottom,
                       int         qual_right,
                       int         qual_top,
                       int         skip_left,
                       int         skip_bottom,
                       int         skip_right,
                       int         skip_top,
                       int         qis[2], // 0..63 if both are the same, single qi will be encoded
                       int         sharpness, // 0..2 - is it obsolete in encodr now?
// table that maps combinbatins of (frame type: 0 - intra, 1  inter whole, 2 - inter, partial (mapped)), AC(/DC) and chroma(/luma)
// to internal (FPGA) indices (1-of-6 - not 1-of-16 as there is not enough room in FPGA Block RAM) 
                       int         pre_ht[12],
// this table maps intenal (1-of-6) Huffman table index to the standard 1-of-16. Not all the tables in the standard set of 80 can work -
// currently code length is limited to 14 bits
                       int         pre_ht30[6]);


int ogg_page      (ogg_header_t *    ogg_header, // pointer to the header data (returns - header length in bytes)
                            char     flags,
                            uint64_t granule,
                            uint32_t stream_serial,
                            int      page,       // page
                            int      num_packets, // number of packets in the page
                   len_data_t *      packets);

/*
    similar to above, but dedicated to the frame data.
    There could be 2 types
    - first page of a frame (packet) then frame header will be attached 
      and bit-combined with the frame
    - continuation  page (flags bit 0 should be set) - 
        then no frame header will be added to the data
    currently there can not be two frames (packets) on one page
*/
int ogg_frame_page      (ogg_header_t *    ogg_header,      // pointer to the header data (returns - header length in bytes)
                            char           flags,           // +1 - continuation, +4 - last (+2 - first)
                            int            maxpage,         // maximal page size (w/o ogg header)
                            uint64_t      granule,         // 
                            uint32_t      stream_serial,   //
                            int            page,            // page
                            len_data_t     frame_header,    // not used if (flags & 1). frame_header.len is measured in bits, not bytes!
                            int          * len,             // pointer frame data length (will modify - if non-zero - need continuation)
                            uint8_t    ** frame_data);     // pointer to pointer to an array with frame data
                                                            // if it is not continuation data is bit-aligned to be combined with frame_header
                                                            // first word of frame data will be modified - OR-ed with the last one from
                                                            // the frame_header
                   




// load headers from file
int load_headers(const char *headers_filename, len_data_t *packets, int *kfgshiftm);

// now encapsulate compressed data and output stream (to stdout?)
// The two arrays will be just mmap-ed in the camera
int oggtheora_send    (const char *headers_filename, // filename of a file with 6 headers and their lengths (in bits)
                       uint8_t    ccam_dma      [CCAM_BYTES_PER_DMABUF],
                       uint32_t   ccam_dma_index[CCAM_DMA_INDEX_SIZE],
                       uint32_t   stream_serial);


int make_OggTheoraHdrs(const char *headers_filename);
int fwrite_all(FILE *  stream, char * data, int len); 
char * fnegets(FILE* fin, char * s);
int  setup_tbl_FPGA( int nframes_total,
		     int nframes_inter_full, // number of INTER frames (full) after each INTRA
                     int nframes_inter_masked, // number of INTER frames (masked) after each INTER full (not tested yet)
                     int time_slow);   // reduce reported fps by this

#if defined(__cplusplus)
}
#endif

#endif 
                      
