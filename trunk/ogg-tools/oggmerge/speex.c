/* speex.c
**
** speex dummy packetizing module
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

#ifdef HAVE_SPEEX
#include <speex/speex.h>
#include <speex/speex_header.h>
#endif

#include "oggmerge.h"
#include "speex.h"

typedef struct speex_page_tag {
	oggmerge_page_t *page;
	struct speex_page_tag *next;
} speex_page_t;

typedef struct {
	ogg_sync_state oy;
	int serialno;
	ogg_int64_t old_granulepos;
	int first_page;
        int rate;
	speex_page_t *pages;
        int old_style;
} speex_state_t;

static void _add_speex_page(speex_state_t *sstate, ogg_page *og);

static unsigned long read32le(const unsigned char *bytes)
{
  unsigned long v=0;
  v|=*bytes++;
  v|=((*bytes++)<<8);
  v|=((*bytes++)<<16);
  v|=((*bytes++)<<24);
  return v;
}


int speex_state_init(oggmerge_state_t *state, int serialno, int old_style)
{
	speex_state_t *sstate;

	if (state == NULL) return 0;

	sstate = (speex_state_t *)malloc(sizeof(speex_state_t));
	if (sstate == NULL) return 0;

	ogg_sync_init(&sstate->oy);
	sstate->first_page = 1;
        sstate->rate = 0;
	sstate->old_granulepos = 0;
        sstate->pages = NULL;
        sstate->old_style = old_style;

	// NOTE: we just ignore serialno for now
	// until a future libogg supports ogg_page_set_serialno

	state->private = (void *)sstate;

	return 1;
}

int speex_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	int ret;
	char *buf;
	speex_state_t *sstate;
	ogg_page og;
	ogg_packet op;
	ogg_stream_state os;
#ifdef HAVE_SPEEX
        SpeexHeader *header;
#endif

	sstate = (speex_state_t *)state->private;

	// stuff data in
	buf = ogg_sync_buffer(&sstate->oy, size);
	memcpy(buf, buffer, size);
	ogg_sync_wrote(&sstate->oy, size);

	// pull pages out
	while ((ret = ogg_sync_pageout(&sstate->oy, &og)) == 1) {
		if (sstate->first_page) {
			sstate->first_page = 0;
			ogg_stream_init(&os, sstate->serialno=ogg_page_serialno(&og));
			if (ogg_stream_pagein(&os, &og) < 0) {
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			if (ogg_stream_packetout(&os, &op) != 1) {
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
#ifdef HAVE_SPEEX
                        header = speex_packet_to_header((char*)op.packet, op.bytes);
			if (!header) {
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
                        sstate->rate = header->rate;
#else
                        sstate->rate = read32le(op.packet+36);
#endif
			ogg_stream_clear(&os);
			
		}
		_add_speex_page(sstate, &og);
	}

	if (ret == 0) return EMOREDATA;
	return EOTHER;
}

oggmerge_page_t *speex_page_out(oggmerge_state_t *state)
{
	speex_state_t *sstate;
	oggmerge_page_t *page;

	sstate = (speex_state_t *)state->private;

	if (sstate->pages == NULL) return NULL;

        /* here, we don't want to make available pages with -1 granulepos,
           instead we wait till we have the next page with an set granulepos.
           returning NULL will force a further read */
        if (sstate->pages->page->timestamp<0) {
          return NULL;
        }

	page = sstate->pages->page;
	sstate->pages = sstate->pages->next;

	return page;
}

static ogg_page *_copy_ogg_page(ogg_page *og)
{
	ogg_page *page;

	page = (ogg_page *)malloc(sizeof(ogg_page));
	if (page == NULL) return NULL;

	page->header_len = og->header_len;
	page->body_len = og->body_len;
	page->header = (unsigned char *)malloc(page->header_len);
	if (page->header == NULL) {
		free(page);
		return NULL;
	}
	memcpy(page->header, og->header, page->header_len);
	page->body = (unsigned char *)malloc(page->body_len);
	if (page->body == NULL) {
		free(page->header);
		free(page);
		return NULL;
	}
	memcpy(page->body, og->body, page->body_len);

	return page;
}

static u_int64_t _make_timestamp(speex_state_t *sstate, ogg_int64_t granulepos)
{
	u_int64_t stamp;
        ogg_int64_t gp=sstate->old_style?sstate->old_granulepos:granulepos;
	
	if (sstate->rate == 0) return 0;
	if (gp == -1) return -1;
	
	stamp = (double)gp * (double)1000000 / (double)sstate->rate;

        if (granulepos>0) {
          sstate->old_granulepos = granulepos;
        }

	return stamp;
}

static void _add_speex_page(speex_state_t *sstate, ogg_page *og)
{
	oggmerge_page_t *page;
	speex_page_t *spage, *temp;

        if (ogg_page_serialno(og)!=sstate->serialno) {
          fprintf(stderr,"Error: oggmerge does not support merging multiplexed streams\n");
          return;
        }

	// build oggmerge page
	page = (oggmerge_page_t *)malloc(sizeof(oggmerge_page_t));
	page->og = _copy_ogg_page(og);
	page->timestamp = -1;
        if (ogg_page_granulepos(og)>=0) {
            page->timestamp = _make_timestamp(sstate, ogg_page_granulepos(og));
            /* if we had queued pages with -1 timestamp (eg, we didn't know yet),
               we want to update them now to the newly known timestamp so they'll
               automatically be selected when appropriate */
            temp = sstate->pages;
            while (temp && temp->page->timestamp<0) {
                temp->page->timestamp = page->timestamp;
                temp=temp->next;
            }
        }
	
	// build speex page
	spage = (speex_page_t *)malloc(sizeof(speex_page_t));
	if (spage == NULL) {
		free(page->og->header);
		free(page->og->body);
		free(page->og);
		free(page);
		return;
	}

	spage->page = page;
	spage->next = NULL;

	// add page to state
	temp = sstate->pages;
	if (temp == NULL) {
		sstate->pages = spage;
	} else {
		while (temp->next) temp = temp->next;
		temp->next = spage;
	}
}

int speex_fisbone_out(oggmerge_state_t *state, ogg_packet *op)
{
    speex_state_t *sstate = (speex_state_t *)state->private;

    int gshift = 0;
    ogg_int64_t gnum = sstate->rate;
    ogg_int64_t gden = 1;
#warning this seems plausible enough, but I'm not sure - ask jm
    add_fisbone_packet(op, sstate->serialno, "audio/speex", 2, 1, gshift, gnum, gden);

    return 0;
}
