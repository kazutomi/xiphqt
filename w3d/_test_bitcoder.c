/**
 *   Bitcoder regression test
 */

#define TEST(x)                                       \
   if(!(x)) {                                         \
      fprintf(stderr, "Test ("#x") FAILED !!!\n");    \
      exit (-1);                                      \
   }


#undef BITCODER
#undef RLECODER
#define BITCODER

#include <time.h>
#include "bitcoder.h"



#define MAX_COUNT 650000           /*  write/read up to 650000 bits   */
#define N_TESTS   100              /*  test 100 times                 */


int main ()
{
   uint32_t j;

   fprintf(stdout, "\nBitCoder Test (N_TESTS == %u, bitstreams up to %u bits)\n",
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

      bit = (uint8_t*) malloc (limit);

     /**
      *   check encoding ...
      */
      {
         BitCoderState encoder;
         uint32_t i;

         bitcoder_encoder_init (&encoder, limit);

         for (i=0; i<limit; i++) {
            bit[i] = (rand() > RAND_MAX/2) ? 0 : 1;
            bitcoder_write_bit (&encoder, bit[i]);
         }

         count = bitcoder_flush (&encoder);

         TEST(count == limit/8 || count == limit/8 + 1);

         bitstream = (uint8_t*) malloc (count);
         memcpy (bitstream, encoder.bitstream, count);

         bitcoder_encoder_done (&encoder);
      }

     /**
      *   decoding ...
      */
      {
         BitCoderState decoder;
         uint32_t i;

         bitcoder_decoder_init (&decoder, bitstream, limit);

         for (i=0; i<limit; i++) {
            TEST(bit[i] == bitcoder_read_bit (&decoder));
         }
      }

      free (bit);
   }

   fprintf (stdout, "\ndone.\n\n");

   return 0;
}

