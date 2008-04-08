/* kate.c
**
** kate dummy packetizing module
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

#ifdef HAVE_KATE
#include <kate/kate.h>
#include <kate/oggkate.h>
#endif

#include "oggmerge.h"
#include "kate.h"

typedef struct kate_page_tag {
	oggmerge_page_t *page;
	struct kate_page_tag *next;
} kate_page_t;

typedef struct {
	ogg_sync_state oy;
	int serialno;
	ogg_int64_t old_granulepos;
	int first_page;
        int num_headers;
        unsigned long int granule_shift;
	unsigned long granule_numerator;
	unsigned long granule_denominator;
	kate_page_t *pages;
        int old_style;
} kate_state_t;

static void _add_kate_page(kate_state_t *kstate, ogg_page *og);

static unsigned long read32le(const unsigned char *bytes)
{
  unsigned long v=0;
  v|=*bytes++;
  v|=((*bytes++)<<8);
  v|=((*bytes++)<<16);
  v|=((*bytes++)<<24);
  return v;
}

int kate_state_init(oggmerge_state_t *state, int serialno, int old_style)
{
	kate_state_t *kstate;

	if (state == NULL) return 0;

	kstate = (kate_state_t *)malloc(sizeof(kate_state_t));
	if (kstate == NULL) return 0;

	ogg_sync_init(&kstate->oy);
	kstate->first_page = 1;
        kstate->num_headers = 0;
        kstate->granule_shift=0;
	kstate->granule_numerator = 0;
	kstate->granule_denominator = 0;
	kstate->old_granulepos = 0;
        kstate->pages = NULL;
        kstate->old_style = old_style;

	// NOTE: we just ignore serialno for now
	// until a future libogg supports ogg_page_set_serialno

	state->private = (void *)kstate;

	return 1;
}

int kate_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	int ret;
	char *buf;
	kate_state_t *kstate;
	ogg_page og;
	ogg_packet op;
	ogg_stream_state os;
#ifdef HAVE_KATE
	kate_info ki;
	kate_comment kc;
#endif

	kstate = (kate_state_t *)state->private;

	// stuff data in
	buf = ogg_sync_buffer(&kstate->oy, size);
	memcpy(buf, buffer, size);
	ogg_sync_wrote(&kstate->oy, size);

	// pull pages out
	while ((ret = ogg_sync_pageout(&kstate->oy, &og)) == 1) {
		if (kstate->first_page) {
			kstate->first_page = 0;
			ogg_stream_init(&os, kstate->serialno=ogg_page_serialno(&og));
			if (ogg_stream_pagein(&os, &og) < 0) {
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
			if (ogg_stream_packetout(&os, &op) != 1) {
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
#ifdef HAVE_KATE
			kate_info_init(&ki);
			kate_comment_init(&kc);
			if (kate_ogg_decode_headerin(&ki, &kc, &op) < 0) {
				kate_comment_clear(&kc);
				kate_info_clear(&ki);
				ogg_stream_clear(&os);
				return EBADHEADER;
			}
                        kstate->num_headers = ki.num_headers;
                        kstate->granule_shift = kate_granule_shift(&ki);
			kstate->granule_numerator = ki.gps_numerator;
			kstate->granule_denominator = ki.gps_denominator;
			kate_comment_clear(&kc);
			kate_info_clear(&ki);
#else
                        if (op.bytes<32) {
                                /* just enough for the data we'll extract */
				ogg_stream_clear(&os);
                                return EBADHEADER;
                        }
                        if (memcmp(op.packet,"\200kate\0\0\0\0",9)) {
				ogg_stream_clear(&os);
                                return EBADHEADER;
                        }
                        kstate->num_headers = op.packet[11];
                        kstate->granule_shift = op.packet[15];
			kstate->granule_numerator = read32le(op.packet+24);
			kstate->granule_denominator = read32le(op.packet+28);
#endif
			ogg_stream_clear(&os);
			
		}
		_add_kate_page(kstate, &og);
	}

	if (ret == 0) return EMOREDATA;
	return EOTHER;
}

oggmerge_page_t *kate_page_out(oggmerge_state_t *state)
{
	kate_state_t *kstate, *temp;
	oggmerge_page_t *page;

	kstate = (kate_state_t *)state->private;

	if (kstate->pages == NULL) return NULL;

        /* here, we don't want to make available pages with -1 granulepos,
           instead we wait till we have the next page with an set granulepos.
           returning NULL will force a further read */
        if (kstate->pages->page->timestamp<0) return NULL;

	page = kstate->pages->page;
	kstate->pages = kstate->pages->next;

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

static u_int64_t _make_timestamp(kate_state_t *kstate, ogg_int64_t granulepos)
{
	u_int64_t base, offset, stamp;
	
	if (kstate->granule_denominator == 0) return 0;
	
        base = granulepos>>kstate->granule_shift;
        offset = granulepos-(base<<kstate->granule_shift);
        stamp = (double)(base+offset) *1000000.0*kstate->granule_denominator/(double)kstate->granule_numerator;

        if (granulepos>=0) {
          kstate->old_granulepos = granulepos;
        }

	return stamp;
}

static void _add_kate_page(kate_state_t *kstate, ogg_page *og)
{
	oggmerge_page_t *page;
	kate_page_t *kpage, *temp;

        if (ogg_page_serialno(og)!=kstate->serialno) {
          fprintf(stderr,"Error: oggmerge does not support merging multiplexed streams (%x %x)\n",ogg_page_serialno(og),kstate->serialno);
          return;
        }

	// build oggmerge page
	page = (oggmerge_page_t *)malloc(sizeof(oggmerge_page_t));
	page->og = _copy_ogg_page(og);
	page->timestamp = -1;
        if (ogg_page_granulepos(og)>=0) {
            page->timestamp = _make_timestamp(kstate, ogg_page_granulepos(og));
            /* if we had queued pages with -1 timestamp (eg, we didn't know yet),
               we want to update them now to the newly known timestamp so they'll
               automatically be selected when appropriate */
            temp = kstate->pages;
            while (temp && temp->page->timestamp<0) {
                temp->page->timestamp = page->timestamp;
                temp=temp->next;
            }
        }
	
	// build kate page
	kpage = (kate_page_t *)malloc(sizeof(kate_page_t));
	if (kpage == NULL) {
		free(page->og->header);
		free(page->og->body);
		free(page->og);
		free(page);
		return;
	}

	kpage->page = page;
	kpage->next = NULL;

	// add page to state
	temp = kstate->pages;
	if (temp == NULL) {
		kstate->pages = kpage;
	} else {
		while (temp->next) temp = temp->next;
		temp->next = kpage;
	}
}

int kate_fisbone_out(oggmerge_state_t *state, ogg_packet *op)
{
    kate_state_t *kstate = (kate_state_t *)state->private;

    int gshift = kstate->granule_shift;
    ogg_int64_t gnum = kstate->granule_numerator;
    ogg_int64_t gden = kstate->granule_denominator;
    int nheaders = kstate->num_headers;
    add_fisbone_packet(op, kstate->serialno, "application/x-kate", nheaders, 0, gshift, gnum, gden);

    return 0;
}
