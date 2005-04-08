#if !defined(_x86_x86int_H)
# define _x86_x86int_H (1)
# include "../internal.h"

void oc_state_vtable_init_x86(oc_theora_state *_state);

void oc_frag_recon_intra_mmx(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t *_residue);
void oc_frag_recon_inter_mmx(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter2_mmx(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue);
void oc_state_frag_copy_mmx(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli);
void oc_restore_fpu_mmx(void);

#endif
