#ifndef __RLE_H
#define __RLE_H

#include <stdio.h>
#include <stdint.h>


#define RLE_HISTOGRAM 1


typedef struct {
   int       bit_count;          /*  number of valid bits in byte    */
   uint8_t   byte;               /*  buffer to save bits             */
   int       byte_count;         /*  number of bytes written         */
   uint8_t  *bitstream;
   size_t    limit;              /*  don't write more bytes to bitstream ... */
} BitCoderState;


typedef struct {
   int mps;                    /*  more probable symbol            */
   int count;                  /*  have seen count+1 mps's         */
   BitCoderState bitcoder;
} RLECoderState;



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

   fprintf (f, "# max. runlength: %u\n", max_runlength);
   for (i=0; i<512; i++)
      fprintf (f, "%i %u\n", i, histogram[i]);
   fclose (f);
#endif

   return bitcoder_flush (&s->bitcoder);
}


static inline
void bit_print (TYPE byte)
{
   int bit = 8*sizeof(TYPE);

   do {
      bit--;
      printf ((byte & (1 << bit)) ? "1" : "0");
   } while (bit);
   printf ("\n");
}

#endif

