/* theora.c
**
** theora dummy packetizing module
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <theora/theora.h>

#include "oggmerge.h"
#include "theora.h"

typedef struct theora_page_tag {
	oggmerge_page_t *page;
	struct theora_page_tag *next;
} theora_page_t;

typedef struct {
	ogg_sync_state oy;
	int serialno;
	ogg_int64_t old_granulepos;
	int first_page;
        unsigned long granule_shift;
	unsigned long fps_numerator;
	unsigned long fps_denominator;
	theora_page_t *pages;
        int old_style;
} theora_state_t;

static void _add_theora_page(theora_state_t *tstate, ogg_page *og);


int theora_state_init(oggmerge_state_t *state, int serialno, int old_style)
{
	theora_state_t *tstate;

	if (state == NULL) return 0;

	tstate = (theora_state_t *)malloc(sizeof(theora_state_t));
	if (tstate == NULL) return 0;

	ogg_sync_init(&tstate->oy);
	tstate->first_page = 1;
        tstate->granule_shift = 0;
	tstate->fps_numerator = 0;
	tstate->fps_denominator = 0;
	tstate->old_granulepos = 0;
        tstate->pages = NULL;
        tstate->old_style = old_style;

	// NOTE: we just ignore serialno for now
	// until a future libogg supports ogg_page_set_serialno

	state->private = (void *)tstate;

	return 1;
}

int theora_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	int ret;
	char *buf;
	theora_state_t *tstate;
	ogg_page og;
	ogg_packet op;
	ogg_stream_state os;
	theora_info ti;
	theora_comment tc;

	tstate = (theora_state_t *)state->private;

	// stuff data in
	buf = ogg_sync_buffer(&tstate->oy, size);
	memcpy(buf, buffer, size);
	ogg_sync_wrote(&tstate->oy, size);

	// pull pages out
	while ((ret = ogg_sync_pageout(&tstate->oy, &og)) == 1) {
		if (tstate->first_page) {
			tstate->first_page = 0;
			ogg_stream_init(&os, tstate->serialno=ogg_page_serialno(&og));
			theora_info_init(&ti);
			theora_comment_init(&tc);
			if (ogg_stream_pagein(&os, &og) < 0) {
				theora_comment_clear(&tc);
				theora_info_clear(&ti);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			if (ogg_stream_packetout(&os, &op) != 1) {
				theora_comment_clear(&tc);
				theora_info_clear(&ti);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			if (theora_decode_header(&ti, &tc, &op) < 0) {
				theora_comment_clear(&tc);
				theora_info_clear(&ti);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			tstate->granule_shift = theora_granule_shift(&ti);
			tstate->fps_numerator = ti.fps_numerator;
			tstate->fps_denominator = ti.fps_denominator;
			theora_comment_clear(&tc);
			theora_info_clear(&ti);
			ogg_stream_clear(&os);
			
		}
		_add_theora_page(tstate, &og);
	}

	if (ret == 0) return EMOREDATA;
	return EOTHER;
}

oggmerge_page_t *theora_page_out(oggmerge_state_t *state)
{
	theora_state_t *tstate;
	oggmerge_page_t *page;

	tstate = (theora_state_t *)state->private;

	if (tstate->pages == NULL) return NULL;

	page = tstate->pages->page;
	tstate->pages = tstate->pages->next;

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

static u_int64_t _make_timestamp(theora_state_t *tstate, ogg_int64_t granulepos)
{
	u_int64_t stamp = 0;
        ogg_int64_t iframe,pframe;
        ogg_int64_t gp = tstate->old_style?tstate->old_granulepos:granulepos;
	
	if (tstate->fps_denominator == 0) return 0;
	
        iframe=gp>>tstate->granule_shift;
        pframe=gp-(iframe<<tstate->granule_shift);

        stamp = (double)(iframe+pframe) * (double)1000000 * tstate->fps_denominator / (double)tstate->fps_numerator;

        if (granulepos>=0) {
          tstate->old_granulepos = granulepos;
        }

	return stamp;
}

static void _add_theora_page(theora_state_t *tstate, ogg_page *og)
{
	oggmerge_page_t *page;
	theora_page_t *tpage, *temp;

        if (ogg_page_serialno(og)!=tstate->serialno) {
          fprintf(stderr,"Error: oggmerge does not support merging multiplexed streams\n");
          return;
        }

	// build oggmerge page
	page = (oggmerge_page_t *)malloc(sizeof(oggmerge_page_t));
	page->og = _copy_ogg_page(og);
	page->timestamp = -1;
        if (ogg_page_granulepos(og)>=0)
            page->timestamp = _make_timestamp(tstate, ogg_page_granulepos(og));
	
	// build theora page
	tpage = (theora_page_t *)malloc(sizeof(theora_page_t));
	if (tpage == NULL) {
		free(page->og->header);
		free(page->og->body);
		free(page->og);
		free(page);
		return;
	}

	tpage->page = page;
	tpage->next = NULL;

	// add page to state
	temp = tstate->pages;
	if (temp == NULL) {
		tstate->pages = tpage;
	} else {
		while (temp->next) temp = temp->next;
		temp->next = tpage;
	}
}

int theora_fisbone_out(oggmerge_state_t *state, ogg_packet *op)
{
    theora_state_t *tstate = (theora_state_t *)state->private;

    int gshift = tstate->granule_shift;
    ogg_int64_t gnum = tstate->fps_numerator;
    ogg_int64_t gden = tstate->fps_denominator;
    add_fisbone_packet(op, tstate->serialno, "video/theora", 3, 0, gshift, gnum, gden);

    return 0;
}
