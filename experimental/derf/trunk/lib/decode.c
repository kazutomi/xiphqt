#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "internal.h"
#include "idct.h"

/*Determines the number of blocks or coefficients to be skipped for a given
   token value.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: A positive value indicates that number of coefficients are to be
           skipped in the current block.
          Otherwise, the negative of the return value indicates that number of
           blocks are to be ended.*/
typedef (*oc_token_skip_func)(int _token,int _extra_bits);

static int oc_token_skip_eob(int _token,int _extra_bits){
  static const int NBLOCKS_ADJUST[OC_NDCT_EOB_TOKEN_MAX]={1,2,3,4,8,16,0};
  return -_extra_bits-NBLOCKS_ADJUST[_token];
}

static int oc_token_skip_zrl(int _token,int _extra_bits){
  return _extra_bits+1;
}

static int oc_token_skip_val(void){
  return 1;
}

static int oc_token_skip_run_cat1a(int _token){
  return _token-OC_DCT_RUN_CAT1A+1;
}

static int oc_token_skip_run(int _token,int _extra_bits){
  static const int NCOEFFS_ADJUST[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    6,10,1,2
  };
  static const int NCOEFFS_MASK[OC_NDCT_RUN_MAX-OC_DCT_RUN_CAT1B]={
    3,7,0,1
  };
  _token-=OC_DCT_RUN_CAT1B;
  return (_extra_bits&NCOEFFS_MASK[_token])+NCOEFFS_ADJUST[_token];
}

/*A jump table for computing the number of coefficients or blocks to skip for
   a given token value.
  This reduces all the conditional branches, etc., needed to parse these token
   values down to one indirect jump.*/
static const oc_token_skip_func OC_TOKEN_SKIP_TABLE[OC_NDCT_TOKENS]={
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_eob,
  oc_token_skip_zrl,
  oc_token_skip_zrl,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_val,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  (oc_token_skip_func)oc_token_skip_run_cat1a,
  oc_token_skip_run,
  oc_token_skip_run,
  oc_token_skip_run,
  oc_token_skip_run
};

static int oc_dec_token_skip(int _token,int _extra_bits){
  return (*OC_TOKEN_SKIP_TABLE[_token])(_token,_extra_bits);
}
