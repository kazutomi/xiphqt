/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2006                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: example rehuff application; optimizes the Huffman codes
   for Theora streams.
  last mod: $Id: rehuff.c,v 1.2 2004/03/24 19:12:42 derf Exp $

 ********************************************************************/

#if !defined(_REENTRANT)
#define _REENTRANT
#endif
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_LARGEFILE_SOURCE)
#define _LARGEFILE_SOURCE
#endif
#if !defined(_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE
#endif
#if !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
/*Yes, yes, we're going to hell.*/
#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif
#include "getopt.h"
#include "../lib/recode.h"
#include <vorbis/codec.h>



#define OC_MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#define OC_MAX(_a,_b) ((_a)>(_b)?(_a):(_b))
/*The ANSI offsetof macro is broken on some platforms (e.g., older DECs).*/
#define _ogg_offsetof(_type,_field)\
 ((size_t)((char *)&((_type *)0)->_field-(char *)0))



typedef struct oc_huff_entry oc_huff_entry;



struct oc_huff_entry{
  oc_huff_entry *next;
  oc_huff_entry *children[2];
  ogg_int64_t    freq;
  int            token;
  int            min_token;
};



/*Fills in an array of Huffman codes given a Huffman tree.
  _entry:   The root of the current branch of the tree.
  _codes:   The array in which to store the codes.
  _pattern: The prefix required to reach the current branch of the tree.
  _nbits:   The number of bits in the prefix.*/
static void huff_codes_create(oc_huff_entry *_entry,
 th_huff_code _codes[TH_NDCT_TOKENS],int _pattern,int _nbits){
  if(_entry->children[0]==NULL&&_entry->children[1]==NULL){
    th_huff_code *code;
    code=_codes+_entry->token;
    code->pattern=_pattern;
    code->nbits=_nbits;
  }
  else{
    _nbits++;
    huff_codes_create(_entry->children[0],_codes,_pattern<<1,_nbits);
    huff_codes_create(_entry->children[1],_codes,_pattern<<1|1,_nbits);
  }
}

/*Inserts an element into a singly linked list of Huffman tree nodes.
  The element is inserted in ascending order by frequency.
  _root:  The start of the list.
  _entry: The entry to insert.
  Return: The new head of the list. */
static oc_huff_entry *huff_list_ins(oc_huff_entry *_root,
 oc_huff_entry *_entry){
  oc_huff_entry **pnext;
  oc_huff_entry  *search;
  for(pnext=&_root,search=_root;search!=NULL&&search->freq<_entry->freq;
   pnext=&search->next,search=search->next);
  _entry->next=search;
  *pnext=_entry;
  return _root;
}

/*Creates a list of Huffman tree nodes for the given frequency table, sorted in
   ascending order.
  _freqs: A list of frequency counts.
          All counts less than 1 will be upgraded to 1.
  Return: A pointer to the first tree node in the list.
          At this point, the nodes just form a list, and are not arranged in a
           single tree.*/
static oc_huff_entry *huff_list_create(oc_huff_entry _entries[TH_NDCT_TOKENS],
 const ogg_int64_t _freqs[TH_NDCT_TOKENS]){
  oc_huff_entry *root;
  int            ti;
  root=NULL;
  for(ti=0;ti<TH_NDCT_TOKENS;ti++){
    oc_huff_entry *entry;
    /*Create a new entry for this value.*/
    entry=_entries+ti;
    entry->children[0]=entry->children[1]=NULL;
    entry->token=ti;
    entry->min_token=ti;
    entry->freq=_freqs[ti]<<5;
    if(entry->freq<=0)entry->freq=1;
    root=huff_list_ins(root,entry);
  }
  return root;
}

/*Builds a complete Huffman code for a given frequency table.
  _codes: The array to store the Huffman codes in.
  _freqs: The frequency table to build the tree from.*/
static void huff_code_build(th_huff_code _codes[TH_NDCT_TOKENS],
 const ogg_int64_t _freqs[TH_NDCT_TOKENS]){
  oc_huff_entry  entries[(TH_NDCT_TOKENS<<1)-1];
  oc_huff_entry *root;
  oc_huff_entry *next;
  /*Create an initial sorted list of what will become the leaf nodes of the
     tree.*/
  root=huff_list_create(entries,_freqs);
  next=entries+TH_NDCT_TOKENS;
  /*Merge pairs of nodes until the tree is complete.*/
  while(root->next!=NULL){
    oc_huff_entry *entry;
    entry=next++;
    entry->children[0]=root;
    entry->children[1]=root->next;
    /*Sort the codes so that the branch containing the smaller minimum token
       value is on the left.
      This ensures that a bitstream of all 0's decodes to an EOB token, which
       is useful if a packet gets truncated.*/
    if(root->next->min_token<root->min_token){
      entry->children[0]=root->next;
      entry->children[1]=root;
    }
    entry->min_token=entry->children[0]->min_token;
    entry->token=-1;
    entry->freq=root->freq+root->next->freq;
    root=huff_list_ins(root->next->next,entry);
  }
  /*Create the Huffman codes corresponding to the generated tree.*/
  huff_codes_create(root,_codes,0,0);
}

/*Builds a complete Huffman code for a given frequency table.
  _codes: The array to store the Huffman codes in.
  _freqs: The frequency table to build the tree from.*/
static void huff_code_buildi(th_huff_code _codes[TH_NDCT_TOKENS],
 const int _freqs[TH_NDCT_TOKENS]){
  ogg_int64_t freqs[TH_NDCT_TOKENS];
  int         ti;
  for(ti=0;ti<TH_NDCT_TOKENS;ti++)freqs[ti]=_freqs[ti];
  huff_code_build(_codes,freqs);
}

/*Prints C code for an array of Huffman codes.*/
static void huff_codes_print(const char *_symbol,
 th_huff_code _codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS]){
  int maxlen;
  int ti;
  int tj;
  maxlen=0;
  printf("th_huff_code %s[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS]={",_symbol);
  for(ti=0;ti<TH_NHUFFMAN_TABLES;ti++){
    for(tj=0;tj<TH_NDCT_TOKENS;tj++){
      if(_codes[ti][tj].nbits>maxlen)maxlen=_codes[ti][tj].nbits;
    }
  }
  maxlen=maxlen+3>>2;
  for(ti=0;ti<TH_NHUFFMAN_TABLES;ti++){
    if(ti>0)printf(",");
    printf("\n  {");
    for(tj=0;tj<TH_NDCT_TOKENS;tj++){
      if(tj>0)printf(",");
      if((tj&3)==0)printf("\n    ");
      printf("{0x%0*X,%2i}",maxlen,_codes[ti][tj].pattern,_codes[ti][tj].nbits);
    }
    printf("\n  }");
  }
  printf("\n};\n");
}



/*Farthest point clustering and k-means clustering code based on work
   originally by Nathan E. Egge.*/

/*Compute the total number of tokens in a frequency count table.*/
int count_tokens(const int *_freqs,int _nfreqs){
  int ti;
  int c;
  c=0;
  for(ti=0;ti<_nfreqs;ti++)c+=_freqs[ti];
  return c;
}

/*Compute the quantized cross entropy of a given vector by counting the number
   of bits it takes to encode it with the Huffman table from another vector.*/
int count_bits(const int *_freqs,int _nfreqs,const th_huff_code *_codes){
  int ti;
  int c;
  c=0;
  for(ti=0;ti<_nfreqs;ti++)c+=_freqs[ti]*_codes[ti].nbits;
  return c;
}

typedef struct oc_tok_vec oc_tok_vec;

struct oc_tok_vec{
  const int *freqs;
  int        entropy;
  int        dist;
  int        idx;
};

#define NDC_TOKENS  (TH_NDCT_TOKENS)
#define NAC_TOKENS  (TH_NDCT_TOKENS<<2)
#define NTOKENS_MAX (OC_MAX(NDC_TOKENS,NAC_TOKENS))

void oc_tok_vec_init(oc_tok_vec *_vec,const int *_freqs,int _nfreqs){
  th_huff_code codes[NTOKENS_MAX];
  int          ti;
  _vec->freqs=_freqs;
  for(ti=0;ti<_nfreqs;ti+=TH_NDCT_TOKENS)huff_code_buildi(codes+ti,_freqs+ti);
  _vec->entropy=count_bits(_freqs,_nfreqs,codes);
  _vec->dist=INT_MAX;
  _vec->idx=-1;
}

/*Farthest point clustering.
  At each step, a new cluster center is added that is the vector that is the
   farthest from any of the existing cluster centers.
  Return: The total number of wasted bits.*/
ogg_int64_t tok_vecs_fpc(oc_tok_vec *_vecs,long _nvecs,int _nfreqs,
 int _cis[16]){
  ogg_int64_t ret;
  long        vi;
  int         best_ntoks;
  int         best_vi;
  int         ntoks;
  int         ti;
  int         cii;
  /*Initialize the first cluster as the frame with the most tokens.*/
  best_ntoks=best_vi=-1;
  for(vi=0;vi<_nvecs;vi++){
    ntoks=count_tokens(_vecs[vi].freqs,_nfreqs);
    if(ntoks>best_ntoks){
      best_ntoks=ntoks;
      best_vi=vi;
    }
  }
  for(cii=0;cii<16;cii++){
    int          dist;
    int          max_dist;
    int          max_ntoks;
    th_huff_code codes[NTOKENS_MAX];
    _cis[cii]=best_vi;
    /*Build the optimal Huffman code for the new cluster center.*/
    for(ti=0;ti<_nfreqs;ti+=TH_NDCT_TOKENS){
      huff_code_buildi(codes+ti,_vecs[best_vi].freqs+ti);
    }
    /*Update cluster membership to see if the new cluster center is closer.*/
    max_dist=0;
    max_ntoks=0;
    ret=0;
    for(vi=0;vi<_nvecs;vi++){
      dist=count_bits(_vecs[vi].freqs,_nfreqs,codes)-_vecs[vi].entropy;
      if(dist<=_vecs[vi].dist){
        _vecs[vi].dist=dist;
        _vecs[vi].idx=cii;
      }
      else dist=_vecs[vi].dist;
      ret+=dist;
      ntoks=count_tokens(_vecs[vi].freqs,_nfreqs);
      /*The distances are inverse weighted by the number of tokens, so that we
         are measuring the average number of bits wasted, not the total.*/
      if(dist*(ogg_int64_t)max_ntoks>=max_dist*(ogg_int64_t)ntoks){
        max_dist=dist;
        max_ntoks=ntoks;
        best_vi=vi;
      }
    }
  }
  fprintf(stderr,"After FPC: %lli wasted bits.\n",ret);
  return ret;
}

/*A single iteration of K-means clustering.
  New cluster centers are computed from the current cluster members, and then
   the numbers are redistributed to belong to the new closest clusters.
  Return: The total number of wasted bits.*/
ogg_int64_t tok_vecs_kmeans(oc_tok_vec *_vecs,long _nvecs,int _nfreqs,
 th_huff_code _codes[16][NTOKENS_MAX],int *_converged){
  ogg_int64_t  freqs[16][NTOKENS_MAX];
  ogg_int64_t  ret;
  int          converged;
  long         vi;
  int          ti;
  int          ci;
  /*Gather the frequency statistics for all the clusters.*/
  memset(freqs,0,sizeof(freqs));
  for(vi=0;vi<_nvecs;vi++)for(ti=0;ti<_nfreqs;ti++){
    freqs[_vecs[vi].idx][ti]+=_vecs[vi].freqs[ti];
  }
  /*Build the new Huffman codes for these clusters.*/
  for(ci=0;ci<16;ci++)for(ti=0;ti<_nfreqs;ti+=TH_NDCT_TOKENS){
    huff_code_build(_codes[ci]+ti,freqs[ci]+ti);
  }
  /*Re-assign points into the new clusters.*/
  converged=1;
  ret=0;
  for(vi=0;vi<_nvecs;vi++){
    int old_idx;
    old_idx=_vecs[vi].idx;
    _vecs[vi].dist=INT_MAX;
    _vecs[vi].idx=0;
    for(ci=0;ci<16;ci++){
      int dist;
      dist=count_bits(_vecs[vi].freqs,_nfreqs,_codes[ci])-_vecs[vi].entropy;
      if(dist<=_vecs[vi].dist){
        _vecs[vi].dist=dist;
        _vecs[vi].idx=ci;
      }
    }
    if(_vecs[vi].idx!=old_idx)converged=0;
    ret+=_vecs[vi].dist;
  }
  fprintf(stderr,"After K-means: %lli wasted bits.\n",ret);
  *_converged=converged;
  return ret;
}

/*Initialize a set of token vectors from the given frame statistics.*/
long tok_vecs_init(oc_tok_vec **_dc_vecs,oc_tok_vec **_ac_vecs,
 const oc_frame_tok_hist *_tok_hists,long _ntok_hists){
  oc_tok_vec *dc_vecs;
  oc_tok_vec *ac_vecs;
  long        vi;
  long        fi;
  dc_vecs=(oc_tok_vec *)_ogg_malloc((_ntok_hists<<1)*sizeof(*dc_vecs));
  ac_vecs=(oc_tok_vec *)_ogg_malloc((_ntok_hists<<1)*sizeof(*ac_vecs));
  for(fi=vi=0;fi<_ntok_hists;fi++){
    oc_tok_vec_init(dc_vecs+vi,_tok_hists[fi].tok_hist[0][0],NDC_TOKENS);
    oc_tok_vec_init(ac_vecs+vi,_tok_hists[fi].tok_hist[0][1],NAC_TOKENS);
    vi++;
    oc_tok_vec_init(dc_vecs+vi,_tok_hists[fi].tok_hist[1][0],NDC_TOKENS);
    oc_tok_vec_init(ac_vecs+vi,_tok_hists[fi].tok_hist[1][1],NAC_TOKENS);
    vi++;
  }
  *_dc_vecs=dc_vecs;
  *_ac_vecs=ac_vecs;
  return vi;
}



typedef struct th_rehuff_ctx      th_rehuff_ctx;
typedef struct page_queue         page_queue;
typedef struct ov_passthrough_ctx ov_passthrough_ctx;

struct th_rehuff_ctx{
  ogg_stream_state  to;
  ogg_stream_state  tp;
  ogg_page          og;
  unsigned char    *page_data;
  int               cpage_data;
  long              serialno;
  int               processing_headers;
  int               page_ready;
  th_info           ti;
  th_comment        tc;
  th_setup_info    *ts;
  th_rec_ctx       *tr;
};

static void th_rehuff_init(th_rehuff_ctx *_ctx,const ogg_stream_state *_to,
 const th_info *_ti,int _processing_headers){
  /*Copy the preliminary data used to parse the BOS packet.*/
  memcpy(&_ctx->to,_to,sizeof(_ctx->to));
  _ctx->serialno=_to->serialno;
  _ctx->processing_headers=_processing_headers;
  memcpy(&_ctx->ti,_ti,sizeof(_ctx->ti));
  /*Init supporting Theora structures needed in header parsing */
  th_comment_init(&_ctx->tc);
  _ctx->ts=NULL;
  _ctx->page_ready=0;
  _ctx->page_data=NULL;
  _ctx->cpage_data=0;
  ogg_stream_init(&_ctx->tp,_ctx->serialno);
}

static void th_rehuff_clear(th_rehuff_ctx *_ctx){
  ogg_stream_clear(&_ctx->to);
  ogg_stream_clear(&_ctx->tp);
  _ogg_free(_ctx->page_data);
  th_info_clear(&_ctx->ti);
  th_comment_clear(&_ctx->tc);
  th_setup_free(_ctx->ts);
  th_recode_free(_ctx->tr);
}

static void th_rehuff_copy_page_data(th_rehuff_ctx *_ctx){
  int npage_data;
  npage_data=_ctx->og.header_len+_ctx->og.body_len;
  if(_ctx->cpage_data<npage_data){
    _ctx->page_data=(unsigned char *)_ogg_realloc(_ctx->page_data,npage_data);
    _ctx->cpage_data=npage_data;
  }
  memcpy(_ctx->page_data,_ctx->og.header,_ctx->og.header_len);
  memcpy(_ctx->page_data+_ctx->og.header_len,_ctx->og.body,_ctx->og.body_len);
  _ctx->og.header=_ctx->page_data;
  _ctx->og.body=_ctx->page_data+_ctx->og.header_len;
}

static int th_rehuff_pagein(th_rehuff_ctx *_ctx,ogg_page *_og){
  ogg_packet op;
  int        ret;
  ret=ogg_stream_pagein(&_ctx->to,_og);
  if(ret<0)return ret;
  /*Recode all the packets available.*/
  while(ogg_stream_packetout(&_ctx->to,&op)>0){
    int err;
    if(!th_packet_isheader(&op)){
     ogg_packet oq;
     err=th_recode_packet_rewrite(_ctx->tr,&op,&oq);
     if(err<0){
       ret=err;
       break;
     }
     ogg_stream_packetin(&_ctx->tp,&oq);
    }
  }
  /*TODO: Also flush packets after a certain elapsed time.*/
  if(!_ctx->page_ready){
    _ctx->page_ready=ogg_stream_pageout(&_ctx->tp,&_ctx->og)>0;
    if(_ctx->page_ready)th_rehuff_copy_page_data(_ctx);
  }
  return ret;
}

static double th_rehuff_pagetime(th_rehuff_ctx *_ctx,int _flush){
  if(!_ctx->page_ready){
    if(!_flush)return -2;
    _ctx->page_ready=ogg_stream_flush(&_ctx->tp,&_ctx->og)>0;
    if(_ctx->page_ready)th_rehuff_copy_page_data(_ctx);
    else return -2;
  }
  return th_granule_time(_ctx->tr,ogg_page_granulepos(&_ctx->og));
}

static size_t th_rehuff_writepage(th_rehuff_ctx *_ctx,FILE *_out){
  size_t ret;
  ret=fwrite(_ctx->og.header,1,_ctx->og.header_len,_out)+
   fwrite(_ctx->og.body,1,_ctx->og.body_len,_out);
  if(_ctx->processing_headers){
    _ctx->page_ready=ogg_stream_flush(&_ctx->tp,&_ctx->og)>0;
    if(_ctx->page_ready)th_rehuff_copy_page_data(_ctx);
    else _ctx->processing_headers=0;
  }
  else{
    _ctx->page_ready=ogg_stream_pageout(&_ctx->tp,&_ctx->og)>0;
    if(_ctx->page_ready)th_rehuff_copy_page_data(_ctx);
  }
  return ret;
}

struct page_queue{
  ogg_page       page;
  page_queue    *next;
  page_queue    *prev;
  unsigned char  data[1];
};

struct ov_passthrough_ctx{
  vorbis_info       vi;
  vorbis_dsp_state  vd;
  long              serialno;
  page_queue       *head;
  page_queue       *tail;
};

static void ov_passthrough_init(ov_passthrough_ctx *_ctx,
 const vorbis_info *_vi,long _serialno);
static void ov_passthrough_clear(ov_passthrough_ctx *_ctx);
static int ov_passthrough_queuepage(ov_passthrough_ctx *_ctx,ogg_page *_og);
static page_queue *ov_passthrough_dequeuepage(ov_passthrough_ctx *_ctx);

static void ov_passthrough_init(ov_passthrough_ctx *_ctx,
 const vorbis_info *_vi,long _serialno){
  memcpy(&_ctx->vi,_vi,sizeof(_ctx->vi));
  vorbis_analysis_init(&_ctx->vd,&_ctx->vi);
  _ctx->serialno=_serialno;
  _ctx->head=_ctx->tail=NULL;
}

static void ov_passthrough_clear(ov_passthrough_ctx *_ctx){
  while(_ctx->tail!=NULL)_ogg_free(ov_passthrough_dequeuepage(_ctx));
  vorbis_dsp_clear(&_ctx->vd);
  vorbis_info_clear(&_ctx->vi);
}

static int ov_passthrough_queuepage(ov_passthrough_ctx *_ctx,ogg_page *_og){
  page_queue *oq;
  /*Only queue pages that belong to our stream.*/
  if(ogg_page_serialno(_og)!=_ctx->serialno)return 0;
  /*The page data gets trampled when the next page is read from the stream, so
     make a copy of it.*/
  oq=(page_queue *)_ogg_malloc(
   _ogg_offsetof(page_queue,data)+_og->header_len+_og->body_len);
  memcpy(&oq->page,_og,sizeof(oq->page));
  oq->page.header=oq->data;
  memcpy(oq->page.header,_og->header,_og->header_len);
  oq->page.body=oq->data+_og->header_len;
  memcpy(oq->page.body,_og->body,_og->body_len);
  oq->next=_ctx->head;
  oq->prev=NULL;
  if(_ctx->tail==NULL)_ctx->tail=oq;
  else oq->next->prev=oq;
  _ctx->head=oq;
  return 1;
}

static page_queue *ov_passthrough_dequeuepage(ov_passthrough_ctx *_ctx){
  page_queue *oq;
  oq=_ctx->tail;
  _ctx->tail=oq->prev;
  if(_ctx->tail==NULL)_ctx->head=NULL;
  else oq->prev->next=NULL;
  return oq;
}

static double ov_passthrough_pagetime(ov_passthrough_ctx *_ctx,int _flush){
  if(_ctx->tail==NULL)return -2;
  return vorbis_granule_time(&_ctx->vd,ogg_page_granulepos(&_ctx->tail->page));
}

static size_t ov_passthrough_writepage(ov_passthrough_ctx *_ctx,FILE *_out){
  page_queue *oq;
  size_t      ret;
  oq=ov_passthrough_dequeuepage(_ctx);
  ret=fwrite(oq->data,1,oq->page.header_len+oq->page.body_len,_out);
  _ogg_free(oq);
  return ret;
}

typedef int    (*stream_pagein_func)(void *_stream,ogg_page *_og);
typedef double (*stream_pagetime_func)(void *_stream,int _flush);
typedef size_t (*stream_writepage_func)(void *_stream,FILE *_out);
typedef struct stream_ctx_vtbl stream_ctx_vtbl;
typedef struct stream_ctx      stream_ctx;

struct stream_ctx_vtbl{
  stream_pagein_func    pagein;
  stream_pagetime_func  pagetime;
  stream_writepage_func writepage;
};

struct stream_ctx{
  const stream_ctx_vtbl *vtbl;
  void                  *ctx;
};

static void write_pages(stream_ctx *_sos,int _nsos,int _flush,FILE *_out){
  double min_time;
  int    min_si;
  int    nready;
  int    si;
  for(;;){
    min_time=DBL_MAX;
    min_si=-1;
    nready=0;
    for(si=0;si<_nsos;si++){
      double time;
      /*Write out any header pages.
        Technically checking for time!=0 is not a reliable check if a page is
         not a header page, but because we process pages in the order they
         appear in the input, in this case it works out even if the check
         fails.*/
      for(;;){
        time=(*_sos[si].vtbl->pagetime)(_sos[si].ctx,_flush);
        /*Write any header or intermediate pages immediately.*/
        if(time!=0&&time!=-1)break;
        (*_sos[si].vtbl->writepage)(_sos[si].ctx,_out);
      }
      if(time>0){
        if(time<min_time){
          min_time=time;
          min_si=si;
        }
        nready++;
      }
    }
    /*If everyone has a data page, or we are flushing the last pages and
       there's at least one, write out the earliest.*/
    if(nready==_nsos||_flush&&nready>0){
      (*_sos[min_si].vtbl->writepage)(_sos[min_si].ctx,_out);
    }
    else break;
  }
}

static const stream_ctx_vtbl STREAM_STATE_VTBL={
  (stream_pagein_func)ogg_stream_pagein,
  NULL,
  NULL
};

static const stream_ctx_vtbl TH_REHUFF_VTBL={
  (stream_pagein_func)th_rehuff_pagein,
  (stream_pagetime_func)th_rehuff_pagetime,
  (stream_writepage_func)th_rehuff_writepage
};

static const stream_ctx_vtbl OV_PASSTHROUGH_VTBL={
  (stream_pagein_func)ov_passthrough_queuepage,
  (stream_pagetime_func)ov_passthrough_pagetime,
  (stream_writepage_func)ov_passthrough_writepage
};

static void stream_ctx_add(stream_ctx **_sos,int *_nsos,int *_csos,
 const stream_ctx_vtbl *_vtbl,void *_ctx){
  stream_ctx *sos;
  int         nsos;
  int         csos;
  sos=*_sos;
  nsos=*_nsos;
  csos=*_csos;
  if(nsos>=csos){
    csos=csos<<1|1;
    sos=(stream_ctx *)_ogg_realloc(sos,csos*sizeof(*sos));
  }
  sos[nsos].vtbl=_vtbl;
  sos[nsos].ctx=_ctx;
  nsos++;
  *_sos=sos;
  *_nsos=nsos;
  *_csos=csos;
}

const char *optstring="o:s:";

const struct option options[]={
  {"output",required_argument,NULL,'o'},
  {"output-stats",required_argument,NULL,'s'},
  /*{"input-stats",required_argument,NULL,'S'},*/
  {NULL,0,NULL,0}
};



/*Grab some more compressed bitstream and sync it for page extraction.*/
size_t buffer_data(FILE *_fin,ogg_sync_state *_oy){
  char   *buf;
  size_t  bytes;
  buf=ogg_sync_buffer(_oy,4096);
  bytes=fread(buf,1,4096,_fin);
  ogg_sync_wrote(_oy,(long)bytes);
  return bytes;
}

/*Push a page into the appropriate steam.*/
static int queue_page(stream_ctx *_sos,int _nsos,ogg_page *_og){
  int si;
  /*This can be done blindly; a stream won't accept a page that doesn't belong
     to it.*/
  for(si=0;si<_nsos;si++){
    if(!(*_sos[si].vtbl->pagein)(_sos[si].ctx,_og))break;
  }
  return 0;
}

static void usage(void){
  fprintf(stderr,"Usage: rehuff [-s <statsout.txt> ] "
   "<infile.ogg> <outfile.ogg>\n");
  exit(-1);
}

static void oc_process_bos0(ogg_page *_og,th_rehuff_ctx **_rehuffs,
 int *_nrehuffs,int *_crehuffs,stream_ctx **_sos,int *_nsos,int *_csos){
  th_rehuff_ctx    *rehuffs;
  int               nrehuffs;
  int               crehuffs;
  ogg_stream_state  test;
  ogg_packet        op;
  th_info           ti;
  int               theorap;
  rehuffs=*_rehuffs;
  nrehuffs=*_nrehuffs;
  crehuffs=*_crehuffs;
  ogg_stream_init(&test,ogg_page_serialno(_og));
  ogg_stream_pagein(&test,_og);
  /*If we don't get a packet back, something is wrong.*/
  if(!ogg_stream_packetpeek(&test,&op)){
    fprintf(stderr,
     "Warning: BOS page encountered without a complete packet.\n");
    ogg_stream_clear(&test);
    return;
  }
  /*Identify the codec: try Theora.*/
  th_info_init(&ti);
  theorap=th_decode_headerin(&ti,NULL,NULL,&op);
  if(theorap>=0){
    /*It is Theora.*/
    if(nrehuffs>=crehuffs){
      int ri;
      crehuffs=crehuffs<<1|1;
      rehuffs=(th_rehuff_ctx *)_ogg_realloc(rehuffs,
       crehuffs*sizeof(*rehuffs));
      for(ri=0;ri<nrehuffs;ri++)(*_sos)[ri].ctx=&rehuffs[ri].to;
    }
    th_rehuff_init(rehuffs+nrehuffs,&test,&ti,theorap);
    /*Advance past the successfully processed header.*/
    ogg_stream_packetout(&rehuffs[nrehuffs].to,NULL);
    stream_ctx_add(_sos,_nsos,_csos,&STREAM_STATE_VTBL,&rehuffs[nrehuffs].to);
    nrehuffs++;
  }
  else{
    /*Whatever it is, we don't care about it.*/
    th_info_clear(&ti);
    ogg_stream_clear(&test);
  }
  *_rehuffs=rehuffs;
  *_nrehuffs=nrehuffs;
  *_crehuffs=crehuffs;
}

static void oc_process_bos1(ogg_page *_og,th_rehuff_ctx *_rehuffs,
 int _nrehuffs,ov_passthrough_ctx **_passthroughs,int *_npassthroughs,
 int *_cpassthroughs,stream_ctx **_sos,int *_nsos,int *_csos){
  ov_passthrough_ctx *passthroughs;
  int                 npassthroughs;
  int                 cpassthroughs;
  ogg_stream_state    test;
  ogg_packet          op;
  th_info             ti;
  int                 theorap;
  int                 vorbisp;
  int                 ri;
  passthroughs=*_passthroughs;
  npassthroughs=*_npassthroughs;
  cpassthroughs=*_cpassthroughs;
  ogg_stream_init(&test,ogg_page_serialno(_og));
  ogg_stream_pagein(&test,_og);
  /*If we don't get a packet back, something is wrong.*/
  if(!ogg_stream_packetpeek(&test,&op)){
    ogg_stream_clear(&test);
    return;
  }
  /*Identify the codec: try Theora.*/
  th_info_init(&ti);
  theorap=th_decode_headerin(&ti,NULL,NULL,&op);
  if(theorap>=0){
    th_rehuff_ctx *rehuff;
    for(ri=0;ri<_nrehuffs;ri++){
      if(_rehuffs[ri].serialno==test.serialno)break;
    }
    if(ri>=_nrehuffs){
      fprintf(stderr,"Error: Stream headers changed after first pass.\n");
      return;
    }
    rehuff=_rehuffs+ri;
    if(!rehuff->page_ready){
      ogg_packet op;
      ogg_stream_clear(&rehuff->to);
      ogg_stream_init(&rehuff->to,rehuff->serialno);
      th_recode_flushheader(rehuff->tr,&rehuff->tc,&op);
      ogg_stream_packetin(&rehuff->tp,&op);
      ogg_stream_pageout(&rehuff->tp,&rehuff->og);
      th_rehuff_copy_page_data(rehuff);
      rehuff->page_ready=1;
      /*processing_headers now means "flush pages until we run out", to ensure
         they all appear before any data pages.
        We don't want to start doing that until all the BOS pages are written.*/
      rehuff->processing_headers=0;
    }
  }
  else{
    vorbis_info vi;
    int         pi;
    vorbis_info_init(&vi);
    vorbisp=vorbis_synthesis_headerin(&vi,NULL,&op);
    if(vorbisp>=0){
      ov_passthrough_ctx *passthrough;
      if(npassthroughs>=cpassthroughs){
        cpassthroughs=cpassthroughs<<1|1;
        passthroughs=(ov_passthrough_ctx *)_ogg_realloc(passthroughs,
         cpassthroughs*sizeof(*passthroughs));
        *_nsos=0;
        for(ri=0;ri<_nrehuffs;ri++){
          stream_ctx_add(_sos,_nsos,_csos,&TH_REHUFF_VTBL,_rehuffs+ri);
        }
        for(pi=0;pi<npassthroughs;pi++){
          stream_ctx_add(_sos,_nsos,_csos,&OV_PASSTHROUGH_VTBL,passthroughs+pi);
        }
      }
      passthrough=passthroughs+npassthroughs++;
      ov_passthrough_init(passthrough,&vi,test.serialno);
      stream_ctx_add(_sos,_nsos,_csos,&OV_PASSTHROUGH_VTBL,passthrough);
      ov_passthrough_queuepage(passthrough,_og);
    }
    else{
      fprintf(stderr,"Warning: Ignoring unknown stream with serialno 0x%08lX\n",
       test.serialno);
    }
  }
  *_passthroughs=passthroughs;
  *_npassthroughs=npassthroughs;
  *_cpassthroughs=cpassthroughs;
}


int main(int _argc,char **_argv){
  th_rehuff_ctx      *rehuffs;
  int                 nrehuffs;
  int                 crehuffs;
  ov_passthrough_ctx *passthroughs;
  int                 npassthroughs;
  int                 cpassthroughs;
  stream_ctx         *sos;
  int                 nsos;
  int                 csos;
  ogg_sync_state      oy;
  ogg_sync_state      oz;
  ogg_page            og;
  ogg_page            og0;
  ogg_page            og1;
  ogg_packet          op;
  FILE               *infile;
  FILE               *outfile;
  /*FILE               *statsin;*/
  FILE               *statsout;
  fpos_t              chain_start;
  int                 long_option_index;
  int                 have_bos0;
  int                 have_bos1;
  int                 done;
  int                 c;
  int                 ret;
  int                 ri;
#if defined(_WIN32)
  /*We need to set stdin/stdout to binary mode on windows.*/
  /*Beware the evil #ifdef.
    We avoid these where we can, but this one we cannot.
    Don't add any more, you'll probably go to hell if you do.*/
  _setmode(_fileno(stdin),_O_BINARY);
  _setmode(_fileno(stdout),_O_BINARY);
#endif
  infile=NULL;
  outfile=NULL;
  /*statsin=NULL;*/
  statsout=NULL;
  /*Process option arguments.*/
  for(;;){
    c=getopt_long(_argc,_argv,optstring,options,&long_option_index);
    if(c==EOF)break;
    switch(c){
      case 'o':{
        if(strcmp(optarg,"-")){
          outfile=fopen(optarg,"wb");
          if(outfile==NULL){
            fprintf(stderr,"Unable to open output file '%s'.\n",optarg);
            exit(1);
          }
        }
        else outfile=stdout;
      }break;
      case 's':{
        if(strcmp(optarg,"-")){
          statsout=fopen(optarg,"ab");
          if(statsout==NULL){
            fprintf(stderr,
             "Unable to open statistics output file '%s'.\n",optarg);
            exit(1);
          }
        }
        else statsout=stdout;
      }break;
      /*case 'S':{
        if(!strcmp(optarg,"-")){
          statsin=fopen(optarg,"rb");
          if(statsin==NULL){
            fprintf(stderr,
             "Unable to open statistics input file '%s'.\n",optarg);
            exit(1);
          }
        }
        else statsin=stdin;
      }break;*/
      default:usage();break;
    }
  }
  if(optind<_argc){
    infile=fopen(_argv[optind],"rb");
    if(infile==NULL){
      fprintf(stderr,"Unable to open input file '%s'.\n",_argv[optind]);
      exit(1);
    }
    optind++;
  }
  if(outfile==NULL&&optind<_argc){
    outfile=fopen(_argv[optind],"wb");
    if(outfile==NULL){
      fprintf(stderr,"Unable to open output file '%s'.\n",_argv[optind]);
      exit(1);
    }
    optind++;
  }
  if(infile==NULL&&outfile==NULL||optind<_argc)usage();
  if(fseek(infile,0,SEEK_END)==-1||fseek(infile,0,SEEK_SET)==-1){
    fprintf(stderr,"Cannot seek on input file.\n");
    exit(1);
  }
  sos=NULL;
  csos=0;
  rehuffs=NULL;
  crehuffs=0;
  passthroughs=NULL;
  cpassthroughs=0;
  /*Start up Ogg stream synchronization layer.*/
  ogg_sync_init(&oy);
  ogg_sync_init(&oz);
  have_bos0=0;
  have_bos1=0;
  /*Loop over the links of a chained file.*/
  do{
    fgetpos(infile,&chain_start);
    /*Parse the headers.*/
    nsos=0;
    nrehuffs=0;
    /*Only interested Theora streams.
      All others are ignored.*/
    for(done=0;!done;){
      /*We have a BOS page encountered during a previous chain segment.*/
      if(have_bos0){
        oc_process_bos0(&og0,&rehuffs,&nrehuffs,&crehuffs,&sos,&nsos,&csos);
        have_bos0=0;
      }
      else{
        if(!buffer_data(infile,&oy))break;
        while(ogg_sync_pageout(&oy,&og)>0){
          /*Is this a mandated initial header? If not, stop parsing.*/
          if(!ogg_page_bos(&og)){
            /*Don't leak the page; get it into the appropriate stream.*/
            queue_page(sos,nsos,&og);
            done=1;
            break;
          }
          oc_process_bos0(&og,&rehuffs,&nrehuffs,&crehuffs,&sos,&nsos,&csos);
        }
      }
    }
    for(;;){
      /*Try to read in any available headers for any streams.*/
      for(ri=0;ri<nrehuffs;ri++){
        while(rehuffs[ri].processing_headers){
          /*We're expecting more header packets.*/
          ret=ogg_stream_packetpeek(&rehuffs[ri].to,&op);
          /*We're ignoring any gaps.
            If it's a problem, we'll find it soon enough.*/
          if(ret<0)continue;
          /*No packet -> stop.*/
          if(!ret)break;
          rehuffs[ri].processing_headers=th_decode_headerin(&rehuffs[ri].ti,
           &rehuffs[ri].tc,&rehuffs[ri].ts,&op);
          if(rehuffs[ri].processing_headers<0){
            printf("Error parsing Theora stream headers; corrupt stream?\n");
            exit(1);
          }
          else if(rehuffs[ri].processing_headers>0){
            /*Advance past the successfully processed header.*/
            ogg_stream_packetout(&rehuffs[ri].to,NULL);
          }
        }
      }
      /*If all Theora streams have all their header packets, stop now so we don't
         fail if there aren't enough pages in a short stream.*/
      for(ri=0;ri<nrehuffs;ri++)if(rehuffs[ri].processing_headers)break;
      if(ri>=nrehuffs)break;
      /*The header pages/packets will arrive before anything else we care about,
         or the stream is not obeying spec.*/
      /*Demux into the appropriate stream.*/
      if(ogg_sync_pageout(&oy,&og)>0)queue_page(sos,nsos,&og);
      else{
        /*Someone needs more data.*/
        if(!buffer_data(infile,&oy)){
          fprintf(stderr,"End of file while searching for codec headers.\n");
          exit(1);
        }
      }
    }
    /*And now we have it all.
      Initialize recoders.*/
    for(ri=0;ri<nrehuffs;ri++){
      rehuffs[ri].tr=th_recode_alloc(&rehuffs[ri].ti,rehuffs[ri].ts);
      fprintf(stderr,"Ogg logical stream %lx is Theora %dx%d %.02f fps video\n"
       "Encoded frame content is %dx%d with %dx%d offset\n",
       rehuffs[ri].to.serialno,
       rehuffs[ri].ti.frame_width,rehuffs[ri].ti.frame_height,
       (double)rehuffs[ri].ti.fps_numerator/rehuffs[ri].ti.fps_denominator,
       rehuffs[ri].ti.pic_width,rehuffs[ri].ti.pic_height,
       rehuffs[ri].ti.pic_x,rehuffs[ri].ti.pic_y);
    }
    /*Queue any remaining pages from data we buffered but that did not contain
       headers.*/
    while(ogg_sync_pageout(&oy,&og)>0)queue_page(sos,nsos,&og);
    /*On to the main decode loop.*/
    for(;;){
      /*Process all the packets from all the Theora streams.*/
      for(ri=0;ri<nrehuffs;ri++){
        while(ogg_stream_packetout(&rehuffs[ri].to,&op)>0){
          th_recode_packetin(rehuffs[ri].tr,&op,NULL);
        }
      }
      if(!buffer_data(infile,&oy))break;
      while(ogg_sync_pageout(&oy,&og)>0){
        /*Stop if we encounter a new chain segment.*/
        if(ogg_page_bos(&og)){
          /*Save the page for the next chain segment to use.*/
          memcpy(&og0,&og,sizeof(og0));
          have_bos0=1;
          break;
        }
        else queue_page(sos,nsos,&og);
      }
      if(have_bos0)break;
    }
    /*Decode of this chain segment complete.*/
    for(ri=0;ri<nrehuffs;ri++){
      th_huff_code       codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];
      th_huff_code       cluster_codes[16][NTOKENS_MAX];
      int                cluster_centers[16];
      oc_frame_tok_hist *tok_hists;
      long               ntok_hists;
      oc_tok_vec        *dc_vecs;
      oc_tok_vec        *ac_vecs;
      ogg_int64_t        bits_wasted;
      ogg_int64_t        granpos;
      int                converged;
      long               nvecs;
      long               fi;
      int                pli;
      int                hgi;
      int                ci;
      /*Get the token statistics for all the frames.*/
      th_recode_ctl(rehuffs[ri].tr,TH_RECCTL_GET_TOK_NSTATS,
       &ntok_hists,sizeof(ntok_hists));
      th_recode_ctl(rehuffs[ri].tr,TH_RECCTL_GET_TOK_STATS,
       &tok_hists,sizeof(tok_hists));
      /*If the user requested output statistics, write some.*/
      if(statsout!=NULL)for(fi=0;fi<ntok_hists;fi++){
        for(pli=0;pli<2;pli++){
          for(hgi=0;hgi<5;hgi++){
            for(ci=0;ci<TH_NDCT_TOKENS;ci++){
              fprintf(statsout,"%i%c",tok_hists[fi].tok_hist[pli][hgi][ci],
               ci+1<TH_NDCT_TOKENS?' ':hgi+1<5?'\t':'\n');
            }
          }
        }
      }
      nvecs=tok_vecs_init(&dc_vecs,&ac_vecs,tok_hists,ntok_hists);
      bits_wasted=tok_vecs_fpc(dc_vecs,nvecs,NDC_TOKENS,cluster_centers);
      do{
        bits_wasted=tok_vecs_kmeans(dc_vecs,nvecs,NDC_TOKENS,cluster_codes,
         &converged);
      }
      while(!converged);
      for(ci=0;ci<16;ci++)memcpy(codes[ci],cluster_codes[ci],sizeof(codes[ci]));
      bits_wasted=tok_vecs_fpc(ac_vecs,nvecs,NAC_TOKENS,cluster_centers);
      do{
        bits_wasted=tok_vecs_kmeans(ac_vecs,nvecs,NAC_TOKENS,cluster_codes,
         &converged);
      }
      while(!converged);
      for(ci=0;ci<16;ci++){
        memcpy(codes[ci+16],cluster_codes[ci]+TH_NDCT_TOKENS*0,
         sizeof(codes[ci+16]));
        memcpy(codes[ci+32],cluster_codes[ci]+TH_NDCT_TOKENS*1,
         sizeof(codes[ci+32]));
        memcpy(codes[ci+48],cluster_codes[ci]+TH_NDCT_TOKENS*2,
         sizeof(codes[ci+48]));
        memcpy(codes[ci+64],cluster_codes[ci]+TH_NDCT_TOKENS*3,
         sizeof(codes[ci+64]));
      }
      huff_codes_print("CODES",codes);
      th_recode_ctl(rehuffs[ri].tr,TH_ENCCTL_SET_HUFFMAN_CODES,
       codes,sizeof(codes));
      /*TODO: Detect start offset in input video and correct for it.*/
      granpos=0;
      th_recode_ctl(rehuffs[ri].tr,TH_DECCTL_SET_GRANPOS,
       &granpos,sizeof(granpos));
    }
    /*Now read the chain segment a second time and rewrite the packets.*/
    fsetpos(infile,&chain_start);
    /*Parse the headers.*/
    nsos=0;
    npassthroughs=0;
    for(ri=0;ri<nrehuffs;ri++){
      stream_ctx_add(&sos,&nsos,&csos,&TH_REHUFF_VTBL,rehuffs+ri);
    }
    /*Only interested in Theora streams.
      Vorbis streams are passed through.
      All others are ignored, since we don't know how to re-mux them.*/
    for(done=0;!done;){
      /*We have a BOS page encountered during a previous chain segment.*/
      if(have_bos1){
        oc_process_bos1(&og1,rehuffs,nrehuffs,&passthroughs,
         &npassthroughs,&cpassthroughs,&sos,&nsos,&csos);
        have_bos1=0;
      }
      else{
        if(!buffer_data(infile,&oz))break;
        while(ogg_sync_pageout(&oz,&og)>0){
          /*Is this a mandated initial header? If not, stop parsing.*/
          if(!ogg_page_bos(&og)){
            /*Don't leak the page; get it into the appropriate stream.*/
            queue_page(sos,nsos,&og);
            done=1;
            break;
          }
          oc_process_bos1(&og,rehuffs,nrehuffs,&passthroughs,
           &npassthroughs,&cpassthroughs,&sos,&nsos,&csos);
        }
      }
      /*Write the new BOS page.*/
      write_pages(sos,nsos,0,outfile);
    }
    /*Queue up the rest of the Theora headers.
      The rest of the streams will write them out as they come to them.*/
    for(ri=0;ri<nrehuffs;ri++){
      ogg_packet op;
      while(th_recode_flushheader(rehuffs[ri].tr,&rehuffs[ri].tc,&op)>0){
        ogg_stream_packetin(&rehuffs[ri].tp,&op);
      }
      rehuffs[ri].processing_headers=1;
    }
    write_pages(sos,nsos,0,outfile);
    /*Main re-coding loop.*/
    for(;;){
      if(!buffer_data(infile,&oz))break;
      while(ogg_sync_pageout(&oz,&og)>0){
        /*Stop if we encounter a new chain segment.*/
        if(ogg_page_bos(&og)){
          /*Save the page for the next chain segment to use.*/
          memcpy(&og1,&og,sizeof(og1));
          have_bos1=1;
          break;
        }
        else{
          queue_page(sos,nsos,&og);
          write_pages(sos,nsos,0,outfile);
        }
      }
      if(have_bos1)break;
    }
    /*Flush out any remaining pages.*/
    write_pages(sos,nsos,1,outfile);
    /*Tear down the structures for this chain segment.*/
    for(ri=0;ri<nrehuffs;ri++)th_rehuff_clear(rehuffs+ri);
    for(ri=0;ri<npassthroughs;ri++)ov_passthrough_clear(passthroughs+ri);
  }
  while(have_bos0||!(feof(infile)||ferror(infile)));
  ogg_sync_clear(&oy);
  ogg_sync_clear(&oz);
  fclose(outfile);
  return(0);
}
