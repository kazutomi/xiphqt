#ifndef __GOLOMB_H
#define __GOLOMB_H


#include "bitcoder.h"


static inline
void write_number_unary (BitCoderState *b, unsigned int x)
{
   while (x--)
      bitcoder_write_bit (b, 1);

   bitcoder_write_bit (b, 0);
}


static inline
unsigned int read_number_unary (BitCoderState *b)
{
   unsigned int x = 0;

   while (bitcoder_read_bit (b))
      x++;

   return x;
}


static inline
void write_number_binary (BitCoderState *b, unsigned int x, int bits)
{
   while (bits) {
      bits--;
      bitcoder_write_bit (b, (x >> bits) & 1);
   }
}


static inline
unsigned int read_number_binary (BitCoderState *b, int bits)
{
   unsigned int x = 0;

   while (bits) {
      bits--;
      x |= bitcoder_read_bit (b) << bits;
   }

   return x;
}


static inline
unsigned int required_bits (unsigned int x)
{
   int bits = 31;

   while ((x & (1 << bits)) == 0 && bits)
      bits--;

   return bits;
}


static inline
void golomb_write_number (BitCoderState *b, unsigned int x, int bits)
{
   unsigned int q, r;

   assert (x > 0);

   q = (x - 1) >> bits;
   r = x - 1 - (q << bits); 

   if (q >= 15) {
      if (x > 0xffff) {
         write_number_unary (b, 16);
         write_number_binary (b, x, 32);
      } else {
         write_number_unary (b, 15);
         write_number_binary (b, x, 16);
      }
   } else {
      write_number_unary (b, q);
      write_number_binary (b, r, bits);
   }
}


static inline
unsigned int golomb_read_number (BitCoderState *b, int bits)
{
   unsigned int q, r, x;

   q = read_number_unary (b);
   if (q == 15)
      x = read_number_binary (b, 16);
   else if (q == 16)
      x = read_number_binary (b, 32);
   else {
      r = read_number_binary (b, bits);
      x = (q << bits) + r + 1;
   }

   return x;
}


typedef struct {
   uint8_t count;
   uint8_t bits;          /* a 5.3 fixed point integer  */
} GolombAdaptiveCoderState;

#define GOLOMB_ADAPTIVE_CODER_STATE_INITIALIZER { 8<<3, 0 }


static const int golomb_w_tab [] = { 256, 128, 64 };




static inline
void golombcoder_encode_number (GolombAdaptiveCoderState *g,
                                BitCoderState *b,
                                unsigned int x)
{
   golomb_write_number (b, x, g->bits >> 3);

   g->bits = ((256 - golomb_w_tab[g->count]) * (int) g->bits +
                     golomb_w_tab[g->count] * ((required_bits(x)<<3) + 4)) / 256;
   g->count++;

   if (g->count > 2)
      g->count = 2;
}


static inline
unsigned int golombcoder_decode_number (GolombAdaptiveCoderState *g,
                                        BitCoderState *b)
{
   unsigned int x;

   x = golomb_read_number (b, g->bits >> 3);

   g->bits = ((256 - golomb_w_tab[g->count]) * g->bits + 
                     golomb_w_tab[g->count] * ((required_bits(x)<<3) + 4)) / 256;
   g->count++;

   if (g->count > 2)
      g->count = 2;

   return x;
}


#endif

