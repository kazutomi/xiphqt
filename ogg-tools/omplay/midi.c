/*****************************************************************

    This file is part of omplay.

    omplay is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    omplay is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with omplay; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This code is copyright (c) 2001 by the Xiph.org Foundation 
    (http://www.xiph.org/).
**********************************************************************/

/* midi.c
** 
** Ogg MIDI decode routines
*/

#include <stdio.h>
#include <sys/asoundlib.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include "midi.h"

int midi_header_init(midi_info_t *mi)
{
	memset(mi, 0, sizeof(midi_info_t));
       
	return 0;
}

int midi_header_clear(midi_info_t *mi)
{
	if (mi->data != NULL) free(mi->data);
	memset(mi, 0, sizeof(midi_info_t));

	return 0;
}

int midi_header_set_queue(midi_info_t *mi, int q)
{
	mi->q = q;
	return 0;
}

int midi_headerin(midi_info_t *mi, ogg_packet *op)
{
	int temp;

	// parse the packet for the OggMIDI header

	// make sure we have enough to read the version
	if (op->bytes < 9) 
		return EHEADER;
	
	if (memcmp(op->packet, "OggMIDI\000", 8) != 0) 
		return EHEADER;

	mi->version = op->packet[8] & 0xFF;
	if (mi->version != 0)
		return EVERSION;

	// version 0 has a 12 byte header; no more no less
	if (op->bytes != 12)
		return EHEADER;

	mi->smtpe = op->packet[9] & 0xFF;
	mi->ticks = (op->packet[11] << 8) | op->packet[10];
	
	// set tempo to default
	mi->tempo = 500000;

	return 0;
}

int midi_packetin(midi_info_t *mi, ogg_packet *op)
{
	if (mi->data != NULL) free(mi->data);

	mi->data = (unsigned char *)malloc(op->bytes);
	mi->size = op->bytes;
	mi->pos = 0;
	memcpy(mi->data, op->packet, mi->size);

	return 0;
}

static unsigned long _data_left(midi_info_t *mi)
{
	return (mi->size - mi->pos);
}

static int _read_byte(midi_info_t *mi)
{
	if (mi->pos >= mi->size) return -1;

	return mi->data[mi->pos++] & 0xFF;
}
	

static int64_t _read_var_value(midi_info_t *mi)
{
	unsigned long stamp = 0;
	int byte;

        do {
		byte = _read_byte(mi);
		if (byte == -1) return -1;
		stamp <<= 7;
		stamp |= byte & 0x7F;
	} while ((byte & 0x80) == 0x80);

	return stamp;
}

int midi_eventout(midi_info_t *mi, snd_seq_event_t *ev)
{
	int64_t stamp, length;
	int status, data;
	int tempo;
	int err;
	void *buf;

	if (_data_left(mi) == 0) return EDATA;

	snd_seq_ev_clear(ev);
	snd_seq_ev_set_fixed(ev);
	// We know at this point that the entire event
	// is within the packet, since it's illegal
	// to span packet boundries for events.
	// this makes our lives _so_ much easier.

	stamp = _read_var_value(mi);
	if (stamp < 0) return EEVENT;
	mi->time += stamp;
	snd_seq_ev_schedule_tick(ev, mi->q, 0, mi->time);

	status = _read_byte(mi);
	if (status < 0) return EEVENT;

	switch (status & 0xF0) {
	case 0x80:
		ev->type = SND_SEQ_EVENT_NOTEOFF;
		ev->data.note.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.note.note = data & 0x7F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.note.velocity = data & 0x7F;
		break;
	case 0x90:
		ev->type = SND_SEQ_EVENT_NOTEON;
		ev->data.note.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.note.note = data & 0x7F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.note.velocity = data & 0x7F;
		break;
	case 0xA0:
		ev->type = SND_SEQ_EVENT_KEYPRESS;
		ev->data.note.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.note.note = data & 0x7F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.note.velocity = data & 0x7F;
		break;
	case 0xB0:
		ev->type = SND_SEQ_EVENT_CONTROLLER;
		ev->data.control.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.control.param = data & 0x7F;
	        data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.control.value = data & 0x7F;
		break;
	case 0xC0:
		ev->type = SND_SEQ_EVENT_PGMCHANGE;
		ev->data.control.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.control.value = data & 0x7F;
		break;
	case 0xD0:
		ev->type = SND_SEQ_EVENT_CHANPRESS;
		ev->data.control.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0 ) return EEVENT;
		ev->data.control.value = data & 0x7F;
		break;
	case 0xE0:
		ev->type = SND_SEQ_EVENT_PITCHBEND;
		ev->data.control.channel = status & 0x0F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.control.param = data & 0x7F;
		data = _read_byte(mi);
		if (data < 0) return EEVENT;
		ev->data.control.value = data & 0x7F;
		break;
	case 0xF0:
		// these events are special
		// we should not send only the tempo changes
		// and sysex events
		switch (status & 0xFF) {
		case 0xF0:
			ev->type = SND_SEQ_EVENT_SYSEX;
			length = _read_var_value(mi);
			if (length < 1) return EEVENT;
			if (_data_left(mi) < length) return EEVENT;
			ev->data.ext.len = length;
		        buf = (void *)malloc(length);
			if (buf == NULL) return EMALLOC;
			memcpy(buf, &mi->data[mi->pos], length);
			snd_seq_ev_set_variable(ev, length, buf);
			mi->pos += length;
			break;
		case 0xFF:
			data = _read_byte(mi);
			if (data < 0) return EEVENT;

			if (data == 0x51) {
				length = _read_var_value(mi);
				if (length != 3) return EEVENT;
				if (_data_left(mi) < length) return EEVENT;
				ev->type = SND_SEQ_EVENT_TEMPO;
				ev->data.queue.queue = mi->q;
				ev->data.queue.param.value = (mi->data[mi->pos] << 16) | (mi->data[mi->pos+1] << 8) | (mi->data[mi->pos+2]);
				mi->pos += length;
			} else {
				snd_seq_ev_clear(ev);
				// skip this event
				length = _read_var_value(mi);
				if (length < 0) return EEVENT;
				if (_data_left(mi) < length) return EEVENT;
				mi->pos += length;
			}
			break;
		default:
			return EEVENT;
		}
		break;
	default:
		return EEVENT;
	}


	return 0;
}




