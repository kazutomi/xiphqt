/* vorbis.c
**
** vorbis dummy packetizing module
*/

#include <stdio.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "oggmerge.h"
#include "vorbis.h"

typedef struct vorbis_page_tag {
	oggmerge_page_t *page;
	struct vorbis_page_tag *next;
} vorbis_page_t;

typedef struct {
	ogg_sync_state oy;
	int serialno;
	ogg_int64_t old_granulepos;
	int first_page;
	unsigned long samplerate;
	vorbis_page_t *pages;
} vorbis_state_t;

static void _add_vorbis_page(vorbis_state_t *vstate, ogg_page *og);


int vorbis_state_init(oggmerge_state_t *state, int serialno)
{
	vorbis_state_t *vstate;

	if (state == NULL) return 0;

	vstate = (vorbis_state_t *)malloc(sizeof(vorbis_state_t));
	if (vstate == NULL) return 0;

	ogg_sync_init(&vstate->oy);
	vstate->first_page = 1;
	vstate->samplerate = 0;
	vstate->old_granulepos = 0;

	// NOTE: we just ignore serialno for now
	// until a future libogg supports ogg_page_set_serialno

	state->private = (void *)vstate;

	return 1;
}

int vorbis_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	int ret;
	char *buf;
	vorbis_state_t *vstate;
	ogg_page og;
	ogg_packet op;
	ogg_stream_state os;
	vorbis_info vi;
	vorbis_comment vc;

	vstate = (vorbis_state_t *)state->private;

	// stuff data in
	buf = ogg_sync_buffer(&vstate->oy, size);
	memcpy(buf, buffer, size);
	ogg_sync_wrote(&vstate->oy, size);

	// pull pages out
	while ((ret = ogg_sync_pageout(&vstate->oy, &og)) == 1) {
		if (vstate->first_page) {
			vstate->first_page = 0;
			ogg_stream_init(&os, ogg_page_serialno(&og));
			vorbis_info_init(&vi);
			vorbis_comment_init(&vc);
			if (ogg_stream_pagein(&os, &og) < 0) {
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			if (ogg_stream_packetout(&os, &op) != 1) {
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			if (vorbis_synthesis_headerin(&vi, &vc, &op) < 0) {
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			vstate->samplerate = vi.rate;
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			ogg_stream_clear(&os);
			
		}
		_add_vorbis_page(vstate, &og);
	}

	if (ret == 0) return EMOREDATA;
	return EOTHER;
}

oggmerge_page_t *vorbis_page_out(oggmerge_state_t *state)
{
	vorbis_state_t *vstate;
	oggmerge_page_t *page;

	vstate = (vorbis_state_t *)state->private;

	if (vstate->pages == NULL) return NULL;

	page = vstate->pages->page;
	vstate->pages = vstate->pages->next;

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

static u_int64_t _make_timestamp(vorbis_state_t *vstate, ogg_int64_t granulepos)
{
	u_int64_t stamp;
	
	if (vstate->samplerate == 0) return 0;
	
	stamp = (double)vstate->old_granulepos * (double)1000000 / (double)vstate->samplerate;

	vstate->old_granulepos = granulepos;

	return stamp;
}

static void _add_vorbis_page(vorbis_state_t *vstate, ogg_page *og)
{
	oggmerge_page_t *page;
	vorbis_page_t *vpage, *temp;

	// build oggmerge page
	page = (oggmerge_page_t *)malloc(sizeof(oggmerge_page_t));
	page->og = _copy_ogg_page(og);
	page->timestamp = _make_timestamp(vstate, ogg_page_granulepos(og));
	
	// build vorbis page
	vpage = (vorbis_page_t *)malloc(sizeof(vorbis_page_t));
	if (vpage == NULL) {
		free(page->og->header);
		free(page->og->body);
		free(page->og);
		free(page);
		return;
	}

	vpage->page = page;
	vpage->next = NULL;

	// add page to state
	temp = vstate->pages;
	if (temp == NULL) {
		vstate->pages = vpage;
	} else {
		while (temp->next) temp = temp->next;
		temp->next = vpage;
	}
}














