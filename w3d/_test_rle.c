/**
 *   RLEcoder regression test
 */

#define TEST(x)                                       \
   if(!(x)) {                                         \
      fprintf(stderr, "Test ("#x") FAILED (i == %i) !!!\n", i);    \
      exit (-1);                                      \
   }

#undef  BITCODER
#undef  RLECODER
#define RLECODER

#include <time.h>
#include "rle.h"
#include "mem.h"
#include "mem.c"      /*  FIXME !!  */



#define MAX_COUNT 650000        /*  write/read up to 650000 bits   */
#define N_TESTS   100           /*  test 100 times                 */


int main ()
{
   uint32_t j;

   fprintf(stdout, "\nRLECoder Test (N_TESTS == %u, bitstreams up to %u bits)\n",
           N_TESTS, MAX_COUNT);

   for (j=1; j<=N_TESTS; j++) {
      uint8_t *bitstream;
      uint8_t *bit;
      uint32_t limit = 0;
      uint32_t count;

      fprintf (stdout, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%u/%u", j, N_TESTS);
      fflush (stdout);

      srand (time(NULL));

      while (limit == 0)
         limit = rand() * 1.0 * MAX_COUNT / RAND_MAX;

      bit = (uint8_t*) MALLOC (limit);

     /**
      *   check encoding ...
      */
      {
         ENTROPY_CODER encoder;
         uint32_t i;

         ENTROPY_ENCODER_INIT(&encoder, limit);

         for (i=0; i<limit; i++) {
            bit[i] = (rand() > RAND_MAX/1000) ? 0 : 1;  /* avg. runlength 1000 */
            OUTPUT_BIT(&encoder, bit[i]);
         }

         count = ENTROPY_ENCODER_FLUSH(&encoder);

         bitstream = (uint8_t*) MALLOC (count);
         memcpy (bitstream, ENTROPY_CODER_BITSTREAM(&encoder), count);

         ENTROPY_ENCODER_DONE(&encoder);
      }

     /**
      *   decoding ...
      */
      {
         ENTROPY_CODER decoder;
         uint32_t i;

         ENTROPY_DECODER_INIT(&decoder, bitstream, count);

         for (i=0; i<limit; i++) {
            int b;
            int skip;

            b = INPUT_BIT(&decoder);
            TEST(bit[i] == b);

            skip = ENTROPY_CODER_RUNLENGTH(&decoder);
            if (skip > 0 && ENTROPY_CODER_SYMBOL(&decoder) == 0) {
               int j;
               for (j=0; j<skip; j++)
                  TEST(bit[i+j] == 0);
               ENTROPY_CODER_SKIP(&decoder, skip);
               i += skip;
            }
         }
      }

      FREE (bit);
      FREE (bitstream);
   }

   fprintf (stdout, "\ndone.\n\n");

   return 0;
}

