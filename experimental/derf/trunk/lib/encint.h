#if !defined(_encint_H)
# define _encint_H (1)
# include "theora/theora.h"
# include "internal.h"
# include "fdct.h"
# include "huffenc.h"

/*This library's version.*/
# define OC_VENDOR_STRING "derf's experimental encoder library " __DATE__

#define OC_NTEST_MODES (1)

/*Fragment information specific to encoding.*/
typedef struct{
  /*The DCT coefficients for each of the various macro block modes tested.*/
  ogg_int16_t     dct_coeffs[OC_NTEST_MODES][64];
  /*The perceptual weight of each coefficient.
    This is an 8.8 fixed point number that should be used to scale the squared
     error of each coefficient.*/
  ogg_int16_t     weights[64];
  /*The importance of this fragment.*/
  float           importance;
}oc_fragment_enc_info;

/*Macro block information specific to encoding.*/
typedef struct{
  /*The neighboring macro blocks with motion vectors available for the current
     frame.*/
  int           cneighbors[4];
  /*The number of current neighbors available.*/
  int           ncneighbors;
  /*The neighboring macro blocks with motion vectors available for the
     previous frame.*/
  int           pneighbors[4];
  /*The number of previous neighbors available.*/
  int           npneighbors;
  /*Motion vectors for a frame against the previous frame and
     against the golden frame.
    Each is a set of 3 vectors, for the co-located block in this
     frame and the previous two frames, which can be used to judge
     constant velocity and constant acceleration.
    Uninitialized MVs are (0,0).*/
  char          mvs[2][3][2];
  /*Minimum motion estimation error from the analysis stage.*/
  int           aerror;
  /*Minimum motion estimation error from the estimation stage.*/
  int           eerror;
}oc_mb_enc_info;

typedef struct oc_impmap_ctx oc_impmap_ctx;
typedef struct oc_mcenc_ctx oc_mcenc_ctx;

typedef struct{
  /*Shared encoder/decoder state.*/
  oc_theora_state       state;
  /*Encoder-specific fragment information.*/
  oc_fragment_enc_info *frinfo;
  /*Encoder-specific macro block information.*/
  oc_mb_enc_info       *mbinfo;
  /*Context information used to perform motion estimation.*/
  oc_mcenc_ctx         *mcenc;
  /*Context information used to generate the importance map.*/
  oc_impmap_ctx        *impmap;
}oc_enc_ctx;

oc_mcenc_ctx *oc_mcenc_alloc(oc_enc_ctx *_enc);
void oc_mcenc_free(oc_mcenc_ctx *_mcenc);
void oc_mcenc_analyze(oc_mcenc_ctx *_mcenc);

oc_impmap_ctx *oc_impmap_alloc(oc_enc_ctx *_enc);
void oc_impmap_free(oc_impmap_ctx *_impmap);
void oc_impmap_fill(oc_impmap_ctx *_impmap,const oc_yuv_buffer _img,
 float _duration);


#endif