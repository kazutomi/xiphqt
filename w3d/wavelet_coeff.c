#include "mem.h"
#include "wavelet.h"
#include "rle.h"



#define SIGN_SHIFT 15


static inline
void encode_coeff (ENTROPY_CODER significand_bitstream [],
                   ENTROPY_CODER insignificand_bitstream [],
                   TYPE coeff)
{
   TYPE mask [2] = { 0, ~0 };
   int sign = (coeff >> SIGN_SHIFT) & 1;
   TYPE significance = coeff ^ mask[sign];
   int i = 10;

   do {
      i--;
      OUTPUT_BIT(&significand_bitstream[i], (significance >> i) & 1);
   } while (!((significance >> i) & 1) && i > 0);

   OUTPUT_BIT(&significand_bitstream[i], sign);

   while (--i >= 0)
      OUTPUT_BIT(&insignificand_bitstream[i], (significance >> i) & 1);
}


static inline
TYPE decode_coeff (ENTROPY_CODER significand_bitstream [],
                   ENTROPY_CODER insignificand_bitstream [])
{
   TYPE mask [2] = { 0, ~0 };
   TYPE significance;
   int sign;
   int i = 10;

   do {
      i--;
      significance = INPUT_BIT(&significand_bitstream[i]) << i;
   } while (!significance && i > 0);

   sign = INPUT_BIT(&significand_bitstream[i]);

   while (--i >= 0) {
      significance |= INPUT_BIT(&insignificand_bitstream[i]) << i;
   };

   return (significance ^ mask[sign]);
}




static inline
uint32_t skip_0coeffs (Wavelet3DBuf* buf,
                       ENTROPY_CODER s_stream [],
                       ENTROPY_CODER i_stream [],
                       uint32_t limit)
{
   int i;
   uint32_t min = limit;

   for (i=0; i<10; i++) {
      if (ENTROPY_CODER_MPS(&s_stream[i]) == 0) {
         uint32_t runlength = ENTROPY_CODER_RUNLENGTH(&s_stream[i]);
         if (min > runlength)
            min = runlength;
         if (min <= 2)
            return 0;
      } else {
         return 0;
      }
   }

   if (min > limit)
      min = limit;

   for (i=0; i<10; i++) {
      ENTROPY_CODER_SKIP(&s_stream[i], min);
      ENTROPY_CODER_SKIP(&i_stream[i], min);
   }

   return min;
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
                      ENTROPY_CODER s_stream [],
                      ENTROPY_CODER i_stream [])
{
   uint32_t x, y, z;

   z = 0;
   do {
      y = 0;
      do {
         x = 0;
         do {
            uint32_t skip;
            uint32_t index = buf->offset [level] [quadrant]
                               + z * buf->width * buf->height
                               + y * buf->width + x;

            buf->data [index] = decode_coeff (s_stream, i_stream);

            skip = skip_0coeffs (buf, s_stream, i_stream,
                                 (w-x-1)+(h-y-1)*w+(f-z-1)*w*h);
            if (skip > 0) {
               x += skip;
               while (x >= w) {
                  y++; x -= w;
                  while (y >= h) {
                     z++; y -= h;
                     if (z >= f)
                        return;
                  }
               }
            } else
               x++;
         } while (x < w);
         y++;
      } while (y < h);
      z++;
   } while (z < f);
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

static
void encode_coefficients (const Wavelet3DBuf* buf,
                          ENTROPY_CODER s_stream [],
                          ENTROPY_CODER i_stream [])
{
   uint32_t i;

   for (i=0; i<buf->width*buf->height*buf->frames; i++)
      encode_coeff(s_stream, i_stream, buf->data[i]);
}




static
void decode_coefficients (Wavelet3DBuf* buf,
                          ENTROPY_CODER s_stream [],
                          ENTROPY_CODER i_stream [])
{
   uint32_t i = 0;

   while (i < buf->width*buf->height*buf->frames) {
      int skip;

      buf->data[i] = decode_coeff(s_stream, i_stream);

      skip = skip_0coeffs (buf, s_stream, i_stream,
                           buf->width*buf->height*buf->frames - i);
      if (skip > 0)
         i += skip;
      else
         i++;
   }
}
#endif


static
uint32_t insignificand_truncation_table [10] = {
//   1, 2, 4, 8, 16, 32, 64, 128, 256, 512
   100, 100, 100, 100, 100, 100, 100, 100, 100, 100
};


static
uint32_t significand_truncation_table [9] = { //1, 2, 4, 8, 16, 32, 64, 128, 256 };
   100, 100, 100, 100, 100, 100, 100, 100, 100
};


static
uint32_t setup_limittabs (ENTROPY_CODER significand_bitstream [],
                          ENTROPY_CODER insignificand_bitstream [],
                          uint32_t significand_limittab [],
                          uint32_t insignificand_limittab [],
                          uint32_t limit)
{
   uint32_t byte_count = 0;
   int i;

printf ("%s: limit == %u\n", __FUNCTION__, limit);
   limit -= 2 * 10 * sizeof(uint32_t);  /* 2 limittabs, coded binary */
printf ("%s: rem. limit == %u\n", __FUNCTION__, limit);

   for (i=0; i<10; i++) {
      uint32_t bytes = ENTROPY_ENCODER_FLUSH(&insignificand_bitstream[i]);

      insignificand_limittab[i] =
                          limit * insignificand_truncation_table[i] / 2048;

      if (bytes < insignificand_limittab[i])
         insignificand_limittab[i] = bytes;
printf ("insignificand_limittab[%i]  == %u\n", i, insignificand_limittab[i]);
      byte_count += insignificand_limittab[i];
   }

   for (i=9; i>0; i--) {
      uint32_t bytes = ENTROPY_ENCODER_FLUSH(&significand_bitstream[i]);

      significand_limittab[i] = limit * significand_truncation_table[9-i] / 2048;

      if (bytes < significand_limittab[i])
         significand_limittab[i] = bytes;
printf ("significand_limittab[%i]  == %u\n", i, significand_limittab[i]);
      byte_count += significand_limittab[i];
   }

   significand_limittab[0] = limit - byte_count;
   byte_count += significand_limittab[0];

printf ("significand_limittab[%i]  == %u\n", 0, significand_limittab[0]);
printf ("byte_count == %u\n", byte_count);
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

   for (i=0; i<10; i++) {
      *(uint32_t*) bitstream = significand_limittab[i];
      bitstream += 4;
   }

   for (i=0; i<10; i++) {
      *(uint32_t*) bitstream = insignificand_limittab[i];
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

   for (i=0; i<10; i++) {
      significand_limittab[i] = *(uint32_t*) bitstream;
printf ("significand_limittab[%i]  == %u\n", i, significand_limittab[i]);
      bitstream += 4;
   }

   for (i=0; i<10; i++) {
      insignificand_limittab[i] = *(uint32_t*) bitstream;
printf ("insignificand_limittab[%i]  == %u\n", i, insignificand_limittab[i]);
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

   for (i=9; i>=0; i--) {
      memcpy (bitstream,
              ENTROPY_CODER_BITSTREAM(&significand_bitstream[i]),
              significand_limittab[i]);

      bitstream += significand_limittab[i];
   }

   for (i=9; i>=0; i--) {
      memcpy (bitstream,
              ENTROPY_CODER_BITSTREAM(&insignificand_bitstream[i]),
              insignificand_limittab[i]);

      bitstream += insignificand_limittab[i];
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

   for (i=9; i>=0; i--) {
      byte_count = significand_limittab[i];
      ENTROPY_DECODER_INIT(&significand_bitstream[i], bitstream, byte_count);
      bitstream += byte_count;
   }

   for (i=9; i>=0; i--) {
      byte_count = insignificand_limittab[i];
      ENTROPY_DECODER_INIT(&insignificand_bitstream[i], bitstream, byte_count);
      bitstream += byte_count;
   }
}


int wavelet_3d_buf_encode_coeff (const Wavelet3DBuf* buf,
                                 uint8_t *bitstream,
                                 uint32_t limit)
{
   ENTROPY_CODER significand_bitstream [10];
   ENTROPY_CODER insignificand_bitstream [10];
   uint32_t significand_limittab [10];
   uint32_t insignificand_limittab [10];
   uint32_t byte_count;
   int i;

   for (i=0; i<10; i++) {
      ENTROPY_ENCODER_INIT(&significand_bitstream[i], limit);
      ENTROPY_ENCODER_INIT(&insignificand_bitstream[i], limit);
   }

   encode_coefficients (buf, significand_bitstream, insignificand_bitstream);

   byte_count = setup_limittabs (significand_bitstream, insignificand_bitstream,
                                 significand_limittab, insignificand_limittab,
                                 limit);

   bitstream = write_limittabs (bitstream,
                                significand_limittab, insignificand_limittab);

   merge_bitstreams (bitstream, significand_bitstream, insignificand_bitstream,
                     significand_limittab, insignificand_limittab);

   for (i=0; i<10; i++) {
      ENTROPY_ENCODER_DONE(&significand_bitstream[i]);
      ENTROPY_ENCODER_DONE(&insignificand_bitstream[i]);
   }

   return byte_count;
}


void wavelet_3d_buf_decode_coeff (Wavelet3DBuf* buf,
                                  uint8_t *bitstream,
                                  uint32_t byte_count)
{
   ENTROPY_CODER significand_bitstream [10];
   ENTROPY_CODER insignificand_bitstream [10];
   uint32_t significand_limittab [10];
   uint32_t insignificand_limittab [10];
   int i;

   memset (buf->data, 0,
           buf->width * buf->height * buf->frames * sizeof(TYPE));

   bitstream = read_limittabs (bitstream,
                               significand_limittab, insignificand_limittab);

   split_bitstreams (bitstream, significand_bitstream, insignificand_bitstream,
                               significand_limittab, insignificand_limittab);

   decode_coefficients (buf, significand_bitstream, insignificand_bitstream);

   for (i=0; i<10; i++) {
      ENTROPY_DECODER_DONE(&significand_bitstream[i]);
      ENTROPY_DECODER_DONE(&insignificand_bitstream[i]);
   }
}



#if defined(DBG_XFORM)

#include "pnm.h"

void wavelet_3d_buf_dump (char *fmt,
                          uint32_t first_frame_in_buf,
                          uint32_t id,
                          Wavelet3DBuf* buf)
{
   char fname [256];
   uint32_t f;

   for (f=0; f<buf->frames; f++) {
      snprintf (fname, 256, fmt, id, first_frame_in_buf + f);

      write_pgm16 (fname, buf->data + f * buf->width * buf->height,
                   buf->width, buf->height);
   }
}
#endif

