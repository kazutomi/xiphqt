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

/* midi.h
** 
** Ogg MIDI decode routines header file
*/

#ifndef __MIDI_H__
#define __MIDI_H__

#include <ogg/ogg.h>
#include <alsa/asoundlib.h>

typedef struct {
	int version;
	
	int smtpe;
	int ticks;

	u_int64_t time;

	unsigned long tempo;

	unsigned long pos;
	unsigned long size;
	unsigned char *data;

	int q;
} midi_info_t;

int midi_header_init(midi_info_t *mi);
int midi_header_clear(midi_info_t *mi);
int midi_header_set_queue(midi_info_t *mi, int q);
int midi_headerin(midi_info_t *mi, ogg_packet *op);
int midi_packetin(midi_info_t *mi, ogg_packet *op);
int midi_eventout(midi_info_t *mi, snd_seq_event_t *ev);

#define EDATA -1
#define EHEADER -2
#define EVERSION -3
#define EEVENT -4
#define EMALLOC -5

#endif  /* __MIDI_H__ */



