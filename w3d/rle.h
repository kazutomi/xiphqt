#ifndef __RLE_H
#define __RLE_H

#include <string.h>
#include <assert.h>
#include "mem.h"
#include "bitcoder.h"

#if defined(RLECODER)

#define OUTPUT_BIT(rlecoder,bit)          rlecoder_write_bit(rlecoder,bit)
#define INPUT_BIT(rlecoder)               rlecoder_read_bit(rlecoder)
#define OUTPUT_BIT_DIRECT(coder,bit)      bitcoder_write_bit(&(coder)->bitcoder,bit)
#define INPUT_BIT_DIRECT(rlecoder)        bitcoder_read_bit(&(rlecoder)->bitcoder)
#define ENTROPY_CODER                     RLECoderState
#define ENTROPY_ENCODER_INIT(coder,limit) rlecoder_encoder_init(coder,limit)
#define ENTROPY_ENCODER_DONE(coder)       rlecoder_encoder_done(coder)
#define ENTROPY_ENCODER_FLUSH(coder)      rlecoder_encoder_flush(coder)
#define ENTROPY_DECODER_INIT(coder,bitstream,limit) \
   rlecoder_decoder_init(coder,bitstream,limit)
#define ENTROPY_DECODER_DONE(coder)       /* nothing to do ... */
#define ENTROPY_CODER_BITSTREAM(coder)    ((coder)->bitcoder.bitstream)
#define ENTROPY_CODER_EOS(coder)          ((coder)->bitcoder.eos)

#define ENTROPY_CODER_MPS(coder)          ((coder)->mps)
#define ENTROPY_CODER_RUNLENGTH(coder)    ((coder)->count+1)
#define ENTROPY_CODER_SKIP(coder,skip)    do { (coder)->count -= skip-1; } while (0)
#endif


#define RLE_HISTOGRAM 1



/*
 *   Ugly.
 */
static inline
uint32_t huffmancoder_read (BitCoderState *s)
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
      uint32_t x = 0;
      int i;
      for (i=7; i>=0; i--)
         if (bitcoder_read_bit (s))
            x |= 1 << i;

      return (x + 5);
   } else {                                    /*  read 32 bit number  */
      uint32_t x = 0;
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
void huffmancoder_write (BitCoderState *s, uint32_t x)
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
         bitcoder_write_bit (s, (x >> i) & 1);
   } else {
      int i;
      x -= 0xff;
      bitcoder_write_bit (s, 1);
      for (i=31; i>=0; i--)
         bitcoder_write_bit (s, (x >> i) & 1);
   }
}



static inline
int required_bits (uint32_t x)
{
   int bits = 32;

   assert (x >= 0);

   while (--bits >= 0 && ((x >> bits) & 1) == 0)
      ;

   return bits;
}


static inline
void write_unsigned_number (BitCoderState *s, uint32_t x)
{
   int bits;

   assert (x >= 0);

   bits = required_bits (x);

   huffmancoder_write (s, bits);

   while (--bits >= 0)
      bitcoder_write_bit (s, (x >> bits) & 1);
}

static inline
uint32_t read_unsigned_number (BitCoderState *s)
{
   int bits = huffmancoder_read (s);
   uint32_t x = 1 << bits;

   if (s->eos)
      return ~0;

   while (--bits >= 0) {
      x |= bitcoder_read_bit (s) << bits;
      if (s->eos)
         return ~0;
   }

   return x;
}


typedef struct {
   int mps;                      /*  more probable symbol            */
   uint32_t count;               /*  have seen count mps's           */
   BitCoderState bitcoder;
} RLECoderState;



#ifdef RLE_HISTOGRAM
uint32_t histogram [512];
uint32_t max_runlength;
#endif


/*
 *   bit should be 0 or 1 !!!
 */
static inline
void rlecoder_write_bit (RLECoderState *s, int bit)
{
   if (s->mps == -1) {
#ifdef RLE_HISTOGRAM
      memset (histogram, 0, 512*sizeof(uint32_t));
      max_runlength = 0;
#endif
      s->mps = bit & 1;
      s->count = 0;
      bitcoder_write_bit (&s->bitcoder, bit);
   }

   if (s->mps == bit) {
      s->count++;
   } else {
#ifdef RLE_HISTOGRAM
      if (s->count < 511)
         histogram [s->count-1]++;
      else
         histogram [511]++;
      if (max_runlength < s->count)
         max_runlength = s->count-1;
#endif
      write_unsigned_number (&s->bitcoder, s->count);
      s->mps = ~s->mps & 1;
      s->count = 1;
   }
}

static inline
int rlecoder_read_bit (RLECoderState *s)
{
   if (s->count == 0) {
      s->count = read_unsigned_number (&s->bitcoder);
      s->mps = ~s->mps & 1;
      if (s->bitcoder.eos) {
         s->mps = 0;
         s->count = ~0;
      }
   }
   s->count--;
   return (s->mps);
}


static inline
void rlecoder_encoder_init (RLECoderState *s, uint32_t limit)
{
   bitcoder_encoder_init (&s->bitcoder, limit);
   s->mps = -1;
   s->count = 0;
}


/**
 *  once you called this, you better should not encode any more symbols ...
 */
static inline
uint32_t rlecoder_encoder_flush (RLECoderState *s)
{
   write_unsigned_number (&s->bitcoder, s->count);
   return bitcoder_flush (&s->bitcoder);
}


static inline
void rlecoder_decoder_init (RLECoderState *s, uint8_t *bitstream, uint32_t limit)
{
   bitcoder_decoder_init (&s->bitcoder, bitstream, limit);
   s->mps = bitcoder_read_bit (&s->bitcoder);
   s->count = read_unsigned_number (&s->bitcoder);
   if (s->bitcoder.eos) {
      s->mps = 0;
      s->count = ~0;
   }
}


static inline
void rlecoder_encoder_done (RLECoderState *s)
{
#ifdef RLE_HISTOGRAM
   FILE *f = fopen ("rle.histogram", "w");
   int i;

   fprintf (f, "# max. runlength: %u\n", max_runlength);
   for (i=0; i<512; i++)
      fprintf (f, "%i %u\n", i, histogram[i]);
   fclose (f);
#endif
   bitcoder_encoder_done (&s->bitcoder);
}


#endif

