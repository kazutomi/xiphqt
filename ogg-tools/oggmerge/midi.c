/* midi.c
**
** oggmerge midi module
**
*/

#include <stdio.h>
#include <ogg/ogg.h>

#include "oggmerge.h"
#include "midi.h"

typedef struct databuf_tag {
	char *data;
	unsigned long size;
	unsigned long pos;

	struct databuf_tag *next;
} databuf_t;

typedef enum {
	e_header,
	e_newpacket,
	e_newevent,
	e_timestamp,
	e_status,
	e_onebytedata,
	e_twobytedata,
	e_sysexlengthdata,
	e_sysexdata,
	e_metatypedata,
	e_metalengthdata,
	e_metadata,
	e_endofevent,
	e_done
} process_state_t;

typedef struct midi_page_tag {
	oggmerge_page_t *page;
	struct midi_page_tag *next;
} midi_page_t;

typedef struct {
	databuf_t *data;
	process_state_t ps;
	ogg_stream_state os;
	ogg_packet *op;
	unsigned long ppos;
	midi_page_t *pages;
	int serialno;
	// timing information
	int ticks;
	long tempo;
	int frames;
	int smtpe;
	// interim holders for midi state values
	u_int64_t current;
	unsigned long timestamp;
	unsigned long length;
	unsigned char metatype;
} midi_state_t;

static int _process_data(midi_state_t *midistate);

int midi_state_init(oggmerge_state_t *state, int serialno)
{
	midi_state_t *midistate;

	if (state == NULL) return 0;

	midistate = (midi_state_t *)malloc(sizeof(midi_state_t));

	if (midistate != NULL) {
		ogg_stream_init(&midistate->os, serialno);
		midistate->ps = e_header;
		midistate->data = NULL;
		midistate->op = NULL;
		midistate->pages = NULL;
		midistate->serialno = serialno;
		midistate->ticks = 480;
		midistate->tempo = 500000; // 120 bpm == 500,000 microseconds per beat
		midistate->frames = 0;
		midistate->smtpe = 0;
		midistate->current = 0;
		state->private = (void *)midistate;
		return 1;
	}

	return 0;
}

/* midi_data_in
**
** all we do here is create a new databuf_t and append it to our 
** buffer list.
**
** we make a copy of the data.
*/
int midi_data_in(oggmerge_state_t *state, char *buffer, unsigned long size)
{
	midi_state_t *midistate;
	databuf_t *databuf, *temp;

	if (state == NULL || buffer == NULL || size <= 0) return;

	midistate = (midi_state_t *)state->private;

	databuf = (databuf_t *)malloc(sizeof(databuf_t));
	if (databuf == NULL) return;

	databuf->data = (char *)malloc(size);
	databuf->size = size;
	databuf->pos = 0;
	databuf->next = NULL;

	if (databuf->data == NULL) {
		free(databuf);
		return;
	}

	// copy data
	memcpy(databuf->data, buffer, size);

	// add to list
	if (midistate->data == NULL) {
		midistate->data = databuf;
	} else {
		temp = midistate->data;
		while (temp->next != NULL) temp = temp->next;
		temp->next = databuf;
	}

	return _process_data(midistate);
}

oggmerge_page_t *midi_page_out(oggmerge_state_t *state)
{
	midi_state_t *midistate;
	oggmerge_page_t *page;

	midistate = (midi_state_t *)state->private;

	if (midistate->pages == NULL) return NULL;

	page = midistate->pages->page;
	midistate->pages = midistate->pages->next;

	return page;
}

static int _little_endian(void)
{
	unsigned short pattern = 0xbabe;
	unsigned char *bytewise = (unsigned char *)&pattern;

	if (bytewise[0] == 0xba) return 0;
	return 1;
}

static unsigned long _swap_long(unsigned long data)
{
	unsigned long ret;
	unsigned char *dataptr = (unsigned char *)&data;
	unsigned char *retptr = (unsigned char *)&ret;

	retptr[0] = dataptr[3];
	retptr[1] = dataptr[2];
	retptr[2] = dataptr[1];
	retptr[3] = dataptr[0];

	return ret;
}

static unsigned short _swap_short(unsigned short data)
{
	unsigned short ret;
	unsigned char *dataptr = (unsigned char *)&data;
	unsigned char *retptr = (unsigned char *)&ret;

	retptr[0] = dataptr[1];
	retptr[1] = dataptr[0];

	return ret;
}

static u_int64_t _data_left(midi_state_t *midistate)
{
	u_int64_t len = 0;
	databuf_t *buf;

	if (midistate->data == NULL) return 0;

	buf = midistate->data;
	while (buf) {
		len += (buf->size - buf->pos);
		buf = buf->next;
	}

	return len;
}

/* _get_data
**
** this returns a buffer of the requested size
** or NULL if there's not enough data
** this advances the data pointer too
** caller is responsible for freeing the buffer
*/
static char *_get_data(midi_state_t *midistate, unsigned long size)
{
	char *buffer, *ptr;
	unsigned long copy;
	databuf_t *databuf;

	if (_data_left(midistate) < size) return NULL;

	buffer = (char *)malloc(size);
	if (buffer == NULL) return NULL;

	databuf = midistate->data;

	ptr = buffer;
	while (size > 0) {
		copy = databuf->size - databuf->pos >= size ? size : databuf->size - databuf->pos;
		memcpy(ptr, &databuf->data[databuf->pos], copy);
		ptr += copy;
		databuf->pos += copy;
		size -= copy;

		// might need to advance to the next block
		if (databuf->pos == databuf->size) {
			midistate->data = databuf->next;
			databuf = databuf->next;
		}
	}

	return buffer;
}

static int _get_byte(midi_state_t *midistate)
{
	int value;
	databuf_t *databuf;

	if (_data_left(midistate) < 1) return -1;

	databuf = midistate->data;
	value = databuf->data[databuf->pos++];

	// may need to advance block
	if (databuf->pos == databuf->size)
		midistate->data = databuf->next;

	return value & 0xFF;
}

static int64_t _get_long(midi_state_t *midistate)
{
	int64_t value;

	if (_data_left(midistate) < 4) return -1;
	
	value = ((_get_byte(midistate) & 0xFF) << 24) | ((_get_byte(midistate) & 0xFF) << 16) | 
		((_get_byte(midistate) & 0xFF) << 8) | (_get_byte(midistate) & 0xFF);

	return value & 0xFFFFFFFF;
}

static long _get_short(midi_state_t *midistate)
{
	long value;
	
	if (_data_left(midistate) < 2) return -1;

	value = ((_get_byte(midistate) & 0xFF) << 8) | (_get_byte(midistate) & 0xFF);

	return value & 0xFFFF;
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

static oggmerge_page_t *_make_oggmerge_page(ogg_page *og, u_int64_t timestamp)
{
	oggmerge_page_t *omp;

	omp = (oggmerge_page_t *)malloc(sizeof(oggmerge_page_t));
	if (omp == NULL) return NULL;

	omp->og = _copy_ogg_page(og);
	omp->timestamp = timestamp;

	return omp;
}

/* _make_timestamp
** 
** convert native midi stamps into absolute microseconds relative
** to the beginning of the stream
*/
static u_int64_t _make_timestamp(midi_state_t *midistate, ogg_int64_t granulepos)
{
	u_int64_t timestamp;

	if (midistate->smtpe) {
		timestamp = (double)granulepos * ((double)(midistate->frames * midistate->ticks) / (double)1000000);
	} else {
		// tempo is in us/quartnernote
		timestamp = (double)granulepos * (double)midistate->tempo / (double)midistate->ticks;
	}

	return timestamp;
}

static void _add_oggmerge_page(midi_state_t *midistate, oggmerge_page_t *omp)
{
	midi_page_t *mp, *temp;
	
	mp = (midi_page_t *)malloc(sizeof(midi_page_t));
	if (mp == NULL) return;

	mp->next = NULL;
	mp->page = omp;

	temp = midistate->pages;
	if (temp == NULL) {
		midistate->pages = mp;
	} else {
		while (temp->next != NULL) temp = temp->next;
		temp->next = mp;
	}
}

static void _flush_pages(midi_state_t *midistate)
{
	ogg_page *og;
	oggmerge_page_t *omp;

	og = (ogg_page *)malloc(sizeof(ogg_page));
	if (og == NULL) return;

	while (ogg_stream_flush(&midistate->os, og)) {
		omp = _make_oggmerge_page(og, _make_timestamp(midistate, ogg_page_granulepos(og)));
		_add_oggmerge_page(midistate, omp);
	}

	free(og);
}

static void _queue_pages(midi_state_t *midistate)
{
	ogg_page *og;
	oggmerge_page_t *omp;

	og = (ogg_page *)malloc(sizeof(ogg_page));
	if (og == NULL) return;

	while (ogg_stream_pageout(&midistate->os, og)) {
		omp = _make_oggmerge_page(og, _make_timestamp(midistate, ogg_page_granulepos(og)));
		_add_oggmerge_page(midistate, omp);
	}

	free(og);
}

static void _resize_packet(midi_state_t *midistate, unsigned long size)
{
	unsigned char *packet;

	// check if the packet has been created
	if (midistate->op->packet == NULL) {
		// create new packet data
		midistate->op->packet = (unsigned char *)malloc(size);
		if (midistate->op->packet == NULL) {
			printf("ERROR!!!!\n");
			return;
		}
		midistate->ppos = 0;
		midistate->op->bytes = size;
	} else {
		// create new buffer
		packet = (unsigned char *)malloc(size);
		if (packet == NULL) return;
		// copy old data to new buffer
		memcpy(packet, midistate->op->packet, midistate->ppos <= size ? midistate->ppos : size);
		// free old buffer
		free(midistate->op->packet);
		// set new buffer
		midistate->op->packet = packet;
		midistate->op->bytes = size;
	}
}

static void _packet_write_byte(midi_state_t *midistate, unsigned char byte)
{
	if (midistate->ppos == midistate->op->bytes) {
		_resize_packet(midistate, midistate->op->bytes + 25);
	}

	midistate->op->packet[midistate->ppos++] = byte;
}

static void _packet_write_data(midi_state_t *midistate, unsigned char *data, unsigned long len)
{
	if (midistate->ppos + len >= midistate->op->bytes) {
		_resize_packet(midistate, midistate->op->bytes + len + 25);
	}

	memcpy(&midistate->op->packet[midistate->ppos], data, len);
	midistate->ppos += len;
}

static int _process_data(midi_state_t *midistate)
{
	unsigned char *buf;
	unsigned short rawtime;
	unsigned char smtpe;
	int frames;
	int ticks;
	ogg_packet *op;
	unsigned char *packet;
	unsigned char version;
	int stamp, status, length, type;

	int out_of_data = 0;
	int all_done = 0;

	while (!out_of_data && midistate->ps != e_done) {
		switch (midistate->ps) {
		case e_header:
			// make sure we have enough data to complete
			// this state
			if (_data_left(midistate) < 22) {
				out_of_data = 1;
				break;
			}
			
			// parse the header, make a packet, and stuff
			// into the stream
			buf = _get_data(midistate, 4);
			if (buf == NULL || memcmp(buf, "MThd", 4) != 0) {
				printf("_process_data(): MThd chunk not found\n");
				if (buf != NULL) free(buf);
				return EBADHEADER;
			}
			free(buf);

			// check header chunk length
			if (_get_long(midistate) != 6) return EBADHEADER;
			// check that format == 0
			if (_get_short(midistate) != 0) return EBADHEADER;
			// check that tracks == 1
			if (_get_short(midistate) != 1) return EBADHEADER;

			rawtime = _get_short(midistate) & 0xFFFF;
			smtpe = (rawtime & 0x8000) == 0x8000;
			if (smtpe) {
				frames = (rawtime & 0x7F80) >> 7;
				ticks = (rawtime & 0x007F);
				frames = -frames;
			} else {
				ticks = (rawtime & 0x7FFF);
			}
		
			// grab second chunk header
			buf = _get_data(midistate, 4);
			if (buf == NULL || memcmp(buf, "MTrk", 4) != 0) {
				printf("_process_data(): MTrk chunk not found\n");
				free(buf);
				return EBADHEADER;
			}
			free(buf);

			// waste chunk length
			_get_long(midistate);

			// create the packet
			op = (ogg_packet *)malloc(sizeof(ogg_packet));
			if (op == NULL) return EMALLOC;

			op->b_o_s = 1;
			op->e_o_s = 0;
			op->granulepos = 0;
			op->packetno = 0;
			op->bytes = 12;
			packet = (unsigned char *)malloc(12);
			if (packet == NULL) {
				free(op);
				return EMALLOC;
			}
			memcpy(packet, "OggMIDI\0", 8);
			version = 0;
			packet[8] = version;
			packet[9] = smtpe == 0 ? 0 : frames;
			memcpy(packet+10, &ticks, 2);

			op->packet = packet;

			// submit the packet
			ogg_stream_packetin(&midistate->os, op);

			// flush the header pages
			_flush_pages(midistate);

			midistate->ps = e_newpacket;

			// fall through
			// at this point everything will be raw midi events
		case e_newpacket:
			// make a new packet
			midistate->op = (ogg_packet *)malloc(sizeof(ogg_packet));
			if (midistate->op == NULL) return EMALLOC;
			memset(midistate->op, 0, sizeof(ogg_packet));
			midistate->ppos = 0;
			midistate->op->granulepos = 0;
			midistate->op->b_o_s = 0;
			midistate->op->e_o_s = 0;
			_resize_packet(midistate, 512);
			midistate->ps = e_newevent;
			// fall through
		case e_newevent:
			midistate->timestamp = 0;
			midistate->length = 0;
			midistate->ps = e_timestamp;
		case e_timestamp:
			// handle timestamp
			stamp = _get_byte(midistate);
			if (stamp < 0) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, stamp & 0xFF);
			// decode
			midistate->timestamp <<= 7;
			midistate->timestamp |= (stamp & 0x7F);
			if ((stamp & 0x80) == 0x80)
				continue;
			else 
				midistate->ps = e_status;
			// fall through
		case e_status:
			// handle status
			status = _get_byte(midistate);
			if (status < 0) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, status & 0xFF);
			switch (status & 0xF0) {
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
				midistate->ps = e_twobytedata;
				continue;
			case 0xC0:
			case 0xD0:
				midistate->ps = e_onebytedata;
				continue;
			case 0xF0:
				// test for sysex or meta
				if (status == 0xFF) {
					midistate->ps = e_metatypedata;
					continue;
				} else if (status == 0xF0) {
					midistate->ps = e_sysexlengthdata;
					continue;
				} else {
					printf("_process_data(): 1bad event of %02x\n", status);
					return EBADEVENT;
				}
			default:
				printf("_process_data(): 2bad event of %02x\n", status);
				return EBADEVENT;
			}
		case e_onebytedata:
			if (_data_left(midistate) < 1) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, _get_byte(midistate) & 0xFF);
			midistate->ps = e_endofevent;
			continue;
		case e_twobytedata:
			if (_data_left(midistate) < 2) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, _get_byte(midistate) & 0xFF);
			_packet_write_byte(midistate, _get_byte(midistate) & 0xFF);
			midistate->ps = e_endofevent;
			continue;
		case e_sysexlengthdata:
			// handle sysex length
			length = _get_byte(midistate);
			if (length < 0) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, length & 0xFF);
			// decode
			midistate->length <<= 7;
			midistate->length |= (length & 0x7F);
			if ((length & 0x80) == 0x80)
				continue;
			else 
				midistate->ps = e_sysexdata;
			// fall through		
		case e_sysexdata:
			// handle the rest of the sysex event
			if (_data_left(midistate) < length) {
				out_of_data = 1;
				continue;
			}
			buf = _get_data(midistate, length);
			if (buf == NULL) return EMALLOC;
			_packet_write_data(midistate, buf, length);
			free(buf);
			midistate->length = 0;
			midistate->ps = e_endofevent;
			continue;
		case e_metatypedata:
			// handle meta-events
			type = _get_byte(midistate);
			if (type < 1) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, type & 0xFF);
			midistate->metatype = type & 0xFF;
			midistate->ps = e_metalengthdata;
			// fall through
		case e_metalengthdata:
			// handle meta length
			length = _get_byte(midistate);
			if (length < 0) {
				out_of_data = 1;
				continue;
			}
			_packet_write_byte(midistate, length & 0xFF);
			// decode
			midistate->length <<= 7;
			midistate->length |= (length & 0x7F);
			if ((length & 0x80) == 0x80)
				continue;
			else 
				midistate->ps = e_metadata;
			// fall through
		case e_metadata:
			// handle data for meta event
			if (midistate->metatype == 0x2F && length == 0) {
				all_done = 1;
			}
			if (length == 0) {
				midistate->ps = e_endofevent;
				continue;
			}
			if (_data_left(midistate) < length) {
				out_of_data = 1;
				continue;
			}
			buf = _get_data(midistate, length);
			if (buf == NULL) return EMALLOC;
			_packet_write_data(midistate, buf, length);
			// need to catch tempo changes here
			if (midistate->metatype == 0x51) {
				// 0x51 has 0x03 length
				midistate->tempo = ((buf[0] << 16) | (buf[1] << 8) | buf[2]);
			}
			free(buf);
			midistate->length = 0;
			midistate->ps = e_endofevent;
			continue;
		case e_endofevent:
			midistate->current += midistate->timestamp;
			if (midistate->ppos >= 512 || all_done) {
				// close out this packet and start a new one
				_resize_packet(midistate, midistate->ppos);
				midistate->op->granulepos = midistate->current;
				ogg_stream_packetin(&midistate->os, midistate->op);
				midistate->op = NULL;
				_queue_pages(midistate);
				if (all_done) {
					midistate->ps = e_done;
					_flush_pages(midistate);
				} else {
					midistate->ps = e_newpacket;
				}
				continue;
			} else {
				midistate->ps = e_newevent;
				continue;
			}
		}
	}

	if (all_done) return 0;
	return EMOREDATA;
}

