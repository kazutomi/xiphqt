/**
 *   This file implements the ZPCoder, a very nice, fast and efficient binary
 *   entropy coder.
 *
 *   It is a C++ to C port of AT&T's and  Lizardtech's DjVu sample
 *   implementation. The original sources were GPL'd, so Tarkin would have to
 *   get GPL'd too if we use it. This and some open patent issues are a real
 *   problem for a compression codec. As soon we are sure we really want to
 *   use this entropy coder in Tarkin I'll contact Leon Bottou, the author
 *   of the original file to clear these questions.
 *
 *   Until then this file is provided for research and documentation purposes
 *   only. See the copyright notices on djvu.att.com and www.lizardtech.com
 *   for the original terms of distribution.
 */

#ifndef __ZPCODER_H
#define __ZPCODER_H

#include <stdint.h>
#include "bitcoder.h"


typedef uint8_t BitContext;


typedef struct {
   BitCoderState bitcoder;
   int encoding;                // Direction (0=decoding, 1=encoding)
   uint8_t byte;
   uint8_t scount;
   uint8_t delay;
   uint32_t a;
   uint32_t code;
   uint32_t fence;
   uint32_t subend;
   uint32_t buffer;
   uint32_t nrun;
   uint32_t p[256];
   uint32_t m[256];
   BitContext up[256];
   BitContext dn[256];
   int8_t ffzt[256];
} ZPCoderState;





static inline 
void zpcoder_encoder (ZPCoderState *s, int bit, BitContext *ctx) 
{
  uint32_t z = s->a + s->p[*ctx];

  if (bit != (*ctx & 1))
    encode_lps (s, ctx, z);
  else if (z >= 0x8000)
    encode_mps (s, ctx, z);
  else
    a = z;
}


static inline 
int zpcoder_IWdecoder (ZPCoderState *s)
{
  return decode_sub_simple (s, 0, 0x8000 + ((s->a + s->a + s->a) >> 3));
}


static inline
int zpcoder_decoder (ZPCoderState *s, BitContext *ctx)
{
   uint32_t z = s->a + s->p[*ctx];

   if (z <= s->fence) { 
      s->a = z;
      return (*ctx & 1);
   } 

   return decode_sub (s, ctx, z);
}


static inline
void zpcoder_encoder_nolearn (ZPCoderState *s, int bit, BitContext *ctx) 
{
   uint32_t z = s->a + s->p[*ctx];

   if (bit != (*ctx & 1))
      encode_lps_nolearn (s, z);
   else if (z >= 0x8000)
      encode_mps_nolearn (s, z);
   else
      s->a = z;
}

 
static inline
int zpcoder_decoder_nolearn (ZPCoderState *s, BitContext *ctx)
{
   uint32_t z = s->a + s->p[*ctx];

   if (z <= s->fence) {
      s->a = z;
      return (*ctx & 1);
   }

   return decode_sub_nolearn (s, *ctx & 1, z);
}


static inline
void zpcoder_encoder (ZPCoderState *s, int bit)
{
  if (bit)
    encode_lps_simple (s, 0x8000 + (s->a >> 1));
  else 
    encode_mps_simple (s, 0x8000 + (s->a >> 1));
}


static inline
int zpcoder_decoder (ZPCoderState *s)
{
  return decode_sub_simple (s, 0, 0x8000 + (s->a >> 1));
}


static inline
void zpcoder_IWencoder (ZPCoderState *s, const bool bit)
{
  const uint32_t z = 0x8000 + ((s->a + s->a + s->a) >> 3);

  if (bit)
    encode_lps_simple (s, z);
  else
    encode_mps_simple (s, z);
}


#endif

