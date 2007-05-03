#include <ogg/ogg.h>
#if !defined(_recode_H)
# define _recode_H (1)
# include "theora/theoradec.h"
# include "theora/theoraenc.h"

#define TH_RECCTL_GET_TOK_NSTATS (0x8000)
#define TH_RECCTL_GET_TOK_STATS  (0x8001)

typedef int                      oc_tok_hist[TH_NDCT_TOKENS];
typedef struct oc_frame_tok_hist oc_frame_tok_hist;
typedef struct th_rec_ctx        th_rec_ctx;



/*The DCT token histograms for a single frame.*/
struct oc_frame_tok_hist{
  oc_tok_hist tok_hist[2][5];
  ogg_int64_t granpos;
  long        pkt_sz;
  long        dct_offs;
  int         ncoded_fragis[3];
};



th_rec_ctx *th_recode_alloc(const th_info *_info,const th_setup_info *_setup);
void th_recode_free(th_rec_ctx *_rec);

int th_recode_packetin(th_rec_ctx *_rec,const ogg_packet *_op,
 ogg_int64_t *_granpos);
int th_recode_ctl(th_rec_ctx *_rec,int _req,void *_buf,size_t _buf_sz);
int th_recode_flushheader(th_rec_ctx *_enc,th_comment *_tc,ogg_packet *_op);
int th_recode_packet_rewrite(th_rec_ctx *_rec,const ogg_packet *_op_in,
 ogg_packet *_op_out);

#endif
