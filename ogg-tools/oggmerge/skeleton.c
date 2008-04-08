/* skeleton.c
**
** skeleton dummy packetizing module
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

#include "oggmerge.h"
#include "skeleton.h"

typedef struct skeleton_page_tag {
	oggmerge_page_t *page;
	struct skeleton_page_tag *next;
} skeleton_page_t;

typedef struct {
	int serialno;
	ogg_stream_state os;
	skeleton_page_t *pages;
} skeleton_state_t;

static void _add_skeleton_page(skeleton_state_t *sstate, ogg_page *og);

int skeleton_state_init(oggmerge_state_t *state, int serialno, int old_style)
{
	skeleton_state_t *sstate;

	if (state == NULL) return 0;

	sstate = (skeleton_state_t *)malloc(sizeof(skeleton_state_t));
	if (sstate == NULL) return 0;

	ogg_stream_init(&sstate->os, sstate->serialno=serialno);

        sstate->pages = NULL;

	// NOTE: we just ignore serialno for now
	// until a future libogg supports ogg_page_set_serialno

	state->private = (void *)sstate;

	return 1;
}

int skeleton_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	return EOTHER;
}

oggmerge_page_t *skeleton_page_out(oggmerge_state_t *state)
{
	skeleton_state_t *sstate;
	oggmerge_page_t *page;

	sstate = (skeleton_state_t *)state->private;

	if (sstate->pages == NULL) return NULL;

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

static void _add_skeleton_page(skeleton_state_t *sstate, ogg_page *og)
{
	oggmerge_page_t *page;
	skeleton_page_t *spage, *temp;

	// build oggmerge page
	page = (oggmerge_page_t *)malloc(sizeof(oggmerge_page_t));
	page->og = _copy_ogg_page(og);
        page->timestamp = 0;
	
	// build skeleton page
	spage = (skeleton_page_t *)malloc(sizeof(skeleton_page_t));
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

int skeleton_packetin(oggmerge_state_t *state, ogg_packet *op, int type)
{
	skeleton_state_t *sstate;
        ogg_page og;
        int (*pager)(ogg_stream_state*,ogg_page*);

	sstate = (skeleton_state_t *)state->private;

        ogg_stream_packetin(&sstate->os, op);

        /* flush on head and tail */
        pager = type==FISBONE?&ogg_stream_pageout:&ogg_stream_flush;
        while ((*pager)(&sstate->os, &og)) {
            _add_skeleton_page(sstate, &og);
        }
}

int skeleton_fisbone_out(oggmerge_state_t *state, ogg_packet *op)
{
    /* skeleton never has a fisbone, alas poor skeleton */
    return 1;
}
