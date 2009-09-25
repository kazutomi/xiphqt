#if !defined(_encvbr_H)
# define _encvbr_H (1)
# include "encint.h"



typedef struct oc_impmap_ctx oc_impmap_ctx;
typedef struct oc_psych_ctx  oc_psych_ctx;



/*Context information for the VBR encoder.*/
struct oc_enc_vbr_ctx{
  /*Configuration information.*/
  th_vbr_cfg         cfg;
  /*The main VBR encoder's pipe stage.*/
  oc_enc_pipe_stage  pipe;
  /*The scale factor for the current quality setting.*/
  float              qscale;
  /*Minimum psychovisual tolerance for the DC coefficients in each plane.*/
  unsigned           dc_tol_mins[3];
  /*The estimated bit cost of the current frame.*/
  int                est_bits;
  /*The encode context.*/
  oc_enc_ctx        *enc;
  /*Context information used to generate the importance map.*/
  oc_impmap_ctx     *impmap;
  /*Context information used to generate low-level perceptual weightings.*/
  oc_psych_ctx      *psych;
};


oc_impmap_ctx *oc_impmap_alloc(oc_enc_ctx *_enc);
void oc_impmap_free(oc_impmap_ctx *_impmap);
oc_enc_pipe_stage *oc_impmap_prepend_to_pipe(oc_impmap_ctx *_impmap,
 oc_enc_pipe_stage *_next);

oc_psych_ctx *oc_psych_alloc(oc_enc_ctx *_enc);
void oc_psych_free(oc_psych_ctx *_psych);
oc_enc_pipe_stage *oc_psych_prepend_to_pipe(oc_psych_ctx *_psych,
 oc_enc_pipe_stage *_next);

#endif
