#!/usr/bin/python

import sre
import string
from pprint import pprint
import vorbies

declarations = """\
extern int      ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op);
extern int      ogg_stream_pageout(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_flush(ogg_stream_state *os, ogg_page *og);

extern int      ogg_sync_init(ogg_sync_state *oy);
extern int      ogg_sync_clear(ogg_sync_state *oy);
extern int      ogg_sync_reset(ogg_sync_state *oy);
extern int	ogg_sync_destroy(ogg_sync_state *oy);

extern char    *ogg_sync_buffer(ogg_sync_state *oy, long size);
extern int      ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern long     ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
extern int      ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
extern int      ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);

extern int      ogg_stream_init(ogg_stream_state *os,int serialno);
extern int      ogg_stream_clear(ogg_stream_state *os);
extern int      ogg_stream_reset(ogg_stream_state *os);
extern int      ogg_stream_reset_serialno(ogg_stream_state *os,int serialno);
extern int      ogg_stream_destroy(ogg_stream_state *os);
extern int      ogg_stream_eos(ogg_stream_state *os);

extern void     ogg_page_checksum_set(ogg_page *og);

extern int      ogg_page_version(ogg_page *og);
extern int      ogg_page_continued(ogg_page *og);
extern int      ogg_page_bos(ogg_page *og);
extern int      ogg_page_eos(ogg_page *og);
extern ogg_int64_t  ogg_page_granulepos(ogg_page *og);
extern int      ogg_page_serialno(ogg_page *og);
extern long     ogg_page_pageno(ogg_page *og);
extern int      ogg_page_packets(ogg_page *og);

extern void     ogg_packet_clear(ogg_packet *op);

extern const char *theora_version_string    (void);
extern ogg_uint32_t theora_version_number   (void);

extern int theora_encode_init       (theora_state *th, theora_info *c);

extern int theora_encode_YUVin      (theora_state *t, yuv_buffer *yuv);
extern int theora_encode_packetout  (theora_state *t, int last_p, ogg_packet *op);

extern int theora_encode_header     (theora_state *t, ogg_packet *op);
extern int theora_encode_comment    (theora_comment *tc, ogg_packet *op);
extern int theora_encode_tables     (theora_state *t, ogg_packet *op);

extern int theora_decode_header     (theora_info  *ci, theora_comment *cc, ogg_packet *op);

extern int theora_decode_init       (theora_state *th, theora_info *c);

extern int theora_decode_packetin   (theora_state *th, ogg_packet *op);
extern int theora_decode_YUVout     (theora_state *th, yuv_buffer *yuv);

extern double theora_granule_time   (theora_state *th, ogg_int64_t granulepos);

extern void theora_info_init        (theora_info *c);
extern void theora_info_clear       (theora_info *c);
extern void theora_clear            (theora_state *t);

extern void theora_comment_init     (theora_comment *tc);
extern void theora_comment_add      (theora_comment *tc, char *comment);
extern void theora_comment_add_tag  (theora_comment *tc, char *tag, char *value);

extern char *theora_comment_query       (theora_comment *tc, char *tag, int count);
extern int   theora_comment_query_count (theora_comment *tc, char *tag);
extern void  theora_comment_clear       (theora_comment *tc);

extern void  oggpack_writeinit(oggpack_buffer *b);
extern void  oggpack_writetrunc(oggpack_buffer *b,long bits);
extern void  oggpack_writealign(oggpack_buffer *b);
extern void  oggpack_writecopy(oggpack_buffer *b,void *source,long bits);
extern void  oggpack_reset(oggpack_buffer *b);
extern void  oggpack_writeclear(oggpack_buffer *b);
extern void  oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);
extern void  oggpack_write(oggpack_buffer *b,unsigned long value,int bits);
extern long  oggpack_look(oggpack_buffer *b,int bits);
extern long  oggpack_look1(oggpack_buffer *b);
extern void  oggpack_adv(oggpack_buffer *b,int bits);
extern void  oggpack_adv1(oggpack_buffer *b);
extern long  oggpack_read(oggpack_buffer *b,int bits);
extern long  oggpack_read1(oggpack_buffer *b);
extern long  oggpack_bytes(oggpack_buffer *b);
extern long  oggpack_bits(oggpack_buffer *b);
extern unsigned char *oggpack_get_buffer(oggpack_buffer *b);
"""

# declarations = vorbies.vorbis

if __name__ == "__main__":
    pre = sre.compile("\s+|(\()|(,)|;|(\))")

    parse0 = []

    for line in [x for x in declarations.splitlines() if x.strip()]:
        items =  pre.split(line)
        items = [i for i in items if i]
        items = [i.strip() for i in items if i.strip() and i != "extern"]
        print items
        parse0.append(items)

    pprint(parse0)

    parse1 = [(item[item.index('(')+1], item) for item in parse0]
    parse1.sort()
    pprint(parse1)
    parse2 = [item[1] for item in parse1]
    parse2 = [(item[0:1] + item[1:] + [" { return "] + item[1:] + [" };"]) for item in parse2]
    parse3 = [string.join(item, ' ') for item in parse2]

    for line in parse3:
        print line
