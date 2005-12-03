/*!***************************************************************************
*! FILE NAME  : oggtheora.c
*! DESCRIPTION: TBD
*! Copyright 2005 Elphel, Inc
*! -----------------------------------------------------------------------------**
*!
*!  oggtheora.c is free software; you can redistribute it and/or modify
*!  it under the terms of the GNU General Public License as published by
*!  the Free Software Foundation; either version 2 of the License, or
*!  (at your option) any later version.
*!
*!  oggtheora.c is distributed in the hope that it will be useful,
*!  but WITHOUT ANY WARRANTY; without even the implied warranty of
*!  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*!  GNU General Public License for more details.
*!
*!  You should have received a copy of the GNU General Public License
*!  along with oggtheora.c if not, write to the Free Software
*!  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*! -----------------------------------------------------------------------------**
*/

/****************** INCLUDE FILES SECTION ***********************************/

#include <stdio.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "c313a.h"
#include "oggtheora.h"


// we do our own ogg, borrowing crc code from libogg-1.1.1/framing.c
#define OC_NHUFFMAN_TABLES (80)
#define OC_NDCT_TOKENS     (32)

//#define D(x) x
#define D(x)

/*A Huffman code for a Theora DCT token.
  Each set of Huffman codes in a given table must form a complete, prefix-free
   code.
  There is no requirement that all the tokens in a table have a valid code,
   but the current encoder is not optimized to take advantage of this.
  If there is not at least one table with a code for every token in each of
   the five groups of 16 tables, then the encoder may fail to encode certain
   frames.
  The complete table in the first group of 16 does not have to be in the same
   place as the complete table in the other groups, but the complete tables in
   the remaining four groups must all be in the same place.*/
typedef struct{
  /*The bit pattern for the code, with the LSbit of the pattern aligned in
     the LSbit of the word.*/
  uint32_t pattern;
  /*The number of bits in the code.
    This must be between 0 and 32, inclusive.*/
  int          nbits;
}theora_huff_code;

#define ogg_uint32_t uint32_t

#include "oggtheora_tables.h"

// from theora/experimental/include/theora/theora.h
#define OC_EINVAL    (-10)

// from theora/experimental/lib/ocintrin.h
#define OC_MAXI(_a,_b)      ((_a)<(_b)?(_b):(_a))


// from theora/experimental/lib/huffenc.c

/*A description of a Huffman code value used when encoding the tree.*/
typedef struct{
  /*The bit pattern, left-shifted so that the MSB of all patterns is
     aligned.*/
  ogg_uint32_t pattern;
  /*The amount the bit pattern was shifted.*/
  int          shift;
  /*The token this bit pattern represents.*/
  int          token;
}oc_huff_entry;

extern int errno;

//#define C333_MAXWIDTH (4096)
//#define C333_MAXHEIGHT (2048)
// FPGA_MASK is used to specify (macroblock*4)-level "skip" and "low quality" (qi[0] - high, qi[1] - low)
// skip applies to some INTER frames and will not be coded.
// the mask is made for the 128x64 macroblock array, (0,0) matching the lower-left corner of the image
// each element (uint_16) encodes 8 macroblock flag pairs (LSB left, in each pair lower bit is quality,
// higher - "skip"

/*
static uint16_t FPGA_MASK[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8]; // [64][16]
static const uint32_t FPGA_ZEROBIN[6]={ // Index Sharpness S.Fact>50 Zbin       Round
                                 0x80a6, //   0       0        0      0.65(166)  0.499(128)
                                 0x76a6, //   1       0        1      0.65(166)  0.460(118)
                                 0x7ac0, //   2       1        0      0.75(192)  0.476(122)
                                 0x66c0, //   3       1        1      0.75(192)  0.400(102)
                                 0x7ae6, //   4       2        0      0.90(230)  0.476(122)
                                 0x55e6};//   5       2        1      0.90(230)  0.333( 85)
//static uint32_t FPGA_QTABLES[512];
static uint32_t FPGA_QTABLES[512];
static uint16_t FPGA_HTI     [12];
static uint16_t FPGA_HTI30   [16];
static uint32_t FPGA_HTABLE  [1024];
static uint16_t FPGA_FIRSTBIT[4];

*/

   
void fill_FPGA_qtable (uint32_t *table,
                         int       inter,
                         int       color,
                         int       qindex_dc, // always the same in a frame
                         int       qindex_ac, // may be different if it is a second qi in a frame
                         int       sharpness);  // 0..2 - common for all q
   void fill_FPGA_qtables (
     uint32_t  tables[512],
     int  sharpness,
     int  intra_y_qi_hi,   // index in DC_Scale/AC_Scale tables, intra frame, luma
     int  intra_c_qi_hi,   // index in DC_Scale/AC_Scale tables, intra frame, chroma
     int  inter_y_qi_hi,   // index in DC_Scale/AC_Scale tables, inter frame, luma
     int  inter_c_qi_hi,   // index in DC_Scale/AC_Scale tables, inter frame, chroma

     int  intra_y_qi_lo,   // index in DC_Scale/AC_Scale tables, intra frame, luma
     int  intra_c_qi_lo,   // index in DC_Scale/AC_Scale tables, intra frame, chroma
     int  inter_y_qi_lo,   // index in DC_Scale/AC_Scale tables, inter frame, luma
     int  inter_c_qi_lo);   // index in DC_Scale/AC_Scale tables, inter frame, chroma
  // {len[3:0],data[13:0]} - data is MSB aligned
   int fill_FPGA_htables (uint16_t hti  [12],
                          uint16_t hti30[16],
                          uint32_t htable[1024],
                          int       pre_ht[12],
                          int       pre_ht30[6]);


int theora_write_bits (char * buf, // buffer
                          int * bp,   // pointer to bit pointer
                          uint32_t data,   // data to write
                          int nb);     // number of bits to write
static int huff_entry_cmp(const void *_c1,const void *_c2);

int oc_huff_codes_pack(char *buf, int* bp,
 const theora_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]);

                   
int theora_identification (char *buf,  // returns length, buf should be big enough - 42 bytes
                              int   fmbw, // width in macroblocks
                              int   fmbh, // height in macroblocks
                              int   frn, // frame rate nominator ( in 1/1000000)
                              int   frd, // frame rate denominator (1000000)
                              int   nobr, // nominal bit rate (-1 - undefined?)
                              int   qual, // relative quality (was 0x10)
                              int   kfgshift); // number of bits in granule used for number of i-frames
int theora_comments (char       *buf,    // returns length, buf should be big enough - ?? bytes
                        int         buflen,  // buffer size (returns -1 if fails to fit)
                        const char *vendor, // Null-terminated string (was "Xiph.Org libTheora I 20040317 3 2 0\0")
                        int         nuser,   // number of user comments
                        const char **user);
   int theora_setup (char       *buf,    // returns length, buf should be big enough - ?? bytes
                     int         buflen,  // buffer size (returns -1 if fails to fit)
// the next pointers will likely point to constant tables, the same ones should be used to program FPGA internal tables                     
                     const int  *lflims,  // if null - skip  alltogether, not null - 5 assumed bits/value
                     const int  *acscale, // will encode 10bits/item
                     const int  *dcscale, // will encode 10bits/item
                     const int  bms[][64]    // array of 3 BMS tables
                        );


void fill_fpga_mask_windows (uint16_t  mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8],
                           int width,
                           int height, 
                           int qual_left, // all in pixels, high quality will be inside and including the spacified coordinates
                           int qual_bottom,
                           int qual_right,
                           int qual_top,
                           int skip_left,
                           int skip_bottom,
                           int skip_right,
                           int skip_top);
   int theora_frameheader (char       *buf,    // returns length, buf should be big enough - ?? bytes
                           int         buflen,  // buffer size (returns -1 if fails to fit) - not implemented yet
                           int         width,   // pixels - will ">>" as needed
                           int         height,  // pixels - will ">>" as needed
                           int         frame_flags,   // 0 - intra, 1 - inter(full), 3 - inter (skip according mask)
                           int         nqis,     // number of qi in a frame (valid 1 or 2)
                           int         qis[2], // q[0] - high, DC, q[1] - low (AC only)
                          
                           uint16_t   mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8] //
//                           int mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8] //
                        );
   void theora_frameheaders(int         width,   // pixels - will ">>" as needed
                           int         height,  // pixels - will ">>" as needed
                           len_data_t  frame_headers[3],   // [0] - intra, [1] - inter(full), [2] - inter (skip according mask)
                           int         nqis,     // number of qi in a frame (valid 1 or 2)
                           int         qis[2], // q[0] - high, DC, q[1] - low (AC only)
                           uint16_t   mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8] //
                        );



    int makeOggTheora(uint32_t * ccam_dma_index, // will be mmaped array in the camera
                      uint8_t  * ccam_dma,       // will be mmaped array in the camera (char array)
                      FILE *      output_fd,
                      int         page_size,  // maximal; page size to use
                      len_data_t  packets[6], // 0 - identification
                                              // 1 - comment
                                              // 2 - setup
                                              // 3 - frame - intra (.len - in bits, not bytes!)
                                              // 4 - frame - inter/full
                                              // 5 - frame - inter/mapped
                      uint32_t   stream_serial,
                      int         kfgshiftm // number of bits in granule used for number of i-frames
                      );
                      
void write_7_2_1(char * buf,         // bitstream buffer
                    int  * bp,          // bitstream bit pointer
                    int    length);     // run length to encode

void write_7_2_2(char * buf,         // bitstream buffer
                    int  * bp,          // bitstream bit pointer
                    int    length);     // run length to encode

void   put_7_2_1(char * buf,         // bitstream buffer
                    int  * bp,          // bitstream bit pointer
                    int  * current_bit, // current running bit (-1 - undefined)
                    int  * run,         // current length of bits (when current bit == -1 undefined)
                    int    bit,         // bit to encode (<0 - flush)
                    int    len);        // number of bits to put

   void   put_7_2_2(char * buf,         // bitstream buffer
                    int  * bp,          // bitstream bit pointer
                    int  * current_bit, // current running bit (-1 - undefined)
                    int  * run,         // current length of bits (when current bit == -1 undefined)
                    int    bit,         // bit to encode (<0 - flush)
                    int    len);        // number of bits to put

                           
//++++++++++++++++++++++++++++++++++++++++++++++

void fill_fpga_mask_windows (uint16_t  mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8],
                           int width,
                           int height, 
                           int qual_left, // all in pixels, high quality will be inside and including the spacified coordinates
                           int qual_bottom,
                           int qual_right,
                           int qual_top,
                           int skip_left,
                           int skip_bottom,
                           int skip_right,
                           int skip_top) {
    const int mask[8]={3,0xc,0x30,0xc0,0x300,0xc00,0x3000,0xc000};
    int x,y,x8,d;
    int w  = (width-1)   >> 5;
    int h  = (height-1)  >> 5;
    int ql = qual_left   >> 5;
    int qb = qual_bottom >> 5;
    int qr = qual_right  >> 5;
    int qt = qual_top    >> 5;
    int sl = skip_left   >> 5;
    int sb = skip_bottom >> 5;
    int sr = skip_right  >> 5;
    int st = skip_top    >> 5;
    if (w>((C333_MAXWIDTH >>5) -1)) w=((C333_MAXWIDTH >>5) -1);
    if (h>((C333_MAXHEIGHT>>5) -1)) h=((C333_MAXHEIGHT>>5) -1);
D(fprintf(stderr,"fill_fpga_mask_windows w=%d, h=%d\n",w,h));
D(fprintf(stderr,"ql=%d qb=%d qr=%d qt=%d sl=%d sb=%d sr=%d st=%d\n",ql,qb,qr,qt,sl,sb,sr,st));
     
// this implementation is not optimized for speed - it is possible to do later.
    for (y=0;y<=h;y++) for (x=0;x<=w;x++) {
      d=((((x>qr) || (x<ql) || (y>qt) || (y<qb))? 1 : 0) |
         (((x>sr) || (x<sl) || (y>st) || (y<sb))? 2 : 0)) << ((x & 7) << 1);
         x8=x>>3;
         mask_array[y][x8] ^= ((mask_array[y][x8] ^ d) & mask[x & 7]);
D(if (d>0) fprintf(stderr,"d= 0x%x, x=%d, mask_array[%d][%d]=0x%x\n",d,x,y,x8,mask_array[y][x8]));
         
    }
}


   void write_7_2_1(char * buf,         // bitstream buffer
                    int  * bp,          // bitstream bit pointer
                    int    length)      // run length to encode
   {
     if        (length>=34) {
       theora_write_bits (buf, bp,  0x3f,6);
       theora_write_bits (buf, bp,  length-34,12);
     } else if (length>=18) {
       theora_write_bits (buf, bp,  0x3e,6);
       theora_write_bits (buf, bp,  length-18,4);
     } else if (length>=10) {
       theora_write_bits (buf, bp,  0x1e,5);
       theora_write_bits (buf, bp,  length-10,3);
     } else if (length>= 6) {
       theora_write_bits (buf, bp,  0x0e,4);
       theora_write_bits (buf, bp,  length- 6,2);
     } else if (length>= 4) {
       theora_write_bits (buf, bp,  0x06,3);
       theora_write_bits (buf, bp,  length-4,1);
     } else if (length>= 2) {
       theora_write_bits (buf, bp,  0x02,2);
       theora_write_bits (buf, bp,  length-2,1);
     } else  {
       theora_write_bits (buf, bp,  0x00,1);
     }
     
   }

void write_7_2_2(char * buf,         // bitstream buffer
                    int  * bp,          // bitstream bit pointer
                    int    length)      // run length to encode
   {
     if        (length>=15) {
       theora_write_bits (buf, bp,  0x1f,5);
       theora_write_bits (buf, bp,  length-15,4);
     } else if (length>=11) {
       theora_write_bits (buf, bp,  0x1e,5);
       theora_write_bits (buf, bp,  length-11,2);
     } else if (length>= 7) {
       theora_write_bits (buf, bp,  0x0e,4);
       theora_write_bits (buf, bp,  length- 7,2);
     } else if (length>= 5) {
       theora_write_bits (buf, bp,  0x06,3);
       theora_write_bits (buf, bp,  length-5,1);
     } else if (length>= 3) {
       theora_write_bits (buf, bp,  0x02,2);
       theora_write_bits (buf, bp,  length-3,1);
     } else  {
       theora_write_bits (buf, bp,  0x00,1);
       theora_write_bits (buf, bp,  length-1,1);
     }
     
   }

void put_7_2_1(char * buf,         // bitstream buffer
                  int  * bp,          // bitstream bit pointer
                  int  * current_bit, // current running bit (-1 - undefined)
                  int  * run,         // current length of bits (when current bit == -1 undefined)
                  int    bit,         // bit to encode (<0 - flush)
                  int    len)         // number of bits to put
   {
//  D(fprintf(stderr, "put_7_2_1: put %d of bit %d\n",len,bit));
     if (current_bit[0]<0) {
       if (bit >=0) { // do nothing for empty string
         theora_write_bits (buf, bp,  bit, 1); // start new run
         current_bit[0]=bit;
         run[0]=len;
         while (run[0]>=4129) {
           write_7_2_1(buf, bp, 4129);
           run[0]-=4129;
           if (run[0]>0) theora_write_bits (buf, bp,  bit, 1);
           else current_bit[0]=-1;
         }
       }
     } else { // current_bit >= 0
       if (bit <0) { //flush
         write_7_2_1(buf, bp, run[0]);
         current_bit[0]=-1;
//         run[0]=0; //?  not needed
       } else { // continue
         if (bit == current_bit[0]) {
           run[0]+=len;
           while (run[0]>=4129) {
             write_7_2_1(buf, bp, 4129);
             run[0]-=4129;
             if (run[0]>0) theora_write_bits (buf, bp,  bit, 1);
             else current_bit[0]=-1;
           }
         } else { // bit flipped
           write_7_2_1(buf, bp, run[0]);
           current_bit[0]=bit;
           run[0]=len;
           while (run[0]>=4129) {
             write_7_2_1(buf, bp, 4129);
             run[0]-=4129;
             if (run[0]>0) theora_write_bits (buf, bp,  bit, 1);
             else current_bit[0]=-1;
           }
         }
       }
     } 
   }
   

void put_7_2_2(char * buf,         // bitstream buffer
                  int  * bp,          // bitstream bit pointer
                  int  * current_bit, // current running bit (-1 - undefined)
                  int  * run,         // current length of bits (when current bit == -1 undefined)
                  int    bit,         // bit to encode (<0 - flush)
                  int    len)         // number of bits to put
   {
//  fprintf(stderr, "put_7_2_2: put %d of bit %d\n",len,bit);  
     if (current_bit[0]<0) {
       if (bit >=0) { // do nothing for empty string
         theora_write_bits (buf, bp,  bit, 1); // start new run
         current_bit[0]=bit;
         run[0]=len;
         while (run[0]>=30) {
           write_7_2_2(buf, bp, 30);
           run[0]-=30;
           if (run[0]>0) theora_write_bits (buf, bp,  bit, 1);
           else current_bit[0]=-1;
         }
       }
     } else { // current_bit >= 0
       if (bit <0) { //flush
         write_7_2_2(buf, bp, run[0]);
         current_bit[0]=-1;
//         run[0]=0; //?  not needed
       } else { // continue
         if (bit == current_bit[0]) {
           run[0]+=len;
           while (run[0]>=30) {
             write_7_2_2(buf, bp, 30);
             run[0]-=30;
             if (run[0]>0) theora_write_bits (buf, bp,  bit, 1);
             else current_bit[0]=-1;
           }
         } else { // bit flipped
           write_7_2_2(buf, bp, run[0]);
           current_bit[0]=bit;
           run[0]=len;
           while (run[0]>=30) {
             write_7_2_2(buf, bp, 30);
             run[0]-=30;
             if (run[0]>0) theora_write_bits (buf, bp,  bit, 1);
             else current_bit[0]=-1;
           }
         }
       }
     } 
   }
   
   int theora_frameheader (char       *buf,    // returns length, buf should be big enough - ?? bytes
                           int         buflen,  // buffer size (returns -1 if fails to fit) - not implemented yet
                           int         width,   // pixels - will ">>" as needed
                           int         height,  // pixels - will ">>" as needed
                           int         frame_flags,   // 0 - intra, 1 - inter(full), 3 - inter (skip according mask)
                           int         nqis,     // number of qi in a frame (valid 1 or 2)
                           int         qis[2], // q[0] - high, DC, q[1] - low (AC only)
                           uint16_t   mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8] //
                          )
   {
  D(fprintf(stderr, "theora_frameheader, frame_flags=%d, nqis=%d, qis[0]=%d,  qis[1]=%d\n",frame_flags,nqis,qis[0],qis[1]));
   
   int block_lev[C333_MAXHEIGHT>>6][C333_MAXWIDTH>>8]; // [32][16]
   int bp=0;
   int current_bit=-1;
   int run=0;
     
   int nstx=(width  >> 7);
   int nsty=(height >> 6);
   int nst=  nsty * nstx;
   int nsbs= nst  * 12;  // number of all superblocks (32x32)
   int nsbl= nst  * 8;   // number of luma superblocks
//   int nsbc= nst  * 2;   // number of superblocks for each color plane
//   int nbs=  nsbs << 4;  // total number of blocks
//   int sbpcoded;
//   int sbfcoded;
   int x,x8,xf,y,d,d1,d2,c,ncmbs,i;
   if (nqis>2) return -1;
   theora_write_bits (buf, &bp,  0,            1); // data packet
   theora_write_bits (buf, &bp,  frame_flags,  1); // frame type
   for (i=0; i<nqis; i++) {
     theora_write_bits (buf, &bp,  qis[0],       6); // high quality, then low one (if present)
     theora_write_bits (buf, &bp,  (i<(nqis-1))?1:0, 1); // more QIS
   }
  D(fprintf(stderr, "qis out - bp=%d\n",bp));
   if (frame_flags==1) { // inter full
     put_7_2_1(buf, &bp, &current_bit, &run, 0, nsbs); // no partially coded superblocks;
     put_7_2_1(buf, &bp, &current_bit, &run, -1, 0); // flush
     put_7_2_1(buf, &bp, &current_bit, &run, 1, nsbs); // all superblocks fully coded;
     put_7_2_1(buf, &bp, &current_bit, &run, -1, 0); // flush
  D(fprintf(stderr, "(1) - bp=%d\n",bp));
   } else if (frame_flags==3) { // inter - use mask
     put_7_2_1(buf, &bp, &current_bit, &run, 0, nsbl); // no partially coded superblocks in luma;
     for (y=0;y<(height>>5);y+=2) for (x=0;x<(width>>5);x+=2) { // build array
       x8=x>>3;
       xf= ((x>>1) & 3)<<2;
       d1= mask_array[y  ][x8]>>xf;
       d2= mask_array[y+1][x8]>>xf;
       d= ((d1 & 2)?0:1) | ((d2 & 2)?0:2) | ((d2 & 8)?0:4) | ((d1 & 8)?0:8);
       block_lev[y>>1][x8] =( (block_lev[y>>1][x8] ^ (d<<xf)) & (0xf << xf)) ^ block_lev[y>>1][x8];
       put_7_2_1(buf, &bp, &current_bit, &run, ((d!=0) && (d!=0xf))?1:0, 1); // specify partial for Cb
     }
  D(fprintf(stderr, "(2) - bp=%d\n",bp));
     for (y=0;y<(height>>5);y+=2) for (x=0;x<(width>>5);x+=2) { // build array
       x8=x>>3;
       xf= ((x>>1) & 3)<<2;
       d= (block_lev[y>>1][x8]>>xf) & 0xf;
       put_7_2_1(buf, &bp, &current_bit, &run, ((d!=0) && (d!=0xf))?1:0, 1); // specify partial for Cr
     }
     put_7_2_1(buf, &bp, &current_bit, &run, -1, 0); // flush
// now - fully coded
// luma first
  D(fprintf(stderr, "(3) - bp=%d\n",bp));
     ncmbs=0; // also count number of coded macroblocks (here - groups of 4)
     for (y=0;y<(height>>5);y++) for (x=0;x<(width>>5);x++) {
//       put_7_2_1(buf, &bp, &current_bit, &run,((mask_array[y  ][x>>3]>>((x&7)<<1)) & 2)?0:1,1);
       c=((mask_array[y  ][x>>3]>>((x&7)<<1)) & 2)?0:1;
       put_7_2_1(buf, &bp, &current_bit, &run,c,1);
       ncmbs+=c;
     }  
// repeat twice - for Cb and Cr
  D(fprintf(stderr, "(4) - bp=%d\n",bp));
     for (i=0;i<2;i++) for (y=0;y<(height>>5);y+=2) for (x=0;x<(width>>5);x+=2) {
       x8=x>>3;
       xf= ((x>>1) & 3)<<2;
       d= (block_lev[y>>1][x8]>>xf) & 0xf;
       put_7_2_1(buf, &bp, &current_bit, &run, (d==0)?1:0, 1); // fully coded
     }
     put_7_2_1(buf, &bp, &current_bit, &run, -1, 0); // flush
// partially coded - repeat twice (for Cb and Cr)
  D(fprintf(stderr, "(5) - bp=%d\n",bp));
     
     for (i=0;i<2;i++) for (y=0;y<(height>>5);y+=2) for (x=0;x<(width>>5);x+=2) { 
       x8=x>>3;
       xf= ((x>>1) & 3)<<2;
       d= (block_lev[y>>1][x8]>>xf) & 0xf;
       if ((d!=0) && (d!=0xf)) {
         put_7_2_2(buf, &bp, &current_bit, &run, (d & 1)?0:1, 4); // 4 blocks
         put_7_2_2(buf, &bp, &current_bit, &run, (d & 2)?0:1, 4); // 4 blocks
         put_7_2_2(buf, &bp, &current_bit, &run, (d & 4)?0:1, 4); // 4 blocks
         put_7_2_2(buf, &bp, &current_bit, &run, (d & 8)?0:1, 4); // 4 blocks

       }
     }
     put_7_2_1(buf, &bp, &current_bit, &run, -1, 0); // flush
   } else theora_write_bits (buf, &bp,  0, 3);  // 3 bits 0 for intra only
   
  D(fprintf(stderr, "(6) - bp=%d\n",bp));

   if (frame_flags!=0) { // inter any
     ncmbs=32*nst; // all coded - 32 macroblocks per supertile
// macroblock coding modes - all INTER_NOMV (0), so using scheme 5 that codes it as a single bit "0"
  D(fprintf(stderr, "ncmbs=%d\n",ncmbs));
     theora_write_bits (buf, &bp,  5, 3); // 3 bits - mscheme=5
     for (;ncmbs>=32;ncmbs-=32) theora_write_bits (buf, &bp,  0, 32); // 32 of coded macroblcks
     theora_write_bits (buf, &bp,  0, ncmbs); // <32 of coded macroblcks
  }
// For all INTER frames one bit MVMODE should be here, even if it is not used (inter_nomv)
   if (frame_flags!=0) // inter any
     theora_write_bits (buf, &bp,  0, 1);
    
// block-level qi
  D(fprintf(stderr, "(7) - bp=%d\n",bp));
   if (nqis>1) {
// for luma blocks granularity is now 1 superblock (16 blocks) so no need for Hilbert    
     if (current_bit>=0) return -2; // should be closed
// now - fully coded
// luma first
     for (y=0;y<(height>>5);y++) for (x=0;x<(width>>5);x++) {
       put_7_2_1(buf, &bp, &current_bit, &run,((mask_array[y  ][x>>3]>>((x&7)<<1)) & 1)?1:0,16);
     }
  D(fprintf(stderr, "(8) - bp=%d\n",bp));
     for (y=0;y<(height>>5);y+=2) for (x=0;x<(width>>5);x+=2) { // process Cb, build array
       x8=x>>3;
       xf= ((x>>1) & 3)<<2;
       d1= mask_array[y  ][x8]>>xf;
       d2= mask_array[y+1][x8]>>xf;
       d= ((d1 & 1)?1:0) | ((d2 & 1)?2:0) | ((d2 & 4)?4:0) | ((d1 & 4)?8:0);
       block_lev[y>>1][x8] ^= (block_lev[y>>1][x8] ^ (d<<xf)) & (0xf << xf);
       put_7_2_1(buf, &bp, &current_bit, &run, (d    ) & 1, 4);// four blocks with the same qi
       put_7_2_1(buf, &bp, &current_bit, &run, (d >>1) & 1, 4);// four blocks with the same qi
       put_7_2_1(buf, &bp, &current_bit, &run, (d >>2) & 1, 4);// four blocks with the same qi
       put_7_2_1(buf, &bp, &current_bit, &run, (d >>3) & 1, 4);// four blocks with the same qi
     }
  D(fprintf(stderr, "(9) - bp=%d\n",bp));
     for (y=0;y<(height>>5);y+=2) for (x=0;x<(width>>5);x+=2) { // build array
       x8=x>>3;
       xf= ((x>>1) & 3)<<2;
       d= (block_lev[y>>1][x8]>>0xf) & 0xf;
       put_7_2_1(buf, &bp, &current_bit, &run, (d    ) & 1, 4);// four blocks with the same qi
       put_7_2_1(buf, &bp, &current_bit, &run, (d >>1) & 1, 4);// four blocks with the same qi
       put_7_2_1(buf, &bp, &current_bit, &run, (d >>2) & 1, 4);// four blocks with the same qi
       put_7_2_1(buf, &bp, &current_bit, &run, (d >>3) & 1, 4);// four blocks with the same qi
     }
  D(fprintf(stderr, "(10) - bp=%d\n",bp));
     
     put_7_2_1(buf, &bp, &current_bit, &run, -1, 0); // flush
  D(fprintf(stderr, "(11) - bp=%d\n",bp));
     
   }
   return bp;
  }


  void theora_frameheaders(int         width,   // pixels - will ">>" as needed
                           int         height,  // pixels - will ">>" as needed
//                           int         frame_flags,   // 0 - intra, 1 - inter(full), 3 - inter (skip according mask)
                           len_data_t  frame_headers[3],   // [0] - intra, [1] - inter(full), [2] - inter (skip according mask)
                           int         nqis,     // number of qi in a frame (valid 1 or 2)
                           int         qis[2], // q[0] - high, DC, q[1] - low (AC only)
                           uint16_t   mask_array[C333_MAXHEIGHT>>5][C333_MAXWIDTH>>8] //
                        )
    {
      char * buf;
      int    i;
      buf = malloc(64000); //?? maximal size?
      frame_headers[0].len= theora_frameheader(buf, 64000, width, height, 0, nqis, qis,  mask_array);
      for (i=frame_headers[0].len; i<(((frame_headers[0].len+31)>>3)& (~0x3)); i++) buf[i]=0;
      frame_headers[0].data=malloc(((frame_headers[0].len+31)>>3)& (~0x3));
      memcpy (frame_headers[0].data,buf,((frame_headers[0].len+31)>>3)& (~0x3));
      
      frame_headers[1].len= theora_frameheader(buf, 64000, width, height, 1, nqis, qis,  mask_array);
      for (i=frame_headers[1].len; i<(((frame_headers[1].len+31)>>3)& (~0x3)); i++) buf[i]=0;
      frame_headers[1].data=malloc(((frame_headers[1].len+31)>>3)& (~0x3));
      memcpy (frame_headers[1].data,buf,((frame_headers[1].len+31)>>3)& (~0x3));

      frame_headers[2].len= theora_frameheader(buf, 64000, width, height, 3, nqis, qis,  mask_array);
      for (i=frame_headers[2].len; i<(((frame_headers[2].len+31)>>3)& (~0x3)); i++) buf[i]=0;
      frame_headers[2].data=malloc(((frame_headers[2].len+31)>>3)& (~0x3));
      memcpy (frame_headers[2].data,buf,((frame_headers[2].len+31)>>3)& (~0x3));
    }

// used void oggpackB_write(oggpack_buffer *b,unsigned long value,int bits) from theora/libogg-1.1.1/src/bitwise.c
// it is probably optimized so it will be a good idea to use the library, but now - just some shortcuts.
//   int theora_write_bits (char * buf, // buffer
//                          int * bp,   // pointer to bit pointer
//                          int data,   // data to write
//                          int nb)     // number of bits to write

/*write theora data bitwise
    but: buffer
    bp: pointer to bit pointer
    data: data to write
    nb: number of bits to write
    Return: bp+nb.*/  
int theora_write_bits (char * buf, int * bp, uint32_t data, int nb) {
    int i,m, bytp,bitp;
    if (((*bp | nb) & 7) == 0) { // byte sync not lost
        for (i=(nb-8); i>=0; i-=8) {
            buf[(*bp)>>3]=(data>>i) & 0xff;
            *bp+=8;
        }
    } else while (nb>0){ // processing on bit level
        m= 0xff >> (bitp=((*bp) & 7));
        bytp=(*bp)>>3;
        if ((bitp+nb)>=8) {
            buf[bytp]=((buf[bytp] ^ (data >> (bitp+nb-8))) & m) ^ buf[bytp];
            *bp=(bytp+1)<<3;
            nb-=(8-bitp);
        } else {
            buf[bytp]=((buf[bytp] ^ (data << (8-bitp-nb))) & m) ^ buf[bytp];
            *bp+=nb;
            break;
        }  
    }
    return *bp;                        
}


/*Compares two oc_huff_entry structures by their bit patterns.
  _c1: The first entry to compare.
  _c2: The second entry to compare.
  Return: <0 if _c1<_c2, >0 if _c1>_c2.*/
static int huff_entry_cmp(const void *_c1,const void *_c2){
  ogg_uint32_t b1;
  ogg_uint32_t b2;
  b1=((const oc_huff_entry *)_c1)->pattern;
  b2=((const oc_huff_entry *)_c2)->pattern;
  return b1<b2?-1:b1>b2?1:0;
}

/*Encodes a description of the given Huffman tables.
  Although the codes are stored in the encoder as flat arrays, in the bit
   stream and in the decoder they are structured as a tree.
  This function recovers the tree structure from the flat array and then
   writes it out.
  Note that the codes MUST form a Huffman code, and not merely a prefix-free
   code, since the binary tree is assumed to be full.
  _opb:   The buffer to store the tree in.
  _codes: The Huffman tables to pack.
  Return: 0 on success, or a negative value if one of the given Huffman tables
   does not form a full, prefix-free code.*/


   
//int oc_huff_codes_pack(oggpack_buffer *_opb,
int oc_huff_codes_pack(char *buf, int* bp,
 const theora_huff_code _codes[OC_NHUFFMAN_TABLES][OC_NDCT_TOKENS]){
  int i;
  for(i=0;i<OC_NHUFFMAN_TABLES;i++){
    oc_huff_entry entries[OC_NDCT_TOKENS];
    int           bpos;
    int           maxlen;
    int           mask;
    int           j;
    /*First, find the maximum code length so we can align all the bit
       patterns.*/
    maxlen=_codes[i][0].nbits;
    for(j=1;j<OC_NDCT_TOKENS;j++){
      maxlen=OC_MAXI(_codes[i][j].nbits,maxlen);
    }
    mask=(1<<maxlen)-1;
    /*Copy over the codes into our temporary workspace.
      The bit patterns are aligned, and the original entry each code is from
       is stored as well.*/
    for(j=0;j<OC_NDCT_TOKENS;j++){
      entries[j].shift=maxlen-_codes[i][j].nbits;
      entries[j].pattern=_codes[i][j].pattern<<entries[j].shift&mask;
      entries[j].token=j;
    }
    /*Sort the codes into ascending order.
      This is the order the leaves of the tree will be traversed.*/
    qsort(entries,OC_NDCT_TOKENS,sizeof(entries[0]),huff_entry_cmp);
    /*For each leaf of the tree:*/
    bpos=maxlen;
    for(j=0;j<OC_NDCT_TOKENS;j++){
      int bit;
      /*If this code has any bits at all.*/
      if(entries[j].shift<maxlen){
        /*Descend into the tree, writing a bit for each branch.*/
//        for(;bpos>entries[j].shift;bpos--)oggpackB_write(_opb,0,1);
        for(;bpos>entries[j].shift;bpos--)theora_write_bits(buf,bp,0,1);
        
        /*Mark this as a leaf node, and write its value.*/
//        oggpackB_write(_opb,1,1);
        theora_write_bits(buf,bp,1,1);
//        oggpackB_write(_opb,entries[j].token,5);
        theora_write_bits(buf,bp,entries[j].token,5);
        /*For each 1 branch we've descended, back up the tree until we reach a
           0 branch.*/
        bit=1<<bpos;
        for(;entries[j].pattern&bit;bpos++)bit<<=1;
        /*Validate the code.*/
        if(j+1<OC_NDCT_TOKENS){
          mask=~(bit-1)<<1;
          /*The next entry should have a 1 bit where we had a 0, and should
             match our code above that bit.
            This verifies both fullness and prefix-freeness simultaneously.*/
          if(!(entries[j+1].pattern&bit)||
           (entries[j].pattern&mask)!=(entries[j+1].pattern&mask)){
            return OC_EINVAL;
          }
        }
        /*If there are no more codes, we should have ascended back to the top
           of the tree.*/
        else if(bpos<maxlen)return OC_EINVAL;
      }
    }
  }
  return 0;
}


// code assumes little endian
int ogg_page      (ogg_header_t *    ogg_header, // pointer to the header data (returns - header length in bytes)
                            char     flags,
                           uint64_t granule,
                           uint32_t stream_serial,
                            int      page,       // page
                            int      num_packets, // number of packets in the page
                   len_data_t *      packets) {
     int i,n,nn,r;
     uint32_t crc_reg;
     char * c_header= (char *) ogg_header;

     memcpy (ogg_header->OggS,oggs_string,4);
     ogg_header->version=        0;
     ogg_header->serial[0]=      stream_serial & 0xff;
     ogg_header->serial[1]=      (stream_serial>> 8) & 0xff;
     ogg_header->serial[2]=      (stream_serial>>16) & 0xff;
     ogg_header->serial[3]=      (stream_serial>>24) & 0xff;
     ogg_header->flags=          flags;
     ogg_header->granule[0]=     granule & 0xff;
     ogg_header->granule[1]=     (granule>> 8) & 0xff;
     ogg_header->granule[2]=     (granule>>16) & 0xff;
     ogg_header->granule[3]=     (granule>>24) & 0xff;
     ogg_header->granule[4]=     (granule>>32) & 0xff;
     ogg_header->granule[5]=     (granule>>40) & 0xff;
     ogg_header->granule[6]=     (granule>>48) & 0xff;
     ogg_header->granule[7]=     (granule>>56) & 0xff;
     ogg_header->page[0]=        page & 0xff;
     ogg_header->page[1]=        (page>> 8) & 0xff;
     ogg_header->page[2]=        (page>>16) & 0xff;
     ogg_header->page[3]=        (page>>24) & 0xff;
     ogg_header->crc[0]=         0;
     ogg_header->crc[1]=         0;
     ogg_header->crc[2]=         0;
     ogg_header->crc[3]=         0;

// first pass - just fill lacing values/number of segments
     n=0;
     for (i=0;i<num_packets; i++) {
        nn=(packets[i].len/255);
        r= packets[i].len - 255* nn;
        nn+=n;
        if (nn>254)
            return -1;
        for (;n<nn;n++)
            ogg_header->lacing_values[n]=255;
        ogg_header->lacing_values[n++]=r;
     }

     ogg_header->num_segm=       n;
     // second pass - calculate the crc     
     crc_reg=0;
     for (i=0;i<(27+c_header[26]);i++) 
        crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)^c_header[i])&0xff];
     for (i=0;i<num_packets; i++) {
       for (n=0;n<packets[i].len;n++) 
            crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)^packets[i].data[n])&0xff];
     }
     ogg_header->crc[0]=        crc_reg & 0xff;
     ogg_header->crc[1]=        (crc_reg>> 8) & 0xff;
     ogg_header->crc[2]=        (crc_reg>>16) & 0xff;
     ogg_header->crc[3]=        (crc_reg>>24) & 0xff;
     return (27+c_header[26]);
}

// similar to above, but dedicated to the frame data.
// There could be 2 types
// - first page of a frame (packet) then frame header will be attached and bit-combined with the frame
// - continuation  page (flags bit 0 should be set) - then no frame header will be added to the data
// currently there could not be two frames (packets) on one page
int ogg_frame_page      (ogg_header_t *    ogg_header,      // pointer to the header data (returns - header length in bytes)
                            char           flags,           // +1 - continuation, +4 - last (+2 - first)
                            int            maxpage,         // maximal page size (w/o ogg header)
                            uint64_t      granule,         // 
                            uint32_t      stream_serial,   //
                            int            page,            // page
                            len_data_t     frame_header,    // not used if (flags & 1). frame_header.len is measured in bits, not bytes!
                            int          * len,             // pointer frame data length (will modify - if non-zero - need continuation)
                            uint8_t    ** frame_datap)     // pointer to pointer to an array with frame data
                                                            // if it is not continuation data is bit-aligned to be combined with frame_header
                                                            // first word of frame data will be modified - OR-ed with the last one from
                                                            // the frame_header
                   
   {
     int i,l,n,nn,r;
     int dl;
//     unsigned char cmask[4];
//     uint32_t  *mask = (uint32_t *) cmask;
     uint8_t   * frame_data = frame_datap[0];
     uint32_t  mask;
     uint8_t  *cmask= (uint8_t  *) &mask;
     uint32_t  *fh32=  (uint32_t *) frame_header.data;
     uint32_t crc_reg;
     uint32_t  *fd32=  (uint32_t *) frame_data;
     char * c_header= (char *) ogg_header;
     uint64_t  granule0=0;
//quick fix to modify (up to) last 31 bits of header (not first data word)
     int        skip_first_data=0;
D(fprintf(stderr,"ogg_frame_page: flags=%d, page= %d, frame_header.len=%d, len=%d\n",flags, page, frame_header.len, len[0]));
     memcpy (ogg_header->OggS,oggs_string,4);
     ogg_header->version=        0;
     ogg_header->serial[0]=      stream_serial & 0xff;
     ogg_header->serial[1]=      (stream_serial>> 8) & 0xff;
     ogg_header->serial[2]=      (stream_serial>>16) & 0xff;
     ogg_header->serial[3]=      (stream_serial>>24) & 0xff;
     ogg_header->page[0]=        page & 0xff;
     ogg_header->page[1]=        (page>> 8) & 0xff;
     ogg_header->page[2]=        (page>>16) & 0xff;
     ogg_header->page[3]=        (page>>24) & 0xff;
     ogg_header->crc[0]=         0;
     ogg_header->crc[1]=         0;
     ogg_header->crc[2]=         0;
     ogg_header->crc[3]=         0;



    // combine frame_data[0] with the tail of frame_header if needed
     if ((flags & 1) == 0) { //first page in packet
      if ((i=(frame_header.len & 0x1f))) {
        skip_first_data=4;
        n=0;
        mask=0;
        while (i>=8) {cmask[n++]=0xff; i-=8;}
        cmask[n]=(0xff00 >> i) & 0xff;
 D(fprintf(stderr,"ogg_frame_page: 0 frame_header.data[0]=0x%x,frame_header.data[1]=0x%x,frame_header.data[2]=0x%x,frame_header.data[3]=0x%x\n",
 frame_header.data[0],frame_header.data[1],frame_header.data[2],frame_header.data[3]));

D(fprintf(stderr,"ogg_frame_page: 1 mask=0x%x, fd32[0]=0x%x, fh32[frame_header.len>>5]=0x%x\n",mask,fd32[0],fh32[frame_header.len>>5]));
// modifying header, not data (was a problem with mmap)
#if 0
       fd32[0]^= (fd32[0]^fh32[frame_header.len>>5]) & mask;
D(fprintf(stderr,"ogg_frame_page: 2 mask=0x%x, fd32[0]=0x%x, fh32[frame_header.len>>5]=0x%x\n",mask,fd32[0],fh32[frame_header.len>>5]));
#else
       fh32[frame_header.len>>5]^=(fd32[0]^fh32[frame_header.len>>5]) & ~mask;
#endif
      }
     }
     
// see if everything will fit on this page
     l=((flags & 1) == 0)?((frame_header.len>>3)&(~3)):0; // add frame header to the first page in a frame
     if ((l+len[0])> maxpage) {
       nn= maxpage/255;
       dl= 255*nn;
       nn--;
       r=255;
       granule0=~0ULL;
       ogg_header->flags= flags & !3; //remove "last page" flag
D(fprintf(stderr,"*"));
     } else {
       granule0=granule;
D(fprintf(stderr," granule0=0x%llx\n",granule0));
       dl=l+len[0];
       nn=dl/255;
       r= dl-255*nn;
       ogg_header->flags=          flags;
     }
//D(fprintf(stderr," granule0=0x%llx\n",granule0));
     frame_datap[0]+=(dl-l);
     ogg_header->granule[0]=     granule0 & 0xff;
     ogg_header->granule[1]=     (granule0>> 8) & 0xff;
     ogg_header->granule[2]=     (granule0>>16) & 0xff;
     ogg_header->granule[3]=     (granule0>>24) & 0xff;
     ogg_header->granule[4]=     (granule0>>32) & 0xff;
     ogg_header->granule[5]=     (granule0>>40) & 0xff;
     ogg_header->granule[6]=     (granule0>>48) & 0xff;
     ogg_header->granule[7]=     (granule0>>56) & 0xff;


// first pass - just fill lacing values/number of segments
D(fprintf(stderr,"            ogg_frame_page: l=%d, len= %d\n",l, len[0]));
//     if (nn>254) return -1;
     for (n=0;n<nn;n++) ogg_header->lacing_values[n]=255;
     ogg_header->lacing_values[n++]=r;
     ogg_header->num_segm=  n;

// second pass - calculate the crc
     crc_reg=0;
     for (i=0;i<(27+c_header[26]);i++) crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)^c_header[i])&0xff];
     if ((l+skip_first_data)>0) for (n=0;n<(l+skip_first_data);n++) crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)^frame_header.data[n])&0xff];
     for (n=skip_first_data;n<(dl-l);n++)        crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)^frame_data[n])&0xff];
     
D(fprintf(stderr,"            ogg_frame_page: l=%d, (dl-l)= %d, n=0x%x\n",l, (dl-l),n));
D(for (i=0;i<32;i++) fprintf(stderr," %x",frame_data[i]));
D(fprintf(stderr,"\n"));
    
D(for (i=-32;i<0;i++) fprintf(stderr," %x",frame_data[(dl-l)+i]));
D(fprintf(stderr,"\n"));
     
     ogg_header->crc[0]=        crc_reg & 0xff;
     ogg_header->crc[1]=        (crc_reg>> 8) & 0xff;
     ogg_header->crc[2]=        (crc_reg>>16) & 0xff;
     ogg_header->crc[3]=        (crc_reg>>24) & 0xff;
     len[0]-=(dl-l);
     return (27+c_header[26]);
}


//static const char theora_string[6]={0x74,0x68,0x65,0x6f,0x72,0x61};
int theora_identification (char *buf,  // returns length, buf should be big enough - 42 bytes
                           int   fmbw, // width in macroblocks
                           int   fmbh, // height in macroblocks
                           int   frn, // frame rate nominator ( in 1/1000000)
                           int   frd, // frame rate denominator (1000000)
                           int   nobr, // nominal bit rate (-1 - undefined?)
                           int   qual, // relative quality (was 0x10)
                           int   kfgshift) // number of bits in granule used for number of i-frames
   {
     int bp;
     buf[ 0]=0x80;
     memcpy (buf+1,theora_string,6);
     buf[ 7]=0x03;
     buf[ 8]=0x02;
     buf[ 9]=0x00;
     buf[10]=(fmbw >> 8)  & 0xff;
     buf[11]=fmbw &         0xff;
     buf[12]=(fmbh >> 8)  & 0xff;
     buf[13]=fmbh &         0xff;
     buf[14]=(fmbw >> 12) & 0xff;
     buf[15]=(fmbw >>  4) & 0xff;
     buf[16]=(fmbw <<  4) & 0xff;
     
     buf[17]=(fmbh >> 12) & 0xff;
     buf[18]=(fmbh >>  4) & 0xff;
     buf[19]=(fmbh <<  4) & 0xff;
     buf[20]=0x00;
     buf[21]=0x00;
     buf[22]=(frn >> 24) &0xff;
     buf[23]=(frn >> 16) &0xff;
     buf[24]=(frn >>  8) &0xff;
     buf[25]= frn        &0xff;
     buf[26]=(frd >> 24) &0xff;
     buf[27]=(frd >> 16) &0xff;
     buf[28]=(frd >>  8) &0xff;
     buf[29]= frd        &0xff;
     buf[30]= 0; //PARN - 3 bytes
     buf[31]= 0;
     buf[32]= 0;
     buf[33]= 0; //PARD - 3 bytes
     buf[34]= 0;
     buf[35]= 0;
     buf[36]= 0; // color space - "undefined"
     buf[37]=(nobr >> 16) &0xff;
     buf[38]=(nobr >>  8) &0xff;
     buf[39]= nobr        &0xff;
     bp=40 << 3; // bit pointer
     theora_write_bits (buf, &bp, qual,6);
     theora_write_bits (buf, &bp, kfgshift,5);
     theora_write_bits (buf, &bp, 0,2); // pixel format 4:2:0
     theora_write_bits (buf, &bp, 0,3); // reserved bits
     return 42; // length
}
                              
int theora_comments (char        *buf,    // returns length, buf should be big enough - ?? bytes
                     int          buflen,  // buffer size (returns -1 if fails to fit)
                     const char  *vendor, // Null-terminated string (was "Xiph.Org libTheora I 20040317 3 2 0\0")
                     int          nuser,   // number of user comments
                     const char **user) {
     int b,len,i;
     if (buflen<15) return -1;
     buf[ 0]=0x81;
     memcpy (buf+1,theora_string,6);
     b=7;
     len=0; if (vendor) len = strlen(vendor);
     buf[b++]= len        &0xff; // here - little endian
     buf[b++]=(len >>  8) &0xff;
     buf[b++]=(len >> 16) &0xff;
     buf[b++]=(len >> 24) &0xff;
     if (len>0) {
       if (buflen<(b+len+4)) return -1;
       memcpy (&buf[b],vendor,len);
       b+=len;
     }
     buf[b++]= nuser        &0xff; // here - little endian
     buf[b++]=(nuser >>  8) &0xff;
     buf[b++]=(nuser >> 16) &0xff;
     buf[b++]=(nuser >> 24) &0xff;
     for (i=0;i<nuser;i++) {
       len=strlen(user[i]);
       if (buflen<(b+len+4)) return -1;
       buf[b++]= len        &0xff; // here - little endian
       buf[b++]=(len >>  8) &0xff;
       buf[b++]=(len >> 16) &0xff;
       buf[b++]=(len >> 24) &0xff;
       memcpy (&buf[b],user[i],len);
       b+=len;
     }
//  D(fprintf(stderr, "theora_comments b=%d\n",b));
    return b;    
}

int theora_setup (char       *buf,    // returns length, buf should be big enough - ?? bytes
                  int         buflen,  // buffer size (returns -1 if fails to fit)
// the next pointers will likely point to constant tables, the same ones should be used to program FPGA internal tables                     
                  const int  *lflims,  // if null - skip  alltogether, not null - 5 assumed bits/value
                  const int  *acscale, // will encode 10bits/item
                  const int  *dcscale, // will encode 10bits/item
                  const int  bms[][64]    // array of 3 BMS tables
                        ) {
     int bp,i,bmi;
     buf[ 0]=0x82;
     memcpy (buf+1,theora_string,6);
     bp=7<<3;
// Here should go the loop filters - try to skip them alltogether as current FPGAa implementation does not support them
    if (lflims) {
     theora_write_bits (buf, &bp, 5,3); // loop filter values are 5 bit long
     for (i=0;i<64;i++)  theora_write_bits (buf, &bp, lflims[i],5);
    } else theora_write_bits (buf, &bp, 0,3); // loop filter values are 0 length (so all zeros)
// now put ACSCALE table
     theora_write_bits (buf, &bp, 9,4); // ACSCALE table will have 10 bits
     for (i=0;i<64;i++) theora_write_bits (buf, &bp, acscale[i],10);
// now put DCSCALE table
     theora_write_bits (buf, &bp, 9,4); // DCSCALE table will have 10 bits
     for (i=0;i<64;i++) theora_write_bits (buf, &bp, dcscale[i],10);
// BMS - counting on standartd VP3 set of 3 base matrices
     theora_write_bits (buf, &bp, 2,9); // NBMS=3
     for (bmi=0;bmi<3;bmi++) for (i=0;i<64;i++) theora_write_bits (buf, &bp, bms[bmi][i],8);
// next tables:
//NQRS=   {{1,1,1},{1,1,1}}
//QRSIZES={{63},{63},{63},{63},{63},{63}}
//QRBMIS{{{0,0},{1,1},{1,1}},{{2,2},{2,2},{2,2}}}
// Maybe just preencode and put 37 bits?
     theora_write_bits (buf, &bp,  0,2); // QRBMIS[0][0][0]=0
     theora_write_bits (buf, &bp, 62,6); // QRSIZES[0][0][0]=63
     theora_write_bits (buf, &bp,  0,2); // QRBMIS[0][0][1]=0
     theora_write_bits (buf, &bp,  1,1); // NEWQR=1
     theora_write_bits (buf, &bp,  1,2); // QRBMIS[0][1][0]=1
     theora_write_bits (buf, &bp, 62,6); // QRSIZES[0][1][0]=63
     theora_write_bits (buf, &bp,  1,2); // QRBMIS[0][1][1]=1
     theora_write_bits (buf, &bp,  0,1); // NEWQR=0:
                                         //  NQRS[0][2]=NQRS[0][1]=1
                                         //  QRSIZES[0][2]=QRSIZES[0][1]={63}
                                         //  QRBMIS[0][2] = QRBMIS[0][1]={1,1}
     theora_write_bits (buf, &bp,  1,1); // NEWQR=1
     theora_write_bits (buf, &bp,  2,2); // QRBMIS[1][0][0]=2
     theora_write_bits (buf, &bp, 62,6); // QRSIZES[1][0][0]=63
     theora_write_bits (buf, &bp,  2,2); // QRBMIS[1][0][1]=2
     theora_write_bits (buf, &bp,  0,1); // NEWQR=0
     theora_write_bits (buf, &bp,  0,1); // (qti>0), so write RPQR=0
                                         //  NQRS[1][1]=NQRS[1][0]=1
                                         //  QRSIZES[1][1]=QRSIZES[1][0]={63}
                                         //  QRBMIS [1][1]=QRBMIS [1][0]={2,2}
     theora_write_bits (buf, &bp,  0,1); // NEWQR=0
     theora_write_bits (buf, &bp,  0,1); // (qti>0), so write RPQR=0
                                         //  NQRS[2][1]=NQRS[1][1]=1
                                         //  QRSIZES[2][1]=QRSIZES[1][1]={63}
                                         //  QRBMIS [2][1]=QRBMIS [1][1]={2,2}
// next - huffman tables                                          
     if ((i=oc_huff_codes_pack(buf, &bp, OC_VP31_HUFF_CODES))<0) return i;
// D(fprintf(stderr, "setup bits=%d\n",bp));
     return (bp>>3)+((bp & 7)? 1:0);

   }


/*filling out FPGA tables
    table:      the qtable to fill
    inter:      ??
    color:      ??
    qindex_dc:  always the same in a frame
    qindex_ac:  may be different if it is a second qi in a frame
    sharpness:  0..2 - common for all q
    Return: void */
void fill_FPGA_qtable (uint32_t *table,
                         int      inter,
                         int      color,
                         int      qindex_dc, 
                         int      qindex_ac,
                         int      sharpness) {
     int       zbin_index_dc= (sharpness<<1) | (theora_DCSCALE[qindex_dc]>50)?1:0;// index of zero bin/rounding table to use (table is separate)
     int       zbin_index_ac= (sharpness<<1) | (theora_DCSCALE[qindex_ac]>50)?1:0;// index of zero bin/rounding table to use (table is separate)
     int i;
     int bmi=inter?2:(color?1:0);
     
     int mindc= inter? 32 :16;
     int minac= inter? 16 : 8;
     int dqcoef  = (((((theora_BMS[bmi][0] * theora_DCSCALE[qindex_dc])/100)<<2) > mindc)?
         (((theora_BMS[bmi][0] * theora_DCSCALE[qindex_dc])/100)<<2): mindc) & 0xfff;
     int qcoef   = (524288/dqcoef) & 0xffff;
 D(fprintf(stderr,"bmi=%d, qindex_dc=%d, qindex_ac=%d, sharpness=%d, zbin_index_dc=%d, zbin_index_ac=%d\n",bmi, qindex_dc, qindex_ac, sharpness,
             zbin_index_dc,zbin_index_ac));
     table[0]=(zbin_index_dc<<28) | (dqcoef<<16) | qcoef;
//  D(fprintf(stderr,"%x/%x=%x ",dqcoef,(524288/dqcoef),table[0]));

// FPGA Quantization table is transposed (at least for now - wil fix later)
     for (i=1; i<64; i=i+1) {
       dqcoef  = (((((theora_BMS[bmi][i] * theora_ACSCALE[qindex_ac])/100)<<2) > minac)?
        (((theora_BMS[bmi][i] * theora_ACSCALE[qindex_ac])/100)<<2): minac) & 0xfff;
       qcoef   = (524288/dqcoef) & 0xffff;
       table[i]=(zbin_index_ac<<28) | (dqcoef<<16) | qcoef;

//Changed back - transposed in FPGA
//       table[((i>>3) & 0x7) | ((i << 3) & 0x38)]=(zbin_index_ac<<28) | (dqcoef<<16) | qcoef;
//   D(fprintf(stderr,"%x/%x=%x ",dqcoef,(524288/dqcoef),table[i]));
     }
   D(fprintf(stderr,"\n"));
}

/*filling out FPGA tables
    tables:         the qtables to fill
    sharpness:      0..2 - common for all q
    intra_y_qi_hi:  index in DC_Scale/AC_Scale tables, intra frame, luma
    intra_c_qi_hi:  index in DC_Scale/AC_Scale tables, intra frame, chroma
    inter_y_qi_hi:  index in DC_Scale/AC_Scale tables, inter frame, luma
    inter_c_qi_hi:  index in DC_Scale/AC_Scale tables, inter frame, chroma

    intra_y_qi_lo:  index in DC_Scale/AC_Scale tables, intra frame, luma
    intra_c_qi_lo:  index in DC_Scale/AC_Scale tables, intra frame, chroma
    inter_y_qi_lo:  index in DC_Scale/AC_Scale tables, inter frame, luma
    inter_c_qi_lo:  index in DC_Scale/AC_Scale tables, inter frame, chroma

    Return: void */
void fill_FPGA_qtables (
     uint32_t  tables[512],
     int  sharpness,
     int  intra_y_qi_hi, int  intra_c_qi_hi, int  inter_y_qi_hi, int  inter_c_qi_hi,

     int  intra_y_qi_lo, int  intra_c_qi_lo, int  inter_y_qi_lo, int  inter_c_qi_lo) {
       fill_FPGA_qtable (&tables[  0], 0, 0, intra_y_qi_hi, intra_y_qi_hi, sharpness);
       fill_FPGA_qtable (&tables[128], 0, 1, intra_c_qi_hi, intra_c_qi_hi, sharpness);
       fill_FPGA_qtable (&tables[256], 1, 0, inter_y_qi_hi, inter_y_qi_hi, sharpness);
       fill_FPGA_qtable (&tables[384], 1, 1, inter_c_qi_hi, inter_c_qi_hi, sharpness);
       fill_FPGA_qtable (&tables[ 64], 0, 0, intra_y_qi_hi, intra_y_qi_lo, sharpness);
       fill_FPGA_qtable (&tables[192], 0, 1, intra_c_qi_hi, intra_c_qi_lo, sharpness);
       fill_FPGA_qtable (&tables[320], 1, 0, inter_y_qi_hi, inter_y_qi_lo, sharpness);
       fill_FPGA_qtable (&tables[448], 1, 1, inter_c_qi_hi, inter_c_qi_lo, sharpness);
}


// {len[3:0],data[13:0]} - data is MSB aligned
/*filling out FPGA htables
    hti:
    hti30:
    htable:
    pre_ht:
    pre_ht30:
    Return: 0 on success, or a negative value on error. */
int fill_FPGA_htables (uint16_t hti  [12], uint16_t hti30[16],
                       uint32_t htable[1024],
                       int pre_ht[12], int pre_ht30[6]) {
    int i,j,k,l,d;
    for (i=0; i<12; i++) hti[i]=pre_ht30[pre_ht[i]];
    for (i=0; i<16; i++) hti30[i]=7;             // unused
    for (i=0; i< 6; i++) hti30[pre_ht30[i]] = i; // fill just needed
    for (i=0; i< 5; i++) for (j=0;j<6;j++) for (k=0;k<32;k++) {
        l=OC_VP31_HUFF_CODES[(i<<4)+pre_ht30[j]][k].nbits;
        if (l>14) {
            fprintf(stderr,"huffman code too long %d > 14 in table %d, token %d\n",
                  l, (i<<4)+j,k ); 
            return -1;
        }
        // only - up to 14 bits long !!!
        d=OC_VP31_HUFF_CODES[(i<<4)+pre_ht30[j]][k].pattern  << (14-l);  
        htable[i*192 + (j<<5) +k] = (d & 0x3fff) | (l << 14);
     }
     return 0;
}
           

/* fwrites data to stream, until all sent
    stream: file handle to write data to
    data:   pointer to data
    len:    length of data to write
    Return: length of data writen */
int fwrite_all(FILE *  stream, char * data, int len) {
    char *cp= data;
    int   l=0;
    while (len>0) {
        if ((l = fwrite(cp, 1,len, stream))<0) return l;
        len -= l;
        cp  += l;
    }  
    return (cp-data);  
}


/*makes write out an ogg theora file
    ccam_dma_index: will be mmaped array in the camera
    ccam_dma:       will be mmaped array in the camera (char array)
    output_fd:     file handler to write ogg theora stream to
    page_size:      maximal; page size to use
    packets[6]:     0 - identification
                    1 - comment
                    2 - setup
                    3 - frame - intra (.len - in bits, not bytes!)
                    4 - frame - inter/full
                    5 - frame - inter/mapped
    stream_serial:  serial number of ogg stream
    kfgshiftm:      number of bits in granule used for number of i-frames
    Return: 0 on success, negative values on error */
int makeOggTheora(uint32_t * ccam_dma_index, 
                      uint8_t  * ccam_dma, 
                      FILE *      output_fd,
                      int         page_size, 
                      len_data_t  packets[6],
                      uint32_t   stream_serial,
                      int         kfgshiftm) {
    
      ogg_header_t header;
      int          header_length, fw;
      uint64_t    granule=0;
      uint64_t    granule_mask=(1<<kfgshiftm)-1;
      //uint64_t    next_granule=0;
      uint8_t   * dp;
      uint8_t   * dp_prev;
      int          nframe=0;
      int          frame_start=0;   // in bytes
      int          next_frame_start;// in bytes
      int          frame_length;
      //int          frame_length_written;
      int          frame_header_length;   // frame header lengh truncated to 32-bit words (the rest is combined with frame data)
      int          frame_type;      // 0 - intra, 1 - inter/full, 2 - inter/partial
      int          page=0;
      int          last_page_flags;
      int          overlapped_header; // 0/4 - if "4" then bits from the first word of data are already copied to the header
      int          first_frame=1;

    // Ogg Header this should go in its own function: ogg_send_header 
    // first make and send first page
    header_length= ogg_page (&header, 2, 0, stream_serial, page++, 1, &packets[0]);
    if (header_length<0) 
        return header_length-1000;
    if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
        fprintf(stderr,"error writing output data1 - error = %d (%s)\n",
                                                         errno,strerror(errno)); 
        return fw;
    }
    if ((fw= fwrite_all(output_fd, packets[0].data, packets[0].len))<0) {
        fprintf(stderr,"error writing output data2 - error = %d (%s)\n",
                                                         errno,strerror(errno)); 
        return fw;
    }
    //make and send second page with 2 packets
    header_length= ogg_page (&header, 0, 0, stream_serial, page++, 2, &packets[1]);
    if (header_length<0) 
        return header_length-2000;    
    if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
        fprintf(stderr,"error writing output data3 - error = %d (%s)\n",
                                                         errno,strerror(errno)); 
        return fw;
    }
    if ((fw= fwrite_all(output_fd, packets[1].data, packets[1].len))<0) {
        fprintf(stderr,"error writing output data4 - error = %d (%s)\n",
                        errno, strerror(errno)); 
        return fw;
    }
    if ((fw= fwrite_all(output_fd, packets[2].data, packets[2].len))<0) {
        fprintf(stderr,"error writing output data5 - error = %d (%s)\n",
                        errno,strerror(errno)); 
        return fw;
    }
    // Ogg Header End

    // now frames
    // what kind of an abort command is ths?
    while (ccam_dma_index[nframe]!=0xffffffff) {      
        frame_length=(ccam_dma_index[nframe] & 0xfffffff)-frame_start;
        next_frame_start=((ccam_dma_index[nframe] & 0xfffffff)+63) & (~0x1f);
        frame_type=(ccam_dma_index[nframe]>>28) & 3;

        frame_header_length =  ((packets[3+frame_type].len+31)>>3)& (~3);
        overlapped_header   =  (packets[3+frame_type].len & 31)?4:0;
        last_page_flags     =  (ccam_dma_index[nframe+1]==0xffffffff)?4:0;


        /* calculated granulepos */
        if (first_frame) {
            first_frame=0;
        } else if (frame_type==INTRA){
            granule=(granule+ (((granule&granule_mask)+1)<<kfgshiftm)) & (~granule_mask);
        } else {
            granule=(granule & (~granule_mask)) |  ((granule+1) &granule_mask);
        }

        //FIXME: this looks wrong to me j^
        dp      = &ccam_dma[frame_start];
        dp_prev = &ccam_dma[frame_start];

        header_length= ogg_frame_page (&header,                 // [1]
                                        last_page_flags,        // [2]
                                        page_size,              // [3]
                                        granule,
                                        stream_serial,
                                        page++,
                                        packets[3+frame_type],  // [4]
                                        &frame_length,          // [5]
                                        &dp);                   // [6]
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
            return header_length-3000;

        // write to output stream first (or only) page of a frame
        if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
            fprintf(stderr,"error writing output data6 - error = %d (%s)\n",
                            errno,strerror(errno)); 
            return fw;
        }
        if ((fw= fwrite_all(output_fd, packets[3+frame_type].data, frame_header_length))<0) {
            fprintf(stderr,"error writing output data7 - error = %d (%s)\n",
                            errno,strerror(errno)); 
            return fw;
        }
        if ((fw= fwrite_all(output_fd, dp_prev+overlapped_header, (int) (dp-dp_prev-overlapped_header)))<0) {
            fprintf(stderr,"error writing output data8 - error = %d (%s)\n",
                            errno,strerror(errno)); 
            return fw;
        }

        // create more pages if needed
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

            // write to output stream other pages of a frame
            if ((fw= fwrite_all(output_fd, (char *) &header, header_length))<0) {
                fprintf(stderr,"error writing output data9 - error = %d (%s)\n",
                                errno,strerror(errno)); 
                return fw;
            }
            if ((fw= fwrite_all(output_fd, dp_prev, (int) (dp-dp_prev)))<0){
                fprintf(stderr,"error writing output data10 - error = %d (%s)\n",
                                errno,strerror(errno)); 
                return fw;
            }                               
        }
        nframe++;
        frame_start=next_frame_start;
    }
    return 0;
}

/*prepare what?
    headers_filename:
    fpga_mask:
    fpga_zerobin:
    fpga_qtables:
    fpga_hti:
    fpga_hti30:
    fpga_htable:
    fpga_firstbit:
    nstx:           number of supertiles (128x64) horizontal
    nsty:           number of supertiles (128x64) vertical
    frn:            frame rate nominator (1000000=1fps)
    nobr:           (-1 - "undfined")
    qual:           realtive quality =16
    kfgshiftm:
    vendor:         "Xiph.Org libTheora I 20040317 3 2 0"
    nuser:
    user:
    qual_left:
    qual_bottom:
    qual_right:
    qual_top:
    skip_left:
    skip_bottom:
    skip_right:
    skip_top:
    qsi:            0..63 if both are the same, single qi will be encoded
    sharpness:      0..2 - is it obsolete in encodr now?
    pre_ht:
    pre_ht30:
    Return: 0 on success, negative values on error */                           
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
                       int         qis[2], 
                       int         sharpness, // 0..2 - is it obsolete in encodr now?
// table that maps combinbatins of (frame type: 0 - intra, 1  inter whole, 2 - inter, partial (mapped)), AC(/DC) and chroma(/luma)
// to internal (FPGA) indices (1-of-6 - not 1-of-16 as there is not enough room in FPGA Block RAM) 
                       int         pre_ht[12],
// this table maps intenal (1-of-6) Huffman table index to the standard 1-of-16. Not all the tables in the standard set of 80 can work -
// currently code length is limited to 14 bits
                       int         pre_ht30[6]){

//     char       s[1024];
//     char      *cp;
//     char      *ep;
     int        i,d; //j,r;
     int        fmbw=nstx<<3; // width in macroblocks
     int        fmbh=nsty<<2; // height in macroblocks
     const int  frd=1000000;  // frame rate denominator (1000000)
//     uint32_t  kfgshiftm;  // number of bits in granule used for number of i-frames
     char       packet1[44];
     char       packet2[4096];
     char       packet3[4096];
     len_data_t packets[6];
     
     int        width = fmbw << 4;
     int        height= fmbh << 4; 
     len_data_t frame_headers[3];
     int        nqis=(qis[1]==qis[0])?1:2;
     FILE      *ofile;
     const uint32_t FPGA_ZEROBIN_TAB[6]={ // Index Sharpness S.Fact>50 Zbin       Round
                                           0x80a6, //   0       0        0      0.65(166)  0.499(128)
                                           0x76a6, //   1       0        1      0.65(166)  0.460(118)
                                           0x7ac0, //   2       1        0      0.75(192)  0.476(122)
                                           0x66c0, //   3       1        1      0.75(192)  0.400(102)
                                           0x7ae6, //   4       2        0      0.90(230)  0.476(122)
                                           0x55e6};//   5       2        1      0.90(230)  0.333( 85)
     
     memcpy (fpga_zerobin,FPGA_ZEROBIN_TAB,sizeof(FPGA_ZEROBIN_TAB));

     packets[0].data=packet1; packets[0].len=42;
     packets[1].data=packet2; packets[1].len=4096;
     packets[2].data=packet3; packets[2].len=4096;
     packets[0].len=theora_identification (packets[0].data,fmbw,fmbh,frn,frd, nobr, qual, kfgshiftm); //28
     if (packets[1].len < 0) return packets[1].len;
     packets[1].len=theora_comments (packets[1].data, packets[1].len, vendor, nuser, (const char **) user);
     if (packets[1].len < 0) return packets[1].len;
     packets[2].len=theora_setup (packets[2].data,packets[2].len, theora_LFLIMS, theora_ACSCALE, theora_DCSCALE, theora_BMS);
     if (packets[2].len < 0) return packets[2].len;
     fill_fpga_mask_windows (fpga_mask, width, height, qual_left, qual_bottom, qual_right, qual_top,
                             skip_left, skip_bottom, skip_right, skip_top);
     theora_frameheaders (width, height, frame_headers, nqis, qis, fpga_mask); //mask_array);
     for (i=0;i<3;i++)  fpga_firstbit[i] = frame_headers[i].len & 0x1f;
 // three different frame headers
     packets[3].len= frame_headers[0].len;
     packets[3].data=frame_headers[0].data;
     packets[4].len= frame_headers[1].len;
     packets[4].data=frame_headers[1].data;
     packets[5].len= frame_headers[2].len;
     packets[5].data=frame_headers[2].data;
// build quantization tables to be loaded to FPGA
     fill_FPGA_qtables (fpga_qtables,
                        sharpness,
                        qis[0],   // index in DC_Scale/AC_Scale tables, intra frame, luma
                        qis[0],   // index in DC_Scale/AC_Scale tables, intra frame, chroma
                        qis[0],   // index in DC_Scale/AC_Scale tables, inter frame, luma
                        qis[0],   // index in DC_Scale/AC_Scale tables, inter frame, chroma

                        qis[1],   // index in DC_Scale/AC_Scale tables, intra frame, luma
                        qis[1],   // index in DC_Scale/AC_Scale tables, intra frame, chroma
                        qis[1],   // index in DC_Scale/AC_Scale tables, inter frame, luma
                        qis[1]);  // index in DC_Scale/AC_Scale tables, inter frame, chroma
// build Huffman tables to be loaded to FPGA
    if ((i=fill_FPGA_htables (fpga_hti, fpga_hti30, fpga_htable, pre_ht, pre_ht30))<0) return -5;

// now save all 6 packets (3 stream headers and 3 different frame headers) in a file.
// Data will be 32-bit aligned, first word will be 6 (number of packets, then length of each one (in bits!) then packets 32-bit aligned
  
    if ((ofile = fopen(headers_filename, "w"))==NULL)
       {fprintf(stderr,"error opening file %s for writing - error = %d (%s)\n",headers_filename,errno,strerror(errno)); return -1;}
    d=6;   
    if (fwrite(&d, 4, 1, ofile)<1) return -2;
    for (i=0;i<6;i++) {
       d=packets[i].len;
       if (i<3) d<<=3; // make them all in bits (first 3 were in bytes)
       if (fwrite(&d, 4, 1, ofile)<1) return -2;
    }
    if (fwrite(&kfgshiftm, 4, 1, ofile)<1) return -2; // save it for the second part - sending clip out
D(int ii=0;)     
    for (i=0;i<6;i++) {
       d=packets[i].len;
       if (i<3) d<<=3; // make them all in bits (first 3 were in bytes)
       d= (d+31) >> 5;
         
         D(for (ii=0;ii<20;ii++) fprintf(stderr," %x",0xff & packets[i].data[ii]));
         D(fprintf(stderr,"\n"));
       if (fwrite(packets[i].data, 4,d, ofile)<(unsigned)d) return -3;
    }
    fclose (ofile);
    for (i=0; i<3; i++) free(frame_headers[i].data);
    return 0;
 }


/* load headers from file
    headers_filename:   filename of a file with 6 headers and their lengths (in bits)
    packets:            ponter to load headers to.
    Return: 0 on success, negative value on error*/
int load_headers(const char *headers_filename, len_data_t *packets, int *kfgshiftm) {
    FILE      *headers_file;
    uint32_t  num_headers;
    uint32_t  header_lengths[6];
    int        i,r;

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
    if (fread (kfgshiftm,     4, 1, headers_file)<1) 
        return -5;

    for (i=0;i<3;i++) 
        packets[i].len=header_lengths[i]>>3; // now - in bytes
    for (i=3;i<6;i++) 
        packets[i].len=header_lengths[i]; // these are needed to be in bits
    for (i=0;i<6;i++) {
        header_lengths[i]=(header_lengths[i]+31)>>5;
        packets[i].data=malloc(header_lengths[i]<<2);
        if (((r=fread (packets[i].data, 4, header_lengths[i], headers_file))) < (int)header_lengths[i]) {
            fprintf(stderr, "i=%d, fread returned %d, wanted - %d\n",i, r,header_lengths[i]);
            return -6;
        }     
    }
    fclose(headers_file);
    return 0;
}

/* now encapsulate compressed data and output stream (to stdout)
   The two arrays will be just mmap-ed in the camera
    headers_filename:   filename of a file with 6 headers and their lengths (in bits)
    ccam_dma:
    ccam_dma_index:
    stream_serial:
    Return: 0 on success, negative value on error*/
int oggtheora_send    (const char *headers_filename, 
                       uint8_t    ccam_dma      [CCAM_BYTES_PER_DMABUF],
                       uint32_t   ccam_dma_index[CCAM_DMA_INDEX_SIZE],
                       uint32_t   stream_serial) {
    int        kfgshiftm=9;
    len_data_t packets[6];
    int        i,r;

    //load header from file
    if((i=load_headers(headers_filename, packets, &kfgshiftm)) < 0)
        return i;

    D(fprintf(stderr, "oggtheora_send:stream_serial=%x\n",stream_serial));
    i= makeOggTheora( ccam_dma_index, // will be mmaped array in the camera
                      ccam_dma,       // will be mmaped array in the camera (char array)
                      stdout,         // output stream
                      8192,           // page_size,      // maximal page size to use
                      packets,        // headers:
                                      // 0 - identification
                                      // 1 - comment
                                      // 2 - setup
                                      // 3 - frame - intra (.len - in bits, not bytes!)
                                      // 4 - frame - inter/full
                                      // 5 - frame - inter/mapped
                      stream_serial,
                      kfgshiftm);        // number of bits in granule used for number of i-frames
    for (i=0; i<6; i++) 
        free(packets[i].data);
    return i;
}

/* reads file into char *
    fin:
    s:
    Return: char*/
char * fnegets(FILE* fin, char * s) {
    int i;
    char * cp;
    fgets(s, 1024, fin);
    while ((s[(i=strspn(s," \t\n"))]=='#') || (s[i]==0)) 
        fgets(s, 1024, fin);;
        cp = s +strspn(s," \t");
        for (i=strlen(cp)-1;strchr(" \t\n",cp[i]) && (i>0);i--) 
            cp[i]=0;
    return cp;
}

/* setup tbl FPGA
    nframes_total:
    nframes_inter_full:     number of INTER frames (full) after each INTRA
    nframes_inter_masked:   number of INTER frames (masked) after each INTER full 
                            (not tested yet)
    time_slow:              reduce reported fps by this
    Return: 0 on success, or a negative value on error.*/
int  setup_tbl_FPGA( int nframes_total,
             int nframes_inter_full, 
                     int nframes_inter_masked, 
                     int time_slow) {

     char       headers_filename[]="/tmp/headers";
     char       parameters_filename[]="/etc/oggtheora_params";
     char       controlfn[]="/dev/ccam_img";
     char       fpga_tables_fn[]="/dev/fpga_tables";
     char       s[1024];
     char      *cp;
     char      *ep;
     int        i,j; //,r;
     int        nstx; // width  in superblocks
     int        nsty; // height in superblocks
     int        frn; // frame rate nominator ( in 1/1000000)
     int        nobr; // nominal bit rate (-1 - undefined?)
     int        qual; // relative quality (was 0x10)
     int        kfgshiftm=9; // will calculate
     int        nuser = 1;
     char      *vendor = NULL;
     char     **user=NULL;
     
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

     int fpga_tables_fd;
     uint32_t d[2];

     FILE *ifile;
     int devfd;

     i= (nframes_inter_masked+1)*nframes_inter_full; // "period-1" of INTRA frames
     for (kfgshiftm=0;i!=0;i>>=1) kfgshiftm++;
      if  (kfgshiftm<6) kfgshiftm=6;


// read parameters from file, then replace some with current
     if ((ifile = fopen(parameters_filename, "r"))==NULL)
        {fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",parameters_filename,errno,strerror(errno)); return -1;}

     cp=fnegets(ifile,s); nstx=          strtol (cp,&ep,10);
     cp=fnegets(ifile,s); nsty=          strtol (cp,&ep,10);
     cp=fnegets(ifile,s); frn=           strtol (cp,&ep,10);
     cp=fnegets(ifile,s); nobr=          strtol (cp,&ep,10);
     cp=fnegets(ifile,s); qual=          strtol (cp,&ep,10);
     vendor= strdup(fnegets(ifile,s));
     cp=fnegets(ifile,s); nuser=     strtol (cp,&ep,10);
     if (nuser) {
       user= (char**) malloc(nuser * sizeof (cp));
       for (i=0; i<nuser;i++) user[i] = strdup(fnegets(ifile,s));
     }
     cp=fnegets(ifile,s); if (cp[0]!='=') { fprintf(stderr, "expecting separator (=) line\n"); return -3;}
     cp=fnegets(ifile,s); qual_left=     strtol (cp,&ep,10);
     cp=fnegets(ifile,s); qual_bottom=   strtol (cp,&ep,10);
     cp=fnegets(ifile,s); qual_right=    strtol (cp,&ep,10);
     cp=fnegets(ifile,s); qual_top=      strtol (cp,&ep,10);
     cp=fnegets(ifile,s); skip_left=     strtol (cp,&ep,10);
     cp=fnegets(ifile,s); skip_bottom=   strtol (cp,&ep,10);
     cp=fnegets(ifile,s); skip_right=    strtol (cp,&ep,10);
     cp=fnegets(ifile,s); skip_top=      strtol (cp,&ep,10);
     cp=fnegets(ifile,s); nqis=          strtol (cp,&ep,10);
     for (i=0; i<nqis; i++) {cp=fnegets(ifile,s); qis[i]=     strtol (cp,&ep,10);}
     if (nqis<2) qis[1]=qis[0];
     cp=fnegets(ifile,s); if (cp[0]!='=') { fprintf(stderr, "expecting separator (=) line\n"); return -3;}
     cp=fnegets(ifile,s); sharpness=      strtol (cp,&ep,10);
     cp=fnegets(ifile,s); if (cp[0]!='=') { fprintf(stderr, "expecting separator (=) line\n"); return -3;}
     ep=fnegets(ifile,s); for (i=0;i<12;i++) { cp=ep; pre_ht[i]=    strtol (cp,&ep,10);}
     ep=fnegets(ifile,s); for (i=0;i<6; i++) { cp=ep; pre_ht30[i]=  strtol (cp,&ep,10);}
     cp=fnegets(ifile,s); if (cp[0]!='=') { fprintf(stderr, "expecting separator (=) line\n"); return -3;}
     fclose (ifile);

// read frame size, frame rate from sensor
    if ((devfd = open(controlfn, O_RDONLY)) <0)
      {fprintf(stderr,"error opening file %s for reading - error = %d (%s)\n",controlfn,errno,strerror(errno)); return -1;}
     nstx=ioctl(devfd, _CCCMD( CCAM_RPARS , P_ACTUAL_WIDTH), 0)>>7;
     nsty=ioctl(devfd, _CCCMD( CCAM_RPARS , P_ACTUAL_HEIGHT), 0)>>6;
     frn=(ioctl(devfd, _CCCMD( CCAM_RPARS , P_FP100S), 0)*10000)/time_slow;
     qis[1]=qis[0]=(ioctl(devfd, _CCCMD( CCAM_RPARS , P_QUALITY), 0)*63)/100;
     fprintf(stderr,"fps=%d/1000000\r\n",frn);
    close (devfd);
 
// now prepare headers and FPGA tables
     i=oggtheora_prepare (headers_filename, // file to save headers
                       fpga_mask, // [64][16]
                       fpga_zerobin,
                       fpga_qtables,
                       fpga_hti,
                       fpga_hti30,
                       fpga_htable,
                       fpga_firstbit,
                       nstx, //number of supertiles (128x64) horizontal
                       nsty, //number of supertiles (128x64) vertical
                       frn,  // frame rate nominator (1000000=1fps)
                       nobr, // (-1 - "undfined")
                       qual, // realtive quality =16
                       kfgshiftm,
                       vendor, // "Xiph.Org libTheora I 20040317 3 2 0"
                       nuser,
                       user,
// selective qi and skipping blocks is not tested yet
                       qual_left,
                       qual_bottom,
                       qual_right,
                       qual_top,
                       skip_left,
                       skip_bottom,
                       skip_right,
                       skip_top,
                       qis, // 0..63 if both are the same, single qi will be encoded
                       sharpness, // 0..2 - is it obsolete in encodr now?
// table that maps combinbatins of (frame type: 0 - intra, 1  inter whole, 2 - inter, partial (mapped)), AC(/DC) and chroma(/luma)
// to internal (FPGA) indices (1-of-6 - not 1-of-16 as there is not enough room in FPGA Block RAM) 
                       pre_ht,
// this table maps intenal (1-of-6) Huffman table index to the standard 1-of-16. Not all the tables in the standard set of 80 can work -
// currently code length is limited to 14 bits
                       pre_ht30);
// free used stings
     free(vendor);
     if (nuser) {
       for (i=0; i<nuser;i++) free (user[i]);
       free (user);
     }
// write tables to FPGA
  
     fpga_tables_fd = open(fpga_tables_fn, O_RDWR);
     lseek(fpga_tables_fd, 0x400<< 2,0);// 400 - start of zerobin table in fpga
     write(fpga_tables_fd, fpga_zerobin,6<<2);

     lseek(fpga_tables_fd, 0x0  << 2,0);// qtables
     for (i=0;i<512;i++) {
        d[0]=  fpga_qtables[i] & 0xffff;
        d[1]= (fpga_qtables[i] >> 16) & 0xffff;
        write(fpga_tables_fd, d,8);
     }
     
     lseek(fpga_tables_fd, 0x1400  << 2,0);// hti
     for (i=0;i<12;i++)  {
        d[0]=  fpga_hti[i];
        write(fpga_tables_fd, d,4);
     }
    
     lseek(fpga_tables_fd, 0x1500  << 2,0);// hti30
     for (i=0;i<16;i++)  {
        d[0]=  fpga_hti30[i];
        write(fpga_tables_fd, d,4);
     }

     lseek(fpga_tables_fd, 0x1000<< 2,0);// htable
     write(fpga_tables_fd, fpga_htable,1024<<2);


     lseek(fpga_tables_fd, 0x1600  << 2,0);// map
     for (i=0;i<3; i++)  {
        d[0]=  fpga_firstbit[i];
        write(fpga_tables_fd, d,4);
     }

     lseek(fpga_tables_fd, 0xc00 << 2,0);// firstbit
     for (i=0;i<(C333_MAXHEIGHT>>5);i++) for (j=0;j<(C333_MAXWIDTH>>8);j++) {
        d[0]=  fpga_mask[i][j];
        write(fpga_tables_fd, d,4);
     }
     close (fpga_tables_fd);
     
    return 0;
}
