/**
 *   RLEcoder regression test, Huffman encoder
 */

#define TEST_EQUAL(x,y)                                        \
   if(!(x) == (y)) {                                           \
      fprintf(stderr, "Test ("#x" == "#y") FAILED !!!\n");     \
      fprintf(stderr, "%u: (%u <-> %u)\n", i, x, y);           \
      exit (-1);                                               \
   }

#undef  RLECODER
#undef  BITCODER
#define BITCODER

#include <time.h>
#include "rle.h"



#define MAX_COUNT 10000           /*  write/read up to 65000 values  */
#define N_TESTS   100             /*  test 100 times                 */


int main ()
{
   uint32_t j;

   fprintf(stdout, "\nBitCoder/Huffman Test "
                   "(N_TESTS == %u, bitstreams up to %u values)\n",
                   N_TESTS, MAX_COUNT);

   for (j=1; j<=N_TESTS; j++) {
      uint8_t *bitstream;
      uint32_t *val;
      uint32_t limit = 0;
      uint32_t count;

      fprintf (stdout, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%u/%u", j, N_TESTS);
      fflush (stdout);

      srand (time(NULL));

      while (limit == 0)
         limit = rand() * 1.0 * MAX_COUNT / RAND_MAX;

      val = (uint32_t*) malloc (limit * sizeof(uint32_t));

     /**
      *   check encoding ...
      */
      {
         ENTROPY_CODER encoder;
         uint32_t i;

         ENTROPY_ENCODER_INIT(&encoder, 20*limit);

         for (i=0; i<limit; i++) {
            val[i] = rand() * 1000.0 / RAND_MAX - 128;
            huffmancoder_write(&encoder, val[i]);
         }

         count = ENTROPY_ENCODER_FLUSH(&encoder);

         bitstream = (uint8_t*) malloc (count * sizeof(uint32_t));
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
            uint32_t b = huffmancoder_read(&decoder);
            TEST_EQUAL(val[i], b);
         }
      }

      free (val);
   }

   fprintf (stdout, "\ndone.\n\n");

   return 0;
}

