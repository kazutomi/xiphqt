#ifndef __RLE_H
#define __RLE_H

#include <string.h>
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

#endif


#define RLE_HISTOGRAM 1


typedef struct {
   int mps;                      /*  more probable symbol            */
   uint32_t count;               /*  have seen count+1 mps's         */
   BitCoderState bitcoder;
} RLECoderState;



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
      huffmancoder_write (&s->bitcoder, bit ? 1 : 0);
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
      huffmancoder_write (&s->bitcoder, s->count-1);
      s->mps = ~s->mps & 1;
      s->count = 1;
   }
}

static inline
int rlecoder_read_bit (RLECoderState *s)
{
   if (s->count == 0) {
      s->count = huffmancoder_read (&s->bitcoder) + 1;
      s->mps = ~s->mps & 1;
      if (bitcoder_is_empty(&s->bitcoder)) {
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
   huffmancoder_write (&s->bitcoder, s->count-1);

   return bitcoder_flush (&s->bitcoder);
}


static inline
void rlecoder_decoder_init (RLECoderState *s, uint8_t *bitstream, uint32_t limit)
{
   bitcoder_decoder_init (&s->bitcoder, bitstream, limit);
   s->mps = huffmancoder_read (&s->bitcoder);
   s->count = huffmancoder_read (&s->bitcoder) + 1;
   if (bitcoder_is_empty(&s->bitcoder)) {
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

