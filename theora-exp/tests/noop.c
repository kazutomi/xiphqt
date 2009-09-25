#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#include "tests.h"

static int
noop_test_encode ()
{
  th_info ti;
  th_enc_ctx th;

  INFO ("+ Initializing th_info struct");
  th_info_init (&ti);

  INFO ("+ Initializing th_state for encoding");
  th = th_encode_alloc(&ti);
  if (th != NULL) {
    INFO ("+ Clearing th_state");
    th_clear (&th);
  }

  INFO ("+ Clearing th_info struct");
  th_info_clear (&ti);

  return 0;
}

static int
noop_test_decode ()
{
  th_info ti;
  th_dec_ctx th;

  INFO ("+ Initializing th_info struct");
  th_info_init (&ti);

  INFO ("+ Initializing th_state for decoding");
  th_decode_init (&th, &ti);

  INFO ("+ Clearing th_state");
  th_clear (&th);

  INFO ("+ Clearing th_info struct");
  th_info_clear (&ti);

  return 0;
}

static int
noop_test_comments ()
{
  th_comment tc;

  th_comment_init (&tc);
  th_comment_clear (&tc);

  return 0;
}

int main(int argc, char *argv[])
{
  /*noop_test_decode ();*/

  noop_test_encode ();

  noop_test_comments ();

  exit (0);
}
