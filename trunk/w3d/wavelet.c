#include <stdlib.h>
#include "wavelet.h"
#include "rle.h"


/**
 *   (The transform code is in wavelet_xform.c)
 */


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) (MAX(a,MAX(b,c)))



Wavelet3DBuf* wavelet_3d_buf_new (uint32_t width, uint32_t height,
                                  uint32_t frames)
{
   Wavelet3DBuf* buf = (Wavelet3DBuf*) malloc (sizeof (Wavelet3DBuf));
   uint32_t _w = width;
   uint32_t _h = height;
   uint32_t _f = frames;
   int level;

   if (!buf)
      return NULL;

   buf->data = (TYPE*) malloc (width * height * frames * sizeof (TYPE));

   if (!buf->data) {
      wavelet_3d_buf_destroy (buf);
      return NULL;
   }

   buf->width = width;
   buf->height = height;
   buf->frames = frames;
   buf->scales = 1;

   while (_w > 1 || _h > 1 || _f > 1) {
      buf->scales++;
      _w = (_w+1)/2;
      _h = (_h+1)/2;
      _f = (_f+1)/2;
   }

   buf->w = (uint32_t*) malloc (buf->scales * sizeof (uint32_t));
   buf->h = (uint32_t*) malloc (buf->scales * sizeof (uint32_t));
   buf->f = (uint32_t*) malloc (buf->scales * sizeof (uint32_t));
   buf->offset = (uint32_t (*) [8]) malloc (8 * buf->scales * sizeof (uint32_t));

   buf->scratchbuf = (TYPE*) malloc (MAX3(width, height, frames) * sizeof (TYPE));

   if (!buf->w || !buf->h || !buf->f || !buf->offset || !buf->scratchbuf) {
      wavelet_3d_buf_destroy (buf);
      return NULL;
   }

   buf->w [buf->scales-1] = width;
   buf->h [buf->scales-1] = height;
   buf->f [buf->scales-1] = frames;

   for (level=buf->scales-2; level>=0; level--) {
      buf->w [level] = (buf->w [level+1] + 1) / 2;
      buf->h [level] = (buf->h [level+1] + 1) / 2;
      buf->f [level] = (buf->f [level+1] + 1) / 2;
      buf->offset[level][0] = 0;
      buf->offset[level][1] = buf->w [level];
      buf->offset[level][2] = buf->h [level] * width;
      buf->offset[level][3] = buf->f [level] * width * height;
      buf->offset[level][4] = buf->offset [level][2] + buf->w [level];
      buf->offset[level][5] = buf->offset [level][3] + buf->w [level];
      buf->offset[level][6] = buf->offset [level][3] + buf->offset [level][2];
      buf->offset[level][7] = buf->offset [level][6] + buf->w [level];
   }

   return buf;
}


void wavelet_3d_buf_destroy (Wavelet3DBuf* buf)
{
   if (buf) {
      if (buf->data)
         free (buf->data);
      if (buf->w)
         free (buf->w);
      if (buf->h)
         free (buf->h);
      if (buf->f)
         free (buf->f);
      if (buf->offset)
         free (buf->offset);
      if (buf->scratchbuf)
         free (buf->scratchbuf);
      free (buf);
   }
}



#define SIGN_SHIFT 15


static inline
void encode_coeff (ENTROPY_CODER significand_bitstream [],
                   ENTROPY_CODER insignificand_bitstream [],
                   TYPE coeff)
{
   int sign = (coeff >> SIGN_SHIFT) & 1;
   int i = 8;

   do {
      int this_bit = (coeff >> i) & 1;
      int bit_is_significand = sign ^ this_bit;

      if (bit_is_significand) {
         OUTPUT_BIT(&significand_bitstream[i], 1);
         OUTPUT_BIT(&significand_bitstream[i], sign);
         break;
      } else {
         OUTPUT_BIT(&significand_bitstream[i], 0);
         if (i == 0)
            OUTPUT_BIT(&significand_bitstream[i], sign);
      }
   } while (--i >= 0);

   while (--i >= 0)
      OUTPUT_BIT(&insignificand_bitstream[i], ((coeff >> i) ^ sign) & 1);
}

static TYPE coefficient_table [][2] = {
   { 1, ~1 },       //  000000001
   { 2, ~2 },       //  000000010
   { 4, ~4 },       //  000000100
   { 8, ~8 },       //  000001000
   { 16, ~16 },     //  000010000
   { 32, ~32 },     //  000100000
   { 64, ~64 },     //  001000000
   { 128, ~128 },   //  010000000
   { 256, ~256 }    //  100000000
};



static inline
TYPE decode_coeff (ENTROPY_CODER significand_bitstream [],
                   ENTROPY_CODER insignificand_bitstream [])
{
   int i = 8;
   TYPE coeff = 0;

   do {
      if (ENTROPY_CODER_IS_EMPTY(&significand_bitstream[i]))
         continue;

      if (INPUT_BIT(&significand_bitstream[i])) {
         int sign;

         if (ENTROPY_CODER_IS_EMPTY(&significand_bitstream[i]))
            continue;

         sign = INPUT_BIT(&significand_bitstream[i]);
         coeff = coefficient_table [i][sign];
         break;
      } else if (i == 0) {
         if (ENTROPY_CODER_IS_EMPTY(&significand_bitstream[i]))
            continue;

         if (INPUT_BIT(&significand_bitstream[i]))
            coeff = ~0;
      }
   } while (--i >= 0);

   while (--i >= 0) {
      if (ENTROPY_CODER_IS_EMPTY(&insignificand_bitstream[i]))
         continue;

      coeff ^= INPUT_BIT(&insignificand_bitstream[i]) << i;
   }

   return coeff;
}



#if 1

static inline
void encode_quadrant (const Wavelet3DBuf* buf,
                      int level, int quadrant, uint32_t w, uint32_t h, uint32_t f,
                      ENTROPY_CODER significand_bitstream [],
                      ENTROPY_CODER insignificand_bitstream [])
{
   uint32_t x, y, z;

   for (z=0; z<f; z++) {
      for (y=0; y<h; y++) {
         for (x=0; x<w; x++) {
            unsigned int index = buf->offset [level] [quadrant]
                                   + z * buf->width * buf->height
                                   + y * buf->width + x;

            encode_coeff (significand_bitstream, insignificand_bitstream,
                          buf->data [index]);
         }
      }
   }
}


static
void encode_coefficients (const Wavelet3DBuf* buf,
                          ENTROPY_CODER s_stream [],
                          ENTROPY_CODER i_stream [])
{
   int level;

   encode_coeff (s_stream, i_stream, buf->data[0]);

   for (level=0; level<buf->scales-1; level++) {
      uint32_t w, h, f, w1, h1, f1;

      w = buf->w [level];
      h = buf->h [level];
      f = buf->f [level];
      w1 = buf->w [level+1] - w;
      h1 = buf->h [level+1] - h;
      f1 = buf->f [level+1] - f;

      if (w1 > 0) encode_quadrant(buf,level,1,w1,h,f,s_stream,i_stream);
      if (h1 > 0) encode_quadrant(buf,level,2,w,h1,f,s_stream,i_stream);
      if (f1 > 0) encode_quadrant(buf,level,3,w,h,f1,s_stream,i_stream);
      if (w1 > 0 && h1 > 0) encode_quadrant(buf,level,4,w1,h1,f,s_stream,i_stream);
      if (w1 > 0 && f1 > 0) encode_quadrant(buf,level,5,w1,h,f1,s_stream,i_stream);
      if (h1 > 0 && f1 > 0) encode_quadrant(buf,level,6,w,h1,f1,s_stream,i_stream);
      if (h1 > 0 && f1 > 0 && f1 > 0)
         encode_quadrant (buf,level,7,w1,h1,f1,s_stream,i_stream);
   }
}


static inline
void decode_quadrant (Wavelet3DBuf* buf,
                      int level, int quadrant, uint32_t w, uint32_t h, uint32_t f,
                      ENTROPY_CODER significand_bitstream [],
                      ENTROPY_CODER insignificand_bitstream [])
{
   uint32_t x, y, z;

   for (z=0; z<f; z++) {
      for (y=0; y<h; y++) {
         for (x=0; x<w; x++) {
            unsigned int index = buf->offset [level] [quadrant]
                                   + z * buf->width * buf->height
                                   + y * buf->width + x;

            buf->data [index] = decode_coeff (significand_bitstream,
                                              insignificand_bitstream);
         }
      }
   }
}


static
void decode_coefficients (Wavelet3DBuf* buf,
                          ENTROPY_CODER s_stream [],
                          ENTROPY_CODER i_stream [])
{
   int level;

   buf->data[0] = decode_coeff (s_stream, i_stream);

   for (level=0; level<buf->scales-1; level++) {
      uint32_t w, h, f, w1, h1, f1;

      w = buf->w [level];
      h = buf->h [level];
      f = buf->f [level];
      w1 = buf->w [level+1] - w;
      h1 = buf->h [level+1] - h;
      f1 = buf->f [level+1] - f;

      if (w1 > 0) decode_quadrant(buf,level,1,w1,h,f,s_stream,i_stream);
      if (h1 > 0) decode_quadrant(buf,level,2,w,h1,f,s_stream,i_stream);
      if (f1 > 0) decode_quadrant(buf,level,3,w,h,f1,s_stream,i_stream);
      if (w1 > 0 && h1 > 0) decode_quadrant(buf,level,4,w1,h1,f,s_stream,i_stream);
      if (w1 > 0 && f1 > 0) decode_quadrant(buf,level,5,w1,h,f1,s_stream,i_stream);
      if (h1 > 0 && f1 > 0) decode_quadrant(buf,level,6,w,h1,f1,s_stream,i_stream);
      if (h1 > 0 && f1 > 0 && f1 > 0)
         decode_quadrant (buf,level,7,w1,h1,f1,s_stream,i_stream);
   }
}
#else

static inline
void encode_coefficients (const Wavelet3DBuf* buf,
                          ENTROPY_CODER s_stream [],
                          ENTROPY_CODER i_stream [])
{
   uint32_t i;

   for (i=0; i<buf->width*buf->height*buf->frames; i++)
      encode_coeff(s_stream, i_stream, buf->data[i]);
}

static inline
void decode_coefficients (Wavelet3DBuf* buf,
                          ENTROPY_CODER s_stream [],
                          ENTROPY_CODER i_stream [])
{
   uint32_t i;

   for (i=0; i<buf->width*buf->height*buf->frames; i++)
      buf->data[i] = decode_coeff(s_stream, i_stream);
}
#endif


static
uint32_t setup_limittabs (ENTROPY_CODER significand_bitstream [],
                          ENTROPY_CODER insignificand_bitstream [],
                          uint32_t significand_limittab [],
                          uint32_t insignificand_limittab [])
{
   uint32_t byte_count = 0;
   int i;

   for (i=0; i<9; i++) {
      uint32_t bytes = ENTROPY_ENCODER_FLUSH(&significand_bitstream[i]);
bytes=i > 3 ? 200 : 20*i;
      significand_limittab[i] = bytes;
      byte_count += bytes;
   }
   
   for (i=0; i<9; i++) {
      uint32_t bytes = ENTROPY_ENCODER_FLUSH(&insignificand_bitstream[i]);
bytes=i > 5 ? 100 : 10*i;

      insignificand_limittab[i] = bytes;
      byte_count += bytes;
   }

   return byte_count;
}


/**
 *  write 'em binary for now, should be easy to compress ...
 */
static
uint8_t* write_limittabs (uint8_t *bitstream,
                          uint32_t significand_limittab [],
                          uint32_t insignificand_limittab [])
{
   int i;

   for (i=0; i<9; i++) {
      *(uint32_t*) bitstream = significand_limittab[i];
printf("significand_limittab[%i] == %u\n", i, significand_limittab[i]);
      bitstream += 4;
   }

   for (i=0; i<9; i++) {
      *(uint32_t*) bitstream = insignificand_limittab[i];
printf("insignificand_limittab[%i] == %u\n", i, insignificand_limittab[i]);
      bitstream += 4;
   }

   return bitstream;
}


static
uint8_t* read_limittabs (uint8_t *bitstream,
                         uint32_t significand_limittab [],
                         uint32_t insignificand_limittab [])
{
   int i;

   for (i=0; i<9; i++) {
      significand_limittab[i] = *(uint32_t*) bitstream;
//printf("> significand_limittab[%i] == %u\n", i, significand_limittab[i]);
      bitstream += 4;
   }

   for (i=0; i<9; i++) {
      insignificand_limittab[i] = *(uint32_t*) bitstream;
//printf("> insignificand_limittab[%i] == %u\n", i, insignificand_limittab[i]);
      bitstream += 4;
   }
 
   return bitstream;
}


/**
 *  for now we simply concatenate the entropy coder bitstreams
 */
static
void merge_bitstreams (uint8_t *bitstream,
                       ENTROPY_CODER significand_bitstream [],
                       ENTROPY_CODER insignificand_bitstream [],
                       uint32_t significand_limittab [],
                       uint32_t insignificand_limittab [])
{
   int i;

   for (i=0; i<9; i++) {
      memcpy (bitstream,
              ENTROPY_CODER_BITSTREAM(&significand_bitstream[i]),
              significand_limittab[i]);

      bitstream += significand_limittab[i];
   }

   for (i=0; i<9; i++) {
      memcpy (bitstream,
              ENTROPY_CODER_BITSTREAM(&insignificand_bitstream[i]),
              insignificand_limittab[i]);

      bitstream += significand_limittab[i];
   }
}


static
void split_bitstreams (uint8_t *bitstream,
                       ENTROPY_CODER significand_bitstream [],
                       ENTROPY_CODER insignificand_bitstream [],
                       uint32_t significand_limittab [],
                       uint32_t insignificand_limittab [])
{
   uint32_t byte_count;
   int i;

   for (i=0; i<9; i++) {
      byte_count = significand_limittab[i];
      ENTROPY_DECODER_INIT(&significand_bitstream[i], bitstream, byte_count);
      bitstream += byte_count;
   }

   for (i=0; i<9; i++) {
      byte_count = significand_limittab[i];
      ENTROPY_DECODER_INIT(&insignificand_bitstream[i], bitstream, byte_count);
      bitstream += byte_count;
   }
}


int wavelet_3d_buf_encode_coeff (const Wavelet3DBuf* buf,
                                 uint8_t *bitstream,
                                 uint32_t limit)
{
   ENTROPY_CODER significand_bitstream [9];
   ENTROPY_CODER insignificand_bitstream [9];
   uint32_t significand_limittab [9];
   uint32_t insignificand_limittab [9];
   uint32_t byte_count;
   int i;

   for (i=0; i<9; i++) {
      ENTROPY_ENCODER_INIT(&significand_bitstream[i], limit);
      ENTROPY_ENCODER_INIT(&insignificand_bitstream[i], limit);
   }

   encode_coefficients (buf, significand_bitstream, insignificand_bitstream);

   byte_count = setup_limittabs (significand_bitstream,
                                 insignificand_bitstream,
                                 significand_limittab,
                                 insignificand_limittab);

   bitstream = write_limittabs (bitstream,
                                significand_limittab, insignificand_limittab);

   merge_bitstreams (bitstream, significand_bitstream, insignificand_bitstream,
                     significand_limittab, insignificand_limittab);

   for (i=0; i<9; i++) {
      ENTROPY_ENCODER_DONE(&significand_bitstream[i]);
      ENTROPY_ENCODER_DONE(&insignificand_bitstream[i]);
   }

   return byte_count;
}


void wavelet_3d_buf_decode_coeff (Wavelet3DBuf* buf,
                                  uint8_t *bitstream,
                                  uint32_t byte_count)
{
   ENTROPY_CODER significand_bitstream [9];
   ENTROPY_CODER insignificand_bitstream [9];
   uint32_t significand_limittab [9];
   uint32_t insignificand_limittab [9];
   int i;

   for (i=0; i<buf->width*buf->height*buf->frames; i++)
      buf->data[i] = 0xff;

   bitstream = read_limittabs (bitstream,
                               significand_limittab, insignificand_limittab);

   split_bitstreams (bitstream, significand_bitstream, insignificand_bitstream,
                               significand_limittab, insignificand_limittab);

   decode_coefficients (buf, significand_bitstream, insignificand_bitstream);

   for (i=0; i<9; i++) {
      ENTROPY_DECODER_DONE(&significand_bitstream[i]);
      ENTROPY_DECODER_DONE(&insignificand_bitstream[i]);
   }
}

