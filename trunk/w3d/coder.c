
#include <stdio.h>
#include "coder.h"

#define RLE_HISTOGRAM 1


static inline
void bitcoder_write_bit (BitCoderState *s, int bit)
{ 
   s->byte <<= 1;

   if (bit)
      s->byte |= 1;

   s->bit_count++;
   if (s->bit_count == 8 && s->byte_count < s->limit) {
      s->bitstream [s->byte_count++] = s->byte;
      s->bit_count = 0;
   }
}


static inline
int bitcoder_read_bit (BitCoderState *s)
{
   int ret;

   if (s->bit_count == 0 && s->byte_count < s->limit) {
      s->byte = s->bitstream [s->byte_count++];
      s->bit_count = 8;
   }

   ret = (s->byte & 0x80) >> 7;
   s->byte <<= 1; 
   s->bit_count--;

   return ret;
}


static inline
size_t bitcoder_flush (BitCoderState *s)
{
   if (s->bit_count > 0 && s->byte_count < s->limit)
      s->bitstream [s->byte_count++] = s->byte;

printf ("%s: %i bytes written.\n", __FUNCTION__, s->byte_count);
   return s->byte_count;
}


/*
 *   Ugly.
 */
static inline
uint32 huffmancoder_read (BitCoderState *s)
{
   if (bitcoder_read_bit (s) == 0)
      return 0;

   if (bitcoder_read_bit (s) == 0)
      return 1;

   if (bitcoder_read_bit (s) == 0) {
      if (bitcoder_read_bit (s) == 0)
         return 2;
      else
         return 3;
   }

   if (bitcoder_read_bit (s) == 0) {
      if (bitcoder_read_bit (s) == 0)
         return 4;
      else
         return 5;
   }

   if (bitcoder_read_bit (s) == 0) {           /* read 8 bit number */
      uint32 x = 0;
      int i;
      for (i=7; i>=0; i--)
         if (bitcoder_read_bit (s))
            x |= 1 << i;

      return (x + 5);
   } else {                                    /*  read 32 bit number  */
      uint32 x = 0;
      int i;
      for (i=31; i>=0; i--)
         if (bitcoder_read_bit (s))
            x |= 1 << i;

      return (x + 0xff + 5);
   }
}

/*
 *   special handling if (x > 2^32 - 2)  ???
 */
static inline
void huffmancoder_write (BitCoderState *s, uint32 x)
{
   if (x == 0) {
      bitcoder_write_bit (s, 0);
      return;
   }

   bitcoder_write_bit (s, 1);
   if (x == 1) {
      bitcoder_write_bit (s, 0);
      return;
   }

   bitcoder_write_bit (s, 1);
   if (x <= 3) {
      bitcoder_write_bit (s, 0);
      if (x == 2) bitcoder_write_bit (s, 0);
      else        bitcoder_write_bit (s, 1);
      return;
   }
   
   bitcoder_write_bit (s, 1);
   if (x <= 5) {
      bitcoder_write_bit (s, 0);
      if (x == 4) bitcoder_write_bit (s, 0);
      else        bitcoder_write_bit (s, 1);
      return;
   }

   x -= 5;
   bitcoder_write_bit (s, 1);
   if (x <= 0xff) {
      int i;
      bitcoder_write_bit (s, 0);
      for (i=7; i>=0; i--)
         bitcoder_write_bit (s, x & (1 << i));
   } else {
      int i;
      x -= 0xff;
      bitcoder_write_bit (s, 1);
      for (i=31; i>=0; i--)
         bitcoder_write_bit (s, x & (1 << i));
   }
}



#ifdef RLE_HISTOGRAM
uint32 histogram [512];
uint32 max_runlength;
#endif

/*
 *   bit should be 0 or 1 !!!
 */
static inline
void rlecoder_write_bit (RLECoderState *s, int bit)
{
   if (s->mps == -1) {
#ifdef RLE_HISTOGRAM
      memset (histogram, 0, 512*sizeof(uint32));
      max_runlength = 0;
#endif
      s->mps = bit ? 1 : 0;
      s->count = 0;
      huffmancoder_write (&s->bitcoder, bit ? 1 : 0);
   }

   if ((bit & 1) == s->mps) 
      s->count++;
   else {

#ifdef RLE_HISTOGRAM
      if (s->count < 511)
         histogram [s->count-1]++;
      else
         histogram [511]++;
      if (max_runlength < s->count)
         max_runlength = s->count-1;
#endif

      huffmancoder_write (&s->bitcoder, s->count-1);
      s->mps = ~s->mps & 1;
      s->count = 1;
   }
}


static inline
int rlecoder_read_bit (RLECoderState *s)
{
   if (s->mps == -1) {
      s->mps = huffmancoder_read (&s->bitcoder);
      s->count = huffmancoder_read (&s->bitcoder) + 1;
   }

   if (s->count == 0) {
      s->count = huffmancoder_read (&s->bitcoder) + 1;
      s->mps = ~s->mps & 1;
   }
   s->count--;

   return (s->mps);
}




/*
 *  returns the number of valid bytes in 
 */
static inline
size_t rlecoder_done (RLECoderState *s)
{
#ifdef RLE_HISTOGRAM
   FILE *f = fopen ("rle.histogram", "w");
   int i;

   fprintf (f, "# max. runlength: %lu\n", max_runlength);
   for (i=0; i<512; i++)
      fprintf (f, "%i %lu\n", i, histogram[i]);
   fclose (f);
#endif

   return bitcoder_flush (&s->bitcoder);
}




#define OUTPUT_BIT(s,bit)   rlecoder_write_bit(s,bit)
#define INPUT_BIT(s)        rlecoder_read_bit(s)


void bit_print (TYPE byte)
{
   int bit = 8*sizeof(TYPE);

   do {
      bit--;
      printf ((byte & (1 << bit)) ? "1" : "0");
   } while (bit);
   printf ("\n");
}



#define SIGN(x) (((x & (1 << (8*sizeof(TYPE)-1))) >> (8*sizeof(TYPE)-1)))


/**
 *  We encode coefficients bitplane by bitplane. If the current bit needs to be
 *  toggled on receiver side (is significand), we transmit a 1, otherwise 0.
 *  Sign bit is sent after first significand bit.
 *
 *  The bits are written to bitstream until either the complete buffer
 *  is encoded or the bitstream reaches rlecoder.bitcoder.limit bytes.
 *
 */
#define ENCODE(offset,w,h,f)                                                  \
do {                                                                          \
   int i, j, k;                                                               \
                                                                              \
   for (j=0; j<h; j++) {                                                      \
      TYPE *row = waveletbuf->data + offset + j * waveletbuf->width;          \
                                                                              \
      for (k=0; k<f; k++) {                                                   \
         TYPE *frame = row + k * waveletbuf->width * waveletbuf->height;      \
                                                                              \
         for (i=0; i<w; i++) {                                                \
            TYPE coeff = frame [i];                                           \
            TYPE tmp = (coeff & (1 << (8*sizeof(TYPE)-1))) ? ~coeff : coeff;  \
                                                                              \
            if (rlecoder.bitcoder.byte_count >= rlecoder.bitcoder.limit)      \
               return  rlecoder_done (&rlecoder);                             \
                                                                              \
            if (mask == 1 && !(tmp & sent_mask)) {                            \
               OUTPUT_BIT(&rlecoder, tmp & 1);                                \
               OUTPUT_BIT(&rlecoder, SIGN(coeff));                            \
            } else if ((coeff ^ (coeff >> sign_shift)) & mask) {              \
               OUTPUT_BIT(&rlecoder,1);                                       \
                                                                              \
               if (!(tmp & sent_mask))                   /*  send sign bit */ \
                  OUTPUT_BIT(&rlecoder, SIGN(coeff));    /*  when first    */ \
            } else                                       /*  significand   */ \
               OUTPUT_BIT(&rlecoder,0);                  /*  bit is sent   */ \
         }                                                                    \
      }                                                                       \
   }                                                                          \
} while (0)


#define DECODE(offset,w,h,f)                                                  \
do {                                                                          \
   int i, j, k;                                                               \
                                                                              \
   for (j=0; j<h; j++) {                                                      \
      TYPE *row = waveletbuf->data + offset + j * width;                      \
                                                                              \
      for (k=0; k<f; k++) {                                                   \
         TYPE *coeff = row + k * width * height;                              \
                                                                              \
         for (i=0; i<w; i++) {                                                \
            if (coeff[i] & (1 << (sizeof(TYPE)-1)))                           \
               coeff[i] |= ~cmask;                 /* clear round-up bits  */ \
            else                                                              \
               coeff[i] &= cmask;                  /* dito if (coeff >= 0) */ \
                                                                              \
            if (mask == 1 && coeff[i] == 0) {                                 \
               if (INPUT_BIT(&rlecoder))                                      \
                  coeff[i] ^= mask;                                           \
               if (INPUT_BIT(&rlecoder))                                      \
                  coeff[i] = ~coeff[i];                                       \
            } else if (INPUT_BIT(&rlecoder)) {                                \
               if (!(coeff[i]))                /*  first significand bit, */  \
                  if (INPUT_BIT(&rlecoder))    /*  read sign              */  \
                     coeff[i] = ~0;                                           \
                                                                              \
               coeff[i] ^= mask | emask;       /*  toggle current bit     */  \
            }                                  /*  if we received a '1'   */  \
                                               /*  and round up           */  \
                                                                              \
            if (rlecoder.bitcoder.byte_count >= count)                        \
               return;                                                        \
         }                                                                    \
      }                                                                       \
   }                                                                          \
} while (0)



#define  BIT_SHIFT  1



/**
 *  encode waveletbuf until limit bytes are written to
 *  bitstream or complete buffer is encoded.
 *
 *  Bitplanes of child levels are offset'ed by BIT_SHIFT bits. This ensures
 *  a fast transmission of high energy low-frequency coefficients.
 */
size_t encode_coeff3d (Wavelet3DBuf *waveletbuf, uint8 *bitstream, size_t limit)
{
   RLECoderState rlecoder = { -1, 0, { 0, 0, 0, bitstream, limit } };
   int b;

   rlecoder.bitcoder.limit = limit;

   for (b=0; b<8*sizeof(TYPE)-1+BIT_SHIFT*(waveletbuf->scales-1); b++) {
      int level;

      for (level=1;
           level < waveletbuf->scales-1 /*&& b+level*BIT_SHIFT >= 0*/;
           level++)
      {
         int bitplane = 8*sizeof(TYPE)-2 - b + level * BIT_SHIFT;
         TYPE mask = 1 << bitplane;

/*
 *  XXX BUG !!!!!!!!!!   This should be work in level 1, tooo !!!!!!!!!!!!!!!!
 */ 
         if ((mask & (TYPE) ~(1 << (8*sizeof(TYPE) - 1))) &&
             (mask <= waveletbuf->minmax [level+1] || level == 1))
         {
            TYPE sent_mask = (~0 << (bitplane+1)) & ~(1 << (8*sizeof(TYPE)-1));
            int  sign_shift = 8*sizeof(TYPE) - 1 - bitplane;
            int  width = waveletbuf->width;
            int  height = waveletbuf->height;
            int  w = waveletbuf->w [level];
            int  h = waveletbuf->h [level];
            int  f = waveletbuf->f [level];
            int  w1 = waveletbuf->w [level+1] - w;
            int  h1 = waveletbuf->h [level+1] - h;
            int  f1 = waveletbuf->f [level+1] - f;

            if (rlecoder.bitcoder.byte_count >= rlecoder.bitcoder.limit)
               return  rlecoder_done (&rlecoder);

            if (level == 1)        ENCODE (0,w,h,f);
            if (w1 > 0)            ENCODE (w,w1,h,f);
            if (h1 > 0)            ENCODE (h*width,w,h1,f);
            if (f1 > 0)            ENCODE (f*width*height,w,h,f1);
            if (w1 > 0 && h1 > 0)  ENCODE (h*width+w,w1,h1,f);
            if (w1 > 0 && f1 > 0)  ENCODE (f*width*height+w,w,h,f1);
            if (h1 > 0 && f1 > 0)  ENCODE (f*width*height+h*width,w,h1,f1);
            if (w1 > 0 && h1 > 0 && f1 > 0)
                                   ENCODE (f*width*height+h*width+w,w1,h1,f1);
         }
      }
   }
   return  rlecoder_done (&rlecoder);
}



/**
 *  decode count bytes from bitstream to waveletbuf.
 */
void decode_coeff3d (Wavelet3DBuf *waveletbuf, uint8 *bitstream, size_t count)
{
   RLECoderState rlecoder = { -1, 0, { 0, 0, 0, bitstream, count } };
   int width = waveletbuf->width;
   int height = waveletbuf->height;
   int b;

   memset (waveletbuf->data, 0,
           width * height * waveletbuf->frames * sizeof(TYPE));


   for (b=0; b<8*sizeof(TYPE)-1+BIT_SHIFT*(waveletbuf->scales-1); b++) {
      int level;

      for (level=1;
           level < waveletbuf->scales-1 /*&& b+level*BIT_SHIFT >= 0*/;
           level++)
      {
         int bitplane = 8*sizeof(TYPE)-2 - b + level * BIT_SHIFT;
         TYPE mask = 1 << bitplane;
         TYPE cmask = ~0 << (bitplane+1);
         TYPE emask = mask >> 1;


         if ((mask & (TYPE) ~(1 << (8*sizeof(TYPE) - 1))) &&
             (mask <= waveletbuf->minmax [level+1] || level == 1))
         {                                  /*  skip leading empty bitplanes */
            int  w = waveletbuf->w [level];
            int  h = waveletbuf->h [level];
            int  f = waveletbuf->f [level];
            int  w1 = waveletbuf->w [level+1] - w;
            int  h1 = waveletbuf->h [level+1] - h;
            int  f1 = waveletbuf->f [level+1] - f;

            if (rlecoder.bitcoder.byte_count >= rlecoder.bitcoder.limit)
               return;

            if (level == 1)        DECODE (0,w,h,f);
            if (w1 > 0)            DECODE (w,w1,h,f);
            if (h1 > 0)            DECODE (h*width,w,h1,f);
            if (f1 > 0)            DECODE (f*width*height,w,h,f1);
            if (w1 > 0 && h1 > 0)  DECODE (h*width+w,w1,h1,f);
            if (w1 > 0 && f1 > 0)  DECODE (f*width*height+w,w,h,f1);
            if (h1 > 0 && f1 > 0)  DECODE (f*width*height+h*width,w,h1,f1);
            if (w1 > 0 && h1 > 0 && f1 > 0)
                                   DECODE (f*width*height+h*width+w,w1,h1,f1);
         }
      }
   }
}

